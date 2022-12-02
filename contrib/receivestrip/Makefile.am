#

AUTOMAKE_OPTIONS = foreign

AM_CFLAGS = -g -Wall -pedantic -Wsign-compare -DDEFAULTTEXTDIR='"@textlibdir@"' -I$(srcdir)/../../include

if WANT_RECEIVESTRIP
  bin_PROGRAMS = mlmmj-receive-strip
endif

mlmmj_receive_strip_SOURCES = mlmmj-receive-strip.c $(MLMMJ_SOURCES)
mlmmj_receive_strip_LDADD =  $(top_builddir)/src/libmlmmj.a
