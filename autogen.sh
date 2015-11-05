#!/bin/sh

./cleanauto.sh

for mod in $(ls modules)
do
	cat >> modules/Makefile.m4 <<EOF
enable_mod_${mod}=\$enable_modules
AC_ARG_ENABLE($mod, [  --enable-$mod turn on $mod], \$enable_mod_${mod}=\$enableval)
if test "\$enable_mod_${mod}" = "yes"; then
	sinclude(modules/$mod/Makefile.m4)
fi

EOF
done

aclocal && \
autoheader && \
autoconf && \
libtoolize --automake --copy --debug --force 2> /dev/null > /dev/null && \
automake --add-missing --foreign 2> /dev/null

if test "$1"; then
	./configure --with-libevent=/usr/local --with-glib=/usr/local
	exit $?
fi

exit 0
