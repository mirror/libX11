/* $TOG: mbWrap.c /main/7 1998/02/06 17:44:30 kaleb $ */
/*

Copyright 1991, 1998  The Open Group

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall
not be used in advertising or otherwise to promote the sale, use or
other dealings in this Software without prior written authorization
from The Open Group.

*/
/*
 * Copyright 1991 by the Open Software Foundation
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Open Software Foundation 
 * not be used in advertising or publicity pertaining to distribution of the
 * software without specific, written prior permission.  Open Software
 * Foundation makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without express or
 * implied warranty.
 *
 * OPEN SOFTWARE FOUNDATION DISCLAIMS ALL WARRANTIES WITH REGARD TO
 * THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS, IN NO EVENT SHALL OPEN SOFTWARE FOUNDATIONN BE
 * LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES 
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 * 
 *		 M. Collins		OSF  
 */				
/*
 * Copyright 2000 by Bruno Haible
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without fee,
 * provided that the above copyright notice appear in all copies and
 * that both that copyright notice and this permission notice appear
 * in supporting documentation, and that the name of Bruno Haible not
 * be used in advertising or publicity pertaining to distribution of the
 * software without specific, written prior permission.  Bruno Haible
 * makes no representations about the suitability of this software for
 * any purpose.  It is provided "as is" without express or implied
 * warranty.
 *
 * Bruno Haible DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN
 * NO EVENT SHALL Bruno Haible BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
 * OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE
 * OR PERFORMANCE OF THIS SOFTWARE.
 */
/* $XFree86: xc/lib/X11/utf8Wrap.c,v 1.3 2001/12/14 19:54:11 dawes Exp $ */

#include "Xlibint.h"
#include "Xlcint.h"

void
XFree86utf8DrawText(
    Display            *dpy,
    Drawable            d,
    GC                  gc,
    int                 x,
    int                 y,
    XmbTextItem        *text_items,
    int                 nitems)
{
    register XFontSet fs = NULL;
    register XmbTextItem *p = text_items;
    register int i = nitems;
    register int esc;

    /* ignore leading items with no fontset */
    while (i && !p->font_set) {
	i--;
	p++;
    }

    for (; --i >= 0; p++) {
	if (p->font_set)
	    fs = p->font_set;
	x += p->delta;
	esc = (*fs->methods->utf8_draw_string) (dpy, d, fs, gc, x, y,
						p->chars, p->nchars);
	if (!esc)
	    esc = fs->methods->utf8_escapement (fs, p->chars, p->nchars);
	x += esc;
    }
}
#ifndef NOXFREE86COMPAT
#undef Xutf8DrawText
void
Xutf8DrawText(
    Display            *dpy,
    Drawable            d,
    GC                  gc,
    int                 x,
    int                 y,
    XmbTextItem        *text_items,
    int                 nitems)
{
    XFree86utf8DrawText(dpy, d, gc, x, y, text_items, nitems);
}
#endif

void
XFree86utf8DrawString(
    Display            *dpy,
    Drawable            d,
    XFontSet            font_set,
    GC                  gc,
    int                 x,
    int                 y,
    _Xconst char       *text,
    int                 text_len)
{
    (void)(*font_set->methods->utf8_draw_string) (dpy, d, font_set, gc, x, y,
						  (char *)text, text_len);
}
#ifndef NOXFREE86COMPAT
#undef Xutf8DrawString
void
Xutf8DrawString(
    Display            *dpy,
    Drawable            d,
    XFontSet            font_set,
    GC                  gc,
    int                 x,
    int                 y,
    _Xconst char       *text,
    int                 text_len)
{
    XFree86utf8DrawString(dpy, d, font_set, gc, x, y, text, text_len);
}
#endif

void
XFree86utf8DrawImageString(
    Display            *dpy,
    Drawable            d,
    XFontSet            font_set,
    GC                  gc,
    int                 x,
    int                 y,
    _Xconst char       *text,
    int                 text_len)
{
    (*font_set->methods->utf8_draw_image_string) (dpy, d, font_set, gc, x, y,
						  (char *)text, text_len);
}
#ifndef NOXFREE86COMPAT
#undef Xutf8DrawImageString
void
Xutf8DrawImageString(
    Display            *dpy,
    Drawable            d,
    XFontSet            font_set,
    GC                  gc,
    int                 x,
    int                 y,
    _Xconst char       *text,
    int                 text_len)
{
    XFree86utf8DrawImageString(dpy, d, font_set, gc, x, y, text, text_len);
}
#endif

int 
XFree86utf8TextEscapement(
    XFontSet        font_set,
    _Xconst char   *text,
    int             text_len)
{
    return (*font_set->methods->utf8_escapement) (font_set,
						  (char *)text, text_len);
}
#ifndef NOXFREE86COMPAT
#undef Xutf8TextEscapement
int 
Xutf8TextEscapement(
    XFontSet        font_set,
    _Xconst char   *text,
    int             text_len)
{
    return XFree86utf8TextEscapement(font_set, text, text_len);
}
#endif

int
XFree86utf8TextExtents(
    XFontSet        font_set,
    _Xconst char   *text,
    int             text_len,
    XRectangle     *overall_ink_extents,
    XRectangle     *overall_logical_extents)
{
    return (*font_set->methods->utf8_extents) (font_set,
					       (char *)text, text_len,
					       overall_ink_extents,
					       overall_logical_extents);
}
#ifndef NOXFREE86COMPAT
#undef Xutf8TextExtents
int
Xutf8TextExtents(
    XFontSet        font_set,
    _Xconst char   *text,
    int             text_len,
    XRectangle     *overall_ink_extents,
    XRectangle     *overall_logical_extents)
{
    return XFree86utf8TextExtents(font_set, text, text_len,
	overall_ink_extents, overall_logical_extents);
}
#endif

Status
XFree86utf8TextPerCharExtents(
    XFontSet        font_set,
    _Xconst char   *text,
    int             text_len,
    XRectangle     *ink_extents_buffer,
    XRectangle     *logical_extents_buffer,
    int             buffer_size,
    int            *num_chars,
    XRectangle     *max_ink_extents,
    XRectangle     *max_logical_extents)
{
    return (*font_set->methods->utf8_extents_per_char)
	     (font_set, (char *)text, text_len, 
	      ink_extents_buffer, logical_extents_buffer,
	      buffer_size, num_chars, max_ink_extents, max_logical_extents);
}
#ifndef NOXFREE86COMPAT
#undef Xutf8TextPerCharExtents
Status
Xutf8TextPerCharExtents(
    XFontSet        font_set,
    _Xconst char   *text,
    int             text_len,
    XRectangle     *ink_extents_buffer,
    XRectangle     *logical_extents_buffer,
    int             buffer_size,
    int            *num_chars,
    XRectangle     *max_ink_extents,
    XRectangle     *max_logical_extents)
{
    return XFree86utf8TextPerCharExtents(font_set, text, text_len,
	ink_extents_buffer, logical_extents_buffer, buffer_size,
	    num_chars, max_ink_extents, max_logical_extents);
}
#endif
