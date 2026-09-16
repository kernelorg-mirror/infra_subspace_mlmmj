/* Copyright (C) 2002, 2003 Mads Martin Joergensen <mmj at mmj.dk>
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
	bool normal = false, digest = false, nomail = false;
	bool unsubconfirm = false;
	bool changeuid = true, quiet = false;
	bool inform_not_subscribed = true;
	bool send_goodbye_mail = false;
	char *address = NULL;
	enum subtype typesub = SUB_ALL;
	enum subreason reasonsub = SUB_ADMIN;
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
			send_goodbye_mail = true;
			break;
		case 'C':
			unsubconfirm = true;
			break;
		case 'd':
			digest = true;
			break;
		case 'h':
			print_help(argv[0]);
			break;
		case 'n':
			nomail = true;
			break;
		case 'N':
			normal = true;
			break;
		case 'q':
			quiet = true;
			break;
		case 'r':
			reasonsub = SUB_REQUEST;
			break;
		case 'R':
			reasonsub = SUB_CONFIRM;
			break;
		case 's':
			inform_not_subscribed = false;
			break;
		case 'U':
			changeuid = false;
			break;
		case 'V':
			print_version(argv[0]);
			exit(0);
		}
	}
	if(ml.dir == NULL || address == NULL) {
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

	if(send_goodbye_mail && unsubconfirm) {
		errx(EXIT_FAILURE, "Cannot specify both -C and -c\n"
		    "%s -h for help", argv[0]);
	}

	if(reasonsub == SUB_CONFIRM && unsubconfirm) {
		errx(EXIT_FAILURE, "Cannot specify both -C and -R\n"
		    "%s -h for help", argv[0]);
	}

	if(changeuid)
		ml_become_list_owner(&ml);
	if (do_unsubscribe(&ml, address, typesub, reasonsub, inform_not_subscribed,
	    unsubconfirm, quiet, send_goodbye_mail))
		return(EXIT_SUCCESS);
	return(EXIT_FAILURE);
}
