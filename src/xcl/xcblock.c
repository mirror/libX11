/* Copyright (C) 2003-2004 Jamey Sharp.
 * This file is licensed under the MIT license. See the file COPYING. */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#if HAVE_FEATURES_H
#define _GNU_SOURCE /* for PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP */
#include <features.h>
#endif

#include "Xlibint.h"
#include "locking.h"
#include "xclint.h"
#include <X11/XCB/xcbext.h>
#include <X11/XCB/xcbxlib.h>

#include <pthread.h>

static void _XLockDisplay(Display *dpy)
{
    pthread_mutex_lock(XCBGetIOLock(dpy->xcl->connection));
    _XGetXCBBufferIf(dpy, _XBufferUnlocked);
    ++dpy->xcl->lock_count;
}

void XLockDisplay(Display* dpy)
{
    LockDisplay(dpy);
    /* We want the threads in the reply queue to all get out before
     * XLockDisplay returns, in case they have any side effects the
     * caller of XLockDisplay was trying to protect against.
     * XLockDisplay puts itself at the head of the event waiters queue
     * to wait for all the replies to come in.
     * TODO: Restore this behavior on XCB.
     */
}

static void _XUnlockDisplay(Display *dpy)
{
    --dpy->xcl->lock_count;
    _XPutXCBBufferIf(dpy, _XBufferUnlocked);

    /* If we're unlocking all the way, make sure that our deferred
     * invariants hold. */
    if(!dpy->xcl->lock_count)
    {
	assert(XCBGetRequestSent(dpy->xcl->connection) == dpy->request);

	/* Traditional Xlib does this in _XSend; see the Xlib/XCB version
	 * of that function for why we do it here instead. */
	_XSetSeqSyncFunction(dpy);
    }

    pthread_mutex_unlock(XCBGetIOLock(dpy->xcl->connection));
}

void XUnlockDisplay(Display* dpy)
{
    UnlockDisplay(dpy);
}

/* returns 0 if initialized ok, -1 if unable to allocate
   a mutex or other memory */
int _XInitDisplayLock(Display *dpy)
{
#ifdef PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP
    pthread_mutex_t lock = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;
    *XCBGetIOLock(dpy->xcl->connection) = lock;
#else
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(XCBGetIOLock(dpy->xcl->connection), &attr);
    pthread_mutexattr_destroy(&attr);
#endif

    dpy->lock_fns = (struct _XLockPtrs*)Xmalloc(sizeof(struct _XLockPtrs));
    if (dpy->lock_fns == NULL)
	return -1;

    dpy->lock = 0;
    dpy->lock_fns->lock_display = _XLockDisplay;
    dpy->lock_fns->unlock_display = _XUnlockDisplay;

    return 0;
}

void _XFreeDisplayLock(Display *dpy)
{
    assert(dpy->lock == NULL);
    if (dpy->lock_fns != NULL) {
	Xfree((char *)dpy->lock_fns);
	dpy->lock_fns = NULL;
    }
}

static void call_handlers(Display *dpy, XCBGenericRep *buf)
{
	_XAsyncHandler *async, *next;
	_XSetLastRequestRead(dpy, (xGenericReply *) buf);
	for(async = dpy->async_handlers; async; async = next)
	{
		next = async->next;
		if(async->handler(dpy, (xReply *) buf, (char *) buf, sizeof(xReply) + (buf->length << 2), async->data))
			return;
	}
	if(buf->response_type == 0) /* unhandled error */
	    _XError(dpy, (xError *) buf);
}

void _XGetXCBBuffer(Display *dpy)
{
    static const xReq dummy_request;

    XCBConnection *c = dpy->xcl->connection;

    dpy->last_req = (char *) &dummy_request;

    dpy->request = XCBGetRequestSent(c);

    while(dpy->xcl->pending_requests)
    {
	XCBGenericRep *reply;
	XCBGenericError *error;
	PendingRequest *req = dpy->xcl->pending_requests;
	/* If this request hasn't been read off the wire yet, save the
	 * rest for later. */
	if((signed int) (XCBGetQueuedRequestRead(c) - req->sequence) <= 0)
	    break;
	dpy->xcl->pending_requests = req->next;
	/* This can't block due to the above test, but it could "fail"
	 * by returning null for any of several different reasons. We
	 * don't care. In any failure cases, we must not have wanted
	 * an entry in the reply queue for this request after all. */
	reply = XCBWaitForReply(c, req->sequence, &error);
	free(req);
	if(!reply)
	    reply = (XCBGenericRep *) error;
	if(reply)
	    call_handlers(dpy, reply);
	free(reply);
    }
    if(!dpy->xcl->pending_requests)
	dpy->xcl->pending_requests_tail = &dpy->xcl->pending_requests;

    dpy->last_request_read = XCBGetQueuedRequestRead(c);
    assert_sequence_less(dpy->last_request_read, dpy->request);
}

