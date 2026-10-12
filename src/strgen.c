/*
 * Copyright (C) 2003 Mads Martin Joergensen <mmj at mmj.dk>
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <netdb.h>
#include <libgen.h>
#include <time.h>
#include <ctype.h>
#include <errno.h>
#include <locale.h>
#if defined(__APPLE__)
#include <xlocale.h>
#endif

#include "xmalloc.h"
#include "sb.h"
#include "mlmmj.h"
#include "strgen.h"
#include "unistr.h"
#include "wrappers.h"
#include "log_error.h"

char *random_str(void)
{
	char *dest;

	xasprintf(&dest, "%08x%08x", random_int(), random_int());

	return dest;
}

int
mkuniqfileat(int dirfd, const char *prefix, mode_t mode, char **name_out)
{
	char *name = NULL;
	int fd;

	if (prefix == NULL)
		prefix = "";
	for (;;) {
		char *r = random_str();

		xasprintf(&name, "%s%s", prefix, r);
		free(r);
		fd = openat(dirfd, name, O_RDWR|O_CREAT|O_EXCL, mode);
		if (fd >= 0)
			break;
		free(name);
		name = NULL;
		if (errno != EEXIST) {
			if (name_out != NULL)
				*name_out = NULL;
			return (-1);
		}
	}
	if (name_out != NULL)
		*name_out = name;
	else
		free(name);
	return (fd);
}

char *genlistname(const char *listaddr)
{
	const char *atsign;

	atsign = strchr(listaddr, '@');
	MY_ASSERT(atsign);
	return (xstrndup(listaddr, atsign - listaddr));
}

const char *genlistfqdn(const char *listaddr)
{
	const char *atsign;

	atsign = strrchr(listaddr, '@');
	if (atsign == NULL)
		return (NULL);
	return (atsign + 1);
}

char *hostnamestr(void)
{
	struct hostent *hostlookup;
	char *hostname = NULL;
	size_t len = 512;

	for (;;) {
		len *= 2;
		free(hostname);

		hostname = xmalloc(len);
		hostname[len-1] = '\0';

		/* gethostname() is allowed to:
		 * a) return -1 and undefined in hostname
		 * b) return 0 and an unterminated string in hostname
		 * c) return 0 and a NUL-terminated string in hostname
		 *
		 * We keep expanding the buffer until the hostname is
		 * NUL-terminated (and pray that it is not truncated)
		 * or an error occurs.
		 */
		if (gethostname(hostname, len - 1)) {
			if (errno == ENAMETOOLONG) {
				continue;
			}
			free(hostname);
			return xstrdup("localhost");
		}

		/* Check if the hostname was truncated: gethostname() may
		 * return 0 even when truncation occurs (POSIX allows it).
		 * If the string fills the buffer, retry with a larger one. */
		if (strlen(hostname) < len - 1) {
			break;
		}
	}

	if (strchr(hostname, '.')) {
		/* hostname is FQDN */
		return hostname;
	}

	if ((hostlookup = gethostbyname(hostname))) {
		free(hostname);
		return xstrdup(hostlookup->h_name);
	}

	return hostname;
}

char *mydirname(const char *path)
{
	char *allocated;
	char *walk;

	if (path == NULL)
		return (xstrdup("."));
	allocated = xstrdup(path);
	walk = strrchr(allocated, '/');
	if (walk == NULL) {
		allocated[0] = '.';
		allocated[1] = '\0';
	} else {
		*walk = '\0';
	}
	return (allocated);
}

const char *mybasename(const char *path)
{
	const char *r;

	r = strrchr(path, '/');
	if (r == NULL)
		return (path);
	return (++r);
}

char *genmsgid(const char *fqdn)
{
	char *buf;

	xasprintf(&buf, "Message-ID: <%ld-%d-mlmmj-%08x@%s>\n",
			(long int)time(NULL), (int)getpid(), random_int(), fqdn);

	return (buf);
}

char *gendatestr(void)
{
	time_t t;
	locale_t l;
	struct tm lttm;
	char *timestr;

	/* 6 + 26 + ' ' + timezone which is 5 + '\n\0' == 40 */
	timestr = (char *)xmalloc(40);
	t = time(NULL);

	localtime_r(&t, &lttm);

	l = newlocale(LC_TIME_MASK, "C", NULL);
	if (l != (locale_t)0) {
		strftime_l(timestr, 40, "Date: %a, %d %b %Y %T %z", &lttm, l);
		freelocale(l);
	} else {
		strftime(timestr, 40, "Date: %a, %d %b %Y %T %z", &lttm);
	}

	return timestr;
}

