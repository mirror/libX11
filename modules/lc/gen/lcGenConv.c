/* $Xorg: lcGenConv.c,v 1.5 2000/08/17 19:45:17 cpqbld Exp $ */
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
 *  (c) Copyright 1995 FUJITSU LIMITED
 *  This is source code modified by FUJITSU LIMITED under the Joint
 *  Development Agreement for the CDE/Motif PST.
 *
 *   Modifier: Masayoshi Shimamura      FUJITSU LIMITED
 *
 */


#include "Xlibint.h"
#include "XlcGeneric.h"
#include <stdio.h>

#if !defined(X_NOT_STDC_ENV) && !defined(macII) && !defined(X_LOCALE)
#define STDCVT
#endif

typedef struct _CTDataRec {
    char *name;
    char *encoding;             /* Compound Text encoding */
} CTDataRec, *CTData;

extern CTDataRec *default_ct_data_list();
extern size_t 	 default_ct_data_list_num();

static CTDataRec directionality_data[] =
{
    { "BEGIN_LEFT-TO-RIGHT_TEXT", "\2331]" },
    { "BEGIN_RIGHT-TO-LEFT_TEXT", "\2332]" },
    { "END_OF_STRING", "\233]" },
};

typedef struct _StateRec {
    XLCd lcd;
    XlcCharSet charset;		/* charset of current state */
    XlcCharSet GL_charset;	/* charset of initial state in GL */
    XlcCharSet GR_charset;	/* charset of initial state in GR */
} StateRec, *State;

#define GR      0x80    /* begins right-side (non-ascii) region */
#define GL      0x7f    /* ends left-side (ascii) region        */
#define ESC	0x1b
#define CSI	0x9b
#define STX	0x02

#define isleftside(c)   (((c) & GR) ? 0 : 1)
#define isrightside(c)  (!isleftside(c))


/* Forward declarations for local routines. */
static int mbstocts();
static int ctstombs();


/* ------------------------------------------------------------------------- */
/*				Misc                                         */
/* ------------------------------------------------------------------------- */

static int
compare(src, encoding, length)
    register char *src;
    register char *encoding;
    register int length;
{
    char *start = src;

    while (length-- > 0) {
	if (*src++ != *encoding++)
	    return 0;
	if (*encoding == '\0')
	    return src - start;
    }

    return 0;
}

static unsigned long
conv_to_dest(conv, code)
    Conversion conv;
    unsigned long code;
{
    int i;
    int conv_num = conv->conv_num;
    FontScope convlist = conv->convlist;

    for (i = 0; i < conv_num; i++) {
  	if (convlist[i].start <= code && code <= convlist[i].end) {
	    switch (convlist[i].shift_direction) {
	    case '+':
	        return(code + convlist[i].shift);
	    case '-':
		return(code - convlist[i].shift);
            default:
		return(code);
	    }
	}
    }

    return(code);
}

static unsigned long
conv_to_source(conv, code)
    Conversion conv;
    unsigned long code;
{
    int i;
    int conv_num;
    FontScope convlist;
    unsigned long start_p;
    unsigned long start_m;
    unsigned long end_p;
    unsigned long end_m;

    if (!conv)
	return(code);

    conv_num = conv->conv_num;
    convlist = conv->convlist;

    for (i = 0; i < conv_num; i++) {
        start_p = convlist[i].start + convlist[i].shift;
        start_m = convlist[i].start - convlist[i].shift;
        end_p = convlist[i].end + convlist[i].shift;
        end_m = convlist[i].end - convlist[i].shift;

        switch (convlist[i].shift_direction) {
	case '+':
	    if (start_p <= code && code <= end_p)
		return(code - convlist[i].shift);
            break;
	case '-':
	    if (start_m <= code && code <= end_m)
		return(code + convlist[i].shift);
            break;
        default:
	    continue;
	}
    }

    return(code);
}

static unsigned long
mb_to_gi(mb, codeset)
    unsigned long mb;
    CodeSet codeset;
{
    int i;
    unsigned long mb_tmp, mask = 0;

    if (codeset->mbconv) {
	mb_tmp = conv_to_dest(codeset->mbconv, mb);
	if (mb_tmp != mb)
	    return(mb_tmp);
    }

    if (codeset->side == XlcC0 || codeset->side == XlcGL || 
	codeset->side == XlcC1 || codeset->side == XlcGR) {

        for (i = 0; i < codeset->length; i++)
	    mask = (mask << 8) | GL;
	mb = mb & mask;
    }

    return(mb);
}

static unsigned long
gi_to_mb(glyph_index, codeset)
    unsigned long glyph_index;
    CodeSet codeset;
{
    int i;
    unsigned long mask = 0;

    if (codeset->side == XlcC1 || codeset->side == XlcGR) {
        for (i = 0; i < codeset->length; i++)
	    mask = (mask << 8) | GR;
	glyph_index = glyph_index | mask;
    }

    if (codeset->mbconv)
        return( conv_to_source(codeset->mbconv, glyph_index) );

    return(glyph_index);
}

static Bool
gi_to_wc(lcd, glyph_index, codeset, wc)
    XLCd lcd;
    unsigned long glyph_index;
    CodeSet codeset;
    wchar_t *wc;
{
    unsigned char mask = 0;
    unsigned long wc_encoding = codeset->wc_encoding;
    int i, length = codeset->length;
    unsigned long wc_shift_bits = XLC_GENERIC(lcd, wc_shift_bits);

    for (i = wc_shift_bits; i > 0; i--)
	mask = (mask << 1) | 0x01;

    for (*wc = 0, length--; length >= 0; length--)
	*wc = (*wc << wc_shift_bits) | ((glyph_index >> (length * 8 )) & mask);

    *wc = *wc | wc_encoding;

    return(True);
}

static Bool
wc_to_gi(lcd, wc, glyph_index, codeset)
    XLCd lcd;
    wchar_t wc;
    unsigned long *glyph_index;
    CodeSet *codeset;
{
    int i;
    unsigned char mask = 0;
    unsigned long wc_encoding;
    unsigned long wc_encode_mask = XLC_GENERIC(lcd, wc_encode_mask);
    unsigned long wc_shift_bits = XLC_GENERIC(lcd, wc_shift_bits);
    int codeset_num = XLC_GENERIC(lcd, codeset_num);
    CodeSet *codeset_list = XLC_GENERIC(lcd, codeset_list);

    wc_encoding = wc & wc_encode_mask;
    for (*codeset = NULL, i = 0; i < codeset_num; i++) {
	if (wc_encoding == codeset_list[i]->wc_encoding) {
	    *codeset = codeset_list[i];
	    break;
        }
    }
    if (*codeset == NULL)
	return(False);

    for (i = wc_shift_bits; i > 0; i--)
	mask = (mask << 1) | 0x01;

    wc = wc & ~wc_encode_mask;
    for (*glyph_index = 0, i = (*codeset)->length - 1; i >= 0; i--)
	*glyph_index = (*glyph_index << 8) | 
		      ( ((unsigned long)wc >> (i * wc_shift_bits)) & mask );

    return(True);
}

static CodeSet
byteM_parse_codeset(lcd, inbufptr)
    XLCd lcd;
    XPointer inbufptr;
{
    unsigned char ch;
    CodeSet codeset;
    ByteInfoList byteM;
    ByteInfoListRec byteM_rec;
    ByteInfo byteinfo;
    ByteInfoRec byteinfo_rec;
    Bool hit;
    int i, j, k;

    int codeset_num               = XLC_GENERIC(lcd, codeset_num);
    CodeSet *codeset_list         = XLC_GENERIC(lcd, codeset_list);

    for (i = 0; i < codeset_num; i++) {
        codeset = codeset_list[i];
        byteM = codeset->byteM;
        if (codeset->side != XlcNONE || byteM == NULL)
	    continue;

        for (j = 0; j < codeset->length; j++) {
	    ch = *((unsigned char *)(inbufptr + j)); 
	    byteM_rec = byteM[j];
	    byteinfo = byteM_rec.byteinfo;

	    for (hit=False,k=0; k < byteM_rec.byteinfo_num; k++) {
	        byteinfo_rec = byteinfo[k];
	        if (byteinfo_rec.start <= ch && ch <= byteinfo_rec.end) {
	            hit = True;
		    break;
                }
            }

            if (!hit)
		break;
        }

        if (hit)
	    return(codeset);
    }

    return(NULL);
}                

static CodeSet
GLGR_parse_codeset(lcd, ch)
    XLCd lcd;
    unsigned char ch;
{
    int i;
    CodeSet initial_state_GL      = XLC_GENERIC(lcd, initial_state_GL);
    CodeSet initial_state_GR      = XLC_GENERIC(lcd, initial_state_GR);
    CodeSet *codeset_list         = XLC_GENERIC(lcd, codeset_list);
    int codeset_num               = XLC_GENERIC(lcd, codeset_num);

    XlcSide side = XlcGL;
    CodeSet codeset = initial_state_GL;

    if (isrightside(ch)) {
        side = XlcGR;
        codeset = initial_state_GR;
    }

    if (codeset)
	return(codeset);

    for (i = 0; i < codeset_num; i++) {
	codeset = codeset_list[i];
	if (codeset->side == side)
	    return(codeset);
    }
    
    return(NULL);
}

