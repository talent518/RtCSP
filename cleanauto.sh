#!/bin/sh

test -f "Makefile" && make uninstall clean > /dev/null

#svn propget svn:ignore | xargs echo

rm -rf *.exe .deps Makefile Makefile.am Makefile.in aclocal.m4 autom4te.cache bench_internal.c compile config.h config.h.in config.log config.status configure depcomp internal.c install-sh missing rtcspb rtcspd stamp-h1 *.o *.a *.la *.lo *.so config.guess ltmain.sh config.sub libtool confdefs.h conftest.c conftest.err

rm -f modules/Makefile.m4

