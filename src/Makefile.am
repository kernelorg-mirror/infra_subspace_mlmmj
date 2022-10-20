#

AUTOMAKE_OPTIONS = foreign

AM_CFLAGS = -g -Wall -std=gnu99 -D_GNU_SOURCE=1 -Wextra -pedantic -Wsign-compare -DDEFAULTTEXTDIR='"@textlibdir@"' -I$(srcdir)/../include

bin_PROGRAMS = mlmmj-send mlmmj-receive mlmmj-process mlmmj-sub \
               mlmmj-unsub mlmmj-bounce mlmmj-maintd mlmmj-list

noinst_LIBRARIES = libmlmmj.a

bin_SCRIPTS = mlmmj-make-ml

EXTRA_DIST = mlmmj-make-ml

libmlmmj_a_SOURCES=	mail-functions.c chomp.c incindexfile.c \
			checkwait_smtpreply.c init_sockfd.c strgen.c \
			random-int.c print-version.c log_error.c mygetline.c \
			getaddrsfromfd.c readn.c \
			subscriberfuncs.c find_email_adr.c dumpfd2fd.c \
			listcontrol.c send_help.c prepstdreply.c \
			do_all_the_voodoo_here.c log_oper.c send_list.c \
			unistr.c gethdrline.c send_digest.c \
			writen.c getlistaddr.c mylocking.c strgen.c statctrl.c \
			ctrlvalue.c readn.c getlistdelim.c ctrlvalues.c

mlmmj_send_SOURCES = mlmmj-send.c
mlmmj_send_LDADD=	libmlmmj.a

mlmmj_receive_SOURCES = mlmmj-receive.c
mlmmj_receive_LDADD=	libmlmmj.a

mlmmj_process_SOURCES = mlmmj-process.c
mlmmj_process_LDADD=	libmlmmj.a

mlmmj_sub_SOURCES = mlmmj-sub.c
mlmmj_sub_LDADD=	libmlmmj.a

mlmmj_unsub_SOURCES = mlmmj-unsub.c
mlmmj_unsub_LDADD = 	libmlmmj.a

mlmmj_bounce_SOURCES = mlmmj-bounce.c
mlmmj_bounce_LDADD = 	libmlmmj.a

mlmmj_maintd_SOURCES = mlmmj-maintd.c
mlmmj_maintd_LDADD = 	libmlmmj.a

mlmmj_list_SOURCES = mlmmj-list.c
mlmmj_list_LDADD = 	libmlmmj.a

install-exec-hook:
	ln -f -s mlmmj-receive $(DESTDIR)$(bindir)/mlmmj-recieve
	ln -f -s mlmmj-make-ml $(DESTDIR)$(bindir)/mlmmj-make-ml.sh
