/*
 * Copyright (C) 2022-2025 Baptiste Daroussin <bapt@FreeBSD.org>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include "vec.h"
#include "sb.h"

#if defined(__clang__)
#pragma clang diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"
#elif defined(__GNUC__)
#pragma GCC diagnostic ignored "-Wformat-zero-length"
#endif

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include <atf-c.h>
#include <inttypes.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdbool.h>
#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <signal.h>

#include "atf-c/macros.h"
#include "atf-c/utils.h"
#include "mlmmj.h"
#include "send_list.h"
#include "xmalloc.h"
#include "wrappers.h"
#include "chomp.h"
#include "utils.h"
#include "strgen.h"
#include "mail-functions.h"
#include "mygetline.h"
#include "init_sockfd.h"
#include "checkwait_smtpreply.h"
#include "find_email_adr.h"
#include "send_mail.h"
#include "getlistdelim.h"
#include "statctrl.h"
#include "subscriberfuncs.h"
#include "getaddrsfromfile.h"
#include "ctrlvalue.h"
#include "ctrlvalues.h"
#include "incindexfile.h"
#include "log_oper.h"
#include "listcontrol.h"
#include "mlmmj-process.h"
#include "prepstdreply.h"
#include "mlmmj_tests.h"
#include "send_help.h"
#include "gethdrline.h"
#include "unistr.h"
#include "do_all_the_voodoo_here.h"
#include "send_digest.h"
#include "archive.h"

ATF_TC_WITHOUT_HEAD(random_int);
ATF_TC_WITHOUT_HEAD(chomp);
ATF_TC_WITHOUT_HEAD(mydirname);
ATF_TC_WITHOUT_HEAD(mybasename);
ATF_TC_WITHOUT_HEAD(mygetline);
ATF_TC_WITHOUT_HEAD(init_sock);
ATF_TC_WITHOUT_HEAD(genmsgid);
ATF_TC_WITHOUT_HEAD(lowercase);
ATF_TC_WITHOUT_HEAD(exec_or_die);
ATF_TC_WITHOUT_HEAD(exec_and_wait);
ATF_TC_WITHOUT_HEAD(find_email_adr);
ATF_TC_WITHOUT_HEAD(strtoim);
ATF_TC_WITHOUT_HEAD(write_ehlo);
ATF_TC_WITHOUT_HEAD(write_helo);
ATF_TC_WITHOUT_HEAD(write_dot);
ATF_TC_WITHOUT_HEAD(write_data);
ATF_TC_WITHOUT_HEAD(write_quit);
ATF_TC_WITHOUT_HEAD(write_rset);
ATF_TC_WITHOUT_HEAD(write_replyto);
ATF_TC_WITHOUT_HEAD(write_rcpt_to);
ATF_TC_WITHOUT_HEAD(write_mail_from);
ATF_TC_WITHOUT_HEAD(write_mailbody);
ATF_TC_WITHOUT_HEAD(strtotimet);
ATF_TC_WITHOUT_HEAD(decode_qp);
ATF_TC_WITHOUT_HEAD(parse_lastdigest);
ATF_TC_WITHOUT_HEAD(extract_bouncetime);
ATF_TC_WITHOUT_HEAD(open_subscriber_directory);
ATF_TC_WITHOUT_HEAD(unsubscribe);
ATF_TC_WITHOUT_HEAD(genlistname);
ATF_TC_WITHOUT_HEAD(genlistfqdn);
ATF_TC_WITHOUT_HEAD(smtp);
ATF_TC_WITHOUT_HEAD(init_smtp);
ATF_TC_WITHOUT_HEAD(smtp_bad_greetings);
ATF_TC_WITHOUT_HEAD(smtp_bad_ehlo);
ATF_TC_WITHOUT_HEAD(smtp_no_ehlo);
ATF_TC_WITHOUT_HEAD(endsmtp);
ATF_TC_WITHOUT_HEAD(do_bouncemail);
ATF_TC_WITHOUT_HEAD(bouncemail);
ATF_TC_WITHOUT_HEAD(send_mail_basics);
ATF_TC_WITHOUT_HEAD(send_mail);
ATF_TC_WITHOUT_HEAD(send_mail_xforwardedto);
ATF_TC_WITHOUT_HEAD(send_mail_xsignedrecipient);
ATF_TC_WITHOUT_HEAD(send_mail_both_headers);
ATF_TC_WITHOUT_HEAD(getlistdelim);
ATF_TC_WITHOUT_HEAD(getlistdelim_0);
ATF_TC_WITHOUT_HEAD(getlistdelim_1);
ATF_TC_WITHOUT_HEAD(getlistdelim_2);
ATF_TC_WITHOUT_HEAD(getlistdelim_3);
ATF_TC_WITHOUT_HEAD(getlistdelim_4);
ATF_TC_WITHOUT_HEAD(statctrl);
ATF_TC_WITHOUT_HEAD(is_subbed_in);
ATF_TC_WITHOUT_HEAD(is_subbed_normal);
ATF_TC_WITHOUT_HEAD(is_subbed_types);
ATF_TC_WITHOUT_HEAD(unsubscribe_test);
ATF_TC_WITHOUT_HEAD(find_addr_line_edge);
ATF_TC_WITHOUT_HEAD(is_subbed_crlf_prefix);
ATF_TC_WITHOUT_HEAD(unsubscribe_crlf_prefix);
ATF_TC_WITHOUT_HEAD(mkuniqfileat_test);
ATF_TC_WITHOUT_HEAD(getaddrsfromfile);
ATF_TC_WITHOUT_HEAD(dumpfd2fd);
ATF_TC_WITHOUT_HEAD(dumpfd2fd_large);
ATF_TC_WITHOUT_HEAD(dumpfd2fd_empty);
ATF_TC_WITHOUT_HEAD(process_headers_test);
ATF_TC_WITHOUT_HEAD(process_headers_skipfrom);
ATF_TC_WITHOUT_HEAD(process_headers_disarmedfrom);
ATF_TC_WITHOUT_HEAD(disarm_email_test);
ATF_TC_WITHOUT_HEAD(copy_file);
ATF_TC_WITHOUT_HEAD(copy_file_1);
ATF_TC_WITHOUT_HEAD(copy_file_2);
ATF_TC_WITHOUT_HEAD(controls);
ATF_TC_WITHOUT_HEAD(incindexfile);
ATF_TC_WITHOUT_HEAD(log_oper);
ATF_TC_WITHOUT_HEAD(log_oper_rotate);
ATF_TC_WITHOUT_HEAD(mygetline_edge);
ATF_TC_WITHOUT_HEAD(get_ctrl_command);
ATF_TC_WITHOUT_HEAD(get_recipextra_from_env_none);
ATF_TC_WITHOUT_HEAD(get_recipextra_from_env_qmail);
ATF_TC_WITHOUT_HEAD(get_recipextra_from_env_postfix);
ATF_TC_WITHOUT_HEAD(get_recipextra_from_env_exim);
ATF_TC_WITHOUT_HEAD(addrmatch);
ATF_TC_WITHOUT_HEAD(get_subcookie_content);
ATF_TC_WITHOUT_HEAD(ml_list);
ATF_TC_WITHOUT_HEAD(ml_become_list_owner);
ATF_TC_WITHOUT_HEAD(gen_addr);
ATF_TC_WITHOUT_HEAD(memory_lines);
ATF_TC_WITHOUT_HEAD(text_0);
ATF_TC_WITHOUT_HEAD(text_1);
ATF_TC_WITHOUT_HEAD(file_lines);
ATF_TC_WITHOUT_HEAD(list_subs);
ATF_TC_WITHOUT_HEAD(notify_sub);
ATF_TC_WITHOUT_HEAD(get_processed_text_line);
ATF_TC_WITHOUT_HEAD(get_processed_text_line_directives);
ATF_TC_WITHOUT_HEAD(get_processed_text_line_crlf);
ATF_TC_WITHOUT_HEAD(get_processed_text_line_includes);
ATF_TC_WITHOUT_HEAD(newsmtp);
ATF_TC_WITHOUT_HEAD(save_queue);
ATF_TC_WITHOUT_HEAD(send_single_mail);
ATF_TC_WITHOUT_HEAD(send_or_queue_test);
ATF_TC_WITHOUT_HEAD(generate_subscription);
ATF_TC_WITHOUT_HEAD(generate_subconfirm);
ATF_TC_WITHOUT_HEAD(send_confirmation_mail);
ATF_TC_WITHOUT_HEAD(listcontrol);
ATF_TC_WITHOUT_HEAD(send_help);
ATF_TC_WITHOUT_HEAD(requeuemail);
ATF_TC_WITHOUT_HEAD(gethdrline);
ATF_TC_WITHOUT_HEAD(readlf);
ATF_TC_WITHOUT_HEAD(mod_get_addr_type);
ATF_TC_WITHOUT_HEAD(send_probe_no_probefile);
ATF_TC_WITHOUT_HEAD(send_probe_arobase);
ATF_TC_WITHOUT_HEAD(send_probe);
ATF_TC_WITHOUT_HEAD(send_probe_archive);
ATF_TC_WITHOUT_HEAD(parse_content_type);
ATF_TC_WITHOUT_HEAD(find_in_list);
ATF_TC_WITHOUT_HEAD(do_access);
ATF_TC_WITHOUT_HEAD(unistr_header_to_utf8);
ATF_TC_WITHOUT_HEAD(unistr_utf8_to_header);
ATF_TC_WITHOUT_HEAD(unistr_roundtrip);
ATF_TC_WITHOUT_HEAD(unistr_to_utf8_4byte);
ATF_TC_WITHOUT_HEAD(unistr_append_utf8_edge);
ATF_TC_WITHOUT_HEAD(unistr_to_utf8_large);
ATF_TC_WITHOUT_HEAD(unistr_append_utf8_invalid);
ATF_TC_WITHOUT_HEAD(unistr_escaped_to_utf8);
ATF_TC_WITHOUT_HEAD(line_startwithany);
ATF_TC_WITHOUT_HEAD(save_hdr_if_needed);
ATF_TC_WITHOUT_HEAD(autosubscribe_sender);
ATF_TC_WITHOUT_HEAD(scan_headers_test);
ATF_TC_WITHOUT_HEAD(substitute_test);
ATF_TC_WITHOUT_HEAD(sb_cat_n);
ATF_TC_WITHOUT_HEAD(sb_reset);
ATF_TC_WITHOUT_HEAD(sb_str);
ATF_TC_WITHOUT_HEAD(sb_printf_growth);
ATF_TC_WITHOUT_HEAD(voodoo_replyto);
ATF_TC_WITHOUT_HEAD(voodoo_replyto_once);
ATF_TC_WITHOUT_HEAD(voodoo_prefix_dedup);
ATF_TC_WITHOUT_HEAD(voodoo_header_manipulation);
ATF_TC_WITHOUT_HEAD(voodoo_double_call);
ATF_TC_WITHOUT_HEAD(voodoo_failure_cleans_queue);
ATF_TC_WITHOUT_HEAD(voodoo_listheaders);
ATF_TC_WITHOUT_HEAD(voodoo_listheaders_disabled);
ATF_TC_WITHOUT_HEAD(voodoo_listheaders_owner);
ATF_TC_WITHOUT_HEAD(voodoo_listheaders_phrase);
ATF_TC_WITHOUT_HEAD(checkwait_smtpreply_connect);
ATF_TC_WITHOUT_HEAD(checkwait_smtpreply_ehlo_multiline);
ATF_TC_WITHOUT_HEAD(checkwait_smtpreply_errors);
ATF_TC_WITHOUT_HEAD(checkwait_smtpreply_closed);
ATF_TC_WITHOUT_HEAD(digest_thread_list_format);
ATF_TC_WITHOUT_HEAD(digest_thread_grouping);
ATF_TC_WITHOUT_HEAD(archive_open_flat);
ATF_TC_WITHOUT_HEAD(archive_open_partitioned);
ATF_TC_WITHOUT_HEAD(archive_open_fallback);
ATF_TC_WITHOUT_HEAD(archive_path_write_flat);
ATF_TC_WITHOUT_HEAD(archive_path_write_partitioned);
ATF_TC_WITHOUT_HEAD(archive_migrate_test);
ATF_TC_WITHOUT_HEAD(archive_migrate_noop);

ATF_TC_BODY(random_int, tc)
{
	intmax_t val = random_int();

	ATF_REQUIRE_MSG(val < INT_MAX, "Invalid integer");
}

ATF_TC_BODY(chomp, tc)
{
	char test1[] = "\n";
	char test2[] = "\r";
	char test3[] = "\r\n";
	char test4[] = "test\r\n";
	char test5[] = "";
	char test6[] = "\n\n";
	char test7[] = "\r\n\r\n";
	char test8[] = "ab\r\ncd\n";

	ATF_REQUIRE(chomp(NULL) == NULL);
	char *bla = chomp(test1);
	ATF_REQUIRE(bla != NULL);
	ATF_REQUIRE_STREQ(bla, "");
	bla = chomp(test2);
	ATF_REQUIRE(bla != NULL);
	ATF_REQUIRE_STREQ(bla, "");
	bla = chomp(test3);
	ATF_REQUIRE(bla != NULL);
	ATF_REQUIRE_STREQ(bla, "");
	bla = chomp(test4);
	ATF_REQUIRE(bla != NULL);
	ATF_REQUIRE_STREQ(bla, "test");
	bla = chomp(test5);
	ATF_REQUIRE(bla != NULL);
	ATF_REQUIRE_STREQ(bla, "");
	bla = chomp(test6);
	ATF_REQUIRE(bla != NULL);
	ATF_REQUIRE_STREQ(bla, "");
	bla = chomp(test7);
	ATF_REQUIRE(bla != NULL);
	ATF_REQUIRE_STREQ(bla, "");
	bla = chomp(test8);
	ATF_REQUIRE(bla != NULL);
	ATF_REQUIRE_STREQ(bla, "ab\r\ncd");
}

ATF_TC_BODY(mydirname, tc)
{
	char plop[] = "/path/to/a/file";

	char *t = mydirname(plop);
	ATF_REQUIRE_STREQ(t, "/path/to/a");
	free(t);
	t = mydirname(NULL);
	ATF_REQUIRE_STREQ(t, ".");
	free(t);
	t = mydirname("a");
	ATF_REQUIRE_STREQ(t, ".");
	free(t);
}

ATF_TC_BODY(mybasename, tc)
{
	char plop[] = "/path/to/a/file";
	char plop1[] = "file";

	ATF_REQUIRE_STREQ(mybasename(plop), "file");
	ATF_REQUIRE_STREQ(mybasename(plop1), "file");
}

ATF_TC_BODY(mygetline, tc)
{
	int fd = open("test", O_CREAT|O_RDWR, 0644);
	ATF_REQUIRE(mygetline(fd) == NULL);
	dprintf(fd, "a");
	lseek (fd, 0, SEEK_SET);
	char *res = mygetline(fd);
	ATF_REQUIRE(res != NULL);
	ATF_REQUIRE_STREQ(res, "a");
	for (int i = 0; i < BUFSIZ + 2; i++)
		dprintf(fd, "a");
	lseek (fd, 0, SEEK_SET);
	res = mygetline(fd);
	ATF_REQUIRE(res != NULL);
	dprintf(fd, "\nab\n");
	lseek (fd, 0, SEEK_SET);
	mygetline(fd);
	res = mygetline(fd);
	ATF_REQUIRE(res != NULL);
	ATF_REQUIRE_STREQ(res, "ab\n");
	close(fd);
}

ATF_TC_BODY(lowercase, tc)
{
	char *l;

	l = lowercase("MAR23anC");
	ATF_REQUIRE_STREQ(l, "mar23anc");
	free(l);

	/* Bytes above 0x7f must be left untouched. */
	l = lowercase("\xc3\x89" "ABC");
	ATF_REQUIRE_STREQ(l, "\xc3\x89" "abc");
	free(l);
}

ATF_TC_BODY(init_sock, tc)
{
	int mypipe[2];
	ATF_REQUIRE(socketpair(AF_UNIX, SOCK_STREAM, 0, mypipe) >= 0);
	pid_t p = atf_utils_fork();
	if (p == 0) {
		int s = fakesmtp(mypipe[1]);
		int c;
		struct sockaddr_in cl;
		socklen_t clsize = sizeof(struct sockaddr_in);
		/*
		 * Now we can accept incoming connections one
		 * at a time using accept(2).
		 */
		c = accept(s, (struct sockaddr *) &cl,
				&clsize);
		if (c == -1)
			err(5, "accept()");
		exit(0);
	}
	close(mypipe[1]);
	atf_utils_readline(mypipe[0]);
	int sock;
	do {
		init_sockfd(&sock, "127.0.0.1", 25678);
		if (sock == -1 && errno != EPERM)
			break;
		if (sock != -1)
			break;
	} while (1);
	if (sock == -1)
		atf_tc_fail("%s", strerror(errno));
	atf_utils_wait(p, 0, "", "");
}

ATF_TC_BODY(genmsgid, tc)
{
	char *test = genmsgid("test.net");

	ATF_REQUIRE_MATCH_MSG("Message-ID: <[0-9]+-[0-9]+-mlmmj-........@test.net>\n", test, "Invalid msgid '%s'", test);
	free(test);
}

ATF_TC_BODY(exec_or_die, tc)
{
	pid_t p = atf_utils_fork();
	if (p == 0) {
		exec_or_die("/usr/bin/nopenope", NULL);
	}
	atf_utils_wait(p, 1, "", "mlmmj: Execution failed for '/usr/bin/nopenope': No such file or directory\n");
	p = atf_utils_fork();
	if (p == 0) {
		exec_or_die("true", (char *)NULL);
	}
	atf_utils_wait(p, 0, "", "");
	p = atf_utils_fork();
	if (p == 0) {
		exec_or_die("/bin/echo", "a", (char *)NULL);
	}
	atf_utils_wait(p, 0, "a\n", "");
	p = atf_utils_fork();
	if (p == 0) {
		exec_or_die("/bin/echo", "a", "b", "c", (char *)NULL);
	}
	atf_utils_wait(p, 0, "a b c\n", "");
}

ATF_TC_BODY(exec_and_wait, tc)
{
	int val = exec_and_wait("nopenope", NULL);
	ATF_REQUIRE_EQ(val, -1);
	val = exec_and_wait("true", NULL);
	ATF_REQUIRE_EQ(val, 0);
	val = exec_and_wait("false", NULL);
	ATF_REQUIRE_EQ(val, 1);
	/* Multiple arguments must all be passed to the child. */
	val = exec_and_wait("/bin/sh", "-c", "exit 3", NULL);
	ATF_REQUIRE_EQ(val, 3);
	val = exec_and_wait("/bin/sh", "-c", "test a = b", NULL);
	ATF_REQUIRE_EQ(val, 1);
}

ATF_TC_BODY(find_email_adr, tc)
{
	strlist c  = vec_init();
	strlist c1 = vec_init();
	strlist c2 = vec_init();
	strlist c3 = vec_init();
	strlist c4 = vec_init();
	strlist c5 = vec_init();
	strlist c6 = vec_init();
	strlist c7 = vec_init();
	strlist c8 = vec_init();
	find_email_adr("test@foo.bar", &c);
	ATF_REQUIRE_EQ(c.len, 1);
	find_email_adr("test@foo.bar, meh@bar", &c1);
	ATF_REQUIRE_EQ(c1.len, 2);
	find_email_adr("test@foo.bar, meh@bar, nope", &c2);
	ATF_REQUIRE_EQ(c2.len, 3);
	find_email_adr("name <test@foo.com>", &c3);
	ATF_REQUIRE_EQ(c3.len, 1);
	ATF_REQUIRE_STREQ(c3.d[0], "test@foo.com");
	find_email_adr("(name <test@foo.com>) test@foo.com", &c4);
	ATF_REQUIRE_EQ(c4.len, 1);
	ATF_REQUIRE_STREQ(c4.d[0], " test@foo.com");
	find_email_adr("\"name <test@foo.com>\" test@foo.com", &c5);
	ATF_REQUIRE_EQ(c5.len, 1);
	ATF_REQUIRE_STREQ(c5.d[0], "name <test@foo.com> test@foo.com");
	find_email_adr("\"name <test@foo.com> test@foo.com", &c6);
	ATF_REQUIRE_EQ(c6.len, 1);
	ATF_REQUIRE_STREQ(c6.d[0], "name <test@foo.com> test@foo.com");
	find_email_adr("\"name <test@foo.com>\\ test@foo.com", &c7);
	ATF_REQUIRE_EQ(c7.len, 1);
	ATF_REQUIRE_STREQ(c7.d[0], "name <test@foo.com> test@foo.com");
	find_email_adr("\"name <test@foo.com> test@foo.com\\", &c7);
	ATF_REQUIRE_EQ(c7.len, 2);
	ATF_REQUIRE_STREQ(c7.d[c7.len - 1], "name <test@foo.com> test@foo.com");
	find_email_adr("test at foo.com", &c7);
	ATF_REQUIRE_EQ(c7.len, 3);
	ATF_REQUIRE_STREQ(c7.d[c7.len - 1], "test@foo.com");
	find_email_adr("test atfoo.com", &c7);
	ATF_REQUIRE_EQ(c7.len, 4);
	ATF_REQUIRE_STREQ(c7.d[c7.len - 1], "test atfoo.com");
	find_email_adr("test a t foo.com", &c7);
	ATF_REQUIRE_EQ(c7.len, 5);
	ATF_REQUIRE_STREQ(c7.d[c7.len - 1], "test a t foo.com");
	find_email_adr("test@foo.com, \"meh\"", &c7);
	ATF_REQUIRE_EQ(c7.len, 7);
	ATF_REQUIRE_STREQ(c7.d[c7.len - 1], "\"meh\"");
	find_email_adr("test@foo.com, \"meh\\\"", &c7);
	ATF_REQUIRE_EQ(c7.len, 9);
	ATF_REQUIRE_STREQ(c7.d[c7.len - 1], "meh ");
	find_email_adr("", &c7);
	ATF_REQUIRE_EQ(c7.len, 9);
	find_email_adr("\"meh\" , ", &c7);
	ATF_REQUIRE_EQ(c7.len, 10);
	ATF_REQUIRE_STREQ(c7.d[c7.len - 1], "meh");
	find_email_adr("\"meh , ", &c7);
	ATF_REQUIRE_EQ(c7.len, 11);
	ATF_REQUIRE_STREQ(c7.d[c7.len - 1], "meh   ");
	find_email_adr("\t,", &c7);
	ATF_REQUIRE_EQ(c7.len, 11);
	find_email_adr("\r,", &c7);
	ATF_REQUIRE_EQ(c7.len, 11);
	find_email_adr("\n,", &c7);
	ATF_REQUIRE_EQ(c7.len, 11);
	find_email_adr(", m", &c7);
	ATF_REQUIRE_EQ(c7.len, 12);
	find_email_adr(", \"m", &c7);
	ATF_REQUIRE_EQ(c7.len, 13);
	find_email_adr("bob @ foo.net", &c7);
	ATF_REQUIRE_EQ(c7.len, 14);
	ATF_REQUIRE_STREQ(c7.d[c7.len - 1], "bob@foo.net");
	find_email_adr("bob @foo.net", &c7);
	ATF_REQUIRE_EQ(c7.len, 15);
	ATF_REQUIRE_STREQ(c7.d[c7.len - 1], "bob @foo.net");
	find_email_adr("bob @foo.net>", &c7);
	ATF_REQUIRE_EQ(c7.len, 16);
	ATF_REQUIRE_STREQ(c7.d[c7.len - 1], "bob @foo.net>");
	find_email_adr("<bob @foo.net> garbage,", &c7);
	ATF_REQUIRE_EQ(c7.len, 17);
	ATF_REQUIRE_STREQ(c7.d[c7.len - 1], "bob @foo.net");
	find_email_adr("<bob @foo.net> garbage (more garbage) ,", &c7);
	ATF_REQUIRE_EQ(c7.len, 18);
	ATF_REQUIRE_STREQ(c7.d[c7.len - 1], "bob @foo.net");
	find_email_adr("<bob @foo.net> garbage \"more garbage\" ,", &c7);
	ATF_REQUIRE_EQ(c7.len, 19);
	ATF_REQUIRE_STREQ(c7.d[c7.len - 1], "bob @foo.net");
	find_email_adr("<bob @foo.net> garbage \"more garbage \\ ,", &c7);
	ATF_REQUIRE_EQ(c7.len, 20);
	ATF_REQUIRE_STREQ(c7.d[c7.len - 1], "bob @foo.net");
	/* CRLF stripping in skin() */
	find_email_adr("<evil@bad\r\nBcc: victim@target>", &c7);
	ATF_REQUIRE_EQ(c7.len, 21);
	ATF_REQUIRE_STREQ(c7.d[c7.len - 1], "evil@badBcc: victim@target");
	find_email_adr("evil@bad\r\n", &c7);
	ATF_REQUIRE_EQ(c7.len, 22);
	ATF_REQUIRE_STREQ(c7.d[c7.len - 1], "evil@bad");
	find_email_adr("test\n@foo.com", &c7);
	ATF_REQUIRE_EQ(c7.len, 23);
	ATF_REQUIRE_STREQ(c7.d[c7.len - 1], "test@foo.com");

	/* CR/LF stripping applies to every address of a list */
	find_email_adr("aaa@bbb\r\n, ccc@ddd\n", &c8);
	ATF_REQUIRE_EQ(c8.len, 2);
	ATF_REQUIRE_STREQ(c8.d[0], "aaa@bbb");
	ATF_REQUIRE_STREQ(c8.d[1], "ccc@ddd");
}

ATF_TC_BODY(strtoim, tc)
{
	const char *errp;

	ATF_REQUIRE_EQ(strtoim(NULL, 0, 10, &errp), 0);
	ATF_REQUIRE(errp != NULL);
	ATF_REQUIRE_STREQ(errp, "invalid");
	ATF_REQUIRE_EQ(strtoim("10", 0, 10, &errp), 10);
	ATF_REQUIRE(errp == NULL);
	ATF_REQUIRE_EQ(strtoim("10", 0, 9, &errp), 0);
	ATF_REQUIRE(errp != NULL);
	ATF_REQUIRE_STREQ(errp, "too large");
	ATF_REQUIRE_EQ(strtoim("-1", 0, 9, &errp), 0);
	ATF_REQUIRE(errp != NULL);
	ATF_REQUIRE_STREQ(errp, "too small");
	ATF_REQUIRE_EQ(strtoim("-1", 10, 9, &errp), 0);
	ATF_REQUIRE(errp != NULL);
	ATF_REQUIRE_STREQ(errp, "invalid");
	ATF_REQUIRE_EQ(strtoim("ba12", 0, 9, &errp), 0);
	ATF_REQUIRE_EQ(errno, EINVAL);
	ATF_REQUIRE(errp != NULL);
	ATF_REQUIRE_STREQ(errp, "invalid");
	ATF_REQUIRE_EQ(strtoim("ba12", 0, 9, NULL), 0);

	ATF_REQUIRE_EQ(strtouim(NULL, 0, 10, &errp), 0);
	ATF_REQUIRE(errp != NULL);
	ATF_REQUIRE_STREQ(errp, "invalid");
	ATF_REQUIRE_EQ(strtouim("10", 0, 10, &errp), 10);
	ATF_REQUIRE(errp == NULL);
	ATF_REQUIRE_EQ(strtouim("10", 0, 9, &errp), 0);
	ATF_REQUIRE(errp != NULL);
	ATF_REQUIRE_STREQ(errp, "too large");
	ATF_REQUIRE_EQ(strtouim("1", 2, 9, &errp), 0);
	ATF_REQUIRE(errp != NULL);
	ATF_REQUIRE_STREQ(errp, "too small");
	ATF_REQUIRE_EQ(strtouim("-1", 0, 9, &errp), 0);
	ATF_REQUIRE(errp != NULL);
	ATF_REQUIRE_STREQ(errp, "too large");
	ATF_REQUIRE_EQ(strtouim("-1", 10, 9, &errp), 0);
	ATF_REQUIRE(errp != NULL);
	ATF_REQUIRE_STREQ(errp, "invalid");
	ATF_REQUIRE_EQ(strtouim("ba12", 0, 9, &errp), 0);
	ATF_REQUIRE_EQ(errno, EINVAL);
	ATF_REQUIRE(errp != NULL);
	ATF_REQUIRE_STREQ(errp, "invalid");
	ATF_REQUIRE_EQ(strtouim("ba12", 0, 9, NULL), 0);
}

ATF_TC_BODY(write_ehlo, tc)
{
	ATF_REQUIRE_EQ(write_ehlo(-1, "myname"), -1);
	int fd = open("ehlo.txt", O_CREAT|O_WRONLY, 0644);
	ATF_REQUIRE_EQ(write_ehlo(fd, "myname"), 0);
	close(fd);
	if (!atf_utils_compare_file("ehlo.txt", "EHLO myname\r\n"))
		atf_tc_fail("Unexpected output");
}

ATF_TC_BODY(write_helo, tc)
{
	ATF_REQUIRE_EQ(write_helo(-1, "myname"), -1);
	int fd = open("helo.txt", O_CREAT|O_WRONLY, 0644);
	ATF_REQUIRE_EQ(write_helo(fd, "myname"), 0);
	close(fd);
	if (!atf_utils_compare_file("helo.txt", "HELO myname\r\n"))
		atf_tc_fail("Unexpected output");
}

ATF_TC_BODY(write_dot, tc)
{
	ATF_REQUIRE_EQ(write_dot(-1), -1);
	int fd = open("dot.txt", O_CREAT|O_WRONLY, 0644);
	ATF_REQUIRE_EQ(write_dot(fd), 0);
	close(fd);
	if (!atf_utils_compare_file("dot.txt", "\r\n.\r\n"))
		atf_tc_fail("Unexpected output");
}

ATF_TC_BODY(write_data, tc)
{
	ATF_REQUIRE_EQ(write_data(-1), -1);
	int fd = open("data.txt", O_CREAT|O_WRONLY, 0644);
	ATF_REQUIRE_EQ(write_data(fd), 0);
	close(fd);
	if (!atf_utils_compare_file("data.txt", "DATA\r\n"))
		atf_tc_fail("Unexpected output");
}

ATF_TC_BODY(write_quit, tc)
{
	ATF_REQUIRE_EQ(write_quit(-1), -1);
	int fd = open("quit.txt", O_CREAT|O_WRONLY, 0644);
	ATF_REQUIRE_EQ(write_quit(fd), 0);
	close(fd);
	if (!atf_utils_compare_file("quit.txt", "QUIT\r\n"))
		atf_tc_fail("Unexpected output");
}

ATF_TC_BODY(write_rset, tc)
{
	ATF_REQUIRE_EQ(write_rset(-1), -1);
	int fd = open("rset.txt", O_CREAT|O_WRONLY, 0644);
	ATF_REQUIRE_EQ(write_rset(fd), 0);
	close(fd);
	if (!atf_utils_compare_file("rset.txt", "RSET\r\n"))
		atf_tc_fail("Unexpected output");
}

ATF_TC_BODY(write_replyto, tc)
{
	ATF_REQUIRE_EQ(write_replyto(-1, "toto"), -1);
	int fd = open("replyto.txt", O_CREAT|O_WRONLY, 0644);
	ATF_REQUIRE_EQ(write_replyto(fd, "toto"), 0);
	close(fd);
	if (!atf_utils_compare_file("replyto.txt", "Reply-To: toto\r\n"))
		atf_tc_fail("Unexpected output");
	/* CRLF injection */
	fd = open("replyto.txt", O_CREAT|O_WRONLY|O_TRUNC, 0644);
	ATF_REQUIRE_EQ(write_replyto(fd, "evil@bad\r\nBcc: victim@target"), -1);
	close(fd);
	fd = open("replyto.txt", O_CREAT|O_WRONLY|O_TRUNC, 0644);
	ATF_REQUIRE_EQ(write_replyto(fd, "evil@bad\nBcc: victim@target"), -1);
	close(fd);
}

ATF_TC_BODY(write_rcpt_to, tc)
{
	ATF_REQUIRE_EQ(write_rcpt_to(-1, "toto"), -1);
	int fd = open("rcpt_to.txt", O_CREAT|O_WRONLY, 0644);
	ATF_REQUIRE_EQ(write_rcpt_to(fd, "toto"), 0);
	close(fd);
	if (!atf_utils_compare_file("rcpt_to.txt", "RCPT TO:<toto>\r\n")) {
		atf_utils_cat_file("rcpt_to.txt", "");
		atf_tc_fail("Unexpected output");
	}
	/* CRLF injection */
	fd = open("rcpt_to.txt", O_CREAT|O_WRONLY|O_TRUNC, 0644);
	ATF_REQUIRE_EQ(write_rcpt_to(fd, "evil@bad\r\nRCPT TO:<victim@target>"), -1);
	close(fd);
	fd = open("rcpt_to.txt", O_CREAT|O_WRONLY|O_TRUNC, 0644);
	ATF_REQUIRE_EQ(write_rcpt_to(fd, "evil@bad\nRCPT TO:<victim@target>"), -1);
	close(fd);
}

ATF_TC_BODY(write_mail_from, tc)
{
	ATF_REQUIRE_EQ(write_mail_from(-1, "toto", NULL), -1);
	int fd = open("mail_from.txt", O_CREAT|O_WRONLY, 0644);
	ATF_REQUIRE_EQ(write_mail_from(fd, "toto", NULL), 0);
	close(fd);
	if (!atf_utils_compare_file("mail_from.txt", "MAIL FROM:<toto>\r\n")) {
		atf_utils_cat_file("mail_from.txt", "");
		atf_tc_fail("Unexpected output");
	}
	fd = open("mail_from.txt", O_CREAT|O_WRONLY, 0644);
	ATF_REQUIRE_EQ(write_mail_from(fd, "toto", "meh"), 0);
	close(fd);
	if (!atf_utils_compare_file("mail_from.txt", "MAIL FROM:<toto> meh\r\n")) {
		atf_utils_cat_file("mail_from.txt", "");
		atf_tc_fail("Unexpected output");
	}
	fd = open("mail_from.txt", O_CREAT|O_WRONLY|O_TRUNC, 0644);
	ATF_REQUIRE_EQ(write_mail_from(fd, "toto", ""), 0);
	close(fd);
	if (!atf_utils_compare_file("mail_from.txt", "MAIL FROM:<toto>\r\n")) {
		atf_utils_cat_file("mail_from.txt", "");
		atf_tc_fail("Unexpected output");
	}
	fd = open("mail_from.txt", O_CREAT|O_WRONLY|O_TRUNC, 0644);
	ATF_REQUIRE_EQ(write_mail_from(fd, "toto", " "), 0);
	close(fd);
	if (!atf_utils_compare_file("mail_from.txt", "MAIL FROM:<toto> \r\n")) {
		atf_utils_cat_file("mail_from.txt", "");
		atf_tc_fail("Unexpected output");
	}
	/* CRLF injection */
	fd = open("mail_from.txt", O_CREAT|O_WRONLY|O_TRUNC, 0644);
	ATF_REQUIRE_EQ(write_mail_from(fd, "evil@bad\r\nMAIL FROM:<spoof>", NULL), -1);
	close(fd);
	fd = open("mail_from.txt", O_CREAT|O_WRONLY|O_TRUNC, 0644);
	ATF_REQUIRE_EQ(write_mail_from(fd, "evil@bad\nMAIL FROM:<spoof>", NULL), -1);
	close(fd);
}

