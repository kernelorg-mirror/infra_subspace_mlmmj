/*
 * Copyright (C) 2004 Mads Martin Joergensen <mmj at mmj dot dk>
 * Copyright (C) 2023-2025 Baptiste Daroussin <bapt@FreeBSD.org>
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
#include <limits.h>
#include <ctype.h>

#include <stdio.h>
#include <string.h>

#include "config.h"
#include "log_error.h"
#include "xmalloc.h"

#define DUMPBUF 4096

int
copy_file(int from, int to, size_t bufsiz)
{
	char buf[bufsiz];
	ssize_t r, wresid, w = 0;
	char *bufp;
	r = read(from, buf, bufsiz);
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
#ifdef HAVE_COPY_FILE_RANGE
	bool cfr = true;
#endif
	int r;

	do {
#ifdef HAVE_COPY_FILE_RANGE
		if (cfr) {
			r = copy_file_range(from, NULL, to, NULL, SSIZE_MAX, 0);
			if (r < 0 && (errno == EINVAL || errno == EXDEV)) {
				/* probably a non seekable FD */
				cfr = false;
			}
		}
		if (!cfr) {
#endif
			r = copy_file(from, to, DUMPBUF);
#ifdef HAVE_COPY_FILE_RANGE
		}
#endif
	} while (r > 0);

	return (r);
}

int process_headers(int infd, FILE *outf, const char *from,
    const char *disarmedfrom, bool skipfrom)
{
	char *line = NULL, *p, *to_be_subst, *new_line;
	size_t linecap = 0;
	int dupfd;
	FILE *f;

	dupfd = dup(infd);
	if (dupfd == -1)
		return -1;
	f = fdopen(dupfd, "r");
	if (f == NULL) {
		close(dupfd);
		return -1;
	}

	while (getline(&line, &linecap, f) > 0) {
		/* stop at end of headers (empty line) */
		p = line;
		while (isspace(*p))
			p++;
		if (*p == '\0')
			break;
		/* Some callers (the digest sender) already set a From:
		 * header and must not get a second one from the custom
		 * headers */
		if (skipfrom && strncasecmp(p, "From:", 5) == 0)
			continue;
		to_be_subst = strstr(p, "$posteraddr$");
		if (to_be_subst != NULL) {
			*to_be_subst = '\0';
			to_be_subst += 12;
			xasprintf(&new_line, "%s%s%s", p, from, to_be_subst);
			if (fprintf(outf, "%s", new_line) < 0) {
				log_error(LOG_ARGS, "Could not write headers");
				free(new_line);
				free(line);
				fclose(f);
				return -1;
			}
			free(new_line);
		} else if (disarmedfrom != NULL &&
		    (to_be_subst = strstr(p, "$disarmedfrom$")) != NULL) {
			*to_be_subst = '\0';
			to_be_subst += 14;
			xasprintf(&new_line, "%s%s%s", p, disarmedfrom,
			    to_be_subst);
			if (fprintf(outf, "%s", new_line) < 0) {
				log_error(LOG_ARGS, "Could not write headers");
				free(new_line);
				free(line);
				fclose(f);
				return -1;
			}
			free(new_line);
		} else {
			if (fprintf(outf, "%s", p) < 0) {
				log_error(LOG_ARGS, "Could not write headers");
				free(line);
				fclose(f);
				return -1;
			}
		}
	}
	free(line);
	fclose(f);
	return 0;
}

