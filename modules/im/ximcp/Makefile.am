INCLUDES = \
	-I${top_srcdir}/src/xlibi18n

if XLIB_LOADABLE_I18N

commonlibdir = $(X11_LOCALELIBDIR)/common
commonlib_LTLIBRARIES=ximcp.la
ximcp_la_LDFLAGS = -module -version-number 2:0:0

else

noinst_LTLIBRARIES=libximcp.la

endif


ximcp_la_SOURCES = \
	imCallbk.c \
	imDefFlt.c \
	imDefIc.c \
	imDefIm.c \
	imDefLkup.c \
	imDispch.c \
	imEvToWire.c \
	imExten.c \
	imImSw.c \
	imInsClbk.c \
	imInt.c \
	imLcFlt.c \
	imLcGIc.c \
	imLcIc.c \
	imLcIm.c \
	imLcLkup.c \
	imLcPrs.c \
	imLcSIc.c \
	imRmAttr.c \
	imRm.c \
	imThaiFlt.c \
	imThaiIc.c \
	imThaiIm.c \
	imTrans.c \
	imTransR.c \
	imTrX.c

libximcp_la_SOURCES = $(ximcp_la_SOURCES)
