#!/bin/sh

./cleanauto.sh

for m4 in $(find modules -name "Makefile.m4")
do
	echo "sinclude($m4)" >> modules/Makefile.m4
done

aclocal && \
autoheader && \
autoconf && \
libtoolize --automake --copy --debug --force 2> /dev/null > /dev/null && \
automake --add-missing --foreign 2> /dev/null

./configure --with-libevent=/opt/ssp

exit $?