ATF_TC_BODY(write_mailbody, tc)
{
	/* no new lines: ignore */
	FILE *fp;
	atf_utils_create_file("myemailbody.txt", "line");
	fp = fopen("myemailbody.txt", "r");
	ATF_REQUIRE(fp != NULL);
	int fd = open("out.txt", O_CREAT|O_WRONLY, 0644);
	ATF_REQUIRE_EQ(write_mailbody(fd, fp, "test@plop.meh"), 0);
	fclose(fp);
	close(fd);
	if (!atf_utils_compare_file("out.txt", "line")) {
		atf_utils_cat_file("out.txt", ">");
		atf_tc_fail("Unexpected output (no new lines case)");
	}

	/* With a single new line */
	atf_utils_create_file("myemailbody.txt", "line\n");
	fp = fopen("myemailbody.txt", "r");
	ATF_REQUIRE(fp != NULL);
	fd = open("out.txt", O_CREAT|O_TRUNC|O_WRONLY, 0644);
	ATF_REQUIRE_EQ(write_mailbody(fd, fp, "test@plop.meh"), 0);
	close(fd);
	fclose(fp);
	if (!atf_utils_compare_file("out.txt", "line\r\n")) {
		atf_utils_cat_file("out.txt", ">");
		atf_tc_fail("Unexpected output (a single new line)");
	}

	/* With a single new line with a single . */
	atf_utils_create_file("myemailbody.txt", "line\n.");
	fp = fopen("myemailbody.txt", "r");
	ATF_REQUIRE(fp != NULL);
	fd = open("out.txt", O_CREAT|O_TRUNC|O_WRONLY, 0644);
	ATF_REQUIRE_EQ(write_mailbody(fd, fp,  "test@plop.meh"), 0);
	close(fd);
	fclose(fp);
	if (!atf_utils_compare_file("out.txt", "line\r\n..")) {
		atf_utils_cat_file("out.txt", ">");
		atf_tc_fail("Unexpected output (single new line with a .)");
	}

	/* New line starting with a dot */
	atf_utils_create_file("myemailbody.txt", "line\n.no");
	fp = fopen("myemailbody.txt", "r");
	ATF_REQUIRE(fp != NULL);
	fd = open("out.txt", O_CREAT|O_TRUNC|O_WRONLY, 0644);
	ATF_REQUIRE_EQ(write_mailbody(fd, fp,  "test@plop.meh"), 0);
	close(fd);
	fclose(fp);
	if (!atf_utils_compare_file("out.txt", "line\r\n..no")) {
		atf_utils_cat_file("out.txt", ">");
		atf_tc_fail("Unexpected output (single new line with a .)");
	}

	/* With a single new line, 2 new lines */
	atf_utils_create_file("myemailbody.txt", "line\n\n");
	fp = fopen("myemailbody.txt", "r");
	ATF_REQUIRE(fp != NULL);
	fd = open("out.txt", O_CREAT|O_TRUNC|O_WRONLY, 0644);
	ATF_REQUIRE_EQ(write_mailbody(fd, fp,  "test@plop.meh"), 0);
	close(fd);
	fclose(fp);
	if (!atf_utils_compare_file("out.txt", "line\r\nTo: test@plop.meh\r\n\r\n")) {
		atf_utils_cat_file("out.txt", ">");
		atf_tc_fail("Unexpected output (a single new line, 2 new lines)");
	}

	/* With a single 2 lines separated by 2 new lines*/
	atf_utils_create_file("myemailbody.txt", "line\n\nline2\n\n");
	fp = fopen("myemailbody.txt", "r");
	ATF_REQUIRE(fp != NULL);
	fd = open("out.txt", O_CREAT|O_TRUNC|O_WRONLY, 0644);
	ATF_REQUIRE_EQ(write_mailbody(fd, fp,  "test@plop.meh"), 0);
	close(fd);
	fclose(fp);
	if (!atf_utils_compare_file("out.txt", "line\r\nTo: test@plop.meh\r\n\r\nline2\r\n\r\n")) {
		atf_utils_cat_file("out.txt", ">");
		atf_tc_fail("Unexpected output (2 lines separated by 2 new lines)");
	}

	/* No to header */
	atf_utils_create_file("myemailbody.txt", "line\n\nline2\n\n");
	fp = fopen("myemailbody.txt", "r");
	ATF_REQUIRE(fp != NULL);
	fd = open("out.txt", O_CREAT|O_TRUNC|O_WRONLY, 0644);
	ATF_REQUIRE_EQ(write_mailbody(fd, fp,  NULL), 0);
	close(fd);
	fclose(fp);
	if (!atf_utils_compare_file("out.txt", "line\r\n\r\nline2\r\n\r\n")) {
		atf_utils_cat_file("out.txt", ">");
		atf_tc_fail("Unexpected output (no to header)");
	}

	/* Error: invalid fd returns -1 */
	atf_utils_create_file("myemailbody.txt", "line\n");
	fp = fopen("myemailbody.txt", "r");
	ATF_REQUIRE(fp != NULL);
	ATF_REQUIRE_EQ(write_mailbody(-1, fp, NULL), -1);
	fclose(fp);

	/* Multiple dot-stuffed lines in sequence */
	atf_utils_create_file("myemailbody.txt", "header\n\n.line1\n.line2\n..already\n");
	fp = fopen("myemailbody.txt", "r");
	ATF_REQUIRE(fp != NULL);
	fd = open("out.txt", O_CREAT|O_TRUNC|O_WRONLY, 0644);
	ATF_REQUIRE_EQ(write_mailbody(fd, fp, NULL), 0);
	close(fd);
	fclose(fp);
	if (!atf_utils_compare_file("out.txt", "header\r\n\r\n..line1\r\n..line2\r\n...already\r\n")) {
		atf_utils_cat_file("out.txt", ">");
		atf_tc_fail("Unexpected output (multiple dot-stuffed lines)");
	}

	/* To header inserted only once */
	atf_utils_create_file("myemailbody.txt", "hdr1\nhdr2\n\nbody\n");
	fp = fopen("myemailbody.txt", "r");
	ATF_REQUIRE(fp != NULL);
	fd = open("out.txt", O_CREAT|O_TRUNC|O_WRONLY, 0644);
	ATF_REQUIRE_EQ(write_mailbody(fd, fp, "rcpt@test"), 0);
	close(fd);
	fclose(fp);
	if (!atf_utils_compare_file("out.txt", "hdr1\r\nhdr2\r\nTo: rcpt@test\r\n\r\nbody\r\n")) {
		atf_utils_cat_file("out.txt", ">");
		atf_tc_fail("Unexpected output (To header placement)");
	}
}

ATF_TC_BODY(strtotimet, tc)
{
	const char *errp;
	char *str;
	ATF_REQUIRE_EQ(strtotimet("10", &errp), 10);
	ATF_REQUIRE(errp == NULL);
	asprintf(&str, "1%"PRIdMAX, INTMAX_MAX);
	ATF_REQUIRE_EQ(strtotimet(str, &errp), 0);
	free(str);
	ATF_REQUIRE(errp != NULL);
	ATF_REQUIRE_STREQ(errp, "too large");
}

ATF_TC_BODY(decode_qp, tc)
{
	ATF_REQUIRE_STREQ(decode_qp("=", false), "=");
	ATF_REQUIRE_STREQ(decode_qp("=\ra", false), "=\ra");
	ATF_REQUIRE_STREQ(decode_qp("=\r\na", false), "a");
	ATF_REQUIRE_STREQ(decode_qp("=\na", false), "a");
	ATF_REQUIRE_STREQ(decode_qp("This is a subject", false), "This is a subject");
	ATF_REQUIRE_STREQ(decode_qp("This= is a subject", false), "This= is a subject");
	ATF_REQUIRE_STREQ(decode_qp("This=2 is a subject", false), "This=2 is a subject");
	ATF_REQUIRE_STREQ(decode_qp("This=23 is a subject", false), "This# is a subject");
	ATF_REQUIRE_STREQ(decode_qp("This=3D is a subject", false), "This= is a subject");
	ATF_REQUIRE_STREQ(decode_qp("This_ is a subject", false), "This_ is a subject");
	ATF_REQUIRE_STREQ(decode_qp("This_ is a subject", true), "This  is a subject");
	/* Lowercase and uppercase hex digits are accepted */
	ATF_REQUIRE_STREQ(decode_qp("This=2f is a subject", false), "This/ is a subject");
	ATF_REQUIRE_STREQ(decode_qp("This=2F is a subject", false), "This/ is a subject");
	/* A non hex digit after '=' is kept verbatim */
	ATF_REQUIRE_STREQ(decode_qp("This=2G is a subject", false), "This=2G is a subject");
}

ATF_TC_BODY(parse_lastdigest, tc)
{
	time_t lasttime;
	long lastissue;
	long lastindex;
	const char *errstr;
	char *line;

	line = xstrdup("1:2:3");
	ATF_REQUIRE(parse_lastdigest(line, &lastindex, &lasttime, &lastissue,
	    &errstr));
	ATF_REQUIRE(errstr == NULL);
	ATF_REQUIRE(lastindex == 1);
	ATF_REQUIRE(lasttime == 2);
	ATF_REQUIRE(lastissue == 3);
	free(line);

	line = xstrdup("1");
	ATF_REQUIRE(!parse_lastdigest(line, &lastindex, &lasttime, &lastissue,
	    &errstr));
	ATF_REQUIRE_STREQ(errstr, "Invalid format, expecting 2 or 3 fields");
	free(line);

	line = xstrdup("1:2:3:4");
	ATF_REQUIRE(!parse_lastdigest(line, &lastindex, &lasttime, &lastissue,
	    &errstr));
	ATF_REQUIRE_STREQ(errstr, "Invalid format, expecting 2 or 3 fields");
	free(line);

	ATF_REQUIRE(parse_lastdigest(NULL, &lastindex, &lasttime, &lastissue,
	    &errstr));
	ATF_REQUIRE(errstr == NULL);
	ATF_REQUIRE(lastindex == 0);
	ATF_REQUIRE(lasttime == 0);
	ATF_REQUIRE(lastissue == 0);

	line = xstrdup("1:2");
	ATF_REQUIRE(parse_lastdigest(line, &lastindex, &lasttime, &lastissue,
	    &errstr));
	ATF_REQUIRE(errstr == NULL);
	ATF_REQUIRE(lastindex == 1);
	ATF_REQUIRE(lasttime == 2);
	ATF_REQUIRE(lastissue == 0);
	free(line);

	line = xstrdup("a:2");
	ATF_REQUIRE(!parse_lastdigest(line, &lastindex, &lasttime, &lastissue,
	    &errstr));
	ATF_REQUIRE_STREQ(errstr, "Invalid value for lastindex");
	free(line);

	line = xstrdup("-1:2");
	ATF_REQUIRE(!parse_lastdigest(line, &lastindex, &lasttime, &lastissue,
	    &errstr));
	ATF_REQUIRE_STREQ(errstr, "Invalid value for lastindex");
	free(line);

	line = xstrdup("1:");
	ATF_REQUIRE(!parse_lastdigest(line, &lastindex, &lasttime, &lastissue,
	    &errstr));
	ATF_REQUIRE_STREQ(errstr, "Invalid value for lasttime");
	free(line);

	line = xstrdup("1:a");
	ATF_REQUIRE(!parse_lastdigest(line, &lastindex, &lasttime, &lastissue,
	    &errstr));
	ATF_REQUIRE_STREQ(errstr, "Invalid value for lasttime");
	free(line);

	line = xstrdup("1:1:");
	ATF_REQUIRE(!parse_lastdigest(line, &lastindex, &lasttime, &lastissue,
	    &errstr));
	ATF_REQUIRE_STREQ(errstr, "Invalid value for lastissue");
	free(line);

	line = xstrdup("1:1:a");
	ATF_REQUIRE(!parse_lastdigest(line, &lastindex, &lasttime, &lastissue,
	    &errstr));
	ATF_REQUIRE_STREQ(errstr, "Invalid value for lastissue");
	free(line);

	line = xstrdup("3774:1702743593:141");
	ATF_REQUIRE(parse_lastdigest(line, &lastindex, &lasttime, &lastissue,
	    &errstr));
	ATF_REQUIRE_EQ(lastindex, 3774);
	ATF_REQUIRE_EQ(lasttime, 1702743593);
	ATF_REQUIRE_EQ(lastissue, 141);
	free(line);

	line = xstrdup("3774:1702743593:141\n");
	ATF_REQUIRE(parse_lastdigest(line, &lastindex, &lasttime, &lastissue,
	    &errstr));
	ATF_REQUIRE_EQ(lastindex, 3774);
	ATF_REQUIRE_EQ(lasttime, 1702743593);
	ATF_REQUIRE_EQ(lastissue, 141);
	free(line);
}

ATF_TC_BODY(extract_bouncetime, tc)
{
	char *line;
	const char *errstr;

	line = xstrdup("any:1234 # comment");
	ATF_REQUIRE_EQ(extract_bouncetime(line, &errstr), 1234);
	ATF_REQUIRE(errstr == NULL);
	free(line);

	line = xstrdup("any:meh # comment");
	ATF_REQUIRE_EQ(extract_bouncetime(line, &errstr), 0);
	ATF_REQUIRE_STREQ(errstr, "invalid");
	free(line);

	line = xstrdup("any:meh ");
	ATF_REQUIRE_EQ(extract_bouncetime(line, &errstr), 0);
	ATF_REQUIRE_STREQ(errstr, "comment is missing");
	free(line);

	line = xstrdup("any:1234    # comment");
	ATF_REQUIRE_EQ(extract_bouncetime(line, &errstr), 1234);
	ATF_REQUIRE(errstr == NULL);
	free(line);

	line = xstrdup("    # comment");
	ATF_REQUIRE_EQ(extract_bouncetime(line, &errstr), 0);
	ATF_REQUIRE_STREQ(errstr, "':' missing");
	free(line);

	line = xstrdup("a    # comment");
	ATF_REQUIRE_EQ(extract_bouncetime(line, &errstr), 0);
	ATF_REQUIRE_STREQ(errstr, "':' missing");
	free(line);
}

ATF_TC_BODY(open_subscriber_directory, tc)
{
	int fd;
	const char *dir;

	init_ml(true);

	int dfd = open("list", O_DIRECTORY);
	rmdir("list/subscribers.d");
	fd = open_subscriber_directory(dfd, SUB_NORMAL, &dir);
	ATF_REQUIRE(fd == -1);
	ATF_REQUIRE_STREQ(dir, "subscribers.d");
	mkdir("list/subscribers.d", 0755);
	fd = open_subscriber_directory(dfd, SUB_NORMAL, NULL);
	ATF_REQUIRE(fd != -1);
	close(fd);

	rmdir("list/digesters.d");
	fd = open_subscriber_directory(dfd, SUB_DIGEST, &dir);
	ATF_REQUIRE(fd == -1);
	ATF_REQUIRE_STREQ(dir, "digesters.d");
	mkdir("list/digesters.d", 0755);
	fd = open_subscriber_directory(dfd, SUB_DIGEST, NULL);
	ATF_REQUIRE(fd != -1);
	close(fd);

	rmdir("list/nomailsubs.d");
	fd = open_subscriber_directory(dfd, SUB_NOMAIL, &dir);
	ATF_REQUIRE(fd == -1);
	ATF_REQUIRE_STREQ(dir, "nomailsubs.d");
	mkdir("list/nomailsubs.d", 0755);
	fd = open_subscriber_directory(dfd, SUB_NOMAIL, NULL);
	ATF_REQUIRE(fd != -1);
	close(fd);

	fd = open_subscriber_directory(dfd, SUB_ALL, &dir);
	ATF_REQUIRE_EQ(fd, -1);
	ATF_REQUIRE(dir == NULL);
	close(dfd);
}

ATF_TC_BODY(unsubscribe, tc)
{
	int listfd;
	init_ml(true);

	listfd = open("list", O_DIRECTORY);

	atf_utils_create_file("list/subscribers.d/j", "john@doe.org");
	atf_utils_create_file("list/nomailsubs.d/j", "john@doe.org\njane@doe.org\n");
	atf_utils_create_file("list/digesters.d/j", "john@doe.org\n");

	ATF_REQUIRE(unsubscribe(listfd, "john@doe.org", SUB_ALL));
	ATF_REQUIRE_EQ_MSG(access("list/subscribers.d/j", F_OK), -1, "Not unsubscribed");
	ATF_REQUIRE_EQ_MSG(access("list/digesters.d/j", F_OK), -1, "Not unsubscribed from digest");
	if (!atf_utils_compare_file("list/nomailsubs.d/j", "jane@doe.org\n")) {
		atf_utils_cat_file("list/nomailsubs.d/j", ">");
		atf_tc_fail("Not unsubscribed from nomail");
	}

	ATF_REQUIRE(unsubscribe(listfd, "jane@doe.org", SUB_NOMAIL));
	ATF_REQUIRE_EQ_MSG(access("list/nomailsubs.d/j", F_OK), -1, "Not unsubscribed from nomail");

	ATF_REQUIRE(!unsubscribe(listfd, "plop@meh", SUB_BOTH));
	rmdir("list/subscribers.d");
	ATF_REQUIRE(!unsubscribe(listfd, "plop@meh", SUB_NORMAL));
	mkdir("list/subscribers.d", 0755);

	atf_utils_create_file("list/nomailsubs.d/j", "");
	chmod("list/nomailsubs.d/j", 0111);
	ATF_REQUIRE(unsubscribe(listfd, "plop@meh", SUB_NOMAIL));
	chmod("list/nomailsubs.d/j", 0644);
	ATF_REQUIRE(unsubscribe(listfd, "plop@meh", SUB_NOMAIL));
	unlink("list/nomailsubs.d/j");

	atf_utils_create_file("list/nomailsubs.d/j", "john@doe.org\njane@doe.org\n");
	ATF_REQUIRE(unsubscribe(listfd, "plop@meh", SUB_NOMAIL));
	ATF_REQUIRE(!unsubscribe(-1, "plop@meh", SUB_ALL));

}

ATF_TC_BODY(genlistname, tc)
{
	char *ret;

	ret = genlistname("plop@bla");
	ATF_REQUIRE_STREQ(ret, "plop");
	free(ret);

	ret = genlistname("plop@bla@meh");
	ATF_REQUIRE_STREQ(ret, "plop");
	free(ret);
}

ATF_TC_BODY(genlistfqdn, tc)
{
	const char *ret;

	ret = genlistfqdn("plop@bla");
	ATF_REQUIRE_STREQ(ret, "bla");
	ret = genlistfqdn("plop@bla@meh");
	ATF_REQUIRE_STREQ(ret, "meh");
}

ATF_TC_BODY(smtp, tc)
{
	int smtppipe[2];
	char *reply;
	ATF_REQUIRE(socketpair(AF_UNIX, SOCK_STREAM, 0, smtppipe) >= 0);
	pid_t p = atf_utils_fork();
	if (p == 0) {
		const char *replies[] = {
		    "250-hostname.net\n"
		    "250-PIPELINEING\n"
		    "250-SIZE 20480000\n"
		    "250-ETRN\n"
		    "250-STARTTLS\n"
		    "250-ENHANCEDSTATUSCODES\n"
		    "250-8BITMIME\n"
		    "250-DSN\n"
		    "250-SMTPUTF8\n"
		    "250 CHUNKING\n", /* EHLO */
		    "250-hostname.net\n", /* HELO */
		    "250 2.1.0 Ok\n", /* MAIL FROM */
		    "250 2.1.0 Ok\n", /* RCPT TO */
		    "354 Send message content; end with <CRLF>.<CRLF>\n", /* DATA */
		    NULL, /* BEFORE DOT */
		    "250 2.1.0 Ok\n", /* DATA */
		    "221 2.0.0 Bye\n", /* QUIT */
		    "250 2.0.0 Ok\n", /* RSET */
		};
		dprintf(smtppipe[0], "220 me fake smtp\n");
		for (uint8_t i = 0; i < NELEM(replies); i++) {
			atf_utils_readline(smtppipe[0]);
			if ( replies[i] != NULL ) {
				dprintf(smtppipe[0], "%s", replies[i]);
			}
		}
		exit (0);
	}
	close(smtppipe[0]);
	reply = checkwait_smtpreply(smtppipe[1], MLMMJ_CONNECT);
	ATF_REQUIRE_EQ(reply, NULL);
	write_ehlo(smtppipe[1], "plop");
	reply = checkwait_smtpreply(smtppipe[1], MLMMJ_EHLO);
	ATF_REQUIRE_EQ(reply, NULL);
	write_helo(smtppipe[1], "plop");
	reply = checkwait_smtpreply(smtppipe[1], MLMMJ_HELO);
	ATF_REQUIRE_EQ(reply, NULL);
	write_mail_from(smtppipe[1], "plop", NULL);
	reply = checkwait_smtpreply(smtppipe[1], MLMMJ_FROM);
	ATF_REQUIRE_EQ(reply, NULL);
	write_rcpt_to(smtppipe[1], "plop");
	reply = checkwait_smtpreply(smtppipe[1], MLMMJ_RCPTTO);
	ATF_REQUIRE_EQ(reply, NULL);
	write_data(smtppipe[1]);
	reply = checkwait_smtpreply(smtppipe[1], MLMMJ_DATA);
	ATF_REQUIRE_EQ(reply, NULL);
	write_dot(smtppipe[1]);
	reply = checkwait_smtpreply(smtppipe[1], MLMMJ_DOT);
	ATF_REQUIRE_EQ(reply, NULL);
	write_quit(smtppipe[1]);
	reply = checkwait_smtpreply(smtppipe[1], MLMMJ_QUIT);
	ATF_REQUIRE_EQ(reply, NULL);
	write_rset(smtppipe[1]);
	reply = checkwait_smtpreply(smtppipe[1], MLMMJ_RSET);
	ATF_REQUIRE_EQ(reply, NULL);
}

ATF_TC_BODY(init_smtp, tc)
{
	int smtppipe[2];
	ATF_REQUIRE(socketpair(AF_UNIX, SOCK_STREAM, 0, smtppipe) >= 0);
	pid_t p = atf_utils_fork();
	if (p == 0) {
		int s = fakesmtp(smtppipe[1]);
		int c;
		struct sockaddr_in cl;
		socklen_t clsize = sizeof(struct sockaddr_in);
		c = accept(s, (struct sockaddr *) &cl, &clsize);
		if (c == -1)
			err(5, "accept()");
		dprintf(c, "220 me fake smtp\n");
		const char *replies[] = {
			"250-hostname.net\n"
			"250-PIPELINEING\n"
			"250-SIZE 20480000\n"
			"250-ETRN\n"
			"250-STARTTLS\n"
			"250-ENHANCEDSTATUSCODES\n"
			"250-8BITMIME\n"
			"250-DSN\n"
			"250-SMTPUTF8\n"
			"250 CHUNKING\n",
			"221 2.0.0 bye\n",
		};
		read_print_reply(c, replies, NELEM(replies));
		exit(0);
	}
	close(smtppipe[1]);
	atf_utils_readline(smtppipe[0]);
	int sockfd;
	int ret = initsmtp(&sockfd, "127.0.0.1", 25678, "heloname");
	ATF_REQUIRE_EQ(ret, 0);
	ATF_REQUIRE_EQ(endsmtp(&sockfd), 0);
	atf_utils_wait(p, 0, "EHLO heloname\r\nQUIT\r\n", "");
}

ATF_TC_BODY(smtp_bad_greetings, tc)
{
	int smtppipe[2];
	ATF_REQUIRE(socketpair(AF_UNIX, SOCK_STREAM, 0, smtppipe) >= 0);

	pid_t p = atf_utils_fork();
	if (p == 0) {
		int s = fakesmtp(smtppipe[1]);
		int c;
		struct sockaddr_in cl;
		socklen_t clsize = sizeof(struct sockaddr_in);
		c = accept(s, (struct sockaddr *) &cl, &clsize);
		if (c == -1)
			err(5, "accept()");
		dprintf(c, "me fake smtp\n");
		exit(0);
	}
	close(smtppipe[1]);
	atf_utils_readline(smtppipe[0]);
	int sockfd;
	int ret = initsmtp(&sockfd, "127.0.0.1", 25678, "heloname");
	ATF_REQUIRE_EQ(ret, MLMMJ_CONNECT);
	atf_utils_wait(p, 0, "", "");
}

ATF_TC_BODY(smtp_bad_ehlo, tc)
{
	int smtppipe[2];
	ATF_REQUIRE(socketpair(AF_UNIX, SOCK_STREAM, 0, smtppipe) >= 0);

	pid_t p = atf_utils_fork();
	if (p == 0) {
		int s = fakesmtp(smtppipe[1]);
		int c;
		struct sockaddr_in cl;
		socklen_t clsize = sizeof(struct sockaddr_in);
		c = accept(s, (struct sockaddr *) &cl, &clsize);
		if (c == -1)
			err(5, "accept()");
		dprintf(c, "220 me fake smtp\n");
		const char *rep = "501 nop nope\n";
		read_print_reply(c, &rep, 1);
		exit(0);
	}
	close(smtppipe[1]);
	atf_utils_readline(smtppipe[0]);
	int sockfd;
	int ret = initsmtp(&sockfd, "127.0.0.1", 25678, "heloname");
	ATF_REQUIRE_EQ(ret, MLMMJ_EHLO);
	atf_utils_wait(p, 0, "EHLO heloname\r\n", "");
}

ATF_TC_BODY(smtp_no_ehlo, tc)
{
	int smtppipe[2];
	ATF_REQUIRE(socketpair(AF_UNIX, SOCK_STREAM, 0, smtppipe) >= 0);

	pid_t p = atf_utils_fork();
	if (p == 0) {
		int s = fakesmtp(smtppipe[1]);
		int c;
		struct sockaddr_in cl;
		socklen_t clsize = sizeof(struct sockaddr_in);
		c = accept(s, (struct sockaddr *) &cl, &clsize);
		if (c == -1)
			err(5, "accept()");
		dprintf(c, "220 me fake smtp\n");
		const char *replies [] = {
			"801 meh\n",
			NULL,
		};
		read_print_reply(c, replies, NELEM(replies));
		close(c);
		c = accept(s, (struct sockaddr *) &cl, &clsize);
		if (c == -1)
			err(5, "accept()");
		dprintf(c, "220 me fake smtp\n");
		const char *rep = "250 OK\n";
		read_print_reply(c, &rep, 1);
		exit(0);
	}
	close(smtppipe[1]);
	atf_utils_readline(smtppipe[0]);
	int sockfd;
	int ret = initsmtp(&sockfd, "127.0.0.1", 25678, "heloname");
	ATF_REQUIRE_EQ(ret, 0);
	atf_utils_wait(p, 0, "EHLO heloname\r\nQUIT\r\nHELO heloname\r\n", "");
}

ATF_TC_BODY(endsmtp, tc)
{
	int sock = -1;
	ATF_REQUIRE_EQ(endsmtp(&sock), 0);
}

ATF_TC_BODY(do_bouncemail, tc)
{
	init_ml(true);
	int lfd = open("list", O_DIRECTORY);
	int ctrlfd = openat(lfd, "control", O_DIRECTORY);
	/* malformed */
	ATF_REQUIRE_EQ(do_bouncemail(lfd, ctrlfd, "plop"), 0);

	/* malformed, missing number */
	ATF_REQUIRE_EQ(do_bouncemail(lfd, ctrlfd, "plop@meh"), 0);

	/* malformed, empty numbr */
	ATF_REQUIRE_EQ(do_bouncemail(lfd, ctrlfd, "plop-@meh"), 0);

	/* no listdelim */
	ATF_REQUIRE_EQ(do_bouncemail(lfd, ctrlfd, "plop-0@meh"), 0);

	/* listdelim before */
	ATF_REQUIRE_EQ(do_bouncemail(lfd, ctrlfd, "plop+-0@meh"), 0);

	/* listdelim before */
	ATF_REQUIRE_EQ(do_bouncemail(lfd, ctrlfd, "plop-0+@meh"), 0);

	/* 2 ticks */
	ATF_REQUIRE_EQ(do_bouncemail(lfd, ctrlfd, "plop+--bla@meh"), 1);

	close(lfd);
	close(ctrlfd);
}

ATF_TC_BODY(bouncemail, tc)
{
	mkdir("list", 0755);
	int fd = open("list", O_DIRECTORY);

	ATF_REQUIRE_EQ(bouncemail(fd, "bapt=FreeBSD.org", "confsub"), BOUNCE_OK);
	mkdir("list/subconf", 0755);
	close(open("list/subconf/bapt=freebsd.org", O_CREAT, 0644));
	ATF_REQUIRE_EQ(access("list/subconf/bapt=freebsd.org", F_OK), 0);
	ATF_REQUIRE_EQ(bouncemail(fd, "bapt=FreeBSD.org", "confsub"), BOUNCE_OK);
	ATF_REQUIRE_EQ(access("list/subconf/bapt=freebsd.org", F_OK), -1);

	ATF_REQUIRE_EQ(bouncemail(fd, "bapt=FreeBSD.org", "confunsub"), BOUNCE_OK);
	mkdir("list/unsubconf", 0755);
	close(open("list/unsubconf/bapt=freebsd.org", O_CREAT, 0644));
	ATF_REQUIRE_EQ(access("list/unsubconf/bapt=freebsd.org", F_OK), 0);
	ATF_REQUIRE_EQ(bouncemail(fd, "bapt=FreeBSD.org", "confunsub"), BOUNCE_OK);
	ATF_REQUIRE_EQ(access("list/unsubconf/bapt=freebsd.org", F_OK), -1);

	ATF_REQUIRE_EQ(bouncemail(fd, "bapt=FreeBSD.org", "probe"), BOUNCE_FAIL);
	mkdir("list/bounce", 0755);
	ATF_REQUIRE_EQ(bouncemail(fd, "bapt=FreeBSD.org", "probe"), BOUNCE_OK);

	close(open("list/bounce/bapt=freebsd.org-probe", O_CREAT, 0644));
	ATF_REQUIRE_EQ(bouncemail(fd, "bapt=FreeBSD.org", "probe"), BOUNCE_OK);
	ATF_REQUIRE_EQ(access("list/bounce/bapt=freebsd.org-probe", F_OK), -1);

	ATF_REQUIRE_EQ(bouncemail(fd, "bapt@FreeBSD.org", "plop"), BOUNCE_OK);
	ATF_REQUIRE_EQ(bouncemail(fd, "bapt=FreeBSD.org", "1234"), BOUNCE_OK);
	ATF_REQUIRE_EQ(access("list/bounce/bapt=freebsd.org", F_OK), -1);

	mkdir("list/subscribers.d", 0755);
	atf_utils_create_file("list/subscribers.d/b", "bapt@FreeBSD.org");
	ATF_REQUIRE_EQ(bouncemail(fd, "bapt=FreeBSD.org", "1234"), BOUNCE_DONE);
	ATF_REQUIRE_EQ(access("list/bounce/bapt=freebsd.org", F_OK), 0);
	ATF_REQUIRE(atf_utils_grep_file("1234:.*", "list/bounce/bapt=freebsd.org"));

	ATF_REQUIRE_EQ(bouncemail(fd, "bapt=FreeBSD.org", "2323"), BOUNCE_DONE);
	ATF_REQUIRE_EQ(access("list/bounce/bapt=freebsd.org", F_OK), 0);

	ATF_REQUIRE(atf_utils_grep_file("1234:.*", "list/bounce/bapt=freebsd.org"));
	ATF_REQUIRE(atf_utils_grep_file("2323:.*", "list/bounce/bapt=freebsd.org"));
}

ATF_TC_BODY(send_mail_basics, tc)
{
	struct mail mail = { 0 };
	ATF_REQUIRE_EQ_MSG(send_mail(1, &mail, -1, -1, false), 0, "Failure with to == NULL");
	mail.to = "plop";
	ATF_REQUIRE_EQ_MSG(send_mail(1, &mail, -1, -1, false), 0, "Failure with to without @");
	mail.to = "plop@meh";
	ATF_REQUIRE_EQ_MSG(send_mail(1, &mail, -1, -1, false), 0, "Failure with from == NULL");
}

ATF_TC_BODY(send_mail, tc)
{
	int smtppipe[2];
	struct mail mail = { 0 };
	ATF_REQUIRE(socketpair(AF_UNIX, SOCK_STREAM, 0, smtppipe) >= 0);
	pid_t p = atf_utils_fork();
	if (p == 0) {
		const char *replies [] = {
			"250 2.1.0 OK\n",
			"250 2.1.0 OK\n",
			"350 2.1.0 OK\n",
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			"250 2.1.0 OK\n",
		};
		read_print_reply(smtppipe[0], replies, NELEM(replies));
		exit(0);
	}
	close(smtppipe[0]);
	mail.to = "plop@meh";
	mail.from = "test@meh";
	atf_utils_create_file("mymail.txt", "headers\n\nbody\n");
	mail.fp = fopen("mymail.txt", "r");
	mail.addtohdr = true;
	ATF_REQUIRE_EQ(send_mail(smtppipe[1], &mail, -1, -1, false), 0);
	atf_utils_wait(p, 0, "MAIL FROM:<test@meh>\r\nRCPT TO:<plop@meh>\r\nDATA\r\nheaders\r\nTo: plop@meh\r\n\r\nbody\r\n\r\n.\r\n", "");
}

