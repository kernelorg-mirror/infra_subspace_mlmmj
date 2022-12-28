/*
 * Copyright (C) 2022 Baptiste Daroussin <bapt@FreeBSD.org>
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

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/mman.h>

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
#include "getlistaddr.h"
#include "statctrl.h"
#include "subscriberfuncs.h"
#include "getaddrsfromfile.h"

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
ATF_TC_WITHOUT_HEAD(write_mailbody_from_map);
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
ATF_TC_WITHOUT_HEAD(send_mail_0);
ATF_TC_WITHOUT_HEAD(send_mail_1);
ATF_TC_WITHOUT_HEAD(send_mail_2);
ATF_TC_WITHOUT_HEAD(send_mail_3);
ATF_TC_WITHOUT_HEAD(send_mail_4);
ATF_TC_WITHOUT_HEAD(send_mail_5);
ATF_TC_WITHOUT_HEAD(send_mail_6);
ATF_TC_WITHOUT_HEAD(send_mail_7);
ATF_TC_WITHOUT_HEAD(send_mail_8);
ATF_TC_WITHOUT_HEAD(getlistdelim);
ATF_TC_WITHOUT_HEAD(getlistaddr);
ATF_TC_WITHOUT_HEAD(statctrl);
ATF_TC_WITHOUT_HEAD(is_subbed_in);
ATF_TC_WITHOUT_HEAD(getaddrsfromfile);

#ifndef NELEM
#define NELEM(array)    (sizeof(array) / sizeof((array)[0]))
#endif

const char *listdir[] = {
	"incoming",
	"queue",
	"queue/discarded",
	"archive",
	"text",
	"subconf",
	"unsubconf",
	"bounce",
	"control",
	"moderation",
	"subscribers.d",
	"digesters.d",
	"requeue",
	"nomailsubs.d"
};

void
init_ml(bool complete)
{
	int fd;
	ATF_CHECK(mkdir("list", 00755) != -1);
	fd = open("list", O_DIRECTORY);
	ATF_CHECK(fd != -1);

	for (uintmax_t i = 0; i < NELEM(listdir); i++) {
		ATF_CHECK(mkdirat(fd, listdir[i], 00755) != -1);
	}

	if (complete) {
		FILE *f = fopen("list/control/listaddress", "w+");
		ATF_CHECK(f != NULL);
		fputs("test@test", f);
		fclose(f);
	}
}

static int
fakesmtp(int pipe)
{
	int s;
	struct sockaddr_in me;
	s = socket(AF_INET, SOCK_STREAM, 0);
	if (s < 0)
		exit(1);
	if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &(int) { 1 }, sizeof(int)) != 0)
		exit(2);
	memset(&me, 0, sizeof(me));
	me.sin_family = AF_INET;
	me.sin_addr.s_addr = inet_addr("127.0.0.1");
	/* specific interface */
	me.sin_port = htons(25678);
	if (bind(s, (struct sockaddr *) &me, sizeof(me)) == -1)
		exit(3);
	if (listen(s, 1) == -1)
			exit(4);
	dprintf(pipe, "ready\n");

	return (s);
}

ATF_TC_BODY(random_int, tc)
{
	intmax_t val = random_int();

	ATF_CHECK_MSG(val < INT_MAX, "Invalid integer");
}

static const char *formatted_text1 =
"Subject: =?utf-8?q?Welcome_to_test=40test?=\n"
"From: test+help@test\n"
"To: john@doe.net\n"
"Message-ID: <1636022023-16598-mlmmj-39117dfa@test>\n"
"Date: Thu, 04 Nov 2021 10:33:43 +0000\n"
"MIME-Version: 1.0\n"
"Content-Type: text/plain; charset=utf-8\n"
"Content-Transfer-Encoding: 8bit\n"
"\n"
"\n"
"Thank you for your request to join us. You have now been added to the\n"
"normal version of the list.\n"
"\n"
"The email address you are subscribed with is <john@doe.net>.\n"
"\n"
"If you ever wish to unsubscribe, send a message to <test+unsubscribe@test>\n"
"using this email address. The subject and the body of the message can be\n"
"anything. You will then receive confirmation or further instructions.\n"
"\n"
"For other information and help about this list, send a message to\n"
"<test+help@test>.\n"
"\n";

