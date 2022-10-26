/*
 * Copyright (C) 2004 Mads Martin Joergensen <mmj at mmj.dk>
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <time.h>
#include <fcntl.h>
#include <err.h>
#include <stdbool.h>

#include "mlmmj-maintd.h"
#include "mlmmj.h"
#include "strgen.h"
#include "chomp.h"
#include "log_error.h"
#include "mygetline.h"
#include "wrappers.h"
#include "xmalloc.h"
#include "ctrlvalue.h"
#include "statctrl.h"
#include "send_digest.h"
#include "log_oper.h"
#include "utils.h"

static void print_help(const char *prg)
{
	printf("Usage: %s [-L | -d] /path/to/dir [-F]\n"
	       " -d: Full path to directory with listdirs\n"
	       "     Use this to run maintenance on all list directories\n"
	       "     in that directory.\n"
	       " -L: Full path to one list directory\n"
	       " -F: Don't fork, performing one maintenance run only.\n"
	       "     This option should be used when one wants to\n"
	       "     avoid running another daemon, and use e.g. "
	       "cron to control it instead.\n", prg);
	exit(EXIT_SUCCESS);
}

static int mydaemon()
{
	if (daemon(1, 0) != 0) {
		log_error(LOG_ARGS, "Unable to become a daemon");
		err(EXIT_FAILURE, "Unable to become a daemon");
	}

	closefrom(3);

	return 0;
}

int delolder(int dfd, const char *dirname, time_t than)
{
	DIR *dir;
	struct dirent *dp;
	struct stat st;
	time_t t;
	int fd;

	fd = openat(dfd, dirname, O_DIRECTORY|O_CLOEXEC);
	if (fd == -1) {
		log_error(LOG_ARGS, "Could not open(%s)", dirname);
		return -1;
	}

	if((dir = fdopendir(fd)) == NULL) {
		log_error(LOG_ARGS, "Could not fdopendir(%s)", dirname);
		return -1;
	}

	while((dp = readdir(dir)) != NULL) {
		if(fstatat(fd, dp->d_name, &st, 0) < 0) {
			log_error(LOG_ARGS, "Could not stat(%s/%s)",
			    dirname, dp->d_name);
			continue;
		}
		if(!S_ISREG(st.st_mode))
			continue;
		t = time(NULL);
		if(t - st.st_mtime > than)
			unlinkat(fd, dp->d_name, 0);
	}
	closedir(dir);

	return 0;
}


int clean_moderation(int dfd)
{

	time_t modreqlife;

	modreqlife = ctrltimet(dfd, "modreqlife", MODREQLIFE);
	return (delolder(dfd, "moderation", modreqlife));
}

int clean_discarded(int dfd)
{
	return (delolder(dfd, "queue/discarded", DISCARDEDLIFE));
}

int clean_subconf(int dfd)
{
	return (delolder(dfd, "subconf", CONFIRMLIFE));
}

int clean_unsubconf(int dfd)
{
	return (delolder(dfd, "unsubconf", CONFIRMLIFE));
}

int resend_queue(int dfd, const char *mlmmjsend, const char *listdir)
{
	DIR *queuedir;
	struct dirent *dp;
	char *mailname, *fromname, *toname, *reptoname, *from, *to, *repto;
	char *ch;
	struct stat st;
	int fromfd, tofd, err = 0, qfd;
	time_t t, bouncelife;

	qfd = openat(dfd, "queue", O_DIRECTORY|O_CLOEXEC);
	if (qfd == -1) {
		log_error(LOG_ARGS, "Could not open(queue)");
		return 1;
	}

	if((queuedir = fdopendir(qfd)) == NULL) {
		log_error(LOG_ARGS, "Could not fdopendir(queue)");
		return 1;
	}

	while((dp = readdir(queuedir)) != NULL) {
		if(fstatat(qfd, dp->d_name, &st, 0) < 0) {
			log_error(LOG_ARGS, "Could not stat(queue/%s)",
			    dp->d_name);
			continue;
		}

		if(!S_ISREG(st.st_mode))
			continue;

		if(strchr(dp->d_name, '.')) {
			mailname = xstrdup(dp->d_name);
			ch = strrchr(mailname, '.');
			MY_ASSERT(ch);
			*ch = '\0';
			/* delete orphaned sidecar files */
			if(fstatat(qfd, mailname, &st, 0) < 0) {
				if(errno == ENOENT) {
					*ch = '.';
					unlinkat(qfd, mailname, 0);
				}
			}
			free(mailname);
			continue;
		}

		xasprintf(&fromname, "%s.mailfrom", dp->d_name);
		xasprintf(&toname, "%s.reciptto", dp->d_name);
		xasprintf(&reptoname, "%s.reply-to", dp->d_name);

		fromfd = openat(qfd, fromname, O_RDONLY);
		if(fromfd < 0)
			err = errno;
		tofd = openat(qfd, toname, O_RDONLY);

		if((fromfd < 0 && err == ENOENT) ||
				(tofd < 0 && errno == ENOENT)) {
			/* only delete old files to avoid deleting
			   mail currently being sent */
			t = time(NULL);
			if(fstatat(qfd, dp->d_name, &st, 0) == 0) {
				if(t - st.st_mtime > (time_t)36000) {
					unlinkat(qfd, dp->d_name, 0);
					/* avoid leaving orphans */
					unlinkat(qfd, fromname, 0);
					unlinkat(qfd, toname, 0);
					unlinkat(qfd, reptoname, 0);
				}
			}
			free(mailname);
			free(fromname);
			free(toname);
			free(reptoname);
			if(fromfd != -1)
				close(fromfd);
			if(tofd != -1)
				close(tofd);
			continue;
		}

		from = mygetline(fromfd);
		chomp(from);
		close(fromfd);
		free(fromname);
		to = mygetline(tofd);
		chomp(to);
		close(tofd);
		free(toname);
		int rtfd = openat(qfd, reptoname, O_RDONLY);
		if(rtfd == -1) {
			free(reptoname);
			repto = NULL;
		} else {
			repto = mygetline(rtfd);
			chomp(repto);
			close(rtfd);
			free(reptoname);
		}

		/* before we try again, check and see if it's old */
		bouncelife = ctrltimet(dfd, "bouncelife", BOUNCELIFE);
		t = time(NULL);
		if(t - st.st_mtime > bouncelife) {
			unlinkat(qfd, dp->d_name, 0);
			free(mailname);
			free(from);
			free(to);
			free(repto);
			continue;
		}

		if(repto)
			exec_and_wait(mlmmjsend, "-l", "1", "-L", listdir, "-m",
			    mailname, "-F", from, "-T", to, "-R", repto, "-a",
			    NULL);
		else
			exec_and_wait(mlmmjsend, "-l", "1", "-L", listdir, "-m",
			    mailname, "-F", from, "-T", to, "-a", NULL);
	}

	closedir(queuedir);

	return 0;
}