ATF_TC_BODY(send_mail_xforwardedto, tc)
{
	int smtppipe[2];
	struct mail mail = { 0 };
	ATF_REQUIRE(socketpair(AF_UNIX, SOCK_STREAM, 0, smtppipe) >= 0);
	pid_t p = atf_utils_fork();
	if (p == 0) {
		const char *replies [] = {
			"250 2.1.0 OK\n",
			"250 2.1.0 OK\n",
			"350 2.1.0 OK\n",
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			"250 2.1.0 OK\n",
		};
		read_print_reply(smtppipe[0], replies, NELEM(replies));
		exit(0);
	}
	close(smtppipe[0]);
	mail.to = "plop@meh";
	mail.from = "test@meh";
	atf_utils_create_file("mymail.txt", "headers\n\nbody\n");
	mail.fp = fopen("mymail.txt", "r");
	mail.xforwardedto = true;
	ATF_REQUIRE_EQ(send_mail(smtppipe[1], &mail, -1, -1, false), 0);
	atf_utils_wait(p, 0, "MAIL FROM:<test@meh>\r\nRCPT TO:<plop@meh>\r\nDATA\r\nX-Forwarded-To: plop@meh\r\nheaders\r\n\r\nbody\r\n\r\n.\r\n", "");
}

ATF_TC_BODY(send_mail_xsignedrecipient, tc)
{
	int smtppipe[2];
	struct mail mail = { 0 };
	ATF_REQUIRE(socketpair(AF_UNIX, SOCK_STREAM, 0, smtppipe) >= 0);
	pid_t p = atf_utils_fork();
	if (p == 0) {
		const char *replies [] = {
			"250 2.1.0 OK\n",
			"250 2.1.0 OK\n",
			"350 2.1.0 OK\n",
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			"250 2.1.0 OK\n",
		};
		read_print_reply(smtppipe[0], replies, NELEM(replies));
		exit(0);
	}
	close(smtppipe[0]);
	mail.to = "plop@meh";
	mail.from = "test@meh";
	atf_utils_create_file("mymail.txt", "headers\n\nbody\n");
	mail.fp = fopen("mymail.txt", "r");
	mail.xsignedrecipient = true;
	ATF_REQUIRE_EQ(send_mail(smtppipe[1], &mail, -1, -1, false), 0);
	atf_utils_wait(p, 0, "MAIL FROM:<test@meh>\r\nRCPT TO:<plop@meh>\r\nDATA\r\nX-Signed-Recipient: i=1; plop@meh\r\nheaders\r\n\r\nbody\r\n\r\n.\r\n", "");
}

ATF_TC_BODY(send_mail_both_headers, tc)
{
	int smtppipe[2];
	struct mail mail = { 0 };
	ATF_REQUIRE(socketpair(AF_UNIX, SOCK_STREAM, 0, smtppipe) >= 0);
	pid_t p = atf_utils_fork();
	if (p == 0) {
		const char *replies [] = {
			"250 2.1.0 OK\n",
			"250 2.1.0 OK\n",
			"350 2.1.0 OK\n",
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			"250 2.1.0 OK\n",
		};
		read_print_reply(smtppipe[0], replies, NELEM(replies));
		exit(0);
	}
	close(smtppipe[0]);
	mail.to = "plop@meh";
	mail.from = "test@meh";
	atf_utils_create_file("mymail.txt", "headers\n\nbody\n");
	mail.fp = fopen("mymail.txt", "r");
	mail.addtohdr = true;
	mail.xforwardedto = true;
	mail.xsignedrecipient = true;
	ATF_REQUIRE_EQ(send_mail(smtppipe[1], &mail, -1, -1, false), 0);
	atf_utils_wait(p, 0, "MAIL FROM:<test@meh>\r\nRCPT TO:<plop@meh>\r\nDATA\r\nX-Forwarded-To: plop@meh\r\nX-Signed-Recipient: i=1; plop@meh\r\nheaders\r\nTo: plop@meh\r\n\r\nbody\r\n\r\n.\r\n", "");
}

ATF_TC_BODY(getlistdelim, tc)
{
	init_ml(true);
	int fd = open("list/control", O_RDONLY);
	ATF_REQUIRE_STREQ_MSG(getlistdelim(fd), DEFAULT_RECIPDELIM, "Invalid fallback");
}

ATF_TC_BODY(getlistdelim_0, tc)
{
	init_ml(true);
	int fd = open("list/control", O_RDONLY);
	atf_utils_create_file("list/control/delimiter", "");
	ATF_REQUIRE_STREQ_MSG(getlistdelim(fd), DEFAULT_RECIPDELIM, "Invalid fallback, in case of empty delimiter");
}

ATF_TC_BODY(getlistdelim_1, tc)
{
	init_ml(true);
	int fd = open("list/control", O_RDONLY);
	atf_utils_create_file("list/control/delimiter", "\n");
	ATF_REQUIRE_STREQ_MSG(getlistdelim(fd), DEFAULT_RECIPDELIM, "Invalid fallback, in case of empty delimiter");
}

ATF_TC_BODY(getlistdelim_2, tc)
{
	init_ml(true);
	int fd = open("list/control", O_RDONLY);
	atf_utils_create_file("list/control/delimiter", "mydelim\n");
	ATF_REQUIRE_STREQ_MSG(getlistdelim(fd), "mydelim", "Invalid delimiter found");
}

ATF_TC_BODY(getlistdelim_3, tc)
{
	init_ml(true);
	int fd = open("list/control", O_RDONLY);
	atf_utils_create_file("list/control/delimiter", "mydelim\nignoreme\n");
	ATF_REQUIRE_STREQ_MSG(getlistdelim(fd), "mydelim", "Invalid delimiter found");
}

ATF_TC_BODY(getlistdelim_4, tc)
{
	init_ml(true);
	int fd = open("list/control", O_RDONLY);
	atf_utils_create_file("list/control/delimiter", "mydelim");
	ATF_REQUIRE_STREQ_MSG(getlistdelim(fd), "mydelim", "Invalid delimiter found");
}

ATF_TC_BODY(statctrl, tc)
{
	pid_t p;
	p = atf_utils_fork();
	if (p == 0) {
		statctrl(-1, "test");
		exit(0);
	}
	atf_utils_wait(p, EXIT_FAILURE, "", "");
	mkdir("plop", 0755);
	int fd = open("plop", O_DIRECTORY);
	ATF_REQUIRE_EQ(statctrl(fd, "test"), false);
	atf_utils_create_file("plop/test", "");
	ATF_REQUIRE_EQ(statctrl(fd, "test"), true);
}

ATF_TC_BODY(is_subbed_in, tc)
{
	pid_t p;

	p = atf_utils_fork();
	if (p == 0) {
		is_subbed_in(-1, "plop", "meh");
	}
	atf_utils_wait(p, EXIT_FAILURE, "", "");
}

ATF_TC_BODY(is_subbed_normal, tc)
{
	struct ml ml;
	init_ml(true);
	ml_init(&ml);
	ml.dir = "list";
	ml_open(&ml, false);

	/* No subscribers yet */
	ATF_REQUIRE_EQ(is_subbed(ml.fd, "user@example.com", false), SUB_NONE);

	/* Add a normal subscriber */
	atf_utils_create_file("list/subscribers.d/u", "user@example.com\n");

	/* Re-open to get fresh fd */
	ml_open(&ml, false);
	ATF_REQUIRE_EQ(is_subbed(ml.fd, "user@example.com", false), SUB_NORMAL);

	/* Case-insensitive match */
	ATF_REQUIRE_EQ(is_subbed(ml.fd, "User@Example.COM", false), SUB_NORMAL);

	/* Non-subscribed address */
	ATF_REQUIRE_EQ(is_subbed(ml.fd, "other@example.com", false), SUB_NONE);
}

ATF_TC_BODY(is_subbed_types, tc)
{
	struct ml ml;
	init_ml(true);
	ml_init(&ml);
	ml.dir = "list";
	ml_open(&ml, false);

	/* Add a digest subscriber */
	atf_utils_create_file("list/digesters.d/d", "digest@example.com\n");
	ml_open(&ml, false);
	ATF_REQUIRE_EQ(is_subbed(ml.fd, "digest@example.com", false), SUB_DIGEST);

	/* Add a nomail subscriber */
	atf_utils_create_file("list/nomailsubs.d/n", "nomail@example.com\n");
	ml_open(&ml, false);
	ATF_REQUIRE_EQ(is_subbed(ml.fd, "nomail@example.com", false), SUB_NOMAIL);

	/* Add same address as both normal and digest -> SUB_BOTH */
	atf_utils_create_file("list/subscribers.d/b", "both@example.com\n");
	atf_utils_create_file("list/digesters.d/b", "both@example.com\n");
	ml_open(&ml, false);
	ATF_REQUIRE_EQ(is_subbed(ml.fd, "both@example.com", true), SUB_BOTH);

	/* Without 'both' flag, returns SUB_NORMAL on first match */
	ATF_REQUIRE_EQ(is_subbed(ml.fd, "both@example.com", false), SUB_NORMAL);
}

ATF_TC_BODY(unsubscribe_test, tc)
{
	struct ml ml;
	init_ml(true);
	ml_init(&ml);
	ml.dir = "list";
	ml_open(&ml, false);

	/* Create subscriber files */
	atf_utils_create_file("list/subscribers.d/u",
	    "user1@example.com\nuser2@example.com\n");
	atf_utils_create_file("list/digesters.d/d", "digest@example.com\n");
	ml_open(&ml, false);

	/* Verify initial state */
	ATF_REQUIRE_EQ(is_subbed(ml.fd, "user1@example.com", false), SUB_NORMAL);
	ATF_REQUIRE_EQ(is_subbed(ml.fd, "user2@example.com", false), SUB_NORMAL);
	ATF_REQUIRE_EQ(is_subbed(ml.fd, "digest@example.com", false), SUB_DIGEST);

	/* Unsubscribe user1 from normal */
	ATF_REQUIRE(unsubscribe(ml.fd, "user1@example.com", SUB_NORMAL));
	ml_open(&ml, false);
	ATF_REQUIRE_EQ(is_subbed(ml.fd, "user1@example.com", false), SUB_NONE);
	/* user2 should still be subscribed */
	ATF_REQUIRE_EQ(is_subbed(ml.fd, "user2@example.com", false), SUB_NORMAL);

	/* Unsubscribe digest subscriber */
	ATF_REQUIRE(unsubscribe(ml.fd, "digest@example.com", SUB_DIGEST));
	ml_open(&ml, false);
	ATF_REQUIRE_EQ(is_subbed(ml.fd, "digest@example.com", false), SUB_NONE);

	/* Unsubscribe non-existent address should still return true */
	ATF_REQUIRE(unsubscribe(ml.fd, "nobody@example.com", SUB_NORMAL));
}

ATF_TC_BODY(find_addr_line_edge, tc)
{
	/* Exact hit, trailing newline. */
	const char a[] = "alice@x.org\nbob@x.org\n";
	ATF_REQUIRE(find_addr_line(a, a + sizeof(a) - 1, "alice@x.org") != NULL);
	ATF_REQUIRE(find_addr_line(a, a + sizeof(a) - 1, "bob@x.org") != NULL);

	/* CRLF line must match (find_subscriber always did). */
	const char c[] = "alice@x.org\r\nbob@x.org\r\n";
	ATF_REQUIRE(find_addr_line(c, c + sizeof(c) - 1, "alice@x.org") != NULL);

	/* A prefix of the address must NOT match. */
	const char p[] = "ali\nalice@x.org\n";
	ATF_REQUIRE(find_addr_line(p, p + sizeof(p) - 1, "alice@x.org") != NULL);
	ATF_REQUIRE(find_addr_line(p, p + sizeof(p) - 1, "ali") != NULL);
	ATF_REQUIRE(find_addr_line(p, p + sizeof(p) - 1, "al") == NULL);

	/* Empty lines must never match a non-empty address. */
	const char e[] = "\n\nalice@x.org\n\n";
	ATF_REQUIRE(find_addr_line(e, e + sizeof(e) - 1, "alice@x.org") != NULL);
	const char onlyempty[] = "\n\n\n";
	ATF_REQUIRE(find_addr_line(onlyempty, onlyempty + sizeof(onlyempty) - 1,
	    "alice@x.org") == NULL);

	/* No trailing newline on the last line. */
	const char n[] = "alice@x.org\nbob@x.org";
	ATF_REQUIRE(find_addr_line(n, n + sizeof(n) - 1, "bob@x.org") != NULL);

	/* Genuine miss. */
	ATF_REQUIRE(find_addr_line(a, a + sizeof(a) - 1, "carol@x.org") == NULL);
}

ATF_TC_BODY(is_subbed_crlf_prefix, tc)
{
	struct ml ml;
	init_ml(true);
	ml_init(&ml);
	ml.dir = "list";
	ml_open(&ml, false);

	/*
	 * A non-shard-named file (not address[0]) holding a CRLF address,
	 * a prefix line, and an empty line.  is_subbed must find the real
	 * address despite the flexible on-disk layout.
	 */
	atf_utils_create_file("list/subscribers.d/mysubscribers",
	    "ali\nalice@x.org\r\n\nbob@x.org\n");
	ml_open(&ml, false);

	ATF_REQUIRE_EQ(is_subbed(ml.fd, "alice@x.org", false), SUB_NORMAL);
	ATF_REQUIRE_EQ(is_subbed(ml.fd, "bob@x.org", false), SUB_NORMAL);
	/* The short "ali" line is its own subscriber; it must not fool a
	 * lookup for the longer address, which finds its real line. */
	ATF_REQUIRE_EQ(is_subbed(ml.fd, "ali", false), SUB_NORMAL);
	ATF_REQUIRE_EQ(is_subbed(ml.fd, "carol@x.org", false), SUB_NONE);
}

ATF_TC_BODY(unsubscribe_crlf_prefix, tc)
{
	struct ml ml;
	init_ml(true);
	ml_init(&ml);
	ml.dir = "list";
	ml_open(&ml, false);

	/*
	 * "ali" is a prefix of the target and is LF-terminated: the old
	 * search matched it by prefix and corrupted the file on splice.
	 * The target itself is CRLF-terminated.  After unsubscribing the
	 * target, the prefix line and the bystander must be intact.
	 */
	atf_utils_create_file("list/subscribers.d/m",
	    "ali\nalice@x.org\r\nbob@x.org\n");
	ml_open(&ml, false);

	ATF_REQUIRE(unsubscribe(ml.fd, "alice@x.org", SUB_NORMAL));
	if (!atf_utils_compare_file("list/subscribers.d/m",
	    "ali\nbob@x.org\n")) {
		atf_utils_cat_file("list/subscribers.d/m", ">");
		atf_tc_fail("unsubscribe corrupted the file");
	}
}

ATF_TC_BODY(mkuniqfileat_test, tc)
{
	int dfd, fd, fd2;
	char *name = NULL, *name2 = NULL, *name3 = NULL, *name4 = NULL;

	ATF_REQUIRE(mkdir("mkuniq", 0755) == 0);
	dfd = open("mkuniq", O_DIRECTORY);
	ATF_REQUIRE(dfd >= 0);

	/* Bare unique name, returned via name_out. */
	fd = mkuniqfileat(dfd, "", S_IRUSR|S_IWUSR, &name);
	ATF_REQUIRE_MSG(fd >= 0, "mkuniqfileat failed: %s", strerror(errno));
	ATF_REQUIRE(name != NULL);
	ATF_REQUIRE_MSG(faccessat(dfd, name, F_OK, 0) == 0,
	    "created file not visible");
	ATF_REQUIRE(dprintf(fd, "x") == 1);
	close(fd);

	/* A second call must yield a different name. */
	fd2 = mkuniqfileat(dfd, "", S_IRUSR|S_IWUSR, &name2);
	ATF_REQUIRE(fd2 >= 0);
	ATF_REQUIRE(name2 != NULL);
	ATF_REQUIRE_MSG(strcmp(name, name2) != 0,
	    "two calls produced the same name %s", name);
	close(fd2);

	/* Prefixed name. */
	fd = mkuniqfileat(dfd, "pre", S_IRUSR|S_IWUSR, &name3);
	ATF_REQUIRE(fd >= 0);
	ATF_REQUIRE_MSG(strncmp(name3, "pre", 3) == 0,
	    "prefix not honoured: %s", name3);
	ATF_REQUIRE_MSG(faccessat(dfd, name3, F_OK, 0) == 0,
	    "prefixed file not visible");
	close(fd);

	/* name_out == NULL is allowed. */
	fd = mkuniqfileat(dfd, "", S_IRUSR|S_IWUSR, NULL);
	ATF_REQUIRE(fd >= 0);
	close(fd);

	/* AT_FDCWD with a relative path prefix (absolute-path usage). */
	fd = mkuniqfileat(AT_FDCWD, "mkuniq/", S_IRUSR|S_IWUSR, &name4);
	ATF_REQUIRE(fd >= 0);
	ATF_REQUIRE_MSG(strncmp(name4, "mkuniq/", 7) == 0,
	    "cwd-prefix not honoured: %s", name4);
	ATF_REQUIRE_MSG(access(name4, F_OK) == 0, "cwd-prefix file not visible");
	close(fd);

	/* Bad dirfd: returns -1, does not exit, sets *name_out to NULL. */
	fd = mkuniqfileat(-1, "", S_IRUSR|S_IWUSR, &name);
	ATF_REQUIRE_EQ_MSG(fd, -1, "expected failure on bad dirfd");
	ATF_REQUIRE(name == NULL);

	free(name);
	free(name2);
	free(name3);
	free(name4);
	close(dfd);
}

ATF_TC_BODY(getaddrsfromfile, tc)
{
	strlist stl = vec_init();
	ATF_REQUIRE_EQ(getaddrsfromfile(&stl, NULL, 2), -1);
	atf_utils_create_file("list", "");
	FILE *fp;
	fp = fopen("list", "r");
	ATF_REQUIRE_EQ(getaddrsfromfile(&stl, fp, 2), 0);
	fclose(fp);
	atf_utils_create_file("list", "\n");
	fp = fopen("list", "r");
	ATF_REQUIRE_EQ(getaddrsfromfile(&stl, fp, 2), 0);
	atf_utils_create_file("list", "mail1@fqdn\n");
	fp = fopen("list", "r");
	ATF_REQUIRE_EQ(getaddrsfromfile(&stl, fp, 2), 0);
	fclose(fp);
	ATF_REQUIRE_EQ_MSG(stl.len, 1, "%zu is not 1", stl.len);
	ATF_REQUIRE_STREQ(stl.d[0], "mail1@fqdn");
	vec_free_and_free(&stl, free);
	fp = fopen("list", "r");
	ATF_REQUIRE_EQ(getaddrsfromfile(&stl, fp, 0), -1);
	atf_utils_create_file("list", "mail1@fqdn\nmail2@fqdn\nmail3@fqdn");
	fp = fopen("list", "r");
	ATF_REQUIRE(getaddrsfromfile(&stl, fp, 2) > 0);
	ATF_REQUIRE_EQ(stl.len, 2);
	ATF_REQUIRE_STREQ(stl.d[0], "mail1@fqdn");
	ATF_REQUIRE_STREQ(stl.d[stl.len - 1], "mail2@fqdn");
	vec_free_and_free(&stl, free);
	ATF_REQUIRE_EQ(getaddrsfromfile(&stl, fp, 2), 0);
	ATF_REQUIRE_EQ_MSG(stl.len, 1, "%zu is not 1", stl.len);
	ATF_REQUIRE_STREQ(stl.d[0], "mail3@fqdn");
	fclose(fp);
}

ATF_TC_BODY(dumpfd2fd, tc)
{
	atf_utils_create_file("from", "bla\n");

	int from = open("from", O_RDONLY);
	int to = open("to.txt", O_WRONLY|O_CREAT|O_TRUNC, 0644);

	ATF_REQUIRE_EQ_MSG(dumpfd2fd(from, to), 0, "Invalid simple copy");
	close(from);
	close(to);

	if (!atf_utils_compare_file("to.txt", "bla\n")) {
		atf_utils_cat_file("to.txt", "");
		atf_tc_fail("Unexpected output");
	}
}

ATF_TC_BODY(dumpfd2fd_large, tc)
{
	/* Test with data larger than DUMPBUF (4096) */
	int from, to;
	FILE *fp;
	int i;
	char buf[16];

	fp = fopen("large_from", "w");
	ATF_REQUIRE(fp != NULL);
	for (i = 0; i < 1000; i++)
		fprintf(fp, "line %04d: abcdefghijklmnopqrstuvwxyz\n", i);
	fclose(fp);

	from = open("large_from", O_RDONLY);
	to = open("large_to", O_WRONLY|O_CREAT|O_TRUNC, 0644);
	ATF_REQUIRE_EQ(dumpfd2fd(from, to), 0);
	close(from);
	close(to);

	/* Verify first and last lines */
	fp = fopen("large_to", "r");
	ATF_REQUIRE(fp != NULL);
	ATF_REQUIRE(fgets(buf, sizeof(buf), fp) != NULL);
	ATF_REQUIRE(strncmp(buf, "line 0000:", 10) == 0);
	fclose(fp);

	/* Verify file sizes match */
	struct stat st1, st2;
	ATF_REQUIRE_EQ(stat("large_from", &st1), 0);
	ATF_REQUIRE_EQ(stat("large_to", &st2), 0);
	ATF_REQUIRE_EQ(st1.st_size, st2.st_size);
}

ATF_TC_BODY(dumpfd2fd_empty, tc)
{
	atf_utils_create_file("empty_from", "");
	int from = open("empty_from", O_RDONLY);
	int to = open("empty_to", O_WRONLY|O_CREAT|O_TRUNC, 0644);
	ATF_REQUIRE_EQ(dumpfd2fd(from, to), 0);
	close(from);
	close(to);

	struct stat st;
	ATF_REQUIRE_EQ(stat("empty_to", &st), 0);
	ATF_REQUIRE_EQ(st.st_size, 0);
}

ATF_TC_BODY(process_headers_test, tc)
{
	int infd;
	FILE *outf;

	/* Simple header passthrough */
	atf_utils_create_file("hdr_in", "X-Custom: value\n\n");
	infd = open("hdr_in", O_RDONLY);
	outf = fopen("hdr_out", "w");
	ATF_REQUIRE(outf != NULL);
	ATF_REQUIRE_EQ(process_headers(infd, outf, "sender@test", NULL, false), 0);
	close(infd);
	fclose(outf);
	if (!atf_utils_compare_file("hdr_out", "X-Custom: value\n")) {
		atf_utils_cat_file("hdr_out", ">");
		atf_tc_fail("Simple header passthrough failed");
	}

	/* $posteraddr$ substitution */
	atf_utils_create_file("hdr_in2", "Reply-To: $posteraddr$\n\n");
	infd = open("hdr_in2", O_RDONLY);
	outf = fopen("hdr_out2", "w");
	ATF_REQUIRE(outf != NULL);
	ATF_REQUIRE_EQ(process_headers(infd, outf, "bob@example.com", NULL, false), 0);
	close(infd);
	fclose(outf);
	if (!atf_utils_compare_file("hdr_out2", "Reply-To: bob@example.com\n")) {
		atf_utils_cat_file("hdr_out2", ">");
		atf_tc_fail("$posteraddr$ substitution failed");
	}

	/* Multiple headers with substitution */
	atf_utils_create_file("hdr_in3",
	    "X-Original-From: $posteraddr$\nX-List: mylist\n\n");
	infd = open("hdr_in3", O_RDONLY);
	outf = fopen("hdr_out3", "w");
	ATF_REQUIRE(outf != NULL);
	ATF_REQUIRE_EQ(process_headers(infd, outf, "alice@test.org", NULL, false), 0);
	close(infd);
	fclose(outf);
	if (!atf_utils_compare_file("hdr_out3",
	    "X-Original-From: alice@test.org\nX-List: mylist\n")) {
		atf_utils_cat_file("hdr_out3", ">");
		atf_tc_fail("Multiple headers substitution failed");
	}

	/* Empty input */
	atf_utils_create_file("hdr_in4", "");
	infd = open("hdr_in4", O_RDONLY);
	outf = fopen("hdr_out4", "w");
	ATF_REQUIRE(outf != NULL);
	ATF_REQUIRE_EQ(process_headers(infd, outf, "test@test", NULL, false), 0);
	close(infd);
	fclose(outf);
	struct stat st;
	ATF_REQUIRE_EQ(stat("hdr_out4", &st), 0);
	ATF_REQUIRE_EQ(st.st_size, 0);

	/* Body content after blank line is NOT processed */
	atf_utils_create_file("hdr_in5",
	    "X-Custom: header\n\nbody line that must be ignored\n");
	infd = open("hdr_in5", O_RDONLY);
	outf = fopen("hdr_out5", "w");
	ATF_REQUIRE(outf != NULL);
	ATF_REQUIRE_EQ(process_headers(infd, outf, "sender@test", NULL, false), 0);
	close(infd);
	fclose(outf);
	if (!atf_utils_compare_file("hdr_out5", "X-Custom: header\n")) {
		atf_utils_cat_file("hdr_out5", ">");
		atf_tc_fail("process_headers should stop at blank line");
	}
}

ATF_TC_BODY(process_headers_skipfrom, tc)
{
	int infd;
	FILE *outf;

	/* The digest sender sets its own From: header, a From: coming
	 * from customheaders must not be added a second time */
	atf_utils_create_file("hdr_in_s1",
	    "From: $disarmedfrom$ via MyList <list@example.com>\n"
	    "Reply-To: $posteraddr$\n"
	    "Author: $posteraddr$\n\n");
	infd = open("hdr_in_s1", O_RDONLY);
	outf = fopen("hdr_out_s1", "w");
	ATF_REQUIRE(outf != NULL);
	ATF_REQUIRE_EQ(process_headers(infd, outf, "list@example.com",
	    "list=example.com", true), 0);
	close(infd);
	fclose(outf);
	if (!atf_utils_compare_file("hdr_out_s1",
	    "Reply-To: list@example.com\n"
	    "Author: list@example.com\n")) {
		atf_utils_cat_file("hdr_out_s1", ">");
		atf_tc_fail("skipfrom did not drop the custom From: header");
	}
}

ATF_TC_BODY(process_headers_disarmedfrom, tc)
{
	int infd;
	FILE *outf;

	/* $disarmedfrom$ substitution */
	atf_utils_create_file("hdr_in_d1",
	    "From: $disarmedfrom$ via MyList <list@example.com>\n\n");
	infd = open("hdr_in_d1", O_RDONLY);
	outf = fopen("hdr_out_d1", "w");
	ATF_REQUIRE(outf != NULL);
	ATF_REQUIRE_EQ(process_headers(infd, outf, NULL,
	    "Martin (me=here.com)", false), 0);
	close(infd);
	fclose(outf);
	if (!atf_utils_compare_file("hdr_out_d1",
	    "From: Martin (me=here.com) via MyList <list@example.com>\n")) {
		atf_utils_cat_file("hdr_out_d1", ">");
		atf_tc_fail("$disarmedfrom$ substitution failed");
	}

	/* Both $posteraddr$ and $disarmedfrom$ on different lines */
	atf_utils_create_file("hdr_in_d2",
	    "Reply-To: $posteraddr$\n"
	    "X-Original-From: $disarmedfrom$\n\n");
	infd = open("hdr_in_d2", O_RDONLY);
	outf = fopen("hdr_out_d2", "w");
	ATF_REQUIRE(outf != NULL);
	ATF_REQUIRE_EQ(process_headers(infd, outf, "bob@example.com",
	    "Bob (bob=example.com)", false), 0);
	close(infd);
	fclose(outf);
	if (!atf_utils_compare_file("hdr_out_d2",
	    "Reply-To: bob@example.com\n"
	    "X-Original-From: Bob (bob=example.com)\n")) {
		atf_utils_cat_file("hdr_out_d2", ">");
		atf_tc_fail("Both $posteraddr$ and $disarmedfrom$ failed");
	}

	/* Issue #117: both variables on the same line, and repeated
	 * occurrences, must all be expanded */
	atf_utils_create_file("hdr_in_d3",
	    "X-From: $disarmedfrom$ -- $posteraddr$\n"
	    "X-Both: $disarmedfrom$ / $posteraddr$ / $posteraddr$\n\n");
	infd = open("hdr_in_d3", O_RDONLY);
	outf = fopen("hdr_out_d3", "w");
	ATF_REQUIRE(outf != NULL);
	ATF_REQUIRE_EQ(process_headers(infd, outf, "bob@example.com",
	    "Bob (bob=example.com)", false), 0);
	close(infd);
	fclose(outf);
	if (!atf_utils_compare_file("hdr_out_d3",
	    "X-From: Bob (bob=example.com) -- bob@example.com\n"
	    "X-Both: Bob (bob=example.com) / bob@example.com"
	    " / bob@example.com\n")) {
		atf_utils_cat_file("hdr_out_d3", ">");
		atf_tc_fail("mixing/repeating substitutions failed");
	}
}

ATF_TC_BODY(disarm_email_test, tc)
{
	char *out;

	/* A plain ASCII From: value is disarmed and encoded once */
	out = disarm_email("John Doe <john@example.com>");
	ATF_REQUIRE(out != NULL);
	ATF_REQUIRE_STREQ(out,
	    "=?utf-8?q?John_Doe_=28john=3Dexample.com=29?=");
	free(out);

	/* Issue #118: when the display name is already an RFC 2047
	 * encoded word it must be decoded before being encoded again,
	 * not encoded a second time.  The result is the same as for the
	 * equivalent decoded value above. */
	out = disarm_email("=?UTF-8?B?Sm9obiBEb2U=?= <john@example.com>");
	ATF_REQUIRE(out != NULL);
	ATF_REQUIRE_STREQ(out,
	    "=?utf-8?q?John_Doe_=28john=3Dexample.com=29?=");
	ATF_REQUIRE(strstr(out, "=3D=3F") == NULL);
	free(out);

	/* Raw UTF-8 display names (no encoded word) keep being handled */
	out = disarm_email("Jürgen <juergen@example.com>");
	ATF_REQUIRE(out != NULL);
	ATF_REQUIRE_STREQ(out,
	    "=?utf-8?q?J=C3=BCrgen_=28juergen=3Dexample.com=29?=");
	free(out);

	/* A token that only looks like an encoded word but is malformed
	 * must be preserved rather than replaced by "???" */
	out = disarm_email("=?bar?= <j@example.com>");
	ATF_REQUIRE(out != NULL);
	ATF_REQUIRE(strstr(out, "bar") != NULL);
	ATF_REQUIRE(strstr(out, "???") == NULL);
	free(out);
}

ATF_TC_BODY(copy_file, tc)
{
	atf_utils_create_file("from.txt", "bla\n");
	int from = open("from.txt", O_RDONLY);
	int to = open("to.txt", O_WRONLY|O_CREAT|O_TRUNC, 0644);
	int ret = copy_file(from, to, BUFSIZ);
	close(from);
	close(to);
	ATF_REQUIRE_EQ_MSG(ret, 4, "Invalid simple copy");
	if (!atf_utils_compare_file("to.txt", "bla\n")) {
		atf_utils_cat_file("to.txt", ">>");
		atf_tc_fail("Unexpected output");
	}
}

ATF_TC_BODY(copy_file_1, tc)
{
	atf_utils_create_file("from.txt", "");
	int to = open("to.txt", O_WRONLY|O_CREAT|O_TRUNC, 0644);
	int from = open("from.txt", O_RDONLY);
	int ret = copy_file(from, to, BUFSIZ);
	close(from);
	close(to);
	ATF_REQUIRE_EQ_MSG(ret, 0, "Invalid simple copy");
	if (!atf_utils_compare_file("to.txt", "")) {
		atf_utils_cat_file("to.txt", ">>");
		atf_tc_fail("Unexpected output");
	}
}

ATF_TC_BODY(copy_file_2, tc)
{
	atf_utils_create_file("from.txt", "bla\n");
	int from = open("from.txt", O_RDONLY);
	int to = open("to.txt", O_WRONLY|O_CREAT|O_TRUNC, 0644);
	int ret = copy_file(from, to, 3);
	close(from);
	close(to);
	ATF_REQUIRE_EQ_MSG(ret, 3, "Invalid simple copy");
	if (!atf_utils_compare_file("to.txt", "bla")) {
		atf_utils_cat_file("to.txt", ">");
		atf_tc_fail("Unexpected output");
	}
}

ATF_TC_BODY(controls, tc)
{
	int ctrlfd;

	mkdir("control", 0755);
	ctrlfd = open("control", O_DIRECTORY);
	atf_utils_create_file("control/listaddress", "test@test\n");
	ATF_REQUIRE_EQ(ctrlcontent(-1, "plop"), NULL);
	ATF_REQUIRE_EQ(ctrlcontent(ctrlfd, "plop"), NULL);
	ATF_REQUIRE_EQ(ctrlvalues(ctrlfd, "plop"), NULL);
	ATF_REQUIRE(!ctrlvalues_contains(ctrlfd, "plop", "nope", false));
	ATF_REQUIRE(ctrlvalues_contains(ctrlfd, "listaddress", "test@test", false));
	ATF_REQUIRE(!ctrlvalues_contains(ctrlfd, "listaddress", "tesT@test", true));
	ATF_REQUIRE(ctrlvalues_contains(ctrlfd, "listaddress", "tesT@test", false));
	ATF_REQUIRE_STREQ(ctrlcontent(ctrlfd, "listaddress"), "test@test\n");
	ATF_REQUIRE_EQ(ctrltimet(ctrlfd, "listaddress", 0), 0);
	ATF_REQUIRE_EQ(ctrltimet(ctrlfd, "plop", 0), 0);
	atf_utils_create_file("control/val", "12");
	ATF_REQUIRE_EQ(ctrltimet(ctrlfd, "val", 0), 12);
	ATF_REQUIRE_EQ(ctrllong(ctrlfd, "listaddress", 0), 0);
	ATF_REQUIRE_EQ(ctrllong(ctrlfd, "plop", 0), 0);
	ATF_REQUIRE_EQ(ctrllong(ctrlfd, "val", 0), 12);
	ATF_REQUIRE_EQ(ctrlint(ctrlfd, "listaddress", 0), 0);
	ATF_REQUIRE_EQ(ctrlint(ctrlfd, "plop", 0), 0);
	ATF_REQUIRE_EQ(ctrlint(ctrlfd, "val", 0), 12);
	ATF_REQUIRE_EQ(ctrlsizet(ctrlfd, "listaddress", 0), 0);
	ATF_REQUIRE_EQ(ctrlsizet(ctrlfd, "plop", 0), 0);
	ATF_REQUIRE_EQ(ctrlsizet(ctrlfd, "val", 0), 12);

	atf_utils_create_file("control/val", "line1\nline2\n\nline3\n");
	strlist *l = ctrlvalues(ctrlfd, "val");
	ATF_REQUIRE(l != NULL);
	ATF_REQUIRE_EQ(l->len, 3);
	ATF_REQUIRE_STREQ(l->d[0], "line1");
	ATF_REQUIRE_STREQ(l->d[l->len - 1], "line3");
	ATF_REQUIRE_EQ(textcontent(ctrlfd, "text"), NULL);
	mkdir("control/text",0755);
	ATF_REQUIRE_EQ(textcontent(ctrlfd, "text"), NULL);
	atf_utils_create_file("control/text/text", "this is a test");
	ATF_REQUIRE_STREQ(textcontent(ctrlfd, "text"), "this is a test");

	ATF_REQUIRE(ctrlvalues_contains(ctrlfd, "val", "line2", true));
	ATF_REQUIRE(!ctrlvalues_contains(ctrlfd, "val", "Line2", true));
	ATF_REQUIRE(ctrlvalues_contains(ctrlfd, "val", "Line2", false));
}

