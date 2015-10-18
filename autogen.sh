#!/bin/sh

rm -rf configure Makefile.in config.log depcomp config.h .deps Makefile config.status stamp-h1 config.h.in autom4te.cache missing aclocal.m4 hook_internal.c install-sh
rm -f modules/Makefile.m4

for m4 in $(find modules -name "Makefile.m4")
do
	echo "sinclude($m4)" >> modules/Makefile.m4
done

aclocal && \
autoheader && \
autoconf && \
automake --add-missing --foreign 2> /dev/null && \
./configure --with-libevent=/opt/ssp

exit 0