dnl ***************************************
dnl *** AC_DECL_OR_ZERO(symbol, header) ***
dnl ***************************************
dnl
dnl Checks if the given symbol (rvalue) is defined in the given header
dnl -- if not, #defines it to 0.

AC_DEFUN([AC_DECL_OR_ZERO], [
  AC_MSG_CHECKING([for $1])
  AC_COMPILE_IFELSE([AC_LANG_SOURCE([#include <$2>
int main()
{
  char *p=(char*)$1;
  return !p;
}
])], [AC_MSG_RESULT([yes])], [
    AC_MSG_RESULT([no, defining as 0])
    AC_DEFINE([$1], [0], [Set to 0 if not supported])
    ])
  ])
])