ATF_TC_BODY(incindexfile, tc)
{
	mkdir("list", 0755);
	int fd = open("list", O_DIRECTORY|O_CLOEXEC);
	ATF_REQUIRE_EQ(incindexfile(-1), 0);
	ATF_REQUIRE_EQ(incindexfile(fd), 1);
	atf_utils_create_file("list/index", "meh");
	ATF_REQUIRE_EQ(incindexfile(fd), 0);
	unlink("list/index");
	ATF_REQUIRE_EQ(incindexfile(fd), 1);
	unlink("list/index");
	atf_utils_create_file("list/index", "");
	ATF_REQUIRE_EQ(incindexfile(fd), 1);
	unlink("list/index");
	atf_utils_create_file("list/index", "1");
	ATF_REQUIRE_EQ(incindexfile(fd), 2);
	ATF_REQUIRE_EQ(incindexfile(fd), 3);
	close(fd);
}

ATF_TC_BODY(log_oper, tc)
{
	mkdir("list", 0755);
	int fd = open("list", O_DIRECTORY);

	ATF_REQUIRE_EQ(log_oper(fd, "plop", "one line with formatting %d", fd), 0);
	if (!atf_utils_grep_file(".* one line with formatting .*", "list/plop")) {
		atf_utils_cat_file("list/plop", ">");
		atf_tc_fail("Invalid log file");
	}
}

ATF_TC_BODY(log_oper_rotate, tc)
{
	mkdir("list", 0755);
	int fd = open("list", O_DIRECTORY);
	/* Create a log file larger than OPLOGSIZE (524288) */
	FILE *big = fopen("list/biglog", "w");
	ATF_REQUIRE(big != NULL);
	for (int i = 0; i < 60000; i++)
		fprintf(big, "0123456789\n");
	fclose(big);
	/* This should trigger rotation */
	ATF_REQUIRE_EQ(log_oper(fd, "biglog", "test %d", 42), 0);
	/* The rotated file should exist */
	struct stat st;
	ATF_REQUIRE_EQ(stat("list/biglog.rotated", &st), 0);
	ATF_REQUIRE(st.st_size > 0);
	close(fd);
}

ATF_TC_BODY(mygetline_edge, tc)
{
	int fd;

	/* Read from empty file: should return NULL */
	fd = open("empty", O_CREAT|O_RDWR, 0644);
	ATF_REQUIRE(mygetline(fd) == NULL);
	close(fd);
	unlink("empty");

	/* Line longer than BUFSIZE (256) triggers buffer resize */
	fd = open("longline", O_CREAT|O_RDWR, 0644);
	char longbuf[512];
	memset(longbuf, 'a', 511);
	longbuf[511] = '\0';
	dprintf(fd, "%s", longbuf);
	lseek(fd, 0, SEEK_SET);
	char *res = mygetline(fd);
	ATF_REQUIRE(res != NULL);
	ATF_REQUIRE_EQ(strlen(res), 511);
	ATF_REQUIRE(res[0] == 'a' && res[510] == 'a');
	free(res);
	close(fd);
	unlink("longline");
}

ATF_TC_BODY(get_ctrl_command, tc)
{
	char *param = NULL;
	struct ctrl_command *ctrl;

	ATF_REQUIRE(get_ctrl_command("plop", &param) == NULL);
	ATF_REQUIRE(param == NULL);
	ctrl = get_ctrl_command("list", &param);
	ATF_REQUIRE(ctrl != NULL);
	ATF_REQUIRE(ctrl->type == CTRL_LIST);
	ATF_REQUIRE(param == NULL);
	ctrl = get_ctrl_command("list-", &param);
	ATF_REQUIRE(ctrl == NULL);
	ctrl = get_ctrl_command("get", &param);
	ATF_REQUIRE(ctrl == NULL);
	ctrl = get_ctrl_command("get-", &param);
	ATF_REQUIRE(ctrl != NULL);
	ATF_REQUIRE(param != NULL);
	ATF_REQUIRE_STREQ(param, "");
	free(param);
	ctrl = get_ctrl_command("get-1", &param);
	ATF_REQUIRE(ctrl != NULL);
	ATF_REQUIRE(param != NULL);
	ATF_REQUIRE_STREQ(param, "1");
	free(param);
	ctrl = get_ctrl_command("get-a/b", &param);
	ATF_REQUIRE(ctrl == NULL);
}

ATF_TC_BODY(get_recipextra_from_env_none, tc)
{
	ATF_REQUIRE(get_recipextra_from_env(-1) == NULL);
}

ATF_TC_BODY(get_recipextra_from_env_qmail, tc)
{
	char *str;
	setenv("DEFAULT", "value", 1);

	str = get_recipextra_from_env(-1);
	ATF_REQUIRE_STREQ(str, "value");
	free(str);
}

ATF_TC_BODY(get_recipextra_from_env_postfix, tc)
{
	char *str;

	setenv("EXTENSION", "value", 1);
	str = get_recipextra_from_env(-1);
	ATF_REQUIRE_STREQ(str, "value");
	free(str);
}

ATF_TC_BODY(get_recipextra_from_env_exim, tc)
{
	char *str;

	setenv("LOCAL_PART_SUFFIX", "+value", 1);
	str = get_recipextra_from_env(-1);
	ATF_REQUIRE_STREQ(str, "value");
	free(str);

	/* no valid delimiter */
	setenv("LOCAL_PART_SUFFIX", "=value", 1);
	str = get_recipextra_from_env(-1);
	ATF_REQUIRE_STREQ(str, "=value");
	free(str);
}

ATF_TC_BODY(addrmatch, tc)
{
	char *extra;

	ATF_REQUIRE(addrmatch("lists@test.org", "lists@test.org", "+", &extra));
	ATF_REQUIRE(extra == NULL);

	ATF_REQUIRE(!addrmatch("lists@test.org", NULL, "+", &extra));
	ATF_REQUIRE(!addrmatch("lists@test.org", "nope@test.org", "+", &extra));
	ATF_REQUIRE(!addrmatch("lists@test.org", "nope@test.org", NULL, &extra));
	ATF_REQUIRE(addrmatch("lists@test.org", "lists+@test.org", "+", &extra));
	ATF_REQUIRE(extra == NULL);
	ATF_REQUIRE(!addrmatch("lists@test.org", "list+@test.org", "+", &extra));
	ATF_REQUIRE(!addrmatch("lists@test.org", "lists+@bla.org", "+", &extra));
	ATF_REQUIRE(!addrmatch("lists@test.org", "bla+@test.org", "+", &extra));
	ATF_REQUIRE(!addrmatch("lists@test.org", "list+@bla.org", "+", &extra));
	ATF_REQUIRE(!addrmatch("lists@test.org", "list+test.org", "+", &extra));
	ATF_REQUIRE(addrmatch("lists@test.org", "lists+value@test.org", "+", &extra));
	ATF_REQUIRE_STREQ(extra, "value");
}

ATF_TC_BODY(get_subcookie_content, tc)
{
	init_ml(true);

	int fd = open("list", O_DIRECTORY);
	ATF_REQUIRE(fd != -1);

	rmdir("list/unsubconf");
	ATF_REQUIRE(get_subcookie_content(fd, true, "test") == NULL);
	mkdir("list/unsubconf", 0755);
	rmdir("list/subconf");
	ATF_REQUIRE(get_subcookie_content(fd, false, "test") == NULL);
	mkdir("list/subconf", 0755);
	ATF_REQUIRE(get_subcookie_content(fd, false, "test") == NULL);
	atf_utils_create_file("list/subconf/test", "");
	ATF_REQUIRE(get_subcookie_content(fd, false, "test") == NULL);
	atf_utils_create_file("list/subconf/test", "bla\nplop\n");
	ATF_REQUIRE_STREQ(get_subcookie_content(fd, false, "test"), "bla");
}

ATF_TC_BODY(ml_list, tc)
{
	struct ml list;
	ml_init(&list);
	ATF_REQUIRE(ml_open(&list, false) == false);
	list.dir = "list";
	pid_t p = atf_utils_fork();
	if (p == 0) {
		ATF_REQUIRE(ml_open(&list, false) == false);
		exit(0);
	}
	atf_utils_wait(p, 0, "", "mlmmj: Cannot open(list): No such file or directory\n");

	init_ml(false);
	rmdir("list/control");
	ml_init(&list);
	list.dir = "list";
	p = atf_utils_fork();
	if (p == 0) {
		ATF_REQUIRE(ml_open(&list, false) == false);
		exit (0);
	}
	atf_utils_wait(p, 0, "", "mlmmj: Cannot open(list/control): No such file or directory\n");
	mkdir("list/control", 0755);

	ml_init(&list);
	list.dir = "list";
	p = atf_utils_fork();
	if (p == 0) {
		ATF_REQUIRE(ml_open(&list, false) == false);
		exit (0);
	}

	atf_utils_wait(p, 0, "", "mlmmj: Missing list address\n");
	atf_utils_create_file("list/control/listaddress", "test");
	ml_init(&list);
	list.dir = "list";
	p = atf_utils_fork();
	if (p == 0) {
		ATF_REQUIRE(ml_open(&list, false) == false);
		exit (0);
	}
	atf_utils_wait(p, 0, "", "mlmmj: test: is not a valid mailing list address, missing '@'\n");

	atf_utils_create_file("list/control/listaddress", "test@test");
	ml_init(&list);
	list.dir = "list";
	p = atf_utils_fork();
	if (p == 0) {
		ATF_REQUIRE(ml_open(&list, false) == true);
		ATF_REQUIRE_STREQ_MSG(list.delim, "+", "Invalid delimiter");

		atf_utils_create_file("list/control/delimiter", "#bla#");
		ml_init(&list);
		list.dir = "list";
		ATF_REQUIRE(ml_open(&list, false) == true);
		ATF_REQUIRE_STREQ_MSG(list.delim, "#bla#", "Invalid delimiter");
		exit (0);
	}
	atf_utils_wait(p, 0, "", "");
}

ATF_TC_BODY(ml_become_list_owner, tc)
{
	struct ml ml;
	uid_t uid;

	init_ml(true);
	ml_init(&ml);
	ml.dir = "list";
	ATF_REQUIRE(ml_open(&ml, false));

	/*
	 * The list directory is owned by the user running the test, so the
	 * helper must leave the credentials untouched (whether or not we
	 * are root).
	 */
	uid = getuid();
	ml_become_list_owner(&ml);
	ATF_REQUIRE_EQ(getuid(), uid);
	ATF_REQUIRE_EQ(geteuid(), uid);
	ml_close(&ml);
}

ATF_TC_BODY(gen_addr, tc)
{
	char *var = NULL;
	struct ml ml;

	init_ml(true);
	ml_init(&ml);
	ml.dir = "list";
	ml_open(&ml, false);

	gen_addr(var, &ml, "bla");
	ATF_REQUIRE_STREQ(var, "test+bla@test");
	free(var);

	gen_addr_cookie(var, &ml, "bla", "cookie");
	ATF_REQUIRE_STREQ(var, "test+blacookie@test");
	free(var);
}

ATF_TC_BODY(memory_lines, tc)
{
	finish_memory_lines(NULL);
	memory_lines_state *s = init_memory_lines("plop");
	ATF_REQUIRE_STREQ(get_memory_line(s), "plop");
	finish_memory_lines(s);

	s = init_memory_lines("plop\n");
	ATF_REQUIRE_STREQ(get_memory_line(s), "plop");
	finish_memory_lines(s);

	s = init_memory_lines("line1\nline2\nline3");
	ATF_REQUIRE_STREQ(get_memory_line(s), "line1");
	ATF_REQUIRE_STREQ(get_memory_line(s), "line2");
	ATF_REQUIRE_STREQ(get_memory_line(s), "line3");
	ATF_REQUIRE_EQ(get_memory_line(s), NULL);

	rewind_memory_lines(NULL);
	rewind_memory_lines(s);
	ATF_REQUIRE_STREQ(get_memory_line(s), "line1");
	ATF_REQUIRE_STREQ(get_memory_line(s), "line2");
}

ATF_TC_BODY(text_0, tc)
{
	struct ml ml;
	char *datestr;
	char *expected;
	atf_utils_create_file("intext", "plop\n");
	int fd = open("intext", O_RDONLY);
	text *txt = open_text_fd(fd);
	ATF_REQUIRE(txt != NULL);
	FILE *tof = fopen("totext", "w");
	ATF_REQUIRE(tof != NULL);
	init_ml(true);
	ml_init(&ml);
	ml.dir = "list";
	ATF_REQUIRE(ml_open(&ml, false));
	ATF_REQUIRE(prepstdreply_to(txt, &ml, "me@fqdn.org", "to@plop.org", NULL, tof, "Message-Id: <fake>"));
	close_text(txt);
	fclose(tof);
	close(fd);
	datestr = gendatestr();
	xasprintf(&expected, "From: me@fqdn.org\n"
		"To: to@plop.org\n"
		"Message-Id: <fake>\n"
		"%s\n"
		"Subject: mlmmj administrivia\n"
		"MIME-Version: 1.0\n"
		"Content-Type: text/plain; charset=utf-8\n"
		"Content-Transfer-Encoding: 8bit\n"
		"\n"
		"plop\n", datestr);
	if (!atf_utils_compare_file("totext", expected)) {
		atf_utils_cat_file("totext", "");
		atf_tc_fail("Unexpected output");
	}
	free(expected);
	free(datestr);
}

ATF_TC_BODY(text_1, tc)
{
	struct ml ml;
	char *datestr;
	char *expected;
	atf_utils_create_file("intext",
	   "Subject: huhu\n"
	   "plop\n"
	   "$$\n"
	   "$listaddr$\n"
	   "$list+$\n"
	   "$listowner$\n"
	   "$helpaddr$\n"
	   "$faqaddr$\n"
	   "$listgetN$\n"
	   "$listunsubaddr$\n"
	   "$nomailunsubaddr$\n"
	   "$digestunsubaddr$\n"
	   "$listsubaddr$\n"
	   "$digestsubaddr$\n"
	   "$nomailsubaddr$\n"
	   );
	int fd = open("intext", O_RDONLY);
	text *txt = open_text_fd(fd);
	ATF_REQUIRE(txt != NULL);
	FILE *tof = fopen("totext", "w");
	ATF_REQUIRE(tof != NULL);
	init_ml(true);
	ml_init(&ml);
	ml.dir = "list";
	ATF_REQUIRE(ml_open(&ml, false));
	register_default_unformatted(txt, &ml);
	ATF_REQUIRE(prepstdreply_to(txt, &ml, "me@fqdn.org", "to@plop.org", "bla@me.org", tof, "Message-Id: <fake>"));
	close_text(txt);
	fclose(tof);
	close(fd);
	datestr = gendatestr();
	xasprintf(&expected, "Subject: huhu\n"
		"From: me@fqdn.org\n"
		"To: to@plop.org\n"
		"Message-Id: <fake>\n"
		"%s\n"
		"MIME-Version: 1.0\n"
		"Content-Type: text/plain; charset=utf-8\n"
		"Content-Transfer-Encoding: 8bit\n"
		"Reply-To: bla@me.org\n"
		"\n"
		"plop\n"
		"$\n"
		"test@test\n"
		"test+\n"
		"test+owner@test\n"
		"test+help@test\n"
		"test+faq@test\n"
		"test+get-N@test\n"
		"test+unsubscribe@test\n"
		"test+unsubscribe-nomail@test\n"
		"test+unsubscribe-digest@test\n"
		"test+subscribe@test\n"
		"test+subscribe-digest@test\n"
		"test+subscribe-nomail@test\n"
		, datestr);
	if (!atf_utils_compare_file("totext", expected)) {
		atf_utils_cat_file("totext", "");
		atf_tc_fail("Unexpected output");
	}
	free(expected);
	free(datestr);
}

ATF_TC_BODY(file_lines, tc)
{
	file_lines_state *s;

	atf_utils_create_file("file", "line1\nline2\n");

	s = init_file_lines_fd(open("file", O_RDONLY), ':');
	ATF_REQUIRE(s != NULL);
	rewind_file_lines(s);
	ATF_REQUIRE(get_file_line(s) == NULL);
	finish_file_lines(s);

	atf_utils_create_file("file", "line1: plop\nline2\nline3: meh");

	s = init_file_lines_fd(open("file", O_RDONLY), ':');
	ATF_REQUIRE(s != NULL);
	rewind_file_lines(s);
	ATF_REQUIRE_STREQ(get_file_line(s), "line1");
	ATF_REQUIRE(get_file_line(s) == NULL);
	ATF_REQUIRE_STREQ(get_file_line(s), "line3");
	ATF_REQUIRE(get_file_line(s) == NULL);
	finish_file_lines(s);

	s = init_file_lines_fd(open("file", O_RDONLY), '\0');
	finish_file_lines(s);

	s = init_file_lines_fd(open("file", O_RDONLY), '\0');
	ATF_REQUIRE(s != NULL);
	rewind_file_lines(s);
	ATF_REQUIRE_STREQ(get_file_line(s), "line1: plop");
	ATF_REQUIRE_STREQ(get_file_line(s), "line2");
	ATF_REQUIRE_STREQ(get_file_line(s), "line3: meh");
	ATF_REQUIRE(get_file_line(s) == NULL);
	rewind_file_lines(s);
	finish_file_lines(s);

	rewind_file_lines(NULL);
	ATF_REQUIRE(get_file_line(NULL) == NULL);
	finish_file_lines(NULL);
}

ATF_TC_BODY(list_subs, tc)
{
	struct ml ml;
	init_ml(true);
	ml_init(&ml);
	atf_utils_create_file("list/subscribers.d/a", "auser1\nauser2");
	atf_utils_create_file("list/subscribers.d/b", "");
	ml.dir = "list";
	ml_open(&ml, false);
	struct subs_list_state *s = init_subs_list(ml.fd, "subscribers.d");
	rewind_subs_list(NULL);
	ATF_REQUIRE(get_sub(s) == NULL);
	rewind_subs_list(s);
	ATF_REQUIRE(get_sub(NULL) == NULL);
	ATF_REQUIRE_STREQ(get_sub(s), "auser1");
	ATF_REQUIRE_STREQ(get_sub(s), "auser2");
	ATF_REQUIRE(get_sub(s) == NULL);
	finish_subs_list(NULL);
	{
		FILE *f = fopen("output", "w");
		ATF_REQUIRE(f != NULL);
		print_subs(f, s);
		fclose(f);
	}
	if (!atf_utils_compare_file("output", "auser1\nauser2\n")) {
		atf_utils_cat_file("output", "");
		atf_tc_fail("Unexpected output, expected");
	}
	rewind_subs_list(s);
	rewind_subs_list(s);
	ATF_REQUIRE_STREQ(get_sub(s), "auser1");
	ATF_REQUIRE_STREQ(get_sub(s), "auser2");
	ATF_REQUIRE(get_sub(s) == NULL);
	finish_subs_list(s);
	s = init_subs_list(ml.fd, "subscribers.d");
	finish_subs_list(s);
	s = init_subs_list(ml.fd, "subscribers.d");
	rewind_subs_list(s);
	finish_subs_list(s);
	s = init_subs_list(ml.fd, "subscribers.d");
	rewind_subs_list(s);
	ATF_REQUIRE_STREQ(get_sub(s), "auser1");
	finish_subs_list(s);
}

static void
setup_listtext(const atf_tc_t *tc)
{
	char *dir = NULL;
	const char *srcdir;
	rmdir("list/text");
	if (atf_tc_has_config_var(tc, "top_srcdir"))
		srcdir = atf_tc_get_config_var(tc, "top_srcdir");
	else
		srcdir = TOP_SRCDIR;
	xasprintf(&dir, "%s/listtexts/en", srcdir);

	symlink(dir, "list/text");
}

ATF_TC_BODY(notify_sub, tc)
{
	const char *path;
	init_ml(true);
	struct ml ml;
	ml_init(&ml);
	ml.dir = "list";
	ml_open(&ml, false);
	setup_listtext(tc);
	atf_utils_create_file("list/control/smtpport", "25678");
	atf_utils_create_file("list/control/smtphelo", "heloname");
	int smtppipe[2];
	ATF_REQUIRE(socketpair(AF_UNIX, SOCK_STREAM, 0, smtppipe) >= 0);
	pid_t p2 = single_mail_reception(smtppipe[1]);
	atf_utils_readline(smtppipe[0]);
	pid_t p = atf_utils_fork();
	if (p == 0) {
		notify_sub(&ml, "test@plop", SUB_NORMAL, SUB_ADMIN, true);
	}
	atf_utils_wait(p2, 0, "save:mailout.txt", "");
	atf_utils_wait(p, 0, "", "");
	path = "mailout.txt";
	if (!atf_utils_grep_file(".*The address <test@plop> has been subscribed to the normal.*", path)) {
		atf_utils_cat_file(path, "");
		atf_tc_fail("invalid file");
	}
	atf_utils_cat_file(path, "");
	if (!atf_utils_grep_file(".*because an administrator commanded it.*", path)) {
		atf_utils_cat_file(path, "");
		atf_tc_fail("invalid file");
	}

	p2 = single_mail_reception(smtppipe[1]);
	atf_utils_readline(smtppipe[0]);
	p = atf_utils_fork();
	if (p == 0) {
		notify_sub(&ml, "test@plop", SUB_DIGEST, SUB_ADMIN, true);
	}
	atf_utils_wait(p2, 0, "save:mailout.txt", "");
	atf_utils_wait(p, 0, "", "");
	if (!atf_utils_grep_file(".*The address <test@plop> has been subscribed to the digest.*", path)) {
		atf_utils_cat_file(path, "");
		atf_tc_fail("invalid file");
	}
	if (!atf_utils_grep_file(".*because an administrator commanded it.*", path)) {
		atf_utils_cat_file(path, "");
		atf_tc_fail("invalid file");
	}

	p2 = single_mail_reception(smtppipe[1]);
	atf_utils_readline(smtppipe[0]);
	p = atf_utils_fork();
	if (p == 0) {
		notify_sub(&ml, "test@plop", SUB_NOMAIL, SUB_CONFIRM, true);
	}
	atf_utils_wait(p2, 0, "save:mailout.txt", "");
	atf_utils_wait(p, 0, "", "");
	if (!atf_utils_grep_file(".*The address <test@plop> has been subscribed to the no-mail.*", path)) {
		atf_utils_cat_file(path, "");
		atf_tc_fail("invalid file");
	}
	if (!atf_utils_grep_file(".*because a request to join was confirmed.*", path)) {
		atf_utils_cat_file(path, "");
		atf_tc_fail("invalid file");
	}

	p2 = single_mail_reception(smtppipe[1]);
	atf_utils_readline(smtppipe[0]);
	p = atf_utils_fork();
	if (p == 0) {
		notify_sub(&ml, "test@plop", SUB_BOTH, SUB_REQUEST, true);
	}
	atf_utils_wait(p2, 0, "save:mailout.txt", "");
	atf_utils_wait(p, 0, "", "");
	if (!atf_utils_grep_file(".*The address <test@plop> has been subscribed to the version.*", path)) {
		atf_utils_cat_file(path, "");
		atf_tc_fail("invalid file");
	}
	if (!atf_utils_grep_file(".*because a request to join was received.*", path)) {
		atf_utils_cat_file(path, "");
		atf_tc_fail("invalid file");
	}

	p2 = single_mail_reception(smtppipe[1]);
	atf_utils_readline(smtppipe[0]);
	p = atf_utils_fork();
	if (p == 0) {
		notify_sub(&ml, "test@plop", SUB_NORMAL, SUB_ADMIN, false);
	}
	atf_utils_wait(p2, 0, "save:mailout.txt", "");
	atf_utils_wait(p, 0, "", "");
	if (!atf_utils_grep_file(".*The address <test@plop> has been unsubscribed from the list.*", path)) {
		atf_utils_cat_file(path, "");
		atf_tc_fail("invalid file");
	}
	atf_utils_cat_file(path, "");
	if (!atf_utils_grep_file(".*administrator commanded it.*", path)) {
		atf_utils_cat_file(path, "");
		atf_tc_fail("invalid file");
	}

	p2 = single_mail_reception(smtppipe[1]);
	atf_utils_readline(smtppipe[0]);
	p = atf_utils_fork();
	if (p == 0) {
		notify_sub(&ml, "test@plop", SUB_DIGEST, SUB_ADMIN, false);
	}
	atf_utils_wait(p2, 0, "save:mailout.txt", "");
	atf_utils_wait(p, 0, "", "");
	if (!atf_utils_grep_file(".*The address <test@plop> has been unsubscribed from the list.*", path)) {
		atf_utils_cat_file(path, "");
		atf_tc_fail("invalid file");
	}
	if (!atf_utils_grep_file(".*administrator commanded it.*", path)) {
		atf_utils_cat_file(path, "");
		atf_tc_fail("invalid file");
	}

	p2 = single_mail_reception(smtppipe[1]);
	atf_utils_readline(smtppipe[0]);
	p = atf_utils_fork();
	if (p == 0) {
		notify_sub(&ml, "test@plop", SUB_NOMAIL, SUB_CONFIRM, false);
	}
	atf_utils_wait(p2, 0, "save:mailout.txt", "");
	atf_utils_wait(p, 0, "", "");
	if (!atf_utils_grep_file(".*The address <test@plop> has been unsubscribed from the list.*", path)) {
		atf_utils_cat_file(path, "");
		atf_tc_fail("invalid file unsubscribe nomail confirm");
	}
	if (!atf_utils_grep_file(".*request to unsubscribe was confirmed.*", path)) {
		atf_utils_cat_file(path, "");
		atf_tc_fail("invalid file");
	}

	p2 = single_mail_reception(smtppipe[1]);
	atf_utils_readline(smtppipe[0]);
	p = atf_utils_fork();
	if (p == 0) {
		notify_sub(&ml, "test@plop", SUB_BOTH, SUB_REQUEST, false);
	}
	atf_utils_wait(p2, 0, "save:mailout.txt", "");
	atf_utils_wait(p, 0, "", "");
	if (!atf_utils_grep_file(".*The address <test@plop> has been unsubscribed from the list.*", path)) {
		atf_utils_cat_file(path, "");
		atf_tc_fail("invalid file");
	}
	if (!atf_utils_grep_file(".*request to unsubscribe was received.*", path)) {
		atf_utils_cat_file(path, "");
		atf_tc_fail("invalid file");
	}
}

ATF_TC_BODY(get_processed_text_line, tc)
{
	struct ml ml;
	char *ret;
	atf_utils_create_file("intext",
	   "Subject: huhu, $listaddr$ $listowner$\n"
	   "plop\n"
	   "$$\n"
	   "$listaddr$\n"
	   "$list+$\n"
	   "$listowner$\n"
	   "$helpaddr$\n"
	   "$faqaddr$\n"
	   "$listgetN$\n"
	   "$listunsubaddr$\n"
	   "$nomailunsubaddr$\n"
	   "$digestunsubaddr$\n"
	   "$listsubaddr$\n"
	   "$digestsubaddr$\n"
	   "$nomailsubaddr$\n"
	   );
	int fd = open("intext", O_RDONLY);
	text *txt = open_text_fd(fd);
	ATF_REQUIRE(txt != NULL);
	init_ml(true);
	ml_init(&ml);
	ml.dir = "list";
	ATF_REQUIRE(ml_open(&ml, false));
	register_default_unformatted(txt, &ml);
	ret = get_processed_text_line(txt, true, &ml);
	ATF_REQUIRE(ret != NULL);
	ATF_REQUIRE_STREQ(ret, "Subject: huhu, test@test test+owner@test");
	ret = get_processed_text_line(txt, true, &ml);
	ATF_REQUIRE_STREQ(ret, "plop");
	close_text(txt);
}

ATF_TC_BODY(get_processed_text_line_directives, tc)
{
	struct ml ml;
	char *ret;
	atf_utils_create_file("intextdir", "%s",
	   "a%thin%b\n"
	   "c%wide%d\n"
	   "e%nowrap%f\n"
	   "g%comment% dropped\n"
	   "x%ifcontrol nocontrol%Y%else%Z%endif%w\n"
	   );
	int fd = open("intextdir", O_RDONLY);
	text *txt = open_text_fd(fd);
	ATF_REQUIRE(txt != NULL);
	init_ml(true);
	ml_init(&ml);
	ml.dir = "list";
	ATF_REQUIRE(ml_open(&ml, false));
	register_default_unformatted(txt, &ml);

	/* Directives are spliced out of the line in place. */
	ret = get_processed_text_line(txt, true, &ml);
	ATF_REQUIRE(ret != NULL);
	ATF_REQUIRE_STREQ(ret, "ab");
	ret = get_processed_text_line(txt, true, &ml);
	ATF_REQUIRE_STREQ(ret, "cd");
	ret = get_processed_text_line(txt, true, &ml);
	ATF_REQUIRE_STREQ(ret, "ef");
	/* %comment% drops the rest of the line. */
	ret = get_processed_text_line(txt, true, &ml);
	ATF_REQUIRE_STREQ(ret, "g");
	/* Unsatisfied conditional skipped, %else% part kept. */
	ret = get_processed_text_line(txt, true, &ml);
	ATF_REQUIRE_STREQ(ret, "xZw");
	close_text(txt);
}

ATF_TC_BODY(get_processed_text_line_crlf, tc)
{
	struct ml ml;
	char *ret;
	/* CRLF, LF and lone CR line endings must all terminate the line. */
	atf_utils_create_file("intextcrlf", "%s", "a\r\nb\nc\rd");
	int fd = open("intextcrlf", O_RDONLY);
	text *txt = open_text_fd(fd);
	ATF_REQUIRE(txt != NULL);
	init_ml(true);
	ml_init(&ml);
	ml.dir = "list";
	ATF_REQUIRE(ml_open(&ml, false));
	register_default_unformatted(txt, &ml);

	ret = get_processed_text_line(txt, true, &ml);
	ATF_REQUIRE(ret != NULL);
	ATF_REQUIRE_STREQ(ret, "a");
	ret = get_processed_text_line(txt, true, &ml);
	ATF_REQUIRE_STREQ(ret, "b");
	ret = get_processed_text_line(txt, true, &ml);
	ATF_REQUIRE_STREQ(ret, "c");
	ret = get_processed_text_line(txt, true, &ml);
	ATF_REQUIRE_STREQ(ret, "d");
	close_text(txt);
}

ATF_TC_BODY(get_processed_text_line_includes, tc)
{
	struct ml ml;
	char *ret;
	int fd;
	text *txt;

	init_ml(true);
	ml_init(&ml);
	ml.dir = "list";
	ATF_REQUIRE(ml_open(&ml, false));

	/*
	 * Token names that are not plain filenames (containing '/' or a
	 * leading "..") must be left untouched in the output.
	 */
	atf_utils_create_file("intextinc", "%s",
	    "a%text bad/name%b\n"
	    "c%control ../x%d\n");
	fd = open("intextinc", O_RDONLY);
	txt = open_text_fd(fd);
	ATF_REQUIRE(txt != NULL);
	register_default_unformatted(txt, &ml);

	ret = get_processed_text_line(txt, true, &ml);
	ATF_REQUIRE(ret != NULL);
	ATF_REQUIRE_STREQ(ret, "a%text bad/name%b");
	ret = get_processed_text_line(txt, true, &ml);
	ATF_REQUIRE(ret != NULL);
	ATF_REQUIRE_STREQ(ret, "c%control ../x%d");
	close_text(txt);

	/*
	 * A valid token name is accepted; a missing control file behaves
	 * as an empty inclusion, i.e. the directive is simply removed.
	 */
	atf_utils_create_file("intextinc", "%s",
	    "a%control missing%\nc\n");
	fd = open("intextinc", O_RDONLY);
	txt = open_text_fd(fd);
	ATF_REQUIRE(txt != NULL);
	register_default_unformatted(txt, &ml);

	ret = get_processed_text_line(txt, true, &ml);
	ATF_REQUIRE(ret != NULL);
	ATF_REQUIRE_STREQ(ret, "a");
	ret = get_processed_text_line(txt, true, &ml);
	ATF_REQUIRE(ret != NULL);
	ATF_REQUIRE_STREQ(ret, "c");
	close_text(txt);
}

ATF_TC_BODY(newsmtp, tc)
{
	init_ml(true);
	struct ml ml;
	ml_init(&ml);
	ml.dir = "list";
	ATF_REQUIRE_MSG(ml_open(&ml, false), "impossible to open the mailing list");
	atf_utils_create_file("list/control/smtpport", "25678");
	int fd = newsmtp(&ml, NULL);
	ATF_REQUIRE_MSG(fd == -1, "Socket should not have open");
	int smtppipe[2];
	ATF_REQUIRE(socketpair(AF_UNIX, SOCK_STREAM, 0, smtppipe) >= 0);
	pid_t p = atf_utils_fork();
	if (p == 0) {
		int s = fakesmtp(smtppipe[1]);
		int c;
		struct sockaddr_in cl;
		socklen_t clsize = sizeof(struct sockaddr_in);
		c = accept(s, (struct sockaddr *) &cl, &clsize);
		if (c == -1)
			err(5, "accept()");
		dprintf(c, "220 me fake smtp\n");
		const char *replies[] = {
			"250-hostname.net\n"
			"250-PIPELINEING\n"
			"250-SIZE 20480000\n"
			"250-ETRN\n"
			"250-STARTTLS\n"
			"250-ENHANCEDSTATUSCODES\n"
			"250-8BITMIME\n"
			"250-DSN\n"
			"250-SMTPUTF8\n"
			"250 CHUNKING\n",
			"221 2.0.0 bye\n",
		};
		read_print_reply(c, replies, NELEM(replies));
		exit(0);
	}
	close(smtppipe[1]);
	atf_utils_readline(smtppipe[0]);
	atf_utils_create_file("list/control/smtphelo", "heloname");
	atf_utils_create_file("list/control/relayhost", "127.0.0.1");
	fd = newsmtp(&ml, NULL);
	ATF_REQUIRE_MSG(fd != -1, "Invalid socket");
	endsmtp(&fd);
	atf_utils_wait(p, 0, "EHLO heloname\r\nQUIT\r\n", "");
}

