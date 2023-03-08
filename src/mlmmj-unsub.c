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
	char *address = NULL;
	const char *subdir;
	enum subtype typesub = SUB_ALL;
	enum subreason reasonsub = SUB_ADMIN;
	uid_t uid;
	struct stat st;
	struct ml ml;

	ml_init(&ml);
	log_set_name(argv[0]);

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
		generate_subconfirm(&ml, address, typesub, reasonsub, false);

	unsubscribe(ml.fd, address, typesub);

	if(confirmunsub)
		confirm_sub(&ml, address, typesub, reasonsub, false);

	notifysub = !quiet && statctrl(ml.ctrlfd, "notifysub");

        /* Notify list owner about subscription */
        if (notifysub)
                notify_sub(&ml, address, typesub, reasonsub, false);

	free(address);

	return EXIT_SUCCESS;
}
