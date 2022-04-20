/* Copyright (C) 2003 Mads Martin Joergensen <mmj at mmj.dk>
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <netdb.h>
#include <libgen.h>
#include <time.h>
#include <ctype.h>
#include <errno.h>
#include <locale.h>

#include "mlmmj.h"
#include "strgen.h"
#include "wrappers.h"
#include "memory.h"
#include "log_error.h"

char *random_str()
{
	size_t len = 17; /* sixteen hex digits plus terminating zero */
	char *dest = mymalloc(len);

	snprintf(dest, len, "%08x%08x", random_int(), random_int());
	
	return dest;
}

char *headerstr(const char *headertoken, const char *str)
{
	size_t len = strlen(headertoken) + strlen(str) + 2;
	char *dest = mymalloc(len);

	snprintf(dest, len, "%s%s\n", headertoken, str);

	return dest;
}

char *genlistname(const char *listaddr)
{
	size_t len;
	char *dest, *atsign;

	atsign = strchr(listaddr, '@');
	MY_ASSERT(atsign);
	len = atsign - listaddr + 1;
	dest = mymalloc(len);
	
	snprintf(dest, len, "%s", listaddr);

	return dest;
}

char *genlistfqdn(const char *listaddr)
{
	size_t len;
	char *dest, *atsign;

	atsign = strchr(listaddr, '@');
	MY_ASSERT(atsign);
	len = strlen(listaddr) - (atsign - listaddr);
	dest = mymalloc(len);
	snprintf(dest, len, "%s", atsign + 1);

	return dest;
}

char *concatstr(int count, ...)
{
	va_list arg;
        const char *str;
        int i;
        size_t len = 0;
        char *retstr;

        va_start(arg, count);

        for(i = 0; i < count; i++) {
                str = va_arg(arg, const char *);
                if(str)
                        len += strlen(str);
        }

        retstr = mymalloc(len + 1);
        retstr[0] = retstr[len] = 0;

        va_start(arg, count);

        for(i = 0; i < count; i++) {
                str = va_arg(arg, const char *);
                if(str)
                        strcat(retstr, str);
        }

        va_end(arg);

        return retstr;
}

char *hostnamestr()
{
	struct hostent *hostlookup;
	char *hostname = NULL;
	size_t len = 512;

	for (;;) {
		len *= 2;
		myfree(hostname);

		hostname = mymalloc(len);
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
			myfree(hostname);
			return mystrdup("localhost");
		}
		
		if (hostname[len-1] == '\0') {
			break;
		}
	}

	if (strchr(hostname, '.')) {
		/* hostname is FQDN */
		return hostname;
	}

	if ((hostlookup = gethostbyname(hostname))) {
		myfree(hostname);
		return mystrdup(hostlookup->h_name);
	}

	return hostname;
}

char *mydirname(const char *path)
{
	char *mypath, *dname, *ret;

	mypath = mystrdup(path);
	dname = dirname(mypath);
	ret = mystrdup(dname);

	/* We don't free mypath until we have strdup()'ed dname, because
	 * dirname() returns a pointer into mypath  -- mortenp 20040527 */
	myfree(mypath);

	return ret;
}

char *mybasename(const char *path)
{
	char *mypath, *bname, *ret;

	mypath = mystrdup(path);
	bname = basename(mypath);
	ret = mystrdup(bname);

	/* We don't free mypath until we have strdup()'ed bname, because
	 * basename() returns a pointer into mypath  -- mortenp 20040527 */
	myfree(mypath);
	
	return ret;
}

char *cleanquotedp(const char *qpstr)
{
	char *retstr;
	char qc[3];
	const char *c = qpstr;
	long qcval;
	int i = 0;
	size_t len;

	/* XXX: We only use this function for checking whether the subject
	 * prefix is only present, so the recoding is neither guaranteed
	 * complete nor correct */

	len = strlen(qpstr);
	retstr = mymalloc(len + 1);
	retstr[len] = '\0';
	qc[2] = '\0';
	while(c < qpstr+len) {
		switch(*c) {
			case '=':
				c++;
				if (!isxdigit(*c))
					break;
				qc[0] = *(c++);
				if (!isxdigit(*c))
					break;
				qc[1] = *(c++);
				qcval = strtol(qc, NULL, 16);
				if(qcval)
					retstr[i++] = (char)qcval;
				break;
			case '_':
				retstr[i++] = ' ';
				c++;
				break;
			default:
				retstr[i++] = *(c++);
				break;
		}
	}
	
	retstr[i] = '\0';

	return retstr;
}

char *genmsgid(const char *fqdn)
{
	char buf[256];

	snprintf(buf, sizeof(buf), "Message-ID: <%ld-%d-mlmmj-%08x@%s>\n",
			(long int)time(NULL), (int)getpid(), random_int(), fqdn);

	return mystrdup(buf);
}

char *gendatestr()
{
	time_t t;
	locale_t l = newlocale(LC_TIME_MASK, "C", NULL);
	struct tm lttm;
	char *timestr;

	/* 6 + 26 + ' ' + timezone which is 5 + '\n\0' == 40 */
	timestr = (char *)mymalloc(40);
	t = time(NULL);

	localtime_r(&t, &lttm);

	strftime_l(timestr, 40, "Date: %a, %d %b %Y %T %z", &lttm, l);
	freelocale(l);

	return timestr;
}
