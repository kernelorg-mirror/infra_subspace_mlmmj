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

#include <sys/types.h>
#include <sys/stat.h>

#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#include "strgen.h"
#include "ctrlvalue.h"
#include "mygetline.h"
#include "chomp.h"
#include "utils.h"
#include "log_error.h"
#include "xmalloc.h"

static char *ctrlvalat(int dfd, const char *subdir,
		const char *ctrlstr, int oneline)
{
	char *filename, *buf = NULL;
	int ctrlfd;
	size_t bufcap = 0;
	ssize_t buflen;
	FILE *fp;

	if (dfd == -1)
		return NULL;

	xasprintf(&filename, "%s/%s", subdir, ctrlstr);
	ctrlfd = openat(dfd, filename, O_RDONLY|O_CLOEXEC);
	free(filename);

	if(ctrlfd < 0)
		return NULL;

	fp = fdopen(ctrlfd, "r");
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
static char *ctrlval(const char *listdir, const char *subdir,
		const char *ctrlstr, int oneline)
{
	int dfd = open(listdir, O_DIRECTORY|O_CLOEXEC);
	char *ret = ctrlvalat(dfd, subdir, ctrlstr, oneline);
	if (dfd != -1)
		close(dfd);
	return (ret);
}

char *ctrlvalue(const char *listdir, const char *ctrlstr)
{
	return ctrlval(listdir, "control", ctrlstr, 1);
}

char *ctrlcontent(const char *listdir, const char *ctrlstr)
{
	return ctrlval(listdir, "control", ctrlstr, 0);
}

char *textcontent(const char *listdir, const char *ctrlstr)
{
	return ctrlval(listdir, "text", ctrlstr, 0);
}

intmax_t
ctrlim(const char *listdir, const char *ctrlstr, intmax_t min, intmax_t max,
    intmax_t fallback)
{
	const char *errstr;
	char *val = ctrlvalue(listdir, ctrlstr);
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
ctrluim(const char *listdir, const char *ctrlstr, uintmax_t min, uintmax_t max,
    uintmax_t fallback)
{
	const char *errstr;
	char *val = ctrlvalue(listdir, ctrlstr);
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
	char *val = ctrlvalat(dfd, "control", ctrlstr, 1);
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
