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
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>

#include "mlmmj.h"
#include "subscriberfuncs.h"
#include "mygetline.h"
#include "log_error.h"
#include "wrappers.h"

char *subtype_strs[] = {
	"normal",
	"digest",
	"nomail",
	"file",
	"all",
	"both",
	"none"
};

char * subreason_strs[] = {
	"request",
	"confirm",
	"permit",
	"admin",
	"bouncing",
	"switch"
};

off_t find_subscriber(int fd, const char *address)
{
	char *start, *cur, *next;
	struct stat st;
	size_t len;

	if(fstat(fd, &st) < 0) {
		log_error(LOG_ARGS, "Could not stat fd");
		return (off_t)-1;
	}

	/* No need to check in 0-size file */
	if(st.st_size == 0)
		return (off_t)-1;

	if(!S_ISREG(st.st_mode)) {
		log_error(LOG_ARGS, "Non regular file in subscribers.d/");
		return (off_t)-1;
	}

	if((start = mmap(0, st.st_size, PROT_READ, MAP_SHARED, fd, 0)) ==
			MAP_FAILED) {
		log_error(LOG_ARGS, "Could not mmap fd");
		return (off_t)-1;
	}
	
	for(next = cur = start; next < start + st.st_size; next++) {
		if(*next == '\n') {
			len = next - cur;
			if((strlen(address) == len) &&
			   (strncasecmp(address, cur, len) == 0)) {
				munmap(start, st.st_size);
				return (off_t)(cur - start);
			}
			cur = next + 1;
		}
	}
	
	if(next > cur) {
		len = next - cur;
		if((strlen(address) == len) &&
		   (strncasecmp(address, cur, len) == 0)) {
			munmap(start, st.st_size);
			return (off_t)(cur - start);
		}
	}
	
	munmap(start, st.st_size);
	return (off_t)-1;
}

int
is_subbed_in(int dirfd, const char *subdirname, const char *address)
{
	int retval = 0, subread;
	off_t suboff;
	DIR *subddir;
	struct dirent *dp;

	if((subddir = fdopendir(dirfd)) == NULL) {
		log_error(LOG_ARGS, "Could not opendir(%s)", subdirname);
		exit(EXIT_FAILURE);
	}

	while((dp = readdir(subddir)) != NULL) {
		if(!strcmp(dp->d_name, "."))
			continue;
		if(!strcmp(dp->d_name, ".."))
			continue;

		subread = openat(dirfd, dp->d_name, O_RDONLY);
		if(subread < 0) {
			log_error(LOG_ARGS, "Could not open %s/%s",
			  subdirname, dp->d_name);
			continue;
		}

		suboff = find_subscriber(subread, address);
		close(subread);

		if(suboff == -1) {
			continue;
		} else {
			retval = 1;
			break;
		}
	}
	closedir(subddir);

	return retval;
}

enum subtype
is_subbedat(int listfd, const char *address, bool both)
{
	enum subtype typesub = SUB_NONE;
	int fd;

	fd = openat(listfd, "subscribers.d", O_DIRECTORY);
	if (fd != -1 && is_subbed_in(fd, "subscribers.d", address)) {
		if (!both) return SUB_NORMAL;
		typesub = SUB_NORMAL;
	}

	fd = openat(listfd, "digesters.d", O_DIRECTORY);
	if (fd != -1 && is_subbed_in(fd, "digesters.d", address)) {
		if (typesub == SUB_NORMAL) return SUB_BOTH;
		return SUB_DIGEST;
	}

	fd = openat(listfd, "nomailsubs.d", O_DIRECTORY);
	if (fd != -1 && is_subbed_in(fd, "nomailsubs.d", address))
		return SUB_NOMAIL;

	return (typesub);
}

enum subtype
is_subbed(const char *listdir, const char *address, bool both)
{
	int dirfd;

	dirfd = open(listdir, O_DIRECTORY);
	if (dirfd == -1) {
		log_error(LOG_ARGS, "Could not opendir()");
		exit(EXIT_FAILURE);
	}
	return (is_subbedat(dirfd, address, both));
}
