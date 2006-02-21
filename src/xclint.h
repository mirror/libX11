/* Copyright (C) 2003-2005 Jamey Sharp.
 * This file is licensed under the MIT license. See the file COPYING. */

#ifndef XCLINT_H
#define XCLINT_H

#include <assert.h>
#include <X11/xcl.h>

#define assert_sequence_less(a,b) assert((b) - (a) < 65536)

typedef struct PendingRequest PendingRequest;
struct PendingRequest {
	PendingRequest *next;
	unsigned int sequence;
};

typedef struct XCLPrivate {
	XCBConnection *connection;
	PendingRequest *pending_requests;
	PendingRequest **pending_requests_tail;
	const char *request_extra;
	int request_extra_size;
	char *reply_data;
	int reply_length;
	int reply_consumed;
	int lock_count;
	enum XEventQueueOwner event_owner;
} XCLPrivate;

/* xcl/display.c */

int _XConnectSetupXCB(Display *dpy);
int _XConnectXCB(Display *dpy, _Xconst char *display, char **fullnamep, int *screenp);
void _XFreeXCLStructure(Display *dpy);

/* xcl/xcblock.c */

/* _XGetXCBBuffer and _XPutXCBBuffer calls must be paired and must not
 * be nested. */

void _XGetXCBBuffer(Display *dpy);
void _XPutXCBBuffer(Display *dpy);

enum _XBufferCondition { _XBufferUnlocked, _XBufferLocked };
void _XGetXCBBufferIf(Display *dpy, enum _XBufferCondition locked);
void _XPutXCBBufferIf(Display *dpy, enum _XBufferCondition locked);

#endif /* XCLINT_H */