int resend_requeue(int dfd, const char *mlmmjsend, const char *listdir)
{
	DIR *queuedir;
	struct dirent *dp;
	char *archivefilename, *subfilename, *subnewname;
	struct stat st;
	time_t t;
	bool fromrequeuedir;

	int fd = openat(dfd, "requeue", O_DIRECTORY|O_CLOEXEC);
	if (fd == -1 || (queuedir = fdopendir(fd)) == NULL) {
		log_error(LOG_ARGS, "Could not opendir(requeue)");
		return 1;
	}
	
	while((dp = readdir(queuedir)) != NULL) {
		if((strcmp(dp->d_name, "..") == 0) ||
			(strcmp(dp->d_name, ".") == 0))
				continue;

		if(fstatat(fd, dp->d_name, &st, 0) < 0) {
			log_error(LOG_ARGS, "Could not stat(%s)",dp->d_name);
			continue;
		}

		if(!S_ISDIR(st.st_mode))
			continue;

		/* Remove old empty directories */
		t = time(NULL);
		if(t - st.st_mtime > (time_t)3600)
			if (unlinkat(fd, dp->d_name, AT_REMOVEDIR) == 0)
				continue;

		xasprintf(&archivefilename, "archive/%s", dp->d_name);

		/* Explicitly initialize for each mail we examine */
		fromrequeuedir = false;

		if(fstatat(dfd, archivefilename, &st, 0) < 0) {
			/* Might be it's just not moved to the archive
			 * yet because it's still getting sent, so just
			 * continue
			 */
			free(archivefilename);

			/* If the list is set not to archive we want to look
			 * in /requeue/ for a mailfile
			 */
			xasprintf(&archivefilename, "requeue/%s/mailfile",
			    dp->d_name);
			if(fstatat(dfd, archivefilename, &st, 0) < 0) {
				free(archivefilename);
				continue;
			}
			fromrequeuedir = true;
		}
		xasprintf(&subfilename, "%s/subscribers", dp->d_name);
		if(fstatat(fd, subfilename, &st, 0) < 0) {
			if (fromrequeuedir)
				unlinkat(dfd, archivefilename, 0);
			free(archivefilename);
			free(subfilename);
			continue;
		}

		xasprintf(&subnewname, "%s.resending", subfilename);

		if(renameat(fd, subfilename, fd, subnewname) < 0) {
			log_error(LOG_ARGS, "Could not rename(requeue/%s, requeue/%s)",
			    subfilename, subnewname);
			free(archivefilename);
			free(subfilename);
			free(subnewname);
			continue;
		}
		free(subfilename);

		exec_and_wait(mlmmjsend, "-l", "3", "-L", listdir, "-m",
		    archivefilename, "-s", subnewname, "-a", "-D", NULL);
	}

	closedir(queuedir);

	return 0;
}

