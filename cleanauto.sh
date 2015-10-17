#!/bin/sh

make clean > /dev/null
svn st | grep "?" | awk '{print $2;}' | xargs echo
