/* $Xorg: lcWrap.c,v 1.6 2001/02/09 02:03:39 xorgcvs Exp $ */
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
 * Copyright 1993 by the TOSHIBA Corp.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the names of Open Software Foundation and TOSHIBA
 * not be used in advertising or publicity pertaining to distribution of the
 * software without specific, written prior permission.  Open Software
 * Foundation and TOSHIBA make no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without express or
 * implied warranty.
 *
 * OPEN SOFTWARE FOUNDATION AND TOSHIBA DISCLAIM ALL WARRANTIES WITH REGARD TO
 * THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS, IN NO EVENT SHALL OPEN SOFTWARE FOUNDATIONN OR TOSHIBA BE
 * LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES 
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 * 
 *		 M. Collins		OSF  
 *
 *		 Katsuhisa Yano		TOSHIBA Corp.
 */				

#include "Xlibint.h"
#include "Xlcint.h"
#include <X11/Xlocale.h>
#include <X11/Xos.h>
#ifdef WIN32
#undef close
#endif
#include <X11/Xutil.h>

#ifdef __STDC__
#define Const const
#else
#define Const /**/
#endif

#ifdef X_NOT_STDC_ENV
extern char *getenv();
#endif

extern void _XlcInitLoader(
#if NeedFunctionPrototypes
    char*
#endif
);

#ifdef XTHREADS
LockInfoPtr _Xi18n_lock;
#endif

#if NeedFunctionPrototypes
char *
XSetLocaleModifiers(
    _Xconst char   *modifiers
)
#else
char *
XSetLocaleModifiers(modifiers)
    char        *modifiers;
#endif
{
    XLCd lcd = _XlcCurrentLC();
    char *user_mods;

    if (!lcd)
	return (char *) NULL;
    if (!modifiers)
	return lcd->core->modifiers;
    user_mods = getenv("XMODIFIERS");
    modifiers = (*lcd->methods->map_modifiers) (lcd,
						user_mods, (char *)modifiers);
    if (modifiers) {
	if (lcd->core->modifiers)
	    Xfree(lcd->core->modifiers);
	lcd->core->modifiers = (char *)modifiers;
    }
    return (char *)modifiers;
}

Bool
XSupportsLocale()
{
    return _XlcCurrentLC() != (XLCd)NULL;
}

Bool _XlcValidModSyntax(mods, valid_mods)
    char *mods;
    char **valid_mods;
{
    int i;
    char **ptr;

    while (mods && (*mods == '@')) {
	mods++;
	if (*mods == '@')
	    break;
	for (ptr = valid_mods; *ptr; ptr++) {
	    i = strlen(*ptr);
	    if (strncmp(mods, *ptr, i) || ((mods[i] != '=')
#ifdef WIN32
					   && (mods[i] != '#')
#endif
					   ))
		continue;
	    mods = strchr(mods+i+1, '@');
	    break;
	}
    }
    return !mods || !*mods;
}

static Const char *im_valid[] = {"im", (char *)NULL};

/*ARGSUSED*/
char *
_XlcDefaultMapModifiers (lcd, user_mods, prog_mods)
    XLCd lcd;
    char *user_mods;
    char *prog_mods;
{
    int i;
    char *mods;

    if (!_XlcValidModSyntax(prog_mods, (char **)im_valid))
	return (char *)NULL;
    if (!_XlcValidModSyntax(user_mods, (char **)im_valid))
	return (char *)NULL;
    i = strlen(prog_mods) + 1;
    if (user_mods)
	i += strlen(user_mods);
    mods = Xmalloc(i);
    if (mods) {
	strcpy(mods, prog_mods);
	if (user_mods)
	    strcat(mods, user_mods);
#ifdef WIN32
	{
	    char *s;
	    for (s = mods; s = strchr(s, '@'); s++) {
		for (s++; *s && *s != '='; s++) {
		    if (*s == '#') {
			*s = '=';
			break;
		    }
		}
	    }
	}
#endif
    }
    return mods;
}

typedef struct _XLCdListRec {
    struct _XLCdListRec *next;
    XLCd lcd;
    int ref_count;
} XLCdListRec, *XLCdList;

static XLCdList lcd_list = NULL;

typedef struct _XlcLoaderListRec {
    struct _XlcLoaderListRec *next;
    XLCdLoadProc proc;
} XlcLoaderListRec, *XlcLoaderList;

static XlcLoaderList loader_list = NULL;

void
_XlcRemoveLoader(proc)
    XLCdLoadProc proc;
{
    XlcLoaderList loader, prev;

    if (loader_list == NULL)
	return;

    prev = loader = loader_list;
    if (loader->proc == proc) {
	loader_list = loader->next;
	Xfree(loader);
	return;
    }

    while ((loader = loader->next)) {
	if (loader->proc == proc) {
	    prev->next = loader->next;
	    Xfree(loader);
	    return;
	}
	prev = loader;
    }

    return;
}

