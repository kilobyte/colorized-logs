dnl ===========================================================================
dnl check compiler flags
AC_DEFUN([AC_C_TRY_FLAG], [
  AC_MSG_CHECKING([whether $CC supports $1])

  ac_save_CFLAGS="$CFLAGS"
  CFLAGS="$CFLAGS $1"

  AC_COMPILE_IFELSE([AC_LANG_SOURCE([ ])], [ac_c_flag=yes], [ac_c_flag=no])

  if test "x$ac_c_flag" = "xno"; then
    CFLAGS="$ac_save_CFLAGS"
  fi
  AC_MSG_RESULT([$ac_c_flag])
])
