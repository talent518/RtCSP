dnl
dnl RTCSP_RUN_ONCE(namespace, variable, code)
dnl
dnl execute code, if variable is not set in namespace
dnl
AC_DEFUN([RTCSP_RUN_ONCE],[
	changequote({,})
	unique=`echo $2|sed 's/[^a-zA-Z0-9]/_/g'`
	changequote([,])
	cmd="echo $ac_n \"\$$1$unique$ac_c\""
	if test -n "$unique" && test "`eval $cmd`" = "" ; then
		eval "$1$unique=set"
		$3
	fi
])

dnl
dnl RTCSP_EXPAND_PATH(path, variable)
dnl
dnl expands path to an absolute path and assigns it to variable
dnl
AC_DEFUN([RTCSP_EXPAND_PATH],[
	if test -z "$1" || echo "$1" | grep '^/' >/dev/null ; then
		$2=$1
	else
		changequote({,})
		ep_dir=`echo $1|sed 's%/*[^/][^/]*/*$%%'`
		changequote([,])
		ep_realdir=`(cd "$ep_dir" && pwd)`
		$2="$ep_realdir"/`basename "$1"`
	fi
])

dnl
dnl RTCSP_ADD_INCLUDE(path [,before])
dnl
dnl add an include path. 
dnl if before is 1, add in the beginning of CFLAGS.
dnl
AC_DEFUN([RTCSP_ADD_INCLUDE],[
	if test "$1" != "/usr/include"; then
		RTCSP_EXPAND_PATH($1, ai_p)
		RTCSP_RUN_ONCE(INCLUDEPATH, $ai_p, [
			if test "$2"; then
				CFLAGS="-I$ai_p $CFLAGS"
			else
				CFLAGS="$CFLAGS -I$ai_p"
			fi
		])
	fi
])

dnl
dnl RTCSP_ADD_LIBRARY_WITH_PATH(path [,before])
dnl
dnl add an library path. 
dnl if before is 1, add in the beginning of LDFLAGS.
dnl
AC_DEFUN([RTCSP_ADD_LIBRARY_WITH_PATH],[
	if test "$1" != "/usr/lib"; then
		RTCSP_EXPAND_PATH($1, ai_p)
		RTCSP_RUN_ONCE(LIBRARYPATH, $ai_p, [
			export PKG_CONFIG_PATH="$ai_p/pkgconfig:$PKG_CONFIG_PATH"
			if test "$2"; then
				LDFLAGS="-L$ai_p -Wl,-rpath,$ai_p $LDFLAGS"
			else
				LDFLAGS="$LDFLAGS -L$ai_p -Wl,-rpath,$ai_p"
			fi
		])
	fi
])

dnl
dnl RTCSP_ADD_LIBRARY(path [,before])
dnl
dnl add an library link. 
dnl if before is 1, add in the beginning of LDFLAGS.
dnl
AC_DEFUN([RTCSP_ADD_LIBRARY],[
	RTCSP_EXPAND_PATH($1, ai_p)
	RTCSP_RUN_ONCE(LIBRARYLINK, $ai_p, [
		if test "$2"; then
			LDFLAGS="-l$ai_p $LDFLAGS"
		else
			LDFLAGS="$LDFLAGS -l$ai_p"
		fi
	])
])

dnl
dnl RTCSP_OUTPUT(file)
dnl
dnl Adds "file" to the list of files generated by AC_OUTPUT
dnl This macro can be used several times.
dnl
AC_DEFUN([RTCSP_OUTPUT],[
	RTCSP_EXPAND_PATH($1, ai_p)
	RTCSP_RUN_ONCE(OUTPUTPATH, $ai_p, [
		RTCSP_OUTPUT_FILES="$RTCSP_OUTPUT_FILES $1"
	])
])

dnl
dnl RTCSP_NEW_MODULE(extname, sources [, shared [, cflags [, ldflags] [, ldadd [, libadd]]]])
dnl
dnl Includes an module in the build.
dnl
dnl "extname" is the name of the modules/ subdir where the module resides.
dnl "sources" is a list of files relative to the subdir which are used
dnl to build the module.
dnl "shared" can be set to "shared" or "yes" to build the module as

