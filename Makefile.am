## Process this file with automake to produce Makefile.in

EXTRA_DIST = include LICENSE UPGRADE FAQ \
	     TUNABLES README.access README.archives README.listtexts man \
	     README.exim4 README.sendmail README.security README.qmail \
	     README.postfix README.footers \
	     Kyuafile tests/Kyuafile \
	     tests/dsnmail

CLEANFILES = *~ mlmmj-*.tar.*

man1_MANS = man/mlmmj-bounce.1 man/mlmmj-make-ml.1 man/mlmmj-receive.1 \
	    man/mlmmj-sub.1 man/mlmmj-maintd.1 man/mlmmj-process.1 \
	    man/mlmmj-send.1 man/mlmmj-unsub.1 man/mlmmj-list.1

SUBDIRS = src listtexts contrib

if WANT_TESTS
AM_CFLAGS += @ATF_CFLAGS@

check_PROGRAMS = \
	tests/fakesmtpd \
	tests/mlmmj

dist_noinst_HEADERS = \
	tests/mlmmj_tests.h

tests_mlmmj_SOURCES = \
	tests/mlmmj.c \
	tests/mlmmj_tests.c
tests_mlmmj_LDADD = \
	$(top_builddir)/src/libmlmmj.a \
	@ATF_LIBS@ -lrt

tests_fakesmtpd_SOURCES = \
	tests/fakesmtpd.c

EXTRA_DIST += \
	tests/*.sh

CLEANFILES += \
	$(test_scripts:.sh=)

test_scripts = \
	tests/mlmmj-send.sh \
	tests/functional-tests.sh \
	tests/mlmmj-maintd.sh \
	tests/mlmmj-sub.sh \
	tests/mlmmj-bounce.sh \
	tests/mlmmj-list.sh \
	tests/mlmmj-process.sh \
	tests/mlmmj-receive.sh

check_SCRIPTS = \
	$(test_scripts:.sh=)

# Copy the test scripts striped of the '.sh' extension in the build directory
SUFFIXES = .sh
.sh:
	install -m 755 $< $@

# Run the Kyua test suite when the 'check' target is invoked
check-local: $(check_SCRIPTS) $(check_PROGRAMS)
	failed=no; \
	kyua --config='$(top_builddir)/kyua.conf' test \
	    --kyuafile='$(top_srcdir)/Kyuafile' --build-root='$(top_builddir)' \
	    || failed=yes; \
	if [ "$${failed}" = yes ]; then \
	    kyua report --results-file='$(abs_top_srcdir)' \
	        --verbose --results-filter=broken,failed; \
	    exit 1; \
	fi

# Build the test programs when the 'all' target is invoked
all-local: $(check_PROGRAMS) $(check_SCRIPTS)
endif

