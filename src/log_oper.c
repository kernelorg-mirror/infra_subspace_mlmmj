/* Copyright (C) 2005 Mads Martin Joergensen < mmj AT mmj DOT dk >
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
#include <errno.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#include "xmalloc.h"
#include "mlmmj.h"
#include "log_error.h"
#include "log_oper.h"
#include "wrappers.h"
#include "utils.h"

int log_oper(int listfd, const char *basename, const char *fmt, ...)
{
	int fd, statres;
	char ct[26], *tmp;
	struct stat st;
	time_t t;
	va_list ap;

	statres = fstatat(listfd, basename, &st, AT_SYMLINK_NOFOLLOW);
	if(statres < 0 && errno != ENOENT) {
		log_error(LOG_ARGS, "Could not stat logfile %s", basename);
		return -1;
	}

	if(statres >= 0 && st.st_size > (off_t)OPLOGSIZE) {
		xasprintf(&tmp, "%s.rotated", basename);
		if(renameat(listfd, basename, listfd, tmp) < 0) {
			log_error(LOG_ARGS, "Could not rename %s,%s",
					basename, tmp);
		}
		free(tmp);
	}

	fd = openat(listfd, basename, O_RDWR|O_CREAT|O_APPEND, S_IRUSR|S_IWUSR);
	if(fd < 0 && !lock(fd, true)) {
		log_error(LOG_ARGS, "Could not open %s", basename);
		return -1;
	}

	if((time(&t) == (time_t)-1) || (ctime_r(&t, ct) == NULL))
		strncpy(ct, "Unknown time", sizeof(ct));
	else
		ct[24] = '\0';

	va_start(ap, fmt);
	if (dprintf(fd, "%s ", ct) < 0)
		log_error(LOG_ARGS, "Could not write to %s", basename);
	if (vdprintf(fd, fmt, ap) <0)
		log_error(LOG_ARGS, "Could not write to %s", basename);
	if (dprintf(fd, "\n") < 0)
		log_error(LOG_ARGS, "Could not write to %s", basename);
	va_end(ap);

	close(fd);

	return 0;
}
