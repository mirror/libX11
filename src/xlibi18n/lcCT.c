/* $Xorg: lcCT.c,v 1.4 2000/08/17 19:45:16 cpqbld Exp $ */
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
/*
 * Copyright 1995 by FUJITSU LIMITED
 * This is source code modified by FUJITSU LIMITED under the Joint
 * Development Agreement for the CDE/Motif PST.
 *
 * Modifier: Takanori Tateno   FUJITSU LIMITED
 *
 */

#include "Xlibint.h"
#include "XlcPubI.h"
#include <X11/Xos.h>
#include <stdio.h>

typedef struct _StateRec {
    XlcCharSet charset;
    XlcCharSet GL_charset;
    XlcCharSet GR_charset;
    XlcCharSet ext_seg_charset;
    int ext_seg_left;
} StateRec, *State;

typedef struct _CTDataRec {
    char *name;
    char *encoding;		/* Compound Text encoding */
} CTDataRec, *CTData;

typedef struct _CTInfoRec {
    XlcCharSet charset;
    int encoding_len;
    char *encoding;		/* Compound Text encoding */
    int ext_segment_len;
    char *ext_segment;		/* extended segment */
    struct _CTInfoRec *next;
} CTInfoRec, *CTInfo;

static CTDataRec default_ct_data[] =
{
    { "ISO8859-1:GL", "\033(B" },
    { "ISO8859-1:GR", "\033-A" },
    { "ISO8859-2:GR", "\033-B" },
    { "ISO8859-3:GR", "\033-C" },
    { "ISO8859-4:GR", "\033-D" },
    { "ISO8859-7:GR", "\033-F" },
    { "ISO8859-6:GR", "\033-G" },
    { "ISO8859-8:GR", "\033-H" },
    { "ISO8859-5:GR", "\033-L" },
    { "ISO8859-9:GR", "\033-M" },
    { "ISO8859-10:GR", "\033-V" },
    { "JISX0201.1976-0:GL", "\033(J" },
    { "JISX0201.1976-0:GR", "\033)I" },

    { "GB2312.1980-0:GL", "\033$(A" },
    { "GB2312.1980-0:GR", "\033$)A" },
    { "JISX0208.1983-0:GL", "\033$(B" },
    { "JISX0208.1983-0:GR", "\033$)B" },
    { "KSC5601.1987-0:GL", "\033$(C" },
    { "KSC5601.1987-0:GR", "\033$)C" },
#ifdef notdef
    { "JISX0212.1990-0:GL", "\033$(D" },
    { "JISX0212.1990-0:GR", "\033$)D" },
    { "CNS11643.1986-1:GL", "\033$(G" },
    { "CNS11643.1986-1:GR", "\033$)G" },
    { "CNS11643.1986-2:GL", "\033$(H" },
    { "CNS11643.1986-2:GR", "\033$)H" },
#endif
    { "TIS620.2533-1:GR", "\033-T"},
    { "ISO10646-1", "\033%B"},
    /* Non-Standard Character Set Encodings */
    { "KOI8-R:GR", "\033%/1\200\210koi8-r\002"},
    { "FCD8859-15:GR", "\033%/1\200\213fcd8859-15\002"},
} ; 

#define XctC0		0x0000
#define XctHT		0x0009
#define XctNL		0x000a
#define XctESC		0x001b
#define XctGL		0x0020
#define XctC1		0x0080
#define XctCSI		0x009b
#define XctGR		0x00a0

#define XctCntrlFunc	0x0023
#define XctMB		0x0024
#define XctOtherCoding	0x0025
#define XctGL94		0x0028
#define XctGR94		0x0029
#define XctGR96		0x002d
#define XctNonStandard	0x002f
#define XctIgnoreExt	0x0030
#define XctNotIgnoreExt	0x0031
#define XctLeftToRight	0x0031
#define XctRightToLeft	0x0032
#define XctDirection	0x005d
#define XctDirectionEnd	0x005d

#define XctGL94MB	0x2428
#define XctGR94MB	0x2429
#define XctExtSeg	0x252f
#define XctOtherSeg	0x2f00

#define XctESCSeq	0x1b00
#define XctCSISeq	0x9b00

