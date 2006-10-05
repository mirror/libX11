/* Copyright (C) 2003-2005 Jamey Sharp.
 * This file is licensed under the MIT license. See the file COPYING. */

#ifndef XCLINT_H
#define XCLINT_H

#include <assert.h>
#include <X11/Xlibint.h>
#include <X11/xcl.h>

#define XCB_SEQUENCE_COMPARE(a,op,b)	((int) ((a) - (b)) op 0)
#define assert_sequence_less(a,b) assert(XCB_SEQUENCE_COMPARE((a), <=, (b)))

typedef struct PendingRequest PendingRequest;
struct PendingRequest {
	PendingRequest *next;
	unsigned int sequence;
};

typedef struct XCLPrivate {
	struct _XLockPtrs lock_fns;
	xcb_connection_t *connection;
	PendingRequest *pending_requests;
	PendingRequest **pending_requests_tail;
	const char *request_extra;
	int request_extra_size;
	char *partial_request;
	int partial_request_offset;
	char *reply_data;
	int reply_length;
	int reply_consumed;
	enum XEventQueueOwner event_owner;
	XID next_xid;
} XCLPrivate;

/* xcl/display.c */

int _XConnectXCB(Display *dpy, _Xconst char *display, char **fullnamep, int *screenp);
void _XFreeXCLStructure(Display *dpy);

/* xcl/xcblock.c */

int _XCBInitDisplayLock(Display *dpy);

/* _XGetXCBBuffer and _XPutXCBBuffer calls must be paired and must not
 * be nested. */

void _XGetXCBBuffer(Display *dpy);
void _XPutXCBBuffer(Display *dpy);

#endif /* XCLINT_H */
