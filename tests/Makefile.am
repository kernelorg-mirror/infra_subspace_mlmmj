AUTOMAKE_OPTIONS = foreign

check_PROGRAMS = mlmmj fakesmtpd

mlmmj_SOURCES = mlmmj.c \
		mlmmj_tests.c

fakesmtpd_SOURCES=	fakesmtpd.c

EXTRA_DIST = *.sh *.h
CLEANFILES= $(test_scripts:.sh=)

AM_CFLAGS += @ATF_CFLAGS@
mlmmj_LDADD =  $(top_builddir)/src/libmlmmj.a @ATF_LIBS@ -lrt

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

.sh:
	install -m 755 $< $@