int clean_nolongerbouncing(int dfd)
{
	DIR *bouncedir;
	char *filename, *probetimestr, *s;
	int probefd;
	time_t probetime, t;
	struct dirent *dp;
	struct stat st;
	int fd;
	const char *errstr;

	fd = openat(dfd, "bounce", O_DIRECTORY|O_CLOEXEC);
	if (fd == -1 || (bouncedir = fdopendir(fd)) == NULL) {
		log_error(LOG_ARGS, "Could not opendir(bounce)");
		return 1;
	}

	while((dp = readdir(bouncedir)) != NULL) {
		if((strcmp(dp->d_name, "..") == 0) ||
		   (strcmp(dp->d_name, ".") == 0))
				continue;

		filename = xstrdup(dp->d_name);

		s = strrchr(filename, '-');
		if(s && (strcmp(s, "-probe") == 0)) {
			if(fstatat(fd, filename, &st, 0) < 0) {
				log_error(LOG_ARGS, "Could not stat(%s)",
					  filename);
				free(filename);
				continue;
			}

			probefd = openat(fd, filename, O_RDONLY|O_CLOEXEC);
			if(probefd < 0) {
				free(filename);
				continue;
			}
			probetimestr = mygetline(probefd);
			if(probetimestr == NULL) {
				free(filename);
				continue;
			}
			close(probefd);
			chomp(probetimestr);
			probetime = strtotimet(probetimestr, &errstr);
			free(probetimestr);
			if (errstr != NULL) {
				free(filename);
				free(probetimestr);
				continue;
			}
			t = time(NULL);
			if(t - probetime > WAITPROBE) {
				unlinkat(fd, filename, 0);
				/* remove -probe onwards from filename */
				*s = '\0';
				unlinkat(fd, filename, 0);
				xasprintf(&s, "%s.lastmsg", filename);
				unlinkat(fd, s, 0);
				free(s);
			}
		}
		free(filename);
	}

	closedir(bouncedir);

	return 0;
}

