/* $Xorg: SetLocale.c,v 1.4 2001/02/09 02:03:36 xorgcvs Exp $ */

/*
 * Copyright 1990, 1991 by OMRON Corporation, NTT Software Corporation,
 *                      and Nippon Telegraph and Telephone Corporation
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the names of OMRON, NTT Software, and NTT
 * not be used in advertising or publicity pertaining to distribution of the
 * software without specific, written prior permission. OMRON, NTT Software,
 * and NTT make no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without express or
 * implied warranty.
 *
 * OMRON, NTT SOFTWARE, AND NTT, DISCLAIM ALL WARRANTIES WITH REGARD
 * TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS, IN NO EVENT SHALL OMRON, NTT SOFTWARE, OR NTT, BE
 * LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES 
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 *	Authors: Li Yuhong		OMRON Corporation
 *		 Tetsuya Kato		NTT Software Corporation
 *		 Hiroshi Kuribayashi	OMRON Corporation
 *   
 */
/*

Copyright 1987,1998  The Open Group

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

#include "Xlibint.h"
#include "Xlcint.h"
#include <X11/Xlocale.h>
#include <X11/Xos.h>

#ifdef X_LOCALE

/* alternative setlocale() for when the OS does not provide one */

#ifdef X_NOT_STDC_ENV
extern char *getenv();
#endif

#if NeedFunctionPrototypes
char *
_Xsetlocale(
    int		  category,
    _Xconst char *name
)
#else
char *
_Xsetlocale(category, name)
    int		category;
    char       *name;
#endif
{
    static char *xsl_name;
    char *old_name;
    XrmMethods methods;
    XPointer state;

    if (category != LC_CTYPE && category != LC_ALL)
	return NULL;
    if (!name) {
	if (xsl_name)
	    return xsl_name;
	return "C";
    }
    if (!*name)
	name = getenv("LC_CTYPE");
    if (!name || !*name)
	name = getenv("LANG");
    if (!name || !*name || !_XOpenLC(name))
	name = "C";
    old_name = xsl_name;
    xsl_name = (char *)name;
    methods = _XrmInitParseInfo(&state);
    xsl_name = old_name;
    if (!methods)
	return NULL;
    name = (*methods->lcname)(state);
    xsl_name = Xmalloc(strlen(name) + 1);
    if (!xsl_name) {
	xsl_name = old_name;
	(*methods->destroy)(state);
	return NULL;
    }
    strcpy(xsl_name, name);
    if (old_name)
	Xfree(old_name);
    (*methods->destroy)(state);
    return xsl_name;
}

#else /* X_LOCALE */

/*
 * _XlcMapOSLocaleName is an implementation dependent routine that derives
 * the LC_CTYPE locale name as used in the sample implementation from that
 * returned by setlocale.
 *
 * Should match the code in Xt ExtractLocaleName.
 * 
 * This function name is a bit of a misnomer. Even the siname parameter
 * name is a misnomer. On most modern operating systems this function is 
 * a no-op, simply returning the osname; but on older operating systems 
 * like Ultrix, or HPUX 9.x and earlier, when you set LANG=german.88591 
 * then the string returned by setlocale(LC_ALL, "") will look something 
 * like: "german.88591 german.88591 ... german.88591". Then this function
 * will pick out the LC_CTYPE component and return a pointer to that.
 */

char *
_XlcMapOSLocaleName(osname, siname)
    char *osname;
    char *siname;
{
#if defined(hpux) || defined(CSRG_BASED) || defined(sun) || defined(SVR4) || defined(sgi) || defined(__osf__) || defined(AIXV3) || defined(ultrix) || defined(WIN32)
#ifdef hpux
#ifndef _LastCategory
/* HPUX 9 and earlier */
#define SKIPCOUNT 2
#define STARTCHAR ':'
#define ENDCHAR ';'
#else
/* HPUX 10 */
#define ENDCHAR ' '
#endif
#else
#ifdef ultrix
#define SKIPCOUNT 2
#define STARTCHAR '\001'
#define ENDCHAR '\001'
#else
#ifdef WIN32
#define SKIPCOUNT 1
#define STARTCHAR '='
#define ENDCHAR ';'
#define WHITEFILL
#else
#if defined(__osf__) || (defined(AIXV3) && !defined(AIXV4))
#define STARTCHAR ' '
#define ENDCHAR ' '
#else
#if !defined(sun) || defined(SVR4)
#define STARTCHAR '/'
#endif
#define ENDCHAR '/'
#endif
#endif
#endif
#endif

    char           *start;
    char           *end;
    int             len;
#ifdef SKIPCOUNT
    int		    n;
#endif

    start = osname;
#ifdef SKIPCOUNT
    for (n = SKIPCOUNT;
	 --n >= 0 && start && (start = strchr (start, STARTCHAR));
	 start++)
	;
    if (!start)
	start = osname;
#endif
#ifdef STARTCHAR
    if (start && (start = strchr (start, STARTCHAR))) {
	start++;
#endif
	if (end = strchr (start, ENDCHAR)) {
	    len = end - start;
	    strncpy(siname, start, len);
	    *(siname + len) = '\0';
#ifdef WHITEFILL
	    for (start = siname; start = strchr(start, ' '); )
		*start++ = '-';
#endif
	    return siname;
#ifdef STARTCHAR
	}
#endif
    }
#ifdef WHITEFILL
    if (strchr(osname, ' ')) {
	strcpy(siname, osname);
	for (start = siname; start = strchr(start, ' '); )
	    *start++ = '-';
	return siname;
    }
#endif
#undef STARTCHAR
#undef ENDCHAR
#undef WHITEFILL
#endif
    return osname;
}

#endif  /* X_LOCALE */