Bool
_XlcAddLoader(proc, position)
    XLCdLoadProc proc;
    XlcPosition position;
{
    XlcLoaderList loader, last;

    _XlcRemoveLoader(proc);		/* remove old loader, if exist */

    loader = (XlcLoaderList) Xmalloc(sizeof(XlcLoaderListRec));
    if (loader == NULL)
	return False;

    loader->proc = proc;

    if (loader_list == NULL)
	position = XlcHead;

    if (position == XlcHead) {
	loader->next = loader_list;
	loader_list = loader;
    } else {
	last = loader_list;
	while (last->next)
	    last = last->next;
	
	loader->next = NULL;
	last->next = loader;
    }

    return True;
}

XLCd
_XOpenLC(name)
    char *name;
{
    XLCd lcd;
    XlcLoaderList loader;
    XLCdList cur;
#if !defined(X_NOT_STDC_ENV) && !defined(X_LOCALE)
    int len;
    char sinamebuf[256];
    char* siname;
    char *_XlcMapOSLocaleName();
#endif

    if (name == NULL) {
	name = setlocale (LC_CTYPE, (char *)NULL);
#if !defined(X_NOT_STDC_ENV) && !defined(X_LOCALE)
    /* 
     * _XlMapOSLOcaleName will return the same string or a substring 
     * of name, so strlen(name) is okay 
     */
    if ((len = strlen(name)) < sizeof sinamebuf) siname = sinamebuf;
    else siname = Xmalloc (len + 1);
    if (siname == NULL) return NULL;
    name = _XlcMapOSLocaleName(name, siname);
#endif
    }

    _XLockMutex(_Xi18n_lock);

    /*
     * search for needed lcd, if found return it
     */
    for (cur = lcd_list; cur; cur = cur->next) {
	if (!strcmp (cur->lcd->core->name, name)) {
	    lcd = cur->lcd;
	    cur->ref_count++;
	    goto found;
	}
    }

    _XlcInitLoader(name);

    /*
     * not there, so try to get and add to list
     */
    for (loader = loader_list; loader; loader = loader->next) {
	lcd = (*loader->proc)(name);
	if (lcd) {
	    cur = (XLCdList) Xmalloc (sizeof(XLCdListRec));
	    if (cur) {
		cur->lcd = lcd;
		cur->ref_count = 1;
		cur->next = lcd_list;
		lcd_list = cur;
	    } else {
		(*lcd->methods->close)(lcd);
		lcd = (XLCd) NULL;
	    }
	    break;
	}
    }

found:
    _XUnlockMutex(_Xi18n_lock);

#if !defined(X_NOT_STDC_ENV) && !defined(X_LOCALE)
    if (siname != sinamebuf) Xfree(siname);
#endif

    return lcd;
}

void
_XCloseLC(lcd)
    XLCd lcd;
{
    XLCdList cur, *prev;

    for (prev = &lcd_list; (cur = *prev); prev = &cur->next) {
	if (cur->lcd == lcd) {
	    if (--cur->ref_count < 1) {
		(*lcd->methods->close)(lcd);
		*prev = cur->next;
		Xfree(cur);
	    }
	    break;
	}
    }
}

/*
 * Get the XLCd for the current locale
 */

XLCd
_XlcCurrentLC()
{
    XLCd lcd;
    static XLCd last_lcd = NULL;

    lcd = _XOpenLC((char *) NULL);

    if (last_lcd)
	_XCloseLC(last_lcd);
    
    last_lcd = lcd;

    return lcd;
}

XrmMethods
_XrmInitParseInfo(state)
    XPointer *state;
{
    XLCd lcd = _XOpenLC((char *) NULL);
    
    if (lcd == (XLCd) NULL)
	return (XrmMethods) NULL;
    
    return (*lcd->methods->init_parse_info)(lcd, state);
}

int
XmbTextPropertyToTextList(dpy, text_prop, list_ret, count_ret)
    Display *dpy;
    XTextProperty *text_prop;
    char ***list_ret;
    int *count_ret;
{
    XLCd lcd = _XlcCurrentLC();
    
    if (lcd == NULL)
	return XLocaleNotSupported;

    return (*lcd->methods->mb_text_prop_to_list)(lcd, dpy, text_prop, list_ret,
						 count_ret);
}

int
XwcTextPropertyToTextList(dpy, text_prop, list_ret, count_ret)
    Display *dpy;
    XTextProperty *text_prop;
    wchar_t ***list_ret;
    int *count_ret;
{
    XLCd lcd = _XlcCurrentLC();
    
    if (lcd == NULL)
	return XLocaleNotSupported;

    return (*lcd->methods->wc_text_prop_to_list)(lcd, dpy, text_prop, list_ret,
						 count_ret);
}

