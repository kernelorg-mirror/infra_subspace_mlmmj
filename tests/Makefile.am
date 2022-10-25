AUTOMAKE_OPTIONS = foreign

check_PROGRAMS = mlmmj

mlmmj_SOURCES = mlmmj.c

AM_CFLAGS = -g -Wall -pedantic -Wsign-compare -DDEFAULTTEXTDIR='"@textlibdir@"' -I$(srcdir)/../include @ATF_CFLAGS@
mlmmj_LDADD =  $(top_builddir)/src/libmlmmj.a @ATF_LIBS@

test_scripts =	mlmmj-send.sh \
		functional-tests.sh
check_SCRIPTS=	$(test_scripts:.sh=)
SUFFIXES=	.sh

all: $(check_PROGRAMS) $(check_SCRIPTS)

check:	$(check_SCRIPTS) $(check_PROGRAMS)
	kyua --config=none test --kyuafile='$(builddir)/Kyuafile' \
		--build-root='$(builddir)'

.sh:
	install -m 755 $< $@
