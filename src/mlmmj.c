/*
 * Copyright (C) 2021-2023 Baptiste Daroussin <bapt@FreeBSD.org>
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

#include <stdlib.h>
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
#include <err.h>

#include "getlistdelim.h"
#include "mlmmj.h"
#include "utils.h"
#include "log_error.h"
#include "xmalloc.h"
#include "chomp.h"
#include "gethdrline.h"
#include "mygetline.h"
#include "subscriberfuncs.h"
#include "find_email_adr.h"
#include "strgen.h"
#include "ctrlvalue.h"

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
	chomp(walk);
	*lastissue = strtoim(walk, LONG_MIN, LONG_MAX, errstr);
	if (*errstr != NULL) {
		*errstr = "Invalid value for lastissue";
		return (false);
	}
	return (true);
}

time_t
extract_bouncetime(char *line, const char **errstr)
{
	char *walk;
	*errstr = NULL;

	walk = strrchr(line, '#');
	if (walk == NULL) {
		*errstr = "comment is missing";
		return (0);
	}

	*walk = '\0';
	walk--;
	while (walk > line && isspace(*walk)) {
		*walk = '\0';
		walk--;
	}

	walk = strchr(line, ':');
	if (walk == NULL) {
		*errstr = "':' missing";
		return (0);
	}
	walk++;

	return (strtotimet(walk, errstr));
}

static const char *subtype_dirs[] = {
	"subscribers.d",
	"digesters.d",
	"nomailsubs.d",
	NULL,
	NULL,
	NULL,
	NULL,
};

int
open_subscriber_directory(int listfd, enum subtype typesub, const char **subdir)
{
	const char *dir = subtype_dirs[typesub];

	if (subdir != NULL)
		*subdir = dir;
	if (dir == NULL)
		return (-1);
	return (openat(listfd, dir, O_DIRECTORY|O_CLOEXEC));
}

bool
unsubscribe(int listfd, const char *address, enum subtype typesub)
{
	struct dirent *dp;
	DIR *dir;
	bool ret = true;
	struct stat st;
	int fd;
	int groupwritable = 0;
	const char *subdir;
	size_t alen, len;

	switch (typesub) {
	case SUB_ALL:
		if (!unsubscribe(listfd, address, SUB_NORMAL))
			ret = false;
		if (!unsubscribe(listfd, address, SUB_DIGEST))
			ret = false;
		if (!unsubscribe(listfd, address, SUB_NOMAIL))
			ret = false;
		return (ret);
	case SUB_NORMAL:
	case SUB_DIGEST:
	case SUB_NOMAIL:
		break;
	default:
		return (false);
	}

	fd = open_subscriber_directory(listfd, typesub, &subdir);
	if (fd == -1 || (dir = fdopendir(fd)) == NULL) {
		log_err("Could not opendir(%s)", subdir);
		return (false);
	}

	if (fstat(fd, &st) == 0) {
		if (st.st_mode & S_IWGRP) {
			groupwritable = S_IRGRP|S_IWGRP;
			umask(S_IWOTH);
		}
	}

	alen = strlen(address);
	while ((dp = readdir(dir)) != NULL) {
		struct stat st;
		int rfd, wfd;
		char *wrname, *start, *cur, *next, *end, *pos, *posend;

		if (strcmp(dp->d_name, ".") == 0 ||
		    strcmp(dp->d_name, "..") == 0)
			continue;

		rfd = openat(fd, dp->d_name, O_RDONLY, 0644);
		if (rfd == -1)
			continue;
		if (!lock(rfd, false))
			continue;

		if (fstat(rfd, &st) == -1) {
			log_err("Impossible to determine the size of %s/%s: %s",
			    subdir, dp->d_name, strerror(errno));
			close(rfd);
			ret = false;
			continue;
		}

		if (st.st_size == 0)
			continue;
		if (!S_ISREG(st.st_mode)) {
			log_err("Non regular files in subscribers.d");
			continue;
		}

		start = mmap(0, st.st_size, PROT_READ, MAP_SHARED, rfd, 0);
		if (start == MAP_FAILED) {
			log_err("Unable to mmap %s/%s: %s", subdir, dp->d_name,
			    strerror(errno));
			close(rfd);
			ret = false;
			continue;
		}

		end = start + st.st_size;

		pos = NULL;
		for (next = cur = start; next < end; next++) {
			if (*next != '\n')
				continue;
			len = next - cur;
			if (strncasecmp(address, cur, len) == 0) {
				pos = cur;
				break;
			}
			cur = next + 1;
		}

		if (pos == NULL && next > cur) {
			len = next - cur;
			if (strncasecmp(address, cur, len) == 0)
				pos = cur;
		}

		/* not found */
		if (pos == NULL) {
			munmap(start, st.st_size);
			close(rfd);
			continue;
		}

		/*
		 * if the file only contains the email to unsubscribe, just
		 * remove it
		 */
		posend = pos + alen;
		while (posend < end && *posend == '\n')
			posend++;
		if (pos == start && posend == end) {
			unlinkat(fd, dp->d_name, 0);
			continue;
		}

		xasprintf(&wrname, "%s.new", dp->d_name);
		wfd = openat(fd, wrname, O_RDWR|O_CREAT|O_TRUNC,
		    S_IRUSR|S_IWUSR|groupwritable);
		if (wfd == -1 && !lock(wfd, true)) {
			log_err("Could not open '%s/%s': %s", subdir, wrname,
			    strerror(errno));
			munmap(start, st.st_size);
			close(rfd);
			free(wrname);
			ret = false;
			continue;
		}

		dprintf(wfd, "%.*s", (int)(pos - start), start);
		dprintf(wfd, "%.*s", (int) (end - posend), posend);
		if (renameat(fd, wrname, fd, dp->d_name) == -1) {
			log_err("Could not rename '%s', to '%s'",
			    wrname, dp->d_name);
			ret = false;
		}
		close(wfd);
		munmap(start, st.st_size);
		close(rfd);
		free(wrname);
	}
	closedir(dir);
	return (ret);
}

