dnl check for valid pty ranges, stolen from screen sources
dnl Note: this does break cross-compiling.
AC_DEFUN(AC_PTYRANGES, [
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



dnl
dnl AC_LBL_LIBRARY_NET
dnl
dnl Written by John Hawkinson <jhawk@mit.edu>. This code is in the Public
dnl Domain.
dnl
dnl usage:
dnl
dnl	AC_LBL_LIBRARY_NET
dnl
dnl results:
dnl
dnl	LIBS
dnl

dnl This test is for network applications that need socket() and
dnl gethostbyname() -ish functions.  Under Solaris, those applications
dnl need to link with "-lsocket -lnsl".  Under IRIX, they need to link
dnl with "-lnsl" but should *not* link with "-lsocket" because
dnl libsocket.a breaks a number of things (for instance:
dnl gethostbyname() under IRIX 5.2, and snoop sockets under most
dnl versions of IRIX).
dnl
dnl Unfortunately, many application developers are not aware of this,
dnl and mistakenly write tests that cause -lsocket to be used under
dnl IRIX.  It is also easy to write tests that cause -lnsl to be used
dnl under operating systems where neither are necessary (or useful),
dnl such as SunOS 4.1.4, which uses -lnsl for TLI.
dnl
dnl This test exists so that every application developer does not test
dnl this in a different, and subtly broken fashion.

dnl It has been argued that this test should be broken up into two
dnl seperate tests, one for the resolver libraries, and one for the
dnl libraries necessary for using Sockets API. Unfortunately, the two
dnl are carefully intertwined and allowing the autoconf user to use
dnl them independantly potentially results in unfortunate ordering
dnl dependancies -- as such, such component macros would have to
dnl carefully use indirection and be aware if the other components were
dnl executed. Since other autoconf macros do not go to this trouble,
dnl and almost no applications use sockets without the resolver, this
dnl complexity has not been implemented.
dnl
dnl The check for libresolv is in case you are attempting to link
dnl statically and happen to have a libresolv.a lying around (and no
dnl libnsl.a).
dnl
AC_DEFUN(AC_LBL_LIBRARY_NET, [
    # Most operating systems have gethostbyname() in the default searched
    # libraries (i.e. libc):
    AC_CHECK_FUNC(gethostbyname, ,
	# Some OSes (eg. Solaris) place it in libnsl:
	AC_CHECK_LIB(nsl, gethostbyname, , 
	    # Some strange OSes (SINIX) have it in libsocket:
	    AC_CHECK_LIB(socket, gethostbyname, ,
		# Unfortunately libsocket sometimes depends on libnsl.
		# AC_CHECK_LIB's API is essentially broken so the
		# following ugliness is necessary:
		AC_CHECK_LIB(socket, gethostbyname,
		    LIBS="-lsocket -lnsl $LIBS",
		    AC_CHECK_LIB(resolv, gethostbyname),
		    -lnsl))))
    AC_CHECK_FUNC(socket, , AC_CHECK_LIB(socket, socket, ,
	AC_CHECK_LIB(socket, socket, LIBS="-lsocket -lnsl $LIBS", ,
	    -lnsl)))
    # DLPI needs putmsg under HPUX so test for -lstr while we're at it
    AC_CHECK_LIB(str, putmsg)
    ])



dnl check whether actual code is generated for inlined functions
AC_DEFUN(AC_EXT_INLINE, [
AC_MSG_CHECKING(whether inline functions can be used extern)
AC_COMPILE_IFELSE([[
extern inline void blah();int main(){return 0;}
]],[AC_DEFINE(EXT_INLINE,[],[Define if inline functions can be called extern])
AC_MSG_RESULT([yes])],
AC_MSG_RESULT([no])
)
])
