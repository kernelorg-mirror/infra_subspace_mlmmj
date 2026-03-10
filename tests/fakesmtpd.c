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

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include <err.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>

#ifndef NELEM
#define NELEM(array) (sizeof(array) / sizeof((array)[0]))
#endif

static struct replies {
	const char *cmd;
	const char *reply;
} reps [] = {
	{ "EHLO", "250-hostname.net\n250 CHUNKING\n" },
	{ "QUIT", "221 2.0.0 bye\n" },
	{ "MAIL FROM", "250 2.1.0 Ok\n"},
	{ "RCPT TO", "250 2.1.0 Ok\n" },
	{ "DATA", "354 End data with <CR><LF>.<CR><LF>\n" },
};

static volatile sig_atomic_t got_signal = 0;

static void
signal_handler(int sig)
{
	(void)sig;
	got_signal = 1;
}

int
main(void)
{
	int s, c;
	FILE *f;
	socklen_t clsize = sizeof(struct sockaddr_in);
	struct sockaddr_in me = { 0 }, cl;
	socklen_t melen = sizeof(me);
	char *line = NULL;
	size_t linecap = 0;
	int mailnum = 0;

	s = socket(AF_INET, SOCK_STREAM, 0);
	if (s < 0)
		err(EXIT_FAILURE, "socket");
	int opt = 1;
	if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) != 0)
		err(EXIT_FAILURE, "setsockopt");
	me.sin_family = AF_INET;
	me.sin_addr.s_addr = inet_addr("127.0.0.1");
	me.sin_port = htons(0);
	if (bind(s, (struct sockaddr *) &me, sizeof(me)) == -1)
		err(EXIT_FAILURE, "bind");
	if (getsockname(s, (struct sockaddr *) &me, &melen) == -1)
		err(EXIT_FAILURE, "getsockname");
	if (listen(s, 1) == -1)
		err(EXIT_FAILURE, "listen");

	/* Write assigned port before daemonizing so the parent can read it */
	f = fopen("fakesmtp.port", "w");
	if (f == NULL)
		err(EXIT_FAILURE, "fopen port");
	fprintf(f, "%d\n", ntohs(me.sin_port));
	fclose(f);

	if (daemon(1, 0) != 0)
		err(EXIT_FAILURE, "daemon");

	signal(SIGTERM, signal_handler);
	signal(SIGINT, signal_handler);

	f = fopen("fakesmtp.pid", "w+");
	if (f == NULL)
		err(EXIT_FAILURE, "fopen pid");
	fprintf(f, "%d\n", getpid());
	fclose(f);
	for (;;) {
		char *mail;
		FILE *fp, *fp2;
		c = accept(s, (struct sockaddr *) &cl, &clsize);
		if (c == -1) {
			if (got_signal)
				break;
			err(EXIT_FAILURE, "accept");
		}

		mailnum++;
		asprintf(&mail, "mail-%d.txt", mailnum);
		fp = fopen(mail, "w+");
		fp2 = fopen("debugmail.txt", "w+");
		free(mail);
		f = fdopen(c, "rb");
		dprintf(c, "220 me fake smtp\n");
		fprintf(fp2, "==>: 220 me fake smtp\n");
		bool data = false;
		while (getline(&line, &linecap, f) > 0) {
			fprintf(fp, "%s", line);
			fprintf(fp2, "<==: %s", line);
			if (!data) {
				for (size_t i = 0; i < NELEM(reps); i++) {
					if (strncmp(reps[i].cmd, line, strlen(reps[i].cmd)) == 0) {
						fprintf(fp2, "==>: %s", reps[i].reply);
						dprintf(c, "%s", reps[i].reply);
						if (strcmp(reps[i].cmd, "DATA") == 0)
							data = true;
						if (strcmp(reps[i].cmd, "QUIT") == 0)
							goto next;
						goto nextline;
					}
				}
				fprintf(fp2, "==>: %s", "500 5.5.2 Error\n");
				dprintf(c, "500 5.5.2 Error: command not recognized\n");
			}
nextline:
			if (data && (strcmp(line, ".\r\n") == 0 || strcmp(line, ".\n") == 0)) {
				data = false;
				fprintf(fp2, "==>: %s", "250 2.1.0 Ok\n");
				dprintf(c, "250 2.1.0 Ok\n");
			}
		}
next:
		fclose(fp);
		fclose(fp2);
		fclose(f);
	}

	free(line);
	close(s);
	unlink("fakesmtp.pid");
	unlink("fakesmtp.port");

	return (EXIT_SUCCESS);

}
