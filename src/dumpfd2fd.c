/*
 * Copyright (C) 2004 Mads Martin Joergensen <mmj at mmj dot dk>
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

#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdbool.h>

#include "config.h"

#define DUMPBUF 4096

static int
_copy_file(int from, int to)
{
	char buf[DUMPBUF];
	ssize_t r, wresid, w = 0;
	char *bufp;
	r = read(from, buf, DUMPBUF);
	if (r < 0)
		return (r);
	for (bufp = buf, wresid = r; ; bufp += w, wresid -= w) {
		w = write(to, bufp, wresid);
		if (w <= 0)
			break;
		if (w >= (ssize_t) wresid)
			break;
	}
	return (w < 0 ? w : r);
}

int dumpfd2fd(int from, int to)
{
#ifdef HAVE_COPY_FILE_RANGE2
	bool cfr = true;
#endif
	int r;

	do {
#ifdef HAVE_COPY_FILE_RANGE2
		if (cfr) {
			r = copy_file_range(from, NULL, to, NULL, SSIZE_MAX, 0);
			if (r < 0 && errno == EINVAL) {
				printf("la\n");
				/* probably a non seekable FD */
				cfr = false;
			}
		}
		if (!cfr) {
#endif
			r = _copy_file(from, to);
#ifdef HAVE_COPY_FILE_RANGE2
		}
#endif
	} while (r > 0);

	return (r);
}
