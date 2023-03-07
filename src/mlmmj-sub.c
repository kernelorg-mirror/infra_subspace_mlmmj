/* Copyright (C) 2002, 2003 Mads Martin Joergensen <mmj at mmj.dk>
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <syslog.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <libgen.h>
#include <sys/wait.h>
#include <ctype.h>
#include <err.h>

#include "xmalloc.h"
#include "mlmmj.h"
#include "mlmmj-sub.h"
#include "wrappers.h"
#include "strgen.h"
#include "subscriberfuncs.h"
#include "log_error.h"
#include "statctrl.h"
#include "prepstdreply.h"
#include "ctrlvalues.h"
#include "chomp.h"
#include "utils.h"
#include "send_help.h"
#include "xstring.h"

static char *subtypes[7] = {
	"SUB_NORMAL",
	"SUB_DIGEST",
	"SUB_NOMAIL",
	NULL,
	NULL,
	"SUB_BOTH",
	NULL,
};

static void moderate_sub(struct ml *ml, const char *subaddr,
		const char *mlmmjsend, enum subtype typesub,
		enum subreason reasonsub)
{
	int fd, status;
	text *txt;
	memory_lines_state *mls;
	char *a = NULL, *queuefilename, *from;
	char *modfilename, *mods, *to, *replyto, *moderators = NULL;
	char *cookie, *obstruct;
	strlist *submods;
	const char *type;
	pid_t childpid, pid;
	xstring *str = NULL;
	int modfd = -1;

	type = subtypes[typesub];

	for (;;) {
		cookie = random_str();
		xasprintf(&modfilename, "moderation/subscribe%s", cookie);
		fd = openat(ml->fd, modfilename, O_RDWR|O_CREAT|O_EXCL,
		    S_IRUSR|S_IWUSR);
		if (fd < 0) {
			if (errno == EEXIST) {
				free(cookie);
				free(modfilename);
				continue;
			}
			log_error(LOG_ARGS, "could not create %s"
					"ignoring request for: %s", subaddr);
			exit(EXIT_FAILURE);
		}
		break;
	}

	if (dprintf(fd, "%s\n%s\n", subaddr, type) < 0) {
		log_error(LOG_ARGS, "could not write to %s"
				"ignoring request for: %s", subaddr);
		exit(EXIT_FAILURE);
	}
	close(fd);

	submods = ctrlvalues(ml->ctrlfd, "submod");
	if (submods == NULL)
		return;
	/* check to see if there's adresses in the submod control file */
	tll_foreach(*submods, it)
		a = strchr(it->item, '@');

	/* no addresses in submod control file, use owner */
	if(a == NULL) {
		/* free the submods struct from above */
		tll_free_and_free(*submods, free);
		free(submods);
		submods = ctrlvalues(ml->ctrlfd, "owner");
		modfd = openat(ml->ctrlfd, "owner", O_RDONLY);
	}
	if (modfd == -1)
		modfd = openat(ml->ctrlfd, "submod", O_RDONLY);

	gen_addr(from, ml, "owner");
	xasprintf(&to, "%s-moderators@%s", ml->name, ml->fqdn);
	gen_addr_cookie(replyto, ml, "permit-", cookie);
	gen_addr_cookie(obstruct, ml, "obstruct-", cookie);
	free(cookie);
	tll_foreach(*submods, sm) {
		if (str == NULL)
			str = xstring_new();
		fprintf(str->fp, "%s\n", sm->item);
	}
	moderators = xstring_get(str);
	mls = init_memory_lines(moderators);
	free(moderators);

	txt = open_text(ml->fd,
			"gatekeep", "sub",
			subreason_strs[reasonsub], subtype_strs[typesub],
			"submod-moderator");
	MY_ASSERT(txt);
	register_default_unformatted(txt, ml);
	register_unformatted(txt, "subaddr", subaddr);
	register_unformatted(txt, "moderateaddr", replyto); /* DEPRECATED */
	register_unformatted(txt, "permitaddr", replyto);
	register_unformatted(txt, "obstructaddr", obstruct);
	register_unformatted(txt, "moderators", "%gatekeepers%"); /* DEPRECATED */
	register_formatted(txt, "gatekeepers",
			rewind_memory_lines, get_memory_line, mls);
	queuefilename = prepstdreply(txt, ml, "$listowner$", to, replyto);
	MY_ASSERT(queuefilename);
	close_text(txt);
	
	/* we might need to exec more than one mlmmj-send */

	if (statctrl(ml->ctrlfd, "nosubmodmails"))
		childpid = -1;
	else {
		childpid = fork();
		if(childpid < 0)
			log_error(LOG_ARGS, "Could not fork; requester not notified");
	}

	if(childpid != 0) {
		if(childpid > 0) {
			do /* Parent waits for the child */
				pid = waitpid(childpid, &status, 0);
			while(pid == -1 && errno == EINTR);
		}
		finish_memory_lines(mls);
		xasprintf(&mods, "%d", modfd);
		execl(mlmmjsend, mlmmjsend,
				"-a",
				"-l", "4",
				"-L", ml->dir,
				"-s", mods,
				"-F", from,
				"-R", replyto,
				"-m", queuefilename, (char *)NULL);
		log_error(LOG_ARGS, "execl() of '%s' failed", mlmmjsend);
		exit(EXIT_FAILURE);
	}

	free(to);
	free(replyto);
	
	/* send mail to requester that the list is submod'ed */

	txt = open_text(ml->fd,
			"wait", "sub",
			subreason_strs[reasonsub], subtype_strs[typesub],
			"submod-requester");
	MY_ASSERT(txt);
	register_default_unformatted(txt, ml);
	register_unformatted(txt, "subaddr", subaddr);
	register_unformatted(txt, "moderators", "%gatekeepers"); /* DEPRECATED */
	register_formatted(txt, "gatekeepers",
			rewind_memory_lines, get_memory_line, mls);
	queuefilename = prepstdreply(txt, ml, "$listowner$", subaddr, NULL);
	MY_ASSERT(queuefilename);
	close_text(txt);

	finish_memory_lines(mls);
	send_help(ml, queuefilename, subaddr);
}