int probe_bouncers(int dfd, const char *mlmmjbounce, const char *listdir)
{
	DIR *bouncedir;
	char *probefile, *s;
	struct dirent *dp;
	struct stat st;
	int fd;

	fd = openat(dfd, "bounce", O_DIRECTORY|O_CLOEXEC);
	if (fd == -1 || (bouncedir = fdopendir(fd)) == NULL) {
		log_error(LOG_ARGS, "Could not opendif(bounce)");
		return 1;
	}

	while((dp = readdir(bouncedir)) != NULL) {
		if((strcmp(dp->d_name, "..") == 0) ||
		   (strcmp(dp->d_name, ".") == 0))
				continue;

		s = strrchr(dp->d_name, '-');
		if(s && (strcmp(s, "-probe") == 0))
			continue;

		s = strrchr(dp->d_name, '.');
		if(s && (strcmp(s, ".lastmsg") == 0))
			continue;

		if(fstatat(fd, dp->d_name, &st, 0) < 0) {
			log_error(LOG_ARGS, "Could not stat(%s)", dp->d_name);
			continue;
		}

		xasprintf(&probefile, "%s-probe", dp->d_name);

		/* Skip files which already have a probe out */
		if(fstatat(fd, probefile, &st, 0) == 0) {
			free(probefile);
			continue;
		}
		free(probefile);

		exec_and_wait(mlmmjbounce, "-L", listdir, "-a", dp->d_name,
		    "-p", NULL);
	}
	closedir(bouncedir);

	return 0;
}

int unsub_bouncers(int dfd, const char *mlmmjunsub, const char *listdir)
{
	DIR *bouncedir;
	char *probefile, *address, *a, *firstbounce, *bouncedata;
	struct dirent *dp;
	struct stat st;
	int bfd;
	time_t bouncetime, t, bouncelife;
	const char *errstr;

	bfd = openat(dfd, "bounce", O_DIRECTORY|O_CLOEXEC);
	if (bfd == -1 || (bouncedir = fdopendir(bfd)) == NULL) {
		log_error(LOG_ARGS, "Could not opendir(bounce)");
		return 1;
	}

	bouncelife = ctrltimet(dfd, "bouncelife", BOUNCELIFE);

	while((dp = readdir(bouncedir)) != NULL) {
		if((strcmp(dp->d_name, "..") == 0) ||
		   (strcmp(dp->d_name, ".") == 0))
				continue;

		a = strrchr(dp->d_name, '-');
		if(a && (strcmp(a, "-probe") == 0))
			continue;

		a = strrchr(dp->d_name, '.');
		if(a && (strcmp(a, ".lastmsg") == 0))
			continue;

		if(fstatat(bfd, dp->d_name, &st, 0) < 0) {
			log_error(LOG_ARGS, "Could not stat(%s)", dp->d_name);
			continue;
		}

		xasprintf(&probefile, "%s-probe", dp->d_name);

		/* Skip files which already have a probe out */
		if(fstatat(bfd, probefile, &st, 0) == 0) {
			free(probefile);
			continue;
		}
		free(probefile);

		/* Get the first line of the bounce file to check if it's
		 * been bouncing for long enough
		 */
		int fd = openat(bfd, dp->d_name, O_RDONLY|O_CLOEXEC);
		if(fd == -1) {
			log_error(LOG_ARGS, "Could not open bounce/%s", dp->d_name);
			continue;
		}
		firstbounce = mygetline(fd);
		close(fd);
		if(firstbounce == NULL)
			continue;

		/* End the string at the comment */
		a = strchr(firstbounce, '#');
		if(a == NULL) {
			free(firstbounce);
			continue;
		}
		*a = '\0';
		bouncedata = xstrdup(a+1); /* Save for the log */
		chomp(bouncedata);
		a = strchr(firstbounce, ':');
		if(a == NULL) {
			free(firstbounce);
			free(bouncedata);
			continue;
		}

		a++; /* Increase to first digit */
		bouncetime = strtotimet(a, &errstr);
		free(firstbounce);
		if (errstr != NULL) {
			free(bouncedata);
			continue;
		}
		t = time(NULL);
		if(t - bouncetime < bouncelife + WAITPROBE) {
			free(bouncedata);
			continue; /* ok, don't unsub this one */
		}
		
		/* Ok, go ahead and unsubscribe the address */
		address = xstrdup(dp->d_name);
		a = strchr(address, '=');
		if(a == NULL) { /* skip malformed */
			free(address);
			free(bouncedata);
			continue;
		}
		*a = '@';

		exec_and_wait(mlmmjunsub, "-L", listdir, "-b", "-a", address,
		    NULL);
		unlinkat(bfd, dp->d_name, 0);
		xasprintf(&a, "%s.lastmsg",  dp->d_name);
		unlinkat(bfd, a, 0);
		free(a);
	}
	closedir(bouncedir);

	return 0;
}

