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
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <libgen.h>
#include <dirent.h>
#include <sys/wait.h>
#include <ctype.h>
#include <err.h>

#include "xmalloc.h"
#include "mlmmj.h"
#include "wrappers.h"
#include "subscriberfuncs.h"
#include "strgen.h"
#include "log_error.h"
#include "statctrl.h"
#include "prepstdreply.h"
#include "utils.h"

void confirm_unsub(struct ml *ml, const char *subaddr, const char *mlmmjsend,
		   enum subtype typesub, enum subreason reasonsub)
{
	text *txt;
	char *queuefilename, *fromaddr, *listtext;

	gen_addr(fromaddr, ml, "bounces-help");

	switch(typesub) {
		default:
		case SUB_NORMAL:
			listtext = xstrdup("unsub-ok");
			break;
		case SUB_DIGEST:
			listtext = xstrdup("unsub-ok-digest");
			break;
		case SUB_NOMAIL:
			listtext = xstrdup("unsub-ok-nomail");
			break;
	}

	txt = open_text(ml->fd, "finish", "unsub",
			subreason_strs[reasonsub], subtype_strs[typesub],
			listtext);
	free(listtext);
	MY_ASSERT(txt);
	register_default_unformatted(txt, ml);
	register_unformatted(txt, "subaddr", subaddr);
	queuefilename = prepstdreply(txt, ml, "$helpaddr$", subaddr, NULL);
	MY_ASSERT(queuefilename);
	close_text(txt);

	exec_or_die(mlmmjsend, "-l", "1", "-L", ml->dir, "-T", subaddr, "-F",
	    fromaddr, "-m", queuefilename, NULL);
}

void generate_unsubconfirm(struct ml *ml, const char *subaddr,
			   const char *mlmmjsend,
			   enum subtype typesub, enum subreason reasonsub)
{
	char *confirmaddr, *tmpstr;
	text *txt;
	char *queuefilename, *fromaddr;
	char *randomstr = NULL, *confirmfilename = NULL, *listtext;
	int subconffd;

        do {
		free(confirmfilename);
		free(randomstr);
		randomstr = random_str();
		xasprintf(&confirmfilename, "%s/unsubconf/%s", ml->dir,
		    randomstr);

                subconffd = open(confirmfilename, O_RDWR|O_CREAT|O_EXCL,
						  S_IRUSR|S_IWUSR);

        } while ((subconffd < 0) && (errno == EEXIST));

	if(subconffd < 0) {
		log_error(LOG_ARGS, "Could not open '%s'", confirmfilename);
                free(randomstr);
		free(confirmfilename);
		exit(EXIT_FAILURE);
	}

	free(confirmfilename);

	if(dprintf(subconffd, "%s", subaddr) < 0) {
		log_error(LOG_ARGS, "Could not write unsubconffile");
                free(randomstr);
		free(confirmfilename);
		exit(EXIT_FAILURE);
	}

	close(subconffd);

	gen_addr_cookie(fromaddr, ml, "bounces-confunsub", randomstr);

	switch(typesub) {
		default:
		case SUB_NORMAL:
			listtext = xstrdup("unsub-confirm");
			tmpstr = xstrdup("confunsub-");
			break;
		case SUB_DIGEST:
			listtext = xstrdup("unsub-confirm-digest");
			tmpstr = xstrdup("confunsub-digest-");
			break;
		case SUB_NOMAIL:
			listtext = xstrdup("unsub-confirm-nomail");
			tmpstr = xstrdup("confunsub-nomail-");
			break;
	}

	gen_addr_cookie(confirmaddr, ml, tmpstr, randomstr);

	free(randomstr);
	free(tmpstr);

	txt = open_text(ml->fd, "confirm", "unsub",
			subreason_strs[reasonsub], subtype_strs[typesub],
			listtext);
	free(listtext);
	MY_ASSERT(txt);
	register_default_unformatted(txt, ml);
	register_unformatted(txt, "subaddr", subaddr);
	register_unformatted(txt, "confaddr", confirmaddr); /* DEPRECATED */
	register_unformatted(txt, "confirmaddr", confirmaddr);
	queuefilename = prepstdreply(txt, ml, "$helpaddr$", subaddr, confirmaddr);
	MY_ASSERT(queuefilename);
	close_text(txt);

	exec_or_die(mlmmjsend, "-l", "1", "-L", ml->dir, "-T", subaddr,
	    "-F", fromaddr, "-m", queuefilename, NULL);
}

static void print_help(const char *prg)
{
	printf("Usage: %s -L /path/to/list -a john@doe.org\n"
	       "       [-c | -C] [-h] [-L] [-d | -n | -N] [-q] [-r | -R] [-s] [-V]\n"
	       " -a: Email address to unsubscribe \n"
	       " -c: Send goodbye mail\n"
	       " -C: Request mail confirmation\n"
	       " -d: Unsubscribe from digest of list\n"
	       " -h: This help\n"
	       " -L: Full path to list directory\n"
	       " -n: Unsubscribe from no mail version of list\n" 
	       " -N: Unsubscribe from normal version of list\n", prg);
	printf(" -q: Be quiet (don't notify owner about the subscription)\n"
	       " -r: Behave as if request arrived via email (internal use)\n"
	       " -R: Behave as if confirmation arrived via email (internal use)\n"
	       " -s: Don't send a mail to the address if not subscribed\n"
	       " -U: Don't switch to the user id of the listdir owner\n"
	       " -V: Print version\n"
	       "To ensure a silent unsubscription, use -q -s\n");
	exit(EXIT_SUCCESS);
}

