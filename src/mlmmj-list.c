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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <string.h>
#include <err.h>
#include <sys/stat.h>

#include "mlmmj.h"
#include "chomp.h"

static void print_help(const char *prg)
{
	printf("Usage: %s -L /path/to/listdir\n"
	       "       [-c] [-d] [-h] [-m] [-n] [-o] [-s] [-V]\n"
	       " -L: Full path to list directory\n"
	       " -c: Print subscriber count\n"
	       " -d: Print for digesters list\n"
	       " -h: This help\n"
	       " -m: Print moderators for list\n"
	       " -n: Print for nomail version of list\n"
	       " -o: Print owner(s) of list\n"
	       " -s: Print normal subscribers (default) \n"
	       " -V: Print version\n", prg);
	exit(EXIT_SUCCESS);
}

static int
dumpcount(int fd, size_t *count)
{
	FILE *f;
	char *line = NULL;
	size_t linecap = 0;
	struct stat fst;

	f = fdopen(fd, "r");
	if (f == NULL)
		return (-1);

	if (fstat(fd, &fst) == 0) {
		if (S_ISDIR(fst.st_mode)) {
			return(-1);
		}
	}

	while (getline(&line, &linecap, f) > 0) {
		chomp(line);
		if (count)
			(*count)++;
		else
			printf("%s\n", line);
	}
	free(line);
	fclose(f);

	return 0;
}

int main(int argc, char **argv)
{
	int opt;
	size_t count = 0;
	bool docount = false;
	const char *listdir = NULL;
	const char *subfile = NULL;
	const char *subdir;
	DIR *dirp;
	struct dirent *dp;
	enum subtype typesub = SUB_NORMAL;
	int subdirfd = -1, listfd, fd;

	while ((opt = getopt(argc, argv, "cdhmnosVL:")) != -1) {
		switch(opt) {
		case 'c':
			docount = 1;
			break;
		case 'd':
			typesub = SUB_DIGEST;
			break;
		case 'h':
			print_help(argv[0]);
			break;
		case 'L':
			listdir = optarg;
			break;
		case 'm':
			typesub = SUB_FILE;
			subfile = "control/moderators";
			break;
		case 'n':
			typesub = SUB_NOMAIL;
			break;
		case 'o':
			typesub = SUB_FILE;
			subfile = "control/owner";
			break;
		case 'V':
			print_version(argv[0]);
			exit(EXIT_SUCCESS);
		default:
		case 's':
			typesub = SUB_NORMAL;
			break;
		}
	}

	if(listdir == NULL) {
		errx(EXIT_FAILURE, "You have to specify -L\n"
		    "%s -h for help", argv[0]);
	}
	listfd = open_listdir(listdir, false);
	if (listfd == -1)
		exit(EXIT_FAILURE);
	subdirfd = open_subscriber_directory(listfd, typesub, &subdir);
	if (subdirfd == -1 && subdir != NULL)
		err(EXIT_FAILURE, "Unable to open(%s/%s)", listdir, subdir);

	if(subdirfd != -1) {
		dirp = fdopendir(subdirfd);
		if(dirp == NULL)
			errx(EXIT_FAILURE,  "Could not opendir(%s);", subdir);
		while((dp = readdir(dirp)) != NULL) {
			if((strcmp(dp->d_name, "..") == 0) ||
			   (strcmp(dp->d_name, ".") == 0))
				continue;

			fd = openat(subdirfd, dp->d_name, O_RDONLY|O_CLOEXEC);
			dumpcount(fd, docount ? &count : NULL);
		}
		closedir(dirp);
	} else {
		fd = openat(listfd, subfile, O_RDONLY|O_CLOEXEC);
		dumpcount(fd, docount ? &count : NULL);
	}

	if (docount)
		printf("%zu\n", count);

	return 0;
}
