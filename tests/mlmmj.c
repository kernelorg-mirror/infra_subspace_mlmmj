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
#include <sys/limits.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdbool.h>
#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

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
	pid_t p = atf_utils_fork();
	if (p == 0) {
		int s, c;
		struct sockaddr_in me, cl;
		socklen_t clsize;
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

		/* Now we can accept incoming connections one
		 *               at a time using accept(2). */

		c = accept(s, (struct sockaddr *) &cl,
				&clsize);
		if (c == -1)
			exit(5);
		exit(0);
	}
	sleep(1);
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
		exec_or_die("/usr/bin/true", (char *)NULL);
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
	val = exec_and_wait("/usr/bin/true", NULL);
	ATF_REQUIRE_EQ(val, 0);
	val = exec_and_wait("/usr/bin/false", NULL);
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
	if (!atf_utils_compare_file("out.txt", "line\r\ntest@plop.meh\r\n")) {
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
	if (!atf_utils_compare_file("out.txt", "line\r\ntest@plop.meh\r\nline2\r\n\r\n")) {
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

	return (atf_no_error());
}