static XlcCharSet
gi_parse_charset(glyph_index, codeset)
    unsigned long glyph_index;
    CodeSet codeset;
{
    int i;
    XlcCharSet *charset_list = codeset->charset_list;
    int num_charsets = codeset->num_charsets;
    ExtdSegment ctextseg = codeset->ctextseg;
    XlcCharSet charset;
    int area_num;
    FontScope area;
    CTDataRec *default_ct_data = default_ct_data_list();
    size_t table_size = default_ct_data_list_num();

    /* lockup ct sequence */
    for (i = 0; i < num_charsets; i++) {
	charset = charset_list[i];
        if (*charset->ct_sequence != '\0')
	    break;
    }
    if (i >= num_charsets)
	return(NULL);

    /* Standard Character Set Encoding ? */
    for (i = 0; i < table_size; i++)
        if (compare(charset->ct_sequence, 
	       default_ct_data[i].encoding, strlen(charset->ct_sequence)))
	    goto check_extended_seg;

    return(charset);

check_extended_seg:
    if (!ctextseg)
        return(charset);

    area = ctextseg->area;
    area_num = ctextseg->area_num;

    for (i = 0; i < area_num; i++) {

        if (area[i].start <= glyph_index && glyph_index <= area[i].end) {

	    charset = ctextseg->charset;

            if (*charset->ct_sequence == '\0')
                return(NULL);

	    break;
	}
    }

    return(charset);
}

static Bool
ct_parse_csi(inbufptr, ctr_seq_len)
    XPointer inbufptr;
    int *ctr_seq_len;
{
    int i;
    int num = sizeof(directionality_data) / sizeof(directionality_data[0]);

    for (i = 0; i < num; i++) {
	if ( !(*ctr_seq_len = strlen(directionality_data[i].encoding)) )
	    continue;

	if ( strncmp(inbufptr, directionality_data[i].encoding, 
						*ctr_seq_len) == 0)
            return(True);
    }

    return(False);
}

static int
cmp_esc_sequence(inbufptr, ct_sequence, encoding_name)
    XPointer inbufptr;
    char *ct_sequence;
    char *encoding_name;
{
    int i, seq_len, name_len, total_len;
    unsigned char byte_m, byte_l;
    CTDataRec *default_ct_data = default_ct_data_list();
    size_t table_size = default_ct_data_list_num();

    /* check esc sequence */
    if ( !(seq_len = strlen(ct_sequence) ) )
	return(0);
    if ( strncmp(inbufptr, ct_sequence, seq_len) != 0)
	return(0);

    /* Standard Character Set Encoding ? */
    for (i = 0; i < table_size; i++) {
        if (compare(ct_sequence, 
                default_ct_data[i].encoding, strlen(ct_sequence)))
            return(seq_len);
    }

    /*
     *   Non-Standard Character Set Encoding
     *
     * +-----+-----+-----+-----+-----+-----+-----+----   ----+-----+-----+
     * |     esc sequence      |  M  |  L  |     encoding name     | STX |
     * +-----+-----+-----+-----+-----+-----+-----+----   ----+-----+-----+
     *           4bytes         1byte 1byte     variable length     1byte 
     * 	                   |                                         |
     * 	                   +-----------------------------------------+
     * 	                     name length  = ((M - 128) * 128) + (L - 128)
     */

    /* get length of encoding name */
    inbufptr += seq_len;
    byte_m = *inbufptr++;
    byte_l = *inbufptr++;
    name_len = ((byte_m - 128) * 128) + (byte_l - 128);
    total_len = seq_len + name_len;

    /* compare encoding names */
    if ( strncmp(inbufptr, encoding_name, name_len - 3) != 0 )
	return(0);

    /* check STX (Start of Text) */
    inbufptr = inbufptr + name_len - 3;
    if ( *inbufptr != STX )
	return(0);

    return(total_len);
}

static Bool
ct_parse_charset(lcd, inbufptr, charset, ctr_seq_len)
    XLCd lcd;
    XPointer inbufptr;
    XlcCharSet *charset;
    int *ctr_seq_len;
{
    int i, j;
    ExtdSegment ctextseg;
    int num_charsets;
    XlcCharSet *charset_list;
    CodeSet codeset;
    int codeset_num       = XLC_GENERIC(lcd, codeset_num);
    CodeSet *codeset_list = XLC_GENERIC(lcd, codeset_list);
    int segment_conv_num  = XLC_GENERIC(lcd, segment_conv_num);
    SegConv segment_conv  = XLC_GENERIC(lcd, segment_conv);


    /* get charset from XLC_XLOCALE by escape sequence */

    for (i = 0; i < codeset_num; i++) {
	codeset = codeset_list[i];

	num_charsets = codeset->num_charsets;
	charset_list = codeset->charset_list;
	ctextseg     = codeset->ctextseg;

	for (j = 0; j < num_charsets; j++) {
	    *charset = charset_list[j];
            if (( *ctr_seq_len = cmp_esc_sequence(inbufptr, 
		    (*charset)->ct_sequence, (*charset)->encoding_name) ))
		return(True);
	}

	if (ctextseg) {
	    *charset = ctextseg->charset;
            if (( *ctr_seq_len = cmp_esc_sequence(inbufptr, 
		    (*charset)->ct_sequence, (*charset)->encoding_name) ))
		return(True);
	}
    }

    /* get charset from XLC_SEGMENTCONVERSION by escape sequence */

    if (!segment_conv)
	return(False);

    for (i = 0; i < segment_conv_num; i++) {
	*charset = segment_conv[i].source;
        if (( *ctr_seq_len = cmp_esc_sequence(inbufptr, 
	        (*charset)->ct_sequence, (*charset)->encoding_name) ))
	    return(True);
	*charset = segment_conv[i].dest;
        if (( *ctr_seq_len = cmp_esc_sequence(inbufptr, 
	        (*charset)->ct_sequence, (*charset)->encoding_name) ))
	    return(True);
    }
    
    return(False);
}

static Bool
segment_conversion(lcd, charset, glyph_index)
    XLCd lcd;
    XlcCharSet *charset;
    unsigned long *glyph_index;
{
    int i;
    int segment_conv_num = XLC_GENERIC(lcd, segment_conv_num);
    SegConv segment_conv = XLC_GENERIC(lcd, segment_conv);
    FontScopeRec range;
    ConversionRec conv_rec;

    if (!segment_conv)
	return(True);

    for (i = 0; i < segment_conv_num; i++) {
	if (segment_conv[i].source == *charset)
	    break;
    }

    if (i >= segment_conv_num)
	return(True);

    range = segment_conv[i].range;
    if (*glyph_index < range.start || range.end < *glyph_index)
	return(True);
    
    *charset = segment_conv[i].dest;
    conv_rec.conv_num = segment_conv[i].conv_num;
    conv_rec.convlist = segment_conv[i].conv;
    *glyph_index = conv_to_dest(&conv_rec, *glyph_index);

    return(True);
}

CodeSet
_XlcGetCodeSetFromName(lcd, name)
    XLCd lcd;
    char *name;
{
    int i, j;
    XlcCharSet charset;
    int num_charsets;
    XlcCharSet *charset_list;
    CodeSet codeset;

    int codeset_num       = XLC_GENERIC(lcd, codeset_num);
    CodeSet *codeset_list = XLC_GENERIC(lcd, codeset_list);

    for (i = 0; i < codeset_num; i++) {
        codeset = codeset_list[i];

        num_charsets = codeset->num_charsets;
        charset_list = codeset->charset_list;

        for (j = 0; j < num_charsets; j++) {
            charset = charset_list[j];

            if (!strlen(charset->name))
                continue;
            if ( strcmp(charset->name, name) == 0)
                return(codeset);
        }
    }

    return(NULL);
}

static Bool
_XlcGetCodeSetFromCharSet(lcd, charset, codeset, glyph_index)
    XLCd lcd;
    XlcCharSet charset;
    CodeSet *codeset;
    unsigned long *glyph_index;
{
    int i, j, num;
    CodeSet *codeset_list = XLC_GENERIC(lcd, codeset_list);
    XlcCharSet *charset_list;
    int codeset_num, num_charsets;
    Conversion ctconv;
    unsigned long glyph_index_tmp;
    ExtdSegment ctextseg;
    CTDataRec *default_ct_data = default_ct_data_list();
    size_t table_size = default_ct_data_list_num();

    codeset_num = XLC_GENERIC(lcd, codeset_num);

    for (num = 0 ; num < codeset_num; num++) {
        *codeset = codeset_list[num];
        ctconv = (*codeset)->ctconv;
	ctextseg = (*codeset)->ctextseg;

        num_charsets = (*codeset)->num_charsets;
        charset_list = (*codeset)->charset_list;

        glyph_index_tmp = conv_to_source(ctconv, *glyph_index);

        /* Standard Character Set Encoding ? */
        for (i = 0; i < table_size; i++) {
            if (compare(charset->ct_sequence, 
	           default_ct_data[i].encoding, strlen(charset->ct_sequence)))
	        break;
        }

        if (i < table_size) {

            /* Standard Character Set Encoding */
	    if (glyph_index_tmp == *glyph_index) {
                for (j = 0; j < num_charsets; j++) {
                    if (charset_list[j] == charset) {
                        goto end_loop;
                    }
                }
            }

	} else {

            /* Non-Standard Character Set Encoding */
            for (j = 0; j < num_charsets; j++) {
                if (charset_list[j] == charset) {
                    goto end_loop;
                }
            }

            if (glyph_index_tmp != *glyph_index) {
		if (ctextseg->charset == charset) {
		    goto end_loop;
                }
            }

	}

    }

end_loop:
    if (num < codeset_num) {
	*glyph_index = glyph_index_tmp;
	return(True);
    }

    return(False);
}

