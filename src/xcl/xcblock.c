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
    pthread_mutex_lock(XCBGetIOLock(XCBConnectionOfDisplay(dpy)));
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
    pthread_mutex_unlock(XCBGetIOLock(XCBConnectionOfDisplay(dpy)));
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
    *XCBGetIOLock(XCBConnectionOfDisplay(dpy)) = lock;
#else
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(XCBGetIOLock(XCBConnectionOfDisplay(dpy)), &attr);
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

    XCBConnection *c = XCBConnectionOfDisplay(dpy);

    dpy->last_req = (char *) &dummy_request;

    dpy->request = XCBGetRequestSent(c);

    while(dpy->xcl->pending_requests)
    {
	XCBGenericRep *reply;
	XCBGenericError *error;
	PendingRequest *req = dpy->xcl->pending_requests;
	/* If this request hasn't been read off the wire yet, save the
	 * rest for later. */
	if((signed int) (XCBGetRequestRead(c) - req->sequence) <= 0)
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

    dpy->last_request_read = XCBGetRequestRead(c);
    assert_sequence_less(dpy->last_request_read, dpy->request);
}

static void _XBeforeFlush(Display *dpy, struct iovec *iov)
{
	static char const pad[3];

	_XExtension *ext;
	for (ext = dpy->flushes; ext; ext = ext->next_flush) {
		ext->before_flush(dpy, &ext->codes, iov->iov_base, iov->iov_len);
		if((iov->iov_len & 3) != 0)
			ext->before_flush(dpy, &ext->codes, pad, XCL_PAD(iov->iov_len));
	}
}

void _XPutXCBBuffer(Display *dpy)
{
    XCBConnection *c = XCBConnectionOfDisplay(dpy);
    XCBProtocolRequest xcb_req = { /* count */ 1 };
    char *bufptr = dpy->buffer;
    PendingRequest ***req = &dpy->xcl->pending_requests_tail;

    assert_sequence_less(dpy->last_request_read, dpy->request);
    assert_sequence_less(XCBGetRequestSent(c), dpy->request);

    while(bufptr < dpy->bufptr)
    {
	struct iovec iov[2];
	int i;
	CARD32 len = ((CARD16 *) bufptr)[1];
	if(len == 0)
	    len = ((CARD32 *) bufptr)[1];

	xcb_req.opcode = bufptr[0];
	iov[0].iov_base = (caddr_t) bufptr;
	iov[0].iov_len = len << 2;

	/* if we have extra request data and this is the last request
	 * in the buffer, send the extra data along with this request. */
	if(bufptr == dpy->last_req && dpy->xcl->request_extra && dpy->xcl->request_extra_size)
	{
	    xcb_req.count = 2;
	    iov[1].iov_base = (void *) dpy->xcl->request_extra;
	    iov[1].iov_len = dpy->xcl->request_extra_size;
	    iov[0].iov_len -= iov[1].iov_len;
	    iov[0].iov_len &= ~3;
	    dpy->xcl->request_extra = 0;
	    assert(bufptr + iov[0].iov_len == dpy->bufptr);
	}

	bufptr += iov[0].iov_len;
	assert(bufptr <= dpy->bufptr);

	for(i = 0; i < xcb_req.count; ++i)
	    _XBeforeFlush(dpy, &iov[i]);

	/* For requests we issue, we need to get back replies and
	 * errors. That's true even if we don't own the event queue, and
	 * also if there are async handlers registered. If we do own the
	 * event queue then errors can be handled elsewhere more
	 * cheaply; and if there aren't any async handlers (but the
	 * pure-Xlib code was correct) then there won't be any replies
	 * so we needn't look for them. */
	if(dpy->xcl->event_owner != XlibOwnsEventQueue || dpy->async_handlers)
	{
	    **req = malloc(sizeof(PendingRequest));
	    assert(**req);
	    (**req)->next = 0;
	    XCBSendRequest(c, &(**req)->sequence, iov, &xcb_req);
	    *req = &(**req)->next;
	}
	else
	{
	    unsigned int dummy;
	    XCBSendRequest(c, &dummy, iov, &xcb_req);
	}
    }
    assert(XCBGetRequestSent(c) == dpy->request);
    dpy->bufptr = dpy->buffer;

    /* Traditional Xlib does this in _XFlush; see the Xlib/XCB version
     * of that function for why we do it here instead. */
    _XSetSeqSyncFunction(dpy);
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
