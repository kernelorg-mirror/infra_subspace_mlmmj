# Process this file with autoconf to produce a configure script.
AC_PREREQ([2.69])
AC_INIT([mlmmj],[1.4.5],[bapt@nours.eu])

VERSION=$PACKAGE_VERSION
AC_SUBST(VERSION)

AC_CONFIG_SRCDIR([src/mlmmj-receive.c])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_AUX_DIR([build-aux])

AM_INIT_AUTOMAKE([1.13 dist-xz foreign subdir-objects])

# Enable silent rules by default
AM_SILENT_RULES([yes])

# Checks for programs.
AC_PROG_CC
AC_PROG_RANLIB

# Checks for libraries.
AC_CHECK_LIB(socket,socket)
AC_CHECK_LIB(iconv,iconv_open)

AC_CHECK_HEADERS([syslog.h ])
AC_CHECK_HEADER([iconv.h])

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

AS_IF([test x"$enable_coverage" = xyes], [
  COVERAGE_CFLAGS="-O0 -ggdb -fprofile-arcs -ftest-coverage -fprofile-generate"
  COVERAGE_LDFLAGS="-fprofile-generate"
])

AC_ARG_ENABLE([asan],
  AS_HELP_STRING([--enable-asan],
		 [build with libasan (default is no)]))

AS_IF([test x"$enable_asan" = xyes], [
  ASAN_CFLAGS="-O0 -ggdb -fsanitize=address"
  ASAN_LDFLAGS="-fsanitize=address"
])

AC_ARG_ENABLE([ubsan],
  AS_HELP_STRING([--enable-ubsan],
		 [build with libubsan (default is no)]))

AS_IF([test x"$enable_ubsan" = xyes], [
  UBSAN_CFLAGS="-O0 -ggdb -fsanitize=undefined"
  UBSAN_LDFLAGS="-fsanitize=undefined"
])

AC_ARG_ENABLE([receive-strip],
  AS_HELP_STRING([--enable-receive-strip],
                 [build mlmmj-receive-strip (default is no)]))
AM_CONDITIONAL(WANT_RECEIVESTRIP, test x"$enable_receive_strip" = xyes)

AC_ARG_ENABLE([tests],
  AS_HELP_STRING([--disable-tests],
		 [build tests (default is yes)]),
  [:], [enable_tests=yes]
)

# Checks for library functions.
AC_FUNC_MALLOC
AC_CHECK_FUNCS([syslog])
AC_CHECK_FUNCS([arc4random_uniform copy_file_range])

AS_IF([test x"$enable_tests" = xyes], [
  PKG_CHECK_MODULES([ATF], [atf-c])
  AC_PATH_PROG([ATFSH], [atf-sh])
  AC_PATH_PROG([KYUA], [kyua])

  AS_IF([test "x$KYUA" == "x"], [
    AC_MSG_ERROR([A 'kyua' binary is required when the tests are enabled. Set the PATH or use the KYUA environment variable to specify its location.])
  ])

  AS_IF([test "x$ATFSH" == "x"], [
    AC_MSG_ERROR([An 'atf-sh' binary is required when the tests are enabled. Set the PATH or use the ATFSH environment variable to specify its location.])
  ])
])
AM_CONDITIONAL([WANT_TESTS], [test x"$enable_tests" = xyes])

AM_CPPFLAGS="-I\$(top_srcdir)/include -DDEFAULTTEXTDIR='\"\$(textlibdir)\"'"
AC_SUBST(AM_CPPFLAGS)

AM_CFLAGS="-g -Wall -std=gnu99 -D_GNU_SOURCE=1 -Wextra -pedantic -Wsign-compare $COVERAGE_CFLAGS $ASAN_CFLAGS $UBSAN_CFLAGS"
AC_SUBST(AM_CFLAGS)

AM_LDFLAGS="$COVERAGE_LDFLAGS $ASAN_LDFLAGS $UBSAN_LDFLAGS"
AC_SUBST(AM_LDFLAGS)

AC_CONFIG_FILES([Makefile])
AC_CONFIG_FILES([kyua.conf])
AC_CONFIG_FILES([src/Makefile])
AC_CONFIG_FILES([src/mlmmj-make-ml])
AC_CONFIG_FILES([listtexts/Makefile])
AC_CONFIG_FILES([contrib/Makefile])
AC_CONFIG_FILES([contrib/receivestrip/Makefile])
AC_CONFIG_FILES([tests/test_env.sh])
AC_CONFIG_FILES([tests/functional-tests],[chmod +x tests/functional-tests])
AC_CONFIG_FILES([tests/mlmmj-bounce],[chmod +x tests/mlmmj-bounce])
AC_CONFIG_FILES([tests/mlmmj-list],[chmod +x tests/mlmmj-list])
AC_CONFIG_FILES([tests/mlmmj-maintd],[chmod +x tests/mlmmj-maintd])
AC_CONFIG_FILES([tests/mlmmj-process],[chmod +x tests/mlmmj-process])
AC_CONFIG_FILES([tests/mlmmj-receive],[chmod +x tests/mlmmj-receive])
AC_CONFIG_FILES([tests/mlmmj-send],[chmod +x tests/mlmmj-send])
AC_CONFIG_FILES([tests/mlmmj-sub],[chmod +x tests/mlmmj-sub])

AC_OUTPUT
