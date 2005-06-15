#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.57)
AC_INIT([libX11],
        6.2.1,
        [xorg@lists.freedesktop.org],
        libX11)
AC_CONFIG_SRCDIR([Makefile.am])
AM_INIT_AUTOMAKE([dist-bzip2])

AM_MAINTAINER_MODE

AM_CONFIG_HEADER([src/config.h])

# Checks for programs.
AC_PROG_LIBTOOL
AC_PROG_CC

# Checks for pkg-config packages
PKG_CHECK_MODULES(BIGREQS, bigreqsproto)
AC_SUBST(BIGREQS_CFLAGS)
AC_SUBST(BIGREQS_LIBS)

PKG_CHECK_MODULES(XPROTO, xproto)
AC_SUBST(XPROTO_LIBS)
AC_SUBST(XPROTO_CFLAGS)

PKG_CHECK_MODULES(X11, xextproto xtrans xau xcmiscproto xdmcp)

AC_SUBST(X11_CFLAGS)
AC_SUBST(X11_LIBS)
AC_SUBST(XDMCP_CFLAGS)
AC_SUBST(XDMCP_LIBS)

# Transport selection macro from xtrans.m4
XTRANS_CONNECTION_FLAGS

# Check for Secure RPC functions - must come after XTRANS_CONNECTION_FLAGS
# so that any necessary networking libraries are already found
AC_ARG_ENABLE(secure-rpc, 
	AC_HELP_STRING([--enable-secure-rpc],[Enable Secure RPC]),
        [SECURE_RPC=$enableval], [SECURE_RPC="try"])

if test "x$SECURE_RPC" == "xyes" -o "x$SECURE_RPC" == "xtry" ; then
	FOUND_SECURE_RPC="no"
	AC_CHECK_FUNCS([authdes_seccreate authdes_create],
			[FOUND_SECURE_RPC="yes"])
	if test "x$FOUND_SECURE_RPC" == "xno" ; then
		if test "x$SECURE_RPC" == "xyes" ; then
	AC_MSG_ERROR([Secure RPC requested, but required functions not found])
		fi	
		SECURE_RPC="no"
	else
		SECURE_RPC="yes"
	fi
fi
AC_MSG_CHECKING([if Secure RPC authentication ("SUN-DES-1") should be supported for X11 clients])
if test "x$SECURE_RPC" == "xyes" ; then
	AC_DEFINE(SECURE_RPC, 1, [Support Secure RPC ("SUN-DES-1") authentication for X11 clients])
fi
AC_MSG_RESULT($SECURE_RPC)

# Check for dlopen
AC_SEARCH_LIBS(dlopen,[dl svld])
if test "x$ac_cv_search_dlopen" = xno; then
	AC_SEARCH_LIBS(shl_load,[dld])
	if test "x$ac_cv_search_shl_load" != xno; then
		AC_DEFINE(HAVE_SHL_LOAD,1,
			  [Use shl_load to load shared libraries])
		AC_CHECK_HEADERS([dl.h])
	fi
else
	AC_DEFINE(HAVE_DLOPEN,1,[Use dlopen to load shared libraries])
	AC_CHECK_HEADERS([dlfcn.h])
fi
if test x$ac_cv_header_dlcfn_h -o x$ac_cv_header_dl_h; then
	AC_DEFINE(USE_DYNAMIC_XCURSOR,1,
		  [Use the X cursor library to load cursors])
fi

# Checks for header files.
AC_HEADER_STDC
dnl AC_CHECK_HEADERS([stdio.h stdlib.h math.h])

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.
AC_CHECK_FUNCS([strtol])
# Used in lcFile.c (see also --enable-xlocaledir settings below)
XLOCALEDIR_IS_SAFE="no"
AC_CHECK_FUNC([issetugid], [XLOCALEDIR_IS_SAFE="yes"]
	AC_DEFINE(HASSETUGID,1,[Has issetugid() function]))
AC_CHECK_FUNC([getresuid], [XLOCALEDIR_IS_SAFE="yes"]
	AC_DEFINE(HASGETRESUID,1,[Has getresuid() & getresgid() functions]))
# Used in Font.c
AC_CHECK_FUNC([shmat], AC_DEFINE(HAS_SHM,1,[Has shm*() functions]))

# Checks for system services
dnl AC_PATH_XTRA

# arch specific things
case $target_alias in
  *os2*) os2="true" ;;
  *) ;;
esac