static Bool
check_string_encoding(codeset)
    CodeSet codeset;
{
    int i;
    XlcCharSet charset;
    XlcCharSet *charset_list = codeset->charset_list;
    int num_charsets = codeset->num_charsets;

    for (i = 0; i < num_charsets; i++) {
        charset = charset_list[i];
        if ( strcmp(charset->encoding_name, "ISO8859-1") == 0 || 
	     charset->string_encoding)
            return(True);
    }

    return(False);
}

static void
output_ulong_value(outbufptr, code, length, side)
    XPointer outbufptr;
    unsigned long code;
    int length;
    XlcSide side;
{
    int i;
    unsigned long mask = 0xff;

    for (i = 0; i < length; i++) {
	*outbufptr = ( code >> (8 * (length - i - 1)) ) & mask;

	if (side == XlcC0 || side == XlcGL) {
	    *outbufptr = *outbufptr & GL;
	} else if (side == XlcC1 || side == XlcGR) {
	    *outbufptr = *outbufptr | GR;
	}

	outbufptr++;
    }
}

/* -------------------------------------------------------------------------- */
/*				Init                                          */
/* -------------------------------------------------------------------------- */

static void
init_state(conv)
    XlcConv conv;
{
    register State state = (State) conv->state;

    /* for CT */
    state->charset = NULL;
    state->GL_charset = _XlcGetCharSet("ISO8859-1:GL");
    state->GR_charset = _XlcGetCharSet("ISO8859-1:GR");
}

/* -------------------------------------------------------------------------- */
/*				Convert                                       */
/* -------------------------------------------------------------------------- */

static int
mbstowcs_org(conv, from, from_left, to, to_left, args, num_args)
    XlcConv conv;
    XPointer *from;
    int *from_left;
    XPointer *to;
    int *to_left;
    XPointer *args;
    int num_args;
{
    State state = (State) conv->state;
    XLCd lcd = state->lcd;

    unsigned char ch;
    unsigned long mb = 0;
    wchar_t wc;

    int length = 0, len_left = 0;
    int unconv_num = 0;
    int num;
    Bool ss_flag = 0;

    CodeSet codeset = NULL;
    ParseInfo parse_info;

    XPointer inbufptr = *from;
    wchar_t *outbufptr = (wchar_t *) *to;
    int from_size = *from_left;

    unsigned char *mb_parse_table = XLC_GENERIC(lcd, mb_parse_table);
    ParseInfo *mb_parse_list      = XLC_GENERIC(lcd, mb_parse_list);


    if (*from_left > *to_left)
        *from_left = *to_left;

    while (*from_left && *to_left) {

	ch = *inbufptr++;
	(*from_left)--;

	/* null ? */
	if (!ch) {
            if (outbufptr) {*outbufptr++ = L'\0';}
	    (*to_left)--;

	    /* error check */
            if (len_left) {
	        unconv_num += (length - len_left);
		len_left = 0;
            }

	    continue;
	}

	/* same mb char data */
        if (len_left)
	    goto output_one_wc;

        /* next mb char data for single shift ? */
	if (mb_parse_table) {
	    if ((num = mb_parse_table[ch]) > 0) {
		parse_info = mb_parse_list[num - 1];

                codeset = parse_info->codeset;
		length = len_left = codeset->length;
		mb = 0;
		ss_flag = 1;

		continue;
	    }
        } 
    
	/* next mb char data for byteM ? */
	if ((codeset = byteM_parse_codeset(lcd, (inbufptr - 1))))
	    goto next_mb_char;

	/* next mb char data for GL or GR side ? */
	if ((codeset = GLGR_parse_codeset(lcd, ch)))
	    goto next_mb_char;
	    
        /* can't find codeset for the ch */
        unconv_num++;
        continue;

next_mb_char:
        length = len_left = codeset->length;
	mb = 0;
	ss_flag = 0;

output_one_wc:
	mb = (mb << 8) | ch;  /* 1 byte left shift */
	len_left--;

        /* last of one mb char data */
        if (!len_left) {
            gi_to_wc(lcd, mb_to_gi(mb, codeset), codeset, &wc);
            if (outbufptr) {*outbufptr++ = wc;}
	    (*to_left)--;
        }

    } /* end of while */

    /* error check on last char */
    if (len_left) {
	inbufptr -= (length - len_left + ss_flag);
	(*from_left) += (length - len_left + ss_flag);
	unconv_num += (length - len_left + ss_flag);
    }

    *from = *from + from_size;
    *from_left = 0;
    *to = (XPointer)outbufptr;

    return unconv_num;
}

static int
stdc_mbstowcs(conv, from, from_left, to, to_left, args, num_args)
    XlcConv conv;
    XPointer *from;
    int *from_left;
    XPointer *to;
    int *to_left;
    XPointer *args;
    int num_args;
{
    char *src = *((char **) from);
    wchar_t *dst = *((wchar_t **) to);
    int src_left = *from_left;
    int dst_left = *to_left;
    int length, unconv_num = 0;

    while (src_left > 0 && dst_left > 0) {
	length = mbtowc(dst, src, src_left);

	if (length > 0) {
	    src += length;
	    src_left -= length;
	    if (dst)
	        dst++;
	    dst_left--;
	} else if (length < 0) {
	    src++;
	    src_left--;
	    unconv_num++;
        } else {
            /* null ? */
            src++;
            src_left--;
            if (dst) 
                *dst++ = L'\0';
            dst_left--;
        }
    }

    *from = (XPointer) src;
    if (dst)
	*to = (XPointer) dst;
    *from_left = src_left;
    *to_left = dst_left;

    return unconv_num;
}

static int
wcstombs_org(conv, from, from_left, to, to_left, args, num_args)
    XlcConv conv;
    XPointer *from;
    int *from_left;
    XPointer *to;
    int *to_left;
    XPointer *args;
    int num_args;
{
    State state = (State) conv->state;
    XLCd lcd = state->lcd;

    char *encoding;
    unsigned long mb, glyph_index;
    wchar_t wc;

    int length;
    int unconv_num = 0;

    CodeSet codeset;

    wchar_t *inbufptr = (wchar_t *) *from;
    XPointer outbufptr = *to;
    int from_size = *from_left;
    
    char *default_string = XLC_PUBLIC(lcd, default_string);
    int defstr_len = strlen(default_string);


    if (*from_left > *to_left)
        *from_left = *to_left;

    while (*from_left && *to_left) {

        wc = *inbufptr++;
        (*from_left)--;

        /* null ? */
        if (!wc) {
            if (outbufptr) {*outbufptr++ = '\0';}
            (*to_left)--;

            continue;
        }

        /* convert */
	if ( !wc_to_gi(lcd, wc, &glyph_index, &codeset) ) {

	    /* output default_string of XDefaultString() */
            if (*to_left < defstr_len)
		break;
	    if (outbufptr) {strncpy((char *)outbufptr, default_string, defstr_len);}

	    if (outbufptr) {outbufptr += defstr_len;}
	    (*to_left) -= defstr_len;

            unconv_num++;

        } else {
            mb = gi_to_mb(glyph_index, codeset);
	    if (codeset->parse_info) {

		/* output shift sequence */
		encoding = codeset->parse_info->encoding;
                length = strlen(encoding);
                if (*to_left < length)
		    break;
	        if (outbufptr) {strncpy((char *)outbufptr, encoding, length);}

	        if (outbufptr) {outbufptr += length;}
	        (*to_left) -= length;
            }

            /* output characters */
	    length = codeset->length;
            if (*to_left < length)
		break;

	    if (outbufptr) {
		output_ulong_value(outbufptr, mb, length, XlcNONE);
	        outbufptr += length;
	    }

	    (*to_left) -= length;
        }

    } /* end of while */

    *from = *from + from_size;
    *from_left = 0;
    *to = outbufptr;

    return unconv_num;
}

static int
stdc_wcstombs(conv, from, from_left, to, to_left, args, num_args)
    XlcConv conv;
    XPointer *from;
    int *from_left;
    XPointer *to;
    int *to_left;
    XPointer *args;
    int num_args;
{
    wchar_t *src = *((wchar_t **) from);
    char *dst = *((char **) to);
    int src_left = *from_left;
    int dst_left = *to_left;
    int length, unconv_num = 0;

    while (src_left > 0 && dst_left >= MB_CUR_MAX) {
	length = wctomb(dst, *src);		/* XXX */

        if (length > 0) {
	    src++;
	    src_left--;
	    if (dst) 
		dst += length;
	    dst_left -= length;
	} else if (length < 0) {
	    src++;
	    src_left--;
	    unconv_num++;
	} 
    }

    *from = (XPointer) src;
    if (dst)
      *to = (XPointer) dst;
    *from_left = src_left;
    *to_left = dst_left;

    return unconv_num;
}