void getaddrandtype(struct ml *ml, const char *modstr,
		char **addrptr, enum subtype *subtypeptr)
{
	int fd;
	char *readaddr, *readtype, *modfilename, *line = NULL;
	FILE *f;
	size_t linecap = 0;

	if (strncmp(modstr, "subscribe", 9) == 0)
			modstr += 9;

	xasprintf(&modfilename, "moderation/subscribe%s", modstr);

	fd = openat(ml->fd, modfilename, O_RDONLY);
	if(fd < 0) {
		log_error(LOG_ARGS, "Could not open %s/%s", ml->dir, modfilename);
		exit(EXIT_FAILURE);
	}
	f = fdopen(fd, "r");

	if (getline(&line, &linecap, f) <= 0) {
		log_error(LOG_ARGS, "Could not parse %s/%s", ml->dir, modfilename);
		exit(EXIT_FAILURE);
	}
	chomp(line);
	readaddr = xstrdup(line);
	if (getline(&line, &linecap, f) <= 0) {
		log_error(LOG_ARGS, "Could not parse %s/%s", ml->dir, modfilename);
		exit(EXIT_FAILURE);
	}
	chomp(line);
	readtype = xstrdup(line);
	fclose(f);

	*addrptr = readaddr;

	for (size_t i = 0; i < NELEM(subtypes); i++) {
		if (subtypes[i] == NULL)
			continue;
		if (strcmp(subtypes[i], readtype) == 0) {
			*subtypeptr = i;
			goto freedone;
		}
	}

	log_error(LOG_ARGS, "Type %s not valid in %s/%s", readtype,
	    ml->dir, modfilename);

freedone:
	free(readtype);
	unlinkat(ml->fd, modfilename, 0);
	free(modfilename);
}

