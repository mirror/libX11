/* $Xorg: XKBCvt.c,v 1.5 2001/02/09 02:03:38 xorgcvs Exp $ */
/*

Copyright 1988, 1989, 1998  The Open Group

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

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <X11/X.h>
#include <X11/Xlib.h>
#define NEED_EVENTS
#include "Xlibint.h"
#include "Xlcint.h"
#include "XlcPubI.h"
#include <X11/Xutil.h>
#include <X11/Xmd.h>
#define XK_LATIN1
#define XK_PUBLISHING
#include <X11/keysym.h>
#include <X11/extensions/XKBproto.h>
#include "XKBlibint.h"
#include <X11/Xlocale.h>
#include <ctype.h>
#include <X11/Xos.h>

#ifdef __sgi_not_xconsortium
#define	XKB_EXTEND_LOOKUP_STRING
#endif

#ifdef X_NOT_STDC_ENV
extern char *getenv();
#endif

#ifdef __STDC__
#define Const const
#else
#define Const /**/
#endif
#if defined(__STDC__) && !defined(NORCONST)
#define RConst const
#else
#define RConst /**/
#endif

/* bit (1<<i) means character is in codeset i at the same codepoint */
extern unsigned int _Xlatin1[];

/* bit (1<<i) means character is in codeset i at the same codepoint */
extern unsigned int _Xlatin2[];

/* maps Cyrillic keysyms to KOI8-R */
extern unsigned char _Xkoi8[];

extern unsigned short _Xkoi8_size;

#define sLatin1         0
#define sLatin2         1
#define sLatin3         2
#define sLatin4         3
#define sKana           4
#define sX0201          0x01000004
#define sArabic         5
#define sCyrillic       6
#define sGreek          7
#define sAPL            11
#define sHebrew         12
#define sThai		13
#define sKorean		14
#define sLatin5		15
#define sLatin6		16
#define sLatin7		17
#define sLatin8		18
#define sLatin9		19
#define sCurrency	32


static unsigned long	WantLatin1 = sLatin1;
static unsigned long	WantLatin2 = sLatin2;
static unsigned long	WantLatin3 = sLatin3;
static unsigned long	WantLatin4 = sLatin4;
static unsigned long	WantLatin5 = sLatin5;
static unsigned long	WantLatin6 = sLatin6;
static unsigned long	WantKana = sKana;
static unsigned long	WantX0201 = sX0201;
static unsigned long	WantArabic = sArabic;
static unsigned long	WantCyrillic = sCyrillic;
static unsigned long	WantGreek = sGreek;
static unsigned long	WantAPL = sAPL;
static unsigned long	WantHebrew = sHebrew;

static int 
#if NeedFunctionPrototypes
_XkbHandleSpecialSym(KeySym keysym, char *buffer, int nbytes, int *extra_rtrn)
#else
_XkbHandleSpecialSym(keysym, buffer, nbytes, extra_rtrn)
    KeySym	 keysym;
    char	*buffer;
    int		 nbytes;
    int *	 extra_rtrn;
#endif
{

    /* try to convert to Latin-1, handling ctrl */
    if (!(((keysym >= XK_BackSpace) && (keysym <= XK_Clear)) ||
	   (keysym == XK_Return) || (keysym == XK_Escape) ||
	   (keysym == XK_KP_Space) || (keysym == XK_KP_Tab) ||
	   (keysym == XK_KP_Enter) || 
	   ((keysym >= XK_KP_Multiply) && (keysym <= XK_KP_9)) ||
	   (keysym == XK_KP_Equal) ||
	   (keysym == XK_Delete)))
	return 0;

    if (nbytes<1) {
	if (extra_rtrn) 
	    *extra_rtrn= 1;
	return 0;
    }
    /* if X keysym, convert to ascii by grabbing low 7 bits */
    if (keysym == XK_KP_Space)
	 buffer[0] = XK_space & 0x7F; /* patch encoding botch */
    else if (keysym == XK_hyphen)
	 buffer[0] = (char)(XK_minus & 0xFF); /* map to equiv character */
    else buffer[0] = (char)(keysym & 0x7F);
    return 1;
}

extern int
_XGetCharCode (
#if NeedFunctionPrototypes
    unsigned long, KeySym, char*, int
#endif
);

