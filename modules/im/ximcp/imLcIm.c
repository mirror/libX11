/* $Xorg: imLcIm.c,v 1.3 2000/08/17 19:45:14 cpqbld Exp $ */
/******************************************************************

          Copyright 1992, 1993, 1994 by FUJITSU LIMITED
          Copyright 1993 by Digital Equipment Corporation

Permission to use, copy, modify, distribute, and sell this software
and its documentation for any purpose is hereby granted without fee,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the name of FUJITSU LIMITED and
Digital Equipment Corporation not be used in advertising or publicity
pertaining to distribution of the software without specific, written
prior permission.  FUJITSU LIMITED and Digital Equipment Corporation
makes no representations about the suitability of this software for
any purpose.  It is provided "as is" without express or implied
warranty.

FUJITSU LIMITED AND DIGITAL EQUIPMENT CORPORATION DISCLAIM ALL 
WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED 
WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL 
FUJITSU LIMITED AND DIGITAL EQUIPMENT CORPORATION BE LIABLE FOR 
ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES 
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER 
IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, 
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF 
THIS SOFTWARE.

  Author:    Takashi Fujiwara     FUJITSU LIMITED 
                               	  fujiwara@a80.tech.yk.fujitsu.co.jp
  Modifier:  Franky Ling          Digital Equipment Corporation
	                          frankyling@hgrd01.enet.dec.com

******************************************************************/

#include <stdio.h>
/*
#include <X11/Xlib.h>
*/
#include <X11/Xmd.h>
#include <X11/Xatom.h>
#include <X11/Xos.h>
#include "Xlibint.h"
#include "Xlcint.h"
#include "XlcPublic.h"
#include "Ximint.h"
#include <ctype.h>

Public Bool
_XimCheckIfLocalProcessing(im)
    Xim          im;
{
    FILE        *fp;
    char        *name;

    if(strcmp(im->core.im_name, "") == 0) {
	name = _XlcFileName(im->core.lcd, COMPOSE_FILE);
	if (name != (char *)NULL) {
	    fp = _XFopenFile (name, "r");
	    Xfree(name);
	    if (fp != (FILE *)NULL) {
		fclose(fp);
		return(True);
	    }
	}
	return(False);
    } else if(strcmp(im->core.im_name, "local") == 0 ||
	      strcmp(im->core.im_name, "none" ) == 0 ) {
	return(True);
    }
    return(False);
}

Private void
XimFreeDefaultTree(top)
    DefTree *top;
{
    if (!top) return;
    if (top->succession) XimFreeDefaultTree(top->succession);
    if (top->next) XimFreeDefaultTree(top->next);
    if (top->mb) Xfree(top->mb);
    if (top->wc) Xfree(top->wc);
    Xfree(top);
}

Public void
_XimLocalIMFree(im)
    Xim		im;
{
    XimFreeDefaultTree(im->private.local.top);
    if(im->core.im_resources) {
	Xfree(im->core.im_resources);
	im->core.im_resources = NULL;
    }
    if(im->core.ic_resources) {
	Xfree(im->core.ic_resources);
	im->core.ic_resources = NULL;
    }
    if(im->core.im_values_list) {
	Xfree(im->core.im_values_list);
	im->core.im_values_list = NULL;
    }
    if(im->core.ic_values_list) {
	Xfree(im->core.ic_values_list);
	im->core.ic_values_list = NULL;
    }
    if(im->core.styles) {
	Xfree(im->core.styles);
	im->core.styles = NULL;
    }
    if(im->core.res_name) {
	Xfree(im->core.res_name);
	im->core.res_name = NULL;
    }
    if(im->core.res_class) {
	Xfree(im->core.res_class);
	im->core.res_class = NULL;
    }
    if(im->core.im_name) {
	Xfree(im->core.im_name);
	im->core.im_name = NULL;
    }
    return;
}

Private Status
_XimLocalCloseIM(xim)
    XIM		xim;
{
    Xim		im = (Xim)xim;
    XIC		ic;
    XIC		next;

    ic = im->core.ic_chain;
    while (ic) {
	(*ic->methods->destroy) (ic);
	next = ic->core.next;
	Xfree ((char *) ic);
	ic = next;
    }
    _XimLocalIMFree(im);
    _XimDestroyIMStructureList(im);
    return(True);
}