#define SKIP_I(str)	while (*(str) >= 0x20 && *(str) <=  0x2f) (str)++;
#define SKIP_P(str)	while (*(str) >= 0x30 && *(str) <=  0x3f) (str)++;

typedef struct {
    XlcSide side;
    int char_size;
    int set_size;
    int ext_seg_length;
    int version;
    CTInfo ct_info;
} CTParseRec, *CTParse;

CTDataRec *default_ct_data_list()
{
	return(default_ct_data);
}

size_t default_ct_data_list_num()
{
	size_t num = sizeof(default_ct_data) / sizeof(CTDataRec);
	return(num);
}

static CTInfo ct_list = NULL;

static CTInfo
_XlcGetCTInfoFromEncoding(encoding, length)
    register char *encoding;
    register int length;
{
    register CTInfo ct_info;

    for (ct_info = ct_list; ct_info; ct_info = ct_info->next) {
	if (length >= ct_info->encoding_len) {
	    if (ct_info->ext_segment) {
		if (!strncmp(ct_info->encoding, encoding, 4) &&
		    !strncmp(ct_info->ext_segment, encoding + 6,
			     ct_info->ext_segment_len))
		    return ct_info;
	    } else if (!strncmp(ct_info->encoding, encoding,
				ct_info->encoding_len)) {
		return ct_info;
	    }
	}
    }

    return (CTInfo) NULL;
}

static unsigned int
_XlcParseCT(parse, text, length)
    register CTParse parse;
    char **text;
    int *length;
{
    unsigned int ret = 0;
    unsigned char ch;
    register unsigned char *str = (unsigned char *) *text;

    bzero((char *) parse, sizeof(CTParseRec));

    switch (ch = *str++) {
	case XctESC:
	    if (*str == XctOtherCoding && *(str + 1) == XctNonStandard
		&& *(str + 2) >= 0x30 && *(str + 2) <= 0x3f && *length >= 6) {

		/* non-standard encodings */
		parse->side = XlcGLGR;
		parse->set_size = 0;
		str += 2;
		if (*str <= 0x34) {
		    parse->char_size = *str - 0x30;
		    if (parse->char_size == 0) parse->char_size = 1;
		    ret = XctExtSeg;
		    parse->ct_info = _XlcGetCTInfoFromEncoding(*text, *length);
		} else
		    ret = XctOtherSeg;
		str++;
		parse->ext_seg_length = (*str - 128) * 128 + *(str + 1) - 128;
		str += 2;

		goto done;
	    } else if (*str == XctCntrlFunc && *length >= 4 &&
		       *(str + 1) >= 0x20 && *(str + 1) <= 0x2f &&
		       (*(str + 2) == XctIgnoreExt ||
			*(str + 2) == XctNotIgnoreExt)) {
		
		/* ignore extension or not */
		str++;
		parse->version = *str++ - 0x20;
		ret = *str++;

		goto done;
	    }
	    
	    if (*str == XctMB) {	/* multiple-byte sets */
		parse->char_size = 2;
		str++;
	    } else
		parse->char_size = 1;
	
	    switch (*str) {
		case XctGL94:
		    parse->side = XlcGL;
		    parse->set_size = 94;
		    ret = (parse->char_size == 1) ? XctGL94 : XctGL94MB;
		    break;
		case XctGR94:
		    parse->side = XlcGR;
		    parse->set_size = 94;
		    ret = (parse->char_size == 1) ? XctGR94 : XctGR94MB;
		    break;
		case XctGR96:
		    if (parse->char_size == 1) {
			parse->side = XlcGR;
			parse->set_size = 96;
			ret = XctGR96;
		    }
		    break;
	    }
	    if (ret) {
		str++;
		if (*str >= 0x24 && *str <= 0x2f) {	/* non-standard */
		    ret = 0;
		    str++;
		}
	    }

	    SKIP_I(str)

	    if (ret && *str < 0x40)			/* non-standard */
		ret = 0;

	    if (*str < 0x30 || *str > 0x7e || (char *) str - *text >= *length)
		break;
	    
	    if (ret == 0)
		ret = XctESCSeq;
	    else {
		if (parse->char_size == 2) {
		    if (*str >= 0x70)
			parse->char_size = 4;
		    else if (*str >= 0x60)
			parse->char_size = 3;
		}
		parse->ct_info = _XlcGetCTInfoFromEncoding(*text, *length);
	    }
	    str++;
	    goto done;
	case XctCSI:
	    /* direction */
	    if (*str == XctLeftToRight && *(str + 1) == XctDirection) {
		ret = XctLeftToRight;
		str += 2;
		goto done;
	    } else if (*str == XctRightToLeft && *(str + 1) == XctDirection) {
		ret = XctRightToLeft;
		str += 2;
		goto done;
	    } else if (*str == XctDirectionEnd) {
		ret = XctDirectionEnd;
		str++;
		goto done;
	    }

	    SKIP_P(str)
	    SKIP_I(str)

	    if (*str < 0x40 && *str > 0x7e)
		break;

	    ret = XctCSISeq;
	    str++;
	    goto done;
    }

    if (ch & 0x80) {
	if (ch < 0xa0)
	    ret = XctC1;
	else
	    ret = XctGR;
    } else {
	if (ch == XctHT || ch == XctNL)
	    ret = ch;
	else if (ch < 0x20)
	    ret = XctC0;
	else
	    ret = XctGL;
    }

    return ret;

done:
    *length -= (char *) str - *text;
    *text = (char *) str;

    return ret;
}