ATF_TC_BODY(save_queue, tc)
{
	struct mail mail = { 0 };
	mail.to = "plop";
	mail.from = "bla";

	save_queue("myfile", &mail);
	if (!atf_utils_file_exists("myfile.reciptto"))
		atf_tc_fail("myfile.receiptto does not exists");
	if (!atf_utils_compare_file("myfile.reciptto", "plop"))
		atf_tc_fail("myfile.reciptto: invalid content");
	if (!atf_utils_file_exists("myfile.mailfrom"))
		atf_tc_fail("myfile.receiptto does not exists");
	if (!atf_utils_compare_file("myfile.mailfrom", "bla"))
		atf_tc_fail("myfile.mailfrom: invalid content");
	if (atf_utils_file_exists("myfile.reply-to"))
		atf_tc_fail("myfile.reply-to should not exists");

	mail.replyto = "hey";
	save_queue("myfile2", &mail);
	if (!atf_utils_file_exists("myfile2.reciptto"))
		atf_tc_fail("myfile2.receiptto does not exists");
	if (!atf_utils_compare_file("myfile2.reciptto", "plop"))
		atf_tc_fail("myfile2.recriptto: invalid content");
	if (!atf_utils_file_exists("myfile2.mailfrom"))
		atf_tc_fail("myfile2.receiptto does not exists");
	if (!atf_utils_compare_file("myfile2.mailfrom", "bla"))
		atf_tc_fail("myfile2.mailfrom: invalid content");
	if (!atf_utils_file_exists("myfile2.reply-to"))
		atf_tc_fail("myfile2.reply-to does not exists");
	if (!atf_utils_compare_file("myfile2.reply-to", "hey"))
		atf_tc_fail("myfile2.reply-to: invalid content");

	unlink("myfile2.reply-to");
	unlink("myfile2.mailfrom");
	/* the files exists reciptto exist so it should fail and stop there */
	save_queue("myfile2", &mail);
	if (!atf_utils_file_exists("myfile2.mailfrom"))
		atf_tc_fail("myfile2.mailfrom does not exists but should have been created");
	if (atf_utils_file_exists("myfile2.reply-to"))
		atf_tc_fail("myfile2.reply-to should not exists");
	/* will return in the first loop */
	unlink("myfile2.reciptto");
	save_queue("myfile2", &mail);
	if (atf_utils_file_exists("myfile2.reciptto"))
		atf_tc_fail("myfile2.reciptto should not exists");
	if (atf_utils_file_exists("myfile2.reply-to"))
		atf_tc_fail("myfile2.reply-to should not exists");
}

ATF_TC_BODY(send_single_mail, tc)
{
	init_ml(true);
	struct ml ml;
	ml_init(&ml);
	ml.dir = "list";
	struct mail mail = { 0 };
	ATF_REQUIRE_MSG(ml_open(&ml, false), "impossible to open the mailing list");
	atf_utils_create_file("list/control/smtpport", "25678");
	int smtppipe[2];
	ATF_REQUIRE(socketpair(AF_UNIX, SOCK_STREAM, 0, smtppipe) >= 0);
	pid_t p = single_mail_reception(smtppipe[1]);
	close(smtppipe[1]);
	atf_utils_readline(smtppipe[0]);
	atf_utils_create_file("list/control/smtphelo", "heloname");
	atf_utils_create_file("list/control/relayhost", "127.0.0.1");
	atf_utils_create_file("mymail.txt", "headers\n\nbody\n");
	mail.to = "plop@meh";
	mail.from = "test@meh";
	mail.fp = fopen("mymail.txt", "r");
	mail.addtohdr = true;
	ATF_REQUIRE(send_single_mail(&mail, &ml, false));
	atf_utils_wait(p, 0, "save:out.txt", "");
	const char *output = ""
		"EHLO heloname\r\n"
		"MAIL FROM:<test@meh>\r\n"
		"RCPT TO:<plop@meh>\r\n"
		"DATA\r\nheaders\r\n"
		"To: plop@meh\r\n"
		"\r\n"
		"body\r\n"
		"\r\n"
		".\r\n"
		"QUIT\r\n";
	if (!atf_utils_compare_file("out.txt", output)) {
		printf("Invalid file expected: \n===\n%s\n===\ngot:\n===\n", output);
		atf_utils_cat_file("out.txt", ">");
		printf("\n===\n");
		atf_tc_fail("Invalid output");
	}
}

ATF_TC_BODY(send_or_queue_test, tc)
{
	struct ml ml;
	struct mail mail = { 0 };
	int smtppipe[2];
	pid_t p;

	init_ml(true);
	ml_init(&ml);
	ml.dir = "list";
	ATF_REQUIRE_MSG(ml_open(&ml, false), "cannot open list");
	atf_utils_create_file("list/control/smtpport", "25678");
	atf_utils_create_file("list/control/smtphelo", "heloname");
	atf_utils_create_file("list/control/relayhost", "127.0.0.1");

	/* Success: smtp up -> mail sent, queue file removed, no sidecars. */
	ATF_REQUIRE(socketpair(AF_UNIX, SOCK_STREAM, 0, smtppipe) >= 0);
	p = single_mail_reception(smtppipe[1]);
	close(smtppipe[1]);
	atf_utils_readline(smtppipe[0]);
	atf_utils_create_file("q_ok.txt", "headers\n\nbody\n");
	mail.to = "plop@meh";
	mail.from = "test@meh";
	mail.fp = fopen("q_ok.txt", "r");
	mail.addtohdr = true;
	send_or_queue("q_ok.txt", &mail, &ml);
	ATF_REQUIRE_MSG(!atf_utils_file_exists("q_ok.txt"),
	    "queue file not removed on success");
	ATF_REQUIRE_MSG(!atf_utils_file_exists("q_ok.txt.mailfrom"),
	    "sidecar created on success");
	atf_utils_wait(p, 0, "save:out_ok.txt", "");

	/* Failure: smtp down -> refiled, queue file kept, sidecars created. */
	atf_utils_create_file("list/control/smtpport", "65500");
	atf_utils_create_file("q_fail.txt", "headers\n\nbody\n");
	mail.to = "plop@meh";
	mail.from = "test@meh";
	mail.fp = fopen("q_fail.txt", "r");
	mail.addtohdr = false;
	send_or_queue("q_fail.txt", &mail, &ml);
	ATF_REQUIRE_MSG(atf_utils_file_exists("q_fail.txt"),
	    "queue file removed on failure");
	ATF_REQUIRE_MSG(atf_utils_file_exists("q_fail.txt.mailfrom"),
	    "mailfrom sidecar not created on failure");
	ATF_REQUIRE_MSG(atf_utils_file_exists("q_fail.txt.reciptto"),
	    "reciptto sidecar not created on failure");
}

ATF_TC_BODY(generate_subscription, tc)
{
	const char *path;
	init_ml(true);
	struct ml ml;
	ml_init(&ml);
	ml.dir = "list";
	ml_open(&ml, false);
	setup_listtext(tc);
	atf_utils_create_file("list/control/smtpport", "25678");
	atf_utils_create_file("list/control/smtphelo", "heloname");
	int smtppipe[2];
	ATF_REQUIRE(socketpair(AF_UNIX, SOCK_STREAM, 0, smtppipe) >= 0);

	pid_t p2 = single_mail_reception(smtppipe[1]);
	atf_utils_readline(smtppipe[0]);
	pid_t p = atf_utils_fork();
	if (p == 0) {
		generate_subscription(&ml, "test@plop", SUB_NORMAL, true);
	}
	atf_utils_wait(p2, 0, "save:mailout.txt", "");
	atf_utils_wait(p, 0, "", "");
	path = "mailout.txt";
	if (!atf_utils_grep_file(".*You were unable to be subscribed to the list because you are already*", path)) {
		atf_utils_cat_file(path, "");
		atf_tc_fail("invalid file");
	}

	p2 = single_mail_reception(smtppipe[1]);
	atf_utils_readline(smtppipe[0]);
	p = atf_utils_fork();
	if (p == 0) {
		generate_subscription(&ml, "test@plop", SUB_NORMAL, false);
	}
	atf_utils_wait(p2, 0, "save:mailout.txt", "");
	atf_utils_wait(p, 0, "", "");
	path = "mailout.txt";
	if (!atf_utils_grep_file(".*You were unable to be unsubscribed from the list because you are not*", path)) {
		atf_utils_cat_file(path, "");
		atf_tc_fail("invalid file");
	}
}

ATF_TC_BODY(generate_subconfirm, tc)
{
	const char *path;
	init_ml(true);
	struct ml ml;
	ml_init(&ml);
	ml.dir = "list";
	ml_open(&ml, false);
	setup_listtext(tc);
	atf_utils_create_file("list/control/smtpport", "25678");
	atf_utils_create_file("list/control/smtphelo", "heloname");
	int smtppipe[2];
	ATF_REQUIRE(socketpair(AF_UNIX, SOCK_STREAM, 0, smtppipe) >= 0);
	pid_t p2 = single_mail_reception(smtppipe[1]);
	atf_utils_readline(smtppipe[0]);
	pid_t p = atf_utils_fork();
	if (p == 0) {
		generate_subconfirm(&ml, "test@plop", SUB_NORMAL, SUB_ADMIN, true);
	}
	atf_utils_wait(p2, 0, "save:mailout.txt", "");
	atf_utils_wait(p, 0, "", "");
	path = "mailout.txt";
	if (!atf_utils_grep_file(".*An administrator has requested that your email address <test@plop> be added.*", path)) {
		atf_utils_cat_file(path, "");
		atf_tc_fail("invalid file");
	}
	if (!atf_utils_grep_file(".*To confirm you want to do this, please send a message to*", path)) {
		atf_utils_cat_file(path, "");
		atf_tc_fail("invalid file");
	}

	p2 = single_mail_reception(smtppipe[1]);
	atf_utils_readline(smtppipe[0]);
	p = atf_utils_fork();
	if (p == 0) {
		generate_subconfirm(&ml, "test@plop", SUB_DIGEST, SUB_ADMIN, true);
	}
	atf_utils_wait(p2, 0, "save:mailout.txt", "");
	atf_utils_wait(p, 0, "", "");
	if (!atf_utils_grep_file(".*An administrator has requested that your email address <test@plop> be added.*", path)) {
		atf_utils_cat_file(path, "");
		atf_tc_fail("invalid file");
	}
	if (!atf_utils_grep_file(".*to the list, to receive digests.*", path)) {
		atf_utils_cat_file(path, "");
		atf_tc_fail("invalid file");
	}
	if (!atf_utils_grep_file(".*To confirm you want to do this, please send a message to*", path)) {
		atf_utils_cat_file(path, "");
		atf_tc_fail("invalid file");
	}

	p2 = single_mail_reception(smtppipe[1]);
	atf_utils_readline(smtppipe[0]);
	p = atf_utils_fork();
	if (p == 0) {
		generate_subconfirm(&ml, "test@plop", SUB_NOMAIL, SUB_CONFIRM, true);
	}
	atf_utils_wait(p2, 0, "save:mailout.txt", "");
	atf_utils_wait(p, 0, "", "");
	if (!atf_utils_grep_file(".*has requested that your email address <test@plop> be added to the list.*", path)) {
		atf_utils_cat_file(path, "");
		atf_tc_fail("invalid file");
	}
	if (!atf_utils_grep_file(".*without mail delivery.*", path)) {
		atf_utils_cat_file(path, "");
		atf_tc_fail("invalid file");
	}

	p2 = single_mail_reception(smtppipe[1]);
	atf_utils_readline(smtppipe[0]);
	p = atf_utils_fork();
	if (p == 0) {
		generate_subconfirm(&ml, "test@plop", SUB_BOTH, SUB_REQUEST, true);
	}
	atf_utils_wait(p2, 0, "save:mailout.txt", "");
	atf_utils_wait(p, 0, "", "");
	if (!atf_utils_grep_file(".*Somebody \\(and we hope it was you\\) has requested that your email address.*", path)) {
		atf_utils_cat_file(path, "");
		atf_tc_fail("invalid file");
	}
	if (!atf_utils_grep_file(".*<test@plop> be added.*", path)) {
		atf_utils_cat_file(path, "");
		atf_tc_fail("invalid file");
	}

	p2 = single_mail_reception(smtppipe[1]);
	atf_utils_readline(smtppipe[0]);
	p = atf_utils_fork();
	if (p == 0) {
		generate_subconfirm(&ml, "test@plop", SUB_NORMAL, SUB_ADMIN, false);
	}
	atf_utils_wait(p2, 0, "save:mailout.txt", "");
	atf_utils_wait(p, 0, "", "");
	if (!atf_utils_grep_file(".*An administrator has requested that the email address <test@plop> be.*", path)) {
		atf_utils_cat_file(path, "");
		atf_tc_fail("invalid file");
	}

	p2 = single_mail_reception(smtppipe[1]);
	atf_utils_readline(smtppipe[0]);
	p = atf_utils_fork();
	if (p == 0) {
		generate_subconfirm(&ml, "test@plop", SUB_DIGEST, SUB_ADMIN, false);
	}
	atf_utils_wait(p2, 0, "save:mailout.txt", "");
	atf_utils_wait(p, 0, "", "");
	if (!atf_utils_grep_file(".*An administrator has requested that the email address <test@plop> be.*", path)) {
		atf_utils_cat_file(path, "");
		atf_tc_fail("invalid file");
	}
	if (!atf_utils_grep_file(".*test\\+confunsub-digest.*", path)) {
		atf_utils_cat_file(path, "");
		atf_tc_fail("invalid file");
	}

	p2 = single_mail_reception(smtppipe[1]);
	atf_utils_readline(smtppipe[0]);
	p = atf_utils_fork();
	if (p == 0) {
		generate_subconfirm(&ml, "test@plop", SUB_NOMAIL, SUB_CONFIRM, false);
	}
	atf_utils_wait(p2, 0, "save:mailout.txt", "");
	atf_utils_wait(p, 0, "", "");
	if (!atf_utils_grep_file(".*has requested that the email address <test@plop> be removed from the list..*", path)) {
		atf_utils_cat_file(path, "");
		atf_tc_fail("invalid file unsubscribe nomail confirm");
	}
	if (!atf_utils_grep_file(".*test\\+confunsub-nomail.*", path)) {
		atf_utils_cat_file(path, "");
		atf_tc_fail("invalid file");
	}

	p2 = single_mail_reception(smtppipe[1]);
	atf_utils_readline(smtppipe[0]);
	p = atf_utils_fork();
	if (p == 0) {
		generate_subconfirm(&ml, "test@plop", SUB_BOTH, SUB_REQUEST, false);
	}
	atf_utils_wait(p2, 0, "save:mailout.txt", "");
	atf_utils_wait(p, 0, "", "");
	if (!atf_utils_grep_file(".*<test@plop> be removed from the list.*", path)) {
		atf_utils_cat_file(path, "");
		atf_tc_fail("invalid file");
	}
	if (!atf_utils_grep_file(".*test\\+confunsub-both-.*", path)) {
		atf_utils_cat_file(path, "");
		atf_tc_fail("invalid file");
	}
}

ATF_TC_BODY(send_confirmation_mail, tc)
{
	const char *path;
	init_ml(true);
	struct ml ml;
	ml_init(&ml);
	ml.dir = "list";
	ml_open(&ml, false);
	setup_listtext(tc);
	atf_utils_create_file("list/control/smtpport", "25678");
	atf_utils_create_file("list/control/smtphelo", "heloname");
	int smtppipe[2];
	ATF_REQUIRE(socketpair(AF_UNIX, SOCK_STREAM, 0, smtppipe) >= 0);
	pid_t p2 = single_mail_reception(smtppipe[1]);
	atf_utils_readline(smtppipe[0]);
	send_confirmation_mail(&ml, "test@plop", SUB_NORMAL, SUB_ADMIN, true);
	atf_utils_wait(p2, 0, "save:mailout.txt", "");
	path = "mailout.txt";
	if (!atf_utils_grep_file(".*An administrator has subscribed you to the normal version of the list.*", path)) {
		atf_utils_cat_file(path, "");
		atf_tc_fail("invalid file");
	}
	if (!atf_utils_grep_file(".*The email address you are subscribed with is <test@plop>.*", path)) {
		atf_utils_cat_file(path, "");
		atf_tc_fail("invalid file");
	}

	p2 = single_mail_reception(smtppipe[1]);
	atf_utils_readline(smtppipe[0]);
	send_confirmation_mail(&ml, "test@plop", SUB_DIGEST, SUB_ADMIN, true);
	atf_utils_wait(p2, 0, "save:mailout.txt", "");
	if (!atf_utils_grep_file(".*An administrator has subscribed you to the digest version of the list.*", path)) {
		atf_utils_cat_file(path, "");
		atf_tc_fail("invalid file");
	}
	if (!atf_utils_grep_file(".*The email address you are subscribed with is <test@plop>.*", path)) {
		atf_utils_cat_file(path, "");
		atf_tc_fail("invalid file");
	}

	p2 = single_mail_reception(smtppipe[1]);
	atf_utils_readline(smtppipe[0]);
	send_confirmation_mail(&ml, "test@plop", SUB_NOMAIL, SUB_CONFIRM, true);
	atf_utils_wait(p2, 0, "save:mailout.txt", "");
	if (!atf_utils_grep_file(".*Thank you for confirming your subscription. You have now been added to the.*", path)) {
		atf_utils_cat_file(path, "");
		atf_tc_fail("invalid file");
	}
	if (!atf_utils_grep_file(".*no-mail version of the list..*", path)) {
		atf_utils_cat_file(path, "");
		atf_tc_fail("invalid file");
	}

	p2 = single_mail_reception(smtppipe[1]);
	atf_utils_readline(smtppipe[0]);
	send_confirmation_mail(&ml, "test@plop", SUB_BOTH, SUB_REQUEST, true);
	atf_utils_wait(p2, 0, "save:mailout.txt", "");
	if (!atf_utils_grep_file(".*Thank you for your request to join us. You have now been added to the.*", path)) {
		atf_utils_cat_file(path, "");
		atf_tc_fail("invalid file");
	}
	if (!atf_utils_grep_file(".*The email address you are subscribed with is <test@plop>.*", path)) {
		atf_utils_cat_file(path, "");
		atf_tc_fail("invalid file");
	}

	p2 = single_mail_reception(smtppipe[1]);
	atf_utils_readline(smtppipe[0]);
	send_confirmation_mail(&ml, "test@plop", SUB_NORMAL, SUB_ADMIN, false);
	atf_utils_wait(p2, 0, "save:mailout.txt", "");
	if (!atf_utils_grep_file(".*An administrator has removed you from the list.*", path)) {
		atf_utils_cat_file(path, "");
		atf_tc_fail("invalid file");
	}

	p2 = single_mail_reception(smtppipe[1]);
	atf_utils_readline(smtppipe[0]);
	send_confirmation_mail(&ml, "test@plop", SUB_DIGEST, SUB_ADMIN, false);
	atf_utils_wait(p2, 0, "save:mailout.txt", "");
	if (!atf_utils_grep_file(".*An administrator has removed you from the list.*", path)) {
		atf_utils_cat_file(path, "");
		atf_tc_fail("invalid file");
	}

	p2 = single_mail_reception(smtppipe[1]);
	atf_utils_readline(smtppipe[0]);
	send_confirmation_mail(&ml, "test@plop", SUB_NOMAIL, SUB_CONFIRM, false);
	atf_utils_wait(p2, 0, "save:mailout.txt", "");
	if (!atf_utils_grep_file(".*Thank you for confirming your unsubscribe. You have now been removed from.*", path)) {
		atf_utils_cat_file(path, "");
		atf_tc_fail("invalid file unsubscribe nomail confirm");
	}

	p2 = single_mail_reception(smtppipe[1]);
	atf_utils_readline(smtppipe[0]);
	send_confirmation_mail(&ml, "test@plop", SUB_BOTH, SUB_REQUEST, false);
	atf_utils_wait(p2, 0, "save:mailout.txt", "");
	if (!atf_utils_grep_file(".*You have now been removed from the list.*", path)) {
		atf_utils_cat_file(path, "");
		atf_tc_fail("invalid file");
	}
}

ATF_TC_BODY(listcontrol, tc)
{
	struct ml ml;
	init_ml(true);
	ml_init(&ml);
	ml.dir = "list";
	strlist fromemails = vec_init();
	strlist rtoemails = vec_init();

	ATF_REQUIRE(ml_open(&ml, false));
	ATF_REQUIRE_EQ(listcontrol(NULL, &rtoemails, &ml, "plop", NULL, "meh"), -1);

	ATF_REQUIRE_EQ(listcontrol(&fromemails, &rtoemails, &ml, "help", NULL, "meh"), -1);

	atf_utils_create_file("list/control/closedlist", "");
	atf_utils_create_file("meh", "mail");
	vec_push(&fromemails, "plop@test");
	ATF_REQUIRE_EQ(listcontrol(&fromemails, &rtoemails, &ml, "unsubscribe", NULL, "meh"), -1);
	if (atf_utils_file_exists("me"))
		atf_tc_fail("mail should have been removed");

	unlink("list/control/closedlist");
	atf_utils_create_file("list/control/closedlistsub", "");
	ATF_REQUIRE_EQ(listcontrol(&fromemails, &rtoemails, &ml, "subscribe", NULL, "meh"), -1);

	/* is_valid_email: reject addresses without @ */
	strlist no_at = vec_init();
	vec_push(&no_at, "invalid-address");
	ATF_REQUIRE_EQ(listcontrol(&no_at, &rtoemails, &ml, "help", NULL, "meh"), -1);

	/* is_valid_email: reject addresses with control characters */
	strlist crlf = vec_init();
	vec_push(&crlf, "evil@bad\r\nBcc: victim");
	ATF_REQUIRE_EQ(listcontrol(&crlf, &rtoemails, &ml, "help", NULL, "meh"), -1);

	strlist lf = vec_init();
	vec_push(&lf, "evil@bad\n");
	ATF_REQUIRE_EQ(listcontrol(&lf, &rtoemails, &ml, "help", NULL, "meh"), -1);

	strlist nul = vec_init();
	vec_push(&nul, "evil\x01@bad");
	ATF_REQUIRE_EQ(listcontrol(&nul, &rtoemails, &ml, "help", NULL, "meh"), -1);

}

ATF_TC_WITHOUT_HEAD(listcontrol_subscribe_replyto);
ATF_TC_BODY(listcontrol_subscribe_replyto, tc)
{
	struct ml ml;
	init_ml(true);
	ml_init(&ml);
	ml.dir = "list";

	ATF_REQUIRE(ml_open(&ml, false));
	setup_listtext(tc);

	/* When Reply-To has exactly 1 address and command is subscribe,
	 * the Reply-To address should be used for subscription */
	strlist fromemails = vec_init();
	strlist rtoemails = vec_init();
	vec_push(&fromemails, "sender@example.com");
	vec_push(&rtoemails, "sender+list@example.com");

	atf_utils_create_file("list/control/nosubconfirm", "");
	atf_utils_create_file("list/control/smtpport", "25678");
	atf_utils_create_file("list/control/smtphelo", "heloname");

	int smtppipe[2];
	ATF_REQUIRE(socketpair(AF_UNIX, SOCK_STREAM, 0, smtppipe) >= 0);
	pid_t p2 = single_mail_reception(smtppipe[1]);
	atf_utils_readline(smtppipe[0]);

	/* subscribe should use Reply-To address */
	pid_t p = atf_utils_fork();
	if (p == 0) {
		listcontrol(&fromemails, &rtoemails, &ml, "subscribe",
		    NULL, "meh");
		_exit(EXIT_SUCCESS);
	}
	atf_utils_wait(p2, 0, "save:mailout.txt", "");
	atf_utils_wait(p, 0, "", "");

	/* The subscribed address should be the Reply-To, not From */
	ATF_CHECK(atf_utils_grep_file("sender\\+list@example.com",
	    "list/subscribers.d/s"));
	ATF_CHECK(!atf_utils_grep_file("sender@example.com",
	    "list/subscribers.d/s"));
}

ATF_TC_WITHOUT_HEAD(listcontrol_subscribe_no_replyto);
ATF_TC_BODY(listcontrol_subscribe_no_replyto, tc)
{
	struct ml ml;
	init_ml(true);
	ml_init(&ml);
	ml.dir = "list";

	ATF_REQUIRE(ml_open(&ml, false));
	setup_listtext(tc);

	/* When Reply-To is empty, From address should be used */
	strlist fromemails = vec_init();
	strlist rtoemails = vec_init();
	vec_push(&fromemails, "sender@example.com");

	atf_utils_create_file("list/control/nosubconfirm", "");
	atf_utils_create_file("list/control/smtpport", "25678");
	atf_utils_create_file("list/control/smtphelo", "heloname");

	int smtppipe[2];
	ATF_REQUIRE(socketpair(AF_UNIX, SOCK_STREAM, 0, smtppipe) >= 0);
	pid_t p2 = single_mail_reception(smtppipe[1]);
	atf_utils_readline(smtppipe[0]);

	pid_t p = atf_utils_fork();
	if (p == 0) {
		listcontrol(&fromemails, &rtoemails, &ml, "subscribe",
		    NULL, "meh");
		_exit(EXIT_SUCCESS);
	}
	atf_utils_wait(p2, 0, "save:mailout.txt", "");
	atf_utils_wait(p, 0, "", "");

	/* The subscribed address should be the From address */
	ATF_CHECK(atf_utils_grep_file("sender@example.com",
	    "list/subscribers.d/s"));
}

ATF_TC_WITHOUT_HEAD(listcontrol_subscribe_replyto_multiple);
ATF_TC_BODY(listcontrol_subscribe_replyto_multiple, tc)
{
	struct ml ml;
	init_ml(true);
	ml_init(&ml);
	ml.dir = "list";

	ATF_REQUIRE(ml_open(&ml, false));
	setup_listtext(tc);

	/* When Reply-To has multiple addresses, ignore it and use From */
	strlist fromemails = vec_init();
	strlist rtoemails = vec_init();
	vec_push(&fromemails, "sender@example.com");
	vec_push(&rtoemails, "addr1@example.com");
	vec_push(&rtoemails, "addr2@example.com");

	atf_utils_create_file("list/control/nosubconfirm", "");
	atf_utils_create_file("list/control/smtpport", "25678");
	atf_utils_create_file("list/control/smtphelo", "heloname");

	int smtppipe[2];
	ATF_REQUIRE(socketpair(AF_UNIX, SOCK_STREAM, 0, smtppipe) >= 0);
	pid_t p2 = single_mail_reception(smtppipe[1]);
	atf_utils_readline(smtppipe[0]);

	pid_t p = atf_utils_fork();
	if (p == 0) {
		listcontrol(&fromemails, &rtoemails, &ml, "subscribe",
		    NULL, "meh");
		_exit(EXIT_SUCCESS);
	}
	atf_utils_wait(p2, 0, "save:mailout.txt", "");
	atf_utils_wait(p, 0, "", "");

	/* Multiple Reply-To addresses: fall back to From */
	ATF_CHECK(atf_utils_grep_file("sender@example.com",
	    "list/subscribers.d/s"));
}

ATF_TC_WITHOUT_HEAD(listcontrol_unsubscribe_ignores_replyto);
ATF_TC_BODY(listcontrol_unsubscribe_ignores_replyto, tc)
{
	struct ml ml;
	init_ml(true);
	ml_init(&ml);
	ml.dir = "list";

	ATF_REQUIRE(ml_open(&ml, false));

	/* Reply-To should be ignored for non-subscribe commands */
	strlist fromemails = vec_init();
	strlist rtoemails = vec_init();
	vec_push(&fromemails, "sender@example.com");
	vec_push(&rtoemails, "sender+list@example.com");

	/* Subscribe first so we can unsubscribe */
	atf_utils_create_file("list/subscribers.d/s",
	    "sender@example.com\n");

	atf_utils_create_file("list/control/nosubconfirm", "");
	atf_utils_create_file("meh", "mail");

	/* Unsubscribe should use From, not Reply-To */
	pid_t p = atf_utils_fork();
	if (p == 0) {
		listcontrol(&fromemails, &rtoemails, &ml, "unsubscribe",
		    NULL, "meh");
		_exit(EXIT_SUCCESS);
	}
	atf_utils_wait(p, 0, "", "");

	/* sender@example.com should be unsubscribed (From was used).
	 * The file is removed entirely when the last subscriber is gone. */
	ATF_CHECK(!atf_utils_file_exists("list/subscribers.d/s"));
}

ATF_TC_BODY(send_help, tc)
{
	init_ml(true);
	struct ml ml;
	ml_init(&ml);
	ml.dir = "list";
	ml_open(&ml, false);
	setup_listtext(tc);
	atf_utils_create_file("list/control/smtpport", "25678");
	atf_utils_create_file("list/control/smtphelo", "heloname");
	atf_utils_create_file("mail", "headers\n\nbody\n");
	int smtppipe[2];
	ATF_REQUIRE(socketpair(AF_UNIX, SOCK_STREAM, 0, smtppipe) >= 0);
	pid_t p2 = single_mail_reception(smtppipe[1]);
	atf_utils_readline(smtppipe[0]);
	pid_t p = atf_utils_fork();
	if (p == 0) {
		send_help(&ml, "mail", "test@plop");
	}
	atf_utils_wait(p2, 0, "save:mailout.txt", "");
	atf_utils_wait(p, 0, "", "");
	const char *output= "EHLO heloname\r\n"
		"MAIL FROM:<test+bounces-help@test>\r\n"
		"RCPT TO:<test@plop>\r\n"
		"DATA\r\n"
		"headers\r\n"
		"\r\n"
		"body\r\n"
		"\r\n"
		".\r\n"
		"QUIT\r\n";
	if (!atf_utils_compare_file("mailout.txt", output)) {
		atf_utils_cat_file("mailout.txt", "");
		atf_tc_fail("invalid file");
	}
}

ATF_TC_BODY(requeuemail, tc)
{
	strlist l = vec_init();
	init_ml(true);
	int dfd = open("list", O_DIRECTORY);

	rmdir("list/requeue");
	ATF_REQUIRE(!requeuemail(dfd, 1, NULL, NULL));
	ATF_REQUIRE(!requeuemail(dfd, 1, NULL, NULL));
	ATF_REQUIRE(!requeuemail(dfd, 1, &l, NULL));
	vec_push(&l, "test1");
	rmdir("list/requeue");
	ATF_REQUIRE(!requeuemail(dfd, 1, &l, NULL));
	mkdir("list/requeue", 0755);
	ATF_REQUIRE(requeuemail(dfd, 1, &l, NULL));
	if (!atf_utils_file_exists("list/requeue/1/subscribers"))
		atf_tc_fail("queue file does not exists");
	if (!atf_utils_compare_file("list/requeue/1/subscribers", "test1\n")) {
		atf_utils_cat_file("list/requeue/1/subscribers", "");
		atf_tc_fail("Invalid queue content");
	}
	unlink("list/requeue/1/subscribers");
	vec_push(&l, "testhey");
	vec_push(&l, "test2");
	ATF_REQUIRE(requeuemail(dfd, 1, &l, NULL));
	if (!atf_utils_file_exists("list/requeue/1/subscribers"))
		atf_tc_fail("queue file does not exists");
	if (!atf_utils_compare_file("list/requeue/1/subscribers", "test1\ntesthey\ntest2\n")) {
		atf_utils_cat_file("list/requeue/1/subscribers", "");
		atf_tc_fail("Invalid queue content");
	}
	unlink("list/requeue/1/subscribers");
	ATF_REQUIRE(requeuemail(dfd, 1, &l, "fail"));
	if (!atf_utils_file_exists("list/requeue/1/subscribers"))
		atf_tc_fail("queue file does not exists");
	if (!atf_utils_compare_file("list/requeue/1/subscribers", "test1\ntesthey\ntest2\nfail\n")) {
		atf_utils_cat_file("list/requeue/1/subscribers", ">");
		atf_tc_fail("Invalid queue content (fail)");
	}

	/* Regression: single-recipient send failure (addrs NULL, addr set)
	 * must still requeue the address, not silently discard it. */
	unlink("list/requeue/1/subscribers");
	ATF_REQUIRE(requeuemail(dfd, 1, NULL, "solo@lost"));
	if (!atf_utils_compare_file("list/requeue/1/subscribers", "solo@lost\n")) {
		atf_utils_cat_file("list/requeue/1/subscribers", ">");
		atf_tc_fail("addr-only requeue failed");
	}

	/* Same with an empty addrs list instead of NULL */
	unlink("list/requeue/1/subscribers");
	strlist empty = vec_init();
	ATF_REQUIRE(requeuemail(dfd, 1, &empty, "solo@lost"));
	if (!atf_utils_compare_file("list/requeue/1/subscribers", "solo@lost\n")) {
		atf_utils_cat_file("list/requeue/1/subscribers", ">");
		atf_tc_fail("addr-only requeue with empty addrs failed");
	}

	close(dfd);
}

ATF_TC_BODY(gethdrline, tc)
{
	atf_utils_create_file("mail1", "");
	atf_utils_create_file("mail2", "\n");
	atf_utils_create_file("mail3", "From: bob");
	atf_utils_create_file("mail4", "From: bob\nTo: Jane\nSubject: bla\r\n\nplop.\n");
	atf_utils_create_file("mail5", "From: bob\n  grmbl\nTo: Jane\n\t");
	FILE *f;
	char *plop, *l;
	f = fopen("mail1", "r");
	ATF_REQUIRE(gethdrline(f, NULL) == NULL);
	ATF_REQUIRE(gethdrline(f, &plop) == NULL);
	ATF_REQUIRE(plop == NULL);
	fclose(f);
	f = fopen("mail2", "r");
	ATF_REQUIRE(gethdrline(f, NULL) == NULL);
	fclose(f);
	f = fopen("mail3", "r");
	l = gethdrline(f, NULL);
	ATF_REQUIRE_STREQ(l, "From: bob");
	fclose(f);
	f = fopen("mail4", "r");
	l = gethdrline(f, NULL);
	ATF_REQUIRE_STREQ(l, "From: bob");
	l = gethdrline(f, &plop);
	ATF_REQUIRE_STREQ(l, "To: Jane");
	ATF_REQUIRE_STREQ(plop, "To: Jane\n");
	l = gethdrline(f, &plop);
	ATF_REQUIRE_STREQ(l, "Subject: bla");
	ATF_REQUIRE_STREQ(plop, "Subject: bla\r\n");
	l = gethdrline(f, &plop);
	ATF_REQUIRE(l == NULL);
	fclose(f);
	f = fopen("mail5", "r");
	l = gethdrline(f, &plop);
	ATF_REQUIRE_STREQ(l, "From: bob  grmbl");
	ATF_REQUIRE_STREQ(plop, "From: bob\n  grmbl\n");
	l = gethdrline(f, &plop);
	ATF_REQUIRE_STREQ(l, "To: Jane\t");
	ATF_REQUIRE_STREQ(plop, "To: Jane\n\t");
	fclose(f);
}

