/* Copyright (C) 2003 Jamey Sharp.
 * This file is licensed under the MIT license. See the file COPYING. */

#ifndef XCL_H
#define XCL_H

#include <X11/Xmd.h>
#include <X11/XCB/xcb.h>
#include <X11/Xlib.h>

/* Coercions from Xlib XID types to XCB XID types.
 * On GCC/x86 with optimizations turned on, these compile to zero
 * instructions. */

#define XCLCASTDECL(src_t, dst_t, field)			\
	static inline xcb_##dst_t xcl_##dst_t(src_t src)	\
	{							\
		xcb_##dst_t dst;				\
		dst.field = src;				\
		return dst;					\
	}
#define XCLXIDCASTDECL(src_t, dst_t) XCLCASTDECL(src_t, dst_t, xid)
#define XCLIDCASTDECL(src_t, dst_t) XCLCASTDECL(src_t, dst_t, id)

XCLXIDCASTDECL(Window, window_t)
XCLXIDCASTDECL(Pixmap, pixmap_t)
XCLXIDCASTDECL(Cursor, cursor_t)
XCLXIDCASTDECL(Font, font_t)
XCLXIDCASTDECL(GContext, gcontext_t)
XCLXIDCASTDECL(Colormap, colormap_t)
XCLXIDCASTDECL(Atom, atom_t)

/* For the union types, pick an arbitrary field of the union to hold the
 * Xlib XID. Assumes the bit pattern is the same regardless of the field. */
XCLCASTDECL(Drawable, drawable_t, window.xid)
XCLCASTDECL(Font, fontable_t, font.xid)

XCLIDCASTDECL(VisualID, visualid_t)
XCLIDCASTDECL(Time, timestamp_t)
XCLIDCASTDECL(KeySym, keysym_t)
XCLIDCASTDECL(KeyCode, keycode_t)
XCLIDCASTDECL(CARD8, button_t)

/* xcl/display.c */

xcb_connection_t *XGetXCBConnection(Display *dpy);

/* xcl/io.c */

enum XEventQueueOwner { XlibOwnsEventQueue = 0, XCBOwnsEventQueue };
void XSetEventQueueOwner(Display *dpy, enum XEventQueueOwner owner);

#endif /* XCL_H */
