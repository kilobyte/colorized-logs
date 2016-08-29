dnl AC_ARG_LIB(arg, lib, LIB, header, link_lib, some_func, failinstr)

AC_DEFUN([AC_ARG_LIB], [
AC_ARG_ENABLE([$1], [])
if [[ "X$enable_$1" != "Xno" ]]
  then
    # Missing pkg-config is ok.
    $3_CFLAGS="$(pkg-config 2>/dev/null --cflags $2)"
    $3_LIBS="$(pkg-config 2>/dev/null --libs $2)"
    ac_save_CFLAGS="$CFLAGS"
    ac_save_LDFLAGS="$LDFLAGS"
    CFLAGS="$CFLAGS ${$3_CFLAGS}"
    LDFLAGS="$LDFLAGS ${$3_CFLAGS:--l$5}"
    ac_$2_is_there=yes
    AC_CHECK_HEADER([$4], , [ac_$2_is_there=no])
    AC_CHECK_LIB([$5], [$6], [:], [ac_$2_is_there=no])
    if [[ "x$ac_$2_is_there" = "xyes" ]]
      then
        AC_DEFINE([HAVE_$3], [1], [Define if $2 is available.])
      else
        AC_MSG_ERROR([
$2 doesn't appear to be available.
$7])
	unset $3_CFLAGS $3_LIBS
    fi
    CFLAGS="$ac_save_CFLAGS"
    LDFLAGS="$ac_save_LDFLAGS"
fi
AC_SUBST([$3_LIBS])
])
