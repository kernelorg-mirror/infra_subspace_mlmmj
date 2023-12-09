# Process this file with autoconf to produce a configure script.
AC_INIT([mlmmj],[1.4.1],[bapt@freebsd.org])

VERSION=$PACKAGE_VERSION
AC_SUBST(VERSION)

AM_INIT_AUTOMAKE([subdir-objects])
AC_CONFIG_SRCDIR([src/mlmmj-receive.c])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CC
AC_PROG_RANLIB

# Checks for libraries.
AC_CHECK_LIB(socket,socket)
AC_CHECK_LIB(nsl,gethostbyname)
AC_CHECK_LIB(iconv,iconv_open)

AC_CHECK_HEADERS([arpa/inet.h fcntl.h netinet/in.h stddef.h stdlib.h string.h])
AC_CHECK_HEADERS([sys/socket.h syslog.h unistd.h time.h])
AC_CHECK_HEADER([iconv.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_SIZE_T

savex=$exec_prefix
test "x$prefix" = xNONE && prefix=$ac_default_prefix
test "x$exec_prefix" = xNONE && exec_prefix=$prefix
tmp=$datadir/$PACKAGE/text.skel
while textlibdir=`eval echo "$tmp"`; test "x$textlibdir" != "x$tmp"; do tmp=$textlibdir; done
exec_prefix=$savex

AC_SUBST(textlibdir)

AC_ARG_ENABLE([coverage],
  AS_HELP_STRING([--enable-coverage],
		 [build with coverage flags activated (default is no)]))
AM_CONDITIONAL(WANT_COVERAGE, test x"$enable_coverage" = xyes)

AC_ARG_ENABLE([asan],
  AS_HELP_STRING([--enable-asan],
		 [build with libasan (default is no)]))
AM_CONDITIONAL(WANT_ASAN, test x"$enable_asan" = xyes)

AC_ARG_ENABLE([ubsan],
  AS_HELP_STRING([--enable-ubsan],
		 [build with libubsan (default is no)]))
AM_CONDITIONAL(WANT_UBSAN, test x"$enable_ubsan" = xyes)

AC_ARG_ENABLE([receive-strip],
  AS_HELP_STRING([--enable-receive-strip],
                 [build mlmmj-receive-strip (default is no)]))
AM_CONDITIONAL(WANT_RECEIVESTRIP, test x"$enable_receive_strip" = xyes)

# Checks for library functions.
AC_FUNC_MALLOC
AC_CHECK_FUNCS([ftruncate memset socket strerror strncasecmp snprintf fcntl])
AC_CHECK_FUNCS([nanosleep time strftime syslog regcomp regexec])
AC_CHECK_FUNCS([arc4random_uniform copy_file_range])
PKG_CHECK_MODULES([ATF], [atf-c])

AC_CONFIG_FILES([Makefile])
AC_CONFIG_FILES([src/Makefile])
AC_CONFIG_FILES([src/mlmmj-make-ml])
AC_CONFIG_FILES([listtexts/Makefile])
AC_CONFIG_FILES([contrib/Makefile])
AC_CONFIG_FILES([contrib/receivestrip/Makefile])
AC_CONFIG_FILES([tests/Makefile])
AC_CONFIG_FILES([tests/Kyuafile])
AC_CONFIG_FILES([tests/test_env.sh])
AC_OUTPUT
