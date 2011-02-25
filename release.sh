#!/bin/sh
#
# $xxxterm$
# XXX This sucks, I know.  send me diffs to make it better
#

cd /tmp
rm -rf xxxterm
cvs -d anoncvs@opensource.conformal.com:/anoncvs/xxxterm -Q co xxxterm
REL=$(awk '/ \$xxxterm$4;}' xxxterm/xxxterm.c)
find xxxterm -name CVS -exec rm -rf {} \; 2> /dev/null
mv xxxterm xxxterm-$REL
tar zcf xxxterm-$REL.tgz xxxterm-$REL
echo /tmp/xxxterm-$REL.tgz
