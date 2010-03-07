#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.61)
AC_COPYRIGHT([Copyright (c) 2009 Kristian Lyngstol])
AC_INIT([wmd], [0.1-devel],[kristian@bohemians.org])
AC_CONFIG_SRCDIR([config.h.in])
AM_CONFIG_HEADER(config.h)

AM_INIT_AUTOMAKE(wmd, 0.1-devel, [-Wall -Werror foreign])
AM_CFLAGS="-Wall -Werror"
PKG_CHECK_MODULES([xcb], [xcb],,[AC_MSG_ERROR([wmd requires xcb.])])

# Checks for programs.
AC_PROG_AWK
AC_PROG_CC

# Checks for libraries.

# Checks for header files.
AC_PATH_X
AC_HEADER_STDC
AC_CHECK_HEADERS([limits.h stdlib.h string.h sys/param.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_C_INLINE

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_MEMCMP
AC_FUNC_VPRINTF
AC_CHECK_FUNCS([strcasecmp strerror strtol])

AC_CONFIG_FILES([Makefile
                 include/Makefile
                 src/Makefile])
AC_OUTPUT