bounce_t
bouncemail(int listfd, const char *theaddress, const char *identifier)
{
	char *tmp, buf[26];
	char *address = lowercase(theaddress);
	int fd, bdfd, bfd;
	time_t t;
	struct stat st;

	/* check if it's sub/unsub requests bouncing, and in that case
	 * simply remove the confirmation file. Variablenames address and
	 * number are a bit misleading in this case due to the different
	 * construction of the sub/unsub confirmation From header.
	 */
	if (strcmp(identifier, "confsub") == 0) {
		fd = openat(listfd, "subconf", O_DIRECTORY);
		if (fd == -1) {
			free(address);
			return (BOUNCE_OK);
		}
		unlinkat(fd, address, 0);
		close(fd);
		free(address);
		return (BOUNCE_OK);
	}

	if (strcmp(identifier, "confunsub") == 0) {
		fd = openat(listfd, "unsubconf", O_DIRECTORY);
		if (fd == -1) {
			free(address);
			return (BOUNCE_OK);
		}
		unlinkat(fd, address, 0);
		close(fd);
		free(address);
		return (BOUNCE_OK);
	}
	/* Below checks for bounce probes bouncing. If they do, simply remove
	 * the probe file and exit successfully. Yes, I know the variables
	 * have horrible names, but please bear with me.
	 */
	bdfd = openat(listfd, "bounce", O_DIRECTORY|O_CLOEXEC);
	if (bdfd == -1) {
		log_error(LOG_ARGS, "No bounce directory");
		free(address);
		return (BOUNCE_FAIL);
	}
	if(strcmp(identifier, "probe") == 0) {
		char *a;
		xasprintf(&a, "%s-probe", address);
		unlinkat(bdfd, a, 0);
		close(bdfd);
		free(address);
		free(a);
		return (BOUNCE_OK);
	}

	tmp = strrchr(address, '=');
	if (tmp == NULL) {
		close(bdfd);
		free(address);
		return (BOUNCE_OK);
	}

	if(fstatat(bdfd, address, &st, AT_SYMLINK_NOFOLLOW) == 0) {
		if(S_ISLNK(st.st_mode)) {
			log_error(LOG_ARGS, "bounce/%s is a symbolic link", address);
			free(address);
			return (BOUNCE_FAIL);
		}
	}
	bfd = openat(bdfd, address, O_WRONLY|O_APPEND|O_CREAT, S_IRUSR|S_IWUSR);
	close(bdfd);
	if (bfd == -1) {
		log_error(LOG_ARGS, "Cound not open(bounce/%s)", address);
		free(address);
		return (BOUNCE_FAIL);
	}
	*tmp = '@';

	if (is_subbed(listfd, address, 0) == SUB_NONE) {
		log_error(LOG_ARGS, "%s is bouncing but not subscribed?",
		    address);
		free(address);
		return (BOUNCE_OK);
	}

	t = time(NULL);
	ctime_r(&t, buf);
	dprintf(bfd, "%s:%ld # %s", identifier, (long int)t, buf);
	close(bfd);
	free(address);

	return (BOUNCE_DONE);
}

void
save_lastbouncedmsg(int listfd, const char *address, const char *mailname)
{
	char *savename, *fname;

	fname = lowercase(address);

	xasprintf(&savename, "bounce/%s.lastmsg", fname);
	renameat(AT_FDCWD, mailname, listfd, savename);
	free(savename);
	free(fname);
}