/*ARGSUSED*/
int 
#if NeedFunctionPrototypes
_XkbKSToKnownSet (	XPointer 	priv,
			KeySym 		keysym,
			char *		buffer,
			int 		nbytes,
			int *		extra_rtrn)
#else
_XkbKSToKnownSet (priv, keysym, buffer, nbytes, extra_rtrn)
    XPointer priv;
    KeySym keysym;
    char *buffer;
    int nbytes;
    int *extra_rtrn;
#endif
{
    unsigned long kset,keysymSet;
    int	count,isLatin1;
    char tbuf[8],*buf;

    if (extra_rtrn)
	*extra_rtrn= 0;

    keysymSet = *((unsigned long *)priv);
    kset = keysymSet&0xffffff;

    /* convert "dead" diacriticals for dumb applications */
    if ( (keysym&0xffffff00)== 0xfe00 ) {
	switch ( keysym ) {
	    case XK_dead_grave:		keysym = XK_grave; break;
	    case XK_dead_acute:		keysym = XK_acute; break;
	    case XK_dead_circumflex:	keysym = XK_asciicircum; break;
	    case XK_dead_tilde:		keysym = XK_asciitilde; break;
	    case XK_dead_macron:	keysym = XK_macron; break;
	    case XK_dead_breve:		keysym = XK_breve; break;
	    case XK_dead_abovedot:	keysym = XK_abovedot; break;
	    case XK_dead_diaeresis:	keysym = XK_diaeresis; break;
	    case XK_dead_abovering:	keysym = XK_degree; break;
	    case XK_dead_doubleacute:	keysym = XK_doubleacute; break;
	    case XK_dead_caron:		keysym = XK_caron; break;
	    case XK_dead_cedilla:	keysym = XK_cedilla; break;
	    case XK_dead_ogonek	:	keysym = XK_ogonek; break;
	    case XK_dead_iota:		keysym = XK_Greek_iota; break;
#ifdef XK_KATAKANA
	    case XK_dead_voiced_sound:	keysym = XK_voicedsound; break;
	    case XK_dead_semivoiced_sound:keysym = XK_semivoicedsound; break;
#endif
	}
    }

    isLatin1 = ((keysym&0xffffff00)==0);
    count = 0;

    if (nbytes<1)	buf= tbuf;
    else		buf= buffer;

    if ((keysym&0xffffff00)==0xff00) {
	return _XkbHandleSpecialSym(keysym, buf, nbytes, extra_rtrn);
    }
    return _XGetCharCode (keysymSet, keysym, buf, nbytes);
}

typedef struct _XkbToKS {
	unsigned	 prefix;
	char		*map;
} XkbToKS;

/*ARGSUSED*/
static KeySym
#if NeedFunctionPrototypes
_XkbKnownSetToKS(XPointer priv,char *buffer,int nbytes,Status *status)
#else
_XkbKnownSetToKS(priv,buffer,nbytes,status)
    XPointer priv;
    char *buffer;
    int nbytes;
    Status *status;
#endif
{
    if (nbytes!=1)
	return NoSymbol;
    if (((buffer[0]&0x80)==0)&&(buffer[0]>=32))
	return buffer[0];
    else if ((buffer[0]&0x7f)>=32) { 
	XkbToKS *map= (XkbToKS *)priv;
	if ( map ) {
	    if ( map->map )	return map->prefix|map->map[buffer[0]&0x7f];
	    else		return map->prefix|buffer[0];
	}
	return buffer[0];
    }
    return NoSymbol;
}

/*ARGSUSED*/
int 
#if NeedFunctionPrototypes
_XkbKSToThai (	XPointer 	priv,
		KeySym 		keysym,
		char *		buffer,
		int 		nbytes,
		int *		extra_rtrn)
#else
_XkbKSToThai (priv, keysym, buffer, nbytes, extra_rtrn)
    XPointer priv;
    KeySym keysym;
    char *buffer;
    int nbytes;
    int *extra_rtrn;
#endif
{

    if ((keysym&0xffffff00)==0xff00) {
        return _XkbHandleSpecialSym(keysym, buffer, nbytes, extra_rtrn);
    }
    else if (((keysym&0xffffff80)==0xd80)||((keysym&0xffffff80)==0)) {
        if (nbytes>0) {
            buffer[0]= (char)(keysym&0xff);
            if (nbytes>1)
                buffer[1]= '\0';
            return 1;
        }
	if (extra_rtrn)
	    *extra_rtrn= 1;
    }
    return 0;
}