XlcCharSet
_XlcAddCT(name, encoding)
    char *name;
    char *encoding;
{
    CTInfo ct_info;
    XlcCharSet charset;
    CTParseRec parse;
    char *ct_ptr = encoding;
    int length;
    unsigned int type;

    length = strlen(encoding);

    switch (type = _XlcParseCT(&parse, &ct_ptr, &length)) {
	case XctExtSeg:
	case XctGL94:
	case XctGL94MB:
	case XctGR94:
	case XctGR94MB:
	case XctGR96:
	    if (parse.ct_info)		/* existed */
		return parse.ct_info->charset;
	    break;
	default:
	    return (XlcCharSet) NULL;
    }

    charset = _XlcCreateDefaultCharSet(name, encoding);
    if (charset == NULL)
	return (XlcCharSet) NULL;
    _XlcAddCharSet(charset);

    ct_info = (CTInfo) Xmalloc(sizeof(CTInfoRec));
    if (ct_info == NULL)
	return (XlcCharSet) NULL;
    
    ct_info->charset = charset;
    ct_info->encoding = charset->ct_sequence;
    ct_info->encoding_len = strlen(ct_info->encoding);
    if (type == XctExtSeg) {
	ct_info->ext_segment = ct_info->encoding + 6;
	ct_info->ext_segment_len = strlen(ct_info->ext_segment);
    } else {
	ct_info->ext_segment = NULL;
	ct_info->ext_segment_len = 0;
    }
    ct_info->next = ct_list;
    ct_list = ct_info;

    return charset;
}

static CTInfo
_XlcGetCTInfoFromCharSet(charset)
    register XlcCharSet charset;
{
    register CTInfo ct_info;

    for (ct_info = ct_list; ct_info; ct_info = ct_info->next)
	if (ct_info->charset == charset)
	    return ct_info;

    return (CTInfo) NULL;
}

Bool
_XlcParseCharSet(charset)
    XlcCharSet charset;
{
    CTParseRec parse;
    char *ptr, *bufp, buf[BUFSIZ];
    int length;

    if (charset->ct_sequence == NULL)
	return False;

    ptr = charset->ct_sequence;
    length = strlen(ptr);

    (void) _XlcParseCT(&parse, &ptr, &length);
	
    if (charset->name) {
	charset->xrm_name = XrmStringToQuark(charset->name);

	if ((length = strlen (charset->name)) < sizeof buf) bufp = buf;
	else bufp = Xmalloc (length + 1);

	if (bufp == NULL) return False;
	strcpy(bufp, charset->name);
	if ((ptr = strchr(bufp, ':')))
	    *ptr = '\0';
	charset->xrm_encoding_name = XrmStringToQuark(bufp);
	if (bufp != buf) Xfree (bufp);
	charset->encoding_name = XrmQuarkToString(charset->xrm_encoding_name);
    } else {
	charset->xrm_name = 0;
	charset->encoding_name = NULL;
	charset->xrm_encoding_name = 0;
    }

    charset->side = parse.side;
    charset->char_size = parse.char_size;
    charset->set_size = parse.set_size;

    return True;
}

