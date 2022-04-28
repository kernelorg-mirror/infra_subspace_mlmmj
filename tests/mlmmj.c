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

ATF_TC_WITHOUT_HEAD(random_int);
ATF_TC_WITHOUT_HEAD(chomp);
ATF_TC_WITHOUT_HEAD(mydirname);
ATF_TC_WITHOUT_HEAD(mybasename);
ATF_TC_WITHOUT_HEAD(mygetline);
ATF_TC_WITHOUT_HEAD(init_sock);
ATF_TC_WITHOUT_HEAD(genmsgid);
ATF_TC_WITHOUT_HEAD(lowercase);

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

	return (atf_no_error());
}