AC_DEFUN([RTCSP_NEW_MODULE],[
	let extsize++
	test "$extnames" && extnames="$extnames,"
	extnames="$extnames\"$1\""
	test "$extmodules" && extmodules="$extmodules,"
	extmodules="$extmodules\&$1_module"
	test "$extincludes" && extincludes="$extincludes\n"
	extincludes="$extincludes#include \"modules/$1/mod_$1.h\""
	extheaders="$extheaders modules/$1/mod_$1.h"

	files=""
	for file in $2;do files="$files modules/$1/$file";done

	if test "$3" = "shared" || test "$3" = "yes"; then
		extdynamics="$extdynamics libbench_$1.la"
		extlibadd="$extlibadd libmod_$1.la"

		cat >> Makefile.am <<EOF
libmod_$1_la_SOURCES = $files
libmod_$1_la_CFLAGS = -DHAVE_RTCSP $4
libmod_$1_la_LDFLAGS = $no_undef $5
EOF
	else
		extstatics="$extstatics libmod_$1.a"
		extldadd="$extldadd libmod_$1.a"

		cat >> Makefile.am <<EOF
libmod_$1_a_SOURCES = $files
libmod_$1_a_CFLAGS = -DHAVE_RTCSP $4
EOF
	fi
])

dnl
dnl RTCSP_NEW_MODULE_WITH_BENCH(extname, sources [, shared [, cflags [, ldflags] [, ldadd [, libadd]]]])
dnl
dnl Includes an module in the build.
dnl
dnl "extname" is the name of the modules/ subdir where the module resides.
dnl "sources" is a list of files relative to the subdir which are used
dnl to build the module.
dnl "shared" can be set to "shared" or "yes" to build the module as

AC_DEFUN([RTCSP_NEW_MODULE_WITH_BENCH],[
	let extbenchsize++
	test "$extbenchnames" && extbenchnames="$extbenchnames,"
	extbenchnames="$extbenchnames\"$1\""
	test "$extbenchmodules" && extbenchmodules="$extbenchmodules,"
	extbenchmodules="$extbenchmodules\&$1_module"
	test "$extbenchincludes" && extbenchincludes="$extbenchincludes\n"
	extbenchincludes="$extbenchincludes#include \"modules/$1/mod_bench_$1.h\""
	extheaders="$extheaders modules/$1/mod_bench_$1.h"

	files=""
	for file in $2;do files="$files modules/$1/$file";done

	if test "$3" = "shared" || test "$3" = "yes"; then
		extdynamics="$extdynamics libbench_$1.la"
		extbenchlibadd="$extbenchlibadd libbench_$1.la"
		
		cat >> Makefile.am <<EOF
libbench_$1_la_SOURCES = $files
libbench_$1_la_CFLAGS = -DHAVE_BENCH $4
libbench_$1_la_LDFLAGS = $no_undef $5
EOF
	else
		extstatics="$extstatics libbench_$1.a"
		extbenchldadd="$extbenchldadd libbench_$1.a"

		cat >> Makefile.am <<EOF
libbench_$1_a_SOURCES = $files
libbench_$1_a_CFLAGS = -DHAVE_BENCH $4
EOF
	fi
])

AC_DEFUN([RTCSP_MAKE_MODULE],[
	cat internal.c.in | sed -e "s'@extincludes@'$extincludes'" -e "s'@extnames@'$extnames'" -e "s'@extmodules@'$extmodules'" -e "s'@extlength@'$extsize'" > internal.c
	cat bench_internal.c.in | sed -e "s'@extbenchincludes@'$extbenchincludes'" -e "s'@extbenchnames@'$extbenchnames'" -e "s'@extbenchmodules@'$extbenchmodules'" -e "s'@extbenchlength@'$extbenchsize'" > bench_internal.c

	sed -e "s'@extheaders@'$extheaders'" -e "s'@extldadd@'$extldadd'" -e "s'@extbenchldadd@'$extbenchldadd'" -e "s'@extlibadd@'$extlibadd'" -e "s'@extbenchlibadd@'$extbenchlibadd'" -e "s'@extstatics@'$extstatics'" -e "s'@extdynamics@'$extdynamics'" -i Makefile.am
	automake --add-missing --foreign 1>&2 2> /dev/null
])
