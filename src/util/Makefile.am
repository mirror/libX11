# $XdotOrg: lib/X11/src/util/Makefile.am,v 1.3 2005-10-07 15:00:01 kem Exp $

noinst_PROGRAMS=makekeys

makekeys_CFLAGS=$(X11_CFLAGS) $(BIGREQS_CFLAGS)

#override CC = gcc
LINK = $(CC) $(AM_CFLAGS) $(CFLAGS) $(AM_LDFLAGS) $(LDFLAGS) -o $@

EXTRA_DIST = mkks.sh
