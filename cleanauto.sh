#!/bin/sh

svn propget svn:ignore | xargs rm -rf
rm -f modules/Makefile.m4
rm -f *.a *.la *.so *.o *.lo