/*ARGSUSED*/
static KeySym
#if NeedFunctionPrototypes
_XkbThaiToKS(XPointer priv,char *buffer,int nbytes,Status *status)
#else
_XkbThaiToKS(priv,buffer,nbytes,status)
    XPointer priv;
    char *buffer;
    int nbytes;
    Status *status;
#endif
{
    if (nbytes!=1)
        return NoSymbol;
    if (((buffer[0]&0x80)==0)&&(buffer[0]>=32))
        return buffer[0];
    else if ((buffer[0]&0x7f)>=32) {
        return 0xd00|buffer[0];
    }
    return NoSymbol;
}

static KeySym
#if NeedFunctionPrototypes
__XkbDefaultToUpper(KeySym sym)
#else
__XkbDefaultToUpper(sym)
    KeySym	sym;
#endif
{
    KeySym	lower,upper;

    XConvertCase(sym, &lower, &upper);
    return upper;
}

int _XkbKSToKoi8 (priv, keysym, buffer, nbytes, status)
    XPointer priv;
    KeySym keysym;
    char *buffer;
    int nbytes;
    Status *status;
{
    if ((keysym&0xffffff00)==0xff00) {
        return _XkbHandleSpecialSym(keysym, buffer, nbytes, status);
    }
    else if (((keysym&0xffffff80)==0x680)||((keysym&0xffffff80)==0)) {
	if (nbytes>0) {
	    if ( (keysym&0x80)==0 )
		 buffer[0] = keysym&0x7f;
	    else buffer[0] = _Xkoi8[keysym & 0x7f];
	    if (nbytes>1)
		buffer[1]= '\0';
	    return 1;
	}
    }
    return 0;
}

static KeySym
_XkbKoi8ToKS(priv,buffer,nbytes,status)
    XPointer priv;
    char *buffer;
    int nbytes;
    Status *status;
{
    if (nbytes!=1)
        return NoSymbol;
    if (((buffer[0]&0x80)==0)&&(buffer[0]>=32))
        return buffer[0];
    else if ((buffer[0]&0x7f)>=32) {
	register int i;
	for (i=0;i<_Xkoi8_size;i++) {
	    if (_Xkoi8[i]==buffer[0])
		return 0x680|i;
	}
    }
    return NoSymbol;
}

/***====================================================================***/


static XkbConverters RConst cvt_ascii = {
	_XkbKSToKnownSet,(XPointer)&WantLatin1,_XkbKnownSetToKS,NULL,__XkbDefaultToUpper
};

static XkbConverters RConst cvt_latin1 = {
	_XkbKSToKnownSet,(XPointer)&WantLatin1,_XkbKnownSetToKS,NULL,NULL
};

static XkbConverters RConst cvt_latin2 = {
	_XkbKSToKnownSet,(XPointer)&WantLatin2,_XkbKnownSetToKS,NULL,NULL
};

static XkbConverters RConst cvt_latin3 = {
	_XkbKSToKnownSet,(XPointer)&WantLatin3,_XkbKnownSetToKS,NULL,NULL
};

static XkbConverters RConst cvt_latin4 = {
	_XkbKSToKnownSet,(XPointer)&WantLatin4,_XkbKnownSetToKS,NULL,NULL
};

static XkbConverters RConst cvt_latin5 = {
	_XkbKSToKnownSet,(XPointer)&WantLatin5,_XkbKnownSetToKS,NULL,NULL
};

static XkbConverters RConst cvt_latin6 = {
	_XkbKSToKnownSet,(XPointer)&WantLatin6,_XkbKnownSetToKS,NULL,NULL
};

static XkbConverters RConst cvt_kana = {
	_XkbKSToKnownSet,(XPointer)&WantKana,_XkbKnownSetToKS,NULL,NULL
};

static XkbConverters RConst cvt_X0201 = {
	_XkbKSToKnownSet,(XPointer)&WantX0201,_XkbKnownSetToKS,NULL,NULL
};