static int
wcstocts(conv, from, from_left, to, to_left, args, num_args)
    XlcConv conv;
    XPointer *from;
    int *from_left;
    XPointer *to;
    int *to_left;
    XPointer *args;
    int num_args;
{
    State state = (State) conv->state;
    XLCd lcd = state->lcd;
    CTDataRec *default_ct_data = default_ct_data_list();
    size_t table_size = default_ct_data_list_num();

    unsigned long glyph_index;
    wchar_t wc;

    int i, total_len, seq_len, name_len;
    int unconv_num = 0;
    Bool first_flag = True, standard_flag;
    XlcSide side;

    CodeSet codeset;
    XlcCharSet charset, old_charset = NULL;
    char *ct_sequence;

    wchar_t *inbufptr = (wchar_t *) *from;
    XPointer outbufptr = *to;
    int from_size = *from_left;


    if (*from_left > *to_left)
        *from_left = *to_left;

    while (*from_left && *to_left) {

        wc = *inbufptr++;
        (*from_left)--;

        /* null ? */
        if (!wc) {
            if (outbufptr) {*outbufptr++ = '\0';}
            (*to_left)--;

            continue;
        }

        /* convert */
	if ( !wc_to_gi(lcd, wc, &glyph_index, &codeset) ) {
            unconv_num++;
	    continue;
        }

        /* parse charset */
        if ( !(charset = gi_parse_charset(glyph_index, codeset)) ) {
            unconv_num++;
	    continue;
        }

        /* Standard Character Set Encoding ? */
	standard_flag = False;
        for (i = 0; i < table_size; i++)
            if (compare(charset->ct_sequence, 
                    default_ct_data[i].encoding, strlen(charset->ct_sequence)))
                standard_flag = True;

        /*
         *   Non-Standard Character Set Encoding
         *
         * +-----+-----+-----+-----+-----+-----+-----+----   ----+-----+-----+
         * |     esc sequence      |  M  |  L  |     encoding name     | STX |
         * +-----+-----+-----+-----+-----+-----+-----+----   ----+-----+-----+
         *           4bytes         1byte 1byte     variable length     1byte 
         * 	                   |                                         |
         * 	                   +-----------------------------------------+
         * 	                     name length  = ((M - 128) * 128) + (L - 128)
         */

        /* make encoding data */
	ct_sequence = charset->ct_sequence;
	side = charset->side;
        seq_len = strlen(charset->ct_sequence);
	if (standard_flag) {
            name_len = 0;
	    total_len = seq_len;
	} else {
            name_len = 2 + strlen(charset->encoding_name) + 1;
	    total_len = seq_len + name_len;
	}

        /* output escape sequence of CT */
	if ( (charset != old_charset) &&
	    !(first_flag && (strcmp(charset->encoding_name,"ISO8859-1")==0))) {

	    if (*to_left < total_len + 1) {
                unconv_num++;
	        break;
	    }

	    if (outbufptr) {
	        strcpy((char *)outbufptr, charset->ct_sequence);
		outbufptr += seq_len;

                if (!standard_flag) {
		    *outbufptr++ = name_len / 128 + 128;
		    *outbufptr++ = name_len % 128 + 128;
	            strcpy((char *)outbufptr, charset->encoding_name);
		    outbufptr = outbufptr + name_len - 2 - 1;
		    *outbufptr++ = STX;
                }
	    }

	    (*to_left) -= total_len;

	    first_flag = False;
	    old_charset = charset;
	}

        /* output glyph index */
	if (codeset->ctconv)
            glyph_index = conv_to_dest(codeset->ctconv, glyph_index);
        if (*to_left < charset->char_size) {
            unconv_num++;
	    break;
        }

	if (outbufptr) {
	   output_ulong_value(outbufptr, glyph_index, charset->char_size, side);
	   outbufptr += charset->char_size;
	}

	(*to_left) -= charset->char_size;

    } /* end of while */

    *from = *from + from_size;
    *from_left = 0;
    *to = outbufptr;

    return unconv_num;
}

static int
stdc_wcstocts(conv, from, from_left, to, to_left, args, num_args)
    XlcConv conv;
    XPointer *from;
    int *from_left;
    XPointer *to;
    int *to_left;
    XPointer *args;
    int num_args;
{
    XPointer buf = Xmalloc((*from_left) * MB_CUR_MAX);
    XPointer buf_ptr1 = buf;
    int buf_left1 = (*from_left) * MB_CUR_MAX;
    XPointer buf_ptr2 = buf_ptr1;
    int buf_left2;
    int unconv_num1 = 0, unconv_num2 = 0;

    unconv_num1 = stdc_wcstombs(conv, 
		from, from_left, &buf_ptr1, &buf_left1, args, num_args);
    if (unconv_num1 < 0)
        goto ret;

    buf_left2 = buf_ptr1 - buf_ptr2;

    unconv_num2 = mbstocts(conv, 
		&buf_ptr2, &buf_left2, to, to_left, args, num_args);
    if (unconv_num2 < 0)
        goto ret;

ret:
    if (buf)
	Xfree((char *)buf);

    return (unconv_num1 + unconv_num2);
}

static int
ctstowcs(conv, from, from_left, to, to_left, args, num_args)
    XlcConv conv;
    XPointer *from;
    int *from_left;
    XPointer *to;
    int *to_left;
    XPointer *args;
    int num_args;
{
    State state = (State) conv->state;
    XLCd lcd = state->lcd;

    unsigned char ch;
    unsigned long glyph_index = 0;
    wchar_t wc;

    int ctr_seq_len = 0, gi_len_left = 0, gi_len = 0;
    int unconv_num = 0;

    CodeSet codeset = NULL;
    XlcCharSet charset_tmp;

    XPointer inbufptr = *from;
    wchar_t *outbufptr = (wchar_t *) *to;
    int from_size = *from_left;


    init_state(conv);

    if (from == NULL || *from == NULL) {
	init_state(conv);
        return( 0 );
    }

    if (*from_left > *to_left)
        *from_left = *to_left;

    while (*from_left && *to_left) {

	ch = *inbufptr++;
	(*from_left)--;

	/* null ? */
	if (!ch) {
            if (outbufptr) {*outbufptr++ = L'\0';}
	    (*to_left)--;

	    /* error check */
            if (gi_len_left) {
	        unconv_num += (gi_len - gi_len_left);
		gi_len_left = 0;
            }

	    continue;
	}

	/* same glyph_index data */
        if (gi_len_left)
	    goto output_one_wc;

        /* control sequence ? */
        if (ch == CSI) {
            if ( !ct_parse_csi(inbufptr - 1, &ctr_seq_len) )
	        goto skip_the_seg;

	    if (*from_left < ctr_seq_len) {
		inbufptr--;
		(*from_left)++;
		unconv_num += *from_left;
		break;
	    }

            /* skip the control sequence */
	    inbufptr += (ctr_seq_len - 1);
	    *from_left -= (ctr_seq_len - 1);

            continue;
	}

        /* escape sequence ? */
        if (ch == ESC) {
	    if ( !ct_parse_charset(lcd, 
			inbufptr - 1, &state->charset, &ctr_seq_len) )
		goto skip_the_seg;

	    if (state->charset->side == XlcC0 || 
		state->charset->side == XlcGL)
	      {
		state->GL_charset = state->charset;
	      }
	    else if (state->charset->side == XlcC1 || 
		     state->charset->side == XlcGR)
	      {
		state->GR_charset = state->charset;
	      }	
	    else if (state->charset->side == XlcGLGR)
	      {
		state->GL_charset = state->charset;
		state->GR_charset = state->charset;
	      }	

	    if (*from_left < ctr_seq_len) {
		inbufptr--;
		(*from_left)++;
		unconv_num += *from_left;
		break;
	    }

            /* skip the escape sequence */
	    inbufptr += (ctr_seq_len - 1);
	    *from_left -= (ctr_seq_len - 1);

            continue;
        } 

 	/* check current state */
	if (isleftside(ch))
	  state->charset = state->GL_charset;
	else
	  state->charset = state->GR_charset;

	gi_len = gi_len_left = state->charset->char_size;
	glyph_index = 0;

output_one_wc:
        if (state->charset->side == XlcC1 || state->charset->side == XlcGR)
            glyph_index = (glyph_index << 8) | (ch & GL);
        else
            glyph_index = (glyph_index << 8) | ch;

	gi_len_left--;

        /* last of one glyph_index data */
        if (!gi_len_left) {

	    /* segment conversion */
	    charset_tmp = state->charset;
	    if ( !segment_conversion(lcd, &charset_tmp, &glyph_index) ) {
		unconv_num += gi_len;
		continue;
            }

            /* get codeset */
            if ( !_XlcGetCodeSetFromCharSet(lcd, charset_tmp, 
						&codeset, &glyph_index) ) {
		unconv_num += gi_len;
		continue;
            }

            /* convert glyph index to wicd char */
            gi_to_wc(lcd, glyph_index, codeset, &wc);
            if (outbufptr) {*outbufptr++ = wc;}
	    (*to_left)--;
        }

        continue;

skip_the_seg:
	/* skip until next escape or control sequence */
        while ( *from_left ) {
	    ch = *inbufptr++;
	    (*from_left)--;
	    unconv_num++;

            if (ch == ESC || ch == CSI) {
		inbufptr--;
		(*from_left)++;
		unconv_num--;
		break;
	    }
        }

        if ( !(*from_left) )
	    break;

    } /* end of while */

    /* error check on last char */
    if (gi_len_left) {
	inbufptr -= (gi_len - gi_len_left);
	(*from_left) += (gi_len - gi_len_left);
	unconv_num += (gi_len - gi_len_left);
    }

    *from = *from + from_size;
    *from_left = 0;
    *to = (XPointer)outbufptr;

    return unconv_num;
}

