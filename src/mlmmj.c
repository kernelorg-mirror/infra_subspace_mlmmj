/*
 * Copyright (C) 2021-2022 Baptiste Daroussin <bapt@FreeBSD.org>
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

#include <sys/mman.h>
#include <sys/param.h>
#include <sys/stat.h>

#include <ctype.h>
#include <dirent.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "mlmmj.h"
#include "utils.h"
#include "log_error.h"
#include "xmalloc.h"
#include "chomp.h"

bool
parse_lastdigest(char *line, long *lastindex, time_t *lasttime,
     long *lastissue, const char **errstr)
{
	size_t nfield = 0;
	char *walk;

	*errstr = NULL;

	if (line == NULL) {
		*lastindex = 0;
		*lasttime = 0;
		*lastissue = 0;
		return (true);
	}
	walk = line;
	while (strsep(&walk, ":") != NULL)
		nfield++;
	if (nfield < 2 || nfield > 3) {
		*errstr = "Invalid format, expecting 2 or 3 fields";
		return (false);
	}
	*lastindex = strtoim(line, 0, LONG_MAX, errstr);
	if (*errstr != NULL) {
		*errstr = "Invalid value for lastindex";
		return (false);
	}
	walk = line + strlen(line) + 1;
	*lasttime = strtotimet(walk, errstr);
	if (*errstr != NULL) {
		*errstr = "Invalid value for lasttime";
		return (false);
	}
	if (nfield == 2) {
		*lastissue = 0;
		return true;
	}
	walk = walk + strlen(walk) + 1;
	*lastissue = strtoim(walk, LONG_MIN, LONG_MAX, errstr);
	if (*errstr != NULL) {
		*errstr = "Invalid value for lastissue";
		return (false);
	}
	return (true);
}