AM_CONDITIONAL(OS2, test x$os2 = xtrue)

AC_ARG_ENABLE(xthreads,
              AC_HELP_STRING([--disable-xthreads],
                [Disable Xlib support for Multithreading]),
              [xthreads=$enableval],[xthreads=yes])

case x$xthreads in
xyes)
	AC_DEFINE(XTHREADS,[],[Whether libX11 is compiled with thread support])
	AC_DEFINE(XUSE_MTSAFE_API,[],[Whether libX11 needs to use MT safe API's])
	;;
*)
	;;
esac
AC_SUBST(XTHREADS)
AC_SUBST(XUSE_MTSAFE_API)

AC_CHECK_LIB(Xdmcp, XdmcpWrap, [xdmauth="yes"], [xdmauth="no"], [$XDMCP_LIBS])

case x$xdmauth in
xyes)
	XDMCP_CFLAGS="$XDMCP_CFLAGS -DHASXDMAUTH"
	;;
xno)
	XDMCP_LIBS=""
	;;
esac
AC_SUBST(XDMCP_CFLAGS)
AC_SUBST(XDMCP_LIBS)

AC_CHECK_FUNC(poll, [AC_DEFINE(USE_POLL, 1, [poll() function is available])], )

#
# Find keysymdef.h
#
KEYSYMDEF=""
AC_MSG_CHECKING([keysymdef.h])
for flag in $XPROTO_CFLAGS -I/usr/include; do
	case "$KEYSYMDEF" in
	"")
		case "$flag" in
		*-I*)
			dir=`echo "$flag" | sed 's/ *-I//'`
			file="$dir/X11/keysymdef.h"
			if test -f "$file"; then
				KEYSYMDEF="$file"
			fi
			;;
		esac
		;;
	esac
done
case "$KEYSYMDEF" in
"")
	AC_MSG_ERROR([Cannot find keysymdef.h])
	;;
*)
	AC_MSG_RESULT([$KEYSYMDEF])
        ;;
esac
AC_SUBST(KEYSYMDEF)

AM_CONDITIONAL(UDC, test xfalse = xtrue)
AM_CONDITIONAL(XCMS, test xtrue = xtrue)

AC_ARG_ENABLE(xcms, 
              AC_HELP_STRING([--disable-xcms],
                [Disable Xlib support for CMS *EXPERIMENTAL*]),
              [XCMS=$enableval],[XCMS=yes])

AC_ARG_ENABLE(xlocale,
              AC_HELP_STRING([--disable-xlocale],
                [Disable Xlib locale implementation *EXPERIMENTAL*]),
              [XLOCALE=$enableval],[XLOCALE=yes])

# This disables XLOCALEDIR.  Set it if you're using BuildLoadableXlibI18n,
# don't have either issetugid() or getresuid(), and you need to protect
# clients that are setgid or setuid to an id other than 0.
AC_MSG_CHECKING([if XLOCALEDIR support should be enabled])
AC_ARG_ENABLE(xlocaledir,
	      AC_HELP_STRING([--enable-xlocaledir],
		[Enable XLOCALEDIR environment variable support]),
	      [XLOCALEDIR=$enableval],[XLOCALEDIR=$XLOCALEDIR_IS_SAFE])
if test "x$XLOCALEDIR" == "xno"; then
	AC_DEFINE(NO_XLOCALEDIR,1,[Disable XLOCALEDIR environment variable])
fi
AC_MSG_RESULT($XLOCALEDIR)

AC_MSG_CHECKING([if XF86BigFont support should be enabled])
AC_ARG_ENABLE(xf86bigfont,
	      AC_HELP_STRING([--disable-xf86bigfont],
		[Disable XF86BigFont extension support]),
	      [XF86BIGFONT=$enableval],[XF86BIGFONT="yes"])
if test "x$XF86BIGFONT" == "xyes"; then
	AC_DEFINE(XF86BIGFONT,1,[Enable XF86BIGFONT extension])
fi
AC_MSG_RESULT($XF86BIGFONT)


AC_ARG_ENABLE(xkb, 
              AC_HELP_STRING([--disable-xkb],
                [Disable XKB support *EXPERIMENTAL*]),
              [XKB=$enableval],[XKB=yes])

# Finalization

AM_CONDITIONAL(DISABLE_XCMS, [test x$XCMS = xno ])
if test "$XCMS" = no; then
	AC_DEFINE(DISABLE_XCMS,1,[No support for XCMS])
