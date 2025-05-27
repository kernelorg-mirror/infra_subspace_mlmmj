/*
 * Copyright (C) 2023 Baptiste Daroussin <bapt@FreeBSD.org>
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

#include "mlmmj_tests.h"

#include <stdint.h>
#include <sys/socket.h>
#include <sys/stat.h>

#include <arpa/inet.h>
#include <netinet/in.h>

#include <atf-c.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>

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

	if (complete)
		atf_utils_create_file("list/control/listaddress", "test@test");
}

int
fakesmtp(int pipe)
{
	int s;
	struct sockaddr_in me = { 0 };
	s = socket(AF_INET, SOCK_STREAM, 0);
	if (s < 0)
		exit(1);
	if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &(int) { 1 }, sizeof(int)) != 0)
		exit(2);
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

void
read_print_reply(int fd, const char **replies, size_t nreplies)
{
	char *r;

	for (size_t i = 0; i < nreplies; i++) {
		r = atf_utils_readline(fd);
		if (r == NULL)
			atf_tc_fail("Impossible to read response %zu", i);
		printf("%s\n", r);
		if (replies[i] != NULL)
			dprintf(fd, "%s", replies[i]);
		free(r);
	}
}

pid_t
single_mail_reception(int fd)
{
	pid_t p = atf_utils_fork();
	if (p == 0) {
		int s = fakesmtp(fd);
		int c;
		char *r;
		struct sockaddr_in cl;
		socklen_t clsize = sizeof(struct sockaddr_in);
		c = accept(s, (struct sockaddr *) &cl, &clsize);
		if (c == -1)
			exit(5);
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
			"250 2.1.0 OK\n",
			"250 2.1.0 OK\n",
			"350 2.1.0 OK\n",
		};
		read_print_reply(c, replies, NELEM(replies));
		while ((r = atf_utils_readline(c)) != NULL) {
			printf("%s\n", r);
			if (strcmp(r, ".\r") == 0) {
				dprintf(c, "250 2.1.0 OK\n");
				break;
			}
		}
		const char *rep = "221 2.0.0 bye\n";
		read_print_reply(c, &rep, 1);
		exit(0);
	}
	return (p);
}
