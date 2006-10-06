/* Copyright (C) 2003-2004 Jamey Sharp.
 * This file is licensed under the MIT license. See the file COPYING. */

#include "Xlibint.h"
#include "xclint.h"
#include <xcb/xcbext.h>
#include <xcb/xcbxlib.h>

#include <assert.h>
#include <stdlib.h>
#include <string.h>

/* Call internal connection callbacks for any fds that are currently
 * ready to read. This function will not block unless one of the
 * callbacks blocks.
 *
 * This code borrowed from _XWaitForReadable. Inverse call tree:
 * _XRead
 *  _XWaitForWritable
 *   _XFlush
 *   _XSend
 *  _XEventsQueued
 *  _XReadEvents
 *  _XRead[0-9]+
 *   _XAllocIDs
 *  _XReply
 *  _XEatData
 * _XReadPad
 */
static void check_internal_connections(Display *dpy)
{
	struct _XConnectionInfo *ilist;  
	fd_set r_mask;
	struct timeval tv;
	int result;
	int highest_fd = -1;

	if(dpy->flags & XlibDisplayProcConni || !dpy->im_fd_info)
		return;

	FD_ZERO(&r_mask);
	for(ilist = dpy->im_fd_info; ilist; ilist = ilist->next)
	{
		assert(ilist->fd >= 0);
		FD_SET(ilist->fd, &r_mask);
		if(ilist->fd > highest_fd)
			highest_fd = ilist->fd;
	}
	assert(highest_fd >= 0);

	tv.tv_sec = 0;
	tv.tv_usec = 0;
	result = select(highest_fd + 1, &r_mask, NULL, NULL, &tv);

	if(result == -1)
	{
		if(errno == EINTR)
			return;
		_XIOError(dpy);
	}

	for(ilist = dpy->im_fd_info; result && ilist; ilist = ilist->next)
		if(FD_ISSET(ilist->fd, &r_mask))
		{
			_XProcessInternalConnection(dpy, ilist);
			--result;
		}
}

static void handle_event(Display *dpy, xcb_generic_event_t *e)
{
	if(!e)
		_XIOError(dpy);
	dpy->last_request_read = e->full_sequence;
	if(e->response_type == X_Error)
		_XError(dpy, (xError *) e);
	else
		_XEnq(dpy, (xEvent *) e);
	free(e);
}

int _XEventsQueued(Display *dpy, int mode)
{
	xcb_connection_t *c;
	xcb_generic_event_t *e;
	int ret;
	if(dpy->xcl->event_owner != XlibOwnsEventQueue)
		return 0;

	c = dpy->xcl->connection;
	if(mode == QueuedAfterFlush)
		_XSend(dpy, 0, 0);
	else
		check_internal_connections(dpy);
	while((e = xcb_poll_for_event(c, &ret)))
		handle_event(dpy, e);
	if(ret)
		_XIOError(dpy);
	return dpy->qlen;
}

/* _XReadEvents - Flush the output queue,
 * then read as many events as possible (but at least 1) and enqueue them
 */
void _XReadEvents(Display *dpy)
{
	_XSend(dpy, 0, 0);
	if(dpy->xcl->event_owner != XlibOwnsEventQueue)
		return;
	handle_event(dpy, xcb_wait_for_event(dpy->xcl->connection));
	_XEventsQueued(dpy, QueuedAfterReading);
}

/*
 * _XSend - Flush the buffer and send the client data. 32 bit word aligned
 * transmission is used, if size is not 0 mod 4, extra bytes are transmitted.
 *
 * Note that the connection must not be read from once the data currently
 * in the buffer has been written.
 */
void _XSend(Display *dpy, const char *data, long size)
{
	xcb_connection_t *c = dpy->xcl->connection;

	assert(!dpy->xcl->request_extra);
	dpy->xcl->request_extra = data;
	dpy->xcl->request_extra_size = size;

	/* give dpy->buffer to XCB */
	_XPutXCBBuffer(dpy);

	if(xcb_flush(c) <= 0)
		_XIOError(dpy);

	/* get a new dpy->buffer */
	_XGetXCBBuffer(dpy);

	check_internal_connections(dpy);

	/* A straight port of XlibInt.c would call _XSetSeqSyncFunction
	 * here. However that does no good: unlike traditional Xlib,
	 * Xlib/XCB almost never calls _XFlush because _XPutXCBBuffer
	 * automatically pushes requests down into XCB, so Xlib's buffer
	 * is empty most of the time. Since setting a synchandler has no
	 * effect until after UnlockDisplay returns, we may as well do
	 * the check in _XUnlockDisplay. */
}

/*
 * _XFlush - Flush the X request buffer.  If the buffer is empty, no
 * action is taken.
 */
void _XFlush(Display *dpy)
{
	_XSend(dpy, 0, 0);

	_XEventsQueued(dpy, QueuedAfterReading);
}

static int
_XIDHandler(Display *dpy)
{
	XID next = xcb_generate_id(dpy->xcl->connection);
	LockDisplay(dpy);
	dpy->xcl->next_xid = next;
	if(dpy->flags & XlibDisplayPrivSync)
	{
		dpy->synchandler = dpy->savedsynchandler;
		dpy->flags &= ~XlibDisplayPrivSync;
	}
	UnlockDisplay(dpy);
	SyncHandle();
	return 0;
}

