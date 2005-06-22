x11thislocaledir = @X11_LOCALEDIR@/$(shell basename `pwd`)

EXTRA_DIST = XLC_LOCALE.pre Compose.pre

dist_x11thislocale_DATA = XI18N_OBJS
x11thislocale_DATA =  XLC_LOCALE Compose

# XXX: WCHAR32 definition <<-- WTF does this mean?
XLC_LOCALE: XLC_LOCALE.pre
	$(CPP) -traditional -DWCHAR32=1 -DXCOMM='#' - < $(srcdir)/XLC_LOCALE.pre > $@

Compose: Compose.pre
	$(CPP) -traditional -DXCOMM='#' - < $(srcdir)/Compose.pre > $@

Compose.pre:
	touch Compose.pre	

DISTCLEANFILES=XLC_LOCALE Compose