int main(int argc, char **argv)
{
	int opt;
	int normal = 0, digest = 0, nomail = 0, subbed;
	int confirmunsub = 0, unsubconfirm = 0, notifysub = 0;
	int changeuid = 1, quiet = 0;
	int nogennotsubscribed = 0;
	int status;
	char *address = NULL;
	char *mlmmjsend, *bindir;
	const char *subdir;
	enum subtype typesub = SUB_ALL;
	enum subreason reasonsub = SUB_ADMIN;
	uid_t uid;
	struct stat st;
	pid_t pid, childpid;
	struct ml ml;

	CHECKFULLPATH(argv[0]);
	ml_init(&ml);
	
	log_set_name(argv[0]);

	bindir = mydirname(argv[0]);
	xasprintf(&mlmmjsend, "%s/mlmmj-send", bindir);
	free(bindir);

	while ((opt = getopt(argc, argv, "hcCdenNVUL:a:sqrR")) != -1) {
		switch(opt) {
		case 'L':
			ml.dir = optarg;
			break;
		case 'a':
			if (strchr(optarg, '@') == NULL)
				errx(EXIT_FAILURE, "No '@' in the provided email address '%s'",
				    optarg);
			address = optarg;
			break;
		case 'c':
			confirmunsub = 1;
			break;
		case 'C':
			unsubconfirm = 1;
			break;
		case 'd':
			digest = 1;
			break;
		case 'h':
			print_help(argv[0]);
			break;
		case 'n':
			nomail = 1;
			break;
		case 'N':
			normal = 1;
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
			nogennotsubscribed = 1;
			break;
		case 'U':
			changeuid = 0;
			break;
		case 'V':
			print_version(argv[0]);
			exit(0);
		}
	}
	if(ml.dir == 0 || address == 0) {
		errx(EXIT_FAILURE, "You have to specify -L and -a\n"
		    "%s -h for help", argv[0]);
	}
	if (!ml_open(&ml, false))
		exit(EXIT_FAILURE);

	if(digest + nomail + normal > 1) {
		errx(EXIT_FAILURE, "Specify at most one of -d, -n and -N\n"
		    "%s -h for help", argv[0]);
	}

	if(digest)
		typesub = SUB_DIGEST;
	if(nomail)
		typesub = SUB_NOMAIL;
	if(normal)
		typesub = SUB_NORMAL;


	if(confirmunsub && unsubconfirm) {
		errx(EXIT_FAILURE, "Cannot specify both -C and -c\n"
		    "%s -h for help", argv[0]);
	}

	if(reasonsub == SUB_CONFIRM && unsubconfirm) {
		errx(EXIT_FAILURE, "Cannot specify both -C and -R\n"
		    "%s -h for help", argv[0]);
	}

	/* Make the address lowercase */
	address = lowercase(address);

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

	if (typesub == SUB_ALL) {
		subbed = is_subbed(ml.fd, address, false) != SUB_NONE;
	} else {
		int subdirfd = open_subscriber_directory(ml.fd, typesub, &subdir);
		if (subdirfd == -1) {
			log_error(LOG_ARGS, "Could not opendir(%s/%s)", ml.dir, subdir);
			exit(EXIT_FAILURE);
		}
		subbed = is_subbed_in(subdirfd, subdir, address);
	}

	if(!subbed) {
		/* Address is not subscribed */
		if(!nogennotsubscribed)
			generate_subscription(&ml, address, typesub, false);
		exit(EXIT_SUCCESS);
	}

	if(unsubconfirm)
		generate_unsubconfirm(&ml, address, mlmmjsend, typesub, reasonsub);

	unsubscribe(ml.fd, address, typesub);

	if(confirmunsub) {
		childpid = fork();

		if(childpid < 0) {
			log_error(LOG_ARGS, "Could not fork");
			confirm_unsub(&ml, address, mlmmjsend, typesub, reasonsub);
		}

		if(childpid > 0) {
			do /* Parent waits for the child */
				pid = waitpid(childpid, &status, 0);
			while(pid == -1 && errno == EINTR);
		}

		/* child confirms subscription */
		if(childpid == 0)
			confirm_unsub(&ml, address, mlmmjsend, typesub, reasonsub);
	}

	notifysub = !quiet && statctrl(ml.ctrlfd, "notifysub");

        /* Notify list owner about subscription */
        if (notifysub)
                notify_sub(&ml, address, typesub, reasonsub, false);

	free(address);
	free(mlmmjsend);

	return EXIT_SUCCESS;
}
