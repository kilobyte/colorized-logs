bin_PROGRAMS=KBtin
helpdir = $(pkgdatadir)
help_DATA= NEWS KBtin_help.gz
INCLUDES = -DDATA_PATH=\"$(helpdir)\"

KBtin_SOURCES=action.c alias.c antisub.c files.c glob.c help.c highlight.c history.c ivars.c llist.c main.c misc.c net.c parse.c path.c session.c substitute.c ticks.c utils.c variables.c events.c user.c run.c colors.c routes.c bind.c telnet.c crc.c hash.c
KBtin_LDADD=

EXTRA_DIST=tintin.h KBtin_help BUGS commands make_commands.sh
CLEANFILES=commands.h load_commands.h KBtin_help.gz

commands.h: make_commands.sh commands
	$(srcdir)/make_commands.sh <$(srcdir)/commands

#fake dependency needed for multi-CPU systems
load_commands.h: make_commands.sh commands commands.h
	$(srcdir)/make_commands.sh <$(srcdir)/commands

# since tt is almost never installed by "make install" strip it at once :)
all-local:
	strip KBtin

# automagically update version number in tintin.h if configure.ac has
# changed
tintin.h: configure.ac
	$(MAKE) update-version

KBtin_help.gz: KBtin_help
	gzip -9 <$(srcdir)/KBtin_help >KBtin_help.gz

# update version number in "tintin.h"
update-version:
	@if cat tintin.h | grep -e "$(VERSION)" >/dev/null; then \
           	touch -r tintin.h configure.ac; \
	else \
		if cat tintin.h | sed -e 's/#define VERSION_NUM.*/#define VERSION_NUM \"$(VERSION)\"/' > /tmp/updated_tintin.h; then \
			mv /tmp/updated_tintin.h tintin.h; \
		fi; \
	fi

# Remake, check and tag new version
cvs-dist: update-version KBtin check
	@if sed 1q NEWS | grep -e "$(VERSION)" > /dev/null; then :; else \
		echo "NEWS not updated; not releasing" 1>&2; \
		exit 1;                               \
	fi
	cvs tag `echo "Release-$(VERSION)" | sed 's/\./-/g'`
	$(MAKE) dist

