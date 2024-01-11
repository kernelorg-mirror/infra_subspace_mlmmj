/*
 * Copyright (C) 2022-2023 Baptiste Daroussin <bapt@FreeBSD.org>
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

#include "tllist.h"
#pragma clang diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"

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
#include <errno.h>

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
ATF_TC_WITHOUT_HEAD(getlistdelim);
ATF_TC_WITHOUT_HEAD(getlistdelim_0);
ATF_TC_WITHOUT_HEAD(getlistdelim_1);
ATF_TC_WITHOUT_HEAD(getlistdelim_2);
ATF_TC_WITHOUT_HEAD(getlistdelim_3);
ATF_TC_WITHOUT_HEAD(getlistdelim_4);
ATF_TC_WITHOUT_HEAD(statctrl);
ATF_TC_WITHOUT_HEAD(is_subbed_in);
ATF_TC_WITHOUT_HEAD(getaddrsfromfile);
ATF_TC_WITHOUT_HEAD(dumpfd2fd);
ATF_TC_WITHOUT_HEAD(copy_file);
ATF_TC_WITHOUT_HEAD(copy_file_1);
ATF_TC_WITHOUT_HEAD(copy_file_2);
ATF_TC_WITHOUT_HEAD(controls);
ATF_TC_WITHOUT_HEAD(incindexfile);
ATF_TC_WITHOUT_HEAD(log_oper);
ATF_TC_WITHOUT_HEAD(get_ctrl_command);
ATF_TC_WITHOUT_HEAD(get_recipextra_from_env_none);
ATF_TC_WITHOUT_HEAD(get_recipextra_from_env_qmail);
ATF_TC_WITHOUT_HEAD(get_recipextra_from_env_postfix);
ATF_TC_WITHOUT_HEAD(get_recipextra_from_env_exim);
ATF_TC_WITHOUT_HEAD(addrmatch);
ATF_TC_WITHOUT_HEAD(get_subcookie_content);
ATF_TC_WITHOUT_HEAD(ml_list);
ATF_TC_WITHOUT_HEAD(gen_addr);
ATF_TC_WITHOUT_HEAD(memory_lines);
ATF_TC_WITHOUT_HEAD(text_0);
ATF_TC_WITHOUT_HEAD(text_1);
ATF_TC_WITHOUT_HEAD(file_lines);
ATF_TC_WITHOUT_HEAD(list_subs);
ATF_TC_WITHOUT_HEAD(notify_sub);
ATF_TC_WITHOUT_HEAD(get_processed_text_line);
ATF_TC_WITHOUT_HEAD(newsmtp);
ATF_TC_WITHOUT_HEAD(save_queue);
ATF_TC_WITHOUT_HEAD(send_single_mail);
ATF_TC_WITHOUT_HEAD(generate_subscription);
ATF_TC_WITHOUT_HEAD(generate_subconfirm);
ATF_TC_WITHOUT_HEAD(send_confirmation_mail);
ATF_TC_WITHOUT_HEAD(listcontrol);
ATF_TC_WITHOUT_HEAD(send_help);
ATF_TC_WITHOUT_HEAD(requeuemail);
ATF_TC_WITHOUT_HEAD(gethdrline);
ATF_TC_WITHOUT_HEAD(readlf);
ATF_TC_WITHOUT_HEAD(mod_get_addr_type);
ATF_TC_WITHOUT_HEAD(send_probe);

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
	ATF_REQUIRE_STREQ(lowercase("MAR23anC"), "mar23anc");
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
		socklen_t clsize = 0;
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
}

ATF_TC_BODY(exec_and_wait, tc)
{
	int val = exec_and_wait("/usr/bin/nopenope", NULL);
	ATF_REQUIRE_EQ(val, -1);
	val = exec_and_wait("true", NULL);
	ATF_REQUIRE_EQ(val, 0);
	val = exec_and_wait("false", NULL);
	ATF_REQUIRE_EQ(val, 1);
}

ATF_TC_BODY(find_email_adr, tc)
{
	strlist c  = tll_init();
	strlist c1 = tll_init();
	strlist c2 = tll_init();
	strlist c3 = tll_init();
	strlist c4 = tll_init();
	strlist c5 = tll_init();
	strlist c6 = tll_init();
	strlist c7 = tll_init();
	find_email_adr("test@foo.bar", &c);
	ATF_REQUIRE_EQ(tll_length(c), 1);
	find_email_adr("test@foo.bar, meh@bar", &c1);
	ATF_REQUIRE_EQ(tll_length(c1), 2);
	find_email_adr("test@foo.bar, meh@bar, nope", &c2);
	ATF_REQUIRE_EQ(tll_length(c2), 3);
	find_email_adr("name <test@foo.com>", &c3);
	ATF_REQUIRE_EQ(tll_length(c3), 1);
	ATF_REQUIRE_STREQ(tll_front(c3), "test@foo.com");
	find_email_adr("(name <test@foo.com>) test@foo.com", &c4);
	ATF_REQUIRE_EQ(tll_length(c4), 1);
	ATF_REQUIRE_STREQ(tll_front(c4), " test@foo.com");
	find_email_adr("\"name <test@foo.com>\" test@foo.com", &c5);
	ATF_REQUIRE_EQ(tll_length(c5), 1);
	ATF_REQUIRE_STREQ(tll_front(c5), "name <test@foo.com> test@foo.com");
	find_email_adr("\"name <test@foo.com> test@foo.com", &c6);
	ATF_REQUIRE_EQ(tll_length(c6), 1);
	ATF_REQUIRE_STREQ(tll_front(c6), "name <test@foo.com> test@foo.com");
	find_email_adr("\"name <test@foo.com>\\ test@foo.com", &c7);
	ATF_REQUIRE_EQ(tll_length(c7), 1);
	ATF_REQUIRE_STREQ(tll_front(c7), "name <test@foo.com> test@foo.com");
	find_email_adr("\"name <test@foo.com> test@foo.com\\", &c7);
	ATF_REQUIRE_EQ(tll_length(c7), 2);
	ATF_REQUIRE_STREQ(tll_back(c7), "name <test@foo.com> test@foo.com");
	find_email_adr("test at foo.com", &c7);
	ATF_REQUIRE_EQ(tll_length(c7), 3);
	ATF_REQUIRE_STREQ(tll_back(c7), "test@foo.com");
	find_email_adr("test atfoo.com", &c7);
	ATF_REQUIRE_EQ(tll_length(c7), 4);
	ATF_REQUIRE_STREQ(tll_back(c7), "test atfoo.com");
	find_email_adr("test a t foo.com", &c7);
	ATF_REQUIRE_EQ(tll_length(c7), 5);
	ATF_REQUIRE_STREQ(tll_back(c7), "test a t foo.com");
	find_email_adr("test@foo.com, \"meh\"", &c7);
	ATF_REQUIRE_EQ(tll_length(c7), 7);
	ATF_REQUIRE_STREQ(tll_back(c7), "\"meh\"");
	find_email_adr("test@foo.com, \"meh\\\"", &c7);
	ATF_REQUIRE_EQ(tll_length(c7), 9);
	ATF_REQUIRE_STREQ(tll_back(c7), "meh ");
	find_email_adr("", &c7);
	ATF_REQUIRE_EQ(tll_length(c7), 9);
	find_email_adr("\"meh\" , ", &c7);
	ATF_REQUIRE_EQ(tll_length(c7), 10);
	ATF_REQUIRE_STREQ(tll_back(c7), "meh");
	find_email_adr("\"meh , ", &c7);
	ATF_REQUIRE_EQ(tll_length(c7), 11);
	ATF_REQUIRE_STREQ(tll_back(c7), "meh   ");
	find_email_adr("\t,", &c7);
	ATF_REQUIRE_EQ(tll_length(c7), 11);
	find_email_adr("\r,", &c7);
	ATF_REQUIRE_EQ(tll_length(c7), 11);
	find_email_adr("\n,", &c7);
	ATF_REQUIRE_EQ(tll_length(c7), 11);
	find_email_adr(", m", &c7);
	ATF_REQUIRE_EQ(tll_length(c7), 12);
	find_email_adr(", \"m", &c7);
	ATF_REQUIRE_EQ(tll_length(c7), 13);
	find_email_adr("bob @ foo.net", &c7);
	ATF_REQUIRE_EQ(tll_length(c7), 14);
	ATF_REQUIRE_STREQ(tll_back(c7), "bob@foo.net");
	find_email_adr("bob @foo.net", &c7);
	ATF_REQUIRE_EQ(tll_length(c7), 15);
	ATF_REQUIRE_STREQ(tll_back(c7), "bob @foo.net");
	find_email_adr("bob @foo.net>", &c7);
	ATF_REQUIRE_EQ(tll_length(c7), 16);
	ATF_REQUIRE_STREQ(tll_back(c7), "bob @foo.net>");
	find_email_adr("<bob @foo.net> garbage,", &c7);
	ATF_REQUIRE_EQ(tll_length(c7), 17);
	ATF_REQUIRE_STREQ(tll_back(c7), "bob @foo.net");
	find_email_adr("<bob @foo.net> garbage (more garbage) ,", &c7);
	ATF_REQUIRE_EQ(tll_length(c7), 18);
	ATF_REQUIRE_STREQ(tll_back(c7), "bob @foo.net");
	find_email_adr("<bob @foo.net> garbage \"more garbage\" ,", &c7);
	ATF_REQUIRE_EQ(tll_length(c7), 19);
	ATF_REQUIRE_STREQ(tll_back(c7), "bob @foo.net");
	find_email_adr("<bob @foo.net> garbage \"more garbage \\ ,", &c7);
	ATF_REQUIRE_EQ(tll_length(c7), 20);
	ATF_REQUIRE_STREQ(tll_back(c7), "bob @foo.net");
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
}

ATF_TC_BODY(write_mailbody, tc)
{
	/* no new lines: ignore */
	FILE *fp;
	atf_utils_create_file("myemailbody.txt", "line");
	fp = fopen("myemailbody.txt", "r");
	ATF_REQUIRE(fp != NULL);
	int fd = open("out.txt", O_CREAT|O_WRONLY, 0644);
	write_mailbody(fd, fp, "test@plop.meh");
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
	write_mailbody(fd, fp, "test@plop.meh");
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
	write_mailbody(fd, fp,  "test@plop.meh");
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
	write_mailbody(fd, fp,  "test@plop.meh");
	close(fd);
	fclose(fp);
	if (!atf_utils_compare_file("out.txt", "line\r\n.no")) {
		atf_utils_cat_file("out.txt", ">");
		atf_tc_fail("Unexpected output (single new line with a .)");
	}

	/* With a single new line, 2 new lines */
	atf_utils_create_file("myemailbody.txt", "line\n\n");
	fp = fopen("myemailbody.txt", "r");
	ATF_REQUIRE(fp != NULL);
	fd = open("out.txt", O_CREAT|O_TRUNC|O_WRONLY, 0644);
	write_mailbody(fd, fp,  "test@plop.meh");
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
	write_mailbody(fd, fp,  "test@plop.meh");
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
	write_mailbody(fd, fp,  NULL);
	close(fd);
	fclose(fp);
	if (!atf_utils_compare_file("out.txt", "line\r\n\r\nline2\r\n\r\n")) {
		atf_utils_cat_file("out.txt", ">");
		atf_tc_fail("Unexpected output (no to header)");
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
	ATF_REQUIRE_STREQ(decode_qp("="), "=");
	ATF_REQUIRE_STREQ(decode_qp("=\ra"), "=\ra");
	ATF_REQUIRE_STREQ(decode_qp("=\r\na"), "a");
	ATF_REQUIRE_STREQ(decode_qp("=\na"), "a");
	ATF_REQUIRE_STREQ(decode_qp("This is a subject"), "This is a subject");
	ATF_REQUIRE_STREQ(decode_qp("This= is a subject"), "This= is a subject");
	ATF_REQUIRE_STREQ(decode_qp("This=2 is a subject"), "This=2 is a subject");
	ATF_REQUIRE_STREQ(decode_qp("This=23 is a subject"), "This# is a subject");
	ATF_REQUIRE_STREQ(decode_qp("This=3D is a subject"), "This= is a subject");
	ATF_REQUIRE_STREQ(decode_qp("This_ is a subject"), "This_ is a subject");
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
		    "250 2.1.0 Ok\n", /* DATA */
		    "221 2.0.0 Bye\n", /* QUIT */
		    "250 2.0.0 Ok\n", /* RSET */
		};
		dprintf(smtppipe[0], "220 me fake smtp\n");
		for (uint8_t i = 0; i < NELEM(replies); i++) {
			atf_utils_readline(smtppipe[0]);
			dprintf(smtppipe[0], "%s", replies[i]);
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
		socklen_t clsize = 0;
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
		socklen_t clsize = 0;
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
		socklen_t clsize = 0;
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
		socklen_t clsize = 0;
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

ATF_TC_BODY(getaddrsfromfile, tc)
{
	strlist stl = tll_init();
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
	ATF_REQUIRE_EQ_MSG(tll_length(stl), 1, "%zu is not 1", tll_length(stl));
	ATF_REQUIRE_STREQ(tll_front(stl), "mail1@fqdn");
	tll_free_and_free(stl, free);
	fp = fopen("list", "r");
	ATF_REQUIRE_EQ(getaddrsfromfile(&stl, fp, 0), -1);
	atf_utils_create_file("list", "mail1@fqdn\nmail2@fqdn\nmail3@fqdn");
	fp = fopen("list", "r");
	ATF_REQUIRE(getaddrsfromfile(&stl, fp, 2) > 0);
	ATF_REQUIRE_EQ(tll_length(stl), 2);
	ATF_REQUIRE_STREQ(tll_front(stl), "mail1@fqdn");
	ATF_REQUIRE_STREQ(tll_back(stl), "mail2@fqdn");
	tll_free_and_free(stl, free);
	ATF_REQUIRE_EQ(getaddrsfromfile(&stl, fp, 2), 0);
	ATF_REQUIRE_EQ_MSG(tll_length(stl), 1, "%zu is not 1", tll_length(stl));
	ATF_REQUIRE_STREQ(tll_front(stl), "mail3@fqdn");
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
	ATF_REQUIRE_EQ(tll_length(*l), 3);
	ATF_REQUIRE_STREQ(tll_front(*l), "line1");
	ATF_REQUIRE_STREQ(tll_back(*l), "line3");
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
	atf_utils_create_file("intext", "plop\n");
	int fd = open("intext", O_RDONLY);
	text *txt = open_text_fd(fd);
	ATF_REQUIRE(txt != NULL);
	int tofd = open("totext", O_CREAT|O_WRONLY, 0644);
	init_ml(true);
	ml_init(&ml);
	ml.dir = "list";
	ATF_REQUIRE(ml_open(&ml, false));
	ATF_REQUIRE(prepstdreply_to(txt, &ml, "me@fqdn.org", "to@plop.org", NULL, tofd, "Message-Id: <fake>"));
	close_text(txt);
	fsync(tofd);
	close(tofd);
	close(fd);
	if (!atf_utils_compare_file("totext", "From: me@fqdn.org\n"
		"To: to@plop.org\n"
		"Message-Id: <fake>\n"
		"Message-Id: <fake>\n"
		"Subject: mlmmj administrivia\n"
		"MIME-Version: 1.0\n"
		"Content-Type: text/plain; charset=utf-8\n"
		"Content-Transfer-Encoding: 8bit\n"
		"\n"
		"plop\n")) {
		atf_utils_cat_file("totext", "");
		atf_tc_fail("Unexpected output");
	}
}

ATF_TC_BODY(text_1, tc)
{
	struct ml ml;
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
	int tofd = open("totext", O_CREAT|O_WRONLY, 0644);
	init_ml(true);
	ml_init(&ml);
	ml.dir = "list";
	ATF_REQUIRE(ml_open(&ml, false));
	register_default_unformatted(txt, &ml);
	ATF_REQUIRE(prepstdreply_to(txt, &ml, "me@fqdn.org", "to@plop.org", "bla@me.org", tofd, "Message-Id: <fake>"));
	close_text(txt);
	fsync(tofd);
	close(tofd);
	close(fd);
	if (!atf_utils_compare_file("totext", "Subject: huhu\n"
		"From: me@fqdn.org\n"
		"To: to@plop.org\n"
		"Message-Id: <fake>\n"
		"Message-Id: <fake>\n"
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
		)) {
		atf_utils_cat_file("totext", "");
		atf_tc_fail("Unexpected output");
	}
}

ATF_TC_BODY(file_lines, tc)
{
	atf_utils_create_file("file", "line1\nline2\n");

	file_lines_state *s = init_file_lines("file", ':');
	ATF_REQUIRE(s != NULL);
	ATF_REQUIRE(get_file_line(s) == NULL);
	rewind_file_lines(s);
	ATF_REQUIRE(get_file_line(s) == NULL);
	finish_file_lines(s);

	atf_utils_create_file("file", "line1: plop\nline2\nline3: meh");

	s = init_file_lines("file", ':');
	ATF_REQUIRE(s != NULL);
	ATF_REQUIRE(get_file_line(s) == NULL);
	rewind_file_lines(s);
	ATF_REQUIRE_STREQ(get_file_line(s), "line1");
	ATF_REQUIRE(get_file_line(s) == NULL);
	ATF_REQUIRE_STREQ(get_file_line(s), "line3");
	ATF_REQUIRE(get_file_line(s) == NULL);
	finish_file_lines(s);

	s = init_file_lines("file", '\0');
	finish_file_lines(s);

	s = init_file_lines("file", '\0');
	ATF_REQUIRE(s != NULL);
	ATF_REQUIRE(get_file_line(s) == NULL);
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
	int fd = open("output", O_WRONLY|O_CREAT, 0644);
	print_subs(fd, s);
	close(fd);
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

ATF_TC_BODY(notify_sub, tc)
{
	char *dir;
	const char *path;
	init_ml(true);
	struct ml ml;
	ml_init(&ml);
	ml.dir = "list";
	ml_open(&ml, false);
	rmdir("list/text");
	xasprintf(&dir, "%s/../listtexts/en",
	    atf_tc_get_config_var(tc, "srcdir"));

	symlink(dir, "list/text");
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
		socklen_t clsize = 0;
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

ATF_TC_BODY(generate_subscription, tc)
{
	char *dir;
	const char *path;
	init_ml(true);
	struct ml ml;
	ml_init(&ml);
	ml.dir = "list";
	ml_open(&ml, false);
	rmdir("list/text");
	xasprintf(&dir, "%s/../listtexts/en",
	    atf_tc_get_config_var(tc, "srcdir"));

	symlink(dir, "list/text");
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
	char *dir;
	const char *path;
	init_ml(true);
	struct ml ml;
	ml_init(&ml);
	ml.dir = "list";
	ml_open(&ml, false);
	rmdir("list/text");
	xasprintf(&dir, "%s/../listtexts/en",
	    atf_tc_get_config_var(tc, "srcdir"));

	symlink(dir, "list/text");
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
	char *dir;
	const char *path;
	init_ml(true);
	struct ml ml;
	ml_init(&ml);
	ml.dir = "list";
	ml_open(&ml, false);
	rmdir("list/text");
	xasprintf(&dir, "%s/../listtexts/en",
	    atf_tc_get_config_var(tc, "srcdir"));

	symlink(dir, "list/text");
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
	strlist fromemails = tll_init();

	ATF_REQUIRE(ml_open(&ml, false));
	ATF_REQUIRE_EQ(listcontrol(NULL, &ml, "plop", NULL, "meh"), -1);

	ATF_REQUIRE_EQ(listcontrol(&fromemails, &ml, "help", NULL, "meh"), -1);

	atf_utils_create_file("list/control/closedlist", "");
	atf_utils_create_file("meh", "mail");
	tll_push_back(fromemails, "plop@test");
	ATF_REQUIRE_EQ(listcontrol(&fromemails, &ml, "unsubscribe", NULL, "meh"), -1);
	if (atf_utils_file_exists("me"))
		atf_tc_fail("mail should have been removed");

	unlink("list/control/closedlist");
	atf_utils_create_file("list/control/closedlistsub", "");
	ATF_REQUIRE_EQ(listcontrol(&fromemails, &ml, "subscribe", NULL, "meh"), -1);
}

ATF_TC_BODY(send_help, tc)
{
	char *dir;
	init_ml(true);
	struct ml ml;
	ml_init(&ml);
	ml.dir = "list";
	ml_open(&ml, false);
	rmdir("list/text");
	xasprintf(&dir, "%s/../listtexts/en",
	    atf_tc_get_config_var(tc, "srcdir"));

	symlink(dir, "list/text");
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
	strlist l = tll_init();
	init_ml(true);
	int dfd = open("list", O_DIRECTORY);

	rmdir("list/requeue");
	ATF_REQUIRE(!requeuemail(dfd, 1, NULL, NULL));
	ATF_REQUIRE(!requeuemail(dfd, 1, NULL, NULL));
	ATF_REQUIRE(!requeuemail(dfd, 1, &l, NULL));
	tll_push_back(l, "test1");
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
	tll_push_back(l, "testhey");
	tll_push_back(l, "test2");
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
	ATF_REQUIRE(readlf(-1, true) == NULL);
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
		enum subtype subtype;
		char *addr;
		ml.fd = open(".", O_DIRECTORY);
		mod_get_addr_and_type(&ml, "modstr", &addr, &subtype);
	}
	atf_utils_wait(p, 1, "", "");

	p = atf_utils_fork();
	if (p == 0) {
		struct ml ml;
		enum subtype subtype;
		char *addr;
		ml.fd = open(".", O_DIRECTORY);
		mod_get_addr_and_type(&ml, "invalid", &addr, &subtype);
	}
	atf_utils_wait(p, 1, "", "");

	p = atf_utils_fork();
	if (p == 0) {
		struct ml ml;
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
		enum subtype subtype = 0;
		char *addr;
		ml.fd = open(".", O_DIRECTORY);
		mod_get_addr_and_type(&ml, "invalid5", &addr, &subtype);
		exit(EXIT_SUCCESS);
	}
	atf_utils_wait(p, 1, "", "");
}

ATF_TC_BODY(send_probe, tc)
{
	char *dir;
	init_ml(true);
	struct ml ml;
	ml_init(&ml);
	ml.dir = "list";
	ml_open(&ml, false);
	rmdir("list/text");
	xasprintf(&dir, "%s/../listtexts/en",
	    atf_tc_get_config_var(tc, "srcdir"));

	symlink(dir, "list/text");
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

	p = atf_utils_fork();
	if (p == 0) {
		if (send_probe(&ml, "bapt@freebsd.org"))
			exit(0);
		exit(1);
	}
	atf_utils_wait(p, 1, "", "");

	atf_utils_create_file("list/bounce/bapt=freebsd.org", "1234:blabla\n");
	p2 = single_mail_reception(smtppipe[1]);
	atf_utils_readline(smtppipe[0]);
	p = atf_utils_fork();
	if (p == 0) {
		if (send_probe(&ml, "bapt=freebsd.org"))
			exit(0);
		exit(1);
	}
	atf_utils_wait(p2, 0, "save:mailout.txt", "");
	atf_utils_wait(p, 0, "", "");
	if (!atf_utils_grep_file("1234", "mailout.txt")) {
		atf_utils_cat_file("mailout.txt", ">");
		atf_tc_fail("invalid mail sent");
	}
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
	ATF_TP_ADD_TC(tp, getlistdelim);
	ATF_TP_ADD_TC(tp, getlistdelim_0);
	ATF_TP_ADD_TC(tp, getlistdelim_1);
	ATF_TP_ADD_TC(tp, getlistdelim_2);
	ATF_TP_ADD_TC(tp, getlistdelim_3);
	ATF_TP_ADD_TC(tp, getlistdelim_4);
	ATF_TP_ADD_TC(tp, statctrl);
	ATF_TP_ADD_TC(tp, is_subbed_in);
	ATF_TP_ADD_TC(tp, getaddrsfromfile);
	ATF_TP_ADD_TC(tp, dumpfd2fd);
	ATF_TP_ADD_TC(tp, copy_file);
	ATF_TP_ADD_TC(tp, copy_file_1);
	ATF_TP_ADD_TC(tp, copy_file_2);
	ATF_TP_ADD_TC(tp, controls);
	ATF_TP_ADD_TC(tp, incindexfile);
	ATF_TP_ADD_TC(tp, log_oper);
	ATF_TP_ADD_TC(tp, get_ctrl_command);
	ATF_TP_ADD_TC(tp, get_recipextra_from_env_none);
	ATF_TP_ADD_TC(tp, get_recipextra_from_env_qmail);
	ATF_TP_ADD_TC(tp, get_recipextra_from_env_postfix);
	ATF_TP_ADD_TC(tp, get_recipextra_from_env_exim);
	ATF_TP_ADD_TC(tp, addrmatch);
	ATF_TP_ADD_TC(tp, get_subcookie_content);
	ATF_TP_ADD_TC(tp, ml_list);
	ATF_TP_ADD_TC(tp, gen_addr);
	ATF_TP_ADD_TC(tp, memory_lines);
	ATF_TP_ADD_TC(tp, text_0);
	ATF_TP_ADD_TC(tp, text_1);
	ATF_TP_ADD_TC(tp, file_lines);
	ATF_TP_ADD_TC(tp, list_subs);
	ATF_TP_ADD_TC(tp, notify_sub);
	ATF_TP_ADD_TC(tp, get_processed_text_line);
	ATF_TP_ADD_TC(tp, newsmtp);
	ATF_TP_ADD_TC(tp, save_queue);
	ATF_TP_ADD_TC(tp, send_single_mail);
	ATF_TP_ADD_TC(tp, generate_subscription);
	ATF_TP_ADD_TC(tp, generate_subconfirm);
	ATF_TP_ADD_TC(tp, send_confirmation_mail);
	ATF_TP_ADD_TC(tp, listcontrol);
	ATF_TP_ADD_TC(tp, send_help);
	ATF_TP_ADD_TC(tp, requeuemail);
	ATF_TP_ADD_TC(tp, gethdrline);
	ATF_TP_ADD_TC(tp, readlf);
	ATF_TP_ADD_TC(tp, mod_get_addr_type);
	ATF_TP_ADD_TC(tp, send_probe);

	return (atf_no_error());
}
