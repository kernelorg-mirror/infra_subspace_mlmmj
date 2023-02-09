/*
 * Copyright (C) 2003 Mads Martin Joergensen <mmj at mmj.dk>
 * Copyright (C) 2022-2023 Baptiste Daroussin <bapt@FreeBSD.org>
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
#include <stdlib.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>

#include "mlmmj.h"
#include "subscriberfuncs.h"
#include "log_error.h"
#include "chomp.h"

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

bool
find_subscriber(int fd, const char *address)
{
	FILE *f;
	char *line = NULL;
	size_t linecap = 0;
	ssize_t linelen;
	bool ret = false;

	f = fdopen(fd, "r");
	while ((linelen = getline(&line, &linecap, f)) > 0) {
		chomp(line);
		if (strcasecmp(address, line) == 0) {
			ret = true;
			break;
		}
	}
	free(line);
	fclose(f);
	return (ret);;
}

int
is_subbed_in(int dirfd, const char *subdirname, const char *address)
{
	int subread;
	DIR *subddir;
	struct dirent *dp;
	bool ret = false;

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

		ret = find_subscriber(subread, address);
		if (ret)
			break;
	}
	closedir(subddir);

	return ret;
}

enum subtype
is_subbed(int listfd, const char *address, bool both)
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

char *
get_subcookie_content(int listfd, bool unsub, const char *param)
{
	int dfd, fd;
	FILE *f;
	char *line = NULL;
	size_t linecap = 0;

	dfd = openat(listfd, unsub ? "unsubconf" : "subconf", O_DIRECTORY|O_CLOEXEC);
	if (dfd == -1) {
		log_error(LOG_ARGS, "Cannot open %ssubconf Ignoring mail", unsub ? "un" : "");
		return (NULL);
	}
	fd = openat(dfd, param, O_RDONLY);
	close(dfd);
	if (fd == -1)
		return (NULL);
	f = fdopen(fd, "r");
	if (getline(&line, &linecap, f) > 0) {
		chomp(line);
		chomp(line);
		fclose(f);
		unlinkat(dfd, param, 0);
		return (line);
	}
	fclose(f);

	return (NULL);
}
