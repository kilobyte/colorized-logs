# Process this file with autoconf to produce a configure script.
AC_INIT([KBtin], [1.0.12], [kilobyte@angband.pl])
AM_INIT_AUTOMAKE
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
AC_CHECK_FUNCS([memcpy getpt ptsname snprintf vsnprintf _getpty grantpt forkpty regcomp getaddrinfo wcwidth])
AC_ARG_LIB(mccp, zlib, ZLIB, zlib.h, z, gzread, [
Please install zlib and its headers.  On most systems, the package will be named
zlib-dev or libz-dev.  Or, you can "./configure --disable-mccp" to ignore this
issue.  That's kind of a bad idea, though, since MCCP usually greatly reduces
bandwidth-related lag.
])
AC_ARG_LIB(ssl, gnutls, GNUTLS, gnutls/gnutls.h, gnutls, gnutls_global_init, [
Please install gnutls and its headers.  On most systems, the package will be named
libgnutls-dev or gnutls-dev.  Or, you can "./configure --disable-ssl" to compile
without support for encryption.
])

AM_PATH_GLIB_2_0([2.10.0], [AC_DEFINE([HAVE_GLIB], [1], [Define if glib 2.10+ is available.])])
AM_ICONV
AM_CONDITIONAL(SANE_MAKE, make -v >/dev/null 2>/dev/null)

AC_PTYRANGES
AC_DEV_PTMX
AC_LBL_LIBRARY_NET
AC_SOL_PROTOCOLS
AC_UCS_SIZE
AC_SCO
AC_AI_ADDRCONFIG
CFLAGS=`echo "$CFLAGS"|sed -e 's/-O2/-O3/'`

AC_CONFIG_FILES([Makefile])

# Initialize the test suite.
AC_CONFIG_TESTDIR(tests)
AC_CONFIG_FILES([tests/Makefile tests/atlocal])
AM_MISSING_PROG([AUTOM4TE], [autom4te])

AC_OUTPUT
