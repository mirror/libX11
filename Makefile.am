if XLIB_LOADABLE_I18N
ORDER=src modules
else
ORDER=modules src
endif
SUBDIRS=include $(ORDER) nls man

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = x11.pc

EXTRA_DIST=x11.pc.in ChangeLog

CLEANFILES=ChangeLog

ChangeLog:
	git-log > ChangeLog

dist-hook: ChangeLog
