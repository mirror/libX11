INCLUDES = \
	-I${top_srcdir}/src/xlibi18n

if XLIB_LOADABLE_I18N

commonlibdir = $(X11_LOCALELIBDIR)/common
commonlib_LTLIBRARIES=xlcDef.la
xlcDef_la_LDFLAGS = -module -version-number 2:0:0

else

noinst_LTLIBRARIES = libxlcDef.la

endif

xlcDef_la_SOURCES = \
	lcDefConv.c

libxlcDef_la_SOURCES = $(xlcDef_la_SOURCES)
