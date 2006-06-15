dnl *****************************
dnl *** Checks for SOL_IP     ***
dnl *****************************
AC_DEFUN([AC_SOL_PROTOCOLS], [
AC_MSG_CHECKING(for SOL_IP)
AC_TRY_COMPILE([#include <netdb.h>], [
        int level = SOL_IP;
], [
        # Yes, we have it...
        AC_MSG_RESULT(yes)
        AC_DEFINE([HAVE_SOL_IP], [], [Define if you have SOL_IP])
], [
        # We'll have to use getprotobyname
        AC_MSG_RESULT(no)
])
dnl AC_MSG_CHECKING(for SOL_IPV6)
dnl AC_TRY_COMPILE([#include <netdb.h>], [
dnl         int level = SOL_IPV6;
dnl ], [
dnl         # Yes, we have it...
dnl         AC_MSG_RESULT(yes)
dnl         AC_DEFINE([HAVE_SOL_IPV6], [], [Define if you have SOL_IPV6])
dnl ], [
dnl         # We'll have to use getprotobyname
dnl         AC_MSG_RESULT(no)
dnl ])
])
