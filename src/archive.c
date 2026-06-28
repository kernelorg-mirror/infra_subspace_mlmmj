/*
 * Copyright (C) 2026 Baptiste Daroussin <bapt@FreeBSD.org>
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

#include <sys/stat.h>

#include <dirent.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "mlmmj.h"
#include "archive.h"
#include "log_error.h"
#include "statctrl.h"
#include "xmalloc.h"

int
archive_open(int listfd, int index)
{
	char buf[64];
	int archivefd, fd;
	DIR *dirp;
	struct dirent *dp;

	/* Fast path: try flat archive/N */
	snprintf(buf, sizeof(buf), "archive/%d", index);
	archivefd = openat(listfd, buf, O_RDONLY|O_CLOEXEC);
	if (archivefd != -1)
		return (archivefd);

	/* Fallback: scan archive/YYYY/MM/ subdirectories */
	fd = openat(listfd, "archive", O_RDONLY|O_DIRECTORY|O_CLOEXEC);
	if (fd == -1)
		return (-1);

	dirp = fdopendir(fd);
	if (dirp == NULL) {
		close(fd);
		return (-1);
	}

	while ((dp = readdir(dirp)) != NULL) {
		int yearfd;
		DIR *monthdir;
		struct dirent *mdp;

		if (dp->d_name[0] == '.')
			continue;
		/* Only consider 4-digit year directories */
		if (strlen(dp->d_name) != 4)
			continue;
		if (dp->d_name[strspn(dp->d_name, "0123456789")] != '\0')
			continue;

		yearfd = openat(dirfd(dirp), dp->d_name,
		    O_RDONLY|O_DIRECTORY|O_CLOEXEC);
		if (yearfd == -1)
			continue;

		monthdir = fdopendir(yearfd);
		if (monthdir == NULL) {
			close(yearfd);
			continue;
		}

		while ((mdp = readdir(monthdir)) != NULL) {
			int mfd;

			if (mdp->d_name[0] == '.')
				continue;

			mfd = openat(dirfd(monthdir), mdp->d_name,
			    O_RDONLY|O_DIRECTORY|O_CLOEXEC);
			if (mfd == -1)
				continue;

			snprintf(buf, sizeof(buf), "%d", index);
			archivefd = openat(mfd, buf, O_RDONLY|O_CLOEXEC);
			close(mfd);
			if (archivefd != -1) {
				closedir(monthdir);
				closedir(dirp);
				return (archivefd);
			}
		}
		closedir(monthdir);
	}
	closedir(dirp);

	return (-1);
}

char *
archive_path_write(int listfd, int ctrlfd, int index)
{
	char *path;

	if (!statctrl(ctrlfd, "archivepartition")) {
		xasprintf(&path, "archive/%d", index);
		return (path);
	}

	time_t now = time(NULL);
	struct tm tm;
	gmtime_r(&now, &tm);
	char yeardir[32], monthdir[48];

	snprintf(yeardir, sizeof(yeardir), "archive/%04d",
	    tm.tm_year + 1900);
	snprintf(monthdir, sizeof(monthdir), "archive/%04d/%02d",
	    tm.tm_year + 1900, tm.tm_mon + 1);

	mkdirat(listfd, yeardir, 0755);
	mkdirat(listfd, monthdir, 0755);

	xasprintf(&path, "%s/%d", monthdir, index);
	return (path);
}

void
archive_migrate(int listfd, int logfd)
{
	int archivefd;
	DIR *dirp;
	struct dirent *dp;
	struct stat st;
	strlist names = vec_init();

	archivefd = openat(listfd, "archive",
	    O_RDONLY|O_DIRECTORY|O_CLOEXEC);
	if (archivefd == -1)
		return;

	dirp = fdopendir(archivefd);
	if (dirp == NULL) {
		close(archivefd);
		return;
	}

	/* Collect flat archive filenames first (all-digit names) */
	while ((dp = readdir(dirp)) != NULL) {
		if (dp->d_name[0] == '.')
			continue;
		if (dp->d_name[strspn(dp->d_name, "0123456789")] != '\0')
			continue;
		vec_push(&names, xstrdup(dp->d_name));
	}
	closedir(dirp);

	archivefd = openat(listfd, "archive",
	    O_RDONLY|O_DIRECTORY|O_CLOEXEC);
	if (archivefd == -1) {
		vec_free_and_free(&names, free);
		return;
	}

	vec_foreach(names, i) {
		char src[64], dst[64], yeardir[32], monthdir[48];
		struct tm tm;

		snprintf(src, sizeof(src), "archive/%s", names.d[i]);

		if (fstatat(listfd, src, &st, 0) == -1) {
			if (logfd != -1)
				dprintf(logfd,
				    "archive_migrate: cannot stat %s\n",
				    src);
			free(names.d[i]);
			continue;
		}

		gmtime_r(&st.st_mtime, &tm);

		snprintf(yeardir, sizeof(yeardir), "archive/%04d",
		    tm.tm_year + 1900);
		snprintf(monthdir, sizeof(monthdir), "archive/%04d/%02d",
		    tm.tm_year + 1900, tm.tm_mon + 1);

		mkdirat(listfd, yeardir, 0755);
		mkdirat(listfd, monthdir, 0755);

		snprintf(dst, sizeof(dst), "%s/%s", monthdir, names.d[i]);

		if (renameat(listfd, src, listfd, dst) == 0) {
			if (logfd != -1)
				dprintf(logfd,
				    "archive_migrate: %s -> %s\n", src, dst);
		} else {
			if (logfd != -1)
				dprintf(logfd,
				    "archive_migrate: rename %s -> %s "
				    "failed\n", src, dst);
		}
		free(names.d[i]);
	}

	close(archivefd);
	vec_free(&names);
}
