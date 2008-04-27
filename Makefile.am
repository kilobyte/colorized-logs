bin_PROGRAMS=KBtin ansi2html ansi2txt ttyrec2ansi
helpdir = $(pkgdatadir)
help_DATA= NEWS KBtin_help.gz
INCLUDES = -DDATA_PATH=\"$(helpdir)\"
#"
man_MANS = ansi2html.1 ansi2txt.1 KBtin.6 ttyrec2ansi.1

BUILT_SOURCES=commands.h load_commands.h profinit.h protos.h
KBtin_C_FILES=tintin.h unicode.h action.c alias.c antisub.c files.c glob.c help.c highlight.c history.c ivars.c llist.c main.c misc.c net.c parse.c path.c session.c substitute.c ticks.c utils.c variables.c events.c user.c run.c colors.c routes.c bind.c telnet.c crc.c hash.c regexp.c prof.c hooks.c unicode.c ui.h user_pipe.c user_tty.c translit.h wcwidth.c malloc.h
KBtin_SOURCES=$(KBtin_C_FILES) $(BUILT_SOURCES)
KBtin_LDADD=@GLIB_LIBS@ @ZLIB_LIBS@ @LTLIBICONV@

AM_CFLAGS=@GLIB_CFLAGS@

ansi2html_SOURCES=ansi2html.c
ansi2txt_SOURCES=ansi2txt.c

KBtin_DOCS=BUGS OLDNEWS KEYPAD FAQ
EXTRA_DIST=KBtin_help.in commands make_commands $(man_MANS) help2ansi fixmanual terminfo.src prof_init kbtin.ico make_protos $(KBtin_DOCS) autogen
CLEANFILES=commands.h load_commands.h KBtin_help.gz manual.html profinit.h protos.h
DISTCLEANFILES=_stdint.h

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

profinit.h: $(KBtin_C_FILES) prof_init
	$(srcdir)/prof_init $(srcdir) $(KBtin_C_FILES)

protos.h: $(KBtin_C_FILES) make_protos
	$(srcdir)/make_protos $(srcdir) $(KBtin_C_FILES)

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

# Remake, check and tag new version
cvs-dist: KBtin check
	@if sed 1q NEWS | grep -e "$(VERSION)" > /dev/null; then :; else \
		echo "NEWS not updated; not releasing" 1>&2; \
		exit 1;                               \
	fi
	cvs tag `echo "Release-$(VERSION)" | sed 's/\./-/g'`
	$(MAKE) dist

SUBDIRS = tests

mac: clean
	./configure --prefix=/usr				&& \
	make							&& \
	make manual.html					&& \
	rm -rf mac                                              && \
	mkdir mac						&& \
	make install DESTDIR=mac				&& \
	cd mac/usr/share/					&& \
	mkdir doc						&& \
	cd doc							&& \
	cp -p ../../../../{manual.html,KEYPAD,FAQ,AUTHORS} .	&& \
	cd ../../..						&& \
	mkdmg -s . -i ../kbtin-$(VERSION).dmg -zlib

bin: $(bin_PROGRAMS) $(help_DATA)
	mkdir -p ~/bin
	cp -p $(bin_PROGRAMS) $(help_DATA) ~/bin

unbin: 
	cd ~/bin && rm -rf $(bin_PROGRAMS) $(help_DATA)