static void init_converter();

Bool
_XlcInitCTInfo()
{
    register XlcCharSet charset;
    register CTData ct_data;
    register int num;

    if (ct_list == NULL) {
	num = sizeof(default_ct_data) / sizeof(CTDataRec);
	for (ct_data = default_ct_data; num-- > 0; ct_data++) {
	    charset = _XlcAddCT(ct_data->name, ct_data->encoding);
	    if (charset == NULL)
		continue;
	}
	init_converter();
    }

    return True;
}


static int
_XlcCheckCTSequence(state, ctext, ctext_len)
    State state;
    char **ctext;
    int *ctext_len;
{
    XlcCharSet charset;
    CTParseRec parse;
    CTInfo ct_info;
    int length;

    _XlcParseCT(&parse, ctext, ctext_len);

    ct_info = parse.ct_info;
    if (parse.ext_seg_length > 0) {	/* XctExtSeg or XctOtherSeg */
	if (ct_info) {
	    length = ct_info->ext_segment_len;
	    *ctext += length;
	    *ctext_len -= length;
	    state->ext_seg_left = parse.ext_seg_length - length;
	    state->ext_seg_charset = ct_info->charset;
	} else {
	    state->ext_seg_left = parse.ext_seg_length;
	    state->ext_seg_charset = NULL;
	}
    } else if (ct_info) {
	if ((charset = ct_info->charset)) {
	    if (charset->side == XlcGL)
		state->GL_charset = charset;
	    else if (charset->side == XlcGR)
		state->GR_charset = charset;
	}
    }

    return 0;
}


static void
init_state(conv)
    XlcConv conv;
{
    State state = (State) conv->state;
    static XlcCharSet GL_charset = NULL;
    static XlcCharSet GR_charset = NULL;

    if (GL_charset == NULL) {
	GL_charset = _XlcGetCharSet("ISO8859-1:GL");
	GR_charset = _XlcGetCharSet("ISO8859-1:GR");
    }

    state->GL_charset = state->charset = GL_charset;
    state->GR_charset = GR_charset;
    state->ext_seg_charset = NULL;
    state->ext_seg_left = 0;
}

static int
cttocs(conv, from, from_left, to, to_left, args, num_args)
    XlcConv conv;
    XPointer *from;
    int *from_left;
    XPointer *to;
    int *to_left;
    XPointer *args;
    int num_args;
{
    register State state = (State) conv->state;
    register unsigned char ch;
    int length;
    XlcCharSet charset = NULL;
    char *ctptr, *bufptr;
    int ctext_len, buf_len;

    ctptr = *((char **) from);
    bufptr = *((char **) to);
    ctext_len = *from_left;
    buf_len = *to_left;

    while (ctext_len > 0 && buf_len > 0) {
	if (state->ext_seg_left > 0) {
	    length = min(state->ext_seg_left, ctext_len);
	    length = min(length, buf_len);

	    ctext_len -= length;
	    state->ext_seg_left -= length;

	    if (state->ext_seg_charset) {
		charset = state->ext_seg_charset;
		buf_len -= length;
		if (charset->side == XlcGL) {
		    while (length-- > 0)
			*bufptr++ = *ctptr++ & 0x7f;
		} else if (charset->side == XlcGR) {
		    while (length-- > 0)
			*bufptr++ = *ctptr++ | 0x80;
		} else {
		    while (length-- > 0)
			*bufptr++ = *ctptr++;
		}

		if (state->ext_seg_left < 1)
		    state->ext_seg_charset = NULL;
	    }
	    break;
	}
	ch = *((unsigned char *) ctptr);
	if (ch == 0x1b || ch == 0x9b) {
	    length = _XlcCheckCTSequence(state, &ctptr, &ctext_len);
	    if (length < 0)
		return -1;
	    if (state->ext_seg_left > 0 && charset)
		break;
	} else {
	    if (charset) {
		if (charset != (ch & 0x80 ? state->GR_charset :
				state->GL_charset))
		    break;
	    } else
		charset = ch & 0x80 ? state->GR_charset : state->GL_charset;

	    if ((ch < 0x20 && ch != '\0' && ch != '\n' && ch != '\t') ||
		    (ch >= 0x80 && ch < 0xa0))
		return -1;

	    *bufptr++ = *ctptr++;
	    ctext_len--;
	    buf_len--;
	}
    }

    if (charset)
	state->charset = charset;
    if (num_args > 0)
	*((XlcCharSet *) args[0]) = state->charset;

    *from_left -= ctptr - *((char **) from);
    *from = (XPointer) ctptr;

    *to_left -= bufptr - *((char **) to);
    *to = (XPointer) bufptr;

    return 0;
}