static int
stdc_ctstowcs(conv, from, from_left, to, to_left, args, num_args)
    XlcConv conv;
    XPointer *from;
    int *from_left;
    XPointer *to;
    int *to_left;
    XPointer *args;
    int num_args;
{
    XPointer buf = Xmalloc((*from_left) * MB_CUR_MAX);
    XPointer buf_ptr1 = buf;
    int buf_left1 = (*from_left) * MB_CUR_MAX;
    XPointer buf_ptr2 = buf_ptr1;
    int buf_left2;
    int unconv_num1 = 0, unconv_num2 = 0;

    unconv_num1 = ctstombs(conv, 
		from, from_left, &buf_ptr1, &buf_left1, args, num_args);
    if (unconv_num1 < 0)
        goto ret;

    buf_left2 = buf_ptr1 - buf_ptr2;

    unconv_num2 = stdc_mbstowcs(conv, 
		&buf_ptr2, &buf_left2, to, to_left, args, num_args);
    if (unconv_num2 < 0)
        goto ret;

ret:
    if (buf)
	Xfree((char *)buf);

    return (unconv_num1 + unconv_num2);
}

static int
mbstocts(conv, from, from_left, to, to_left, args, num_args)
    XlcConv conv;
    XPointer *from;
    int *from_left;
    XPointer *to;
    int *to_left;
    XPointer *args;
    int num_args;
{
    XPointer buf = Xmalloc((*from_left) * sizeof(wchar_t));
    XPointer buf_ptr1 = buf;
    int buf_left1 = (*from_left);
    XPointer buf_ptr2 = buf_ptr1;
    int buf_left2;
    int unconv_num1 = 0, unconv_num2 = 0;

    unconv_num1 = mbstowcs_org(conv, 
		from, from_left, &buf_ptr1, &buf_left1, args, num_args);
    if (unconv_num1 < 0)
        goto ret;

    buf_left2 = (buf_ptr1 - buf_ptr2) / sizeof(wchar_t);

    unconv_num2 += wcstocts(conv, 
		&buf_ptr2, &buf_left2, to, to_left, args, num_args);
    if (unconv_num2 < 0)
        goto ret;

ret:
    if (buf)
	Xfree((char *)buf);

    return (unconv_num1 + unconv_num2);
}

static int
mbstostr(conv, from, from_left, to, to_left, args, num_args)
    XlcConv conv;
    XPointer *from;
    int *from_left;
    XPointer *to;
    int *to_left;
    XPointer *args;
    int num_args;
{
    State state = (State) conv->state;
    XLCd lcd = state->lcd;

    unsigned char ch;
    unsigned long mb = 0;


    int length = 0, len_left = 0;
    int unconv_num = 0;
    int num;
    Bool ss_flag = 0;

    CodeSet codeset = NULL;
    ParseInfo parse_info;

    XPointer inbufptr = *from;
    XPointer outbufptr = *to;
    int from_size = *from_left;

    unsigned char *mb_parse_table = XLC_GENERIC(lcd, mb_parse_table);
    ParseInfo *mb_parse_list      = XLC_GENERIC(lcd, mb_parse_list);


    if (*from_left > *to_left)
        *from_left = *to_left;

    while (*from_left && *to_left) {

	ch = *inbufptr++;
	(*from_left)--;

	/* null ? */
	if (!ch) {
            if (outbufptr) {*outbufptr++ = '\0';}
	    (*to_left)--;

            /* error check */
            if (len_left) {
                unconv_num += (length - len_left);
                len_left = 0;
            }

	    continue;
	}

        /* same mb char data */
        if (len_left)
            goto output_one_mb;

        /* next mb char data for single shift ? */
	if (mb_parse_table) {
	    if ((num = mb_parse_table[ch]) > 0) {
		parse_info = mb_parse_list[num - 1];

                codeset = parse_info->codeset;
                length = len_left = codeset->length;
                mb = 0;
                ss_flag = 1;

                continue;
	    }
        } 
    
	/* next char data : byteM ? */
	if ((codeset = byteM_parse_codeset(lcd, (inbufptr - 1))))
	    goto next_mb_char;

	/* next char data : GL or GR side ? */
	if ((codeset = GLGR_parse_codeset(lcd, ch)))
	    goto next_mb_char;
	    
        /* can't find codeset for the ch */
        unconv_num++;
        continue;

next_mb_char:
        length = len_left = codeset->length;
        mb = 0;
        ss_flag = 0;

output_one_mb:
        mb = (mb << 8) | ch;  /* 1 byte left shift */
        len_left--;

        /* last of one mb char data */
        if (!len_left) {
            if (check_string_encoding(codeset)) {
                if (outbufptr) {*outbufptr++ = mb & 0xff;}
		(*to_left)--;
            } else {
	        unconv_num++;
            }
        }

    } /* end of while */

    /* error check on last char */
    if (len_left) {
        inbufptr -= (length - len_left + ss_flag);
        (*from_left) += (length - len_left + ss_flag);
        unconv_num += (length - len_left + ss_flag);
    }

    *from = *from + from_size;
    *from_left = 0;
    *to = (XPointer)outbufptr;

    return unconv_num;
}

static int
mbtocs(conv, from, from_left, to, to_left, args, num_args)
    XlcConv conv;
    XPointer *from;
    int *from_left;
    XPointer *to;
    int *to_left;
    XPointer *args;
    int num_args;
{
    State state = (State) conv->state;
    XLCd lcd = state->lcd;

    unsigned char ch;
    unsigned long mb = 0;
    unsigned long glyph_index;

    int length = 0, len_left = 0, char_len;
    int unconv_num = 0;
    int num;
    XlcSide side;

    CodeSet codeset = NULL;
    XlcCharSet charset;
    ParseInfo parse_info;

    XPointer inbufptr = *from;
    XPointer outbufptr = *to;
    int from_size = *from_left;


    unsigned char *mb_parse_table = XLC_GENERIC(lcd, mb_parse_table);
    ParseInfo *mb_parse_list      = XLC_GENERIC(lcd, mb_parse_list);


    if (*from_left > *to_left)
        *from_left = *to_left;

    while (*from_left && *to_left) {

	ch = *inbufptr++;
	(*from_left)--;

	/* null ? */
	if (!ch) {
            unconv_num = 1;
            if (len_left)
	        unconv_num += (length - len_left);
	    break;
	}

	/* same mb char data */
        if (len_left)
	    goto output;

        /* next mb char data for single shift ? */
	if (mb_parse_table) {
	    if ((num = mb_parse_table[ch]) > 0) {
		parse_info = mb_parse_list[num - 1];

                codeset = parse_info->codeset;
		length = len_left = codeset->length;
		mb = 0;

		continue;
	    }
        } 
    
	/* next mb char data for byteM ? */
	if ((codeset = byteM_parse_codeset(lcd, (inbufptr - 1))))
	    goto next_mb_char;

	/* next mb char data for GL or GR side ? */
	if ((codeset = GLGR_parse_codeset(lcd, ch)))
	    goto next_mb_char;
	    
        /* can't find codeset for the ch */
        unconv_num = 1;
        break;

next_mb_char:
        length = len_left = codeset->length;
	mb = 0;

output:
	mb = (mb << 8) | ch;  /* 1 byte left shift */
	len_left--;

        /* last of one mb char data */
        if (!len_left) {
            glyph_index = mb_to_gi(mb, codeset);
            if (!(charset = gi_parse_charset(glyph_index, codeset))) {
                unconv_num = length;
                break;
            }
            char_len = charset->char_size;
	    side = charset->side;

            /* output glyph index */
            if (codeset->ctconv)
                glyph_index = conv_to_dest(codeset->ctconv, glyph_index);
            if (*to_left < char_len) {
                unconv_num = length;
                break;
            }

	    if (outbufptr) {
	        output_ulong_value(outbufptr, glyph_index, char_len, side);
	        outbufptr += char_len;
	    }

            (*to_left) -= char_len;

            break;
        }

    } /* end of while */

    /* error end */
    if (unconv_num) {
        *from = *from + from_size;
        *from_left = 0;
        *to = outbufptr;
	return -1;
    }

    /* nomal end */
    *from = inbufptr;
    *to = outbufptr;

    if (num_args > 0)
        *((XlcCharSet *) args[0]) = charset;

    return 0;
}

