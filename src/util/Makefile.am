# $XdotOrg: lib/X11/src/util/Makefile.am,v 1.2 2005-05-21 04:26:12 alanc Exp $

noinst_PROGRAMS=makekeys

makekeys_CFLAGS=$(X11_CFLAGS) $(BIGREQS_CFLAGS)

#override CC = gcc
LINK = $(CC) $(AM_CFLAGS) $(CFLAGS) $(AM_LDFLAGS) $(LDFLAGS) -o $@
