/* $Xorg: lcCharSet.c,v 1.3 2000/08/17 19:45:16 cpqbld Exp $ */
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

#include <stdio.h>
#include "Xlibint.h"
#include "XlcPublic.h"

#if NeedVarargsPrototypes
char *
_XlcGetCSValues(XlcCharSet charset, ...)
#else
char *
_XlcGetCSValues(charset, va_alist)
    XlcCharSet charset;
    va_dcl
#endif
{
    va_list var;
    XlcArgList args;
    char *ret;
    int num_args;

    Va_start(var, charset);
    _XlcCountVaList(var, &num_args);
    va_end(var);

    Va_start(var, charset);
    _XlcVaToArgList(var, num_args, &args);
    va_end(var);

    if (args == (XlcArgList) NULL)
	return (char *) NULL;
    
    if (charset->get_values)
	ret = (*charset->get_values)(charset, args, num_args);
    else
	ret = args->name;

    Xfree(args);

    return ret;
}

typedef struct _XlcCharSetListRec {
    XlcCharSet charset;
    struct _XlcCharSetListRec *next;
} XlcCharSetListRec, *XlcCharSetList;

static XlcCharSetList charset_list = NULL;

XlcCharSet
_XlcGetCharSet(name)
    char *name;
{
    XlcCharSetList list;
    XrmQuark xrm_name;
    
    xrm_name = XrmStringToQuark(name);

    for (list = charset_list; list; list = list->next) {
	if (xrm_name == list->charset->xrm_name)
	    return (XlcCharSet) list->charset;
    }

    return (XlcCharSet) NULL;
}

Bool
_XlcAddCharSet(charset)
    XlcCharSet charset;
{
    XlcCharSetList list;

    if (_XlcGetCharSet(charset->name))
	return False;

    list = (XlcCharSetList) Xmalloc(sizeof(XlcCharSetListRec));
    if (list == NULL)
	return False;
    
    list->charset = charset;
    list->next = charset_list;
    charset_list = list;

    return True;
}

static XlcResource resources[] = {
    { XlcNName, NULLQUARK, sizeof(char *),
      XOffsetOf(XlcCharSetRec, name), XlcGetMask },
    { XlcNEncodingName, NULLQUARK, sizeof(char *),
      XOffsetOf(XlcCharSetRec, encoding_name), XlcGetMask },
    { XlcNSide, NULLQUARK, sizeof(XlcSide),
      XOffsetOf(XlcCharSetRec, side), XlcGetMask },
    { XlcNCharSize, NULLQUARK, sizeof(int),
      XOffsetOf(XlcCharSetRec, char_size), XlcGetMask },
    { XlcNSetSize, NULLQUARK, sizeof(int),
      XOffsetOf(XlcCharSetRec, set_size), XlcGetMask },
    { XlcNControlSequence, NULLQUARK, sizeof(char *),
      XOffsetOf(XlcCharSetRec, ct_sequence), XlcGetMask }
};

static char *
get_values(charset, args, num_args)
    register XlcCharSet charset;
    register XlcArgList args;
    register int num_args;
{
    if (resources[0].xrm_name == NULLQUARK)
	_XlcCompileResourceList(resources, XlcNumber(resources));

    return _XlcGetValues((XPointer) charset, resources, XlcNumber(resources),
			 args, num_args, XlcGetMask);
}

XlcCharSet
_XlcCreateDefaultCharSet(name, ct_sequence)
    char *name;
    char *ct_sequence;
{
    XlcCharSet charset;

    charset = (XlcCharSet) Xmalloc(sizeof(XlcCharSetRec));
    if (charset == NULL)
	return (XlcCharSet) NULL;
    bzero((char *) charset, sizeof(XlcCharSetRec));
    
    charset->name = (char *) Xmalloc(strlen(name) + strlen(ct_sequence) + 2);
    if (charset->name == NULL) {
	Xfree((char *) charset);
	return (XlcCharSet) NULL;
    }
    strcpy(charset->name, name);
    charset->ct_sequence = charset->name + strlen(name) + 1;
    strcpy(charset->ct_sequence, ct_sequence);
    charset->get_values = get_values;

    _XlcParseCharSet(charset);

    return (XlcCharSet) charset;
}