int run_digests(int dfd, const char *mlmmjsend, const char *listdir)
{
	char *lasttimestr, *lastindexstr, *lastissuestr;
	char *s1, *s2;
	time_t digestinterval, t, lasttime;
	long digestmaxmails, lastindex, index, lastissue;
	int fd, indexfd;
	const char *errstr = NULL;

	if (statctrl(listdir, "noarchive")) {
		return 0;
	}

	digestinterval = ctrltimet(dfd, "digestinterval", DIGESTINTERVAL);
	digestmaxmails = ctrllong(listdir, "digestmaxmail", DIGESTMAXMAILS);

	fd = openat(dfd, "lastdigest", O_RDWR|O_CREAT|O_EXLOCK, S_IRUSR | S_IWUSR);
	if (fd < 0) {
		log_error(LOG_ARGS, "Could not open 'lastdigest'");
		return 1;
	}

	s1 = mygetline(fd);

	/* Syntax is lastindex:lasttime or lastindex:lasttime:lastissue */
	if (s1 && (lasttimestr = strchr(s1, ':'))) {
		*(lasttimestr++) = '\0';
		if ((lastissuestr = strchr(lasttimestr, ':'))) {
			*(lastissuestr++) = '\0';
			lastissue = strtoim(lastissuestr, LONG_MIN, LONG_MAX,
			    &errstr);
			if (errstr != NULL) {
				log_error(LOG_ARGS, "'lastdigests' contains "
				    "malformed data: Impossible to parse "
				    "lastissue: '%s': %s",
				    lastissuestr, errstr);
				return (1);
			}
		} else {
			lastissue = 0;
		}
		lasttime = strtotimet(lasttimestr, &errstr);
		if (errstr) {
			log_error(LOG_ARGS, "'lastdigest' invalid lasttimestr "
			    "'%s': %s", lasttimestr, errstr);
			return (1);
		}
		lastindexstr = s1;
		lastindex = strtoim(lastindexstr, LONG_MIN, LONG_MAX, &errstr);
		if (errstr != NULL) {
			log_error(LOG_ARGS, "'lastdigests' contains malformed "
			    "data: Impossible to parse lastindex: '%s': %s",
			    lastindexstr, errstr);
			return (1);
		}
	} else {
		if (s1 && (strlen(s1) > 0)) {
			log_error(LOG_ARGS, "'lastdigest' contains malformed "
			    "data");
			free(s1);
			close(fd);
			return 1;
		}
		/* If lastdigest is empty, we start from scratch */
		lasttime = 0;
		lastindex = 0;
		lastissue = 0;
	}

	indexfd = openat(dfd, "index", O_RDONLY);
	if (indexfd < 0) {
		log_error(LOG_ARGS, "Could not open 'index'");
		free(s1);
		close(fd);
		return 1;
	}
	s2 = mygetline(indexfd);
	close(indexfd);
	if (!s2) {
		/* If we don't have an index, no mails have been sent to the
		 * list, and therefore we don't need to send a digest */
		free(s1);
		close(fd);
		return 1;
	}

	index = strtoim(s2, 0, LONG_MAX, &errstr);
	if (errstr == NULL) {
		log_error(LOG_ARGS, "Invalid index content '%s'", s2);
		free(s1);
		free(s2);
		close(fd);
		return 1;
	}
	t = time(NULL);

	if ((t - lasttime >= digestinterval) ||
			(index - lastindex >= digestmaxmails)) {

		if (index > lastindex+digestmaxmails)
			index = lastindex+digestmaxmails;

		if (index > lastindex) {
			lastissue++;
			send_digest(listdir, lastindex+1, index, lastissue, NULL, mlmmjsend);
		}

		if (lseek(fd, 0, SEEK_SET) < 0) {
			log_error(LOG_ARGS, "Could not seek 'lastdigest'");
		} else {
			if (dprintf(fd, "%ld:%ld:%ld\n", index, (long)t, lastissue) < 0 ) {
				log_error(LOG_ARGS, "Could not write new "
				    "'lastdigest'");
			}
		}
	}

	free(s1);
	free(s2);
	close(fd);

	return 0;
}