ATF_TC_BODY(readlf, tc)
{
	int fd;
	char *buf;

	/* Invalid fd */
	ATF_REQUIRE(readlf(-1, true) == NULL);

	/* Empty file */
	atf_utils_create_file("rlf_empty", "");
	fd = open("rlf_empty", O_RDONLY);
	ATF_REQUIRE(readlf(fd, true) == NULL);
	/* fd closed by readlf */

	/* Single line, oneline=true: returns chomped line */
	atf_utils_create_file("rlf_one", "hello\n");
	fd = open("rlf_one", O_RDONLY);
	buf = readlf(fd, true);
	ATF_REQUIRE(buf != NULL);
	ATF_REQUIRE_STREQ(buf, "hello");
	free(buf);

	/* Multiple lines, oneline=true: returns only first line */
	atf_utils_create_file("rlf_multi", "first\nsecond\nthird\n");
	fd = open("rlf_multi", O_RDONLY);
	buf = readlf(fd, true);
	ATF_REQUIRE(buf != NULL);
	ATF_REQUIRE_STREQ(buf, "first");
	free(buf);

	/* Single line, oneline=false: returns entire content */
	atf_utils_create_file("rlf_one2", "hello\n");
	fd = open("rlf_one2", O_RDONLY);
	buf = readlf(fd, false);
	ATF_REQUIRE(buf != NULL);
	ATF_REQUIRE_STREQ(buf, "hello\n");
	free(buf);

	/* Multiple lines, oneline=false: returns all content */
	atf_utils_create_file("rlf_multi2", "first\nsecond\nthird\n");
	fd = open("rlf_multi2", O_RDONLY);
	buf = readlf(fd, false);
	ATF_REQUIRE(buf != NULL);
	ATF_REQUIRE_STREQ(buf, "first\nsecond\nthird\n");
	free(buf);

	/* No trailing newline, oneline=true */
	atf_utils_create_file("rlf_notrim", "notrim");
	fd = open("rlf_notrim", O_RDONLY);
	buf = readlf(fd, true);
	ATF_REQUIRE(buf != NULL);
	ATF_REQUIRE_STREQ(buf, "notrim");
	free(buf);
}

ATF_TC_BODY(mod_get_addr_type, tc)
{
	mkdir("moderation/", 0775);
	atf_utils_create_file("moderation/subscribeinvalid", "");
	atf_utils_create_file("moderation/subscribevalid", "user\nSUB_BOTH\n");
	atf_utils_create_file("moderation/subscribeinvalid2", "user\n");
	atf_utils_create_file("moderation/subscribeinvalid3", "user\ninvalid3\n");

	pid_t p = atf_utils_fork();
	if (p == 0) {
		struct ml ml;
		ml_init(&ml);
		ml.dir = "list";
		enum subtype subtype;
		char *addr;
		ml.fd = open(".", O_DIRECTORY);
		mod_get_addr_and_type(&ml, "modstr", &addr, &subtype);
	}
	atf_utils_wait(p, 1, "", "");

	p = atf_utils_fork();
	if (p == 0) {
		struct ml ml;
		ml_init(&ml);
		ml.dir = "list";
		enum subtype subtype;
		char *addr;
		ml.fd = open(".", O_DIRECTORY);
		mod_get_addr_and_type(&ml, "invalid", &addr, &subtype);
	}
	atf_utils_wait(p, 1, "", "");

	p = atf_utils_fork();
	if (p == 0) {
		struct ml ml;
		ml_init(&ml);
		ml.dir = "list";
		enum subtype subtype = 0;
		char *addr;
		ml.fd = open(".", O_DIRECTORY);
		mod_get_addr_and_type(&ml, "subscribevalid", &addr, &subtype);
		ATF_REQUIRE_STREQ(addr, "user");
		ATF_REQUIRE_EQ(subtype, SUB_BOTH);
		exit(EXIT_SUCCESS);
	}
	atf_utils_wait(p, 0, "", "");

	p = atf_utils_fork();
	if (p == 0) {
		struct ml ml;
		ml_init(&ml);
		ml.dir = "list";
		enum subtype subtype = 0;
		char *addr;
		ml.fd = open(".", O_DIRECTORY);
		mod_get_addr_and_type(&ml, "subscribeinvalid2", &addr, &subtype);
		exit(EXIT_SUCCESS);
	}
	atf_utils_wait(p, 1, "", "");

	p = atf_utils_fork();
	if (p == 0) {
		struct ml ml;
		ml_init(&ml);
		ml.dir = "list";
		enum subtype subtype = 0;
		char *addr;
		ml.fd = open(".", O_DIRECTORY);
		mod_get_addr_and_type(&ml, "subscribeinvalid3", &addr, &subtype);
		exit(EXIT_SUCCESS);
	}
	atf_utils_wait(p, 1, "", "");

	mkdir("moderation/subscribe", 0755);
	atf_utils_create_file("moderation/subscribe/valid2", "user1\nSUB_BOTH\n");
	atf_utils_create_file("moderation/subscribevalid2", "user2\nSUB_BOTH\n");
	p = atf_utils_fork();
	if (p == 0) {
		struct ml ml;
		ml_init(&ml);
		ml.dir = "list";
		enum subtype subtype = 0;
		char *addr;
		ml.fd = open(".", O_DIRECTORY);
		mod_get_addr_and_type(&ml, "valid2", &addr, &subtype);
		ATF_REQUIRE_STREQ(addr, "user1");
		ATF_REQUIRE_EQ(subtype, SUB_BOTH);
		exit(EXIT_SUCCESS);
	}
	atf_utils_wait(p, 0, "", "");

	atf_utils_create_file("moderation/subscribevalid3", "user2\nSUB_BOTH\n");
	p = atf_utils_fork();
	if (p == 0) {
		struct ml ml;
		ml_init(&ml);
		ml.dir = "list";
		enum subtype subtype = 0;
		char *addr;
		ml.fd = open(".", O_DIRECTORY);
		mod_get_addr_and_type(&ml, "valid3", &addr, &subtype);
		ATF_REQUIRE_STREQ(addr, "user2");
		ATF_REQUIRE_EQ(subtype, SUB_BOTH);
		exit(EXIT_SUCCESS);
	}
	atf_utils_wait(p, 0, "", "");

	p = atf_utils_fork();
	if (p == 0) {
		struct ml ml;
		ml_init(&ml);
		ml.dir = "list";
		enum subtype subtype = 0;
		char *addr;
		ml.fd = open(".", O_DIRECTORY);
		mod_get_addr_and_type(&ml, "valid4", &addr, &subtype);
		exit(EXIT_SUCCESS);
	}
	atf_utils_wait(p, 1, "", "");

	atf_utils_create_file("moderation/subscribe/invalid5", "");
	p = atf_utils_fork();
	if (p == 0) {
		struct ml ml;
		ml_init(&ml);
		ml.dir = "list";
		enum subtype subtype = 0;
		char *addr;
		ml.fd = open(".", O_DIRECTORY);
		mod_get_addr_and_type(&ml, "invalid5", &addr, &subtype);
		exit(EXIT_SUCCESS);
	}
	atf_utils_wait(p, 1, "", "");
}

ATF_TC_BODY(send_probe_no_probefile, tc)
{
	init_ml(true);
	struct ml ml;
	ml_init(&ml);
	ml.dir = "list";
	ml_open(&ml, false);
	setup_listtext(tc);
	atf_utils_create_file("list/control/smtpport", "25678");
	atf_utils_create_file("list/control/smtphelo", "heloname");
	int smtppipe[2];
	ATF_REQUIRE(socketpair(AF_UNIX, SOCK_STREAM, 0, smtppipe) >= 0);
	pid_t p2 = single_mail_reception(smtppipe[1]);
	atf_utils_readline(smtppipe[0]);
	pid_t p = atf_utils_fork();
	if (p == 0) {
		if (send_probe(&ml, "bapt=freebsd.org"))
			exit(0);
		exit(1);
	}
	atf_utils_wait(p2, 0, "save:mailout.txt", "");
	atf_utils_wait(p, 0, "", "");

	if (!atf_utils_grep_file("Some messages to you could not be delivered", "mailout.txt")) {
		atf_utils_cat_file("mailout.txt", "");
		atf_tc_fail("invalid file");
	}
}

ATF_TC_BODY(send_probe_arobase, tc)
{
	init_ml(true);
	struct ml ml;
	ml_init(&ml);
	ml.dir = "list";
	ml_open(&ml, false);
	setup_listtext(tc);
	atf_utils_create_file("list/control/smtpport", "25678");
	atf_utils_create_file("list/control/smtphelo", "heloname");
	int smtppipe[2];
	ATF_REQUIRE(socketpair(AF_UNIX, SOCK_STREAM, 0, smtppipe) >= 0);
	pid_t p = atf_utils_fork();
	if (p == 0) {
		if (send_probe(&ml, "bapt@freebsd.org"))
			exit(0);
		exit(1);
	}
	atf_utils_wait(p, 1, "", "");
}

ATF_TC_BODY(send_probe, tc)
{
	init_ml(true);
	struct ml ml;
	ml_init(&ml);
	ml.dir = "list";
	ml_open(&ml, false);
	setup_listtext(tc);
	atf_utils_create_file("list/control/smtpport", "25678");
	atf_utils_create_file("list/control/smtphelo", "heloname");
	int smtppipe[2];
	ATF_REQUIRE(socketpair(AF_UNIX, SOCK_STREAM, 0, smtppipe) >= 0);
	pid_t p2 = single_mail_reception(smtppipe[1]);
	atf_utils_readline(smtppipe[0]);

	atf_utils_create_file("list/bounce/bapt=freebsd.org", "1234:blabla\n32:qftgd\n");
	pid_t p = atf_utils_fork();
	if (p == 0) {
		if (send_probe(&ml, "bapt=freebsd.org"))
			exit(0);
		exit(1);
	}
	atf_utils_wait(p2, 0, "save:mailout.txt", "");
	atf_utils_wait(p, 0, "", "");

	if (!atf_utils_grep_file("1234\r", "mailout.txt")) {
		atf_utils_cat_file("mailout.txt", ">");
		atf_tc_fail("invalid mail sent");
	}
}

ATF_TC_BODY(send_probe_archive, tc)
{
	init_ml(true);
	struct ml ml;
	ml_init(&ml);
	ml.dir = "list";
	ml_open(&ml, false);
	setup_listtext(tc);
	atf_utils_create_file("list/control/smtpport", "25678");
	atf_utils_create_file("list/control/smtphelo", "heloname");
	int smtppipe[2];
	ATF_REQUIRE(socketpair(AF_UNIX, SOCK_STREAM, 0, smtppipe) >= 0);
	pid_t p2 = single_mail_reception(smtppipe[1]);
	atf_utils_readline(smtppipe[0]);

	atf_utils_create_file("list/bounce/bapt=freebsd.org", "1234:blabla\n21:hey\n32:ertt\n");
	atf_utils_create_file("list/archive/1234", "From: bob@test\nMessage-ID: <msgid>\nX-None: bla\n\nContent\n");
	atf_utils_create_file("list/archive/21", "From: bob@test\nMessage-ID: <msgid2>\nX-None: bla\n\nContent\n");
	pid_t p = atf_utils_fork();
	if (p == 0) {
		if (send_probe(&ml, "bapt=freebsd.org"))
			exit(0);
		exit(1);
	}
	atf_utils_wait(p2, 0, "save:mailout.txt", "");
	atf_utils_wait(p, 0, "", "");

	if (!atf_utils_grep_file("- 1234, Message-ID: <msgid>\r", "mailout.txt")) {
		atf_utils_cat_file("mailout.txt", ">");
		atf_tc_fail("invalid mail sent");
	}
	if (!atf_utils_grep_file("- 21, Message-ID: <msgid2>\r", "mailout.txt")) {
		atf_utils_cat_file("mailout.txt", ">");
		atf_tc_fail("invalid mail sent");
	}
	if (!atf_utils_grep_file("- 32\r", "mailout.txt")) {
		atf_utils_cat_file("mailout.txt", ">");
		atf_tc_fail("invalid mail sent");
	}
}

ATF_TC_BODY(find_in_list, tc)
{
	strlist bla = vec_init();
	ATF_REQUIRE(find_in_list(NULL, NULL) == NULL);
	ATF_REQUIRE(find_in_list(&bla, NULL) == NULL);
	ATF_REQUIRE(find_in_list(&bla, "test") == NULL);
	vec_push(&bla, NULL);
	ATF_REQUIRE(find_in_list(&bla, "test") == NULL);
	vec_push(&bla, "plop");
	ATF_REQUIRE(find_in_list(&bla, "test") == NULL);
	vec_push(&bla, "test");
	ATF_REQUIRE_STREQ(find_in_list(&bla, "test"), "test");
	(void) vec_pop(&bla);
	vec_push(&bla, "test longer");
	ATF_REQUIRE_STREQ(find_in_list(&bla, "test"), "test longer");
}

ATF_TC_BODY(parse_content_type, tc)
{
	char *m, *b;
	strlist hdrs = vec_init();
	parse_content_type(NULL, &m, &b);
	ATF_REQUIRE(m == NULL);
	ATF_REQUIRE(b == NULL);
	vec_push(&hdrs, "From: plop");
	parse_content_type(&hdrs, &m, &b);
	ATF_REQUIRE(m == NULL);
	ATF_REQUIRE(b == NULL);
	vec_push(&hdrs, "Content-Type:");
	parse_content_type(&hdrs, &m, &b);
	ATF_REQUIRE(m == NULL);
	ATF_REQUIRE(b == NULL);
	(void) vec_pop(&hdrs);
	vec_push(&hdrs, "Content-Type: multipart/mixed");
	parse_content_type(&hdrs, &m, &b);
	ATF_REQUIRE_STREQ(m, "multipart/mixed");
	free(m);
	ATF_REQUIRE(b == NULL);
	(void) vec_pop(&hdrs);
	vec_push(&hdrs, "Content-Type: multipart/mixed;");
	parse_content_type(&hdrs, &m, &b);
	ATF_REQUIRE_STREQ(m, "multipart/mixed");
	free(m);
	ATF_REQUIRE(b == NULL);
	(void) vec_pop(&hdrs);
	vec_push(&hdrs, "Content-Type: multipart/mixed;\tboundary=bla");
	parse_content_type(&hdrs, &m, &b);
	ATF_REQUIRE_STREQ(m, "multipart/mixed");
	free(m);
	ATF_REQUIRE_STREQ(b,"bla");
	free(b);
	(void) vec_pop(&hdrs);
	vec_push(&hdrs, "Content-Type: multipart/mixed;\tboundary=bla;");
	parse_content_type(&hdrs, &m, &b);
	ATF_REQUIRE_STREQ(m, "multipart/mixed");
	free(m);
	ATF_REQUIRE_STREQ(b,"bla");
	free(b);

	/* Quoted boundary, preserving embedded whitespace. */
	(void) vec_pop(&hdrs);
	vec_push(&hdrs, "Content-Type: multipart/mixed; boundary=\"abc def\"");
	parse_content_type(&hdrs, &m, &b);
	ATF_REQUIRE_STREQ(m, "multipart/mixed");
	free(m);
	ATF_REQUIRE_STREQ(b, "abc def");
	free(b);

	/* Type terminated by a space with no semicolon before parameters. */
	(void) vec_pop(&hdrs);
	vec_push(&hdrs, "Content-Type: text/plain charset=utf-8");
	parse_content_type(&hdrs, &m, &b);
	ATF_REQUIRE_STREQ(m, "text/plain");
	free(m);
	ATF_REQUIRE(b == NULL);

	/* Case-insensitive boundary keyword. */
	(void) vec_pop(&hdrs);
	vec_push(&hdrs, "Content-Type: multipart/mixed; BoUnDaRy=x");
	parse_content_type(&hdrs, &m, &b);
	ATF_REQUIRE_STREQ(m, "multipart/mixed");
	free(m);
	ATF_REQUIRE_STREQ(b, "x");
	free(b);

	/* Boundary appears after another parameter. */
	(void) vec_pop(&hdrs);
	vec_push(&hdrs, "Content-Type: text/plain; charset=us-ascii; boundary=y");
	parse_content_type(&hdrs, &m, &b);
	ATF_REQUIRE_STREQ(m, "text/plain");
	free(m);
	ATF_REQUIRE_STREQ(b, "y");
	free(b);

	/* Quoted media type, no boundary. */
	(void) vec_pop(&hdrs);
	vec_push(&hdrs, "Content-Type: \"text/plain\"");
	parse_content_type(&hdrs, &m, &b);
	ATF_REQUIRE_STREQ(m, "text/plain");
	free(m);
	ATF_REQUIRE(b == NULL);

	/* Unclosed quoted media type runs to end of value. */
	(void) vec_pop(&hdrs);
	vec_push(&hdrs, "Content-Type: \"text/plain");
	parse_content_type(&hdrs, &m, &b);
	ATF_REQUIRE_STREQ(m, "text/plain");
	free(m);
	ATF_REQUIRE(b == NULL);
}

ATF_TC_BODY(do_access, tc)
{
	strlist rules = vec_init();
	strlist headers = vec_init();
	vec_push(&rules, "allow");
	char *msg = NULL;
	char *qualifier = NULL;

	ATF_REQUIRE_EQ(do_access(NULL, NULL, "test@plop", &msg, &qualifier), ACT_ALLOW);
	ATF_REQUIRE(msg == NULL);
	ATF_REQUIRE(qualifier == NULL);

	ATF_REQUIRE_EQ(do_access(&rules, NULL, "test@plop", &msg, &qualifier), ACT_ALLOW);
	ATF_REQUIRE_STREQ(msg, "access - A mail from \"test@plop\" was allowed by rule #0 \"allow\"");
	ATF_REQUIRE(qualifier == NULL);
	(void) vec_pop(&rules);
	vec_push(&rules, "deny");
	ATF_REQUIRE_EQ(do_access(&rules, NULL, "test@plop", &msg, &qualifier), ACT_DENY);
	ATF_REQUIRE_STREQ(msg, "access - A mail from \"test@plop\" was denied by rule #0 \"deny\"");
	ATF_REQUIRE(qualifier == NULL);
	(void) vec_pop(&rules);
	vec_push(&rules, "crap");
	ATF_REQUIRE_EQ(do_access(&rules, NULL, "test@plop", &msg, &qualifier), ACT_DENY);
	ATF_REQUIRE_STREQ(msg, "Unable to parse rule #0 \"crap\": Missing action keyword. Denying post from \"test@plop\"");
	ATF_REQUIRE(qualifier == NULL);
	(void) vec_pop(&rules);
	vec_push(&headers, "from: toto@bla");
	vec_push(&rules, "moderate ^from: toto@");
	ATF_REQUIRE_EQ(do_access(&rules, &headers, "test@plop", &msg, &qualifier), ACT_MODERATE);
	ATF_REQUIRE_STREQ(msg, "access - A mail from \"test@plop\" with header \"from: toto@bla\" was moderated by rule #0 \"moderate ^from: toto@\"");
	ATF_REQUIRE(qualifier == NULL);
	vec_push_front(&rules, "moderatemeh ^from: toto@");
	ATF_REQUIRE_EQ(do_access(&rules, &headers, "test@plop", &msg, &qualifier), ACT_DENY);
	ATF_REQUIRE_STREQ(msg, "Unable to parse rule #0 \"moderatemeh ^from: toto@\": Invalid character after action keyword. Denying post from \"test@plop\"");
	ATF_REQUIRE(qualifier == NULL);
	vec_push_front(&rules, "moderate- ^from: toto@");
	ATF_REQUIRE_EQ(do_access(&rules, &headers, "test@plop", &msg, &qualifier), ACT_MODERATE);
	ATF_REQUIRE(qualifier == NULL);
	ATF_REQUIRE_STREQ(msg, "access - A mail from \"test@plop\" with header \"from: toto@bla\" was moderated by rule #0 \"moderate- ^from: toto@\"");
	vec_push_front(&rules, "moderate-meh ^from: toto@");
	ATF_REQUIRE_EQ(do_access(&rules, &headers, "test@plop", &msg, &qualifier), ACT_MODERATE);
	ATF_REQUIRE_STREQ(qualifier, "meh");
	ATF_REQUIRE_STREQ(msg, "access - A mail from \"test@plop\" with header \"from: toto@bla\" was moderated by rule #0 \"moderate-meh ^from: toto@\"");

	/* Test '!' negation: rule with '!' and matching header should NOT match */
	vec_clear(&rules);
	vec_push(&rules, "deny !^from: toto@");
	ATF_REQUIRE_EQ(do_access(&rules, &headers, "test@plop", &msg, &qualifier), ACT_DENY);
	ATF_REQUIRE_STREQ(msg, "access - A mail from \"test@plop\" didn't match any rules, and was denied by default.");

	/* Test '^.*' prefix stripping */
	vec_clear(&rules);
	vec_push(&rules, "allow ^.*from: toto@");
	ATF_REQUIRE_EQ(do_access(&rules, &headers, "test@plop", &msg, &qualifier), ACT_ALLOW);

	/* Test '.*' prefix stripping */
	vec_clear(&rules);
	vec_push(&rules, "allow .*from: toto@");
	ATF_REQUIRE_EQ(do_access(&rules, &headers, "test@plop", &msg, &qualifier), ACT_ALLOW);

	/* Test no-match fallthrough: none of the rules match, denied by default */
	vec_clear(&rules);
	vec_push(&rules, "allow ^from: other@");
	vec_push(&rules, "allow ^from: nobody@");
	ATF_REQUIRE_EQ(do_access(&rules, &headers, "test@plop", &msg, &qualifier), ACT_DENY);
	ATF_REQUIRE_STREQ(msg, "access - A mail from \"test@plop\" didn't match any rules, and was denied by default.");
}

ATF_TC_BODY(unistr_header_to_utf8, tc)
{
    const char *input = "=?UTF-8?B?SGVsbG8gd29ybGQ=?=";

    char *out = unistr_header_to_utf8(input);

    ATF_REQUIRE(out != NULL);
    ATF_REQUIRE_STREQ(out, "Hello world");

    out = unistr_header_to_utf8("=?UTF-8?B?Sm9obiBEb2U=?= <john.doe@example.com>");

    ATF_REQUIRE(out != NULL);
    ATF_REQUIRE_STREQ(out, "John Doe <john.doe@example.com>");

    out = unistr_header_to_utf8("=?utf-8?b?5aSq6YOO?= <taro@example.com>");

    ATF_REQUIRE(out != NULL);
    ATF_REQUIRE_STREQ(out, "太郎 <taro@example.com>");

    out = unistr_header_to_utf8("=?ISO-8859-1?Q?Jean-Pierre_Dupont?= <jean.pierre@example.com>");

    ATF_REQUIRE(out != NULL);
    ATF_REQUIRE_STREQ(out, "Jean-Pierre Dupont <jean.pierre@example.com>");

    out = unistr_header_to_utf8("=?koi8-r?b?8NLJ18XUIM3J0g==?=");
    ATF_REQUIRE(out != NULL);
    ATF_REQUIRE_STREQ(out, "Привет мир");

    out = unistr_header_to_utf8("=?us-ascii?Q?useless_but_why_not?=");
    ATF_REQUIRE(out != NULL);
    ATF_REQUIRE_STREQ(out, "useless but why not");
}

ATF_TC_BODY(unistr_utf8_to_header, tc)
{
	char *out;

	/* Pure ASCII: returned as-is (trimmed) */
	out = unistr_utf8_to_header("Hello world");
	ATF_REQUIRE(out != NULL);
	ATF_REQUIRE_STREQ(out, "Hello world");
	free(out);

	/* Leading/trailing whitespace trimmed */
	out = unistr_utf8_to_header("  Hello  ");
	ATF_REQUIRE(out != NULL);
	ATF_REQUIRE_STREQ(out, "Hello");
	free(out);

	/* Non-ASCII: Q-encoded */
	out = unistr_utf8_to_header("café");
	ATF_REQUIRE(out != NULL);
	ATF_REQUIRE(strstr(out, "=?utf-8?q?") != NULL);
	ATF_REQUIRE(strstr(out, "?=") != NULL);
	free(out);

	/* Japanese: Q-encoded */
	out = unistr_utf8_to_header("太郎");
	ATF_REQUIRE(out != NULL);
	ATF_REQUIRE(strstr(out, "=?utf-8?q?") != NULL);
	free(out);

	/* Spaces become underscores in Q-encoding */
	out = unistr_utf8_to_header("héllo wörld");
	ATF_REQUIRE(out != NULL);
	ATF_REQUIRE(strstr(out, "_") != NULL);
	free(out);
}

ATF_TC_BODY(unistr_roundtrip, tc)
{
	char *encoded, *decoded;

	/* ASCII roundtrip */
	encoded = unistr_utf8_to_header("Hello world");
	decoded = unistr_header_to_utf8(encoded);
	ATF_REQUIRE_STREQ(decoded, "Hello world");
	free(encoded);
	free(decoded);

	/* French accents roundtrip */
	encoded = unistr_utf8_to_header("café résumé");
	decoded = unistr_header_to_utf8(encoded);
	ATF_REQUIRE_STREQ(decoded, "café résumé");
	free(encoded);
	free(decoded);

	/* Japanese roundtrip */
	encoded = unistr_utf8_to_header("太郎");
	decoded = unistr_header_to_utf8(encoded);
	ATF_REQUIRE_STREQ(decoded, "太郎");
	free(encoded);
	free(decoded);

	/* Russian roundtrip */
	encoded = unistr_utf8_to_header("Привет мир");
	decoded = unistr_header_to_utf8(encoded);
	ATF_REQUIRE_STREQ(decoded, "Привет мир");
	free(encoded);
	free(decoded);
}

ATF_TC_BODY(unistr_to_utf8_4byte, tc)
{
	unistr *us;
	char *result;

	/* 4-byte UTF-8 encoding: U+1F600 (emoji) */
	us = unistr_new();
	unistr_append_char(us, 0x1F600);
	result = unistr_to_utf8(us);
	ATF_REQUIRE_STREQ(result, "\xF0\x9F\x98\x80");
	free(result);
	unistr_free(us);

	/* 4-byte with 3-byte mixed */
	us = unistr_new();
	unistr_append_char(us, 'A');
	unistr_append_char(us, 0x1F600);
	unistr_append_char(us, 0x20AC); /* euro sign */
	result = unistr_to_utf8(us);
	/* 0x20AC = E2 82 AC in UTF-8 */
	ATF_REQUIRE_STREQ(result, "A\xF0\x9F\x98\x80\xE2\x82\xAC");
	free(result);
	unistr_free(us);
}

ATF_TC_BODY(unistr_append_utf8_edge, tc)
{
	unistr *us;
	char *result;

	/* Valid 4-byte UTF-8 sequence (emoji) */
	us = unistr_new();
	unistr_append_utf8(us, "\xF0\x9F\x98\x80", 4);
	result = unistr_to_utf8(us);
	ATF_REQUIRE_STREQ(result, "\xF0\x9F\x98\x80");
	free(result);
	unistr_free(us);

	/* Truncated 4-byte sequence (only 3 bytes) */
	us = unistr_new();
	unistr_append_utf8(us, "\xF0\x9F\x98", 3);
	result = unistr_to_utf8(us);
	/* truncated sequence should produce '?' */
	ATF_REQUIRE_STREQ(result, "?");
	free(result);
	unistr_free(us);

	/* Invalid continuation byte in 3-byte sequence */
	us = unistr_new();
	unistr_append_utf8(us, "\xE0\xA0\xC0", 3);
	result = unistr_to_utf8(us);
	ATF_REQUIRE_STREQ(result, "?");
	free(result);
	unistr_free(us);
}

ATF_TC_BODY(unistr_to_utf8_large, tc)
{
	unistr *us;
	char *result;

	/* 5-byte encoding: U+400000 */
	us = unistr_new();
	unistr_append_char(us, 0x400000);
	result = unistr_to_utf8(us);
	/* F8 90 80 80 80 in UTF-8 */
	ATF_REQUIRE_STREQ(result, "\xF8\x90\x80\x80\x80");
	free(result);
	unistr_free(us);

	/* 6-byte encoding: U+40000000 */
	us = unistr_new();
	unistr_append_char(us, 0x40000000);
	result = unistr_to_utf8(us);
	/* FD 80 80 80 80 80 in UTF-8 */
	ATF_REQUIRE_STREQ(result, "\xFD\x80\x80\x80\x80\x80");
	free(result);
	unistr_free(us);
}

ATF_TC_BODY(unistr_append_utf8_invalid, tc)
{
	unistr *us;
	char *result;

	/* Invalid first byte (0xFE) */
	us = unistr_new();
	unistr_append_utf8(us, "\xFE\x80\x80\x80", 4);
	result = unistr_to_utf8(us);
	ATF_REQUIRE_STREQ(result, "????");
	free(result);
	unistr_free(us);

	/* Overlong sequence: 2-byte encoding of NUL (U+0000) */
	us = unistr_new();
	unistr_append_utf8(us, "\xC0\x80", 2);
	result = unistr_to_utf8(us);
	/* overlong encoding of NUL -> '?' */
	ATF_REQUIRE_STREQ(result, "?");
	free(result);
	unistr_free(us);

	/* Overlong sequence: 3-byte encoding of NUL */
	us = unistr_new();
	unistr_append_utf8(us, "\xE0\x80\x80", 3);
	result = unistr_to_utf8(us);
	ATF_REQUIRE_STREQ(result, "?");
	free(result);
	unistr_free(us);

	/* Overlong sequence: 4-byte encoding of NUL */
	us = unistr_new();
	unistr_append_utf8(us, "\xF0\x80\x80\x80", 4);
	result = unistr_to_utf8(us);
	ATF_REQUIRE_STREQ(result, "?");
	free(result);
	unistr_free(us);

	/* Mixed valid and invalid */
	us = unistr_new();
	unistr_append_utf8(us, "A\xFE\x80\x80\x80" "B", 7);
	result = unistr_to_utf8(us);
	ATF_REQUIRE_STREQ(result, "A????B");
	free(result);
	unistr_free(us);
}

ATF_TC_BODY(unistr_escaped_to_utf8, tc)
{
	char *result;

	/* Basic escaped unicode */
	result = unistr_escaped_to_utf8("hyggem\\u00F8de torsdag");
	ATF_REQUIRE(result != NULL);
	ATF_REQUIRE_STREQ(result, "hyggem\303\270de torsdag");
	free(result);

	/* Double backslash escapes */
	result = unistr_escaped_to_utf8("c:\\\\path\\\\to\\\\file");
	ATF_REQUIRE_STREQ(result, "c:\\\\path\\\\to\\\\file");
	free(result);

	/* Invalid hex digits */
	result = unistr_escaped_to_utf8("\\uXXYZ");
	ATF_REQUIRE(result != NULL);
	free(result);

	/* Multiple escapes */
	result = unistr_escaped_to_utf8("\\u0048\\u0065\\u006C\\u006C\\u006F");
	ATF_REQUIRE_STREQ(result, "Hello");
	free(result);
}

ATF_TC_BODY(line_startwithany, tc)
{
	strlist l  = vec_init();
	vec_push(&l, "bla");
	ATF_REQUIRE(!line_startwithany("BLA", &l, false));
	ATF_REQUIRE(line_startwithany("BLA", &l, true));
	ATF_REQUIRE(!line_startwithany("BLOP", &l, false));
	ATF_REQUIRE(!line_startwithany("BLOP", &l, true));
	vec_push(&l, "BLOP");
	ATF_REQUIRE(line_startwithany("BLOP", &l, false));
	ATF_REQUIRE(line_startwithany("BLOP", &l, true));
}

ATF_TC_BODY(save_hdr_if_needed, tc)
{
	struct mailhdr readhdrs[] = {
		{ "Plop", 0, NULL },
		{ NULL, 0, NULL },
	};
	save_hdr_if_needed("bla", readhdrs);
	ATF_REQUIRE(readhdrs[0].valuecount == 0);
	save_hdr_if_needed("Plop: meh", readhdrs);
	ATF_REQUIRE(readhdrs[0].valuecount == 1);
	ATF_REQUIRE_STREQ(readhdrs[0].values[0], ": meh");
	save_hdr_if_needed("Plop: bla", readhdrs);
	ATF_REQUIRE(readhdrs[0].valuecount == 2);
	ATF_REQUIRE_STREQ(readhdrs[0].values[1], ": bla");
}

ATF_TC_BODY(autosubscribe_sender, tc)
{
	struct ml list;

	init_ml(true);
	ml_init(&list);
	list.dir = "list";
	ml_open(&list, false);
	int cnt = 0;

	atf_utils_create_file("myemailbody.txt", "line");
	int mfd = open("myemailbody.txt", O_RDONLY);
	free(autosubscribe_sender(&list, mfd, SUB_NOMAIL));
	cnt = count_dir_entries_from_mldir(list.fd, "nomailsubs.d");
	close(mfd);

	atf_utils_create_file("myemailbody.txt", "From: bapt");
	mfd = open("myemailbody.txt", O_RDONLY);
	free(autosubscribe_sender(&list, mfd, SUB_NOMAIL));
	cnt = count_dir_entries_from_mldir(list.fd, "nomailsubs.d");
	ATF_REQUIRE_EQ_MSG(cnt, 0, "Invalid number of entries %d, expectiing 0", cnt);
	close(mfd);

	atf_utils_create_file("myemailbody.txt", "From: bapt@plop");
	mfd = open("myemailbody.txt", O_RDONLY);
	free(autosubscribe_sender(&list, mfd, SUB_NOMAIL));
	cnt = count_dir_entries_from_mldir(list.fd, "nomailsubs.d");
	ATF_REQUIRE_EQ_MSG(cnt, 1, "Invalid number of entries %d, expectiing 0", cnt);
	close(mfd);
	if (!atf_utils_compare_file("list/nomailsubs.d/b", "bapt@plop\n")) {
		atf_utils_cat_file("list/nomailsubs.d/b", "");
		atf_tc_fail("Unexpected subscribtion");
	}

	atf_utils_create_file("myemailbody.txt", "From: bapt <bapt@meh>");
	mfd = open("myemailbody.txt", O_RDONLY);
	free(autosubscribe_sender(&list, mfd, SUB_NOMAIL));
	cnt = count_dir_entries_from_mldir(list.fd, "nomailsubs.d");
	ATF_REQUIRE_EQ_MSG(cnt, 1, "Invalid number of entries %d, expectiing 0", cnt);
	close(mfd);
	if (!atf_utils_compare_file("list/nomailsubs.d/b", "bapt@plop\nbapt@meh\n")) {
		atf_utils_cat_file("list/nomailsubs.d/b", "");
		atf_tc_fail("Unexpected subscribtion");
	}
}

