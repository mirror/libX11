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
    pthread_mutex_lock(xcb_get_io_lock(dpy->xcl->connection));
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
	assert(dpy->xcl->partial_request == 0);
	assert(xcb_get_request_sent(dpy->xcl->connection) == dpy->request);

	/* Traditional Xlib does this in _XSend; see the Xlib/XCB version
	 * of that function for why we do it here instead. */
	_XSetSeqSyncFunction(dpy);
    }

    pthread_mutex_unlock(xcb_get_io_lock(dpy->xcl->connection));
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
    *xcb_get_io_lock(dpy->xcl->connection) = lock;
#else
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(xcb_get_io_lock(dpy->xcl->connection), &attr);
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

static void call_handlers(Display *dpy, xcb_generic_reply_t *buf)
{
	_XAsyncHandler *async, *next;
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
    unsigned int xcb_req;
    void *reply;
    xcb_generic_error_t *error;
    PendingRequest *req;

    xcb_connection_t *c = dpy->xcl->connection;

    dpy->last_req = (char *) &dummy_request;

    xcb_req = xcb_get_request_sent(c);
    /* if Xlib has a partial request pending then XCB doesn't know about
     * the current request yet */
    if(dpy->xcl->partial_request)
	++xcb_req;

    assert(XCB_SEQUENCE_COMPARE(xcb_req, >=, dpy->request));
    dpy->request = xcb_req;

    while((req = dpy->xcl->pending_requests)
	  && dpy->request != req->sequence
	  && xcb_poll_for_reply(c, req->sequence, &reply, &error))
    {
	dpy->xcl->pending_requests = req->next;
	if(!reply)
	    reply = error;
	if(reply)
	{
	    dpy->last_request_read = req->sequence;
	    call_handlers(dpy, reply);
	}
	free(req);
	free(reply);
    }
    if(!dpy->xcl->pending_requests)
	dpy->xcl->pending_requests_tail = &dpy->xcl->pending_requests;

    assert_sequence_less(dpy->last_request_read, dpy->request);
}

static size_t request_length(struct iovec *vec)
{
    /* we have at least part of a request. dig out the length field.
     * note that length fields are always in vec[0]: Xlib doesn't split
     * fixed-length request parts. */
    size_t len;
    assert(vec[0].iov_len >= 4);
    len = ((uint16_t *) vec[0].iov_base)[1];
    if(len == 0)
    {
	/* it's a bigrequest. dig out the *real* length field. */
	assert(vec[0].iov_len >= 8);
	len = ((uint32_t *) vec[0].iov_base)[1];
    }
    return len << 2;
}

static inline int issue_complete_request(Display *dpy, int veclen, struct iovec *vec)
{
    xcb_protocol_request_t xcb_req = { 0 };
    unsigned int sequence;
    int flags = XCB_REQUEST_RAW;
    int i;
    size_t len;

    /* skip empty iovecs. if no iovecs remain, we're done. */
    assert(veclen >= 0);
    while(veclen > 0 && vec[0].iov_len == 0)
	--veclen, ++vec;
    if(!veclen)
	return 0;

    len = request_length(vec);

    /* do we have enough data for a complete request? how many iovec
     * elements does it span? */
    for(i = 0; i < veclen; ++i)
    {
	size_t oldlen = len;
	len -= vec[i].iov_len;
	/* if len is now 0 or has wrapped, we have enough data. */
	if((len - 1) > oldlen)
	    break;
    }
    if(i == veclen)
	return 0;

    /* we have enough data to issue one complete request. the remaining
     * code can't fail. */

    /* len says how far we overshot our data needs. (it's "negative" if
     * we actually overshot, or 0 if we're right on.) */
    vec[i].iov_len += len;
    xcb_req.count = i + 1;
    xcb_req.opcode = ((uint8_t *) vec[0].iov_base)[0];

    /* if we don't own the event queue, we have to ask XCB to set our
     * errors aside for us. */
    if(dpy->xcl->event_owner != XlibOwnsEventQueue)
	flags |= XCB_REQUEST_CHECKED;

    /* XCB will always skip request 0; account for that in the Xlib count */
    if (xcb_get_request_sent(dpy->xcl->connection) == 0xffffffff)
	dpy->request++;
    /* send the accumulated request. */
    sequence = xcb_send_request(dpy->xcl->connection, flags, vec, &xcb_req);
    if(!sequence)
	_XIOError(dpy);

    /* update the iovecs to refer only to data not yet sent. */
    vec[i].iov_len = -len;

    /* iff we asked XCB to set aside errors, we must pick those up
     * eventually. iff there are async handlers, we may have just
     * issued requests that will generate replies. in either case,
     * we need to remember to check later. */
    if(flags & XCB_REQUEST_CHECKED || dpy->async_handlers)
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
    static char const pad[3];
    const int padsize = -dpy->xcl->request_extra_size & 3;
    xcb_connection_t *c = dpy->xcl->connection;
    _XExtension *ext;
    struct iovec iov[6];

    assert_sequence_less(dpy->last_request_read, dpy->request);
    assert_sequence_less(xcb_get_request_sent(c), dpy->request);

    for(ext = dpy->flushes; ext; ext = ext->next_flush)
    {
	ext->before_flush(dpy, &ext->codes, dpy->buffer, dpy->bufptr - dpy->buffer);
	if(dpy->xcl->request_extra)
	{
	    ext->before_flush(dpy, &ext->codes, dpy->xcl->request_extra, dpy->xcl->request_extra_size);
	    if(padsize)
		ext->before_flush(dpy, &ext->codes, pad, padsize);
	}
    }

    iov[2].iov_base = dpy->xcl->partial_request;
    iov[2].iov_len = dpy->xcl->partial_request_offset;
    iov[3].iov_base = dpy->buffer;
    iov[3].iov_len = dpy->bufptr - dpy->buffer;
    iov[4].iov_base = (caddr_t) dpy->xcl->request_extra;
    iov[4].iov_len = dpy->xcl->request_extra_size;
    iov[5].iov_base = (caddr_t) pad;
    iov[5].iov_len = padsize;

    while(issue_complete_request(dpy, 4, iov + 2))
	/* empty */;

    /* first discard any completed partial_request. */
    if(iov[2].iov_len == 0 && dpy->xcl->partial_request)
    {
	free(dpy->xcl->partial_request);
	dpy->xcl->partial_request = 0;
	dpy->xcl->partial_request_offset = 0;
    }

    /* is there anything to copy into partial_request? */
    if(iov[3].iov_len != 0 || iov[4].iov_len != 0 || iov[5].iov_len != 0)
    {
	int i;
	if(!dpy->xcl->partial_request)
	{
	    size_t len = request_length(iov + 3);
	    assert(!dpy->xcl->partial_request_offset);
	    dpy->xcl->partial_request = malloc(len);
	    assert(dpy->xcl->partial_request);
	}
	for(i = 3; i < sizeof(iov) / sizeof(*iov); ++i)
	{
	    memcpy(dpy->xcl->partial_request + dpy->xcl->partial_request_offset, iov[i].iov_base, iov[i].iov_len);
	    dpy->xcl->partial_request_offset += iov[i].iov_len;
	}
    }

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