void do_maintenance(int listfd, const char *listdir, const char *mlmmjsend,
		    const char *mlmmjbounce, const char *mlmmjunsub)
{
	char *random, *logname;
	char timenow[64];
	struct stat st;
	int maintdlogfd;
	uid_t uid = getuid();
	time_t t;

	if (listfd == -1)
		return;

	if (fstat(listfd, &st) < 0) {
		log_error(LOG_ARGS, "Could not stat "
		    "No maintenance run performed.");
		return;
	}

	if(uid == 0) { /* We're root. Do something about it.*/
		if(setuid(st.st_uid) < 0) {
			log_error(LOG_ARGS, "Could not setuid listdir owner.");
			return;
		}
	} else if(uid != st.st_uid) {
		log_error(LOG_ARGS,
				"User ID not equal to the ID of %s. No "
				"maintenance run performed.", listdir);
		return;
	}

	random = random_str();
	xasprintf(&logname, "maintdlog-%s", random);
	free(random);

	maintdlogfd = openat(listfd, logname, O_WRONLY|O_EXCL|O_CREAT, S_IRUSR|S_IWUSR);
	if(maintdlogfd == -1) {
		log_error(LOG_ARGS, "Could not open %s", logname);
		free(logname);
		return;
	}

	t = time(NULL);
	if(ctime_r(&t, timenow))
		dprintf(maintdlogfd, "Starting maintenance run at %s\n",
		    timenow);

	dprintf(maintdlogfd, "clean_moderation\n");
	clean_moderation(listfd);

	dprintf(maintdlogfd, "clean_discarded\n");
	clean_discarded(listfd);

	dprintf(maintdlogfd, "clean_subconf\n");
	clean_subconf(listfd);

	dprintf(maintdlogfd, "clean_unsubconf\n");
	clean_unsubconf(listfd);

	dprintf(maintdlogfd, "resend_queue(%s);\n", mlmmjsend);
	resend_queue(listfd, mlmmjsend, listdir);

	dprintf(maintdlogfd, "resend_requeue(%s);\n", mlmmjsend);
	resend_requeue(listfd, mlmmjsend, listdir);

	dprintf(maintdlogfd, "clean_nolongerbouncing\n");
	clean_nolongerbouncing(listfd);

	dprintf(maintdlogfd, "unsub_bouncers(%s);\n", mlmmjunsub);
	unsub_bouncers(listfd, mlmmjunsub, listdir);

	dprintf(maintdlogfd, "probe_bouncers(%s);\n", mlmmjbounce);
	probe_bouncers(listfd, mlmmjbounce, listdir);

	dprintf(maintdlogfd, "run_digests(%s);\n", mlmmjsend);
	run_digests(listfd, mlmmjsend, listdir);

	close(maintdlogfd);

	if (renameat(listfd, logname, listfd, MAINTD_LOGFILE) < 0)
		log_error(LOG_ARGS, "Could not rename(%s,%s)",
				logname, MAINTD_LOGFILE);

	free(logname);
}

