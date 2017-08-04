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

# Checks for libraries.

# Checks for header files.
AC_HEADER_STDC
AC_HEADER_SYS_WAIT
AC_HEADER_TIOCGWINSZ
AC_CHECK_HEADERS([strings.h string.h sys/time.h termios.h unistd.h stropts.h \
    pty.h libutil.h util.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_C_INLINE
AC_HEADER_TIME
AC_C_BIGENDIAN

# Checks for library functions.
AC_SEARCH_LIBS(forkpty, util)
AC_CHECK_FUNCS([memcpy getpt ptsname _getpty grantpt \
	forkpty regcomp getaddrinfo wcwidth posix_openpt cfmakeraw])

AC_PTYRANGES
AC_DEV_PTMX
AC_SCO
AC_SYS_LARGEFILE
AC_C_TRY_FLAG([-Wno-unused-parameter])
AC_C_TRY_FLAG([-Wno-implicit-fallthrough])

# Enable some warnings.
AC_C_TRY_FLAG([-Wwrite-strings])
AC_C_TRY_FLAG([-Wstrict-prototypes])

AC_CONFIG_FILES([Makefile])

# Initialize the test suite.
AC_CONFIG_TESTDIR(tests)
AC_CONFIG_FILES([tests/Makefile tests/atlocal])
AM_MISSING_PROG([AUTOM4TE], [autom4te])

AC_OUTPUT