static int
mbstocs(conv, from, from_left, to, to_left, args, num_args)
    XlcConv conv;
    XPointer *from;
    int *from_left;
    XPointer *to;
    int *to_left;
    XPointer *args;
    int num_args;
{
    int ret;
    XlcCharSet charset_old, charset = NULL;
    XPointer tmp_args[1];

    XPointer inbufptr;
    int	in_left;
    XPointer outbufptr;
    int	out_left;
    tmp_args[0] = (XPointer) &charset;

    ret = mbtocs(conv, from, from_left, to, to_left, tmp_args, 1);
    charset_old = charset;
    inbufptr = *from;
    in_left = *from_left;
    outbufptr = *to;
    out_left = *to_left;

    while ( ret == 0 && *from_left && *to_left && charset_old == charset ) {
	charset_old = charset;
	inbufptr = *from;
	in_left = *from_left;
	outbufptr = *to;
	out_left = *to_left;
        ret = mbtocs(conv, from, from_left, to, to_left, tmp_args, 1);
    }

    *from = inbufptr;
    *from_left = in_left;
    *to = outbufptr;
    *to_left = out_left;

    if (num_args > 0)
        *((XlcCharSet *) args[0]) = charset_old;

    /* error end */
    if (ret != 0)
	return( -1 );

    return(0);
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
    State state = (State) conv->state;
    XLCd lcd = state->lcd;

    char *encoding;
    unsigned long mb, glyph_index;
    wchar_t wc;

    int length;
    int unconv_num = 0;

    CodeSet codeset;

    wchar_t *inbufptr = (wchar_t *) *from;
    XPointer outbufptr = *to;
    int from_size = *from_left;
    
    char *default_string = XLC_PUBLIC(lcd, default_string);
    int defstr_len = strlen(default_string);


    if (*from_left > *to_left)
        *from_left = *to_left;

    while (*from_left && *to_left) {

        wc = *inbufptr++;
        (*from_left)--;

        /* null ? */
        if (!wc) {
            if (outbufptr) {*outbufptr++ = '\0';}
            (*to_left)--;

            continue;
        }

        /* convert */
	if ( !wc_to_gi(lcd, wc, &glyph_index, &codeset) ) {

	    /* output default_string of XDefaultString() */
            if (*to_left < defstr_len)
		break;
	    if (outbufptr) {strncpy((char *)outbufptr, default_string, defstr_len);}

	    if (outbufptr) {outbufptr += defstr_len;}
	    (*to_left) -= defstr_len;

            unconv_num++;

        } else {
            mb = gi_to_mb(glyph_index, codeset);
	    if (codeset->parse_info) {

		/* output shift sequence */
		encoding = codeset->parse_info->encoding;
                length = strlen(encoding);
                if (*to_left < length)
		    break;
	        if (check_string_encoding(codeset)) {
	            if (outbufptr) {strncpy((char *)outbufptr, encoding, length);}
	            if (outbufptr) {outbufptr += length;}
	            (*to_left) -= length;
                }
            }

            /* output characters */
	    length = codeset->length;
            if (*to_left < length)
		break;
            if (check_string_encoding(codeset)) {

	        if (outbufptr) {
	            output_ulong_value(outbufptr, mb, length, XlcNONE);
	            outbufptr += length;
	        }

	        (*to_left) -= length;
            } else {
		unconv_num++;
            }
        }

    } /* end of while */

    *from = *from + from_size;
    *from_left = 0;
    *to = outbufptr;

    return unconv_num;
}

static int
stdc_wcstostr(conv, from, from_left, to, to_left, args, num_args)
    XlcConv conv;
    XPointer *from;
    int *from_left;
    XPointer *to;
    int *to_left;
    XPointer *args;
    int num_args;
{
    XPointer buf = Xmalloc((*from_left) * MB_CUR_MAX);
    XPointer buf_ptr1 = buf;
    int buf_left1 = (*from_left) * MB_CUR_MAX;
    XPointer buf_ptr2 = buf_ptr1;
    int buf_left2;
    int unconv_num1 = 0, unconv_num2 = 0;

    unconv_num1 = stdc_wcstombs(conv, 
		from, from_left, &buf_ptr1, &buf_left1, args, num_args);
    if (unconv_num1 < 0)
        goto ret;

    buf_left2 = buf_ptr1 - buf_ptr2;

    unconv_num2 = mbstostr(conv, 
		&buf_ptr2, &buf_left2, to, to_left, args, num_args);
    if (unconv_num2 < 0)
        goto ret;

ret:
    if (buf)
	Xfree((char *)buf);

    return (unconv_num1 + unconv_num2);
}

static int
wctocs(conv, from, from_left, to, to_left, args, num_args)
    XlcConv conv;
    XPointer *from;
    int *from_left;
    XPointer *to;
    int *to_left;
    XPointer *args;
    int num_args;
{
    State state = (State) conv->state;
    XLCd lcd = state->lcd;

    wchar_t wc;
    unsigned long glyph_index;

    int char_len;
    int unconv_num = 0;
    XlcSide side;

    CodeSet codeset;
    XlcCharSet charset;

    wchar_t *inbufptr = (wchar_t *) *from;
    XPointer outbufptr = *to;
    int from_size = *from_left;



    if (*from_left > *to_left)
        *from_left = *to_left;

    if (*from_left && *to_left) {

        wc = *inbufptr++;
        (*from_left)--;

        /* null ? */
        if (!wc) {
            unconv_num = 1;
            goto end;
        }

        /* convert */
	if ( !wc_to_gi(lcd, wc, &glyph_index, &codeset) ) {
            unconv_num = 1;
	    goto end;
        }

        if ( !(charset = gi_parse_charset(glyph_index, codeset)) ) {
            unconv_num = 1;
	    goto end;
        }
	char_len = charset->char_size;
	side = charset->side;

        /* output glyph index */
	if (codeset->ctconv)
            glyph_index = conv_to_dest(codeset->ctconv, glyph_index);
        if (*to_left < char_len) {
            unconv_num++;
	    goto end;
        }

        if (outbufptr) {
            output_ulong_value(outbufptr, glyph_index, char_len, side);
            outbufptr += char_len;
        }

	(*to_left) -= char_len;

    }

end:

     /* error end */
    if (unconv_num) {
        *from = *from + from_size;
        *from_left = 0;
        *to = outbufptr;
        return -1;
    }

    /* nomal end */
    *from = (XPointer)inbufptr;
    *to = outbufptr;

    if (num_args > 0)
        *((XlcCharSet *) args[0]) = charset;

    return 0;
}

static int
stdc_wctocs(conv, from, from_left, to, to_left, args, num_args)
    XlcConv conv;
    XPointer *from;
    int *from_left;
    XPointer *to;
    int *to_left;
    XPointer *args;
    int num_args;
{
    wchar_t wch, *src = *((wchar_t **) from);
    XPointer tmp_from, save_from = *from;
    char tmp[32];
    int length, ret, src_left = *from_left;
    int from_size = *from_left;

    if (src_left > 0 && *to_left > 0) {
	if ((wch = *src)) {
	    length = wctomb(tmp, wch);
	} else {
	    goto end;
	}
		
	if (length < 0)
	    goto end;

	tmp_from = (XPointer) tmp;
	ret = mbtocs(conv, &tmp_from, &length, to, to_left, args, num_args);
	if (ret < 0)
	    goto end;

	src++;
	src_left--;
    }

end:
     /* error end */
    if (save_from == (XPointer) src) {
        *from = *from + from_size;
        *from_left = 0;
	return -1;
    }

    /* nomal end */
    *from = (XPointer) src;
    *from_left = src_left;

    return 0;
}

static int
wcstocs(conv, from, from_left, to, to_left, args, num_args)
    XlcConv conv;
    XPointer *from;
    int *from_left;
    XPointer *to;
    int *to_left;
    XPointer *args;
    int num_args;
{
    int ret;
    XlcCharSet charset_old, charset = NULL;
    XPointer tmp_args[1];

    wchar_t *inbufptr;
    int	in_left;
    XPointer outbufptr;
    int	out_left;
    tmp_args[0] = (XPointer) &charset;

    ret = wctocs(conv, from, from_left, to, to_left, tmp_args, 1);
    charset_old = charset;
    inbufptr = (wchar_t *)(*from);
    in_left = *from_left;
    outbufptr = *to;
    out_left = *to_left;

    while ( ret == 0 && *from_left && *to_left && charset_old == charset ) {
	charset_old = charset;
	inbufptr = (wchar_t *)(*from);
	in_left = *from_left;
	outbufptr = *to;
	out_left = *to_left;
        ret = wctocs(conv, from, from_left, to, to_left, tmp_args, 1);
    }

    *from = (XPointer)inbufptr;
    *from_left = in_left;
    *to = outbufptr;
    *to_left = out_left;

    if (num_args > 0)
        *((XlcCharSet *) args[0]) = charset_old;

    /* error end */
    if (ret != 0)
	return( -1 );

    return(0);
}

static int
stdc_wcstocs(conv, from, from_left, to, to_left, args, num_args)
    XlcConv conv;
    XPointer *from;
    int *from_left;
    XPointer *to;
    int *to_left;
    XPointer *args;
    int num_args;
{
    int ret;
    XlcCharSet charset_old, charset = NULL;
    XPointer tmp_args[1];

    wchar_t *inbufptr;
    int	in_left;
    XPointer outbufptr;
    int	out_left;
    tmp_args[0] = (XPointer) &charset;

    ret = stdc_wctocs(conv, from, from_left, to, to_left, tmp_args, 1);
    charset_old = charset;
    inbufptr = (wchar_t *)(*from);
    in_left = *from_left;
    outbufptr = *to;
    out_left = *to_left;

    while ( ret == 0 && *from_left && *to_left && charset_old == charset ) {
	charset_old = charset;
	inbufptr = (wchar_t *)(*from);
	in_left = *from_left;
	outbufptr = *to;
	out_left = *to_left;
        ret = stdc_wctocs(conv, from, from_left, to, to_left, tmp_args, 1);
    }

    *from = (XPointer)inbufptr;
    *from_left = in_left;
    *to = outbufptr;
    *to_left = out_left;

    if (num_args > 0)
        *((XlcCharSet *) args[0]) = charset_old;

    /* error end */
    if (ret != 0)
	return( -1 );

    return(0);
}

