bin_PROGRAMS=ansi2html ansi2txt ttyrec2ansi pipetty
man_MANS = ansi2html.1 ansi2txt.1 ttyrec2ansi.1 pipetty.1

AM_CFLAGS=-Wall

ansi2html_SOURCES=ansi2html.c
ansi2txt_SOURCES=ansi2txt.c
pipetty_SOURCES=pipetty.c pty.c pty.h

EXTRA_DIST=autogen.sh VERSION get_version

VERSION: FORCE
	[ ! -d .git ] || $(srcdir)/get_version >VERSION
FORCE:

SUBDIRS = . tests
