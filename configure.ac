# Process this file with autoconf to produce a configure script.
AC_INIT
AM_INIT_AUTOMAKE(KBtin, 0.4.4)
AC_CONFIG_SRCDIR([action.c])
AM_CONFIG_HEADER(config.h)

# No stinking -g!
CFLAGS=-O6

# Checks for programs.
AC_PROG_AWK
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_LN_S

# Checks for libraries.

# Checks for header files.
AC_HEADER_STDC
AC_HEADER_SYS_WAIT
AC_HEADER_TIOCGWINSZ
AC_CHECK_HEADERS([stdlib.h strings.h string.h sys/time.h termios.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_C_INLINE
AC_HEADER_TIME

# Checks for library functions.
AC_PROG_GCC_TRADITIONAL
AC_TYPE_SIGNAL
AC_FUNC_VPRINTF
AC_CHECK_FUNCS([memcpy getpt ptsname vsnprintf _getpty])

AC_LBL_LIBRARY_NET

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
