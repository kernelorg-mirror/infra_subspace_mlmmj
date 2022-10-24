/* Copyright (C) 2004 Mads Martin Joergensen <mmj at mmj.dk>
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
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <time.h>
#include <fcntl.h>
#include <err.h>

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

static int mydaemon(const char *rootdir)
{
	if (daemon(1, 0) != 0) {
		log_error(LOG_ARGS, "Unable to become a daemon");
		err(EXIT_FAILURE, "Unable to become a daemon");
	}

	if(chdir(rootdir) < 0)
		log_error(LOG_ARGS, "Could not chdir(%s)", rootdir);

	closefrom(3);
	
	return 0;
}

int delolder(const char *dirname, time_t than)
{
	DIR *dir;
	struct dirent *dp;
	struct stat st;
	time_t t;

	if(chdir(dirname) < 0) {
		log_error(LOG_ARGS, "Could not chdir(%s)", dirname);
		return -1;
	}
	if((dir = opendir(dirname)) == NULL) {
		log_error(LOG_ARGS, "Could not opendir(%s)", dirname);
		return -1;
	}

	while((dp = readdir(dir)) != NULL) {
		if(stat(dp->d_name, &st) < 0) {
			log_error(LOG_ARGS, "Could not stat(%s)", dp->d_name);
			continue;
		}
		if(!S_ISREG(st.st_mode))
			continue;
		t = time(NULL);
		if(t - st.st_mtime > than)
			unlink(dp->d_name);
	}
	closedir(dir);

	return 0;
}


int clean_moderation(const char *listdir)
{

	time_t modreqlife = 0;
	char *modreqlifestr;
	char *moddirname;
	int ret;

	modreqlifestr = ctrlvalue(listdir, "modreqlife");
	if(modreqlifestr) {
		modreqlife = atol(modreqlifestr);
		free(modreqlifestr);
	}
	if(modreqlife == 0)
		modreqlife = MODREQLIFE;

	moddirname = concatstr(2, listdir, "/moderation");
	ret = delolder(moddirname, modreqlife);
		
	free(moddirname);

	return ret;
}

int clean_discarded(const char *listdir)
{
	char *discardeddirname = concatstr(2, listdir, "/queue/discarded");
	int ret = delolder(discardeddirname, DISCARDEDLIFE);

	free(discardeddirname);

	return ret;
}

int clean_subconf(const char *listdir)
{
	char *subconfdirname = concatstr(2, listdir, "/subconf");
	int ret = delolder(subconfdirname, CONFIRMLIFE);

	free(subconfdirname);

	return ret;
}

int clean_unsubconf(const char *listdir)
{
	char *unsubconfdirname = concatstr(2, listdir, "/unsubconf");
	int ret = delolder(unsubconfdirname, CONFIRMLIFE);

	free(unsubconfdirname);

	return ret;
}

int discardmail(const char *old, const char *new, time_t age)
{
	struct stat st;
	time_t t;
	int fd, ret = 0;

	fd = open(old, O_RDWR|O_EXLOCK);
	if(fd < 0)
		return 0;

	stat(old, &st);
	t = time(NULL);

	if(t - st.st_mtime > age) {
		if(rename(old, new) < 0)
			ret = 0;
		else
			ret = 1;
	}

	close(fd);
	return ret;
}

int resend_queue(const char *listdir, const char *mlmmjsend)
{
	DIR *queuedir;
	struct dirent *dp;
	char *mailname, *fromname, *toname, *reptoname, *from, *to, *repto;
	char *ch, *dirname = concatstr(2, listdir, "/queue/");
	char *bouncelifestr;
	struct stat st;
	int fromfd, tofd, fd, err = 0;
	time_t t, bouncelife = 0;

	if(chdir(dirname) < 0) {
		log_error(LOG_ARGS, "Could not chdir(%s)", dirname);
		free(dirname);
		return 1;
	}
		
	if((queuedir = opendir(dirname)) == NULL) {
		log_error(LOG_ARGS, "Could not opendir(%s)", dirname);
		free(dirname);
		return 1;
	}
	free(dirname);

	while((dp = readdir(queuedir)) != NULL) {
		mailname = concatstr(3, listdir, "/queue/", dp->d_name);

		if(stat(mailname, &st) < 0) {
			log_error(LOG_ARGS, "Could not stat(%s)", mailname);
			free(mailname);
			continue;
		}

		if(!S_ISREG(st.st_mode)) {
			free(mailname);
			continue;
		}

		if(strchr(dp->d_name, '.')) {
			ch = strrchr(mailname, '.');
			MY_ASSERT(ch);
			*ch = '\0';
			/* delete orphaned sidecar files */
			if(stat(mailname, &st) < 0) {
				if(errno == ENOENT) {
					*ch = '.';
					unlink(mailname);
				}
			}
			free(mailname);
			continue;
		}

		fromname = concatstr(2, mailname, ".mailfrom");
		toname = concatstr(2, mailname, ".reciptto");
		reptoname = concatstr(2, mailname, ".reply-to");

		fromfd = open(fromname, O_RDONLY);
		if(fromfd < 0)
			err = errno;
		tofd = open(toname, O_RDONLY);

		if((fromfd < 0 && err == ENOENT) ||
				(tofd < 0 && errno == ENOENT)) {
			/* only delete old files to avoid deleting
			   mail currently being sent */
			t = time(NULL);
			if(stat(mailname, &st) == 0) {
				if(t - st.st_mtime > (time_t)36000) {
					unlink(mailname);
					/* avoid leaving orphans */
					unlink(fromname);
					unlink(toname);
					unlink(reptoname);
				}
			}
			free(mailname);
			free(fromname);
			free(toname);
			free(reptoname);
			if(fromfd >= 0)
				close(fromfd);
			if(tofd >= 0)
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
		fd = open(reptoname, O_RDONLY);
		if(fd < 0) {
			free(reptoname);
			repto = NULL;
		} else {
			repto = mygetline(fd);
			chomp(repto);
			close(fd);
			free(reptoname);
		}

		/* before we try again, check and see if it's old */
		bouncelifestr = ctrlvalue(listdir, "bouncelife");
		if(bouncelifestr) {
			bouncelife = atol(bouncelifestr);
			free(bouncelifestr);
		}
		if(bouncelife == 0)
			bouncelife = BOUNCELIFE;

		t = time(NULL);
		if(t - st.st_mtime > bouncelife) {
			unlink(mailname);
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

int resend_requeue(const char *listdir, const char *mlmmjsend)
{
	DIR *queuedir;
	struct dirent *dp;
	char *dirname = concatstr(2, listdir, "/requeue/");
	char *archivefilename, *subfilename, *subnewname;
	struct stat st;
	time_t t;
	int fromrequeuedir;

	if (statctrl(listdir, "nobounceprobe")) {
		return 0;
	}

	if(chdir(dirname) < 0) {
		log_error(LOG_ARGS, "Could not chdir(%s)", dirname);
		free(dirname);
		return 1;
	}
		
	if((queuedir = opendir(dirname)) == NULL) {
		log_error(LOG_ARGS, "Could not opendir(%s)", dirname);
		free(dirname);
		return 1;
	}
	
	while((dp = readdir(queuedir)) != NULL) {
		if((strcmp(dp->d_name, "..") == 0) ||
			(strcmp(dp->d_name, ".") == 0))
				continue;

		if(stat(dp->d_name, &st) < 0) {
			log_error(LOG_ARGS, "Could not stat(%s)",dp->d_name);
			continue;
		}

		if(!S_ISDIR(st.st_mode))
			continue;

		/* Remove old empty directories */
		t = time(NULL);
		if(t - st.st_mtime > (time_t)3600)
			if(rmdir(dp->d_name) == 0)
				continue;

		archivefilename = concatstr(3, listdir, "/archive/",
						dp->d_name);

		/* Explicitly initialize for each mail we examine */
		fromrequeuedir = 0;

		if(stat(archivefilename, &st) < 0) {
			/* Might be it's just not moved to the archive
			 * yet because it's still getting sent, so just
			 * continue
			 */
			free(archivefilename);

			/* If the list is set not to archive we want to look
			 * in /requeue/ for a mailfile
			 */
			archivefilename = concatstr(4, listdir, "/requeue/",
							dp->d_name, "/mailfile");
			if(stat(archivefilename, &st) < 0) {
				free(archivefilename);
				continue;
			}
			fromrequeuedir = 1;
		}
		subfilename = concatstr(3, dirname, dp->d_name, "/subscribers");
		if(stat(subfilename, &st) < 0) {
			if (fromrequeuedir)
				unlink(archivefilename);
			free(archivefilename);
			free(subfilename);
			continue;
		}

		subnewname = concatstr(2, subfilename, ".resending");

		if(rename(subfilename, subnewname) < 0) {
			log_error(LOG_ARGS, "Could not rename(%s, %s)",
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

	free(dirname);

	return 0;
}

int clean_nolongerbouncing(const char *listdir)
{
	DIR *bouncedir;
	char *dirname = concatstr(2, listdir, "/bounce/");
	char *filename, *probetimestr, *s;
	int probefd;
	time_t probetime, t;
	struct dirent *dp;
	struct stat st;
	
	if(chdir(dirname) < 0) {
		log_error(LOG_ARGS, "Could not chdir(%s)", dirname);
		free(dirname);
		return 1;
	}
		
	if((bouncedir = opendir(dirname)) == NULL) {
		log_error(LOG_ARGS, "Could not opendir(%s)", dirname);
		free(dirname);
		return 1;
	}

	free(dirname);

	while((dp = readdir(bouncedir)) != NULL) {
		if((strcmp(dp->d_name, "..") == 0) ||
		   (strcmp(dp->d_name, ".") == 0))
				continue;

		filename = xstrdup(dp->d_name);

		s = strrchr(filename, '-');
		if(s && (strcmp(s, "-probe") == 0)) {
			if(stat(filename, &st) < 0) {
				log_error(LOG_ARGS, "Could not stat(%s)",
					  filename);
				free(filename);
				continue;
			}

			probefd = open(filename, O_RDONLY);
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
			probetime = (time_t)strtol(probetimestr, NULL, 10);
			free(probetimestr);
			t = time(NULL);
			if(t - probetime > WAITPROBE) {
				unlink(filename);
				/* remove -probe onwards from filename */
				*s = '\0';
				unlink(filename);
				s = concatstr(2, filename, ".lastmsg");
				unlink(s);
				free(s);
			}
		}
		free(filename);
	}

	closedir(bouncedir);

	return 0;
}

int probe_bouncers(const char *listdir, const char *mlmmjbounce)
{
	DIR *bouncedir;
	char *dirname = concatstr(2, listdir, "/bounce/");
	char *probefile, *s;
	struct dirent *dp;
	struct stat st;
	
	if(chdir(dirname) < 0) {
		log_error(LOG_ARGS, "Could not chdir(%s)", dirname);
		free(dirname);
		return 1;
	}
		
	if((bouncedir = opendir(dirname)) == NULL) {
		log_error(LOG_ARGS, "Could not opendir(%s)", dirname);
		free(dirname);
		return 1;
	}

	free(dirname);

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
			
		if(stat(dp->d_name, &st) < 0) {
			log_error(LOG_ARGS, "Could not stat(%s)", dp->d_name);
			continue;
		}
		
		probefile = concatstr(2, dp->d_name, "-probe");
		
		/* Skip files which already have a probe out */
		if(stat(probefile, &st) == 0) {
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

int unsub_bouncers(const char *listdir, const char *mlmmjunsub)
{
	DIR *bouncedir;
	char *dirname = concatstr(2, listdir, "/bounce/");
	char *probefile, *address, *a, *firstbounce, *bouncedata;
	char *bouncelifestr;
	struct dirent *dp;
	struct stat st;
	int fd;
	time_t bouncetime, t, bouncelife = 0;
	
	if(chdir(dirname) < 0) {
		log_error(LOG_ARGS, "Could not chdir(%s)", dirname);
		free(dirname);
		return 1;
	}
		
	if((bouncedir = opendir(dirname)) == NULL) {
		log_error(LOG_ARGS, "Could not opendir(%s)", dirname);
		free(dirname);
		return 1;
	}

	free(dirname);

	bouncelifestr = ctrlvalue(listdir, "bouncelife");
	if(bouncelifestr) {
		bouncelife = atol(bouncelifestr);
		free(bouncelifestr);
	}
	
	if(bouncelife == 0)
		bouncelife = BOUNCELIFE;

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
		
		if(stat(dp->d_name, &st) < 0) {
			log_error(LOG_ARGS, "Could not stat(%s)", dp->d_name);
			continue;
		}
		
		probefile = concatstr(2, dp->d_name, "-probe");
		
		/* Skip files which already have a probe out */
		if(stat(probefile, &st) == 0) {
			free(probefile);
			continue;
		}
		free(probefile);

		/* Get the first line of the bounce file to check if it's
		 * been bouncing for long enough
		 */
		fd = open(dp->d_name, O_RDONLY);
		if(fd < 0) {
			log_error(LOG_ARGS, "Could not open %s", dp->d_name);
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
		bouncetime = (time_t)strtol(a, NULL, 10);
		free(firstbounce);
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
		unlink(dp->d_name);
		a = concatstr(2, dp->d_name, ".lastmsg");
		unlink(a);
		free(a);
	}
	closedir(bouncedir);

	return 0;
}

int run_digests(const char *listdir, const char *mlmmjsend)
{
	char *lasttimestr, *lastindexstr, *lastissuestr;
	char *digestname, *indexname;
	char *digestintervalstr, *digestmaxmailsstr;
	char *s1, *s2;
	time_t digestinterval, t, lasttime;
	long digestmaxmails, lastindex, index, lastissue;
	int fd, indexfd;

	if (statctrl(listdir, "noarchive")) {
		return 0;
	}
	
	digestintervalstr = ctrlvalue(listdir, "digestinterval");
	if (digestintervalstr) {
		digestinterval = (time_t)atol(digestintervalstr);
		free(digestintervalstr);
	} else {
		digestinterval = (time_t)DIGESTINTERVAL;
	}

	digestmaxmailsstr = ctrlvalue(listdir, "digestmaxmails");
	if (digestmaxmailsstr) {
		digestmaxmails = atol(digestmaxmailsstr);
		free(digestmaxmailsstr);
	} else {
		digestmaxmails = DIGESTMAXMAILS;
	}

	digestname = concatstr(2, listdir, "/lastdigest");
	fd = open(digestname, O_RDWR|O_CREAT|O_EXLOCK, S_IRUSR | S_IWUSR);
	if (fd < 0) {
		log_error(LOG_ARGS, "Could not open '%s'", digestname);
		free(digestname);
		return 1;
	}

	s1 = mygetline(fd);

	/* Syntax is lastindex:lasttime or lastindex:lasttime:lastissue */
	if (s1 && (lasttimestr = strchr(s1, ':'))) {
		*(lasttimestr++) = '\0';
		if ((lastissuestr = strchr(lasttimestr, ':'))) {
			*(lastissuestr++) = '\0';
			lastissue = atol(lastissuestr);
		} else {
			lastissue = 0;
		}
		lasttime = atol(lasttimestr);
		lastindexstr = s1;
		lastindex = atol(lastindexstr);
	} else {
		if (s1 && (strlen(s1) > 0)) {
			log_error(LOG_ARGS, "'%s' contains malformed data",
					digestname);
			free(digestname);
			free(s1);
			close(fd);
			return 1;
		}
		/* If lastdigest is empty, we start from scratch */
		lasttime = 0;
		lastindex = 0;
		lastissue = 0;
	}
	
	indexname = concatstr(2, listdir, "/index");
	indexfd = open(indexname, O_RDONLY);
	if (indexfd < 0) {
		log_error(LOG_ARGS, "Could not open '%s'", indexname);
		free(digestname);
		free(indexname);
		free(s1);
		close(fd);
		return 1;
	}
	s2 = mygetline(indexfd);
	close(indexfd);
	if (!s2) {
		/* If we don't have an index, no mails have been sent to the
		 * list, and therefore we don't need to send a digest */
		free(digestname);
		free(indexname);
		free(s1);
		close(fd);
		return 1;
	}

	index = atol(s2);
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
			log_error(LOG_ARGS, "Could not seek '%s'", digestname);
		} else {
			if (dprintf(fd, "%ld:%ld:%ld\n", index, (long)t, lastissue) < 0 ) {
				log_error(LOG_ARGS, "Could not write new '%s'",
						digestname);
			}
		}
	}

	free(digestname);
	free(indexname);
	free(s1);
	free(s2);
	close(fd);
	
	return 0;
}

void do_maintenance(const char *listdir, const char *mlmmjsend,
		    const char *mlmmjbounce, const char *mlmmjunsub)
{
	char *random, *logname, *logstr;
	char timenow[64];
	struct stat st;
	int maintdlogfd;
	uid_t uid = getuid();
	time_t t;

	if(!listdir)
		return;
	
	if(stat(listdir, &st) < 0) {
		log_error(LOG_ARGS, "Could not stat(%s) "
				    "No maintenance run performed.", listdir);
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

	if(chdir(listdir) < 0) {
		log_error(LOG_ARGS, "Could not chdir(%s). "
				    "No maintenance run performed.", listdir);
		return;
	}

	random = random_str();
	logname = concatstr(3, listdir, "/maintdlog-", random);
	free(random);

	maintdlogfd = open(logname, O_WRONLY|O_EXCL|O_CREAT, S_IRUSR|S_IWUSR);
	if(maintdlogfd < 0) {
		log_error(LOG_ARGS, "Could not open %s", logname);
		free(logname);
		return;
	}

	t = time(NULL);
	if(ctime_r(&t, timenow))
		WRITEMAINTLOG4(3, "Starting maintenance run at ",
				timenow, "\n");


	WRITEMAINTLOG4(3, "clean_moderation(", listdir, ");\n");
	clean_moderation(listdir);

	WRITEMAINTLOG4(3, "clean_discarded(", listdir, ");\n");
	clean_discarded(listdir);

	WRITEMAINTLOG4(3, "clean_subconf(", listdir, ");\n");
	clean_subconf(listdir);

	WRITEMAINTLOG4(3, "clean_unsubconf(", listdir, ");\n");
	clean_unsubconf(listdir);

	WRITEMAINTLOG6(5, "resend_queue(", listdir, ", ", mlmmjsend,
			");\n");
	resend_queue(listdir, mlmmjsend);

	WRITEMAINTLOG6(5, "resend_requeue(", listdir, ", ", mlmmjsend,
			");\n");
	resend_requeue(listdir, mlmmjsend);

	WRITEMAINTLOG4(3, "clean_nolongerbouncing(", listdir, ");\n");
	clean_nolongerbouncing(listdir);

	WRITEMAINTLOG6(5, "unsub_bouncers(", listdir, ", ",
			mlmmjunsub, ");\n");
	unsub_bouncers(listdir, mlmmjunsub);

	WRITEMAINTLOG6(5, "probe_bouncers(", listdir, ", ",
			mlmmjbounce, ");\n");
	probe_bouncers(listdir, mlmmjbounce);

	WRITEMAINTLOG6(5, "run_digests(", listdir, ", ", mlmmjsend,
			");\n");
	run_digests(listdir, mlmmjsend);

	close(maintdlogfd);

	logstr = concatstr(3, listdir, "/", MAINTD_LOGFILE);

	if(rename(logname, logstr) < 0)
		log_error(LOG_ARGS, "Could not rename(%s,%s)",
				logname, logstr);

	free(logname);
	free(logstr);
}

int main(int argc, char **argv)
{
	int opt, daemonize = 1, ret = 0;
	char *bindir, *listdir = NULL, *mlmmjsend, *mlmmjbounce, *mlmmjunsub;
	char *dirlists = NULL, *s, *listiter;
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
	mlmmjsend = concatstr(2, bindir, "/mlmmj-send");
	mlmmjbounce = concatstr(2, bindir, "/mlmmj-bounce");
	mlmmjunsub = concatstr(2, bindir, "/mlmmj-unsub");
	free(bindir);

	if(daemonize) {
		if(dirlists)
			ret = mydaemon(dirlists);
		else
			ret = mydaemon(listdir);
	}

	if(daemonize && ret < 0) {
		log_error(LOG_ARGS, "Could not daemonize. Only one "
				"maintenance run will be done.");
		daemonize = 0;
	}

	while(1) {
		if(listdir) {
			do_maintenance(listdir, mlmmjsend, mlmmjbounce,
					mlmmjunsub);
			goto mainsleep;
		}

		if(chdir(dirlists) < 0) {
			log_error(LOG_ARGS, "Could not chdir(%s).",
					dirlists);
			free(mlmmjbounce);
			free(mlmmjsend);
			free(mlmmjunsub);
			exit(EXIT_FAILURE);
		}

		if((dirp = opendir(dirlists)) == NULL) {
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
			
			listiter = concatstr(3, dirlists, "/", dp->d_name);

			if(stat(listiter, &st) < 0) {
				log_error(LOG_ARGS, "Could not stat(%s)",
						listiter);
				free(listiter);
				continue;
			}

			if(!S_ISDIR(st.st_mode)) {
				free(listiter);
				continue;
			}

			s = concatstr(2, listiter, "/control/listaddress");
			ret = stat(s, &st);
			free(s);

			if(ret < 0) { /* If ret < 0 it's not a listiter */
				free(listiter);
				continue;
			}

			do_maintenance(listiter, mlmmjsend, mlmmjbounce,
					mlmmjunsub);

			free(listiter);
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
