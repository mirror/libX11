/* $Xorg: lcDefConv.c,v 1.3 2000/08/17 19:45:17 cpqbld Exp $ */
/*
 * Copyright 1992, 1993 by TOSHIBA Corp.
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of TOSHIBA not be used in advertising
 * or publicity pertaining to distribution of the software without specific,
 * written prior permission. TOSHIBA make no representations about the
 * suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 *
 * TOSHIBA DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
 * ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
 * TOSHIBA BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
 * ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 * WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
 * ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 *
 * Author: Katsuhisa Yano	TOSHIBA Corp.
 *			   	mopi@osa.ilab.toshiba.co.jp
 */

#include "Xlibint.h"
#include "XlcPubI.h"

typedef struct _StateRec {
    XlcCharSet charset;
    XlcCharSet GL_charset;
    XlcCharSet GR_charset;
    XlcConv ct_conv;
    int (*to_converter)();
} StateRec, *State;

static int
strtostr(conv, from, from_left, to, to_left, args, num_args)
    XlcConv conv;
    XPointer *from;
    int *from_left;
    XPointer *to;
    int *to_left;
    XPointer *args;
    int num_args;
{
    register char *src, *dst;
    unsigned char side;
    register int length;

    if (from == NULL || *from == NULL)
	return 0;

    src = (char *) *from;
    dst = (char *) *to;

    length = min(*from_left, *to_left);

    if (num_args > 0) {
	side = (length > 0) ? *((unsigned char *) src) & 0x80 : 0;
	while (length-- > 0 && side == (*((unsigned char *) src) & 0x80))
	    *dst++ = *src++;
    } else {
	while (length-- > 0)
	    *dst++ = *src++;
    }
    
    *from_left -= src - (char *) *from;
    *from = (XPointer) src;
    *to_left -= dst - (char *) *to;
    *to = (XPointer) dst;

    if (num_args > 0) {
	State state = (State) conv->state;

	*((XlcCharSet *)args[0]) = side ? state->GR_charset : state->GL_charset;
    }

    return 0;
}

static int
wcstostr(conv, from, from_left, to, to_left, args, num_args)
    XlcConv conv;
    XPointer *from;
    int *from_left;
    XPointer *to;
    int *to_left;
    XPointer *args;
    int num_args;
{
    register wchar_t *src, side;
    register char *dst;
    register int length;

    if (from == NULL || *from == NULL)
	return 0;

    src = (wchar_t *) *from;
    dst = (char *) *to;

    length = min(*from_left, *to_left);

    if (num_args > 0) {
	side = (length > 0) ? *src & 0x80 : 0;
	while (length-- > 0 && side == (*src & 0x80))
	    *dst++ = *src++;
    } else {
	while (length-- > 0)
	    *dst++ = *src++;
    }
    
    *from_left -= src - (wchar_t *) *from;
    *from = (XPointer) src;
    *to_left -= dst - (char *) *to;
    *to = (XPointer) dst;

    if (num_args > 0) {
	State state = (State) conv->state;

	*((XlcCharSet *)args[0]) = side ? state->GR_charset : state->GL_charset;
    }

    return 0;
}

static int
cstostr(conv, from, from_left, to, to_left, args, num_args)
    XlcConv conv;
    XPointer *from;
    int *from_left;
    XPointer *to;
    int *to_left;
    XPointer *args;
    int num_args;
{
    register char *src, *dst;
    unsigned char side;
    register int length;

    if (from == NULL || *from == NULL)
	return 0;

    if (num_args > 0) {
	State state = (State) conv->state;
	XlcCharSet charset = (XlcCharSet) args[0];

	if (charset != state->GL_charset && charset != state->GR_charset)
	    return -1;
    }

    src = (char *) *from;
    dst = (char *) *to;

    length = min(*from_left, *to_left);

    if (num_args > 0) {
	side = (length > 0) ? *((unsigned char *) src) & 0x80 : 0;
	while (length-- > 0 && side == (*((unsigned char *) src) & 0x80))
	    *dst++ = *src++;
    } else {
	while (length-- > 0)
	    *dst++ = *src++;
    }
    
    *from_left -= src - (char *) *from;
    *from = (XPointer) src;
    *to_left -= dst - (char *) *to;
    *to = (XPointer) dst;

    if (num_args > 0) {
	State state = (State) conv->state;

	*((XlcCharSet *)args[0]) = side ? state->GR_charset : state->GL_charset;
    }

    return 0;
}

