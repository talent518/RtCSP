#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.63])
m4_define([M4_RTCSP_NAME], [rtcsp])
m4_define([M4_RTCSP_VERSION], [1.0])
AC_INIT(M4_RTCSP_NAME, M4_RTCSP_VERSION, [talent518@yeah.net])

AC_DEFINE([RTCSP_NAME],["M4_RTCSP_NAME"],[application name])
AC_DEFINE([RTCSP_VERSION],["M4_RTCSP_VERSION"],[application version])
AC_CONFIG_SRCDIR([config.h.in])
AC_CONFIG_HEADERS([config.h])

AM_INIT_AUTOMAKE(M4_RTCSP_NAME, M4_RTCSP_VERSION)

# Checks for programs.
AC_PROG_CC
AC_PROG_RANLIB
AC_PROG_LIBTOOL
LT_INIT

# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS([arpa/inet.h inttypes.h locale.h malloc.h netinet/in.h stddef.h stdlib.h string.h strings.h sys/socket.h sys/time.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_TYPE_SIZE_T

# Checks for library functions.
AC_FUNC_FORK
AC_FUNC_MALLOC
AC_FUNC_STRTOD
AC_CHECK_FUNCS([bzero getcwd gettimeofday memchr memset socket strchr strdup strerror strrchr strstr strtol strtoul])

sinclude(acinclude.m4)

CFLAGS="$CFLAGS -lstdc++ -w"

AC_ARG_ENABLE(debug, [  --enable-debug turn on debug], CFLAGS="-g -O0 -Wall -DDEBUG")
AC_ARG_ENABLE(debuginfo, [  --enable-debuginfo turn on debug conn info], CFLAGS="-g -O0 -Wall -DDEBUG_INFO")
AC_ARG_ENABLE(debugfull, [  --enable-debugfull turn on debug full], CFLAGS="-g -O0 -DDEBUG -Wall -DDEBUG_INFO")
AC_ARG_ENABLE(runtime, [  --enable-runtime turn on print run time], CFLAGS="$CFLAGS -DDEBUG_RUN_TIME")
AC_ARG_ENABLE(skip-bench, [  --enable-skip-bench skip bench for send_recv handler], CFLAGS="$CFLAGS -DHAVE_SKIP_BENCH")

# libevent is required
AC_ARG_WITH(libevent, [  --with-libevent=DIR         where to find the libevent library, default /usr], , [with_libevent=/usr])
RTCSP_ADD_INCLUDE($with_libevent/include)
RTCSP_ADD_LIBRARY_WITH_PATH($with_libevent/lib)
AC_CHECK_LIB([event], event_base_loop, [event_LIB=yes], [event_LIB=no])
if test x$event_LIB = xno; then
	AC_CHECK_HEADERS([event.h],[
		EVENT_STATIC_LIB="-levent"
		AC_SUBST(EVENT_STATIC_LIB)
	],[
		AC_MSG_ERROR([In -levent library not found function event_base_loop])
	])
else
	AC_CHECK_HEADERS([event.h],[
		LDFLAGS="$LDFLAGS -levent"
		EVENT_STATIC_LIB=
		AC_SUBST(EVENT_STATIC_LIB)
	],[
		AC_MSG_ERROR([In -levent library not found function event_base_loop])
	])
fi

# glib is required
AC_ARG_WITH(glib, [  --with-glib=DIR         where to find the glib-2.0 library, default /usr], , [with_glib=/usr])
RTCSP_ADD_INCLUDE($with_glib/include/glib-2.0)
if test -d "$with_glib/lib64/glib-2.0/include"; then
	RTCSP_ADD_INCLUDE($with_glib/lib64/glib-2.0/include)
	RTCSP_ADD_LIBRARY_WITH_PATH($with_glib/lib64)
else
	RTCSP_ADD_INCLUDE($with_glib/lib/glib-2.0/include)
	RTCSP_ADD_LIBRARY_WITH_PATH($with_glib/lib)
fi
PKG_CHECK_MODULES(glib, glib-2.0 >= 2.40.0)
AC_CHECK_LIB(glib-2.0, g_hash_table_new, [glib_LIB=yes], [glib_LIB=no])
if test x$glib_LIB = xno; then
	AC_CHECK_HEADERS([glib.h],[
		GLIB_STATIC_LIB="-lglib-2.0 -lintl -liconv"
		AC_SUBST(GLIB_STATIC_LIB)
	],[
		AC_MSG_ERROR([In -lglib-2.0 library not found function g_hash_table_new])
	])
else
	AC_CHECK_HEADERS([glib.h],[
		LDFLAGS="$LDFLAGS -lglib-2.0"
		GLIB_STATIC_LIB=
		AC_SUBST(GLIB_STATIC_LIB)
	],[
		AC_MSG_ERROR([In -lglib-2.0 library not found function g_hash_table_new])
	])
fi

AC_CHECK_LIB([m], pow,,[AC_MSG_ERROR([math support requested, but not found])])
AC_CHECK_LIB([pthread], pthread_create,,[AC_MSG_ERROR([libpthread support requested, but not found])])

# mysql is required
AC_ARG_WITH(mysql_config, [  --with-mysql-config=DIR         where to find the mysqlclient library, default /usr/bin/mysql_config], , [with_mysql_config=/usr/bin/mysql_config])
if test ! -f "$with_mysql_config"; then
	AC_MSG_ERROR([>= mysqlclient-5.1 support requested, but not found])
fi
CFLAGS="$($with_mysql_config --cflags) $CFLAGS"
LDFLAGS="$($with_mysql_config --libs_r) -Wl,-rpath,$($with_mysql_config --variable=pkglibdir) $LDFLAGS"
AC_CHECK_LIB([mysqlclient_r], mysql_real_connect,,[AC_MSG_ERROR([>= mysqlclient-5.1 support requested, but not found])])

\cp Makefile.am.in Makefile.am

no_undef=
if test "$build_os" = "cygwin"; then
	no_undef=-no-undefined
fi

enable_modules=yes
AC_ARG_ENABLE(modules, [  --enable-modules turn on modules [[default=yes]]], enable_modules=$enableval)

let extsize=0
let extbenchsize=0

sinclude(modules/Makefile.m4)

RTCSP_MAKE_MODULE

AC_CONFIG_FILES([Makefile $RTCSP_OUTPUT_FILES])

AC_OUTPUT
