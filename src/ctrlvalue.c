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
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#include "strgen.h"
#include "ctrlvalue.h"
#include "mygetline.h"
#include "chomp.h"
#include "memory.h"
#include "utils.h"
#include "log_error.h"

static char *ctrlval(const char *listdir, const char *subdir,
		const char *ctrlstr, int oneline)
{
	char *filename, *value = NULL;
	int ctrlfd, i;

	if(listdir == NULL)
		return NULL;

	filename = concatstr(5, listdir, "/", subdir, "/", ctrlstr);
	ctrlfd = open(filename, O_RDONLY);
	free(filename);

	if(ctrlfd < 0)
		return NULL;

	if (oneline) {
		value = mygetline(ctrlfd);
		chomp(value);
	} else {
		value = mygetcontent(ctrlfd);
		i = strlen(value) - 1;
		if (i >= 0 && value[i] == '\n') {
			value[i] = '\0';
			i--;
		}
		if (i >= 0 && value[i] == '\r') {
			value[i] = '\0';
			i--;
		}
	}
	close(ctrlfd);

	return value;
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
	if (errstr != NULL)
		log_error(LOG_ARGS, "Invalid value for '%s': %s", ctrlstr,
		    errstr);
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
	if (errstr != NULL)
		log_error(LOG_ARGS, "Invalid value for '%s': %s", ctrlstr,
		    errstr);
	free(val);
	return (ret);
}

time_t
ctrltimet(const char *listdir, const char *ctrlstr, time_t fallback)
{
	const char *errstr;
	char *val = ctrlvalue(listdir, ctrlstr);
	time_t ret;

	if (val == NULL)
		return (fallback);

	ret = strtotimet(val, &errstr);
	if (errstr != NULL) {
		log_error(LOG_ARGS, "Invalid value for '%s': %s", ctrlstr,
		    errstr);
		return (fallback);
	}
	free(val);
	return (ret);
}