static int
strtowcs(conv, from, from_left, to, to_left, args, num_args)
    XlcConv conv;
    XPointer *from;
    int *from_left;
    XPointer *to;
    int *to_left;
    XPointer *args;
    int num_args;
{
    register char *src;
    register wchar_t *dst;
    register int length;

    if (from == NULL || *from == NULL)
	return 0;

    if (num_args > 0) {
	State state = (State) conv->state;
	XlcCharSet charset = (XlcCharSet) args[0];

	if (charset != state->GL_charset && charset != state->GR_charset)
	    return -1;
    }

    src = (char *) *from;
    dst = (wchar_t *) *to;

    length = min(*from_left, *to_left);

    while (length-- > 0)
	*dst++ = (wchar_t) *src++;
    
    *from_left -= src - (char *) *from;
    *from = (XPointer) src;
    *to_left -= dst - (wchar_t *) *to;
    *to = (XPointer) dst;

    return 0;
}


static void
close_converter(conv)
    XlcConv conv;
{
    if (conv->state)
	Xfree((char *) conv->state);

    Xfree((char *) conv);
}

static XlcConv
create_conv(methods)
    XlcConvMethods methods;
{
    register XlcConv conv;
    State state;
    static XlcCharSet GL_charset = NULL;
    static XlcCharSet GR_charset = NULL;

    if (GL_charset == NULL) {
	GL_charset = _XlcGetCharSet("ISO8859-1:GL");
	GR_charset = _XlcGetCharSet("ISO8859-1:GR");
    }

    conv = (XlcConv) Xmalloc(sizeof(XlcConvRec));
    if (conv == NULL)
	return (XlcConv) NULL;

    state = (State) Xmalloc(sizeof(StateRec));
    if (state == NULL)
	goto err;
    
    state->GL_charset = state->charset = GL_charset;
    state->GR_charset = GR_charset;

    conv->methods = methods;
    conv->state = (XPointer) state;

    return conv;

err:
    close_converter(conv);

    return (XlcConv) NULL;
}

static XlcConvMethodsRec strtostr_methods = {
    close_converter,
    strtostr,
    NULL
} ;

static XlcConv
open_strtostr(from_lcd, from_type, to_lcd, to_type)
    XLCd from_lcd;
    char *from_type;
    XLCd to_lcd;
    char *to_type;
{
    return create_conv(&strtostr_methods);
}

static XlcConvMethodsRec wcstostr_methods = {
    close_converter,
    wcstostr,
    NULL
} ;

static XlcConv
open_wcstostr(from_lcd, from_type, to_lcd, to_type)
    XLCd from_lcd;
    char *from_type;
    XLCd to_lcd;
    char *to_type;
{
    return create_conv(&wcstostr_methods);
}

static XlcConvMethodsRec cstostr_methods = {
    close_converter,
    cstostr,
    NULL
} ;

static XlcConv
open_cstostr(from_lcd, from_type, to_lcd, to_type)
    XLCd from_lcd;
    char *from_type;
    XLCd to_lcd;
    char *to_type;
{
    return create_conv(&cstostr_methods);
}

static XlcConvMethodsRec strtowcs_methods = {
    close_converter,
    strtowcs,
    NULL
} ;

static XlcConv
open_strtowcs(from_lcd, from_type, to_lcd, to_type)
    XLCd from_lcd;
    char *from_type;
    XLCd to_lcd;
    char *to_type;
{
    return create_conv(&strtowcs_methods);
}

XLCd
_XlcDefaultLoader(name)
    char *name;
{
    XLCd lcd;

    if (strcmp(name, "C"))
	return (XLCd) NULL;

    lcd = _XlcCreateLC(name, _XlcPublicMethods);

    _XlcSetConverter(lcd, XlcNMultiByte, lcd, XlcNWideChar, open_strtowcs);
    _XlcSetConverter(lcd, XlcNMultiByte, lcd, XlcNCompoundText, open_strtostr);
    _XlcSetConverter(lcd, XlcNMultiByte, lcd, XlcNString, open_strtostr);
    _XlcSetConverter(lcd, XlcNMultiByte, lcd, XlcNCharSet, open_strtostr);
    _XlcSetConverter(lcd, XlcNMultiByte, lcd, XlcNChar, open_strtostr);/* XXX */

    _XlcSetConverter(lcd, XlcNWideChar, lcd, XlcNMultiByte, open_wcstostr);
    _XlcSetConverter(lcd, XlcNWideChar, lcd, XlcNCompoundText, open_wcstostr);
    _XlcSetConverter(lcd, XlcNWideChar, lcd, XlcNString, open_wcstostr);
    _XlcSetConverter(lcd, XlcNWideChar, lcd, XlcNCharSet, open_wcstostr);

    _XlcSetConverter(lcd, XlcNString, lcd, XlcNMultiByte, open_strtostr);
    _XlcSetConverter(lcd, XlcNString, lcd, XlcNWideChar, open_strtowcs);

    _XlcSetConverter(lcd, XlcNCharSet, lcd, XlcNMultiByte, open_cstostr);
    _XlcSetConverter(lcd, XlcNCharSet, lcd, XlcNWideChar, open_strtowcs);

    return lcd;
}
