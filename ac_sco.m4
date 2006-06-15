AC_DEFUN([AC_SCO], [
AC_MSG_CHECKING([for SCO's intellectual property])
case `uname -s`X in
  SCO_*) AC_MSG_RESULT([nope, it belongs to Novell])
;;
  *) AC_MSG_RESULT([not found])
esac
])