static bool
ishexdigit(char c)
{
	return ((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') ||
	    (c >= 'A' && c <= 'F'));
}

static int
hexval(int c)
{
	if ('0' <= c && c <= '9')
		return (c - '0');
	return (10 + c - 'A');
}

static int
decode_char(const char *s)
{
	return (16 * hexval(toupper(s[1])) + hexval(toupper(s[2])));
}

char *
decode_qp(const char *qpstr, bool qformat)
{
	sb_t str = sb_init();

	while (*qpstr) {
		switch (*qpstr) {
		case '=':
			if (strlen(qpstr) < 2) {
				sb_cat_c(&str, *qpstr);
				break;
			}
			if (qpstr[1] == '\r' && qpstr[2] == '\n') {
				qpstr += 2;
				break;
			}
			if (qpstr[1] == '\n') {
				qpstr++;
				break;
			}
			if (!ishexdigit(qpstr[1]) || !ishexdigit(qpstr[2])) {
				sb_cat_c(&str, *qpstr);
				break;
			}
			sb_cat_c(&str, decode_char(qpstr));
			qpstr += 2;
			break;
		case '_':
			if (qformat) {
				sb_cat_c(&str, 0x20);
				break;
			}
			/* FALLTHROUGH */
		default:
			sb_cat_c(&str, *qpstr);
			break;
		}
		qpstr++;
	}

	return (sb_get(&str));
}

/*
 * Return true if the len-byte token tok is a well-formed RFC 2047
 * encoded word ("=?charset?encoding?encoded-text?="), that is one that
 * unistr_header_to_utf8() decodes instead of replacing with "???".
 */
static bool
is_encoded_word(const char *tok, size_t len)
{
	const char *p = tok, *end = tok + len;
	size_t fieldlen;
	int i;

	if (len < 7 || p[0] != '=' || p[1] != '?')
		return (false);
	p += 2;
	/* charset?encoding?encoded-text?= */
	for (i = 0; i < 3; i++) {
		const char *field = p;

		p = memchr(p, '?', (size_t)(end - p));
		if (p == NULL)
			return (false);
		fieldlen = (size_t)(p - field);
		if (fieldlen == 0)
			return (false);
		if (i == 1 && tolower((unsigned char)field[0]) != 'q' &&
		    tolower((unsigned char)field[0]) != 'b')
			return (false);
		p++;		/* skip the '?' */
	}
	/* the encoded word ends right after the third '?' with '=' */
	return (p + 1 == end && p[0] == '=');
}

/*
 * Return true if s contains only well-formed encoded words among the
 * tokens that look like one, and at least one such encoded word.
 */
static bool
has_encoded_word(const char *s)
{
	bool found = false;

	while (*s != '\0') {
		size_t len;

		s += strspn(s, " \t\n");
		if (*s == '\0')
			break;
		len = strcspn(s, " \t\n");
		if (is_encoded_word(s, len)) {
			found = true;
		} else if (s[0] == '=' && s[1] == '?') {
			/* a broken encoded word, unistr_header_to_utf8()
			 * would turn it into "???" */
			return (false);
		}
		s += len;
	}
	return (found);
}

/*
 * Return a copy of the From: header value with special characters
 * replaced to make it safe for use in a display name:
 *   '<' -> '(',  '>' -> ')',  '@' -> '=',
 *   ',' -> ' ',  ';' -> ' ',  truncate at '\n'
 * The result is encoded as an RFC 2047 encoded word when it contains
 * characters that are not allowed in a display name, so that it is
 * displayed as is, and not as an RFC 5322 comment.
 * Returns NULL if s is NULL or empty.
 */
char *
disarm_email(const char *from)
{
	char *cpy, *enc, *p;

	if (from == NULL || *from == '\0')
		return (NULL);
	/* The value is taken verbatim from the message, so a non-ASCII
	 * display name may already be an RFC 2047 encoded word.  Decode
	 * it first: unistr_utf8_to_header() below expects UTF-8 and would
	 * otherwise encode the encoded word a second time (#118).  Only
	 * decode when an encoded word is present, as
	 * unistr_header_to_utf8() is not meant to handle raw 8-bit text. */
	if (has_encoded_word(from))
		cpy = unistr_header_to_utf8(from);
	else
		cpy = xstrdup(from);
	/* Skip leading whitespace (e.g. the space after the From: colon) */
	for (p = cpy; *p != '\0' && isspace((unsigned char)*p); p++)
		;
	if (p != cpy)
		memmove(cpy, p, strlen(p) + 1);
	for (p = cpy; *p != '\0'; p++) {
		switch (*p) {
		case '<':
			*p = '(';
			break;
		case '>':
			*p = ')';
			break;
		case '@':
			*p = '=';
			break;
		case ',':
		case ';':
			/* A comma or semicolon left in the display name makes some
			 * providers see more than one address in the From: header and
			 * reject the mail, even when the value is an RFC 2047 encoded
			 * word.
			 * Replace them with a space. */
			*p = ' ';
			break;
		case '\n':
			*p = '\0';
			break;
		}
	}
	enc = unistr_utf8_to_header(cpy);
	free(cpy);

	return (enc);
}

bool
splitlistaddr(const char *listaddr, char **listname, const char **listfqdn)
{
	char *allocated, *walk;
	allocated = xstrdup(listaddr);

	walk = strchr(allocated, '@');
	if (walk == NULL) {
		free(allocated);
		return (false);
	}
	*walk = '\0';
	walk++;
	*listname = allocated;
	*listfqdn = walk;

	return (true);
}