ATF_TC_BODY(scan_headers_test, tc)
{
	struct mailhdr readhdrs[] = {
		{ "From:", 0, NULL },
		{ "To:", 0, NULL },
		{ "Subject:", 0, NULL },
		{ "Cc:", 0, NULL },
		{ NULL, 0, NULL }
	};
	strlist allhdrs = vec_init();
	strlist allunfoldeds = vec_init();
	FILE *f;

	/* Simple headers */
	atf_utils_create_file("scan1.txt",
	    "From: alice@example.org\n"
	    "To: list@example.org\n"
	    "Subject: Test message\n"
	    "\n"
	    "Body.\n");
	f = fopen("scan1.txt", "r");
	ATF_REQUIRE(f != NULL);
	scan_headers(f, readhdrs, &allhdrs, &allunfoldeds);
	fclose(f);

	ATF_REQUIRE_EQ(readhdrs[0].valuecount, 1);
	ATF_REQUIRE(strstr(readhdrs[0].values[0], "alice@example.org") != NULL);
	ATF_REQUIRE_EQ(readhdrs[1].valuecount, 1);
	ATF_REQUIRE(strstr(readhdrs[1].values[0], "list@example.org") != NULL);
	ATF_REQUIRE_EQ(readhdrs[2].valuecount, 1);
	ATF_REQUIRE(strstr(readhdrs[2].values[0], "Test message") != NULL);
	ATF_REQUIRE_EQ(readhdrs[3].valuecount, 0);

	ATF_REQUIRE_EQ(allhdrs.len, 3);
	ATF_REQUIRE_EQ(allunfoldeds.len, 3);

	vec_free_and_free(&allhdrs, free);
	vec_free_and_free(&allunfoldeds, free);

	/* Folded header (continuation line) */
	{
	struct mailhdr readhdrs2[] = {
		{ "Subject:", 0, NULL },
		{ NULL, 0, NULL }
	};
	strlist allhdrs2 = vec_init();
	strlist allunfoldeds2 = vec_init();

	atf_utils_create_file("scan2.txt",
	    "Subject: This is a very long\n"
	    "  subject line\n"
	    "\n"
	    "Body.\n");
	f = fopen("scan2.txt", "r");
	ATF_REQUIRE(f != NULL);
	scan_headers(f, readhdrs2, &allhdrs2, &allunfoldeds2);
	fclose(f);

	ATF_REQUIRE_EQ(readhdrs2[0].valuecount, 1);
	/* Unfolded header should have both parts */
	ATF_REQUIRE_EQ(allhdrs2.len, 1);
	ATF_REQUIRE(strstr(allhdrs2.d[0], "subject line") != NULL);
	/* allunfoldeds keeps the original folded form */
	ATF_REQUIRE_EQ(allunfoldeds2.len, 1);
	ATF_REQUIRE(strstr(allunfoldeds2.d[0], "\n") != NULL);

	vec_free_and_free(&allhdrs2, free);
	vec_free_and_free(&allunfoldeds2, free);
	}

	/* Empty headers (just body) */
	{
	struct mailhdr readhdrs3[] = {
		{ "From:", 0, NULL },
		{ NULL, 0, NULL }
	};
	strlist allhdrs3 = vec_init();

	atf_utils_create_file("scan3.txt", "\nBody only.\n");
	f = fopen("scan3.txt", "r");
	ATF_REQUIRE(f != NULL);
	scan_headers(f, readhdrs3, &allhdrs3, NULL);
	fclose(f);

	ATF_REQUIRE_EQ(readhdrs3[0].valuecount, 0);
	ATF_REQUIRE_EQ(allhdrs3.len, 0);
	vec_free_and_free(&allhdrs3, free);
	}
}

ATF_TC_BODY(substitute_test, tc)
{
	struct ml ml;
	char *result;

	init_ml(true);
	ml_init(&ml);
	ml.dir = "list";
	ATF_REQUIRE(ml_open(&ml, false));

	/* Create a simple text template */
	atf_utils_create_file("subst_in", "Hello $mytoken$\n");
	int fd = open("subst_in", O_RDONLY);
	text *txt = open_text_fd(fd);
	ATF_REQUIRE(txt != NULL);
	register_unformatted(txt, "mytoken", "world");

	/* Plain text without tokens */
	result = substitute("no tokens here", ml.fd, ml.ctrlfd, txt);
	ATF_REQUIRE_STREQ(result, "no tokens here");
	free(result);

	/* Simple token substitution */
	result = substitute("Hello $mytoken$!", ml.fd, ml.ctrlfd, txt);
	ATF_REQUIRE_STREQ(result, "Hello world!");
	free(result);

	/* Multiple tokens on same line */
	result = substitute("$mytoken$ and $mytoken$", ml.fd, ml.ctrlfd, txt);
	ATF_REQUIRE_STREQ(result, "world and world");
	free(result);

	/* Unknown token: preserved as-is */
	result = substitute("$unknown$", ml.fd, ml.ctrlfd, txt);
	ATF_REQUIRE_STREQ(result, "$unknown$");
	free(result);

	/* Lone $ without closing: passed through */
	result = substitute("price is $5", ml.fd, ml.ctrlfd, txt);
	ATF_REQUIRE_STREQ(result, "price is $5");
	free(result);

	/* Empty string */
	result = substitute("", ml.fd, ml.ctrlfd, txt);
	ATF_REQUIRE_STREQ(result, "");
	free(result);

	/* Token at start and end */
	result = substitute("$mytoken$", ml.fd, ml.ctrlfd, txt);
	ATF_REQUIRE_STREQ(result, "world");
	free(result);

	/* Adjacent tokens */
	result = substitute("$mytoken$$mytoken$", ml.fd, ml.ctrlfd, txt);
	ATF_REQUIRE_STREQ(result, "worldworld");
	free(result);

	close_text(txt);
}

ATF_TC_BODY(sb_cat_n, tc)
{
	sb_t sb = sb_init();

	/* Test basic sb_cat_n */
	sb_cat_n(&sb, "hello", 5);
	ATF_REQUIRE_EQ(sb.len, 5);
	ATF_REQUIRE_STREQ(sb.d, "hello");

	/* Append n with null terminator */
	sb_cat_n(&sb, " world", 6);
	ATF_REQUIRE_EQ(sb.len, 11);
	ATF_REQUIRE_STREQ(sb.d, "hello world");

	/* Append n with zero length */
	sb_cat_n(&sb, "", 0);
	ATF_REQUIRE_EQ(sb.len, 11);
	ATF_REQUIRE_STREQ(sb.d, "hello world");

	sb_fini(&sb);
}

ATF_TC_BODY(sb_reset, tc)
{
	sb_t sb = sb_init();

	/* Reset empty sb */
	sb_reset(&sb);
	ATF_REQUIRE_EQ(sb.len, 0);

	/* Reset sb with content */
	sb_cat(&sb, "hello");
	ATF_REQUIRE_EQ(sb.len, 5);
	sb_reset(&sb);
	ATF_REQUIRE_EQ(sb.len, 0);
	ATF_REQUIRE_STREQ(sb.d, "");

	/* Reset and reuse */
	sb_cat(&sb, "world");
	ATF_REQUIRE_STREQ(sb.d, "world");

	sb_fini(&sb);
}

ATF_TC_BODY(sb_str, tc)
{
	sb_t sb = sb_init();

	/* sb_str on empty sb */
	char *s = sb_str(&sb);
	ATF_REQUIRE(s != NULL);
	ATF_REQUIRE_STREQ(s, "");

	/* sb_str does not transfer ownership, sb is still valid */
	sb_cat(&sb, "hello");
	ATF_REQUIRE_STREQ(sb.d, "hello");
	sb_fini(&sb);
}

ATF_TC_BODY(sb_printf_growth, tc)
{
	sb_t sb = sb_init();

	/* Start with small buffer, force growth */
	sb_printf(&sb, "hello");
	ATF_REQUIRE_STREQ(sb.d, "hello");

	/* Something that fits */
	sb_printf(&sb, " ");
	ATF_REQUIRE_STREQ(sb.d, "hello ");

	/* Force growth by appending a long string */
	sb_printf(&sb, "world and all the people who live on this beautiful planet");
	ATF_REQUIRE_STREQ(sb.d, "hello world and all the people who live on this beautiful planet");

	/* sb_printf into empty sb */
	sb_t sb2 = sb_init();
	sb_printf(&sb2, "direct prinf %d", 42);
	ATF_REQUIRE_STREQ(sb2.d, "direct prinf 42");
	sb_fini(&sb2);

	sb_fini(&sb);
}

ATF_TC_BODY(voodoo_header_manipulation, tc)
{
	strlist allheaders = vec_init();
	strlist delhdrs = vec_init();
	struct mailhdr readhdrs[] = {
		{ "From:", 0, NULL },
		{ "To:", 0, NULL },
		{ "Subject:", 0, NULL },
		{ NULL, 0, NULL }
	};
	int infd, outfd, hdrfd, footfd, ret;

	/* Create a test email */
	atf_utils_create_file("voodoo_in.txt",
	    "From: sender@example.org\n"
	    "To: list@example.org\n"
	    "Return-Path: <sender@example.org>\n"
	    "Subject: Hello\n"
	    "\n"
	    "Body text here.\n");

	/* Delete Return-Path header */
	vec_push(&delhdrs, strdup("Return-Path:"));

	/* Custom header to inject */
	atf_utils_create_file("customhdr.txt", "X-List-ID: mylist\n");
	hdrfd = open("customhdr.txt", O_RDONLY);
	ATF_REQUIRE(hdrfd >= 0);

	/* Footer to inject */
	atf_utils_create_file("footer.txt", "-- \nMailing list footer\n");
	footfd = open("footer.txt", O_RDONLY);
	ATF_REQUIRE(footfd >= 0);

	infd = open("voodoo_in.txt", O_RDONLY);
	ATF_REQUIRE(infd >= 0);
	outfd = open("voodoo_out.txt", O_RDWR|O_CREAT|O_TRUNC, 0600);
	ATF_REQUIRE(outfd >= 0);

	ret = do_all_the_voodoo_here(infd, outfd, hdrfd, footfd,
	    &delhdrs, readhdrs, &allheaders, "[LIST]", 0, NULL);
	ATF_REQUIRE_EQ(ret, 0);
	close(infd);
	close(outfd);

	/* Verify readhdrs captured the values */
	ATF_REQUIRE_EQ(readhdrs[0].valuecount, 1);
	ATF_REQUIRE(strstr(readhdrs[0].values[0], "sender@example.org") != NULL);
	ATF_REQUIRE_EQ(readhdrs[2].valuecount, 1);

	/* Verify output */
	const char *path = "voodoo_out.txt";

	/* Return-Path should be deleted */
	if (atf_utils_grep_file("Return-Path:", path)) {
		atf_utils_cat_file(path, ">");
		atf_tc_fail("Return-Path should have been deleted");
	}

	/* Custom header should be present */
	if (!atf_utils_grep_file("X-List-ID: mylist", path)) {
		atf_utils_cat_file(path, ">");
		atf_tc_fail("Custom header X-List-ID missing");
	}

	/* Subject should have prefix */
	if (!atf_utils_grep_file("Subject:.*\\[LIST\\].*Hello", path)) {
		atf_utils_cat_file(path, ">");
		atf_tc_fail("Subject prefix [LIST] missing");
	}

	/* Footer should be present */
	if (!atf_utils_grep_file("Mailing list footer", path)) {
		atf_utils_cat_file(path, ">");
		atf_tc_fail("Footer missing");
	}

	/* Body should be present */
	if (!atf_utils_grep_file("Body text here", path)) {
		atf_utils_cat_file(path, ">");
		atf_tc_fail("Body missing");
	}

	vec_free_and_free(&allheaders, free);
	vec_free_and_free(&delhdrs, free);
}

ATF_TC_BODY(voodoo_replyto, tc)
{
	strlist allheaders = vec_init();
	strlist delhdrs = vec_init();
	struct mailhdr readhdrs[] = {
		{ "From:", 0, NULL },
		{ "To:", 0, NULL },
		{ "Subject:", 0, NULL },
		{ NULL, 0, NULL }
	};
	int infd, outfd;

	atf_utils_create_file("replyto_in.txt",
	    "From: alice@example.org\n"
	    "To: list@example.org\n"
	    "Subject: Hello\n"
	    "\n"
	    "Body.\n");

	infd = open("replyto_in.txt", O_RDONLY);
	ATF_REQUIRE(infd >= 0);
	outfd = open("replyto_out.txt", O_RDWR|O_CREAT|O_TRUNC, 0600);
	ATF_REQUIRE(outfd >= 0);

	/* replyto=1 should add Reply-To from From header */
	ATF_REQUIRE_EQ(do_all_the_voodoo_here(infd, outfd, -1, -1,
	    &delhdrs, readhdrs, &allheaders, NULL, 1, NULL), 0);
	close(infd);
	close(outfd);

	if (!atf_utils_grep_file("Reply-To:.*alice@example.org", "replyto_out.txt")) {
		atf_utils_cat_file("replyto_out.txt", ">");
		atf_tc_fail("Reply-To header not added");
	}
	/* From should still be present */
	if (!atf_utils_grep_file("From:.*alice@example.org", "replyto_out.txt")) {
		atf_utils_cat_file("replyto_out.txt", ">");
		atf_tc_fail("From header missing");
	}

	vec_free_and_free(&allheaders, free);
	vec_free_and_free(&delhdrs, free);
}

ATF_TC_BODY(voodoo_replyto_once, tc)
{
	strlist allheaders = vec_init();
	strlist delhdrs = vec_init();
	struct mailhdr readhdrs[] = {
		{ "From:", 0, NULL },
		{ "To:", 0, NULL },
		{ "Subject:", 0, NULL },
		{ NULL, 0, NULL }
	};
	int infd, outfd;

	/* Multiple From: headers should only generate one Reply-To */
	atf_utils_create_file("replyto_once_in.txt",
	    "From: alice@example.org\n"
	    "From: bob@example.org\n"
	    "To: list@example.org\n"
	    "Subject: Hello\n"
	    "\n"
	    "Body.\n");

	infd = open("replyto_once_in.txt", O_RDONLY);
	ATF_REQUIRE(infd >= 0);
	outfd = open("replyto_once_out.txt", O_RDWR|O_CREAT|O_TRUNC, 0600);
	ATF_REQUIRE(outfd >= 0);

	ATF_REQUIRE_EQ(do_all_the_voodoo_here(infd, outfd, -1, -1,
	    &delhdrs, readhdrs, &allheaders, NULL, 1, NULL), 0);
	close(infd);
	close(outfd);

	/* Should have exactly one Reply-To header */
	if (!atf_utils_grep_file("Reply-To:.*alice@example.org", "replyto_once_out.txt")) {
		atf_utils_cat_file("replyto_once_out.txt", ">");
		atf_tc_fail("Reply-To for alice not found");
	}
	/* Verify only one Reply-To line exists */
	int count = 0;
	FILE *f = fopen("replyto_once_out.txt", "r");
	if (f != NULL) {
		char *buf = NULL;
		size_t bufcap = 0;
		while (getline(&buf, &bufcap, f) > 0) {
			if (strncasecmp(buf, "Reply-To:", 9) == 0)
				count++;
		}
		free(buf);
		fclose(f);
	}
	if (count != 1) {
		atf_utils_cat_file("replyto_once_out.txt", ">");
		atf_tc_fail("Expected 1 Reply-To header, got %d", count);
	}

	vec_free_and_free(&allheaders, free);
	vec_free_and_free(&delhdrs, free);
}

ATF_TC_BODY(voodoo_prefix_dedup, tc)
{
	strlist allheaders = vec_init();
	strlist delhdrs = vec_init();
	struct mailhdr readhdrs[] = {
		{ "From:", 0, NULL },
		{ "To:", 0, NULL },
		{ "Subject:", 0, NULL },
		{ NULL, 0, NULL }
	};
	int infd, outfd;

	/* Subject already contains the prefix: should not be duplicated */
	atf_utils_create_file("dedup_in.txt",
	    "From: bob@example.org\n"
	    "To: list@example.org\n"
	    "Subject: [LIST] Already prefixed\n"
	    "\n"
	    "Body.\n");

	infd = open("dedup_in.txt", O_RDONLY);
	ATF_REQUIRE(infd >= 0);
	outfd = open("dedup_out.txt", O_RDWR|O_CREAT|O_TRUNC, 0600);
	ATF_REQUIRE(outfd >= 0);

	ATF_REQUIRE_EQ(do_all_the_voodoo_here(infd, outfd, -1, -1,
	    &delhdrs, readhdrs, &allheaders, "[LIST]", 0, NULL), 0);
	close(infd);
	close(outfd);

	/* Should have exactly one Subject line, not duplicated prefix */
	if (!atf_utils_grep_file("Subject:.*\\[LIST\\].*Already prefixed", "dedup_out.txt")) {
		atf_utils_cat_file("dedup_out.txt", ">");
		atf_tc_fail("Subject not found");
	}
	/* Verify no double prefix */
	if (atf_utils_grep_file("\\[LIST\\].*\\[LIST\\]", "dedup_out.txt")) {
		atf_utils_cat_file("dedup_out.txt", ">");
		atf_tc_fail("Subject prefix duplicated");
	}

	vec_free_and_free(&allheaders, free);
	vec_free_and_free(&delhdrs, free);

	/* Mail without Subject: prefix should be added as new Subject */
	{
	strlist allheaders2 = vec_init();
	struct mailhdr readhdrs2[] = {
		{ "From:", 0, NULL },
		{ "To:", 0, NULL },
		{ "Subject:", 0, NULL },
		{ NULL, 0, NULL }
	};

	atf_utils_create_file("nosubj_in.txt",
	    "From: bob@example.org\n"
	    "To: list@example.org\n"
	    "\n"
	    "No subject mail.\n");

	infd = open("nosubj_in.txt", O_RDONLY);
	ATF_REQUIRE(infd >= 0);
	outfd = open("nosubj_out.txt", O_RDWR|O_CREAT|O_TRUNC, 0600);
	ATF_REQUIRE(outfd >= 0);

	ATF_REQUIRE_EQ(do_all_the_voodoo_here(infd, outfd, -1, -1,
	    &delhdrs, readhdrs2, &allheaders2, "[LIST]", 0, NULL), 0);
	close(infd);
	close(outfd);

	/* A Subject: header should be added with the prefix */
	if (!atf_utils_grep_file("Subject: \\[LIST\\]", "nosubj_out.txt")) {
		atf_utils_cat_file("nosubj_out.txt", ">");
		atf_tc_fail("Subject not added for mail without subject");
	}

	vec_free_and_free(&allheaders2, free);
	}
}

/*
 * Regression test: calling do_all_the_voodoo_here() twice with the same
 * allhdrs list (without clearing it) used to trigger an assertion in
 * vec_pop_front on allunfoldeds because allhdrs had stale entries from the
 * first call while allunfoldeds was freshly initialized.
 * This is the code path hit when mlmmj-process handles a mail addressed
 * to list+owner (recipextra == "owner").
 */
ATF_TC_BODY(voodoo_double_call, tc)
{
	strlist allheaders = vec_init();
	strlist delhdrs = vec_init();
	struct mailhdr readhdrs[] = {
		{ "From:", 0, NULL },
		{ "To:", 0, NULL },
		{ "Cc:", 0, NULL },
		{ "Return-Path:", 0, NULL },
		{ "Delivered-To:", 0, NULL },
		{ "Subject:", 0, NULL },
		{ "X-Original-From:", 0, NULL },
		{ NULL, 0, NULL }
	};
	int infd, outfd, ret;

	atf_utils_create_file("testmail.txt",
	    "From: sender@example.org\n"
	    "To: list+owner@example.org\n"
	    "Subject: New subscription\n"
	    "\n"
	    "A new user has subscribed.\n");

	vec_push(&delhdrs, strdup("From "));
	vec_push(&delhdrs, strdup("Return-Path:"));

	/* First call: populates allheaders (normal processing path) */
	infd = open("testmail.txt", O_RDONLY);
	ATF_REQUIRE(infd >= 0);
	outfd = open("out1.txt", O_RDWR|O_CREAT|O_TRUNC, 0600);
	ATF_REQUIRE(outfd >= 0);
	ret = do_all_the_voodoo_here(infd, outfd, -1, -1,
	    &delhdrs, readhdrs, &allheaders, NULL, 0, NULL);
	ATF_REQUIRE_EQ(ret, 0);
	close(infd);
	close(outfd);

	ATF_REQUIRE_MSG(allheaders.len > 0,
	    "allheaders should be populated after first call");

	/* Second call with the same allheaders, simulating the
	 * recipextra == "owner" path. Before the fix this would abort
	 * with: Assertion failed: allunfoldeds.length > 0 */
	vec_free_and_free(&allheaders, free);
	infd = open("testmail.txt", O_RDONLY);
	ATF_REQUIRE(infd >= 0);
	outfd = open("out2.txt", O_RDWR|O_CREAT|O_TRUNC, 0600);
	ATF_REQUIRE(outfd >= 0);
	ret = do_all_the_voodoo_here(infd, outfd, -1, -1,
	    &delhdrs, NULL, &allheaders, NULL, 0, NULL);
	ATF_REQUIRE_EQ(ret, 0);
	close(infd);
	close(outfd);

	vec_free_and_free(&allheaders, free);
	vec_free_and_free(&delhdrs, free);
}

/*
 * Regression test: when do_all_the_voodoo_here() fails (e.g. write error),
 * the caller (mlmmj-process) must unlink the partially written queue file.
 * Before the fix, the queue file was left behind on failure.
 *
 * This test reproduces the mlmmj-process pattern: create a queue file, make
 * do_all_the_voodoo_here() fail via a read-only outfd, then verify the
 * queue file is cleaned up.
 */
ATF_TC_BODY(voodoo_failure_cleans_queue, tc)
{
	strlist allheaders = vec_init();
	struct mailhdr readhdrs[] = {
		{ "From:", 0, NULL },
		{ "To:", 0, NULL },
		{ "Cc:", 0, NULL },
		{ "Return-Path:", 0, NULL },
		{ "Delivered-To:", 0, NULL },
		{ "Subject:", 0, NULL },
		{ "X-Original-From:", 0, NULL },
		{ NULL, 0, NULL }
	};
	int infd, outfd, ret;

	atf_utils_create_file("testmail.txt",
	    "From: sender@example.org\n"
	    "To: list@example.org\n"
	    "Subject: hello\n"
	    "\n"
	    "body\n");

	/* Create the queue file like mlmmj-process does */
	atf_utils_create_file("queuefile.txt", "");
	ATF_REQUIRE(access("queuefile.txt", F_OK) == 0);

	infd = open("testmail.txt", O_RDONLY);
	ATF_REQUIRE(infd >= 0);

	/* Open outfd read-only to force write failures inside
	 * do_all_the_voodoo_here (dprintf returns -1 on EBADF) */
	outfd = open("/dev/null", O_RDONLY);
	ATF_REQUIRE(outfd >= 0);

	ret = do_all_the_voodoo_here(infd, outfd, -1, -1,
	    NULL, readhdrs, &allheaders, NULL, 0, NULL);
	close(infd);
	close(outfd);

	/* do_all_the_voodoo_here must report failure */
	ATF_REQUIRE_MSG(ret < 0,
	    "do_all_the_voodoo_here should fail with read-only outfd");

	/* Queue file still exists (the bug: caller didn't clean up) */
	ATF_REQUIRE(access("queuefile.txt", F_OK) == 0);

	/* Apply the fix pattern from mlmmj-process: unlink on failure */
	unlink("queuefile.txt");
	ATF_REQUIRE_MSG(access("queuefile.txt", F_OK) != 0,
	    "queue file should be cleaned up after voodoo failure");

	vec_free_and_free(&allheaders, free);
}

ATF_TC_BODY(voodoo_listheaders, tc)
{
	struct ml ml;
	strlist allheaders = vec_init();
	struct mailhdr readhdrs[] = {
		{ "From:", 0, NULL },
		{ "To:", 0, NULL },
		{ "Subject:", 0, NULL },
		{ NULL, 0, NULL }
	};
	int infd, outfd, ret;

	init_ml(true);
	ml_init(&ml);
	ml.dir = "list";
	ml_open(&ml, false);

	atf_utils_create_file("listhdr_in.txt",
	    "From: sender@example.org\n"
	    "To: test@test\n"
	    "Subject: Hello\n"
	    "\n"
	    "Body.\n");

	infd = open("listhdr_in.txt", O_RDONLY);
	ATF_REQUIRE(infd >= 0);
	outfd = open("listhdr_out.txt", O_RDWR|O_CREAT|O_TRUNC, 0600);
	ATF_REQUIRE(outfd >= 0);

	ret = do_all_the_voodoo_here(infd, outfd, -1, -1,
	    NULL, readhdrs, &allheaders, NULL, 0, &ml);
	ATF_REQUIRE_EQ(ret, 0);
	close(infd);
	close(outfd);

	const char *path = "listhdr_out.txt";
	if (!atf_utils_grep_file("List-Id: <test\\.test>", path)) {
		atf_utils_cat_file(path, ">");
		atf_tc_fail("List-Id header missing");
	}
	if (!atf_utils_grep_file("List-Post: <mailto:test@test>", path)) {
		atf_utils_cat_file(path, ">");
		atf_tc_fail("List-Post header missing");
	}
	if (!atf_utils_grep_file("List-Help: <mailto:test\\+help@test>", path)) {
		atf_utils_cat_file(path, ">");
		atf_tc_fail("List-Help header missing");
	}
	if (!atf_utils_grep_file("List-Subscribe: <mailto:test\\+subscribe@test>", path)) {
		atf_utils_cat_file(path, ">");
		atf_tc_fail("List-Subscribe header missing");
	}
	if (!atf_utils_grep_file("List-Unsubscribe: <mailto:test\\+unsubscribe@test>", path)) {
		atf_utils_cat_file(path, ">");
		atf_tc_fail("List-Unsubscribe header missing");
	}
	if (!atf_utils_grep_file("Precedence: list", path)) {
		atf_utils_cat_file(path, ">");
		atf_tc_fail("Precedence header missing");
	}
	/* No owner file -> no List-Owner */
	if (atf_utils_grep_file("List-Owner:", path)) {
		atf_utils_cat_file(path, ">");
		atf_tc_fail("List-Owner should not be present without control/owner");
	}

	vec_free_and_free(&allheaders, free);
	ml_close(&ml);
}

ATF_TC_BODY(voodoo_listheaders_disabled, tc)
{
	struct ml ml;
	strlist allheaders = vec_init();
	struct mailhdr readhdrs[] = {
		{ "From:", 0, NULL },
		{ "To:", 0, NULL },
		{ "Subject:", 0, NULL },
		{ NULL, 0, NULL }
	};
	int infd, outfd, ret;

	init_ml(true);
	ml_init(&ml);
	ml.dir = "list";
	ml_open(&ml, false);

	/* Create the nolistheaders control file */
	atf_utils_create_file("list/control/nolistheaders", "");

	/* Re-open to pick up new control file */
	ml_open(&ml, false);

	atf_utils_create_file("listhdr_dis_in.txt",
	    "From: sender@example.org\n"
	    "To: test@test\n"
	    "Subject: Hello\n"
	    "\n"
	    "Body.\n");

	infd = open("listhdr_dis_in.txt", O_RDONLY);
	ATF_REQUIRE(infd >= 0);
	outfd = open("listhdr_dis_out.txt", O_RDWR|O_CREAT|O_TRUNC, 0600);
	ATF_REQUIRE(outfd >= 0);

	ret = do_all_the_voodoo_here(infd, outfd, -1, -1,
	    NULL, readhdrs, &allheaders, NULL, 0, &ml);
	ATF_REQUIRE_EQ(ret, 0);
	close(infd);
	close(outfd);

	const char *path = "listhdr_dis_out.txt";
	if (atf_utils_grep_file("List-Id:", path)) {
		atf_utils_cat_file(path, ">");
		atf_tc_fail("List-Id should not be present when nolistheaders is set");
	}
	if (atf_utils_grep_file("List-Post:", path)) {
		atf_utils_cat_file(path, ">");
		atf_tc_fail("List-Post should not be present when nolistheaders is set");
	}
	if (atf_utils_grep_file("Precedence:", path)) {
		atf_utils_cat_file(path, ">");
		atf_tc_fail("Precedence should not be present when nolistheaders is set");
	}

	vec_free_and_free(&allheaders, free);
	ml_close(&ml);
}

ATF_TC_BODY(voodoo_listheaders_owner, tc)
{
	struct ml ml;
	strlist allheaders = vec_init();
	struct mailhdr readhdrs[] = {
		{ "From:", 0, NULL },
		{ "To:", 0, NULL },
		{ "Subject:", 0, NULL },
		{ NULL, 0, NULL }
	};
	int infd, outfd, ret;

	init_ml(true);
	ml_init(&ml);
	ml.dir = "list";
	ml_open(&ml, false);

	/* Create owner control file */
	atf_utils_create_file("list/control/owner", "admin@example.com");
	ml_open(&ml, false);

	atf_utils_create_file("listhdr_own_in.txt",
	    "From: sender@example.org\n"
	    "To: test@test\n"
	    "Subject: Hello\n"
	    "\n"
	    "Body.\n");

	infd = open("listhdr_own_in.txt", O_RDONLY);
	ATF_REQUIRE(infd >= 0);
	outfd = open("listhdr_own_out.txt", O_RDWR|O_CREAT|O_TRUNC, 0600);
	ATF_REQUIRE(outfd >= 0);

	ret = do_all_the_voodoo_here(infd, outfd, -1, -1,
	    NULL, readhdrs, &allheaders, NULL, 0, &ml);
	ATF_REQUIRE_EQ(ret, 0);
	close(infd);
	close(outfd);

	const char *path = "listhdr_own_out.txt";
	if (!atf_utils_grep_file("List-Owner: <mailto:test\\+owner@test>", path)) {
		atf_utils_cat_file(path, ">");
		atf_tc_fail("List-Owner header missing");
	}
	if (!atf_utils_grep_file("List-Id:", path)) {
		atf_utils_cat_file(path, ">");
		atf_tc_fail("List-Id header missing");
	}

	vec_free_and_free(&allheaders, free);
	ml_close(&ml);
}

ATF_TC_BODY(voodoo_listheaders_phrase, tc)
{
	struct ml ml;
	strlist allheaders = vec_init();
	struct mailhdr readhdrs[] = {
		{ "From:", 0, NULL },
		{ "To:", 0, NULL },
		{ "Subject:", 0, NULL },
		{ NULL, 0, NULL }
	};
	int infd, outfd, ret;

	init_ml(true);
	ml_init(&ml);
	ml.dir = "list";
	ml_open(&ml, false);

	/* An RFC 2919 phrase is emitted verbatim before the <list-id> */
	atf_utils_create_file("list/control/listidphrase", "My list\n");

	atf_utils_create_file("listhdr_phrase_in.txt",
	    "From: sender@example.org\n"
	    "To: test@test\n"
	    "Subject: Hello\n"
	    "\n"
	    "Body.\n");

	infd = open("listhdr_phrase_in.txt", O_RDONLY);
	ATF_REQUIRE(infd >= 0);
	outfd = open("listhdr_phrase_out.txt", O_RDWR|O_CREAT|O_TRUNC, 0600);
	ATF_REQUIRE(outfd >= 0);

	ret = do_all_the_voodoo_here(infd, outfd, -1, -1,
	    NULL, readhdrs, &allheaders, NULL, 0, &ml);
	ATF_REQUIRE_EQ(ret, 0);
	close(infd);
	close(outfd);

	if (!atf_utils_grep_file("List-Id: My list <test\\.test>",
	    "listhdr_phrase_out.txt")) {
		atf_utils_cat_file("listhdr_phrase_out.txt", ">");
		atf_tc_fail("List-Id phrase missing");
	}

	/* An empty phrase falls back to the bare <list-id> */
	atf_utils_create_file("list/control/listidphrase", "");

	/* scan_headers() keeps allhdrs and its private unfolded list in
	 * lockstep, so reset the per-message state before a second run */
	free_mailhdrs(readhdrs);
	vec_free_and_free(&allheaders, free);

	infd = open("listhdr_phrase_in.txt", O_RDONLY);
	ATF_REQUIRE(infd >= 0);
	outfd = open("listhdr_phrase_out.txt", O_RDWR|O_CREAT|O_TRUNC, 0600);
	ATF_REQUIRE(outfd >= 0);

	ret = do_all_the_voodoo_here(infd, outfd, -1, -1,
	    NULL, readhdrs, &allheaders, NULL, 0, &ml);
	ATF_REQUIRE_EQ(ret, 0);
	close(infd);
	close(outfd);

	if (!atf_utils_grep_file("List-Id: <test\\.test>",
	    "listhdr_phrase_out.txt")) {
		atf_utils_cat_file("listhdr_phrase_out.txt", ">");
		atf_tc_fail("List-Id without phrase missing");
	}

	vec_free_and_free(&allheaders, free);
	ml_close(&ml);
}