static void print_help(const char *prg)
{
	printf("Usage: %s -L /path/to/list {-a john@doe.org | -m str}\n"
	       "       [-c] [-C] [-f] [-h] [-L] [-d | -n] [-q] [-r | -R] [-s] [-U] [-V]\n"
	       " -a: Email address to subscribe \n"
	       " -c: Send welcome mail (unless requesting confirmation)\n"
	       " -C: Request mail confirmation (unless switching versions)\n"
	       " -d: Subscribe to digest of list\n"
	       " -f: Force subscription (do not moderate)\n"
	       " -h: This help\n"
	       " -L: Full path to list directory\n"
	       " -m: moderation string\n"
	       " -n: Subscribe to no mail version of list\n", prg);
	printf(" -q: Be quiet (don't notify owner about the subscription)\n"
	       " -r: Behave as if request arrived via email (internal use)\n"
	       " -R: Behave as if confirmation arrived via email (internal use)\n"
	       " -s: Don't send a mail to subscriber if already subscribed\n"
	       " -U: Don't switch to the user id of the listdir owner\n"
	       " -V: Print version\n"
	       "To ensure a silent subscription, use -f -q -s\n");
	exit(EXIT_SUCCESS);
}

static void subscribe_type(int listfd, char *address, enum subtype typesub) {
	int dirfd;;
	char chstr[2];
	const char *subdir;
	int groupwritable = 0, subfilefd;
	struct stat st;

	dirfd = open_subscriber_directory(listfd, typesub, &subdir);
	if (dirfd == -1)
		err(EXIT_FAILURE, "cannot open(%s)", subdir);
	if (fstat(dirfd, &st) == 0) {
		if(st.st_mode & S_IWGRP) {
			groupwritable = S_IRGRP|S_IWGRP;
			umask(S_IWOTH);
			setgid(st.st_gid);
		}
	}

	chstr[0] = address[0];
	chstr[1] = '\0';

	subfilefd = openat(dirfd, chstr, O_RDWR|O_CREAT|O_APPEND,
				S_IRUSR|S_IWUSR|groupwritable);
	if(subfilefd == -1 && !lock(subfilefd, true)) {
		log_error(LOG_ARGS, "Could not open '%s/%s'", subdir, chstr);
		exit(EXIT_FAILURE);
	}

	dprintf(subfilefd, "%s\n", address);
	close(dirfd);
	close(subfilefd);
}

