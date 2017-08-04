AC_INIT([colorized-logs], m4_esyscmd([./get_version -n]), [kilobyte@angband.pl])
AM_INIT_AUTOMAKE([-Wall dist-xz no-dist-gzip foreign])
AC_CONFIG_SRCDIR([ansi2html.c])
AC_CONFIG_HEADERS([config.h])
# Use kernel-like CC instead of the usual verbose insanity.
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

# Checks for programs.
AC_PROG_CC_C99
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_EGREP

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.
AC_SEARCH_LIBS(forkpty, util)

AC_SYS_LARGEFILE

AC_CONFIG_FILES([Makefile])

# Initialize the test suite.
AC_CONFIG_TESTDIR(tests)
AC_CONFIG_FILES([tests/Makefile tests/atlocal])
AM_MISSING_PROG([AUTOM4TE], [autom4te])

AC_OUTPUT
