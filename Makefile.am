bin_PROGRAMS=KBtin ansi2html ansi2txt
helpdir = $(pkgdatadir)
help_DATA= NEWS KBtin_help.gz
INCLUDES = -DDATA_PATH=\"$(helpdir)\"
man_MANS = ansi2html.6 ansi2txt.6 KBtin.6

BUILT_SOURCES=commands.h load_commands.h
KBtin_SOURCES=action.c alias.c antisub.c files.c glob.c help.c highlight.c history.c ivars.c llist.c main.c misc.c net.c parse.c path.c session.c substitute.c ticks.c utils.c variables.c events.c user.c run.c colors.c routes.c bind.c telnet.c crc.c hash.c regexp.c $(BUILT_SOURCES)
KBtin_LDADD=

ansi2html_SOURCES=ansi2html.c
ansi2txt_SOURCES=ansi2txt.c

EXTRA_DIST=tintin.h KBtin_help.in BUGS commands make_commands $(man_MANS) OLDNEWS help2ansi fixmanual terminfo.src KEYPAD FAQ
CLEANFILES=commands.h load_commands.h KBtin_help.gz manual.html

#kludge to prevent $(BUILT_SOURCES) from being distributed
dist-hook:
	for x in $(BUILT_SOURCES);do rm -f "$(distdir)/$$x";done

#link kbtin to KBtin
#Don't overwrite it on filesystems that ignore case, though.
install-exec-hook:
	if [ ! -e $(DESTDIR)$(bindir)/kbtin ]; then \
	    $(LN_S) -f KBtin $(DESTDIR)$(bindir)/kbtin; \
	fi

install-data-hook:
	if [ ! -e $(DESTDIR)$(mandir)/man6/kbtin.6 ]; then \
	    $(LN_S) -f KBtin.6 $(DESTDIR)$(mandir)/man6/kbtin.6; \
	fi

uninstall-local:
	if [ -L $(DESTDIR)$(bindir)/kbtin ]; then \
	    rm $(DESTDIR)$(bindir)/kbtin; \
	fi
	if [ -L $(DESTDIR)$(mandir)/man6/kbtin.6 ]; then \
	    rm $(DESTDIR)$(mandir)/man6/kbtin.6; \
	fi

commands.h: make_commands commands tintin.h
	$(srcdir)/make_commands <$(srcdir)/commands

load_commands.h: commands make_commands commands.h

tintin.h: configure.ac update-version
# automagically update version number in tintin.h if configure.ac has
# changed

KBtin_help.gz: KBtin_help.in
	gzip -9 <$(srcdir)/KBtin_help.in >KBtin_help.gz

# convert the help file to html
manual.html: KBtin_help.in help2ansi ansi2html fixmanual
	$(srcdir)/help2ansi <$(srcdir)/KBtin_help.in|./ansi2html|$(srcdir)/fixmanual $(srcdir)/KBtin_help.in >manual.html
#This won't work during a cross-compile, you will have to use a locally
#built version of ansi2html.

# install the terminfo data
install-terminfo: terminfo.src
	tic terminfo.src -o $(DESTDIR)$(TERMINFODIR)

# update version number in "tintin.h"
update-version:
	@if cat tintin.h | grep -e "$(VERSION)" >/dev/null; then \
		if [ tintin.h -ot configure.ac ]; then \
			touch -r tintin.h configure.ac; \
		fi; \
	else \
		if cat tintin.h | sed -e 's/#define VERSION_NUM.*/#define VERSION_NUM \"$(VERSION)\"/' > 'updated_tintin.h~'; then \
			mv 'updated_tintin.h~' tintin.h; \
		fi; \
	fi


# Remake, check and tag new version
cvs-dist: update-version KBtin check
	@if sed 1q NEWS | grep -e "$(VERSION)" > /dev/null; then :; else \
		echo "NEWS not updated; not releasing" 1>&2; \
		exit 1;			       \
	fi
	cvs tag `echo "Release-$(VERSION)" | sed 's/\./-/g'`
	$(MAKE) dist

