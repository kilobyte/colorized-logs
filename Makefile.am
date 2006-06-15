bin_PROGRAMS=KBtin

KBtin_SOURCES=action.c alias.c antisub.c files.c glob.c help.c highlight.c history.c ivars.c llist.c main.c misc.c net.c parse.c path.c session.c substitute.c text.c ticks.c utils.c variables.c events.c user.c run.c colors.c
KBtin_LDADD=-lcurses

EXTRA_DIST=tintin.h

# since tt is almost never installed by "make install" strip it at once :)
# all-local:
# 	strip tt

# automagically update version number in tintin.h if configure.in has
# changed
tintin.h: configure.in
	$(MAKE) update-version

# update version number in "tintin.h"
update-version:
	@if cat tintin.h | grep -e "$(VERSION)" >/dev/null; then \
           	touch -r tintin.h configure.in; \
	else \
		if cat tintin.h | sed -e 's/#define VERSION_NUM.*/#define VERSION_NUM \"$(VERSION)\"/' > /tmp/updated_tintin.h; then \
			mv /tmp/updated_tintin.h tintin.h; \
		fi; \
	fi

# Remake, check and tag new version
cvs-dist: update-version KB-tin check
	@if sed 1q NEWS | grep -e "$(VERSION)" > /dev/null; then :; else \
		echo "NEWS not updated; not releasing" 1>&2; \
		exit 1;                               \
	fi
	cvs tag `echo "Release-$(VERSION)" | sed 's/\./-/g'`
	$(MAKE) dist

