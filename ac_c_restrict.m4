AC_DEFUN([AC_C_RESTRICT], [
AC_MSG_CHECKING(for restrict on pointers)
AC_COMPILE_IFELSE([AC_LANG_SOURCE([[void foo(int *restrict p) {*p=0;}]])], [
        AC_MSG_RESULT(yes)
], [
        AC_MSG_RESULT(no)
        AC_DEFINE([restrict], [], [Define away if not supported])
])
])