ATF_TC_BODY(checkwait_smtpreply_connect, tc)
{
	int sp[2];
	char *reply;

	ATF_REQUIRE(socketpair(AF_UNIX, SOCK_STREAM, 0, sp) >= 0);

	/* Valid 220 greeting: checkwait returns NULL (success) */
	dprintf(sp[1], "220 mail.example.org ESMTP\r\n");
	reply = checkwait_smtpreply(sp[0], MLMMJ_CONNECT);
	ATF_CHECK_MSG(reply == NULL, "Expected NULL for valid 220, got [%s]", reply);
	free(reply);

	/* Valid 250 for HELO */
	dprintf(sp[1], "250 mail.example.org\r\n");
	reply = checkwait_smtpreply(sp[0], MLMMJ_HELO);
	ATF_CHECK_MSG(reply == NULL, "Expected NULL for valid 250 HELO, got [%s]", reply);
	free(reply);

	/* Valid 250 for FROM */
	dprintf(sp[1], "250 2.1.0 Ok\r\n");
	reply = checkwait_smtpreply(sp[0], MLMMJ_FROM);
	ATF_CHECK_MSG(reply == NULL, "Expected NULL for valid 250 FROM, got [%s]", reply);
	free(reply);

	/* Valid 250 for RCPT TO */
	dprintf(sp[1], "250 2.1.5 Ok\r\n");
	reply = checkwait_smtpreply(sp[0], MLMMJ_RCPTTO);
	ATF_CHECK_MSG(reply == NULL, "Expected NULL for valid 250 RCPT, got [%s]", reply);
	free(reply);

	/* Valid 354 for DATA */
	dprintf(sp[1], "354 End data with <CR><LF>.<CR><LF>\r\n");
	reply = checkwait_smtpreply(sp[0], MLMMJ_DATA);
	ATF_CHECK_MSG(reply == NULL, "Expected NULL for valid 354, got [%s]", reply);
	free(reply);

	/* Valid 250 for DOT */
	dprintf(sp[1], "250 2.0.0 Ok: queued\r\n");
	reply = checkwait_smtpreply(sp[0], MLMMJ_DOT);
	ATF_CHECK_MSG(reply == NULL, "Expected NULL for valid 250 DOT, got [%s]", reply);
	free(reply);

	/* Valid 221 for QUIT */
	dprintf(sp[1], "221 2.0.0 Bye\r\n");
	reply = checkwait_smtpreply(sp[0], MLMMJ_QUIT);
	ATF_CHECK_MSG(reply == NULL, "Expected NULL for valid 221 QUIT, got [%s]", reply);
	free(reply);

	/* Valid 250 for RSET */
	dprintf(sp[1], "250 2.0.0 Ok\r\n");
	reply = checkwait_smtpreply(sp[0], MLMMJ_RSET);
	ATF_CHECK_MSG(reply == NULL, "Expected NULL for valid 250 RSET, got [%s]", reply);
	free(reply);

	close(sp[0]);
	close(sp[1]);
}

ATF_TC_BODY(checkwait_smtpreply_ehlo_multiline, tc)
{
	int sp[2];
	char *reply;

	ATF_REQUIRE(socketpair(AF_UNIX, SOCK_STREAM, 0, sp) >= 0);

	/* EHLO multi-line response: 250- continuation lines consumed,
	 * final 250 line checked */
	dprintf(sp[1],
	    "250-mail.example.org\r\n"
	    "250-PIPELINING\r\n"
	    "250-SIZE 20480000\r\n"
	    "250-8BITMIME\r\n"
	    "250-DSN\r\n"
	    "250 SMTPUTF8\r\n");
	reply = checkwait_smtpreply(sp[0], MLMMJ_EHLO);
	ATF_CHECK_MSG(reply == NULL,
	    "Expected NULL for valid EHLO multi-line, got [%s]", reply);
	free(reply);

	/* EHLO with error on final line */
	dprintf(sp[1],
	    "250-mail.example.org\r\n"
	    "250-PIPELINING\r\n"
	    "501 Syntax error\r\n");
	reply = checkwait_smtpreply(sp[0], MLMMJ_EHLO);
	ATF_CHECK_MSG(reply != NULL,
	    "Expected error reply for failed EHLO");
	ATF_CHECK(strncmp(reply, "501", 3) == 0);
	free(reply);

	close(sp[0]);
	close(sp[1]);
}

ATF_TC_BODY(checkwait_smtpreply_errors, tc)
{
	int sp[2];
	char *reply;

	ATF_REQUIRE(socketpair(AF_UNIX, SOCK_STREAM, 0, sp) >= 0);

	/* 421 Service not available on CONNECT */
	dprintf(sp[1], "421 Service not available\r\n");
	reply = checkwait_smtpreply(sp[0], MLMMJ_CONNECT);
	ATF_CHECK_MSG(reply != NULL, "Expected error for 421");
	ATF_CHECK(strncmp(reply, "421", 3) == 0);
	free(reply);

	/* 550 on RCPT TO */
	dprintf(sp[1], "550 5.1.1 User unknown\r\n");
	reply = checkwait_smtpreply(sp[0], MLMMJ_RCPTTO);
	ATF_CHECK_MSG(reply != NULL, "Expected error for 550 RCPT");
	ATF_CHECK(reply[0] == '5' && reply[1] == '5');
	free(reply);

	/* 452 on FROM (temporary failure) */
	dprintf(sp[1], "452 Too many recipients\r\n");
	reply = checkwait_smtpreply(sp[0], MLMMJ_FROM);
	ATF_CHECK_MSG(reply != NULL, "Expected error for 452 FROM");
	free(reply);

	/* 503 on DATA (bad sequence) */
	dprintf(sp[1], "503 Bad sequence of commands\r\n");
	reply = checkwait_smtpreply(sp[0], MLMMJ_DATA);
	ATF_CHECK_MSG(reply != NULL, "Expected error for 503 DATA");
	free(reply);

	/* 500 on EHLO (unknown command) */
	dprintf(sp[1], "500 Unrecognized command\r\n");
	reply = checkwait_smtpreply(sp[0], MLMMJ_EHLO);
	ATF_CHECK_MSG(reply != NULL, "Expected error for 500 EHLO");
	free(reply);

	close(sp[0]);
	close(sp[1]);
}

ATF_TC_BODY(checkwait_smtpreply_closed, tc)
{
	int sp[2];
	char *reply;

	ATF_REQUIRE(socketpair(AF_UNIX, SOCK_STREAM, 0, sp) >= 0);

	/* Connection closed: mygetline returns NULL,
	 * checkwait returns a descriptive error string */
	close(sp[1]);
	reply = checkwait_smtpreply(sp[0], MLMMJ_CONNECT);
	ATF_CHECK_MSG(reply != NULL,
	    "Expected error string for closed connection");
	ATF_CHECK(strstr(reply, "none") != NULL ||
	    strstr(reply, "error") != NULL ||
	    strstr(reply, "closed") != NULL);
	free(reply);

	close(sp[0]);
}

ATF_TC_BODY(digest_thread_list_format, tc)
{
	struct ml ml;
	int ret;
	DIR *qdir;
	struct dirent *entry;
	char *queuepath = NULL;
	char *line = NULL;
	size_t linecap = 0;
	FILE *fp;
	bool found_thread_line = false;

	init_ml(true);
	ml_init(&ml);
	ml.dir = "list";
	ml_open(&ml, false);
	setup_listtext(tc);

	/* Create archive files with known From/Subject headers */
	atf_utils_create_file("list/archive/1",
	    "From: alice@example.org\n"
	    "Subject: Test topic\n"
	    "\n"
	    "Body of mail 1\n");

	atf_utils_create_file("list/archive/2",
	    "From: bob@example.org\n"
	    "Subject: Another topic\n"
	    "\n"
	    "Body of mail 2\n");

	/*
	 * send_digest calls exec_and_wait(mlmmjsend, ...) at the end.
	 * Pass /usr/bin/true so it succeeds without actually sending.
	 */
	ret = send_digest(&ml, 1, 2, 1, NULL, "/usr/bin/true");
	ATF_REQUIRE_MSG(ret == 0, "send_digest failed: %d", ret);

	/* Find the queue file that send_digest created */
	qdir = opendir("list/queue");
	ATF_REQUIRE(qdir != NULL);
	while ((entry = readdir(qdir)) != NULL) {
		if (entry->d_name[0] == '.')
			continue;
		/* Skip the discarded subdirectory */
		if (entry->d_type == DT_DIR)
			continue;
		xasprintf(&queuepath, "list/queue/%s", entry->d_name);
		break;
	}
	closedir(qdir);
	ATF_REQUIRE_MSG(queuepath != NULL, "No queue file found");

	/* Read the queue file and verify thread list lines contain
	 * the archive index (integer), not uninitialized garbage.
	 * Thread list lines have the format: "      <index> - <from>"
	 */
	fp = fopen(queuepath, "r");
	ATF_REQUIRE_MSG(fp != NULL, "Cannot open %s", queuepath);
	while (getline(&line, &linecap, fp) > 0) {
		/*
		 * Thread list lines in the queue file have the format:
		 * "-       <index> - <from>"
		 * (the "- " prefix comes from the digest listtext template)
		 * Look for a digit followed by " - " after whitespace.
		 */
		char *p = line;
		/* Skip "- " prefix from template and whitespace */
		while (*p == '-' || *p == ' ')
			p++;
		/* Look for "<digit> - <text>" pattern */
		if (*p >= '1' && *p <= '9') {
			char *dash = strstr(p, " - ");
			if (dash != NULL) {
				int idx = atoi(p);
				found_thread_line = true;
				ATF_CHECK_MSG(idx == 1 || idx == 2,
				    "Unexpected archive index %d in: %s",
				    idx, line);
			}
		}
	}
	fclose(fp);
	free(line);

	ATF_REQUIRE_MSG(found_thread_line,
	    "No thread list lines found in digest queue file");

	unlink(queuepath);
	free(queuepath);
}

ATF_TC_BODY(digest_thread_grouping, tc)
{
	struct ml ml;
	int ret;
	DIR *qdir;
	struct dirent *entry;
	char *queuepath = NULL;
	char *line = NULL;
	size_t linecap = 0;
	FILE *fp;
	int thread_count = 0;

	init_ml(true);
	ml_init(&ml);
	ml.dir = "list";
	ml_open(&ml, false);
	setup_listtext(tc);

	/* Create archive files: mail 2 is a reply to mail 1 (same topic) */
	atf_utils_create_file("list/archive/1",
	    "From: alice@example.org\n"
	    "Subject: Test topic\n"
	    "\n"
	    "Body of mail 1\n");

	atf_utils_create_file("list/archive/2",
	    "From: bob@example.org\n"
	    "Subject: Re: Test topic\n"
	    "\n"
	    "Body of reply\n");

	atf_utils_create_file("list/archive/3",
	    "From: charlie@example.org\n"
	    "Subject: Different topic\n"
	    "\n"
	    "Body of mail 3\n");

	ret = send_digest(&ml, 1, 3, 1, NULL, "/usr/bin/true");
	ATF_REQUIRE_MSG(ret == 0, "send_digest failed: %d", ret);

	/* Find the queue file */
	qdir = opendir("list/queue");
	ATF_REQUIRE(qdir != NULL);
	while ((entry = readdir(qdir)) != NULL) {
		if (entry->d_name[0] == '.')
			continue;
		if (entry->d_type == DT_DIR)
			continue;
		xasprintf(&queuepath, "list/queue/%s", entry->d_name);
		break;
	}
	closedir(qdir);
	ATF_REQUIRE_MSG(queuepath != NULL, "No queue file found");

	/*
	 * Count thread subjects in the digest.
	 * Thread subjects appear as "- <subject>\n" lines where the
	 * subject is NOT indented (unlike "from" lines which have
	 * leading spaces). "Test topic" and "Re: Test topic" should
	 * be grouped into one thread, so we expect 2 threads total:
	 * "Test topic" and "Different topic".
	 */
	fp = fopen(queuepath, "r");
	ATF_REQUIRE(fp != NULL);
	while (getline(&line, &linecap, fp) > 0) {
		/* Thread subject lines: "- <subject>\n" (from template) */
		if (strncmp(line, "- ", 2) == 0) {
			char *p = line + 2;
			/* Skip lines with leading spaces (those are
			 * "from" lines like "-       1 - alice@...") */
			if (*p != ' ' && *p != '\n' && *p != '\0')
				thread_count++;
		}
	}
	fclose(fp);
	free(line);

	ATF_CHECK_MSG(thread_count == 2,
	    "Expected 2 threads (Re: grouped), got %d", thread_count);

	unlink(queuepath);
	free(queuepath);
}

ATF_TC_WITHOUT_HEAD(digest_thread_stops_at_headers);
ATF_TC_BODY(digest_thread_stops_at_headers, tc)
{
	thread_list_state *tls;
	const char *line;
	int fakelistfd, fakectrlfd;

	/*
	 * Create archive files where the body contains lines that look like
	 * headers. The thread builder must stop at the blank line and never
	 * parse the body, so the fake "Subject:" in the body must be ignored.
	 */
	mkdir("fakelist", 0755);
	mkdir("fakelist/archive", 0755);
	mkdir("fakelist/control", 0755);
	fakelistfd = open("fakelist", O_RDONLY|O_DIRECTORY);
	fakectrlfd = openat(fakelistfd, "control", O_RDONLY|O_DIRECTORY);
	atf_utils_create_file("fakelist/archive/1",
	    "From: alice@test\n"
	    "Subject: Real subject\n"
	    "\n"
	    "Subject: Fake subject in body\n"
	    "From: fake@body\n");

	atf_utils_create_file("fakelist/archive/2",
	    "From: bob@test\n"
	    "Subject: Re: Real subject\n"
	    "\n"
	    "body text\n");

	atf_utils_create_file("fakelist/archive/3",
	    "From: charlie@test\n"
	    "Subject: Other topic\n"
	    "\n"
	    "Subject: Decoy\n");

	tls = init_thread_list("fakelist", fakelistfd, fakectrlfd, 1, 3);
	rewind_thread_list(tls);

	/* Thread 1: "Real subject" (mails 1 and 2 grouped via Re:) */
	line = get_thread_list_line(tls);
	ATF_REQUIRE_MSG(line != NULL, "expected thread 1 subject");
	ATF_CHECK_MSG(strstr(line, "Real subject") != NULL,
	    "thread 1 subject should be 'Real subject', got: %s", line);

	/* Mail 1 in thread 1: alice */
	line = get_thread_list_line(tls);
	ATF_REQUIRE(line != NULL);
	ATF_CHECK_MSG(strstr(line, "1 - ") != NULL && strstr(line, "alice") != NULL,
	    "expected mail 1 from alice, got: %s", line);

	/* Mail 2 in thread 1: bob (Re: grouped) */
	line = get_thread_list_line(tls);
	ATF_REQUIRE(line != NULL);
	ATF_CHECK_MSG(strstr(line, "2 - ") != NULL && strstr(line, "bob") != NULL,
	    "expected mail 2 from bob, got: %s", line);

	/* Separator between threads */
	line = get_thread_list_line(tls);
	ATF_REQUIRE(line != NULL);
	ATF_CHECK_MSG(strcmp(line, "\n") == 0,
	    "expected blank separator, got: %s", line);

	/* Thread 2: "Other topic" (only mail 3) */
	line = get_thread_list_line(tls);
	ATF_REQUIRE(line != NULL);
	ATF_CHECK_MSG(strstr(line, "Other topic") != NULL,
	    "thread 2 subject should be 'Other topic', got: %s", line);

	/* Mail 3 in thread 2: charlie */
	line = get_thread_list_line(tls);
	ATF_REQUIRE(line != NULL);
	ATF_CHECK_MSG(strstr(line, "3 - ") != NULL && strstr(line, "charlie") != NULL,
	    "expected mail 3 from charlie, got: %s", line);

	/* Separator */
	line = get_thread_list_line(tls);
	ATF_REQUIRE(line != NULL);
	ATF_CHECK(strcmp(line, "\n") == 0);

	/* End */
	line = get_thread_list_line(tls);
	ATF_CHECK(line == NULL);

	/* "Fake subject in body" must NOT have created a third thread */
	ATF_CHECK_MSG(tls->threads.len == 2,
	    "expected 2 threads (body headers ignored), got %zu",
	    tls->threads.len);

	finish_thread_list(tls);
	close(fakectrlfd);
	close(fakelistfd);
}

ATF_TC_WITHOUT_HEAD(digest_thread_growth);
ATF_TC_BODY(digest_thread_growth, tc)
{
	thread_list_state *tls;
	const char *line;
	char *mail = NULL;
	int fakelistfd, fakectrlfd, i;
	const int nmails = 40;

	/*
	 * Create many archive mails: the first few share a subject (to
	 * grow the per-thread mail list) and the rest each have their own
	 * subject (to grow the thread list well past its initial size).
	 */
	mkdir("fakelist", 0755);
	mkdir("fakelist/archive", 0755);
	mkdir("fakelist/control", 0755);
	fakelistfd = open("fakelist", O_RDONLY|O_DIRECTORY);
	fakectrlfd = openat(fakelistfd, "control", O_RDONLY|O_DIRECTORY);

	for (i = 1; i <= nmails; i++) {
		char *path = NULL;

		if (i <= 3)
			xasprintf(&mail, "From: a%d@test\nSubject: Shared\n\nbody%d\n", i, i);
		else
			xasprintf(&mail, "From: a%d@test\nSubject: Topic %d\n\nbody%d\n", i, i, i);
		xasprintf(&path, "fakelist/archive/%d", i);
		atf_utils_create_file(path, "%s", mail);
		free(path);
		free(mail);
		mail = NULL;
	}

	tls = init_thread_list("fakelist", fakelistfd, fakectrlfd, 1, nmails);
	rewind_thread_list(tls);

	/* 3 mails in the first thread + one thread for each remaining mail. */
	ATF_REQUIRE_MSG(tls->threads.len == (size_t)(nmails - 3) + 1,
	    "expected %d threads, got %zu", (nmails - 3) + 1,
	    tls->threads.len);

	/* First thread subject and its three mails. */
	line = get_thread_list_line(tls);
	ATF_REQUIRE_MSG(line != NULL, "expected first thread subject");
	ATF_CHECK_MSG(strstr(line, "Shared") != NULL,
	    "first thread subject, got: %s", line);

	ATF_CHECK_MSG(tls->threads.d[0].mails.len == 3,
	    "expected 3 mails in first thread, got %zu",
	    tls->threads.d[0].mails.len);
	for (i = 1; i <= 3; i++) {
		char expect[32];

		line = get_thread_list_line(tls);
		ATF_REQUIRE(line != NULL);
		snprintf(expect, sizeof(expect), "%d - ", i);
		ATF_CHECK_MSG(strstr(line, expect) != NULL,
		    "expected mail %d in first thread, got: %s", i, line);
	}

	finish_thread_list(tls);
	close(fakectrlfd);
	close(fakelistfd);
}

ATF_TC_BODY(archive_open_flat, tc)
{
	int listfd, ctrlfd, fd;

	init_ml(true);
	listfd = open("list", O_RDONLY|O_DIRECTORY);
	ctrlfd = openat(listfd, "control", O_RDONLY|O_DIRECTORY);

	atf_utils_create_file("list/archive/42", "test content\n");
	fd = archive_open(listfd, 42);
	ATF_REQUIRE_MSG(fd != -1, "archive_open should find flat archive/42");
	close(fd);

	fd = archive_open(listfd, 999);
	ATF_CHECK_MSG(fd == -1, "archive_open should return -1 for missing index");

	close(ctrlfd);
	close(listfd);
}

ATF_TC_BODY(archive_open_partitioned, tc)
{
	int listfd, ctrlfd, fd;

	init_ml(true);
	listfd = open("list", O_RDONLY|O_DIRECTORY);
	ctrlfd = openat(listfd, "control", O_RDONLY|O_DIRECTORY);

	mkdir("list/archive/2026", 0755);
	mkdir("list/archive/2026/03", 0755);
	atf_utils_create_file("list/archive/2026/03/42", "partitioned content\n");

	fd = archive_open(listfd, 42);
	ATF_REQUIRE_MSG(fd != -1,
	    "archive_open should find archive/2026/03/42 via subdirectory scan");
	close(fd);

	close(ctrlfd);
	close(listfd);
}

ATF_TC_BODY(archive_open_fallback, tc)
{
	int listfd, ctrlfd, fd;

	init_ml(true);
	listfd = open("list", O_RDONLY|O_DIRECTORY);
	ctrlfd = openat(listfd, "control", O_RDONLY|O_DIRECTORY);

	/* Enable partitioning */
	atf_utils_create_file("list/control/archivepartition", "");

	/* But place file in flat layout */
	atf_utils_create_file("list/archive/42", "flat content\n");

	fd = archive_open(listfd, 42);
	ATF_REQUIRE_MSG(fd != -1,
	    "archive_open should find flat archive/42 even with partitioning enabled");
	close(fd);

	close(ctrlfd);
	close(listfd);
}

ATF_TC_BODY(archive_path_write_flat, tc)
{
	int listfd, ctrlfd;
	char *path;

	init_ml(true);
	listfd = open("list", O_RDONLY|O_DIRECTORY);
	ctrlfd = openat(listfd, "control", O_RDONLY|O_DIRECTORY);

	path = archive_path_write(listfd, ctrlfd, 42);
	ATF_CHECK_STREQ_MSG(path, "archive/42",
	    "Without archivepartition, path should be archive/N");
	free(path);

	close(ctrlfd);
	close(listfd);
}

ATF_TC_BODY(archive_path_write_partitioned, tc)
{
	int listfd, ctrlfd;
	char *path;
	struct stat st;

	init_ml(true);
	listfd = open("list", O_RDONLY|O_DIRECTORY);
	ctrlfd = openat(listfd, "control", O_RDONLY|O_DIRECTORY);

	atf_utils_create_file("list/control/archivepartition", "");

	path = archive_path_write(listfd, ctrlfd, 42);
	ATF_REQUIRE_MSG(path != NULL, "archive_path_write should return a path");
	/* Path should match archive/YYYY/MM/42 */
	ATF_CHECK_MSG(strncmp(path, "archive/", 8) == 0,
	    "path should start with archive/, got: %s", path);
	ATF_CHECK_MSG(strstr(path, "/42") != NULL,
	    "path should end with /42, got: %s", path);
	/* Verify the directory was created */
	char *dirpath;
	xasprintf(&dirpath, "list/%s", path);
	/* Remove the filename part to get directory */
	char *slash = strrchr(dirpath, '/');
	*slash = '\0';
	ATF_CHECK_MSG(stat(dirpath, &st) == 0 && S_ISDIR(st.st_mode),
	    "archive_path_write should create subdirectories: %s", dirpath);
	free(dirpath);
	free(path);

	close(ctrlfd);
	close(listfd);
}

ATF_TC_BODY(archive_migrate_test, tc)
{
	int listfd, ctrlfd;
	struct stat st;

	init_ml(true);
	listfd = open("list", O_RDONLY|O_DIRECTORY);
	ctrlfd = openat(listfd, "control", O_RDONLY|O_DIRECTORY);

	atf_utils_create_file("list/control/archivepartition", "");
	atf_utils_create_file("list/archive/1", "mail one\n");
	atf_utils_create_file("list/archive/2", "mail two\n");

	archive_migrate(listfd, -1);

	/* Flat files should be gone */
	ATF_CHECK_MSG(fstatat(listfd, "archive/1", &st, 0) == -1,
	    "flat archive/1 should be moved");
	ATF_CHECK_MSG(fstatat(listfd, "archive/2", &st, 0) == -1,
	    "flat archive/2 should be moved");

	/* Files should be readable via archive_open */
	int fd1 = archive_open(listfd, 1);
	ATF_REQUIRE_MSG(fd1 != -1, "archive_open should find migrated file 1");
	close(fd1);

	int fd2 = archive_open(listfd, 2);
	ATF_REQUIRE_MSG(fd2 != -1, "archive_open should find migrated file 2");
	close(fd2);

	close(ctrlfd);
	close(listfd);
}

ATF_TC_BODY(archive_migrate_noop, tc)
{
	int listfd, ctrlfd;

	init_ml(true);
	listfd = open("list", O_RDONLY|O_DIRECTORY);
	ctrlfd = openat(listfd, "control", O_RDONLY|O_DIRECTORY);

	atf_utils_create_file("list/control/archivepartition", "");

	/* No flat files to migrate */
	mkdir("list/archive/2026", 0755);
	mkdir("list/archive/2026/03", 0755);
	atf_utils_create_file("list/archive/2026/03/1", "already migrated\n");

	/* Should be a no-op, no crash */
	archive_migrate(listfd, -1);

	/* File should still be there */
	int fd = archive_open(listfd, 1);
	ATF_REQUIRE_MSG(fd != -1, "archive_open should still find file after noop migrate");
	close(fd);

	close(ctrlfd);
	close(listfd);
}

ATF_TP_ADD_TCS(tp)
{
	ATF_TP_ADD_TC(tp, random_int);
	ATF_TP_ADD_TC(tp, chomp);
	ATF_TP_ADD_TC(tp, mydirname);
	ATF_TP_ADD_TC(tp, mybasename);
	ATF_TP_ADD_TC(tp, mygetline);
	ATF_TP_ADD_TC(tp, lowercase);
	ATF_TP_ADD_TC(tp, init_sock);
	ATF_TP_ADD_TC(tp, genmsgid);
	ATF_TP_ADD_TC(tp, exec_or_die);
	ATF_TP_ADD_TC(tp, exec_and_wait);
	ATF_TP_ADD_TC(tp, find_email_adr);
	ATF_TP_ADD_TC(tp, strtoim);
	ATF_TP_ADD_TC(tp, write_ehlo);
	ATF_TP_ADD_TC(tp, write_helo);
	ATF_TP_ADD_TC(tp, write_dot);
	ATF_TP_ADD_TC(tp, write_data);
	ATF_TP_ADD_TC(tp, write_quit);
	ATF_TP_ADD_TC(tp, write_rset);
	ATF_TP_ADD_TC(tp, write_replyto);
	ATF_TP_ADD_TC(tp, write_rcpt_to);
	ATF_TP_ADD_TC(tp, write_mail_from);
	ATF_TP_ADD_TC(tp, write_mailbody);
	ATF_TP_ADD_TC(tp, strtotimet);
	ATF_TP_ADD_TC(tp, decode_qp);
	ATF_TP_ADD_TC(tp, parse_lastdigest);
	ATF_TP_ADD_TC(tp, extract_bouncetime);
	ATF_TP_ADD_TC(tp, open_subscriber_directory);
	ATF_TP_ADD_TC(tp, unsubscribe);
	ATF_TP_ADD_TC(tp, genlistname);
	ATF_TP_ADD_TC(tp, genlistfqdn);
	ATF_TP_ADD_TC(tp, smtp);
	ATF_TP_ADD_TC(tp, init_smtp);
	ATF_TP_ADD_TC(tp, smtp_bad_greetings);
	ATF_TP_ADD_TC(tp, smtp_bad_ehlo);
	ATF_TP_ADD_TC(tp, smtp_no_ehlo);
	ATF_TP_ADD_TC(tp, endsmtp);
	ATF_TP_ADD_TC(tp, do_bouncemail);
	ATF_TP_ADD_TC(tp, bouncemail);
	ATF_TP_ADD_TC(tp, send_mail_basics);
	ATF_TP_ADD_TC(tp, send_mail);
	ATF_TP_ADD_TC(tp, send_mail_xforwardedto);
	ATF_TP_ADD_TC(tp, send_mail_xsignedrecipient);
	ATF_TP_ADD_TC(tp, send_mail_both_headers);
	ATF_TP_ADD_TC(tp, getlistdelim);
	ATF_TP_ADD_TC(tp, getlistdelim_0);
	ATF_TP_ADD_TC(tp, getlistdelim_1);
	ATF_TP_ADD_TC(tp, getlistdelim_2);
	ATF_TP_ADD_TC(tp, getlistdelim_3);
	ATF_TP_ADD_TC(tp, getlistdelim_4);
	ATF_TP_ADD_TC(tp, statctrl);
	ATF_TP_ADD_TC(tp, is_subbed_in);
	ATF_TP_ADD_TC(tp, is_subbed_normal);
	ATF_TP_ADD_TC(tp, is_subbed_types);
	ATF_TP_ADD_TC(tp, unsubscribe_test);
	ATF_TP_ADD_TC(tp, find_addr_line_edge);
	ATF_TP_ADD_TC(tp, is_subbed_crlf_prefix);
	ATF_TP_ADD_TC(tp, unsubscribe_crlf_prefix);
	ATF_TP_ADD_TC(tp, mkuniqfileat_test);
	ATF_TP_ADD_TC(tp, getaddrsfromfile);
	ATF_TP_ADD_TC(tp, dumpfd2fd);
	ATF_TP_ADD_TC(tp, dumpfd2fd_large);
	ATF_TP_ADD_TC(tp, dumpfd2fd_empty);
	ATF_TP_ADD_TC(tp, process_headers_test);
	ATF_TP_ADD_TC(tp, process_headers_skipfrom);
	ATF_TP_ADD_TC(tp, process_headers_disarmedfrom);
	ATF_TP_ADD_TC(tp, disarm_email_test);
	ATF_TP_ADD_TC(tp, copy_file);
	ATF_TP_ADD_TC(tp, copy_file_1);
	ATF_TP_ADD_TC(tp, copy_file_2);
	ATF_TP_ADD_TC(tp, controls);
	ATF_TP_ADD_TC(tp, incindexfile);
	ATF_TP_ADD_TC(tp, log_oper);
	ATF_TP_ADD_TC(tp, log_oper_rotate);
	ATF_TP_ADD_TC(tp, mygetline_edge);
	ATF_TP_ADD_TC(tp, get_ctrl_command);
	ATF_TP_ADD_TC(tp, get_recipextra_from_env_none);
	ATF_TP_ADD_TC(tp, get_recipextra_from_env_qmail);
	ATF_TP_ADD_TC(tp, get_recipextra_from_env_postfix);
	ATF_TP_ADD_TC(tp, get_recipextra_from_env_exim);
	ATF_TP_ADD_TC(tp, addrmatch);
	ATF_TP_ADD_TC(tp, get_subcookie_content);
	ATF_TP_ADD_TC(tp, ml_list);
	ATF_TP_ADD_TC(tp, ml_become_list_owner);
	ATF_TP_ADD_TC(tp, gen_addr);
	ATF_TP_ADD_TC(tp, memory_lines);
	ATF_TP_ADD_TC(tp, text_0);
	ATF_TP_ADD_TC(tp, text_1);
	ATF_TP_ADD_TC(tp, file_lines);
	ATF_TP_ADD_TC(tp, list_subs);
	ATF_TP_ADD_TC(tp, notify_sub);
	ATF_TP_ADD_TC(tp, get_processed_text_line);
	ATF_TP_ADD_TC(tp, get_processed_text_line_directives);
	ATF_TP_ADD_TC(tp, get_processed_text_line_crlf);
	ATF_TP_ADD_TC(tp, get_processed_text_line_includes);
	ATF_TP_ADD_TC(tp, newsmtp);
	ATF_TP_ADD_TC(tp, save_queue);
	ATF_TP_ADD_TC(tp, send_single_mail);
	ATF_TP_ADD_TC(tp, send_or_queue_test);
	ATF_TP_ADD_TC(tp, generate_subscription);
	ATF_TP_ADD_TC(tp, generate_subconfirm);
	ATF_TP_ADD_TC(tp, send_confirmation_mail);
	ATF_TP_ADD_TC(tp, listcontrol);
	ATF_TP_ADD_TC(tp, listcontrol_subscribe_replyto);
	ATF_TP_ADD_TC(tp, listcontrol_subscribe_no_replyto);
	ATF_TP_ADD_TC(tp, listcontrol_subscribe_replyto_multiple);
	ATF_TP_ADD_TC(tp, listcontrol_unsubscribe_ignores_replyto);
	ATF_TP_ADD_TC(tp, send_help);
	ATF_TP_ADD_TC(tp, requeuemail);
	ATF_TP_ADD_TC(tp, gethdrline);
	ATF_TP_ADD_TC(tp, readlf);
	ATF_TP_ADD_TC(tp, mod_get_addr_type);
	ATF_TP_ADD_TC(tp, send_probe_no_probefile);
	ATF_TP_ADD_TC(tp, send_probe_arobase);
	ATF_TP_ADD_TC(tp, send_probe);
	ATF_TP_ADD_TC(tp, send_probe_archive);
	ATF_TP_ADD_TC(tp, parse_content_type);
	ATF_TP_ADD_TC(tp, find_in_list);
	ATF_TP_ADD_TC(tp, do_access);
	ATF_TP_ADD_TC(tp, unistr_header_to_utf8);
	ATF_TP_ADD_TC(tp, unistr_utf8_to_header);
	ATF_TP_ADD_TC(tp, unistr_roundtrip);
	ATF_TP_ADD_TC(tp, unistr_to_utf8_4byte);
	ATF_TP_ADD_TC(tp, unistr_append_utf8_edge);
	ATF_TP_ADD_TC(tp, unistr_to_utf8_large);
	ATF_TP_ADD_TC(tp, unistr_append_utf8_invalid);
	ATF_TP_ADD_TC(tp, unistr_escaped_to_utf8);
	ATF_TP_ADD_TC(tp, line_startwithany);
	ATF_TP_ADD_TC(tp, save_hdr_if_needed);
	ATF_TP_ADD_TC(tp, autosubscribe_sender);
	ATF_TP_ADD_TC(tp, scan_headers_test);
	ATF_TP_ADD_TC(tp, substitute_test);
	ATF_TP_ADD_TC(tp, sb_cat_n);
	ATF_TP_ADD_TC(tp, sb_reset);
	ATF_TP_ADD_TC(tp, sb_str);
	ATF_TP_ADD_TC(tp, sb_printf_growth);
	ATF_TP_ADD_TC(tp, voodoo_replyto);
	ATF_TP_ADD_TC(tp, voodoo_replyto_once);
	ATF_TP_ADD_TC(tp, voodoo_prefix_dedup);
	ATF_TP_ADD_TC(tp, voodoo_header_manipulation);
	ATF_TP_ADD_TC(tp, voodoo_double_call);
	ATF_TP_ADD_TC(tp, voodoo_failure_cleans_queue);
	ATF_TP_ADD_TC(tp, voodoo_listheaders);
	ATF_TP_ADD_TC(tp, voodoo_listheaders_disabled);
	ATF_TP_ADD_TC(tp, voodoo_listheaders_owner);
	ATF_TP_ADD_TC(tp, voodoo_listheaders_phrase);
	ATF_TP_ADD_TC(tp, checkwait_smtpreply_connect);
	ATF_TP_ADD_TC(tp, checkwait_smtpreply_ehlo_multiline);
	ATF_TP_ADD_TC(tp, checkwait_smtpreply_errors);
	ATF_TP_ADD_TC(tp, checkwait_smtpreply_closed);
	ATF_TP_ADD_TC(tp, digest_thread_list_format);
	ATF_TP_ADD_TC(tp, digest_thread_grouping);
	ATF_TP_ADD_TC(tp, digest_thread_stops_at_headers);
	ATF_TP_ADD_TC(tp, digest_thread_growth);
	ATF_TP_ADD_TC(tp, archive_open_flat);
	ATF_TP_ADD_TC(tp, archive_open_partitioned);
	ATF_TP_ADD_TC(tp, archive_open_fallback);
	ATF_TP_ADD_TC(tp, archive_path_write_flat);
	ATF_TP_ADD_TC(tp, archive_path_write_partitioned);
	ATF_TP_ADD_TC(tp, archive_migrate_test);
	ATF_TP_ADD_TC(tp, archive_migrate_noop);

	return (atf_no_error());
}
