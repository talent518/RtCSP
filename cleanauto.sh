#!/bin/sh

svn st | grep "?" | awk '{print $2;}' | xargs echo
