/* Copyright (C) 2003 Jamey Sharp.
 * This file is licensed under the MIT license. See the file COPYING. */

#ifndef XCL_H
#define XCL_H

#include <X11/XCB/xcb.h>
#include <X11/Xlib.h>

/* Coercions from Xlib XID types to XCB XID types.
 * On GCC/x86 with optimizations turned on, these compile to zero
 * instructions. */

#define XCLCASTDECL(src_t, dst_t, field)			\
	static inline XCB##dst_t XCL##dst_t(src_t src)		\
	{							\
		XCB##dst_t dst;					\
		dst.field = src;				\
		return dst;					\
	}
#define XCLXIDCASTDECL(src_t, dst_t) XCLCASTDECL(src_t, dst_t, xid)
#define XCLIDCASTDECL(src_t, dst_t) XCLCASTDECL(src_t, dst_t, id)

XCLXIDCASTDECL(Window, WINDOW)
XCLXIDCASTDECL(Pixmap, PIXMAP)
XCLXIDCASTDECL(Cursor, CURSOR)
XCLXIDCASTDECL(Font, FONT)
XCLXIDCASTDECL(GContext, GCONTEXT)
XCLXIDCASTDECL(Colormap, COLORMAP)
XCLXIDCASTDECL(Atom, ATOM)

/* For the union types, pick an arbitrary field of the union to hold the
 * Xlib XID. Assumes the bit pattern is the same regardless of the field. */
XCLCASTDECL(Drawable, DRAWABLE, window.xid)
XCLCASTDECL(Font, FONTABLE, font.xid)

XCLIDCASTDECL(VisualID, VISUALID)
XCLIDCASTDECL(Time, TIMESTAMP)
XCLIDCASTDECL(KeySym, KEYSYM)
XCLIDCASTDECL(KeyCode, KEYCODE)
XCLIDCASTDECL(CARD8, BUTTON)

/* xcl/display.c */

XCBConnection *XCBConnectionOfDisplay(Display *dpy);

/* xcl/io.c */

enum XEventQueueOwner { XlibOwnsEventQueue = 0, XCBOwnsEventQueue };
void XSetEventQueueOwner(Display *dpy, enum XEventQueueOwner owner);

#endif /* XCL_H */
