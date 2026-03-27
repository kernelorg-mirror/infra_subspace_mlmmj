/*
 * Copyright (C) 2004 Mads Martin Joergensen <mmj at mmj.dk>
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
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <time.h>
#include <fcntl.h>
#include <err.h>
#include <stdbool.h>

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
#include "send_mail.h"

#define log(...) dprintf(logfd, __VA_ARGS__);
#define opendirat(_dirfd, _fd, _dirent, _path) \
	do { \
		_fd = openat(_dirfd, _path, O_DIRECTORY|O_CLOEXEC|O_RDONLY); \
		if (_fd == -1 || (_dirent = fdopendir(_fd)) == NULL) { \
			log(" - Could not open '%s': %s\n", _path, \
			    strerror(errno)); \
			return (false); \
		} \
	} while (0);

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

static int mydaemon(void)
{
	if (daemon(1, 0) != 0) {
		log_error(LOG_ARGS, "Unable to become a daemon");
		err(EXIT_FAILURE, "Unable to become a daemon");
	}

	return 0;
}

static bool
delolder(int dfd, const char *dirname, time_t than, int logfd)
{
	DIR *dir;
	struct dirent *dp;
	struct stat st;
	time_t t;
	int fd;
	bool ret = true;

	opendirat(dfd, fd, dir, dirname);
	while((dp = readdir(dir)) != NULL) {
		if(fstatat(fd, dp->d_name, &st, 0) < 0) {
			log(" - Could not stat(%s/%s): %s\n",
			    dirname, dp->d_name, strerror(errno));
			ret = false;
			continue;
		}
		if(!S_ISREG(st.st_mode))
			continue;
		t = time(NULL);
		if(t - st.st_mtime > than) {
			if (unlinkat(fd, dp->d_name, 0) == -1) {
				log("- Could not remove ('%s/%s'): %s\n",
				    dirname, dp->d_name, strerror(errno));
				ret = false;
			}
		}
	}
	closedir(dir);

	return (ret);
}


static bool
clean_moderation(int dfd, int ctrlfd, int logfd)
{

	time_t modreqlife;

	modreqlife = ctrltimet(ctrlfd, "modreqlife", MODREQLIFE);
	return (delolder(dfd, "moderation", modreqlife, logfd));
}

static bool
clean_discarded(int dfd, int logfd)
{
	return (delolder(dfd, "queue/discarded", DISCARDEDLIFE, logfd));
}

static bool
clean_subconf(int dfd, int logfd)
{
	return (delolder(dfd, "subconf", CONFIRMLIFE, logfd));
}

static bool
clean_unsubconf(int dfd, int logfd)
{
	return (delolder(dfd, "unsubconf", CONFIRMLIFE, logfd));
}

static bool
resend_queue(struct ml *ml, int logfd)
{
	DIR *queuedir;
	struct dirent *dp;
	char *fromname, *toname, *reptoname, *from, *to, *repto;
	char *ch;
	struct stat st;
	int errn = 0, qfd;
	time_t t, bouncelife;
	bool ret = true;
	struct mail mail;

	opendirat(ml->fd, qfd, queuedir, "queue");

	while((dp = readdir(queuedir)) != NULL) {
		/* we might have deleted some files like the "mail.*" */
		if(fstatat(qfd, dp->d_name, &st, 0) < 0)
			continue;

		if(!S_ISREG(st.st_mode))
			continue;

		if(strchr(dp->d_name, '.')) {
			char *mailname = xstrdup(dp->d_name);
			ch = strrchr(mailname, '.');
			MY_ASSERT(ch);
			*ch = '\0';
			/* delete orphaned sidecar files */
			if(faccessat(qfd, mailname, F_OK, 0) < 0) {
				if(errno == ENOENT) {
					*ch = '.';
					if (unlinkat(qfd, mailname, 0) == -1) {
						log(" - Could not remove %s: %s\n",
						    mailname, strerror(errno));
						ret = false;
					}
				}
			}
			free(mailname);
			continue;
		}

		xasprintf(&fromname, "%s.mailfrom", dp->d_name);
		xasprintf(&toname, "%s.reciptto", dp->d_name);
		xasprintf(&reptoname, "%s.reply-to", dp->d_name);

		errn = 0;
		from = ctrlvalue(qfd, fromname);
		if (from == NULL)
			errn = errno;
		to = ctrlvalue(qfd, toname);
		if((from == NULL && errn == ENOENT) ||
				(to == NULL && errno == ENOENT)) {
			/* only delete old files to avoid deleting
			   mail currently being sent */
			t = time(NULL);
			if(fstatat(qfd, dp->d_name, &st, 0) == 0) {
				if(t -  (time_t)36000 > st.st_mtime) {
					unlinkat(qfd, dp->d_name, 0);
					/* avoid leaving orphans */
					unlinkat(qfd, fromname, 0);
					unlinkat(qfd, toname, 0);
					unlinkat(qfd, reptoname, 0);
				}
			}
			free(from);
			free(to);
			free(reptoname);
			free(fromname);
			free(toname);
			continue;
		}
		repto = ctrlvalue(qfd, reptoname);

		/* before we try again, check and see if it's old */
		bouncelife = ctrltimet(ml->ctrlfd, "bouncelife", BOUNCELIFE);
		t = time(NULL);
		if(t - bouncelife > st.st_mtime ) {
			if (unlinkat(qfd, dp->d_name, 0) == -1) {
				log(" - Could not remove queue/%s: %s\n",
				    dp->d_name, strerror(errno));
				ret = false;
			}
			/* avoid leaving orphans */
			unlinkat(qfd, fromname, 0);
			unlinkat(qfd, toname, 0);
			unlinkat(qfd, reptoname, 0);
			free(from);
			free(to);
			free(repto);
			continue;
		}

		memset(&mail, 0, sizeof(mail));
		mail.to = to;
		mail.from = from;
		mail.replyto = repto;
		int mailfd = openat(qfd, dp->d_name, O_RDONLY);
		mail.fp = fdopen(mailfd, "r");
		if (mail.fp == NULL)
			err(1, "fdopen queue/%s", dp->d_name);
		if (send_single_mail(&mail, ml, false)) {
			unlinkat(qfd, dp->d_name, 0);
			unlinkat(qfd, fromname, 0);
			unlinkat(qfd, toname, 0);
			unlinkat(qfd, reptoname, 0);
		}
		free(from);
		free(to);
		free(repto);
		free(reptoname);
		free(fromname);
		free(toname);
		fclose(mail.fp);
	}

	closedir(queuedir);

	return (ret);
}

