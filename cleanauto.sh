#!/bin/sh

test -f "Makefile" && make uninstall clean > /dev/null
svn propget svn:ignore | xargs rm -rf
rm -f modules/Makefile.m4
rm -f *.a *.la *.so *.o *.lo
