/*
 * Copyright (C) 2004 Mads Martin Joergensen <mmj at mmj.dk>
 * Copyright (C) 2023 Baptiste Daroussin <bapt@FreeBSD.org>
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

#include <fcntl.h>

#include "xmalloc.h"
#include "ctrlvalues.h"
#include "chomp.h"
#include "mlmmj.h"


typedef int (cmpf)(const char *s1, const char *s2);

strlist *ctrlvalues(int ctrlfd, const char *ctrlstr)
{
	char *line = NULL;
	size_t linecap = 0;
	FILE *f;
	int fd;
	strlist *ret;

	fd = openat(ctrlfd, ctrlstr, O_RDONLY|O_CLOEXEC);
	if (fd == -1 || (f = fdopen(fd, "r")) == NULL)
		return (NULL);

	ret = xcalloc(1, sizeof(*ret));
	while (getline(&line, &linecap, f) > 0) {
		chomp(line);
		/* Ignore empty lines */
		if (*line == '\0')
			continue;
		vec_push(ret, xstrdup(line));
	}
	free(line);
	fclose(f);

	return ret;
}

bool
ctrlvalues_contains(int ctrlfd, const char *ctrlstr, const char *comp, bool casesensitive)
{
	char *line = NULL;
	size_t linecap = 0;
	FILE *f;
	int fd;
	cmpf *cmp;

	cmp = casesensitive ? strcmp : strcasecmp;

	fd = openat(ctrlfd, ctrlstr, O_RDONLY|O_CLOEXEC);
	if (fd == -1 || (f = fdopen(fd, "r")) == NULL)
		return (false);
	while (getline(&line, &linecap, f) > 0) {
		chomp(line);
		if (*line == '\0')
			continue;
		if (cmp(comp, line) == 0) {
			free(line);
			fclose(f);
			return (true);
		}
	}

	free(line);
	fclose(f);
	return (false);
}
