/*
 * Copyright (C) 2004 Mads Martin Joergensen <mmj at mmj.dk>
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

#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#include "ctrlvalue.h"
#include "chomp.h"
#include "utils.h"
#include "log_error.h"
#include "xmalloc.h"

static char *
ctrlval(int ctrlfd, const char *ctrlstr, int oneline)
{
	int fd;
	FILE *fp;
	size_t bufcap = 0;
	ssize_t buflen;
	char *buf = NULL;

	if(ctrlfd < 0)
		return NULL;

	fd = openat(ctrlfd, ctrlstr, O_RDONLY|O_CLOEXEC);
	if (fd == -1)
		return (NULL);

	fp = fdopen(fd, "r");
	if (fp == NULL) {
		close(ctrlfd);
		return (NULL);
	}

	if (oneline) {
		buflen = getline(&buf, &bufcap, fp);
		if (buflen > 0)
			chomp(buf);
	} else {
		buflen = getdelim(&buf, &bufcap, EOF, fp);
	}
	fclose(fp);

	return (buf);
}

char *
ctrlvalue(int ctrlfd, const char *ctrlstr)
{
	return (ctrlval(ctrlfd, ctrlstr, true));
}

char *
ctrlcontent(int ctrlfd, const char *ctrlstr)
{
	return (ctrlval(ctrlfd, ctrlstr, false));
}

char *
textcontent(int listfd, const char *ctrlstr)
{
	int fd = openat(listfd, "text", O_DIRECTORY|O_CLOEXEC);
	char *ret = ctrlval(fd, ctrlstr, false);
	close(fd);
	return (ret);
}

intmax_t
ctrlim(int ctrlfd, const char *ctrlstr, intmax_t min, intmax_t max,
    intmax_t fallback)
{
	const char *errstr;
	char *val = ctrlval(ctrlfd, ctrlstr, true);
	intmax_t ret;

	if (val == NULL)
		return (fallback);

	ret = strtoim(val, min, max, &errstr);
	if (errstr != NULL) {
		log_error(LOG_ARGS, "Invalid value for '%s': %s", ctrlstr,
		    errstr);
		ret = fallback;
	}
	free(val);
	return (ret);
}

uintmax_t
ctrluim(int ctrlfd, const char *ctrlstr, uintmax_t min, uintmax_t max,
    uintmax_t fallback)
{
	const char *errstr;
	char *val = ctrlval(ctrlfd, ctrlstr, true);
	uintmax_t ret;

	if (val == NULL)
		return (fallback);

	ret = strtouim(val, min, max, &errstr);
	if (errstr != NULL) {
		log_error(LOG_ARGS, "Invalid value for '%s': %s", ctrlstr,
		    errstr);
		ret = fallback;
	}
	free(val);
	return (ret);
}

time_t
ctrltimet(int dfd, const char *ctrlstr, time_t fallback)
{
	const char *errstr;
	char *val = ctrlval(dfd, ctrlstr, true);
	time_t ret;

	if (val == NULL)
		return (fallback);

	ret = strtotimet(val, &errstr);
	if (errstr != NULL) {
		log_error(LOG_ARGS, "Invalid value for '%s': %s", ctrlstr,
		    errstr);
		ret = fallback;
	}
	free(val);
	return (ret);
}
