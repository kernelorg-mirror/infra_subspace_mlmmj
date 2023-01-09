#

AUTOMAKE_OPTIONS = foreign

AM_CFLAGS = -g -Wall -pedantic -Wsign-compare -DDEFAULTTEXTDIR='"@textlibdir@"' -I$(srcdir)/../../include
AM_LDFLAGS =
if WANT_COVERAGE
AM_CFLAGS += -O0 -ggdb -fprofile-arcs -ftest-coverage -fprofile-instr-generate -fcoverage-mapping
AM_LDFLAGS += -fprofile-instr-generate
endif
if WANT_ASAN
AM_CFLAGS += -O0 -ggdb -fsanitize=address
AM_LDFLAGS += -fsanitize=address
endif
if WANT_UBSAN
AM_CFLAGS += -O0 -ggdb -fsanitize=undefined
AM_LDFLAGS += -fsanitize=undefined
endif

if WANT_RECEIVESTRIP
  bin_PROGRAMS = mlmmj-receive-strip
endif

mlmmj_receive_strip_SOURCES = mlmmj-receive-strip.c $(MLMMJ_SOURCES)
mlmmj_receive_strip_LDADD =  $(top_builddir)/src/libmlmmj.a
