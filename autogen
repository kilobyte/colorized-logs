#!/bin/sh
set -e

touch -t 197101010100 Makefile.protos
aclocal
autoheader
if [ ! -e config.rpath ]
  then
    cp -p /usr/share/gettext/config.rpath config.rpath
fi
automake --add-missing --copy
autoconf
