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

.PHONY: ChangeLog

ChangeLog:
	GIT_DIR=${srcdir}/.git git-log > ChangeLog

dist-hook: ChangeLog

if LINT
# Check source code with tools like lint & sparse
LINT_SUBDIRS=src # modules

lint:
	@for subdir in $(LINT_SUBDIRS) ; do \
		echo "Making $@ in $$subdir"; \
		(cd $$subdir && $(MAKE) $(AM_MAKEFLAGS) lint) ; \
	done
endif LINT
