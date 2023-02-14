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

int write_mailbody_from_map(int sockfd, char *mapstart, size_t size,
			    const char *tohdr)
{
	char *cur, *next;
	char newlinebuf[3];
	size_t len;
	int i = 1;

	for(next = cur = mapstart; next < mapstart + size; next++) {
		if(*next == '\n') {
			if(dprintf(sockfd, "%.*s", (int)(next - cur), cur) < 0) {
				log_error(LOG_ARGS, "Could not write mail");
				return -1;
			}
			newlinebuf[0] = '\r';
			newlinebuf[1] = '\n';
			len = 2;
			if(next < (mapstart + size - 1) && *(next+1) == '.') {
				newlinebuf[2] = '.';
				len++;
			}
			if(dprintf(sockfd, "%.*s", (int)len, newlinebuf) < 0) {
				log_error(LOG_ARGS, "Could not write mail");
				return -1;
			}
			if(i && tohdr && *(next+1) == '\n') { /* add To: header */
				if(dprintf(sockfd, "To: %s\r\n", tohdr) < 0) {
					log_error(LOG_ARGS, "Could not write"
							    " To: header");
					return -1;
				}
				i = 0; /* Make sure we don't write it again */
			}
			cur = next + 1;
		}
	}

	return 0;
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
		if (addhdr && tohdr != NULL && next == '\n') {
			dprintf(sockfd, "To: %s\r\n", tohdr);
			addhdr = false;
		}
		ungetc(next, fp);
	}
}

char *get_preppedhdrs_from_map(char *mapstart, size_t maplen, size_t *hlen)
{
	char *cur, *next, *endhdrs, *retstr, *r;
	const char newlinebuf[] = "\r\n";
	size_t hdrlen, n = 0;

	endhdrs = strnstr(mapstart, "\n\n", maplen);
	if(endhdrs == NULL)
		return NULL; /* The map doesn't map a file with a mail */

	hdrlen = endhdrs - mapstart + 1;

	for(next = cur = mapstart; next < mapstart + hdrlen; next++)
		if(*next == '\n')
			n++;

	retstr = xmalloc(hdrlen + n + 1);
	*hlen = hdrlen + n;	
	r = retstr;	

	for(next = cur = mapstart; next < mapstart + hdrlen; next++) {
		if(*next == '\n') {
			strncpy(r, cur, next - cur);
			r += next - cur;
			strncpy(r, newlinebuf, 2);
			r += 2;
			cur = next + 1;
		}
	}
	retstr[*hlen] = '\0';

	return retstr;
}

char *get_prepped_mailbody_from_map(char *mapstart, size_t size, size_t *blen)
{
	char *cur, *next, *endhdrs, *retstr, *r;
	char newlinebuf[3];
	size_t bodylen, len, n = 0;

	endhdrs = strnstr(mapstart, "\n\n", size);
	if(endhdrs == NULL)
		return NULL; /* The map doesn't map a file with a mail */

	endhdrs++; /* Skip the first newline, it's in hdrs */

	bodylen = size - (endhdrs - mapstart);
	
	for(next = cur = endhdrs; next < mapstart + size; next++) {
		if(*next == '\n') {
			n++;
			if((next < mapstart + size - 1) && *(next+1) == '.')
				n++;
		}
	}

	retstr = xmalloc(bodylen + n +1);
	*blen = bodylen + n;
	r = retstr;
	
	for(next = cur = endhdrs; next < mapstart + size; next++) {
		if(*next == '\n') {
			strncpy(r, cur, next - cur);
			r += next - cur;
			newlinebuf[0] = '\r';
			newlinebuf[1] = '\n';
			len = 2;
			if((next < mapstart + size - 1) && *(next+1) == '.') {
				newlinebuf[2] = '.';
				len = 3;
			}
			strncpy(r, newlinebuf, len);
			r += len;
			cur = next + 1;
		}
	}
	retstr[*blen] = '\0';

	return retstr;
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
