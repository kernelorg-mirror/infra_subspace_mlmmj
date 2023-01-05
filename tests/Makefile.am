AUTOMAKE_OPTIONS = foreign

check_PROGRAMS = mlmmj

mlmmj_SOURCES = mlmmj.c
EXTRA_DIST = *.sh
CLEANFILES= $(test_scripts:.sh=)

AM_CFLAGS = -g -Wall -std=gnu99 -D_GNU_SOURCE=1 -Wextra -pedantic -Wsign-compare -DDEFAULTTEXTDIR='"@textlibdir@"' -I$(srcdir)/../include @ATF_CFLAGS@
mlmmj_LDADD =  $(top_builddir)/src/libmlmmj.a @ATF_LIBS@
AM_LDFLAGS =
if WANT_COVERAGE
AM_CFLAGS += -O0 -ggdb -fprofile-arcs -ftest-coverage -fprofile-instr-generate -fcoverage-mapping
AM_LDFLAGS += -fprofile-instr-generate
endif

test_scripts =	mlmmj-send.sh \
		functional-tests.sh \
		mlmmj-maintd.sh \
		mlmmj-sub.sh \
		mlmmj-bounce.sh \
		mlmmj-list.sh
check_SCRIPTS=	$(test_scripts:.sh=)
SUFFIXES=	.sh

all: $(check_PROGRAMS) $(check_SCRIPTS)

check:	$(check_SCRIPTS) $(check_PROGRAMS)
	kyua --config=none test --kyuafile='$(builddir)/Kyuafile' \
		--build-root='$(builddir)'

.sh:
	install -m 755 $< $@