int main(int argc, char **argv)
{
	char *mlmmjsend, *bindir;
	char *address = NULL, *modstr = NULL;
	int opt, subconfirm = 0, confirmsub = 0, notifysub;
	int changeuid = 1, status, digest = 0, nomail = 0, both = 0;
	int nogensubscribed = 0;
	int force = 0, quiet = 0;
	enum subtype subbed;
	struct stat st;
	pid_t pid, childpid = 0;
	uid_t uid;
	enum subtype typesub = SUB_NORMAL;
	enum subreason reasonsub = SUB_ADMIN;
	struct ml ml;

	ml_init(&ml);
	CHECKFULLPATH(argv[0]);

	log_set_name(argv[0]);

	bindir = mydirname(argv[0]);
	xasprintf(&mlmmjsend, "%s/mlmmj-send", bindir);
	free(bindir);

	while ((opt = getopt(argc, argv, "hbcCdfm:nsVUL:a:qrR")) != -1) {
		switch(opt) {
		case 'a':
			if (strchr(optarg, '@') == NULL)
				errx(EXIT_FAILURE, "No '@' in the provided email address '%s'",
				    optarg);
			address = optarg;
			break;
		case 'b':
			both = 1;
			break;
		case 'c':
			confirmsub = 1;
			break;
		case 'C':
			subconfirm = 1;
			break;
		case 'd':
			digest = 1;
			break;
		case 'f':
			force = 1;
			break;
		case 'h':
			print_help(argv[0]);
			break;
		case 'L':
			ml.dir = optarg;
			break;
		case 'm':
			modstr = optarg;
			break;
		case 'n':
			nomail = 1;
			break;
		case 'q':
			quiet = 1;
			break;
		case 'r':
			reasonsub = SUB_REQUEST;
			break;
		case 'R':
			reasonsub = SUB_CONFIRM;
			break;
		case 's':
			nogensubscribed = 1;
			break;
		case 'U':
			changeuid = 0;
			break;
		case 'V':
			print_version(argv[0]);
			exit(0);
		}
	}

	if(ml.dir == NULL) {
		errx(EXIT_FAILURE, "You have to specify -L\n"
		    "%s -h for help", argv[0]);
	}
	if (!ml_open(&ml, false))
		exit(EXIT_FAILURE);

	if(address == NULL && modstr == NULL) {
		errx(EXIT_FAILURE, "You have to specify -a or -m\n"
		    "%s -h for help", argv[0]);
	}

	if(both + digest + nomail > 1) {
		errx(EXIT_FAILURE, "Specify at most one of -b, -d and -n\n"
		    "%s -h for help", argv[0]);
	}

	if(digest)
		typesub = SUB_DIGEST;
	if(nomail)
		typesub = SUB_NOMAIL;
	if(both)
		typesub = SUB_BOTH;

	if(reasonsub == SUB_CONFIRM && subconfirm) {
		errx(EXIT_FAILURE, "Cannot specify both -C and -R\n"
		    "%s -h for help", argv[0]);
	}

	if(modstr) {
		getaddrandtype(&ml, modstr, &address, &typesub);
		reasonsub = SUB_PERMIT;
	}

	/* Make the address lowercase */
	address = lowercase(address);

	if(strncasecmp(ml.addr, address, strlen(ml.addr)) == 0)
		errx(EXIT_FAILURE, "Cannot subscribe the list address to the list");

	if(changeuid) {
		uid = getuid();
		if(!uid && fstat(ml.fd, &st) == 0 && uid != st.st_uid) {
			printf("Changing to uid %d, owner of %s.\n",
					(int)st.st_uid, ml.dir);
			if(setuid(st.st_uid) < 0) {
				perror("setuid");
				fprintf(stderr, "Continuing as uid %d\n",
				    (int)uid);
			}
		}
	}

	subbed = is_subbed(ml.fd, address, 1);

	if(subbed == typesub) {
		if(!nogensubscribed)
			generate_subscription(&ml, address, typesub, true);
		return EXIT_SUCCESS;
	} else if(subbed != SUB_NONE) {
		reasonsub = SUB_SWITCH;
		/* If we want to subscribe to both, we can just subscribe the
		 * missing version, so don't unsub. */
		if (!(typesub == SUB_BOTH &&
				subbed != SUB_NOMAIL)) {
			enum subtype ts = SUB_ALL;
			if (subbed == SUB_BOTH) {
				if (typesub == SUB_NORMAL) ts = SUB_DIGEST;
				if (typesub == SUB_DIGEST) ts = SUB_NORMAL;
			}
			if (!unsubscribe(ml.fd, address, ts))
				log_error(LOG_ARGS, "not unsubscribed from "
				    "current version");
		}
	}

	if(subbed == SUB_NONE && subconfirm)
		generate_subconfirm(&ml, address, typesub, reasonsub, true);

	if(modstr == NULL && subbed == SUB_NONE && !force &&
			statctrl(ml.ctrlfd, "submod")) {
		moderate_sub(&ml, address, mlmmjsend, typesub, reasonsub);
	}

	if (typesub == SUB_BOTH) {
		if (subbed != SUB_NORMAL) {
			subscribe_type(ml.fd, address, SUB_NORMAL);
		}
		if (subbed != SUB_DIGEST) {
			subscribe_type(ml.fd, address, SUB_DIGEST);
		}
	} else if (!(subbed == SUB_BOTH && typesub != SUB_NOMAIL)) {
		subscribe_type(ml.fd, address, typesub);
	}

	if(confirmsub) {
		childpid = fork();

		if(childpid < 0) {
			log_error(LOG_ARGS, "Could not fork; owner not notified");
			confirm_sub(&ml, address, typesub, reasonsub, true);
		}
		
		if(childpid > 0) {
			do /* Parent waits for the child */
				pid = waitpid(childpid, &status, 0);
			while(pid == -1 && errno == EINTR);
		}

		/* child confirms subscription */
		if(childpid == 0)
			confirm_sub(&ml, address, typesub, reasonsub, true);
	}

	notifysub = !quiet && reasonsub != SUB_SWITCH &&
			statctrl(ml.ctrlfd, "notifysub");

	/* Notify list owner about subscription */
	if (notifysub)
		notify_sub(&ml, address, typesub, reasonsub, true);

	free(address);
	free(mlmmjsend);

	return EXIT_SUCCESS;
}
