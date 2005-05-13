noinst_PROGRAMS=makekeys

makekeys_CFLAGS=$(X11_CFLAGS) $(BIGREQS_CFLAGS)

override CC = gcc
LINK = gcc $(AM_CFLAGS) $(CFLAGS) $(AM_LDFLAGS) $(LDFLAGS) -o $@
