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

#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#include "incindexfile.h"
#include "log_error.h"
#include "xmalloc.h"
#include "utils.h"

int incindexfile(int listfd)
{
	int fd, index = 0;
	FILE *fp;
	char *line = NULL;
	size_t linecap = 0;
	const char *errstr;

	fd = openat(listfd, "index", O_RDWR|O_CREAT, S_IRUSR|S_IWUSR);
	if(fd == -1 && !lock(fd, true)) {
		log_error(LOG_ARGS, "Error opening index file");
		return 0;
	}

	fp = fdopen(fd, "r+");
	if (fp == NULL) {
		log_error(LOG_ARGS, "Error fdopening index file");
		close(fd);
		return (0);
	}
	index = 0;
	if (getline(&line, &linecap, fp) > 0) {
		/* eliminate everything which is not a number */
		line[strspn(line, "0123456789")] = '\0';
		index = strtoim(line, 0, INT_MAX, &errstr);
		if (errstr != NULL) {
			log_error(LOG_ARGS, "Error reading index file: invalid "
			    "line: %s", line);
			free(line);
			return (0);
		}
		free(line);
	}
	index++;
	rewind(fp);
	fprintf(fp, "%d", index);
	fclose(fp); /* Lock is also released */

	return index;
}