static bool
resend_requeue(struct ml *ml, const char *mlmmjsend, int logfd)
{
	DIR *queuedir;
	struct dirent *dp;
	char *archivefilename, *subnewname;
	struct stat st;
	time_t t;
	bool fromrequeuedir;
	int fd;
	bool ret = true;

	opendirat(ml->fd, fd, queuedir, "requeue");
	while((dp = readdir(queuedir)) != NULL) {
		if((strcmp(dp->d_name, "..") == 0) ||
			(strcmp(dp->d_name, ".") == 0))
				continue;

		if(fstatat(fd, dp->d_name, &st, 0) < 0) {
			log(" - Could not stat(requeue/%s): %s\n", dp->d_name,
			    strerror(errno));
			continue;
		}

		if(!S_ISDIR(st.st_mode))
			continue;

		/* Remove old empty directories */
		t = time(NULL);
		if(t - (time_t)3600 > st.st_mtime &&
		    unlinkat(fd, dp->d_name, AT_REMOVEDIR) == 0)
			continue;

		int requeuefd = openat(fd, dp->d_name, O_DIRECTORY);

		xasprintf(&archivefilename, "archive/%s", dp->d_name);
		int archivefd = openat(ml->fd, archivefilename, O_RDONLY);
		free(archivefilename);

		/* Explicitly initialize for each mail we examine */
		fromrequeuedir = false;

		if (archivefd == -1) {
			/* If the list is set not to archive we want to look
			 * in /requeue/ for a mailfile
			 */
			archivefd = openat(requeuefd, "mailfile", O_RDONLY);
			if (archivefd == -1) {
				close(requeuefd);
				continue;
			}
			fromrequeuedir = true;
		}
		int subfd = openat(requeuefd, "subscribers", O_RDONLY);
		if (subfd == -1) {
			if (fromrequeuedir && unlinkat(requeuefd, "mailfile", 0) == -1) {
				log(" - Cound not remove requeue/%s/mailfile: %s\n",
				    dp->d_name, strerror(errno));
				ret = false;
			}
			close(requeuefd);
			continue;
		}
		unlinkat(requeuefd, "subscribers", 0);

		xasprintf(&subnewname, "%d", subfd);
		xasprintf(&archivefilename, "%d", archivefd);
		exec_and_wait(mlmmjsend, "-l", "3", "-L", ml->dir, "-m",
		    archivefilename, "-s", subnewname, "-a", "-D", NULL);
		free(subnewname);
		free(archivefilename);
		if (fromrequeuedir)
			unlinkat(requeuefd, "mailfile", 0);
		close(subfd);
		close(requeuefd);
		unlinkat(fd, dp->d_name, AT_REMOVEDIR);
	}

	closedir(queuedir);

	return ret;
}