int
XmbTextListToTextProperty(dpy, list, count, style, text_prop)
    Display *dpy;
    char **list;
    int count;
    XICCEncodingStyle style;
    XTextProperty *text_prop;
{
    XLCd lcd = _XlcCurrentLC();
    
    if (lcd == NULL)
	return XLocaleNotSupported;

    return (*lcd->methods->mb_text_list_to_prop)(lcd, dpy, list, count, style,
						 text_prop);
}

int
XwcTextListToTextProperty(dpy, list, count, style, text_prop)
    Display *dpy;
    wchar_t **list;
    int count;
    XICCEncodingStyle style;
    XTextProperty *text_prop;
{
    XLCd lcd = _XlcCurrentLC();
    
    if (lcd == NULL)
	return XLocaleNotSupported;

    return (*lcd->methods->wc_text_list_to_prop)(lcd, dpy, list, count, style,
						 text_prop);
}

void
XwcFreeStringList(list)
    wchar_t **list;
{
    XLCd lcd = _XlcCurrentLC();
    
    if (lcd == NULL)
	return;

    (*lcd->methods->wc_free_string_list)(lcd, list);
}

char *
XDefaultString()
{
    XLCd lcd = _XlcCurrentLC();
    
    if (lcd == NULL)
	return (char *) NULL;
    
    return (*lcd->methods->default_string)(lcd);
}

void
_XlcCopyFromArg(src, dst, size)
    char *src;
    register char *dst;
    register int size;
{
    if (size == sizeof(long))
	*((long *) dst) = (long) src;
#ifdef LONG64
    else if (size == sizeof(int))
	*((int *) dst) = (int) src;
#endif
    else if (size == sizeof(short))
	*((short *) dst) = (short)(long) src;
    else if (size == sizeof(char))
	*((char *) dst) = (char)(long) src;
    else if (size == sizeof(XPointer))
	*((XPointer *) dst) = (XPointer) src;
    else if (size > sizeof(XPointer))
	memcpy(dst, (char *) src, size);
    else
	memcpy(dst, (char *) &src, size);
}

void
_XlcCopyToArg(src, dst, size)
    register char *src;
    register char **dst;
    register int size;
{
    if (size == sizeof(long))
	*((long *) *dst) = *((long *) src);
    else if (size == sizeof(short))
	*((short *) *dst) = *((short *) src);
    else if (size == sizeof(char))
	*((char *) *dst) = *((char *) src);
    else if (size == sizeof(XPointer))
	*((XPointer *) *dst) = *((XPointer *) src);
    else
	memcpy(*dst, src, size);
}

void
_XlcCountVaList(var, count_ret)
    va_list var;
    int *count_ret;
{
    register int count;

    for (count = 0; va_arg(var, char *); count++)
	va_arg(var, XPointer);
    
    *count_ret = count;
}

void
_XlcVaToArgList(var, count, args_ret)
    va_list var;
    register int count;
    XlcArgList *args_ret;
{
    register XlcArgList args;

    *args_ret = args = (XlcArgList) Xmalloc(sizeof(XlcArg) * count);
    if (args == (XlcArgList) NULL)
	return;
    
    for ( ; count-- > 0; args++) {
	args->name = va_arg(var, char *);
	args->value = va_arg(var, XPointer);
    }
}

void
_XlcCompileResourceList(resources, num_resources)
    register XlcResourceList resources;
    register int num_resources;
{
    for ( ; num_resources-- > 0; resources++)
	resources->xrm_name = XrmPermStringToQuark(resources->name);
}

char *
_XlcGetValues(base, resources, num_resources, args, num_args, mask)
    XPointer base;
    XlcResourceList resources;
    int num_resources;
    XlcArgList args;
    int num_args;
    unsigned long mask;
{
    XlcResourceList res;
    XrmQuark xrm_name;
    int count;

    for ( ; num_args-- > 0; args++) {
	res = resources;
	count = num_resources;
	xrm_name = XrmPermStringToQuark(args->name);

	for ( ; count-- > 0; res++) {
	    if (xrm_name == res->xrm_name && (mask & res->mask)) {
		    _XlcCopyToArg(base + res->offset, &args->value, res->size);
		break;
	    }
	}

	if (count < 0)
	    return args->name;
    }

    return NULL;
}

char *
_XlcSetValues(base, resources, num_resources, args, num_args, mask)
    XPointer base;
    XlcResourceList resources;
    int num_resources;
    XlcArgList args;
    int num_args;
    unsigned long mask;
{
    XlcResourceList res;
    XrmQuark xrm_name;
    int count;

    for ( ; num_args-- > 0; args++) {
	res = resources;
	count = num_resources;
	xrm_name = XrmPermStringToQuark(args->name);

	for ( ; count-- > 0; res++) {
	    if (xrm_name == res->xrm_name && (mask & res->mask)) {
		_XlcCopyFromArg(args->value, base + res->offset, res->size);
		break;
	    }
	}

	if (count < 0)
	    return args->name;
    }

    return NULL;
}
