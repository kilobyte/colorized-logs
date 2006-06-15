dnl check for valid pty ranges, stolen from screen sources
dnl Note: this does break cross-compiling.
AC_DEFUN([AC_PTYRANGES], [
AC_MSG_CHECKING(ptyranges)
if test -d /dev/ptym ; then
pdir='/dev/ptym'
else
pdir='/dev'
fi
dnl SCO uses ptyp%d
AC_EGREP_CPP(yes,
[#ifdef M_UNIX
   yes;
#endif
], ptys=`echo /dev/ptyp??`, ptys=`echo $pdir/pty??`)
dnl if test -c /dev/ptyp19; then
dnl ptys=`echo /dev/ptyp??`
dnl else
dnl ptys=`echo $pdir/pty??`
dnl fi
if test "$ptys" != "$pdir/pty??" ; then
p0=`echo $ptys | tr ' ' '\012' | sed -e 's/^.*\(.\).$/\1/g' | sort -u | tr -d '\012'`
p1=`echo $ptys | tr ' ' '\012' | sed -e 's/^.*\(.\)$/\1/g'  | sort -u | tr -d '\012'`
AC_DEFINE_UNQUOTED(PTYRANGE0,"$p0",[The range of Xes in /dev/ptyXY])
AC_DEFINE_UNQUOTED(PTYRANGE1,"$p1",[The range of Ys in /dev/ptyXY])
AC_MSG_RESULT([$p0,$p1])
else
AC_MSG_RESULT([])
fi
])
