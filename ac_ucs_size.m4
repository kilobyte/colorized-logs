dnl Check the size of wchar_t.

AC_DEFUN([AC_UCS_SIZE], [
AC_MSG_CHECKING([if wchar_t can hold any Unicode char])
AC_COMPILE_IFELSE([#include <wchar.h>
extern char foo[[2*(sizeof(wchar_t)==4)-1]];], [
  AC_DEFINE([WCHAR_IS_UCS4], [1], [Define if wchar_t can hold any Unicode value.])
  AC_MSG_RESULT([yes])
], [AC_MSG_RESULT([no, UCS-2 only])])
])