static int
cstoct(conv, from, from_left, to, to_left, args, num_args)
    XlcConv conv;
    XPointer *from;
    int *from_left;
    XPointer *to;
    int *to_left;
    XPointer *args;
    int num_args;
{
    State state = (State) conv->state;
    XlcSide side;
    unsigned char min_ch, max_ch;
    register unsigned char ch;
    int length;
    CTInfo ct_info;
    XlcCharSet charset;
    char *csptr, *ctptr;
    int csstr_len, ct_len;

    if (num_args < 1)
	return -1;
    
    csptr = *((char **) from);
    ctptr = *((char **) to);
    csstr_len = *from_left;
    ct_len = *to_left;
    
    charset = (XlcCharSet) args[0];

    ct_info = _XlcGetCTInfoFromCharSet(charset);
    if (ct_info == NULL)
	return -1;

    side = charset->side;

    if (ct_info->ext_segment) {
	if (charset != state->ext_seg_charset && state->ext_seg_left < 1) {
	    length = ct_info->encoding_len;
	    if (ct_len < length)
		return -1;
	    strcpy(ctptr, ct_info->encoding);
	    ctptr[4] = ((ct_info->ext_segment_len + csstr_len) / 128) | 0x80;
	    ctptr[5] = ((ct_info->ext_segment_len + csstr_len) % 128) | 0x80;
	    ctptr += length;
	    ct_len -= length;
	    state->ext_seg_left = csstr_len;
	}
	length = min(state->ext_seg_left, csstr_len);
	state->ext_seg_left -= length;

	if (side == XlcGL) {
	    while (length-- > 0)
		*ctptr++ = *csptr++ & 0x7f;
	} else if (side == XlcGR) {
	    while (length-- > 0)
		*ctptr++ = *csptr++ | 0x80;
	} else {
	    while (length-- > 0)
		*ctptr++ = *csptr++;
	}
	state->ext_seg_charset = (state->ext_seg_left > 0) ? charset : NULL;
    } else {
	if ((side == XlcGR && charset != state->GR_charset) ||
	    (side == XlcGL && charset != state->GL_charset)) {

	    ct_len -= ct_info->encoding_len;
	    if (ct_len < 0)
		return -1;
	    strcpy(ctptr, ct_info->encoding);
	    ctptr += ct_info->encoding_len;
	}

	min_ch = 0x20;
	max_ch = 0x7f;

	if (charset->set_size == 94) {
	    max_ch--;
	    if (charset->char_size > 1 || side == XlcGR)
		min_ch++;
	}

	while (csstr_len > 0 && ct_len > 0) {
	    ch = *((unsigned char *) csptr++) & 0x7f;
	    if (ch < min_ch || ch > max_ch)
		if (ch != 0x00 && ch != 0x09 && ch != 0x0a && ch != 0x1b)
		    continue;	/* XXX */
	    if (side == XlcGL)
		*ctptr++ = ch & 0x7f;
	    else if (side == XlcGR)
		*ctptr++ = ch | 0x80;
	    else
		*ctptr++ = ch;
	    csstr_len--;
	    ct_len--;
	}
	if (side == XlcGR)
	    state->GR_charset = charset;
	else if (side == XlcGL)
	    state->GL_charset = charset;
    }

    *from_left -= csptr - *((char **) from);
    *from = (XPointer) csptr;

    *to_left -= ctptr - *((char **) to);
    *to = (XPointer) ctptr;

    return 0;
}

