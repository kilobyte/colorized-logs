# Process this file with autoconf to produce a configure script.
AC_INIT
AM_INIT_AUTOMAKE(KBtin, 1.0.6a)
AC_CONFIG_SRCDIR([action.c])
AM_CONFIG_HEADER(config.h)

# Checks for programs.
AC_PROG_AWK
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_EGREP

# Checks for libraries.

# Checks for header files.
AC_HEADER_STDC
AC_HEADER_SYS_WAIT
AC_CREATE_STDINT_H
AC_HEADER_TIOCGWINSZ
AC_CHECK_HEADERS([strings.h string.h sys/time.h termios.h unistd.h stropts.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_C_INLINE
AC_HEADER_TIME
AC_C_BIGENDIAN

# Checks for library functions.
AC_PROG_GCC_TRADITIONAL
AC_TYPE_SIGNAL
AC_FUNC_VPRINTF
AC_CHECK_FUNCS([memcpy getpt ptsname snprintf vsnprintf _getpty grantpt forkpty regcomp])

AC_PTYRANGES
AC_LBL_LIBRARY_NET
CFLAGS=`echo "$CFLAGS"|sed -e 's/-O2/-O3/'`

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