char *dsnparseaddr(const char *mailname)
{
	FILE *f;
	char *buf = NULL;
	size_t bufcap = 0;
	char *line, *hdr, *walk, *addr = NULL, *boundary = NULL;
	bool quoted = false;
	bool indsn = false;
	strlist emails = tll_init();

	f = fopen(mailname, "r");
	if(f == NULL) {
		log_error(LOG_ARGS, "Could not open bounceindexfile %s",
				mailname);
		return NULL;
	}

	while((line = gethdrline(f, NULL))) {
		if (strncasecmp(line, "content-type:", 13) != 0) {
			free(line);
			continue;
		}
		walk = line + 13;
		while (isspace(*walk) && *walk != '\0')
			walk++;
		if (strncasecmp(walk, "multipart/report;", 17) != 0) {
			free(line);
			break;
		}
		walk += 17;
		while (isspace(*walk) && *walk != '\0')
			walk++;
		if (strncasecmp(walk, "report-type=delivery-status;", 28) != 0) {
			free(line);
			break;
		}
		walk += 28;
		while (isspace(*walk) && *walk != '\0')
			walk++;
		if (strncasecmp(walk, "boundary=", 9) != 0) {
			free(line);
			break;
		}
		walk += 9;

		if (*walk == '"') {
			walk++;
			quoted = true;
		}
		xasprintf(&boundary, "--%s", walk);
		boundary[strcspn(boundary, quoted ? " \t\"" : " \t")] = '\0';
		free(line);
		break;
	}
	/* this is not a miltipart/report mail see RFC1839 */
	if (boundary == NULL)
		return NULL;
	while ((getline(&buf, &bufcap, f) > 0)) {
		chomp(buf);
		if (indsn) {
			if (strncasecmp(buf, "Final-Recipient:", 16) == 0) {
				walk = strchr(buf, ';');
				if (walk == NULL) {
					break;
				}
				find_email_adr(walk+1, &emails);
				if(tll_length(emails) > 0) {
					addr = xstrdup(tll_front(emails));
					tll_free_and_free(emails, free);
				}
				break;
			}
		}
		if (strcmp(buf, boundary) == 0) {
			if (indsn)
				break;
			/* check our content type is a valid one */
			while ((hdr = gethdrline(f, NULL))) {
				if (indsn) {
					/* skip the rest of the headers if any */
					free(hdr);
					continue;
				}
				if (strncasecmp(hdr, "content-type:", 13) != 0) {
					free(hdr);
					continue;
				}
				walk = hdr + 13;
				while (isspace(*walk) && *walk != '\0')
					walk++;
				if (strncasecmp(walk, "message/delivery-status", 23) != 0) {
					free(hdr);
					break;
				}
				free(hdr);
				indsn = true;
			}
		}
	}
	free(buf);
	return addr;
}

int
open_listdir(const char *listdir, bool usercheck)
{
	struct stat st;
	uid_t uid;
	int fd;

	fd = open(listdir, O_DIRECTORY|O_CLOEXEC);
	if (fd == -1) {
		warn("Cannot open(%s)", listdir);
		return (-1);
	}
	if (!usercheck)
		return (fd);
	if (fstat(fd, &st) != 0) {
		log_error(LOG_ARGS, "Cannot stat %s", listdir);
		errx(EXIT_FAILURE, "Cannot stat %s", listdir);
	}
	uid = getuid();
	if (uid && uid != st.st_uid) {
		log_error(LOG_ARGS, "Have to invoke either as root or as the "
		    "user owning listdir");
		errx(EXIT_FAILURE, "Have to invoke either as root or as the "
		    "user owning listdir");
	}
	return (fd);
}

void
ml_init(struct ml *ml)
{
	ml->dir = NULL;
	ml->fd = -1;
	ml->ctrlfd = -1;
	ml->delim = NULL;
	ml->addr = NULL;
	ml->fqdn = NULL;
	ml->name = NULL;
}

bool
ml_open(struct ml *ml, bool checkuser)
{
	if (ml->dir == NULL)
		return (false);

	ml->fd = open_listdir(ml->dir, checkuser);
	if (ml->fd == -1)
		return (false);
	ml->ctrlfd = openat(ml->fd, "control", O_DIRECTORY|O_CLOEXEC);
	if (ml->ctrlfd == -1) {
		warn("Cannot open(%s/control)", ml->dir);
		ml_close(ml);
		return (false);
	}

	ml->addr = ctrlvalue(ml->ctrlfd, "listaddress");
	if (ml->addr == NULL) {
		warnx("Missing list address");
		ml_close(ml);
		return (false);
	}
	ml->delim = getlistdelim(ml->ctrlfd);
	if (!splitlistaddr(ml->addr, &ml->name, &ml->fqdn)) {
		warnx("%s: is not a valid mailing list address, "
		    "missing '@'", ml->addr);
		ml_close(ml);
		return (false);
	}

	return (true);
}

void
ml_close(struct ml *ml)
{
	free(ml->delim);
	free(ml->addr);
	free(ml->name);
	if (ml->fd != -1)
		close(ml->fd);
	if (ml->ctrlfd != -1)
		close(ml->ctrlfd);
}
