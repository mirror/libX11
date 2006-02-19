AM_CFLAGS= \
	-I$(top_srcdir)/include \
	-I$(top_srcdir)/include/X11 \
	-I$(top_builddir)/include \
	-I$(top_builddir)/include/X11 \
	-I$(top_srcdir)/src/xcms \
	-I$(top_srcdir)/src/xkb \
	-I$(top_srcdir)/src/xlibi18n \
	$(X11_CFLAGS) \
	$(BIGFONT_CFLAGS) \
	$(XDMCP_CFLAGS) \
	-D_BSD_SOURCE \
	$(XMALLOC_ZERO_CFLAGS)

noinst_LTLIBRARIES = libxkb.la

libxkb_la_SOURCES = \
	XKB.c \
	XKBBind.c \
	XKBCompat.c \
	XKBCtrls.c \
	XKBCvt.c \
	XKBGetMap.c \
	XKBGetByName.c \
	XKBNames.c \
	XKBRdBuf.c \
	XKBSetMap.c \
	XKBUse.c \
	XKBleds.c \
	XKBBell.c \
	XKBGeom.c \
	XKBSetGeom.c \
	XKBExtDev.c \
	XKBList.c \
	XKBMisc.c \
	XKBMAlloc.c \
	XKBGAlloc.c \
	XKBAlloc.c \
	XKBlibint.h