static int
ctstombs(conv, from, from_left, to, to_left, args, num_args)
    XlcConv conv;
    XPointer *from;
    int *from_left;
    XPointer *to;
    int *to_left;
    XPointer *args;
    int num_args;
{
    XPointer buf = Xmalloc((*from_left) * sizeof(wchar_t));
    XPointer buf_ptr1 = buf;
    int buf_left1 = (*from_left);
    XPointer buf_ptr2 = buf_ptr1;
    int buf_left2;
    int unconv_num1 = 0, unconv_num2 = 0;

    unconv_num1 = ctstowcs(conv, 
		from, from_left, &buf_ptr1, &buf_left1, args, num_args);
    if (unconv_num1 < 0)
        goto ret;

    buf_left2 = (buf_ptr1 - buf_ptr2) / sizeof(wchar_t);

    unconv_num2 += wcstombs_org(conv, 
		&buf_ptr2, &buf_left2, to, to_left, args, num_args);
    if (unconv_num2 < 0)
        goto ret;

ret:
    if (buf)
	Xfree((char *)buf);

    return (unconv_num1 + unconv_num2);
}

static int
strtombs(conv, from, from_left, to, to_left, args, num_args)
    XlcConv conv;
    XPointer *from;
    int *from_left;
    XPointer *to;
    int *to_left;
    XPointer *args;
    int num_args;
{
    State state = (State) conv->state;
    XLCd lcd = state->lcd;

    char *encoding;
    unsigned long mb, glyph_index;
    unsigned char ch;

    int length;
    int unconv_num = 0;

    CodeSet codeset;

    XPointer inbufptr = *from;
    XPointer outbufptr = *to;
    int from_size = *from_left;

    if (*from_left > *to_left)
        *from_left = *to_left;

    while (*from_left && *to_left) {

        ch = *inbufptr++;
        (*from_left)--;

        /* null ? */
        if (!ch) {
            if (outbufptr) {*outbufptr++ = '\0';}
            (*to_left)--;

            continue;
        }

        /* convert */
        if (isleftside(ch)) {
	    glyph_index = ch;
	    codeset = _XlcGetCodeSetFromName(lcd, "ISO8859-1:GL");
	} else {
	    glyph_index = ch & GL;
	    codeset = _XlcGetCodeSetFromName(lcd, "ISO8859-1:GR");
	}

        if (!codeset) {
	    unconv_num++;
	    continue;
        }

        mb = gi_to_mb(glyph_index, codeset);
	if (codeset->parse_info) {

	    /* output shift sequence */
	    encoding = codeset->parse_info->encoding;
            length = strlen(encoding);
            if (*to_left < length)
		break;
	    if (outbufptr) {strncpy((char *)outbufptr, encoding, length);}

	    if (outbufptr) {outbufptr += length;}
	    (*to_left) -= length;
        }

        /* output characters */
	length = codeset->length;
        if (*to_left < length)
	    break;

        if (outbufptr) {
            output_ulong_value(outbufptr, mb, length, XlcNONE);
            outbufptr += length;
        }

	(*to_left) -= length;

    } /* end of while */

    *from = *from + from_size;
    *from_left = 0;
    *to = outbufptr;

    return unconv_num;
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
    State state = (State) conv->state;
    XLCd lcd = state->lcd;

    unsigned char ch;
    unsigned long glyph_index;
    wchar_t wc;

    int unconv_num = 0;
    CodeSet codeset;

    XPointer inbufptr = *from;
    wchar_t *outbufptr = (wchar_t *)*to;
    int from_size = *from_left;

    if (*from_left > *to_left)
        *from_left = *to_left;

    while (*from_left && *to_left) {

        ch = *inbufptr++;
        (*from_left)--;

        /* null ? */
        if (!ch) {
            if (outbufptr) {*outbufptr++ = L'\0';}
            (*to_left)--;

            continue;
        }

        /* convert */
        if (isleftside(ch)) {
	    glyph_index = ch;
	    codeset = _XlcGetCodeSetFromName(lcd, "ISO8859-1:GL");
	} else {
	    glyph_index = ch & GL;
	    codeset = _XlcGetCodeSetFromName(lcd, "ISO8859-1:GR");
	}

        if (!codeset) {
	    unconv_num++;
	    continue;
        }

        gi_to_wc(lcd, glyph_index, codeset, &wc);
	if (outbufptr) {*outbufptr++ = wc;}
	(*to_left)--;

    } /* end of while */

    *from = *from + from_size;
    *from_left = 0;
    *to = (XPointer)outbufptr;

    return unconv_num;
}

static int
stdc_strtowcs(conv, from, from_left, to, to_left, args, num_args)
    XlcConv conv;
    XPointer *from;
    int *from_left;
    XPointer *to;
    int *to_left;
    XPointer *args;
    int num_args;
{
    XPointer buf = Xmalloc((*from_left) * MB_CUR_MAX);
    XPointer buf_ptr1 = buf;
    int buf_left1 = (*from_left) * MB_CUR_MAX;
    XPointer buf_ptr2 = buf_ptr1;
    int buf_left2;
    int unconv_num1 = 0, unconv_num2 = 0;

    unconv_num1 = strtombs(conv, 
		from, from_left, &buf_ptr1, &buf_left1, args, num_args);
    if (unconv_num1 < 0)
        goto ret;

    buf_left2 = buf_ptr1 - buf_ptr2;

    unconv_num2 = stdc_mbstowcs(conv, 
		&buf_ptr2, &buf_left2, to, to_left, args, num_args);
    if (unconv_num2 < 0)
        goto ret;

ret:
    if (buf)
	Xfree((char *)buf);

    return (unconv_num1 + unconv_num2);
}

/* -------------------------------------------------------------------------- */
/*				Close                                         */
/* -------------------------------------------------------------------------- */

static void
close_converter(conv)
    XlcConv conv;
{
    if (conv->state) {
	Xfree((char *) conv->state);
    }

    if (conv->methods) {
	Xfree((char *) conv->methods);
    }

    Xfree((char *) conv);
}

/* -------------------------------------------------------------------------- */
/*				Open                                          */
/* -------------------------------------------------------------------------- */

static XlcConv
create_conv(lcd, methods)
    XLCd lcd;
    XlcConvMethods methods;
{
    XlcConv conv;
    State state;

    conv = (XlcConv) Xmalloc(sizeof(XlcConvRec));
    if (conv == NULL)
	return (XlcConv) NULL;
    
    conv->methods = (XlcConvMethods) Xmalloc(sizeof(XlcConvMethodsRec));
    if (conv->methods == NULL)
	goto err;
    *conv->methods = *methods;
    if (XLC_PUBLIC(lcd, is_state_depend))
	conv->methods->reset = init_state;

    conv->state = (XPointer) Xmalloc(sizeof(StateRec));
    if (conv->state == NULL)
	goto err;
    bzero((char *) conv->state, sizeof(StateRec));
    
    state = (State) conv->state;
    state->lcd = lcd;
    init_state(conv);
    
    return conv;

err:
    close_converter(conv);

    return (XlcConv) NULL;
}

static XlcConvMethodsRec mbstocts_methods = {
    close_converter,
    mbstocts,
    NULL
} ;

static XlcConv
open_mbstocts(from_lcd, from_type, to_lcd, to_type)
    XLCd from_lcd;
    char *from_type;
    XLCd to_lcd;
    char *to_type;
{
    return create_conv(from_lcd, &mbstocts_methods);
}

static XlcConvMethodsRec mbstostr_methods = {
    close_converter,
    mbstostr,
    NULL
} ;

static XlcConv
open_mbstostr(from_lcd, from_type, to_lcd, to_type)
    XLCd from_lcd;
    char *from_type;
    XLCd to_lcd;
    char *to_type;
{
    return create_conv(from_lcd, &mbstostr_methods);
}

static XlcConvMethodsRec mbstocs_methods = {
    close_converter,
    mbstocs,
    NULL
} ;

static XlcConv
open_mbstocs(from_lcd, from_type, to_lcd, to_type)
    XLCd from_lcd;
    char *from_type;
    XLCd to_lcd;
    char *to_type;
{
    return create_conv(from_lcd, &mbstocs_methods);
}

static XlcConvMethodsRec mbtocs_methods = {
    close_converter,
    mbtocs,
    NULL
} ;

static XlcConv
open_mbtocs(from_lcd, from_type, to_lcd, to_type)
    XLCd from_lcd;
    char *from_type;
    XLCd to_lcd;
    char *to_type;
{
    return create_conv(from_lcd, &mbtocs_methods);
}

static XlcConvMethodsRec ctstombs_methods = {
    close_converter,
    ctstombs,
    NULL
} ;

static XlcConv
open_ctstombs(from_lcd, from_type, to_lcd, to_type)
    XLCd from_lcd;
    char *from_type;
    XLCd to_lcd;
    char *to_type;
{
    return create_conv(from_lcd, &ctstombs_methods);
}

static XlcConvMethodsRec strtombs_methods = {
    close_converter,
    strtombs,
    NULL
} ;