static int
strtocs(conv, from, from_left, to, to_left, args, num_args)
    XlcConv conv;
    XPointer *from;
    int *from_left;
    XPointer *to;
    int *to_left;
    XPointer *args;
    int num_args;
{
    State state = (State) conv->state;
    register char *src, *dst;
    unsigned char side;
    register int length;

    src = (char *) *from;
    dst = (char *) *to;

    length = min(*from_left, *to_left);
    side = *((unsigned char *) src) & 0x80;

    while (side == (*((unsigned char *) src) & 0x80) && length-- > 0)
	*dst++ = *src++;
    
    *from_left -= src - (char *) *from;
    *from = (XPointer) src;
    *to_left -= dst - (char *) *to;
    *to = (XPointer) dst;

    if (num_args > 0)
	*((XlcCharSet *)args[0]) = side ? state->GR_charset : state->GL_charset;

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
    State state = (State) conv->state;
    char *csptr, *string_ptr;
    int csstr_len, str_len;
    unsigned char ch;
    int unconv_num = 0;

    if (num_args < 1 || (state->GL_charset != (XlcCharSet) args[0] &&
	state->GR_charset != (XlcCharSet) args[0]))
	return -1;
    
    csptr = *((char **) from);
    string_ptr = *((char **) to);
    csstr_len = *from_left;
    str_len = *to_left;

    while (csstr_len-- > 0 && str_len > 0) {
	ch = *((unsigned char *) csptr++);
	if ((ch < 0x20 && ch != 0x00 && ch != 0x09 && ch != 0x0a) ||
	    ch == 0x7f || ((ch & 0x80) && ch < 0xa0)) {
	    unconv_num++;
	    continue;
	}
	*((unsigned char *) string_ptr++) = ch;
	str_len--;
    }

    *from_left -= csptr - *((char **) from);
    *from = (XPointer) csptr;

    *to_left -= string_ptr - *((char **) to);
    *to = (XPointer) string_ptr;

    return unconv_num;
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

    conv = (XlcConv) Xmalloc(sizeof(XlcConvRec));
    if (conv == NULL)
	return (XlcConv) NULL;

    conv->state = (XPointer) Xmalloc(sizeof(StateRec));
    if (conv->state == NULL)
	goto err;
    
    conv->methods = methods;

    init_state(conv);

    return conv;

err:
    close_converter(conv);

    return (XlcConv) NULL;
}

static XlcConvMethodsRec cttocs_methods = {
    close_converter,
    cttocs,
    init_state
} ;

static XlcConv
open_cttocs(from_lcd, from_type, to_lcd, to_type)
    XLCd from_lcd;
    char *from_type;
    XLCd to_lcd;
    char *to_type;
{
    return create_conv(&cttocs_methods);
}

static XlcConvMethodsRec cstoct_methods = {
    close_converter,
    cstoct,
    init_state
} ;

static XlcConv
open_cstoct(from_lcd, from_type, to_lcd, to_type)
    XLCd from_lcd;
    char *from_type;
    XLCd to_lcd;
    char *to_type;
{
    return create_conv(&cstoct_methods);
}

static XlcConvMethodsRec strtocs_methods = {
    close_converter,
    strtocs,
    init_state
} ;

static XlcConv
open_strtocs(from_lcd, from_type, to_lcd, to_type)
    XLCd from_lcd;
    char *from_type;
    XLCd to_lcd;
    char *to_type;
{
    return create_conv(&strtocs_methods);
}

static XlcConvMethodsRec cstostr_methods = {
    close_converter,
    cstostr,
    init_state
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

static void
init_converter()
{
    _XlcSetConverter((XLCd) NULL, XlcNCompoundText, (XLCd) NULL, XlcNCharSet,
		     open_cttocs);
    _XlcSetConverter((XLCd) NULL, XlcNString, (XLCd) NULL, XlcNCharSet,
		     open_strtocs);

    _XlcSetConverter((XLCd) NULL, XlcNCharSet, (XLCd) NULL, XlcNCompoundText,
		     open_cstoct);
    _XlcSetConverter((XLCd) NULL, XlcNCharSet, (XLCd) NULL, XlcNString,
		     open_cstostr);
}