int main(int argc, char **argv)
{
	int opt, daemonize = 1, ret = 0;
	char *bindir, *listdir = NULL, *mlmmjsend, *mlmmjbounce, *mlmmjunsub;
	char *dirlists = NULL;
	struct stat st;
	struct dirent *dp;
	DIR *dirp;

	CHECKFULLPATH(argv[0]);

	log_set_name(argv[0]);

	while ((opt = getopt(argc, argv, "hFVL:d:")) != -1) {
		switch(opt) {
		case 'd':
			dirlists = optarg;
			break;
		case 'F':
			daemonize = 0;
			break;
		case 'L':
			listdir = optarg;
			break;
		case 'h':
			print_help(argv[0]);
			break;
		case 'V':
			print_version(argv[0]);
			exit(EXIT_SUCCESS);
		}
	}

	if(listdir == NULL && dirlists == NULL) {
		errx(EXIT_FAILURE, "You have to specify -d or -L\n"
		    "%s -h for help", argv[0]);
	}

	if(listdir && dirlists) {
		errx(EXIT_FAILURE, "You have to specify either -d or -L\n"
		    "%s -h for help", argv[0]);
	}

	bindir = mydirname(argv[0]);
	xasprintf(&mlmmjsend, "%s/mlmmj-send", bindir);
	xasprintf(&mlmmjbounce, "%s/mlmmj-bounce", bindir);
	xasprintf(&mlmmjunsub, "%s/mlmmj-unsub", bindir);
	free(bindir);

	if(daemonize)
		ret = mydaemon();

	if(daemonize && ret < 0) {
		log_error(LOG_ARGS, "Could not daemonize. Only one "
				"maintenance run will be done.");
		daemonize = 0;
	}

	while(1) {
		if(listdir) {
			int fd = open(listdir, O_DIRECTORY);
			do_maintenance(fd, listdir, mlmmjsend, mlmmjbounce,
					mlmmjunsub);
			goto mainsleep;
		}

		int dfd = open(dirlists, O_DIRECTORY);
		if(dfd == -1) {
			log_error(LOG_ARGS, "Could not open directory(%s).",
					dirlists);
			free(mlmmjbounce);
			free(mlmmjsend);
			free(mlmmjunsub);
			exit(EXIT_FAILURE);
		}

		if((dirp = fdopendir(dfd)) == NULL) {
			log_error(LOG_ARGS, "Could not opendir(%s).",
					dirlists);
			free(mlmmjbounce);
			free(mlmmjsend);
			free(mlmmjunsub);
			exit(EXIT_FAILURE);
		}

		while((dp = readdir(dirp)) != NULL) {
			if((strcmp(dp->d_name, "..") == 0) ||
					(strcmp(dp->d_name, ".") == 0))
				continue;

			if (fstatat(dfd, dp->d_name, &st, 0) <0) {
				log_error(LOG_ARGS, "Could not stat(%s/%s)",
				    dirlists, dp->d_name);
				continue;
			}

			int fd = openat(dfd, dp->d_name,  O_DIRECTORY);
			if (fd == -1)
				continue;

			if (fstatat(fd, "control/listaddress", &st, 0) <0)
				continue;

			char *l;
			xasprintf(&l, "%s/%s", dirlists, dp->d_name);
			do_maintenance(fd, l, mlmmjsend, mlmmjbounce,
					mlmmjunsub);
			free(l);
			close(fd);
		}

		closedir(dirp);

mainsleep:
		if(!daemonize)
			break;
		else
			sleep(MAINTD_SLEEP);
	}

	free(mlmmjbounce);
	free(mlmmjsend);
	free(mlmmjunsub);

	log_free_name();
		
	exit(EXIT_SUCCESS);
}