static XkbConverters RConst cvt_Arabic = {
	_XkbKSToKnownSet,(XPointer)&WantArabic,_XkbKnownSetToKS,NULL,NULL
};

static XkbConverters RConst cvt_Cyrillic = {
	_XkbKSToKnownSet,(XPointer)&WantCyrillic,_XkbKnownSetToKS,NULL,NULL
};

static XkbConverters RConst cvt_Greek = {
	_XkbKSToKnownSet,(XPointer)&WantGreek,_XkbKnownSetToKS,NULL,NULL
};

static XkbConverters RConst cvt_APL = {
	_XkbKSToKnownSet,(XPointer)&WantAPL,_XkbKnownSetToKS,NULL,NULL
};

static XkbConverters RConst cvt_Hebrew = {
	_XkbKSToKnownSet,(XPointer)&WantHebrew,_XkbKnownSetToKS,NULL,NULL
};

static XkbConverters    cvt_Thai = {
        _XkbKSToThai, NULL, _XkbThaiToKS, NULL, NULL
};

static XkbConverters    cvt_Koi8 = {
        _XkbKSToKoi8, NULL, _XkbKoi8ToKS, NULL, NULL
};

static int
#if NeedFunctionPrototypes
Strcmp(char *str1, char *str2)
#else
Strcmp(str1, str2)
    char *str1, *str2;
#endif
{
    char str[256];
    char c, *s;

    if (strlen (str1) >= sizeof str) /* almost certain it's a mismatch */
	return 1;

    for (s = str; (c = *str1++); ) {
	if (isupper(c))
	    c = tolower(c);
	*s++ = c;
    }
    *s = '\0';
    return (strcmp(str, str2));
}

int 
#if NeedFunctionPrototypes
_XkbGetConverters(char *encoding_name, XkbConverters *cvt_rtrn)
#else
_XkbGetConverters(encoding_name, cvt_rtrn)
    char *encoding_name;
    XkbConverters *cvt_rtrn;
#endif
{
    if ( cvt_rtrn ) {
	if ( (encoding_name==NULL) || 
	     (Strcmp(encoding_name,"ascii")==0) ||
	     (Strcmp(encoding_name,"string")==0) )
	     *cvt_rtrn = cvt_ascii;
	else if (Strcmp(encoding_name,"iso8859-1")==0)
	     *cvt_rtrn = cvt_latin1;
	else if (Strcmp(encoding_name, "iso8859-2")==0)
	     *cvt_rtrn = cvt_latin2;
	else if (Strcmp(encoding_name, "iso8859-3")==0)
	     *cvt_rtrn = cvt_latin3;
	else if (Strcmp(encoding_name, "iso8859-4")==0)
	     *cvt_rtrn = cvt_latin4;
	else if (Strcmp(encoding_name, "iso8859-5")==0)
	     *cvt_rtrn = cvt_Cyrillic;
	else if (Strcmp(encoding_name, "iso8859-6")==0)
	     *cvt_rtrn = cvt_Arabic;
	else if (Strcmp(encoding_name, "iso8859-7")==0)
	     *cvt_rtrn = cvt_Greek;
	else if (Strcmp(encoding_name, "iso8859-8")==0)
	     *cvt_rtrn = cvt_Hebrew;
	else if (Strcmp(encoding_name, "iso8859-9")==0)
	     *cvt_rtrn = cvt_latin5;
	else if (Strcmp(encoding_name, "iso8859-10")==0)
	     *cvt_rtrn = cvt_latin6;
	else if (Strcmp(encoding_name, "apl")==0)
	     *cvt_rtrn = cvt_APL;
#if 0
	else if (Strcmp(encoding_name, "ja.euc")==0)
	     *cvt_rtrn = ???;
	else if (Strcmp(encoding_name, "ja.jis")==0)
	     *cvt_rtrn = ???;
	else if (Strcmp(encoding_name, "ja.sjis")==0)
	     *cvt_rtrn = ???;
#endif 
	else if (Strcmp(encoding_name, "jisx0201")==0) /* ??? */
	     *cvt_rtrn = cvt_X0201;
	else if (Strcmp(encoding_name, "kana")==0) /* ??? */
	     *cvt_rtrn = cvt_kana;
	else if ((Strcmp(encoding_name, "tactis")==0) ||
		 (Strcmp(encoding_name, "tis620.2533-1")==0))
	     *cvt_rtrn = cvt_Thai;
	else if (Strcmp(encoding_name, "koi8-r")==0)
	     *cvt_rtrn = cvt_Koi8;
	/* other codesets go here */
	else *cvt_rtrn = cvt_latin1;
	return 1;
    }
    *cvt_rtrn= cvt_latin1;
    return 1;
}

