# $XdotOrg: lib/X11/src/util/Makefile.am,v 1.4 2006-02-19 02:14:12 jamey Exp $

noinst_PROGRAMS=makekeys

makekeys_CFLAGS=$(X11_CFLAGS)

#override CC = gcc
LINK = $(CC) $(AM_CFLAGS) $(CFLAGS) $(AM_LDFLAGS) $(LDFLAGS) -o $@

EXTRA_DIST = mkks.sh
