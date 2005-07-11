INCLUDES = \
	-I${top_srcdir}/src/xlibi18n

if XLIB_LOADABLE_I18N

commonlibdir = $(X11_LOCALELIBDIR)/common
commonlib_LTLIBRARIES=xlibi18n.la
xlibi18n_la_LDFLAGS = -module -version-number 2:0:0

else

noinst_LTLIBRARIES = libxlibi18n.la

endif

xlibi18n_la_SOURCES = \
	lcGenConv.c

libxlibi18n_la_SOURCES = $(xlibi18n_la_SOURCES)
