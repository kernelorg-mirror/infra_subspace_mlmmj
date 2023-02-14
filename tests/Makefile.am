AUTOMAKE_OPTIONS = foreign

check_PROGRAMS = mlmmj

mlmmj_SOURCES = mlmmj.c \
		mlmmj_tests.c
EXTRA_DIST = *.sh *.h
CLEANFILES= $(test_scripts:.sh=)

AM_CFLAGS = -g -Wall -std=gnu11 -D_GNU_SOURCE=1 -Wextra -pedantic -Wsign-compare -DDEFAULTTEXTDIR='"@textlibdir@"' -I$(srcdir)/../include @ATF_CFLAGS@
mlmmj_LDADD =  $(top_builddir)/src/libmlmmj.a @ATF_LIBS@ -lrt
AM_LDFLAGS =
if WANT_COVERAGE
AM_CFLAGS += -O0 -ggdb -fprofile-arcs -ftest-coverage -fprofile-generate
AM_LDFLAGS += -fprofile-generate
endif
if WANT_ASAN
AM_CFLAGS += -O0 -ggdb -fsanitize=address
AM_LDFLAGS += -fsanitize=address
endif
if WANT_UBSAN
AM_CFLAGS += -O0 -ggdb -fsanitize=undefined
AM_LDFLAGS += -fsanitize=undefined
endif

test_scripts =	mlmmj-send.sh \
		functional-tests.sh \
		mlmmj-maintd.sh \
		mlmmj-sub.sh \
		mlmmj-bounce.sh \
		mlmmj-list.sh \
		mlmmj-process.sh \
		mlmmj-receive.sh
check_SCRIPTS=	$(test_scripts:.sh=)
SUFFIXES=	.sh

all: $(check_PROGRAMS) $(check_SCRIPTS)

check:	$(check_SCRIPTS) $(check_PROGRAMS)
	kyua --config=none test --kyuafile='$(builddir)/Kyuafile' \
		--build-root='$(builddir)'

.sh:
	install -m 755 $< $@