Public char *
_XimLocalGetIMValues(xim, values)
    XIM			 xim;
    XIMArg		*values;
{
    Xim			 im = (Xim)xim;
    XimDefIMValues	 im_values;

    _XimGetCurrentIMValues(im, &im_values);
    return(_XimGetIMValueData(im, (XPointer)&im_values, values,
			im->core.im_resources, im->core.im_num_resources));
}

Public char *
_XimLocalSetIMValues(xim, values)
    XIM			 xim;
    XIMArg		*values;
{
    Xim			 im = (Xim)xim;
    XimDefIMValues	 im_values;
    char		*name = (char *)NULL;

    _XimGetCurrentIMValues(im, &im_values);
    name = _XimSetIMValueData(im, (XPointer)&im_values, values,
		im->core.im_resources, im->core.im_num_resources);
    _XimSetCurrentIMValues(im, &im_values);
    return(name);
}

Private void
_XimCreateDefaultTree(im)
    Xim		im;
{
    FILE *fp;
    char *name;

    name = _XlcFileName(im->core.lcd, COMPOSE_FILE);
    if (name == (char *)NULL)
         return;
    fp = _XFopenFile (name, "r");
    Xfree(name);
    if (fp == (FILE *)NULL)
	 return;
    _XimParseStringFile(fp, &im->private.local.top);
    fclose(fp);
}

Private XIMMethodsRec      Xim_im_local_methods = {
    _XimLocalCloseIM,           /* close */
    _XimLocalSetIMValues,       /* set_values */
    _XimLocalGetIMValues,       /* get_values */
    _XimLocalCreateIC,          /* create_ic */
    _XimLcctstombs,		/* ctstombs */
    _XimLcctstowcs		/* ctstowcs */
};

Public Bool
_XimLocalOpenIM(im)
    Xim			 im;
{
    XLCd		 lcd = im->core.lcd;
    XlcConv		 ctom_conv;
    XlcConv		 ctow_conv;
    XimDefIMValues	 im_values;

    _XimInitialResourceInfo();
    if(_XimSetIMResourceList(&im->core.im_resources,
		 		&im->core.im_num_resources) == False) {
	goto Open_Error;
    }
    if(_XimSetICResourceList(&im->core.ic_resources,
				&im->core.ic_num_resources) == False) {
	goto Open_Error;
    }

    _XimSetIMMode(im->core.im_resources, im->core.im_num_resources);

    _XimGetCurrentIMValues(im, &im_values);
    if(_XimSetLocalIMDefaults(im, (XPointer)&im_values,
		im->core.im_resources, im->core.im_num_resources) == False) {
	goto Open_Error;
    }
    _XimSetCurrentIMValues(im, &im_values);

    _XimCreateDefaultTree(im);

    if (!(ctom_conv = _XlcOpenConverter(lcd,
					XlcNCompoundText, lcd, XlcNMultiByte)))
	goto Open_Error;
    if (!(ctow_conv = _XlcOpenConverter(lcd,
					XlcNCompoundText, lcd, XlcNWideChar)))
	goto Open_Error;

    im->methods = &Xim_im_local_methods;
    im->private.local.current_ic = (XIC)NULL;
    im->private.local.ctom_conv = ctom_conv;
    im->private.local.ctow_conv = ctow_conv;

    return(True);

Open_Error :
    if (im->core.im_resources) {
	Xfree(im->core.im_resources);
	im->core.im_resources = NULL;
    }
    if (im->core.ic_resources) {
	Xfree(im->core.ic_resources);
	im->core.ic_resources = NULL;
    }
    if (im->core.im_values_list) {
	Xfree(im->core.im_values_list);
	im->core.im_values_list = NULL;
    }
    if (im->core.ic_values_list) {
	Xfree(im->core.ic_values_list);
	im->core.ic_values_list = NULL;
    }
    if (im->core.styles) {
	Xfree(im->core.styles);
	im->core.styles = NULL;
    }
    if (im->private.local.ctom_conv) {
	_XlcCloseConverter(im->private.local.ctom_conv);
    }
    if (im->private.local.ctow_conv) {
	_XlcCloseConverter(im->private.local.ctow_conv);
    }
    return(False);
}
