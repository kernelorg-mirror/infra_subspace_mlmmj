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

extern char *subtypes[];

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


int main(int argc, char **argv)
{
	char *mlmmjsend, *bindir;
	char *address = NULL;
	struct subscription sub = { 0 };
	int opt;
	bool changeuid = true, digest = false, nomail = false, both = false;
	struct stat st;
	uid_t uid;
	struct ml ml;

	ml_init(&ml);
	CHECKFULLPATH(argv[0]);

	log_set_name(argv[0]);

	bindir = mydirname(argv[0]);
	xasprintf(&mlmmjsend, "%s/mlmmj-send", bindir);
	free(bindir);
	sub.reasonsub = SUB_ADMIN;
	sub.typesub = SUB_NORMAL;
	sub.gensubscribed = true;
	sub.mlmmjsend = mlmmjsend;

	while ((opt = getopt(argc, argv, "hbcCdfm:nsVUL:a:qrR")) != -1) {
		switch(opt) {
		case 'a':
			if (strchr(optarg, '@') == NULL)
				errx(EXIT_FAILURE, "No '@' in the provided email address '%s'",
				    optarg);
			address = optarg;
			break;
		case 'b':
			both = true;
			break;
		case 'c':
			sub.send_welcome_email = true;
			break;
		case 'C':
			sub.subconfirm = true;
			break;
		case 'd':
			digest = true;
			break;
		case 'f':
			sub.force = true;
			break;
		case 'h':
			print_help(argv[0]);
			break;
		case 'L':
			ml.dir = optarg;
			break;
		case 'm':
			sub.modstr = optarg;
			break;
		case 'n':
			nomail = true;
			break;
		case 'q':
			sub.quiet = true;
			break;
		case 'r':
			sub.reasonsub = SUB_REQUEST;
			break;
		case 'R':
			sub.reasonsub = SUB_CONFIRM;
			break;
		case 's':
			sub.gensubscribed = false;
			break;
		case 'U':
			changeuid = false;
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

	if(address == NULL && sub.modstr == NULL) {
		errx(EXIT_FAILURE, "You have to specify -a or -m\n"
		    "%s -h for help", argv[0]);
	}

	if(both + digest + nomail > 1) {
		errx(EXIT_FAILURE, "Specify at most one of -b, -d and -n\n"
		    "%s -h for help", argv[0]);
	}

	if(digest)
		sub.typesub = SUB_DIGEST;
	if(nomail)
		sub.typesub = SUB_NOMAIL;
	if(both)
		sub.typesub = SUB_BOTH;

	if(sub.reasonsub == SUB_CONFIRM && sub.subconfirm) {
		errx(EXIT_FAILURE, "Cannot specify both -C and -R\n"
		    "%s -h for help", argv[0]);
	}

	if(changeuid) {
		uid = getuid();
		if(!uid && fstat(ml.fd, &st) == 0 && uid != st.st_uid) {
			printf("Changing to uid %d gid %d, owner of %s.\n",
					(int)st.st_uid, (int)st.st_gid,
					ml.dir);
			if(setgid(st.st_gid) < 0)
				err(EXIT_FAILURE, "setgid");
			if(setuid(st.st_uid) < 0)
				err(EXIT_FAILURE, "setuid");
		}
	}

	if (do_subscribe(&ml, &sub, address))
		exit(EXIT_SUCCESS);
	exit(EXIT_FAILURE);
}
