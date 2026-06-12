/*
 * Copyright (C) 2004 Morten K. Poulsen <morten at afdelingp.dk>
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
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <dirent.h>
#include <sys/mman.h>
#include <stdbool.h>
#include <ctype.h>
#include <err.h>

#include "chomp.h"
#include "mlmmj.h"
#include "strgen.h"
#include "wrappers.h"
#include "log_error.h"
#include "subscriberfuncs.h"
#include "prepstdreply.h"
#include "xmalloc.h"
#include "find_email_adr.h"
#include "utils.h"
#include "send_mail.h"

void
do_probe(struct ml *ml, const char *addr)
{
	if (send_probe(ml, addr))
		exit(EXIT_SUCCESS);
	exit(EXIT_FAILURE);
}

static void print_help(const char *prg)
{
	printf("Usage: %s -L /path/to/list\n"
	       "       [-a john=doe.org | -d] [-n num | -p]\n"
	       " -a: Address string that bounces\n"
	       " -h: This help\n"
	       " -L: Full path to list directory\n"
	       " -n: Message number in the archive\n"
	       " -d: Attempt to parse DSN to determine address\n"
	       " -p: Send out a probe\n"
	       " -V: Print version\n", prg);

	exit(EXIT_SUCCESS);
}

int main(int argc, char **argv)
{
	int opt, dsnbounce = 0;
	char *address = NULL, *number = NULL;
	const char *thisaddr = NULL;
	char *a;
	const char *mailname = NULL;
	bool probe = false;
	bounce_t bret;
	struct ml ml;

	log_set_name(argv[0]);
	ml_init(&ml);

	while ((opt = getopt(argc, argv, "hdVL:a:n:m:p")) != -1) {
		switch(opt) {
		case 'L':
			ml.dir = optarg;
			break;
		case 'a':
			thisaddr = optarg;
			break;
		case 'd':
			dsnbounce = 1;
			break;
		case 'm':
			mailname = optarg;
			break;
		case 'n':
			number = optarg;
			break;
		case 'p':
			probe = true;
			break;
		case 'h':
			print_help(argv[0]);
			break;
		case 'V':
			print_version(argv[0]);
			exit(0);
		}
	}

	if(ml.dir == NULL || (thisaddr == NULL && dsnbounce == 0)
				|| (number == NULL && !probe)) {
		errx(EXIT_FAILURE, "You have to specify -L, -a or -d and -n or -p\n"
		    "%s -h for help", argv[0]);
	}
	if (!ml_open(&ml, true))
		exit(EXIT_FAILURE);

	if(dsnbounce) {
		address = dsnparseaddr(mailname);

		/* Delete the mailfile, no need for it anymore */
		if(mailname)
			unlink(mailname);

		if(address == NULL)
			exit(EXIT_SUCCESS);

		a = strrchr(address, '@');
		MY_ASSERT(a);
		*a = '=';
		thisaddr = address;
	}

	if(number != NULL && probe) {
		errx(EXIT_FAILURE, "You can only specify one of -n or -p\n"
		    "%s -h for help", argv[0]);
	}

	if (probe) {
		/* send out a probe */
		do_probe(&ml, thisaddr);
		/* do_probe() will never return */
		exit(EXIT_FAILURE);
	}

	bret = bouncemail(ml.fd, thisaddr, number);
	if (bret == BOUNCE_DONE && mailname != NULL)
		save_lastbouncedmsg(ml.fd, thisaddr, mailname);
	free(address);
	if (bret == BOUNCE_OK && mailname != NULL)
		unlink(mailname);

	return (bret == BOUNCE_FAIL ? EXIT_FAILURE : EXIT_SUCCESS);
}