/* _XAllocID - resource ID allocation routine. */
XID _XAllocID(Display *dpy)
{
	XID ret = dpy->xcl->next_xid;
	dpy->xcl->next_xid = 0;

	assert(!(dpy->flags & XlibDisplayPrivSync));
	dpy->savedsynchandler = dpy->synchandler;
	dpy->flags |= XlibDisplayPrivSync;
	dpy->synchandler = _XIDHandler;
	return ret;
}

/* _XAllocIDs - multiple resource ID allocation routine. */
void _XAllocIDs(Display *dpy, XID *ids, int count)
{
	int i;
	_XPutXCBBuffer(dpy);
	for (i = 0; i < count; i++)
		ids[i] = xcb_generate_id(dpy->xcl->connection);
	_XGetXCBBuffer(dpy);
}

static void _XFreeReplyData(Display *dpy, Bool force)
{
	if(!force && dpy->xcl->reply_consumed < dpy->xcl->reply_length)
		return;
	free(dpy->xcl->reply_data);
	dpy->xcl->reply_data = 0;
}

/*
 * _XReply - Wait for a reply packet and copy its contents into the
 * specified rep.
 * extra: number of 32-bit words expected after the reply
 * discard: should I discard data following "extra" words?
 */
Status _XReply(Display *dpy, xReply *rep, int extra, Bool discard)
{
	xcb_generic_error_t *error;
	xcb_connection_t *c = dpy->xcl->connection;
	unsigned long request = dpy->request;
	char *reply;

	assert(!dpy->xcl->reply_data);

	UnlockDisplay(dpy);
	reply = xcb_wait_for_reply(c, request, &error);
	LockDisplay(dpy);

	check_internal_connections(dpy);

	if(dpy->xcl->event_owner == XlibOwnsEventQueue)
	{
		xcb_generic_event_t *e;
		int ret;
		while((e = xcb_poll_for_event(c, &ret)))
			if(e->response_type == 0 && e->full_sequence == request)
				error = (xcb_generic_error_t *) e;
			else
				handle_event(dpy, e);
	}

	if(error)
	{
		_XExtension *ext;
		xError *err = (xError *) error;
		int ret_code;

		dpy->last_request_read = error->full_sequence;

		/* Xlib is evil and assumes that even errors will be
		 * copied into rep. */
		memcpy(rep, error, 32);

		/* do not die on "no such font", "can't allocate",
		   "can't grab" failures */
		switch(err->errorCode)
		{
			case BadName:
				switch(err->majorCode)
				{
					case X_LookupColor:
					case X_AllocNamedColor:
						return 0;
				}
				break;
			case BadFont:
				if(err->majorCode == X_QueryFont)
					return 0;
				break;
			case BadAlloc:
			case BadAccess:
				return 0;
		}

		/* 
		 * we better see if there is an extension who may
		 * want to suppress the error.
		 */
		for(ext = dpy->ext_procs; ext; ext = ext->next)
			if(ext->error && ext->error(dpy, err, &ext->codes, &ret_code))
				return ret_code;

		_XError(dpy, (xError *) error);
		return 0;
	}

	/* it's not an error, but we don't have a reply, so it's an I/O
	 * error. */
	if(!reply)
	{
		_XIOError(dpy);
		return 0;
	}

	dpy->last_request_read = request;

	/* there's no error and we have a reply. */
	dpy->xcl->reply_data = reply;
	dpy->xcl->reply_consumed = sizeof(xReply) + (extra * 4);
	dpy->xcl->reply_length = sizeof(xReply);
	if(dpy->xcl->reply_data[0] == 1)
		dpy->xcl->reply_length += (((xcb_generic_reply_t *) dpy->xcl->reply_data)->length * 4);

	/* error: Xlib asks too much. give them what we can anyway. */
	if(dpy->xcl->reply_length < dpy->xcl->reply_consumed)
		dpy->xcl->reply_consumed = dpy->xcl->reply_length;

	memcpy(rep, dpy->xcl->reply_data, dpy->xcl->reply_consumed);
	_XFreeReplyData(dpy, discard);
	return 1;
}

int _XRead(Display *dpy, char *data, long size)
{
	assert(size >= 0);
	if(size == 0)
		return 0;
	assert(dpy->xcl->reply_data != 0);
	assert(dpy->xcl->reply_consumed + size <= dpy->xcl->reply_length);
	memcpy(data, dpy->xcl->reply_data + dpy->xcl->reply_consumed, size);
	dpy->xcl->reply_consumed += size;
	_XFreeReplyData(dpy, False);
	return 0;
}

/*
 * _XReadPad - Read bytes from the socket taking into account incomplete
 * reads.  If the number of bytes is not 0 mod 4, read additional pad
 * bytes.
 */
void _XReadPad(Display *dpy, char *data, long size)
{
	_XRead(dpy, data, size);
	dpy->xcl->reply_consumed += -size & 3;
	_XFreeReplyData(dpy, False);
}

/* Read and discard "n" 8-bit bytes of data */
void _XEatData(Display *dpy, unsigned long n)
{
	dpy->xcl->reply_consumed += n;
	_XFreeReplyData(dpy, False);
}
