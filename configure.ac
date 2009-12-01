
AC_COPYRIGHT([Copyright (c) 2009 Kristian Lyngstol])
AC_INIT([wmd], [0.1-devel],,[wmd])
AC_CONFIG_HEADER(config.h)
AM_INIT_AUTOMAKE
AC_PROG_LIBTOOL
AC_PROG_MAKE_SET
AC_PROG_CC
AC_PROG_CC_STDC
if test "x$ac_cv_prog_cc_c99" = xno; then
	AC_MSG_ERROR([Could not find a C99 compatible compiler])
fi

AC_PATH_X
PKG_CHECK_MODULES([x11], [x11],,[AC_MSG_ERROR([wmd requires x11.])])
AC_SUBST(WMD_REQUIERS)
AC_CONFIG_FILES([Makefile
	include/Makefile
	src/Makefile])
AC_OUTPUT