fi

AM_CONDITIONAL(DISABLE_XLOCALE, [ test x$XLOCALE = xno ])
if test "$XLOCALE" = no; then
	AC_DEFINE(DISABLE_XLOCALE,1,[No support for X Locales])
fi

AM_CONDITIONAL(XKB, [ test x$XKB = xyes ])
if test "$XKB" = yes; then
   PKG_CHECK_MODULES(XKBPROTO, kbproto inputproto)
   AC_DEFINE(XKB, YES, [Use XKB])
   X11_CFLAGS="$X11_CFLAGS $XKBPROTO_CFLAGS"
   X11_LIBS="$X11_LIBS $XKBPROTO_LIBS"
fi

if test "x$GCC" = "xyes"; then
	GCC_WARNINGS="-Wall -Wpointer-arith -Wstrict-prototypes \
	-Wmissing-prototypes -Wmissing-declarations \
	-Wnested-externs -fno-strict-aliasing"
	X11_CFLAGS="$GCC_WARNINGS $X11_CFLAGS"
fi

X11_DATADIR="${datadir}/X11"
X11_LOCALEDIR="${X11_DATADIR}/locale"
XKEYSYMDB="${X11_DATADIR}/XKeysymDB"
XERRORDB="${X11_DATADIR}/XErrorDB"

AC_SUBST(X11_DATADIR)
AC_SUBST(X11_LOCALEDIR)
AC_DEFINE_DIR(XKEYSYMDB, XKEYSYMDB, [Location of keysym database])
AC_DEFINE_DIR(XERRORDB, XERRORDB, [Location of error message database])
AC_DEFINE_DIR(XLOCALEDIR, X11_LOCALEDIR, [Location of libX11 locale data])

AC_OUTPUT([Makefile
           include/Makefile
	   man/Makefile
           src/Makefile
           src/util/Makefile
	   nls/Makefile
	   nls/armscii-8/Makefile
	   nls/C/Makefile
	   nls/en_US.UTF-8/Makefile
	   nls/georgian-academy/Makefile
	   nls/georgian-ps/Makefile
	   nls/ibm-cp1133/Makefile
	   nls/iscii-dev/Makefile
	   nls/isiri-3342/Makefile
	   nls/iso8859-1/Makefile
	   nls/iso8859-10/Makefile
	   nls/iso8859-11/Makefile
	   nls/iso8859-13/Makefile
	   nls/iso8859-14/Makefile
	   nls/iso8859-15/Makefile
	   nls/iso8859-2/Makefile
	   nls/iso8859-3/Makefile
	   nls/iso8859-4/Makefile
	   nls/iso8859-5/Makefile
	   nls/iso8859-6/Makefile
	   nls/iso8859-7/Makefile
	   nls/iso8859-8/Makefile
	   nls/iso8859-9/Makefile
	   nls/iso8859-9e/Makefile
	   nls/ja/Makefile
	   nls/ja.JIS/Makefile
	   nls/ja_JP.UTF-8/Makefile
	   nls/ja.S90/Makefile
	   nls/ja.SJIS/Makefile
	   nls/ja.U90/Makefile
	   nls/ko/Makefile
	   nls/koi8-c/Makefile
	   nls/koi8-r/Makefile
	   nls/koi8-u/Makefile
	   nls/ko_KR.UTF-8/Makefile
	   nls/microsoft-cp1251/Makefile
	   nls/microsoft-cp1255/Makefile
	   nls/microsoft-cp1256/Makefile
	   nls/mulelao-1/Makefile
	   nls/nokhchi-1/Makefile
	   nls/pt_BR.UTF-8/Makefile
	   nls/tatar-cyr/Makefile
	   nls/th_TH/Makefile
	   nls/th_TH.UTF-8/Makefile
	   nls/tscii-0/Makefile
	   nls/vi_VN.tcvn/Makefile
	   nls/vi_VN.viscii/Makefile
	   nls/zh_CN/Makefile
	   nls/zh_CN.gb18030/Makefile
	   nls/zh_CN.gbk/Makefile
	   nls/zh_CN.UTF-8/Makefile
	   nls/zh_HK.big5/Makefile
	   nls/zh_HK.big5hkscs/Makefile
	   nls/zh_HK.UTF-8/Makefile
	   nls/zh_TW/Makefile
	   nls/zh_TW.big5/Makefile
	   nls/zh_TW.UTF-8/Makefile
	   x11.pc])
