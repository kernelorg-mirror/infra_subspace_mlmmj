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

static bool
addr_has_crlf(const char *addr)
{
	if (addr == NULL)
		return false;
	return (strchr(addr, '\r') != NULL || strchr(addr, '\n') != NULL);
}

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
	if (addr_has_crlf(from_addr)) {
		log_error(LOG_ARGS, "MAIL FROM address contains CR/LF, rejecting");
		return -1;
	}
	if(extra && extra[0] != '\0') {
		if(extra[0] == ' ') extra++;
		return (write_sock(sockfd, "FROM", "MAIL FROM:<%s> %s\r\n",
		    from_addr, extra));
	} else
		return (write_sock(sockfd, "FROM", "MAIL FROM:<%s>\r\n", from_addr));
}

int write_rcpt_to(int sockfd, const char *rcpt_addr)
{
	if (addr_has_crlf(rcpt_addr)) {
		log_error(LOG_ARGS, "RCPT TO address contains CR/LF, rejecting");
		return -1;
	}
	return (write_sock(sockfd, "TO", "RCPT TO:<%s>\r\n", rcpt_addr));
}

int
write_mailbody(int sockfd, FILE *fp, const char *tohdr)
{
	int c, next;
	bool addhdr = true;
	int dupfd = dup(sockfd);
	FILE *out = dupfd != -1 ? fdopen(dupfd, "w") : NULL;

	if (out == NULL) {
		if (dupfd != -1)
			close(dupfd);
		return -1;
	}

	while ((c = fgetc(fp)) != EOF) {
		if (c != '\n') {
			fputc(c, out);
			continue;
		}
		/* LF -> CRLF */
		fputs("\r\n", out);
		next = fgetc(fp);
		if (next == '.') {
			/*
			 * Dot-stuffing per RFC 5321 section 4.5.2:
			 * If a line starts with '.', insert an extra '.'.
			 */
			fputc('.', out);
			next = fgetc(fp);
			ungetc(next, fp);
			next = '.';
		}
		if (addhdr && tohdr != NULL && next == '\n') {
			fprintf(out, "To: %s\r\n", tohdr);
			addhdr = false;
		}
		if (next != EOF)
			ungetc(next, fp);
	}
	fflush(out);
	if (ferror(out)) {
		fclose(out);
		return -1;
	}
	fclose(out);
	return 0;
}

int write_dot(int sockfd)
{
	return write_sock(sockfd, "", "\r\n.\r\n");
}

int write_replyto(int sockfd, const char *replyaddr)
{
	if (addr_has_crlf(replyaddr)) {
		log_error(LOG_ARGS, "Reply-To address contains CR/LF, rejecting");
		return -1;
	}
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
