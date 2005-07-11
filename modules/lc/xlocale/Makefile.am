INCLUDES = \
	-I${top_srcdir}/src/xlibi18n

if XLIB_LOADABLE_I18N

commonlibdir = $(X11_LOCALELIBDIR)/common
commonlib_LTLIBRARIES=xlocale.la
xlocale_la_LDFLAGS = -module -version-number 2:0:0

else

noinst_LTLIBRARIES = libxlocale.la

endif


xlocale_la_SOURCES = \
	lcEuc.c
	lcJis.c
	lcSjis.c
	
libxlocale_la_SOURCES = $(xlocale_la_SOURCES)
