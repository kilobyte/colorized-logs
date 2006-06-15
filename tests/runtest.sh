#!/bin/sh

echo "#mess all 0;#delay 5 {#showme TIMEOUT;#end}" >_be_quiet_
rm -rf aa

export LC_ALL=en_US.UTF-8
export LC_CTYPE=$LC_ALL

#$PATH has the local binary first.
KBtin -p -q _be_quiet_ <testin

rm -rf _be_quiet_
