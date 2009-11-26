
AC_COPYRIGHT([Copyright (c) 2009 Kristian Lyngstol])
AC_INIT([kwmd], [0.0.0])
AC_CONFIG_HEADER(config.h)
AM_INIT_AUTOMAKE
AC_PROG_CC
AC_PROG_CC_STDC
if test "x$ac_cv_prog_cc_c99" = xno; then
	AC_MSG_ERROR([Could not find a C99 compatible compiler])
fi

KWMD_REQUIERS="x11"
PKG_CHECK_MODULES(KWMD, $KWMD_REQUIERS)
AC_SUBST(KWMD_REQUIERS)
AC_CONFIG_FILES([Makefile
	include/Makefile
	src/Makefile])
AC_OUTPUT
