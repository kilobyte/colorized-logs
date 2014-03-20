AC_DEFUN([AC_AI_ADDRCONFIG], [
AC_MSG_CHECKING(for AI_ADDRCONFIG)
AC_TRY_COMPILE([#include <netdb.h>], [
        int foo = AI_ADDRCONFIG;
], [
        # Yes, we have it...
        AC_MSG_RESULT(yes)
], [
        # No, set it to 0 instead.
        AC_MSG_RESULT(no)
        AC_DEFINE([AI_ADDRCONFIG], [0], [Set to 0 if it's not supported])
])
])
