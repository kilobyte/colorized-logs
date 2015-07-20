dnl Check for the existence of a function that can be a macro.

AC_DEFUN([AC_FUNC_STRLCPY], [
AC_MSG_CHECKING([for strlcpy])
AC_LINK_IFELSE([AC_LANG_SOURCE([[
#include <string.h>

int main()
{
    char buf[10];
    return strlcpy(buf, "", 5);
}]])], [
  AC_DEFINE([HAVE_STRLCPY], [1], [Define if strlcpy is a function or macro.])
  AC_MSG_RESULT([yes])], [AC_MSG_RESULT([no])])
])
