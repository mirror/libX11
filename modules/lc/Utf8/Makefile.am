INCLUDES = \
	-I${top_srcdir}/src/xlibi18n

if XLIB_LOADABLE_I18N

commonlibdir = $(X11_LOCALELIBDIR)/common
commonlib_LTLIBRARIES=xlcUTF8Load.la
xlcUTF8Load_la_LDFLAGS = -module -version-number 2:0:0

else

noinst_LTLIBRARIES = libxlcUTF8Load.la

endif


xlcUTF8Load_la_SOURCES = \
	lcUTF8Load.c

libxlcUTF8Load_la_SOURCES = $(xlcUTF8Load_la_SOURCES)