static XlcConv
open_strtombs(from_lcd, from_type, to_lcd, to_type)
    XLCd from_lcd;
    char *from_type;
    XLCd to_lcd;
    char *to_type;
{
    return create_conv(from_lcd, &strtombs_methods);
}

#ifdef STDCVT

static XlcConvMethodsRec stdc_mbstowcs_methods = {
    close_converter,
    stdc_mbstowcs,
    NULL
} ;

static XlcConv
open_stdc_mbstowcs(from_lcd, from_type, to_lcd, to_type)
    XLCd from_lcd;
    char *from_type;
    XLCd to_lcd;
    char *to_type;
{
    return create_conv(from_lcd, &stdc_mbstowcs_methods);
}

static XlcConvMethodsRec stdc_wcstombs_methods = {
    close_converter,
    stdc_wcstombs,
    NULL
} ;

static XlcConv
open_stdc_wcstombs(from_lcd, from_type, to_lcd, to_type)
    XLCd from_lcd;
    char *from_type;
    XLCd to_lcd;
    char *to_type;
{
    return create_conv(from_lcd, &stdc_wcstombs_methods);
}

static XlcConvMethodsRec stdc_wcstocts_methods = {
    close_converter,
    stdc_wcstocts,
    NULL
} ;

static XlcConv
open_stdc_wcstocts(from_lcd, from_type, to_lcd, to_type)
    XLCd from_lcd;
    char *from_type;
    XLCd to_lcd;
    char *to_type;
{
    return create_conv(from_lcd, &stdc_wcstocts_methods);
}

static XlcConvMethodsRec stdc_wcstostr_methods = {
    close_converter,
    stdc_wcstostr,
    NULL
} ;

static XlcConv
open_stdc_wcstostr(from_lcd, from_type, to_lcd, to_type)
    XLCd from_lcd;
    char *from_type;
    XLCd to_lcd;
    char *to_type;
{
    return create_conv(from_lcd, &stdc_wcstostr_methods);
}

static XlcConvMethodsRec stdc_wcstocs_methods = {
    close_converter,
    stdc_wcstocs,
    NULL
} ;

static XlcConv
open_stdc_wcstocs(from_lcd, from_type, to_lcd, to_type)
    XLCd from_lcd;
    char *from_type;
    XLCd to_lcd;
    char *to_type;
{
    return create_conv(from_lcd, &stdc_wcstocs_methods);
}

static XlcConvMethodsRec stdc_wctocs_methods = {
    close_converter,
    stdc_wctocs,
    NULL
} ;

static XlcConv
open_stdc_wctocs(from_lcd, from_type, to_lcd, to_type)
    XLCd from_lcd;
    char *from_type;
    XLCd to_lcd;
    char *to_type;
{
    return create_conv(from_lcd, &stdc_wctocs_methods);
}

static XlcConvMethodsRec stdc_ctstowcs_methods = {
    close_converter,
    stdc_ctstowcs,
    NULL
} ;

static XlcConv
open_stdc_ctstowcs(from_lcd, from_type, to_lcd, to_type)
    XLCd from_lcd;
    char *from_type;
    XLCd to_lcd;
    char *to_type;
{
    return create_conv(from_lcd, &stdc_ctstowcs_methods);
}

static XlcConvMethodsRec stdc_strtowcs_methods = {
    close_converter,
    stdc_strtowcs,
    NULL
} ;

static XlcConv
open_stdc_strtowcs(from_lcd, from_type, to_lcd, to_type)
    XLCd from_lcd;
    char *from_type;
    XLCd to_lcd;
    char *to_type;
{
    return create_conv(from_lcd, &stdc_strtowcs_methods);
}

#endif /* STDCVT */

static XlcConvMethodsRec mbstowcs_methods = {
    close_converter,
    mbstowcs_org,
    NULL
} ;

static XlcConv
open_mbstowcs(from_lcd, from_type, to_lcd, to_type)
    XLCd from_lcd;
    char *from_type;
    XLCd to_lcd;
    char *to_type;
{
    return create_conv(from_lcd, &mbstowcs_methods);
}

static XlcConvMethodsRec wcstombs_methods = {
    close_converter,
    wcstombs_org,
    NULL
} ;

static XlcConv
open_wcstombs(from_lcd, from_type, to_lcd, to_type)
    XLCd from_lcd;
    char *from_type;
    XLCd to_lcd;
    char *to_type;
{
    return create_conv(from_lcd, &wcstombs_methods);
}

static XlcConvMethodsRec wcstocts_methods = {
    close_converter,
    wcstocts,
    NULL
} ;

static XlcConv
open_wcstocts(from_lcd, from_type, to_lcd, to_type)
    XLCd from_lcd;
    char *from_type;
    XLCd to_lcd;
    char *to_type;
{
    return create_conv(from_lcd, &wcstocts_methods);
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
    return create_conv(from_lcd, &wcstostr_methods);
}

static XlcConvMethodsRec wcstocs_methods = {
    close_converter,
    wcstocs,
    NULL
} ;

static XlcConv
open_wcstocs(from_lcd, from_type, to_lcd, to_type)
    XLCd from_lcd;
    char *from_type;
    XLCd to_lcd;
    char *to_type;
{
    return create_conv(from_lcd, &wcstocs_methods);
}

static XlcConvMethodsRec wctocs_methods = {
    close_converter,
    wctocs,
    NULL
} ;

static XlcConv
open_wctocs(from_lcd, from_type, to_lcd, to_type)
    XLCd from_lcd;
    char *from_type;
    XLCd to_lcd;
    char *to_type;
{
    return create_conv(from_lcd, &wctocs_methods);
}

static XlcConvMethodsRec ctstowcs_methods = {
    close_converter,
    ctstowcs,
    NULL
} ;

static XlcConv
open_ctstowcs(from_lcd, from_type, to_lcd, to_type)
    XLCd from_lcd;
    char *from_type;
    XLCd to_lcd;
    char *to_type;
{
    return create_conv(from_lcd, &ctstowcs_methods);
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
    return create_conv(from_lcd, &strtowcs_methods);
}

/* -------------------------------------------------------------------------- */
/*				Loader                                        */
/* -------------------------------------------------------------------------- */

XLCd
_XlcGenericLoader(name)
    char *name;
{
    XLCd lcd;
    XLCdGenericPart *gen;

    lcd = _XlcCreateLC(name, _XlcGenericMethods);

    if (lcd == NULL)
	return lcd;

    _XlcSetConverter(lcd, XlcNMultiByte, lcd, XlcNCompoundText, open_mbstocts);
    _XlcSetConverter(lcd, XlcNMultiByte, lcd, XlcNString,       open_mbstostr);
    _XlcSetConverter(lcd, XlcNMultiByte, lcd, XlcNCharSet,      open_mbstocs);
    _XlcSetConverter(lcd, XlcNMultiByte, lcd, XlcNChar,         open_mbtocs);
    _XlcSetConverter(lcd, XlcNCompoundText, lcd, XlcNMultiByte, open_ctstombs);
    _XlcSetConverter(lcd, XlcNString,    lcd, XlcNMultiByte,    open_strtombs);

#ifdef STDCVT
     gen = XLC_GENERIC_PART(lcd);

     if (gen->use_stdc_env != True) {
#endif
        _XlcSetConverter(lcd, XlcNMultiByte, lcd, XlcNWideChar,     open_mbstowcs);
        _XlcSetConverter(lcd, XlcNWideChar,  lcd, XlcNMultiByte,    open_wcstombs);
        _XlcSetConverter(lcd, XlcNWideChar,  lcd, XlcNCompoundText, open_wcstocts);
        _XlcSetConverter(lcd, XlcNWideChar,  lcd, XlcNString,       open_wcstostr);
        _XlcSetConverter(lcd, XlcNWideChar,  lcd, XlcNCharSet,      open_wcstocs);
        _XlcSetConverter(lcd, XlcNWideChar,  lcd, XlcNChar,         open_wctocs);
        _XlcSetConverter(lcd, XlcNCompoundText, lcd, XlcNWideChar,  open_ctstowcs);
        _XlcSetConverter(lcd, XlcNString,    lcd, XlcNWideChar,     open_strtowcs);
#ifdef STDCVT
    }
#endif

#ifdef STDCVT
    if (gen->use_stdc_env == True) {
        _XlcSetConverter(lcd, XlcNMultiByte, lcd, XlcNWideChar,     open_stdc_mbstowcs);
        _XlcSetConverter(lcd, XlcNWideChar,  lcd, XlcNMultiByte,    open_stdc_wcstombs);
        _XlcSetConverter(lcd, XlcNWideChar,  lcd, XlcNCompoundText, open_stdc_wcstocts);
        _XlcSetConverter(lcd, XlcNWideChar,  lcd, XlcNString,       open_stdc_wcstostr);
        _XlcSetConverter(lcd, XlcNWideChar,  lcd, XlcNCharSet,      open_stdc_wcstocs);
        _XlcSetConverter(lcd, XlcNWideChar,  lcd, XlcNChar,         open_stdc_wctocs);
        _XlcSetConverter(lcd, XlcNCompoundText, lcd, XlcNWideChar,  open_stdc_ctstowcs);
        _XlcSetConverter(lcd, XlcNString,    lcd, XlcNWideChar,     open_stdc_strtowcs);
    }
#endif

    return lcd;
}

