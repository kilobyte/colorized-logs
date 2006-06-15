bin_PROGRAMS=KBtin
helpdir = $(datadir)/KBtin
help_DATA= NEWS KBtin_help.gz
INCLUDES = -DDATA_PATH=\"$(helpdir)\"

KBtin_SOURCES=action.c alias.c antisub.c files.c glob.c help.c highlight.c history.c ivars.c llist.c main.c misc.c net.c parse.c path.c session.c substitute.c text.c ticks.c utils.c variables.c events.c user.c run.c colors.c routes.c bind.c telnet.c crc.c hash.c
KBtin_LDADD=

EXTRA_DIST=tintin.h KBtin_help.gz BUGS

# since tt is almost never installed by "make install" strip it at once :)
all-local:
	strip KBtin

# automagically update version number in tintin.h if configure.in has
# changed
tintin.h: configure.in
	$(MAKE) update-version

KBtin_help.gz:
	@if [ -e KBtin_help ]; then \
		if [ KBtin_help -nt KBtin_help.gz ]; then \
			echo "Updating help." \
			gzip -9 <KBtin_help >KBtin_help.gz; \
		fi; \
	fi
#no update_help because help is distributed in compressed form

# update version number in "tintin.h"
update-version:
	@if cat tintin.h | grep -e "$(VERSION)" >/dev/null; then \
           	touch -r tintin.h configure.in; \
	else \
		if cat tintin.h | sed -e 's/#define VERSION_NUM.*/#define VERSION_NUM \"$(VERSION)\"/' > /tmp/updated_tintin.h; then \
			mv /tmp/updated_tintin.h tintin.h; \
		fi; \
	fi

#update the help file
update-help: KBtin_help
	gzip -9 <KBtin_help >KBtin_help.gz

# Remake, check and tag new version
cvs-dist: update-version KBtin check
	@if sed 1q NEWS | grep -e "$(VERSION)" > /dev/null; then :; else \
		echo "NEWS not updated; not releasing" 1>&2; \
		exit 1;                               \
	fi
	cvs tag `echo "Release-$(VERSION)" | sed 's/\./-/g'`
	$(MAKE) dist

