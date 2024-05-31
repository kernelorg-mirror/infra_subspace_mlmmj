/*
 * Copyright (C) 2002, 2003 Mads Martin Joergensen <mmj at mmj.dk>
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

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <fcntl.h>
#include <stdbool.h>

#include "xmalloc.h"
#include "mail-functions.h"
#include "wrappers.h"
#include "log_error.h"

static int
write_sock(int sockfd, const char *errstr, const char *fmt, ...) {
	int bytes_written;
	va_list ap;

	va_start(ap, fmt);
	bytes_written = vdprintf(sockfd, fmt, ap);
	va_end(ap);
	if(bytes_written < 0) {
		log_error(LOG_ARGS, "Could not write %s", errstr);
		return -1;
	}
	return 0;
}

int write_ehlo(int sockfd, const char *hostname)
{
	return (write_sock(sockfd, "EHLO", "EHLO %s\r\n", hostname));
}

int write_helo(int sockfd, const char *hostname)
{
	return (write_sock(sockfd, "HELO", "HELO %s\r\n", hostname));
}

int write_mail_from(int sockfd, const char *from_addr, const char *extra)
{
	if(extra && extra[0] != '\0') {
		if(extra[0] == ' ') extra++;
		return (write_sock(sockfd, "FROM", "MAIL FROM:<%s> %s\r\n",
		    from_addr, extra));
	} else
		return (write_sock(sockfd, "FROM", "MAIL FROM:<%s>\r\n", from_addr));
}

int write_rcpt_to(int sockfd, const char *rcpt_addr)
{
	return (write_sock(sockfd, "TO", "RCPT TO:<%s>\r\n", rcpt_addr));
}

void
write_mailbody(int sockfd, FILE *fp, const char *tohdr)
{
	int c, next;
	bool addhdr = true;
	while ((c = fgetc(fp)) != EOF) {
		if (c != '\n') {
			dprintf(sockfd, "%c", c);
			continue;
		}
		next = fgetc(fp);
		dprintf(sockfd, "\r\n");
		if (next == '.') {
			/*
			 * o Before sending a line of mail text, the SMTP client checks the
			 * first character of the line. If it is a period, one additional
			 * period is inserted at the beginning of the line.
			 * 
			 * o When a line of mail text is received by the SMTP server, it checks
			 * the line. If the line is composed of a single period, it is
			 * treated as the end of mail indicator. If the first character is a
			 * period and there are other characters on the line, the first
			 * character is deleted.
			 */
			c = fgetc(fp);
			dprintf(sockfd, ".");
			ungetc(c, fp);
		}
		if (addhdr && tohdr != NULL && next == '\n') {
			dprintf(sockfd, "To: %s\r\n", tohdr);
			addhdr = false;
		}
		ungetc(next, fp);
	}
}

int write_dot(int sockfd)
{
	return write_sock(sockfd, "", "\r\n.\r\n");
}

int write_replyto(int sockfd, const char *replyaddr)
{
	return write_sock(sockfd, "Reply-To header", "Reply-To: %s\r\n", replyaddr);
}

int write_data(int sockfd)
{
	return write_sock(sockfd, "DATA", "DATA\r\n");
}

int write_quit(int sockfd)
{
	return write_sock(sockfd, "QUIT", "QUIT\r\n");
}

int write_rset(int sockfd)
{
	return write_sock(sockfd, "RSET", "RSET\r\n");
}
