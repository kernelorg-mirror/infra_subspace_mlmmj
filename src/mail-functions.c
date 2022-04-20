/* Copyright (C) 2002, 2003 Mads Martin Joergensen <mmj at mmj.dk>
 *
 * $Id$
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

#include "xmalloc.h"
#include "mail-functions.h"
#include "wrappers.h"
#include "log_error.h"

/* "EHLO \r\n" has length 7 */
#define EXTRA_EHLO_LEN 7
int write_ehlo(int sockfd, const char *hostname)
{
	size_t bytes_written;
	
	bytes_written = dprintf(sockfd, "EHLO %s\r\n", hostname);
	if(bytes_written < 0) {
		log_error(LOG_ARGS, "Could not write EHLO");
		return errno;
	}
	return 0;
}
/* "HELO \r\n" has length 7 */
#define EXTRA_HELO_LEN 7
int write_helo(int sockfd, const char *hostname)
{
	size_t bytes_written;
	
	bytes_written = dprintf(sockfd, "HELO %s\r\n", hostname);
	if(bytes_written < 0) {
		log_error(LOG_ARGS, "Could not write HELO");
		return errno;
	}
	return 0;
}
/* "MAIL FROM:<> \r\n" has length 15 */
#define EXTRA_FROM_LEN 15
int write_mail_from(int sockfd, const char *from_addr, const char *extra)
{
	size_t bytes_written;

	if(extra && extra[0] != '\0') {
		if(extra[0] == ' ') extra++;
		bytes_written = dprintf(sockfd, "MAIL FROM:<%s> %s\r\n", from_addr,
				extra);
	} else
		bytes_written = dprintf(sockfd, "MAIL FROM:<%s>\r\n", from_addr);
	if(bytes_written < 0) {
		log_error(LOG_ARGS, "Could not write FROM");
		return errno;
	}
	return 0;
}

/* "RCPT TO:<>\r\n" has length 12 */
#define EXTRA_RCPT_LEN 12
int write_rcpt_to(int sockfd, const char *rcpt_addr)
{
	size_t bytes_written;
	
	bytes_written = dprintf(sockfd, "RCPT TO:<%s>\r\n", rcpt_addr);
	if(bytes_written < 0) {
		log_error(LOG_ARGS, "Could not write TO");
		return errno;
	}
	return 0;
}


int write_mailbody_from_map(int sockfd, char *mapstart, size_t size,
			    const char *tohdr)
{
	char *cur, *next;
	char newlinebuf[3];
	int i = 1;

	for(next = cur = mapstart; next < mapstart + size; next++) {
		if(*next == '\n') {
			if(writen(sockfd, cur, next - cur) < 0) {
				log_error(LOG_ARGS, "Could not write mail");
				return -1;
			}
			newlinebuf[0] = '\r';
			newlinebuf[1] = '\n';
			if(next < (mapstart + size - 1) && *(next+1) == '.') {
				newlinebuf[2] = '.';
			}
			if(dprintf(sockfd, "%s", newlinebuf) < 0) {
				log_error(LOG_ARGS, "Could not write mail");
				return -1;
			}
			if(i && tohdr && *(next+1) == '\n') { /* add To: header */
				if(dprintf(sockfd, "%s", tohdr) < 0) {
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

char *get_preppedhdrs_from_map(char *mapstart, size_t *hlen)
{
	char *cur, *next, *endhdrs, *retstr, *r;
	const char newlinebuf[] = "\r\n";
	size_t hdrlen, n = 0;

	endhdrs = strstr(mapstart, "\n\n");
	if(endhdrs == NULL)
		return NULL; /* The map doesn't map a file with a mail */

	hdrlen = endhdrs - mapstart + 1;

	for(next = cur = mapstart; next < mapstart + hdrlen; next++)
		if(*next == '\n')
			n++;

	retstr = xmalloc(hdrlen + n);
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

	return retstr;
}

char *get_prepped_mailbody_from_map(char *mapstart, size_t size, size_t *blen)
{
	char *cur, *next, *endhdrs, *retstr, *r;
	char newlinebuf[3];
	size_t bodylen, len, n = 0;

	endhdrs = strstr(mapstart, "\n\n");
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

	retstr = xmalloc(bodylen + n);
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

	return retstr;
}

int write_dot(int sockfd)
{
	size_t bytes_written;

	bytes_written = dprintf(sockfd, "\r\n.\r\n");	
	if(bytes_written < 0)
		return errno;

	return 0;
}

/* "\r\n" has length 2 */
#define EXTRA_CUSTOM_LEN 2
int write_custom_line(int sockfd, const char *line)
{
	size_t bytes_written;
	
	bytes_written = dprintf(sockfd, "%s\r\n", line);
	if(bytes_written < 0) {
		log_error(LOG_ARGS, "Could not write customline");
		return errno;
	}
	return 0;
}

/* "Reply-To: \r\n" has length 12 */
#define EXTRA_REPLYTO_LEN 12
int write_replyto(int sockfd, const char *replyaddr)
{
	size_t bytes_written;
	
	bytes_written = dprintf(sockfd, "Reply-To: %s\r\n", replyaddr);
	if(bytes_written < 0) {
		log_error(LOG_ARGS, "Could not write Reply-To header");
		return errno;
	}
	return 0;
}

int write_data(int sockfd)
{
	if(write_custom_line(sockfd, "DATA")) {
		log_error(LOG_ARGS, "Could not write DATA");
		return errno;
	}

	return 0;
}

int write_quit(int sockfd)
{
	if(write_custom_line(sockfd, "QUIT")) {
		log_error(LOG_ARGS, "Could not write QUIT");
		return errno;
	}

	return 0;
}

int write_rset(int sockfd)
{
	if(write_custom_line(sockfd, "RSET")) {
		log_error(LOG_ARGS, "Could not write RSET");
		return errno;
	}

	return 0;
}
