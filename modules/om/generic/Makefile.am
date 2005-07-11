INCLUDES = \
	-I${top_srcdir}/src/xlibi18n

if XLIB_LOADABLE_I18N

commonlibdir = $(X11_LOCALELIBDIR)/common
commonlib_LTLIBRARIES=xomGeneric.la
xomGeneric_la_LDFLAGS = -module -version-number 2:0:0

else

noinst_LTLIBRARIES = libxomGeneric.la

endif


xomGeneric_la_SOURCES = \
	omDefault.c \
	omGeneric.c \
	omImText.c \
	omText.c \
	omTextEsc.c \
	omTextExt.c \
	omTextPer.c \
	omXChar.c

libxomGeneric_la_SOURCES = $(xomGeneric_la_SOURCES)