static inline int issue_complete_request(Display *dpy, int veclen, struct iovec *vec)
{
    XCBProtocolRequest xcb_req = { 0 };
    unsigned int sequence;
    int bigreq = 0;
    int i;
    CARD32 len;
    size_t rem;

    /* skip empty iovecs. if no iovecs remain, we're done. */
    while(veclen > 0 && vec[0].iov_len == 0)
	--veclen, ++vec;
    if(!veclen)
	return 0;

    /* we have at least part of a request. dig out the length field.
     * note that length fields are always in vec[0]: Xlib doesn't split
     * fixed-length request parts. */
    len = ((CARD16 *) vec[0].iov_base)[1];
    if(len == 0)
    {
	/* it's a bigrequest. dig out the *real* length field. */
	len = ((CARD32 *) vec[0].iov_base)[1];
	bigreq = 1;
    }

    /* do we have enough data for a complete request? how many iovec
     * elements does it span? */
    for(i = 0; i < veclen; ++i)
    {
	CARD32 oldlen = len;
	len -= (vec[i].iov_len + 3) >> 2;
	/* if len is now 0 or has wrapped, we have enough data. */
	if((len - 1) > oldlen)
	    break;
    }
    if(i == veclen)
	return 0;

    /* we have enough data to issue one complete request. the remaining
     * code can't fail. */

    /* len says how far we overshot our data needs in 4-byte units.
     * (it's negative if we actually overshot, or 0 if we're right on.)
     * rem is overshoot in 1-byte units, so it needs to have trailing
     * padding subtracted off if we're not using that padding in this
     * request. */
    if(len)
	rem = (-len << 2) - (-vec[i].iov_len & 3);
    else
	rem = 0;
    vec[i].iov_len -= rem;
    xcb_req.count = i + 1;
    xcb_req.opcode = ((CARD8 *) vec[0].iov_base)[0];

    /* undo bigrequest formatting. XCBSendRequest will redo it. */
    if(bigreq)
    {
	CARD32 *p = vec[0].iov_base;
	p[1] = p[0];
	vec[0].iov_base = (char *) vec[0].iov_base + 4;
	vec[0].iov_len -= 4;
    }

    /* send the accumulated request. */
    XCBSendRequest(dpy->xcl->connection, &sequence, vec, &xcb_req);

    /* update the iovecs to refer only to data not yet sent. */
    vec[i].iov_base = (char *) vec[i].iov_base + vec[i].iov_len;
    vec[i].iov_len = rem;
    while(--i >= 0)
	vec[i].iov_len = 0;

    /* For requests we issue, we need to get back replies and
     * errors. That's true even if we don't own the event queue, and
     * also if there are async handlers registered. If we do own the
     * event queue then errors can be handled elsewhere more
     * cheaply; and if there aren't any async handlers (but the
     * pure-Xlib code was correct) then there won't be any replies
     * so we needn't look for them. */
    if(dpy->xcl->event_owner != XlibOwnsEventQueue || dpy->async_handlers)
    {
	PendingRequest *req = malloc(sizeof(PendingRequest));
	assert(req);
	req->next = 0;
	req->sequence = sequence;
	*dpy->xcl->pending_requests_tail = req;
	dpy->xcl->pending_requests_tail = &req->next;
    }
    return 1;
}

void _XPutXCBBuffer(Display *dpy)
{
    XCBConnection *c = dpy->xcl->connection;
    _XExtension *ext;
    struct iovec iov[2];

    assert_sequence_less(dpy->last_request_read, dpy->request);
    assert_sequence_less(XCBGetRequestSent(c), dpy->request);

    for(ext = dpy->flushes; ext; ext = ext->next_flush)
    {
	ext->before_flush(dpy, &ext->codes, dpy->buffer, dpy->bufptr - dpy->buffer);
	if(dpy->xcl->request_extra)
	{
	    static char const pad[3];
	    int padsize = -dpy->xcl->request_extra_size & 3;
	    ext->before_flush(dpy, &ext->codes, dpy->xcl->request_extra, dpy->xcl->request_extra_size);
	    if(padsize)
		ext->before_flush(dpy, &ext->codes, pad, padsize);
	}
    }

    iov[0].iov_base = dpy->buffer;
    iov[0].iov_len = dpy->bufptr - dpy->buffer;
    iov[1].iov_base = (caddr_t) dpy->xcl->request_extra;
    iov[1].iov_len = dpy->xcl->request_extra_size;

    while(issue_complete_request(dpy, 2, iov))
	/* empty */;

    assert(iov[0].iov_len == 0 && iov[1].iov_len == 0);

    dpy->xcl->request_extra = 0;
    dpy->xcl->request_extra_size = 0;
    dpy->bufptr = dpy->buffer;
}

/*  */

void _XGetXCBBufferIf(Display *dpy, enum _XBufferCondition locked)
{
    if((dpy->xcl->lock_count > 0) == locked)
	_XGetXCBBuffer(dpy);
}

void _XPutXCBBufferIf(Display *dpy, enum _XBufferCondition locked)
{
    if((dpy->xcl->lock_count > 0) == locked)
	_XPutXCBBuffer(dpy);
}