/***====================================================================***/

/* 
 * The function _XkbGetCharset seems to be missnamed as what it seems to
 * be used for is to determine the encoding-name for the locale. ???
 */

#ifdef XKB_EXTEND_LOOKUP_STRING

/* 
 * XKB_EXTEND_LOOKUP_STRING is not used by the SI. It is used by various
 * X Consortium/X Project Team members, so we leave it in the source as
 * an simplify integration by these companies.
 */

#define	CHARSET_FILE	"/usr/lib/X11/input/charsets"
static char *_XkbKnownLanguages = "c=ascii:da,de,en,es,fi,fr,is,it,nl,no,pt,sv=iso8859-1:hu,pl,cs=iso8859-2:bg,ru=iso8859-5:ar,ara=iso8859-6:el=iso8859-7:th,th_TH,th_TH.TACTIS=tactis";

char	*
_XkbGetCharset()
{
    /*
     * PAGE USAGE TUNING: explicitly initialize to move these to data
     * instead of bss
     */
    static char buf[100] = { 0 };
    char lang[256];
    char *start,*tmp,*end,*next,*set;
    char *country,*charset;
    char *locale;

    tmp = getenv( "_XKB_CHARSET" );
    if ( tmp )
	return tmp;
    locale = setlocale(LC_CTYPE,NULL);

    if ( locale == NULL )
	return NULL;

    for (tmp = lang; *tmp = *locale++; tmp++) {
	if (isupper(*tmp))
	    *tmp = tolower(*tmp);
    }
    country = strchr( lang, '_');
    if ( country ) {
	*country++ = '\0';
	charset = strchr( country, '.' );
	if ( charset )	*charset++ = '\0';
	if ( charset ) {
	    strncpy(buf,charset,99);
	    buf[99] = '\0';
	    return buf;
	}
    }
    else { 
	charset = NULL;
    }

    if ((tmp = getenv("_XKB_LOCALE_CHARSETS"))!=NULL) {
	start = _XkbAlloc(strlen(tmp) + 1);
	strcpy(start, tmp);
	tmp = start;
    } else {
	struct stat sbuf;
	FILE *file;
	if ( (stat(CHARSET_FILE,&sbuf)==0) && (sbuf.st_mode&S_IFREG) &&
	    (file = fopen(CHARSET_FILE,"r")) ) {
	    tmp = _XkbAlloc(sbuf.st_size+1);
	    if (tmp!=NULL) {
		sbuf.st_size = (long)fread(tmp,1,sbuf.st_size,file);
		tmp[sbuf.st_size] = '\0';
	    }
	    fclose(file);
	}
    }

    if ( tmp == NULL ) {
	tmp = _XkbAlloc(strlen(_XkbKnownLanguages) + 1);
	if (!tmp)
	    return NULL;
	strcpy(tmp, _XkbKnownLanguages);
    }
    start = tmp;
    do {
	if ( (set=strchr(tmp,'=')) == NULL )
	    break;
	*set++ = '\0';
	if ( (next=strchr(set,':')) != NULL ) 
	    *next++ = '\0';
	while ( tmp && *tmp ) {
	    if ( (end=strchr(tmp,',')) != NULL )
		*end++ = '\0';
	    if ( Strcmp( tmp, lang ) == 0 ) {
		strncpy(buf,set,100);
		buf[99] = '\0';
		Xfree(start);
		return buf;
	    }
	    tmp = end;
	}
	tmp = next;
    } while ( tmp && *tmp );
    Xfree(start);
    return NULL;
}
#else
char	*
_XkbGetCharset()
{
    char *tmp;
    XLCd lcd;

    tmp = getenv( "_XKB_CHARSET" );
    if ( tmp )
	return tmp;

    lcd = _XlcCurrentLC();
    if ( lcd )
	return XLC_PUBLIC(lcd,encoding_name);

    return NULL;
}
#endif

