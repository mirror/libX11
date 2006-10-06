/* Copyright (C) 2003,2006 Jamey Sharp, Josh Triplett
 * This file is licensed under the MIT license. See the file COPYING. */

#include "Xlibint.h"
#include "xclint.h"

xcb_connection_t *XGetXCBConnection(Display *dpy)
{
	return dpy->xcl->connection;
}

void XSetEventQueueOwner(Display *dpy, enum XEventQueueOwner owner)
{
	dpy->xcl->event_owner = owner;
}
