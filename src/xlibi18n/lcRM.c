/* $Xorg: lcRM.c,v 1.3 2000/08/17 19:45:19 cpqbld Exp $ */
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
#include <stdio.h>

typedef struct _StateRec {
    XLCd lcd;
    XlcConv conv;
} StateRec, *State;

static void
mbinit(state)
    XPointer state;
{
    _XlcResetConverter(((State) state)->conv);
}

static char
mbchar(state, str, lenp)
    XPointer state;
    char *str;
    int *lenp;
{
    XlcConv conv = ((State) state)->conv;
    XlcCharSet charset;
    char *from, *to, buf[BUFSIZ];
    int from_left, to_left;
    XPointer args[1];

    from = str;
    *lenp = from_left = XLC_PUBLIC(((State) state)->lcd, mb_cur_max);
    to = buf;
    to_left = BUFSIZ;
    args[0] = (XPointer) &charset;

    _XlcConvert(conv, (XPointer *) &from, &from_left, (XPointer *) &to,
		&to_left, args, 1);
    
    *lenp -= from_left;

    /* XXX */
    return buf[0];
}

static void
mbfinish(state)
    XPointer state;
{
}

static char *
lcname(state)
    XPointer state;
{
    return ((State) state)->lcd->core->name;
}

static void
destroy(state)
    XPointer state;
{
    _XlcCloseConverter(((State) state)->conv);
    _XCloseLC(((State) state)->lcd);
    Xfree((char *) state);
}

static XrmMethodsRec rm_methods = {
    mbinit,
    mbchar,
    mbfinish,
    lcname,
    destroy
} ;

XrmMethods
_XrmDefaultInitParseInfo(lcd, rm_state)
    XLCd lcd;
    XPointer *rm_state;
{
    State state;

    state = (State) Xmalloc(sizeof(StateRec));
    if (state == NULL)
	return (XrmMethods) NULL;

    state->lcd = lcd;
    state->conv = _XlcOpenConverter(lcd, XlcNMultiByte, lcd, XlcNChar);
    if (state->conv == NULL) {
	Xfree((char *) state);

	return (XrmMethods) NULL;
    }
    
    *rm_state = (XPointer) state;
    
    return &rm_methods;
}