static bool
clean_nolongerbouncing(int dfd, int logfd)
{
	DIR *bouncedir;
	char *filename, *s;
	time_t probetime, t;
	struct dirent *dp;
	int fd;
	bool ret = true;

	opendirat(dfd, fd, bouncedir, "bounce");
	while((dp = readdir(bouncedir)) != NULL) {
		if((strcmp(dp->d_name, "..") == 0) ||
		   (strcmp(dp->d_name, ".") == 0))
				continue;

		filename = xstrdup(dp->d_name);

		s = strrchr(filename, '-');
		if(s && (strcmp(s, "-probe") == 0)) {
			if (faccessat(fd, filename, F_OK, 0) < 0) {
				log(" - Could not access(bounce/%s)",
				    filename);
				free(filename);
				ret = false;
				continue;
			}

			probetime = ctrltimet(fd, filename, (time_t)-1);
			if (probetime == (time_t)-1) {
				free(filename);
				continue;
			}
			t = time(NULL);
			if(t - probetime > WAITPROBE) {
				if (unlinkat(fd, filename, 0) == -1) {
					log(" - Could not remove bounce/%s: "
					    "%s\n", filename,
					    strerror(errno));
					ret = false;
				}
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

	return (ret);
}

static bool
probe_bouncers(struct ml *ml, const char *mlmmjbounce, int logfd)
{
	DIR *bouncedir;
	char *probefile, *s;
	struct dirent *dp;
	int fd;
	bool ret = true;

	opendirat(ml->fd, fd, bouncedir, "bounce");
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

		if(faccessat(fd, dp->d_name, F_OK, 0) < 0) {
			log(" - Could not stat(bounce/%s): %s\n", dp->d_name,
			    strerror(errno));
			ret = false;
			continue;
		}

		xasprintf(&probefile, "%s-probe", dp->d_name);

		/* Skip files which already have a probe out */
		if(faccessat(fd, probefile, F_OK, 0) == 0) {
			free(probefile);
			continue;
		}
		free(probefile);

		exec_and_wait(mlmmjbounce, "-L", ml->dir, "-a", dp->d_name,
		    "-p", NULL);
	}
	closedir(bouncedir);

	return (ret);
}

static bool
unsub_bouncers(int dfd, int ctrlfd, int logfd)
{
	DIR *bouncedir;
	char *probefile, *address, *a, *firstbounce;
	struct dirent *dp;
	int bfd;
	time_t bouncetime, t, bouncelife;
	const char *errstr;
	bool ret = true;

	opendirat(dfd, bfd, bouncedir, "bounce");

	bouncelife = ctrltimet(ctrlfd, "bouncelife", BOUNCELIFE);

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

		if(faccessat(bfd, dp->d_name, F_OK, 0) < 0) {
			log(" - Could not stat(bounce/%s): %s\n",
			    dp->d_name, strerror(errno));
			ret = false;
			continue;
		}

		xasprintf(&probefile, "%s-probe", dp->d_name);

		/* Skip files which already have a probe out */
		if(faccessat(bfd, probefile, F_OK, 0) == 0) {
			free(probefile);
			continue;
		}
		free(probefile);

		/* Get the first line of the bounce file to check if it's
		 * been bouncing for long enough
		 */
		int fd = openat(bfd, dp->d_name, O_RDONLY|O_CLOEXEC);
		if(fd == -1) {
			log(" - Could not open bounce/%s: %s\n", dp->d_name,
			    strerror(errno));
			ret = false;
			continue;
		}
		firstbounce = readlf(fd, true);
		if(firstbounce == NULL)
			continue;

		bouncetime = extract_bouncetime(firstbounce, &errstr);
		free(firstbounce);
		if (errstr != NULL) {
			log(" - Error parsing the first line of bounce/%s: %s\n",
			    dp->d_name, errstr);
			ret = false;
			continue;
		}
		t = time(NULL);
		if(t - bouncetime < bouncelife + WAITPROBE)
			continue; /* ok, don't unsub this one */
		
		/* Ok, go ahead and unsubscribe the address */
		address = xstrdup(dp->d_name);
		a = strchr(address, '=');
		if(a == NULL) { /* skip malformed */
			free(address);
			continue;
		}
		*a = '@';

		if (!unsubscribe(dfd, address, SUB_ALL)) {
			log(" - Some errors during unsubscription of %s\n",
			    address);
			free(address);
			ret = false;
			continue;
		}
		unlinkat(bfd, dp->d_name, 0);
		xasprintf(&a, "%s.lastmsg",  dp->d_name);
		unlinkat(bfd, a, 0);
		free(a);
		free(address);
	}
	closedir(bouncedir);

	return (ret);
}

static bool
run_digests(struct ml *ml, const char *mlmmjsend, int logfd)
{
	char *s1, *s2;
	time_t digestinterval, t, lasttime;
	long digestmaxmails, lastindex, index, lastissue;
	int fd, indexfd;
	const char *errstr = NULL;
	bool ret = false;

	if (statctrl(ml->ctrlfd, "noarchive")) {
		log(" - noarchive tunable: skipping digest\n");
		return (true);
	}
	if (faccessat(ml->fd, "index", R_OK, 0) == -1) {
		log(" - No readable index file: no digest\n");
		return (true);
	}

	digestinterval = ctrltimet(ml->ctrlfd, "digestinterval", DIGESTINTERVAL);
	digestmaxmails = ctrllong(ml->ctrlfd, "digestmaxmail", DIGESTMAXMAILS);

	fd = openat(ml->fd, "lastdigest", O_RDWR|O_CREAT, S_IRUSR | S_IWUSR);
	if (fd < 0 || !lock(fd, true)) {
		log(" - Could not open or create 'lastdigest': %s\n",
		    strerror(errno));
		return (false);
	}

	s1 = mygetline(fd);

	/* Syntax is lastindex:lasttime or lastindex:lasttime:lastissue */
	if (!parse_lastdigest(s1, &lastindex, &lasttime, &lastissue, &errstr)) {
		log(" - malformerd lastdigest '%s': %s", s1, errstr);
		return (false);
	}

	indexfd = openat(ml->fd, "index", O_RDONLY);
	if (indexfd < 0) {
		log("Could not open'index': %s", strerror(errno));
		free(s1);
		close(fd);
		return (false);
	}
	s2 = readlf(indexfd, true);
	if (!s2) {
		/* If we don't have an index, no mails have been sent to the
		 * list, and therefore we don't need to send a digest */
		free(s1);
		close(fd);
		return (true);
	}
	index = strtoim(s2, 0, LONG_MAX, &errstr);
	if (errstr != NULL) {
		log(" - Invalid index content '%s': %s\n", s2, errstr);
		free(s1);
		free(s2);
		close(fd);
		return (false);
	}
	t = time(NULL);

	if ((t - lasttime >= digestinterval) ||
			(index - lastindex >= digestmaxmails)) {

		if (index > lastindex+digestmaxmails)
			index = lastindex+digestmaxmails;

		if (index > lastindex) {
			lastissue++;
			send_digest(ml, lastindex+1, index, lastissue, NULL, mlmmjsend);
		}

		if (lseek(fd, 0, SEEK_SET) < 0) {
			log(" - Could not seek 'lastdigest': %s\n", strerror(errno));
			goto out;
		} else {
			if (dprintf(fd, "%ld:%ld:%ld\n", index, (long)t, lastissue) < 0 ) {
				log("Could not write new 'lastdigest': %s\n",
				    strerror(errno));
				goto out;
			}
		}
	}

	ret = true;
out:
	free(s1);
	free(s2);
	close(fd);

	return (ret);
}

void do_maintenance(struct ml *ml, const char *mlmmjsend, const char *mlmmjbounce)
{
	char *random, *logname;
	char timenow[64];
	int logfd;
	time_t t;

	random = random_str();
	xasprintf(&logname, "maintdlog-%s", random);
	free(random);

	logfd = openat(ml->fd, logname, O_WRONLY|O_EXCL|O_CREAT, S_IRUSR|S_IWUSR);
	if(logfd == -1) {
		log_err("Could not open %s: %s", logname, strerror(errno));
		free(logname);
		return;
	}

	t = time(NULL);
	if(ctime_r(&t, timenow))
		log("Starting maintenance run at %s\n",
		    timenow);

	log("clean_moderation\n");
	if (!clean_moderation(ml->fd, ml->ctrlfd, logfd))
		log_err("An error occurred while cleaning moderation, see %s",
		    MAINTD_LOGFILE);

	log("clean_discarded\n");
	if (!clean_discarded(ml->fd, logfd))
		log_err("An error occurred while cleaning discarded mails, see "
		    "%s", MAINTD_LOGFILE);

	log("clean_subconf\n");
	if (!clean_subconf(ml->fd, logfd))
		log_err("An error occurred while cleaning subscribtion "
		    "confirmations, mails, see %s", MAINTD_LOGFILE);

	log("clean_unsubconf\n");
	if (!clean_unsubconf(ml->fd, logfd))
		log_err("An error occurred while cleaning unsubscribtion "
		    "confirmations, mails, see %s", MAINTD_LOGFILE);

	log("resend_queue\n");
	if (!resend_queue(ml, logfd))
		log_err("An error occurred while resending queued mails, see %s",
		    MAINTD_LOGFILE);

	log("resend_requeue\n");
	if (!resend_requeue(ml, mlmmjsend, logfd))
		log_err("An error occurred while resending requeued mails, see "
		    "%s", MAINTD_LOGFILE);

	log("clean_nolongerbouncing\n");
	if (!clean_nolongerbouncing(ml->fd, logfd))
		log_err("An error occurred while cleaning no longer bouncing "
		   "traces, see %s", MAINTD_LOGFILE);

	log("unsub_bouncers\n");
	if (!unsub_bouncers(ml->fd, ml->ctrlfd, logfd))
		log_err("An error occurred while unsubscribing bouncing emails, "
		    "see %s", MAINTD_LOGFILE);

	log("probe_bouncers\n");
	if (!probe_bouncers(ml, mlmmjbounce, logfd))
		log_err("An error occurred while sending probes for bouncers, "
		    "see %s", MAINTD_LOGFILE);

	log("run_digests\n");
	if (!run_digests(ml, mlmmjsend, logfd))
		log_err("An error occurred while running digests, see %s",
		   MAINTD_LOGFILE);

	close(logfd);

	if (renameat(ml->fd, logname, ml->fd, MAINTD_LOGFILE) < 0)
		log_error(LOG_ARGS, "Could not rename(%s,%s)",
				logname, MAINTD_LOGFILE);

	free(logname);
}

int main(int argc, char **argv)
{
	int opt;
	char *bindir, *listdir = NULL, *mlmmjsend, *mlmmjbounce;
	char *dirlists = NULL;
	struct dirent *dp;
	DIR *dirp;
	bool daemonize = true;

	CHECKFULLPATH(argv[0]);

	log_set_name(argv[0]);

	while ((opt = getopt(argc, argv, "hFVL:d:")) != -1) {
		switch(opt) {
		case 'd':
			dirlists = optarg;
			break;
		case 'F':
			daemonize = false;
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
	free(bindir);

	if(daemonize && mydaemon() < 0) {
		log_error(LOG_ARGS, "Could not daemonize. Only one "
				"maintenance run will be done.");
		daemonize = false;
	}

	if (!daemonize)
		log_activate_stderr();

	while(1) {
		if(listdir) {
			struct ml ml;
			ml_init(&ml);
			ml.dir = listdir;
			log_set_namef("%s(%s)", argv[0], listdir);
			if (!ml_open(&ml, true))
				goto mainsleep;
			do_maintenance(&ml, mlmmjsend, mlmmjbounce);
			ml_close(&ml);
			goto mainsleep;
		}

		int dfd = open(dirlists, O_DIRECTORY|O_RDONLY);
		if (dfd == -1 || (dirp = fdopendir(dfd)) == NULL) {
			log_err("Could not open the directory containing "
			    "mailing lists (%s): %s", dirlists,
			    strerror(errno));
			free(mlmmjbounce);
			free(mlmmjsend);
			exit(EXIT_FAILURE);
		}

		while((dp = readdir(dirp)) != NULL) {
			char *l;
			if((strcmp(dp->d_name, "..") == 0) ||
					(strcmp(dp->d_name, ".") == 0))
				continue;

			xasprintf(&l, "%s/%s", dirlists, dp->d_name);
			log_set_namef("%s(%s)", argv[0], l);
			struct ml ml;
			ml_init(&ml);
			ml.dir = l;
			if (!ml_open(&ml, true))
				continue;
			do_maintenance(&ml, mlmmjsend, mlmmjbounce);
			ml_close(&ml);
			free(l);
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

	log_free_name();
		
	exit(EXIT_SUCCESS);
}