ATF_TC_BODY(chomp, tc)
{
	char test1[] = "\n";
	char test2[] = "\r";
	char test3[] = "\r\n";
	char test4[] = "test\r\n";
	char test5[] = "";
	ATF_CHECK(chomp(NULL) == NULL);
	char *bla = chomp(test1);
	ATF_CHECK(bla != NULL);
	ATF_REQUIRE_STREQ(bla, "");
	bla = chomp(test2);
	ATF_CHECK(bla != NULL);
	ATF_REQUIRE_STREQ(bla, "");
	bla = chomp(test3);
	ATF_CHECK(bla != NULL);
	ATF_REQUIRE_STREQ(bla, "");
	bla = chomp(test4);
	ATF_CHECK(bla != NULL);
	ATF_REQUIRE_STREQ(bla, "test");
	bla = chomp(test5);
	ATF_CHECK(bla != NULL);
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
	res = mygetline(fd);
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
	ATF_REQUIRE(pipe(mypipe) >= 0);
	pid_t p = atf_utils_fork();
	if (p == 0) {
		int s = fakesmtp(mypipe[0]);
		int c;
		struct sockaddr_in me, cl;
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
	close(mypipe[0]);
	mygetline(mypipe[1]);
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
	struct email_container c = {0, NULL};
	struct email_container c1 = {0, NULL};
	struct email_container c2 = {0, NULL};
	struct email_container c3 = {0, NULL};
	struct email_container c4 = {0, NULL};
	struct email_container c5 = {0, NULL};
	struct email_container c6 = {0, NULL};
	struct email_container c7 = {0, NULL};
	find_email_adr("test@foo.bar", &c);
	ATF_REQUIRE_EQ(c.emailcount, 1);
	find_email_adr("test@foo.bar, meh@bar", &c1);
	ATF_REQUIRE_EQ(c1.emailcount, 2);
	find_email_adr("test@foo.bar, meh@bar, nope", &c2);
	ATF_REQUIRE_EQ(c2.emailcount, 3);
	find_email_adr("name <test@foo.com>", &c3);
	ATF_REQUIRE_EQ(c3.emailcount, 1);
	ATF_REQUIRE_STREQ(c3.emaillist[0], "test@foo.com");
	find_email_adr("(name <test@foo.com>) test@foo.com", &c4);
	ATF_REQUIRE_EQ(c4.emailcount, 1);
	ATF_REQUIRE_STREQ(c4.emaillist[0], " test@foo.com");
	find_email_adr("\"name <test@foo.com>\" test@foo.com", &c5);
	ATF_REQUIRE_EQ(c5.emailcount, 1);
	ATF_REQUIRE_STREQ(c5.emaillist[0], "name <test@foo.com> test@foo.com");
	find_email_adr("\"name <test@foo.com> test@foo.com", &c6);
	ATF_REQUIRE_EQ(c6.emailcount, 1);
	ATF_REQUIRE_STREQ(c6.emaillist[0], "name <test@foo.com> test@foo.com");
	find_email_adr("\"name <test@foo.com>\\ test@foo.com", &c7);
	ATF_REQUIRE_EQ(c7.emailcount, 1);
	ATF_REQUIRE_STREQ(c7.emaillist[0], "name <test@foo.com> test@foo.com");
	find_email_adr("\"name <test@foo.com> test@foo.com\\", &c7);
	ATF_REQUIRE_EQ(c7.emailcount, 2);
	ATF_REQUIRE_STREQ(c7.emaillist[1], "name <test@foo.com> test@foo.com");
	find_email_adr("test at foo.com", &c7);
	ATF_REQUIRE_EQ(c7.emailcount, 3);
	ATF_REQUIRE_STREQ(c7.emaillist[2], "test@foo.com");
	find_email_adr("test atfoo.com", &c7);
	ATF_REQUIRE_EQ(c7.emailcount, 4);
	ATF_REQUIRE_STREQ(c7.emaillist[3], "test atfoo.com");
	find_email_adr("test a t foo.com", &c7);
	ATF_REQUIRE_EQ(c7.emailcount, 5);
	ATF_REQUIRE_STREQ(c7.emaillist[4], "test a t foo.com");
	find_email_adr("test@foo.com, \"meh\"", &c7);
	ATF_REQUIRE_EQ(c7.emailcount, 7);
	ATF_REQUIRE_STREQ(c7.emaillist[6], "\"meh\"");
	find_email_adr("test@foo.com, \"meh\\\"", &c7);
	ATF_REQUIRE_EQ(c7.emailcount, 9);
	ATF_REQUIRE_STREQ(c7.emaillist[8], "meh ");
	find_email_adr("", &c7);
	ATF_REQUIRE_EQ(c7.emailcount, 9);
	find_email_adr("\"meh\" , ", &c7);
	ATF_REQUIRE_EQ(c7.emailcount, 10);
	ATF_REQUIRE_STREQ(c7.emaillist[9], "meh");
	find_email_adr("\"meh , ", &c7);
	ATF_REQUIRE_EQ(c7.emailcount, 11);
	ATF_REQUIRE_STREQ(c7.emaillist[10], "meh   ");
	find_email_adr("\t,", &c7);
	ATF_REQUIRE_EQ(c7.emailcount, 11);
	find_email_adr("\r,", &c7);
	ATF_REQUIRE_EQ(c7.emailcount, 11);
	find_email_adr("\n,", &c7);
	ATF_REQUIRE_EQ(c7.emailcount, 11);
	find_email_adr(", m", &c7);
	ATF_REQUIRE_EQ(c7.emailcount, 12);
	find_email_adr(", \"m", &c7);
	ATF_REQUIRE_EQ(c7.emailcount, 13);
	find_email_adr("bob @ foo.net", &c7);
	ATF_REQUIRE_EQ(c7.emailcount, 14);
	ATF_REQUIRE_STREQ(c7.emaillist[13], "bob@foo.net");
	find_email_adr("bob @foo.net", &c7);
	ATF_REQUIRE_EQ(c7.emailcount, 15);
	ATF_REQUIRE_STREQ(c7.emaillist[14], "bob @foo.net");
	find_email_adr("bob @foo.net>", &c7);
	ATF_REQUIRE_EQ(c7.emailcount, 16);
	ATF_REQUIRE_STREQ(c7.emaillist[15], "bob @foo.net>");
	find_email_adr("<bob @foo.net> garbage,", &c7);
	ATF_REQUIRE_EQ(c7.emailcount, 17);
	ATF_REQUIRE_STREQ(c7.emaillist[16], "bob @foo.net");
	find_email_adr("<bob @foo.net> garbage (more garbage) ,", &c7);
	ATF_REQUIRE_EQ(c7.emailcount, 18);
	ATF_REQUIRE_STREQ(c7.emaillist[17], "bob @foo.net");
	find_email_adr("<bob @foo.net> garbage \"more garbage\" ,", &c7);
	ATF_REQUIRE_EQ(c7.emailcount, 19);
	ATF_REQUIRE_STREQ(c7.emaillist[18], "bob @foo.net");
	find_email_adr("<bob @foo.net> garbage \"more garbage \\ ,", &c7);
	ATF_REQUIRE_EQ(c7.emailcount, 20);
	ATF_REQUIRE_STREQ(c7.emaillist[19], "bob @foo.net");
}

ATF_TC_BODY(strtoim, tc)
{
	const char *errp;

	ATF_REQUIRE_EQ(strtoim(NULL, 0, 10, &errp), 0);
	ATF_REQUIRE(errp != NULL);
	ATF_REQUIRE_STREQ(errp, "invalid");
	ATF_REQUIRE_EQ(strtoim("10", 0, 10, &errp), 10);
	ATF_CHECK(errp == NULL);
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


	ATF_REQUIRE_EQ(strtouim(NULL, 0, 10, &errp), 0);
	ATF_REQUIRE(errp != NULL);
	ATF_REQUIRE_STREQ(errp, "invalid");
	ATF_REQUIRE_EQ(strtouim("10", 0, 10, &errp), 10);
	ATF_CHECK(errp == NULL);
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

ATF_TC_BODY(write_mailbody_from_map, tc)
{
	/* no new lines: ignore */
	struct stat st;
	char *start;
	FILE *f = fopen("myemailbody.txt", "w");
	fprintf(f, "line");
	fclose(f);
	int fd1 = open("myemailbody.txt", O_RDONLY);
	fstat(fd1, &st);
	ATF_REQUIRE((start = mmap(0, st.st_size, PROT_READ, MAP_SHARED, fd1, 0)) != NULL);
	int fd = open("out.txt", O_CREAT|O_WRONLY, 0644);
	ATF_REQUIRE_EQ(write_mailbody_from_map(fd, start, st.st_size, "test@plop.meh"), 0);
	close(fd);
	munmap(start, st.st_size);
	close(fd1);
	if (!atf_utils_compare_file("out.txt", "")) {
		atf_utils_cat_file("out.txt", ">");
		atf_tc_fail("Unexpected output");
	}

	/* With a single new line */
	f = fopen("myemailbody.txt", "w");
	fprintf(f, "line\n");
	fclose(f);
	fd1 = open("myemailbody.txt", O_RDONLY);
	fstat(fd1, &st);
	ATF_REQUIRE((start = mmap(0, st.st_size, PROT_READ, MAP_SHARED, fd1, 0)) != NULL);
	fd = open("out.txt", O_CREAT|O_TRUNC|O_WRONLY, 0644);
	ATF_REQUIRE_EQ(write_mailbody_from_map(fd, start, st.st_size, "test@plop.meh"), 0);
	close(fd);
	munmap(start, st.st_size);
	close(fd1);
	if (!atf_utils_compare_file("out.txt", "line\r\n")) {
		atf_utils_cat_file("out.txt", ">");
		atf_tc_fail("Unexpected output");
	}

	/* With a single new line with a . */
	f = fopen("myemailbody.txt", "w");
	fprintf(f, "line\n.");
	fclose(f);
	fd1 = open("myemailbody.txt", O_RDONLY);
	fstat(fd1, &st);
	ATF_REQUIRE((start = mmap(0, st.st_size, PROT_READ, MAP_SHARED, fd1, 0)) != NULL);
	fd = open("out.txt", O_CREAT|O_TRUNC|O_WRONLY, 0644);
	ATF_REQUIRE_EQ(write_mailbody_from_map(fd, start, st.st_size, "test@plop.meh"), 0);
	close(fd);
	munmap(start, st.st_size);
	close(fd1);
	if (!atf_utils_compare_file("out.txt", "line\r\n.")) {
		atf_utils_cat_file("out.txt", ">");
		atf_tc_fail("Unexpected output");
	}

	/* With a single new line, 2 new lines */
	f = fopen("myemailbody.txt", "w");
	fprintf(f, "line\n\n");
	fclose(f);
	fd1 = open("myemailbody.txt", O_RDONLY);
	fstat(fd1, &st);
	ATF_REQUIRE((start = mmap(0, st.st_size, PROT_READ, MAP_SHARED, fd1, 0)) != NULL);
	fd = open("out.txt", O_CREAT|O_TRUNC|O_WRONLY, 0644);
	ATF_REQUIRE_EQ(write_mailbody_from_map(fd, start, st.st_size, "test@plop.meh"), 0);
	close(fd);
	munmap(start, st.st_size);
	close(fd1);
	if (!atf_utils_compare_file("out.txt", "line\r\nTo: test@plop.meh\r\n\r\n")) {
		atf_utils_cat_file("out.txt", ">");
		atf_tc_fail("Unexpected output");
	}


	/* With a single 2 lines separated by 2 new lines*/
	f = fopen("myemailbody.txt", "w");
	fprintf(f, "line\n\nline2\n\n");
	fclose(f);
	fd1 = open("myemailbody.txt", O_RDONLY);
	fstat(fd1, &st);
	ATF_REQUIRE((start = mmap(0, st.st_size, PROT_READ, MAP_SHARED, fd1, 0)) != NULL);
	fd = open("out.txt", O_CREAT|O_TRUNC|O_WRONLY, 0644);
	ATF_REQUIRE_EQ(write_mailbody_from_map(fd, start, st.st_size, "test@plop.meh"), 0);
	close(fd);
	munmap(start, st.st_size);
	close(fd1);
	if (!atf_utils_compare_file("out.txt", "line\r\nTo: test@plop.meh\r\n\r\nline2\r\n\r\n")) {
		atf_utils_cat_file("out.txt", ">");
		atf_tc_fail("Unexpected output");
	}


	/* No to header */
	f = fopen("myemailbody.txt", "w");
	fprintf(f, "line\n\nline2\n\n");
	fclose(f);
	fd1 = open("myemailbody.txt", O_RDONLY);
	fstat(fd1, &st);
	ATF_REQUIRE((start = mmap(0, st.st_size, PROT_READ, MAP_SHARED, fd1, 0)) != NULL);
	fd = open("out.txt", O_CREAT|O_TRUNC|O_WRONLY, 0644);
	ATF_REQUIRE_EQ(write_mailbody_from_map(fd, start, st.st_size, NULL), 0);
	close(fd);
	munmap(start, st.st_size);
	close(fd1);
	if (!atf_utils_compare_file("out.txt", "line\r\n\r\nline2\r\n\r\n")) {
		atf_utils_cat_file("out.txt", ">");
		atf_tc_fail("Unexpected output");
	}

	/* With invalid socket */
	f = fopen("myemailbody.txt", "w");
	fprintf(f, "line\n");
	fclose(f);
	fd1 = open("myemailbody.txt", O_RDONLY);
	fstat(fd1, &st);
	ATF_REQUIRE((start = mmap(0, st.st_size, PROT_READ, MAP_SHARED, fd1, 0)) != NULL);
	ATF_REQUIRE_EQ(write_mailbody_from_map(-1, start, st.st_size, "test@plop.meh"), -1);
	munmap(start, st.st_size);
	close(fd1);
}

ATF_TC_BODY(strtotimet, tc)
{
	const char *errp;
	char *str;
	ATF_REQUIRE_EQ(strtotimet("10", &errp), 10);
	ATF_CHECK(errp == NULL);
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
	FILE *f;
	init_ml(true);

	listfd = open("list", O_DIRECTORY);

	f = fopen("list/subscribers.d/j", "w");
	fputs("john@doe.org", f);
	fclose(f);

	f = fopen("list/nomailsubs.d/j", "w");
	fputs("john@doe.org\n", f);
	fputs("jane@doe.org\n", f);
	fclose(f);

	f = fopen("list/digesters.d/j", "w");
	fputs("john@doe.org\n", f);
	fclose(f);

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

	f = fopen("list/nomailsubs.d/j", "w");
	fclose(f);
	chmod("list/nomailsubs.d/j", 0111);
	ATF_REQUIRE(unsubscribe(listfd, "plop@meh", SUB_NOMAIL));
	chmod("list/nomailsubs.d/j", 0644);
	ATF_REQUIRE(unsubscribe(listfd, "plop@meh", SUB_NOMAIL));
	unlink("list/nomailsubs.d/j");

	f = fopen("list/nomailsubs.d/j", "w");
	fputs("john@doe.org\n", f);
	fputs("jane@doe.org\n", f);
	fclose(f);
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
	char *ret;

	ret = genlistfqdn("plop@bla");
	ATF_REQUIRE_STREQ(ret, "bla");
	ret = genlistfqdn("plop@bla@meh");
	ATF_REQUIRE_STREQ(ret, "bla@meh");
}

ATF_TC_BODY(smtp, tc)
{
	int smtppipe[2];
	char *reply;
	ATF_REQUIRE(pipe(smtppipe) >= 0);
	pid_t p = atf_utils_fork();
	if (p == 0) {
		dprintf(smtppipe[0], "220 me fake smtp\n");
		/* EHLO */
		reply = mygetline(smtppipe[0]);
		dprintf(smtppipe[0],
		    "250-hostname.net\n"
		    "250-PIPELINEING\n"
		    "250-SIZE 20480000\n"
		    "250-ETRN\n"
		    "250-STARTTLS\n"
		    "250-ENHANCEDSTATUSCODES\n"
		    "250-8BITMIME\n"
		    "250-DSN\n"
		    "250-SMTPUTF8\n"
		    "250 CHUNKING\n");
		/* HELO */
		reply = mygetline(smtppipe[0]);
		dprintf(smtppipe[0],
		    "250-hostname.net\n");
		/* MAIL FROM */
		reply = mygetline(smtppipe[0]);
		dprintf(smtppipe[0],
		    "250 2.1.0 Ok\n");
		/* RCPT TO */
		reply = mygetline(smtppipe[0]);
		dprintf(smtppipe[0],
		    "250 2.1.0 Ok\n");
		/* DATA */
		reply = mygetline(smtppipe[0]);
		dprintf(smtppipe[0],
		    "354 Send message content; end with <CRLF>.<CRLF>\n");
		/* DATA */
		reply = mygetline(smtppipe[0]);
		dprintf(smtppipe[0],
		    "250 2.1.0 Ok\n");
		/* QUIT */
		reply = mygetline(smtppipe[0]);
		dprintf(smtppipe[0],
		    "221 2.0.0 Bye\n");
		/* RSET */
		reply = mygetline(smtppipe[0]);
		dprintf(smtppipe[0],
		    "250 2.0.0 Ok\n");
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
	ATF_REQUIRE(pipe(smtppipe) >= 0);
	pid_t p = atf_utils_fork();
	if (p == 0) {
		int s = fakesmtp(smtppipe[0]);
		int c;
		struct sockaddr_in cl;
		char *reply;
		socklen_t clsize = 0;
		c = accept(s, (struct sockaddr *) &cl, &clsize);
		if (c == -1)
			err(5, "accept()");
		dprintf(c, "220 me fake smtp\n");
		reply = mygetline(c);
		printf("%s", reply);
		dprintf(c,
			"250-hostname.net\n"
			"250-PIPELINEING\n"
			"250-SIZE 20480000\n"
			"250-ETRN\n"
			"250-STARTTLS\n"
			"250-ENHANCEDSTATUSCODES\n"
			"250-8BITMIME\n"
			"250-DSN\n"
			"250-SMTPUTF8\n"
			"250 CHUNKING\n");
		reply = mygetline(c);
		printf("%s", reply);
		dprintf(c, "221 2.0.0 bye\n");
		exit(0);
	}
	close(smtppipe[0]);
	mygetline(smtppipe[1]);
	int sockfd;
	int ret = initsmtp(&sockfd, "127.0.0.1", 25678, "heloname");
	ATF_REQUIRE_EQ(ret, 0);
	ATF_REQUIRE_EQ(endsmtp(&sockfd), 0);
	atf_utils_wait(p, 0, "EHLO heloname\r\nQUIT\r\n", "");
}

ATF_TC_BODY(smtp_bad_greetings, tc)
{
	int smtppipe[2];
	ATF_REQUIRE(pipe(smtppipe) >= 0);

	pid_t p = atf_utils_fork();
	if (p == 0) {
		int s = fakesmtp(smtppipe[0]);
		int c;
		struct sockaddr_in cl;
		socklen_t clsize = 0;
		c = accept(s, (struct sockaddr *) &cl, &clsize);
		if (c == -1)
			err(5, "accept()");
		dprintf(c, "me fake smtp\n");
		exit(0);
	}
	close(smtppipe[0]);
	mygetline(smtppipe[1]);
	int sockfd;
	int ret = initsmtp(&sockfd, "127.0.0.1", 25678, "heloname");
	ATF_REQUIRE_EQ(ret, MLMMJ_CONNECT);
	atf_utils_wait(p, 0, "", "");
}

ATF_TC_BODY(smtp_bad_ehlo, tc)
{
	int smtppipe[2];
	ATF_REQUIRE(pipe(smtppipe) >= 0);

	pid_t p = atf_utils_fork();
	if (p == 0) {
		int s = fakesmtp(smtppipe[0]);
		int c;
		struct sockaddr_in cl;
		char *reply;
		socklen_t clsize = 0;
		c = accept(s, (struct sockaddr *) &cl, &clsize);
		if (c == -1)
			err(5, "accept()");
		dprintf(c, "220 me fake smtp\n");
		reply = mygetline(c);
		printf("%s", reply);
		dprintf(c, "501 nop nope\n");
		exit(0);
	}
	close(smtppipe[0]);
	mygetline(smtppipe[1]);
	int sockfd;
	int ret = initsmtp(&sockfd, "127.0.0.1", 25678, "heloname");
	ATF_REQUIRE_EQ(ret, MLMMJ_EHLO);
	atf_utils_wait(p, 0, "EHLO heloname\r\n", "");
}

ATF_TC_BODY(smtp_no_ehlo, tc)
{
	int smtppipe[2];
	ATF_REQUIRE(pipe(smtppipe) >= 0);

	pid_t p = atf_utils_fork();
	if (p == 0) {
		int s = fakesmtp(smtppipe[0]);
		int c;
		struct sockaddr_in cl;
		char *reply;
		socklen_t clsize = 0;
		c = accept(s, (struct sockaddr *) &cl, &clsize);
		if (c == -1)
			err(5, "accept()");
		dprintf(c, "220 me fake smtp\n");
		reply = mygetline(c);
		printf("%s", reply);
		dprintf(c, "801 meh\n");
		reply = mygetline(c);
		printf("%s", reply);
		close(c);
		c = accept(s, (struct sockaddr *) &cl, &clsize);
		if (c == -1)
			err(5, "accept()");
		dprintf(c, "220 me fake smtp\n");
		reply = mygetline(c);
		printf("%s", reply);
		dprintf(c, "250 OK\n");
		exit(0);
	}
	close(smtppipe[0]);
	mygetline(smtppipe[1]);
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

ATF_TC_BODY(send_mail_0, tc)
{
	int smtppipe[2];
	struct mail mail = {};
	ATF_REQUIRE_EQ(send_mail(-1, NULL, -1, -1, false), EBADF);
	ATF_REQUIRE(pipe(smtppipe) >= 0);
	pid_t p = atf_utils_fork();
	if (p == 0) {
		char *reply = mygetline(smtppipe[0]);
		printf("%s", reply);
		/* nope */
		dprintf(smtppipe[0], "550 2.1.0 kO\n");
		reply = mygetline(smtppipe[0]);
		printf("%s", reply);
		exit(0);
	}
	close(smtppipe[0]);
	ATF_REQUIRE_EQ(send_mail(smtppipe[1], &mail, -1, -1, false), 0);
	mail.to = "plop@meh";
	ATF_REQUIRE_EQ(send_mail(smtppipe[1], &mail, -1, -1, false), 0);
	mail.from = "test@meh";
	mail.inmem = true;
	ATF_REQUIRE_EQ(send_mail(smtppipe[1], &mail, -1, -1, false), 0);
	mail.hdrs = "plop";
	ATF_REQUIRE_EQ(send_mail(smtppipe[1], &mail, -1, -1, false), 0);
	mail.hdrs = NULL;
	mail.body = "body";
	ATF_REQUIRE_EQ(send_mail(smtppipe[1], &mail, -1, -1, false), 0);
	mail.inmem = false;
	ATF_REQUIRE_EQ(send_mail(smtppipe[1], &mail, -1, -1, false), 0);
	mail.size = 1;
	ATF_REQUIRE_EQ(send_mail(smtppipe[1], &mail, -1, -1, false), 0);
	mail.size = 0;
	mail.map = "bla";
	ATF_REQUIRE_EQ(send_mail(smtppipe[1], &mail, -1, -1, false), 0);
	mail.inmem = true;
	mail.hdrs = "headers";
	ATF_REQUIRE_EQ(send_mail(smtppipe[1], &mail, -1, -1, false), MLMMJ_FROM);
	atf_utils_wait(p, 0, "MAIL FROM:<test@meh>\r\nRSET\r\n", "");
}

ATF_TC_BODY(send_mail_1, tc)
{
	int smtppipe[2];
	struct mail mail = {};
	ATF_REQUIRE(pipe(smtppipe) >= 0);
	pid_t p = atf_utils_fork();
	if (p == 0) {
		char *reply = mygetline(smtppipe[0]);
		printf("%s", reply);
		dprintf(smtppipe[0], "250 2.1.0 Ok\n");
		reply = mygetline(smtppipe[0]);
		printf("%s", reply);
		dprintf(smtppipe[0], "550 2.1.0 OK\n");
		exit(0);
	}
	close(smtppipe[0]);
	mail.to = "plop@meh";
	mail.from = "test@meh";
	mail.inmem = true;
	mail.hdrs = "headers";
	mail.body = "body";
	ATF_REQUIRE_EQ(send_mail(smtppipe[1], &mail, -1, -1, false), MLMMJ_RCPTTO);
	atf_utils_wait(p, 0, "MAIL FROM:<test@meh>\r\nRCPT TO:<plop@meh>\r\n", "");
}

ATF_TC_BODY(send_mail_2, tc)
{
	int smtppipe[2];
	struct mail mail = {};
	ATF_REQUIRE(pipe(smtppipe) >= 0);
	pid_t p = atf_utils_fork();
	if (p == 0) {
		char *reply = mygetline(smtppipe[0]);
		printf("%s", reply);
		dprintf(smtppipe[0], "250 2.1.0 Ok\n");
		reply = mygetline(smtppipe[0]);
		printf("%s", reply);
		dprintf(smtppipe[0], "250 2.1.0 OK\n");
		reply = mygetline(smtppipe[0]);
		printf("%s", reply);
		dprintf(smtppipe[0], "550 2.1.0 OK\n");
		exit(0);
	}
	close(smtppipe[0]);
	mail.to = "plop@meh";
	mail.from = "test@meh";
	mail.inmem = true;
	mail.hdrs = "headers";
	mail.body = "body";
	ATF_REQUIRE_EQ(send_mail(smtppipe[1], &mail, -1, -1, false), MLMMJ_DATA);
	atf_utils_wait(p, 0, "MAIL FROM:<test@meh>\r\nRCPT TO:<plop@meh>\r\nDATA\r\n", "");
}

ATF_TC_BODY(send_mail_3, tc)
{
	int smtppipe[2];
	struct mail mail = {};
	ATF_REQUIRE(pipe(smtppipe) >= 0);
	pid_t p = atf_utils_fork();
	if (p == 0) {
		char *reply = mygetline(smtppipe[0]);
		printf("%s", reply);
		dprintf(smtppipe[0], "250 2.1.0 Ok\n");
		reply = mygetline(smtppipe[0]);
		printf("%s", reply);
		dprintf(smtppipe[0], "250 2.1.0 OK\n");
		reply = mygetline(smtppipe[0]);
		printf("%s", reply);
		dprintf(smtppipe[0], "550 2.1.0 OK\n");
		exit(0);
	}
	close(smtppipe[0]);
	mail.to = "plop@meh";
	mail.from = "test@meh";
	mail.inmem = true;
	mail.hdrs = "headers";
	mail.body = "body";
	ATF_REQUIRE_EQ(send_mail(smtppipe[1], &mail, -1, -1, false), MLMMJ_DATA);
	atf_utils_wait(p, 0, "MAIL FROM:<test@meh>\r\nRCPT TO:<plop@meh>\r\nDATA\r\n", "");
}

ATF_TC_BODY(send_mail_4, tc)
{
	int smtppipe[2];
	struct mail mail = {};
	ATF_REQUIRE(pipe(smtppipe) >= 0);
	pid_t p = atf_utils_fork();
	if (p == 0) {
		char *reply = mygetline(smtppipe[0]);
		printf("%s", reply);
		dprintf(smtppipe[0], "250 2.1.0 Ok\n");
		reply = mygetline(smtppipe[0]);
		printf("%s", reply);
		dprintf(smtppipe[0], "250 2.1.0 OK\n");
		reply = mygetline(smtppipe[0]);
		printf("%s", reply);
		dprintf(smtppipe[0], "250 2.1.0 OK\n");
		exit(0);
	}
	close(smtppipe[0]);
	mail.to = "plop@meh";
	mail.from = "test@meh";
	mail.inmem = true;
	mail.hdrs = "headers";
	mail.body = "body";
	ATF_REQUIRE_EQ(send_mail(smtppipe[1], &mail, -1, -1, false), MLMMJ_DATA);
	atf_utils_wait(p, 0, "MAIL FROM:<test@meh>\r\nRCPT TO:<plop@meh>\r\nDATA\r\n", "");
}

ATF_TC_BODY(send_mail_5, tc)
{
	int smtppipe[2];
	struct mail mail = {};
	ATF_REQUIRE(pipe(smtppipe) >= 0);
	pid_t p = atf_utils_fork();
	if (p == 0) {
		char *reply = mygetline(smtppipe[0]);
		printf("%s", reply);
		dprintf(smtppipe[0], "250 2.1.0 Ok\n");
		reply = mygetline(smtppipe[0]);
		printf("%s", reply);
		dprintf(smtppipe[0], "250 2.1.0 OK\n");
		reply = mygetline(smtppipe[0]);
		printf("%s", reply);
		dprintf(smtppipe[0], "350 2.1.0 OK\n");
		reply = mygetline(smtppipe[0]);
		printf("%s", reply);
		reply = mygetline(smtppipe[0]);
		printf("%s", reply);
		reply = mygetline(smtppipe[0]);
		printf("%s", reply);
		reply = mygetline(smtppipe[0]);
		printf("%s", reply);
		dprintf(smtppipe[0], "350 2.1.0 OK\n");
		exit(0);
	}
	close(smtppipe[0]);
	mail.to = "plop@meh";
	mail.from = "test@meh";
	mail.inmem = true;
	mail.hdrs = "headers\n";
	mail.body = "body\n";
	ATF_REQUIRE_EQ(send_mail(smtppipe[1], &mail, -1, -1, false), MLMMJ_DOT);
	atf_utils_wait(p, 0, "MAIL FROM:<test@meh>\r\nRCPT TO:<plop@meh>\r\nDATA\r\nheaders\nbody\n\r\n.\r\n", "");
}

ATF_TC_BODY(send_mail_6, tc)
{
	int smtppipe[2];
	struct mail mail = {};
	ATF_REQUIRE(pipe(smtppipe) >= 0);
	pid_t p = atf_utils_fork();
	if (p == 0) {
		char *reply = mygetline(smtppipe[0]);
		printf("%s", reply);
		dprintf(smtppipe[0], "250 2.1.0 Ok\n");
		reply = mygetline(smtppipe[0]);
		printf("%s", reply);
		dprintf(smtppipe[0], "250 2.1.0 OK\n");
		reply = mygetline(smtppipe[0]);
		printf("%s", reply);
		dprintf(smtppipe[0], "350 2.1.0 OK\n");
		reply = mygetline(smtppipe[0]);
		printf("%s", reply);
		reply = mygetline(smtppipe[0]);
		printf("%s", reply);
		reply = mygetline(smtppipe[0]);
		printf("%s", reply);
		reply = mygetline(smtppipe[0]);
		printf("%s", reply);
		reply = mygetline(smtppipe[0]);
		printf("%s", reply);
		dprintf(smtppipe[0], "250 2.1.0 OK\n");
		exit(0);
	}
	close(smtppipe[0]);
	mail.to = "plop@meh";
	mail.from = "test@meh";
	mail.inmem = true;
	mail.hdrs = "headers\n";
	mail.body = "body\n";
	mail.replyto= "yeah@meh";
	ATF_REQUIRE_EQ(send_mail(smtppipe[1], &mail, -1, -1, false), 0);
	atf_utils_wait(p, 0, "MAIL FROM:<test@meh>\r\nRCPT TO:<plop@meh>\r\nDATA\r\nReply-To: yeah@meh\r\nheaders\nbody\n\r\n.\r\n", "");
}

ATF_TC_BODY(send_mail_7, tc)
{
	int smtppipe[2];
	struct mail mail = {};
	ATF_REQUIRE(pipe(smtppipe) >= 0);
	pid_t p = atf_utils_fork();
	if (p == 0) {
		char *reply = mygetline(smtppipe[0]);
		printf("%s", reply);
		dprintf(smtppipe[0], "250 2.1.0 Ok\n");
		reply = mygetline(smtppipe[0]);
		printf("%s", reply);
		dprintf(smtppipe[0], "250 2.1.0 OK\n");
		reply = mygetline(smtppipe[0]);
		printf("%s", reply);
		dprintf(smtppipe[0], "350 2.1.0 OK\n");
		reply = mygetline(smtppipe[0]);
		printf("%s", reply);
		reply = mygetline(smtppipe[0]);
		printf("%s", reply);
		reply = mygetline(smtppipe[0]);
		printf("%s", reply);
		reply = mygetline(smtppipe[0]);
		printf("%s", reply);
		reply = mygetline(smtppipe[0]);
		printf("%s", reply);
		dprintf(smtppipe[0], "250 2.1.0 OK\n");
		exit(0);
	}
	close(smtppipe[0]);
	mail.to = "plop@meh";
	mail.from = "test@meh";
	mail.inmem = true;
	mail.hdrs = "headers\n";
	mail.body = "body\n";
	mail.addtohdr = true;
	ATF_REQUIRE_EQ(send_mail(smtppipe[1], &mail, -1, -1, false), 0);
	atf_utils_wait(p, 0, "MAIL FROM:<test@meh>\r\nRCPT TO:<plop@meh>\r\nDATA\r\nheaders\nTo: plop@meh\r\nbody\n\r\n.\r\n", "");
}

ATF_TC_BODY(send_mail_8, tc)
{
	int smtppipe[2];
	struct mail mail = {};
	struct stat st;
	ATF_REQUIRE(pipe(smtppipe) >= 0);
	pid_t p = atf_utils_fork();
	if (p == 0) {
		char *reply = mygetline(smtppipe[0]);
		printf("%s", reply);
		dprintf(smtppipe[0], "250 2.1.0 Ok\n");
		reply = mygetline(smtppipe[0]);
		printf("%s", reply);
		dprintf(smtppipe[0], "250 2.1.0 OK\n");
		reply = mygetline(smtppipe[0]);
		printf("%s", reply);
		dprintf(smtppipe[0], "350 2.1.0 OK\n");
		reply = mygetline(smtppipe[0]);
		printf("%s", reply);
		reply = mygetline(smtppipe[0]);
		printf("%s", reply);
		reply = mygetline(smtppipe[0]);
		printf("%s", reply);
		reply = mygetline(smtppipe[0]);
		printf("%s", reply);
		reply = mygetline(smtppipe[0]);
		printf("%s", reply);
		reply = mygetline(smtppipe[0]);
		printf("%s", reply);
		dprintf(smtppipe[0], "250 2.1.0 OK\n");
		exit(0);
	}
	close(smtppipe[0]);
	mail.to = "plop@meh";
	mail.from = "test@meh";
	mail.inmem = false;
	FILE *f = fopen("mymail.txt", "w+");
	fprintf(f, "headers\n\nbody\n");
	fclose(f);
	int fd = open("mymail.txt", O_RDONLY);
	fstat(fd, &st);
	mail.size = st.st_size;
	mail.map = mmap(0, mail.size, PROT_READ, MAP_SHARED, fd, 0);
	mail.addtohdr = true;
	ATF_REQUIRE_EQ(send_mail(smtppipe[1], &mail, -1, -1, false), 0);
	atf_utils_wait(p, 0, "MAIL FROM:<test@meh>\r\nRCPT TO:<plop@meh>\r\nDATA\r\nheaders\r\nTo: plop@meh\r\n\r\nbody\r\n\r\n.\r\n", "");
}

ATF_TC_BODY(getlistdelim, tc)
{
	init_ml(true);
	int fd = open("list/control", O_RDONLY);
	ATF_REQUIRE_STREQ_MSG(getlistdelim(fd), DEFAULT_RECIPDELIM, "Invalid fallback");

	FILE *f = fopen("list/control/delimiter", "w+");
	fclose(f);
	ATF_REQUIRE_STREQ_MSG(getlistdelim(fd), DEFAULT_RECIPDELIM, "Invalid fallback, in case of empty delimiter");
	f = fopen("list/control/delimiter", "w+");
	fprintf(f,"\n");
	fclose(f);
	ATF_REQUIRE_STREQ_MSG(getlistdelim(fd), DEFAULT_RECIPDELIM, "Invalid fallback, in case of empty delimiter");

	f = fopen("list/control/delimiter", "w+");
	fprintf(f,"mydelim\n");
	fclose(f);
	ATF_REQUIRE_STREQ_MSG(getlistdelim(fd), "mydelim", "Invalid delimiter found");

	f = fopen("list/control/delimiter", "w+");
	fprintf(f,"mydelim\nignoreme\n");
	fclose(f);
	ATF_REQUIRE_STREQ_MSG(getlistdelim(fd), "mydelim", "Invalid delimiter found");

	f = fopen("list/control/delimiter", "w+");
	fprintf(f,"mydelim");
	fclose(f);
	ATF_REQUIRE_STREQ_MSG(getlistdelim(fd), "mydelim", "Invalid delimiter found");
}

ATF_TC_BODY(getlistaddr, tc)
{
	init_ml(false);
	int fd = open("list/control", O_RDONLY);

	pid_t p = atf_utils_fork();
	if (p == 0) {
		ATF_REQUIRE_EQ_MSG(getlistaddr(fd), NULL, "no control/listaddress file should be null");
	}
	atf_utils_wait(p, EXIT_FAILURE, "", "");
	FILE *f = fopen("list/control/listaddress", "w+");
	fprintf(f, "plop\n");
	fclose(f);
	p = atf_utils_fork();
	if (p == 0) {
		ATF_REQUIRE_EQ_MSG(getlistaddr(fd), NULL, "no control/listaddress file should be null");
	}
	atf_utils_wait(p, EXIT_FAILURE, "", "");
	f = fopen("list/control/listaddress", "w+");
	fprintf(f, "plop@meh\n");
	fclose(f);
	ATF_REQUIRE_STREQ_MSG(getlistaddr(fd), "plop@meh", "list address not read properly");
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
	fclose(fopen("plop/test", "w+"));
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
	struct strlist stl = {};
	ATF_REQUIRE_EQ(getaddrsfromfile(&stl, NULL, 2), -1);
	fclose(fopen("list", "w+"));
	FILE *fp;
	fp = fopen("list", "r");
	ATF_REQUIRE_EQ(getaddrsfromfile(&stl, fp, 2), 0);
	fclose(fp);
	FILE *f = fopen("list", "w+");
	fprintf(f,"\n");
	fclose(f);
	fp = fopen("list", "r");
	ATF_REQUIRE_EQ(getaddrsfromfile(&stl, fp, 2), 0);
	f = fopen("list", "w+");
	fprintf(f,"mail1@fqdn\n");
	fclose(f);
	fp = fopen("list", "r");
	ATF_REQUIRE_EQ(getaddrsfromfile(&stl, fp, 2), 0);
	fclose(fp);
	ATF_REQUIRE_EQ_MSG(stl.count, 1, "%d is not 1", stl.count);
	ATF_REQUIRE_STREQ(stl.strs[0], "mail1@fqdn");
	free(stl.strs[0]);
	stl.count = 0;
	fp = fopen("list", "r");
	ATF_REQUIRE_EQ(getaddrsfromfile(&stl, fp, 0), -1);
	f = fopen("list", "w+");
	fprintf(f,"mail1@fqdn\nmail2@fqdn\nmail3@fqdn");
	fclose(f);
	fp = fopen("list", "r");
	ATF_REQUIRE(getaddrsfromfile(&stl, fp, 2) > 0);
	ATF_REQUIRE_EQ(stl.count, 2);
	ATF_REQUIRE_STREQ(stl.strs[0], "mail1@fqdn");
	ATF_REQUIRE_STREQ(stl.strs[1], "mail2@fqdn");
	free(stl.strs[0]);
	free(stl.strs[1]);
	stl.count = 0;
	ATF_REQUIRE_EQ(getaddrsfromfile(&stl, fp, 2), 0);
	ATF_REQUIRE_EQ_MSG(stl.count, 1, "%d is not 1", stl.count);
	ATF_REQUIRE_STREQ(stl.strs[0], "mail3@fqdn");
	fclose(fp);
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
	ATF_TP_ADD_TC(tp, write_mailbody_from_map);
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
	ATF_TP_ADD_TC(tp, send_mail_0);
	ATF_TP_ADD_TC(tp, send_mail_1);
	ATF_TP_ADD_TC(tp, send_mail_2);
	ATF_TP_ADD_TC(tp, send_mail_3);
	ATF_TP_ADD_TC(tp, send_mail_4);
	ATF_TP_ADD_TC(tp, send_mail_5);
	ATF_TP_ADD_TC(tp, send_mail_6);
	ATF_TP_ADD_TC(tp, send_mail_7);
	ATF_TP_ADD_TC(tp, send_mail_8);
	ATF_TP_ADD_TC(tp, getlistdelim);
	ATF_TP_ADD_TC(tp, getlistaddr);
	ATF_TP_ADD_TC(tp, statctrl);
	ATF_TP_ADD_TC(tp, is_subbed_in);
	ATF_TP_ADD_TC(tp, getaddrsfromfile);

	return (atf_no_error());
}
