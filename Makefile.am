## Process this file with automake to produce Makefile.in

AUTOMAKE_OPTIONS = foreign dist-xz
EXTRA_DIST = include LICENSE UPGRADE FAQ \
	     TUNABLES README.access README.archives README.listtexts man \
	     README.exim4 README.sendmail README.security README.qmail \
	     README.postfix README.footers
CLEANFILES = *~ mlmmj-*.tar.*

man1_MANS = man/mlmmj-bounce.1 man/mlmmj-make-ml.1 man/mlmmj-receive.1 \
	    man/mlmmj-sub.1 man/mlmmj-maintd.1 man/mlmmj-process.1 \
	    man/mlmmj-send.1 man/mlmmj-unsub.1 man/mlmmj-list.1

SUBDIRS = src listtexts contrib tests

check:	$(check_SCRIPTS) $(check_PROGRAMS)
	kyua --config=none test --kyuafile='$(builddir)/Kyuafile' \
		--build-root='$(builddir)'

