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
#include "getlistaddr.h"
#include "getlistdelim.h"
#include "strgen.h"
#include "subscriberfuncs.h"
#include "log_error.h"
#include "mygetline.h"
#include "statctrl.h"
#include "prepstdreply.h"
#include "ctrlvalues.h"
#include "chomp.h"
#include "utils.h"

static void moderate_sub(const char *listdir, const char *listaddr,
		const char *listdelim, const char *subaddr,
		const char *mlmmjsend, enum subtype typesub, enum subreason reasonsub)
{
	int i, fd, status, nosubmodmails = 0;
	text *txt;
	memory_lines_state *mls;
	char *a = NULL, *queuefilename, *from, *listname, *listfqdn, *str;
	char *modfilename, *mods, *to, *replyto, *moderators = NULL;
	char *cookie, *obstruct;
	struct strlist *submods;
	pid_t childpid, pid;

	/* generate the file in moderation/ */
	switch(typesub) {
		default:
		case SUB_NORMAL:
			str = concatstr(4, subaddr, "\n", "SUB_NORMAL", "\n");
			break;
		case SUB_DIGEST:
			str = concatstr(4, subaddr, "\n", "SUB_DIGEST", "\n");
			break;
		case SUB_NOMAIL:
			str = concatstr(4, subaddr, "\n", "SUB_NOMAIL", "\n");
			break;
		case SUB_BOTH:
			str = concatstr(4, subaddr, "\n", "SUB_BOTH", "\n");
			break;
	}
	
	for (;;) {
		cookie = random_str();
		modfilename = concatstr(3, listdir, "/moderation/subscribe",
				cookie);
		fd = open(modfilename, O_RDWR|O_CREAT|O_EXCL, S_IRUSR|S_IWUSR);
		if (fd < 0) {
			if (errno == EEXIST) {
				free(cookie);
				free(modfilename);
				continue;
			}
			log_error(LOG_ARGS, "could not create %s"
					"ignoring request: %s", str);
			exit(EXIT_FAILURE);
		}
		break;
	}

	if(dprintf(fd, "%s", str) < 0) {
		log_error(LOG_ARGS, "could not write to %s"
				"ignoring request: %s", str);
		exit(EXIT_FAILURE);
	}
	
	close(fd);
	
	free(str);

	submods = ctrlvalues(listdir, "submod");
	mods = concatstr(2, listdir, "/control/submod");
	/* check to see if there's adresses in the submod control file */
	for(i = 0; i < submods->count; i++)
		a = strchr(submods->strs[i], '@');

	/* no addresses in submod control file, use owner */
	if(a == NULL) {
		/* free the submods struct from above */
		for(i = 0; i < submods->count; i++)
			free(submods->strs[i]);
		free(submods->strs);
		free(submods);
		submods = ctrlvalues(listdir, "owner");
		free(mods);
		mods = concatstr(2, listdir, "/control/owner");
	}

	/* send mail to moderators about request pending */
	listdelim = getlistdelim(listdir);
	listfqdn = genlistfqdn(listaddr);
	listname = genlistname(listaddr);

	from = concatstr(4, listname, listdelim, "owner@", listfqdn);
	to = concatstr(3, listname, "-moderators@", listfqdn);
	replyto = concatstr(6, listname, listdelim, "permit-", cookie,
			"@", listfqdn);
	obstruct = concatstr(6, listname, listdelim, "obstruct-", cookie,
			"@", listfqdn);
	free(cookie);
	for(i = 0; i < submods->count; i++) {
		printf("%s", submods->strs[i]);
		str = moderators;
		moderators = concatstr(3, moderators, submods->strs[i], "\n");
		free(str);
	}
	mls = init_memory_lines(moderators);
	free(moderators);

	txt = open_text(listdir,
			"gatekeep", "sub",
			subreason_strs[reasonsub], subtype_strs[typesub],
			"submod-moderator");
	MY_ASSERT(txt);
	register_unformatted(txt, "subaddr", subaddr);
	register_unformatted(txt, "moderateaddr", replyto); /* DEPRECATED */
	register_unformatted(txt, "permitaddr", replyto);
	register_unformatted(txt, "obstructaddr", obstruct);
	register_unformatted(txt, "moderators", "%gatekeepers%"); /* DEPRECATED */
	register_formatted(txt, "gatekeepers",
			rewind_memory_lines, get_memory_line, mls);
	queuefilename = prepstdreply(txt, listdir, "$listowner$", to, replyto);
	MY_ASSERT(queuefilename);
	close_text(txt);
	
	/* we might need to exec more than one mlmmj-send */
	
	nosubmodmails = statctrl(listdir,"nosubmodmails");
	
	if (nosubmodmails)
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
		execl(mlmmjsend, mlmmjsend,
				"-a",
				"-l", "4",
				"-L", listdir,
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

	from = concatstr(4, listname, listdelim, "bounces-help@", listfqdn);

	txt = open_text(listdir,
			"wait", "sub",
			subreason_strs[reasonsub], subtype_strs[typesub],
			"submod-requester");
	MY_ASSERT(txt);
	register_unformatted(txt, "subaddr", subaddr);
	register_unformatted(txt, "moderators", "%gatekeepers"); /* DEPRECATED */
	register_formatted(txt, "gatekeepers",
			rewind_memory_lines, get_memory_line, mls);
	queuefilename = prepstdreply(txt, listdir,
			"$listowner$", subaddr, NULL);
	MY_ASSERT(queuefilename);
	close_text(txt);

	finish_memory_lines(mls);
	free(listname);
	free(listfqdn);
	execl(mlmmjsend, mlmmjsend,
				"-l", "1",
				"-L", listdir,
				"-T", subaddr,
				"-F", from,
				"-m", queuefilename, (char *)NULL);
	log_error(LOG_ARGS, "execl() of '%s' failed", mlmmjsend);
	exit(EXIT_FAILURE);
}

void getaddrandtype(const char *listdir, const char *modstr,
		char **addrptr, enum subtype *subtypeptr)
{
	int fd;
	char *readaddr, *readtype, *modfilename;

	if (strncmp(modstr, "subscribe", 9) == 0)
			modstr += 9;

	modfilename = concatstr(3, listdir, "/moderation/subscribe", modstr);

	fd = open(modfilename, O_RDONLY);
	if(fd < 0) {
		log_error(LOG_ARGS, "Could not open %s", modfilename);
		exit(EXIT_FAILURE);
	}

	readaddr = mygetline(fd);
	readtype = mygetline(fd);

	close(fd);

	if(readaddr == NULL || readtype == NULL) {
		log_error(LOG_ARGS, "Could not parse %s", modfilename);
		exit(EXIT_FAILURE);
	}
	
	chomp(readaddr);
	*addrptr = readaddr;

	if(strncmp(readtype, "SUB_NORMAL", 10) == 0) {
		*subtypeptr = SUB_NORMAL;
		goto freedone;
	}

	if(strncmp(readtype, "SUB_DIGEST", 10) == 0) {
		*subtypeptr = SUB_DIGEST;
		goto freedone;
	}

	if(strncmp(readtype, "SUB_NOMAIL", 10) == 0) {
		*subtypeptr = SUB_NOMAIL;
		goto freedone;
	}

	if(strncmp(readtype, "SUB_BOTH", 8) == 0) {
		*subtypeptr = SUB_BOTH;
		goto freedone;
	}

	log_error(LOG_ARGS, "Type %s not valid in %s", readtype,
			modfilename);

freedone:
	free(readtype);
	unlink(modfilename);
	free(modfilename);
}

void confirm_sub(const char *listdir, const char *listaddr,
		const char *listdelim, const char *subaddr,
		const char *mlmmjsend, enum subtype typesub, enum subreason reasonsub)
{
	text *txt;
	char *queuefilename, *fromaddr, *listname, *listfqdn, *listtext;

	listname = genlistname(listaddr);
	listfqdn = genlistfqdn(listaddr);

	fromaddr = concatstr(4, listname, listdelim, "bounces-help@", listfqdn);

	free(listname);
	free(listfqdn);

	switch(typesub) {
		default:
		case SUB_NORMAL:
			listtext = xstrdup("sub-ok");
			break;
		case SUB_DIGEST:
			listtext = xstrdup("sub-ok-digest");
			break;
		case SUB_NOMAIL:
			listtext = xstrdup("sub-ok-nomail");
			break;
		case SUB_BOTH:
			/* No legacy list text as feature didn't exist. */
			listtext = xstrdup("sub-ok");
			break;
	}

	txt = open_text(listdir, "finish", "sub",
			subreason_strs[reasonsub], subtype_strs[typesub],
			listtext);
	free(listtext);
	MY_ASSERT(txt);
	register_unformatted(txt, "subaddr", subaddr);
	queuefilename = prepstdreply(txt, listdir,
			"$helpaddr$", subaddr, NULL);
	MY_ASSERT(queuefilename);
	close_text(txt);

	exec_or_die(mlmmjsend, "-l", "1", "-L", listdir, "-T", subaddr, "-F",
	    fromaddr, "-m", queuefilename, NULL);
}

void notify_sub(const char *listdir, const char *listaddr,
		const char *listdelim, const char *subaddr,
		const char *mlmmjsend, enum subtype typesub, enum subreason reasonsub)
{
	char *listfqdn, *listname, *fromaddr, *tostr;
	text *txt;
	char *queuefilename = NULL, *listtext = NULL;

	listname = genlistname(listaddr);
	listfqdn = genlistfqdn(listaddr);

	fromaddr = concatstr(4, listname, listdelim, "bounces-help@", listfqdn);
	tostr = concatstr(4, listname, listdelim, "owner@", listfqdn);
	
	free(listname);
	free(listfqdn);

	switch(typesub) {
		default:
		case SUB_NORMAL:
			listtext = xstrdup("notifysub");
			break;
		case SUB_DIGEST:
			listtext = xstrdup("notifysub-digest");
			break;
		case SUB_NOMAIL:
			listtext = xstrdup("notifysub-nomail");
			break;
		case SUB_BOTH:
			/* No legacy list text as feature didn't exist. */
			listtext = xstrdup("notifysub");
			break;
	}

	txt = open_text(listdir, "notify", "sub",
			subreason_strs[reasonsub], subtype_strs[typesub],
			listtext);
	free(listtext);
	MY_ASSERT(txt);
	register_unformatted(txt, "subaddr", subaddr);
	register_unformatted(txt, "newsub", subaddr); /* DEPRECATED */
	queuefilename = prepstdreply(txt, listdir,
			"$listowner$", "$listowner$", NULL);
	MY_ASSERT(queuefilename);
	close_text(txt);

	exec_or_die(mlmmjsend, "-l", "1", "-L", listdir, "-T", tostr,
	    "-F", fromaddr, "-m", queuefilename, NULL);
}

void generate_subconfirm(const char *listdir, const char *listaddr,
			 const char *listdelim, const char *subaddr,
			 const char *mlmmjsend, enum subtype typesub, enum subreason reasonsub)
{
	int subconffd;
	char *confirmaddr, *listname, *listfqdn, *confirmfilename = NULL;
	text *txt;
	char *listtext, *queuefilename = NULL, *fromaddr;
	char *randomstr = NULL, *tmpstr;

	listname = genlistname(listaddr);
	listfqdn = genlistfqdn(listaddr);

        do {
		free(confirmfilename);
		free(randomstr);
		randomstr = random_str();
                confirmfilename = concatstr(3, listdir, "/subconf/",
					    randomstr);

                subconffd = open(confirmfilename, O_RDWR|O_CREAT|O_EXCL,
						  S_IRUSR|S_IWUSR);

        } while ((subconffd < 0) && (errno == EEXIST));

	if(subconffd < 0) {
		log_error(LOG_ARGS, "Could not open '%s'", confirmfilename);
		free(confirmfilename);
                free(randomstr);
		exit(EXIT_FAILURE);
	}

	free(confirmfilename);

	if(dprintf(subconffd, "%s", subaddr) < 0) {
		log_error(LOG_ARGS, "Could not write to subconffd");
		free(confirmfilename);
                free(randomstr);
		exit(EXIT_FAILURE);
	}

	close(subconffd);

	fromaddr = concatstr(6, listname, listdelim, "bounces-confsub-",
				randomstr, "@", listfqdn);
	
	switch(typesub) {
		default:
		case SUB_NORMAL:
			listtext = xstrdup("sub-confirm");
			tmpstr = xstrdup("confsub-");
			break;
		case SUB_DIGEST:
			listtext = xstrdup("sub-confirm-digest");
			tmpstr = xstrdup("confsub-digest-");
			break;
		case SUB_NOMAIL:
			listtext = xstrdup("sub-confirm-nomail");
			tmpstr = xstrdup("confsub-nomail-");
			break;
		case SUB_BOTH:
			/* No legacy list text as feature didn't exist. */
			listtext = xstrdup("sub-confirm");
			tmpstr = xstrdup("confsub-both-");
			break;
	}

	confirmaddr = concatstr(6, listname, listdelim, tmpstr, randomstr, "@",
				listfqdn);

	free(randomstr);
	free(tmpstr);

	txt = open_text(listdir, "confirm", "sub",
			subreason_strs[reasonsub], subtype_strs[typesub],
			listtext);
	free(listtext);
	MY_ASSERT(txt);
	register_unformatted(txt, "subaddr", subaddr);
	register_unformatted(txt, "confaddr", confirmaddr); /* DEPRECATED */
	register_unformatted(txt, "confirmaddr", confirmaddr);
	queuefilename = prepstdreply(txt, listdir,
			"$helpaddr$", subaddr, confirmaddr);
	MY_ASSERT(queuefilename);
	close_text(txt);

	free(listname);
	free(listfqdn);

	exec_or_die(mlmmjsend, "-l", "1", "-L", listdir, "-T", subaddr, "-F",
	    fromaddr, "-m", queuefilename, NULL);
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

void generate_subscribed(const char *listdir, const char *subaddr,
		const char *mlmmjsend, enum subtype typesub)
{
	text *txt;
	char *queuefilename, *fromaddr, *listname, *listfqdn, *listaddr;
	char *listdelim = getlistdelim(listdir);

	listaddr = getlistaddr(listdir);
	listname = genlistname(listaddr);
	listfqdn = genlistfqdn(listaddr);

	fromaddr = concatstr(4, listname, listdelim, "bounces-help@", listfqdn);
	free(listdelim);

	txt = open_text(listdir,
			"deny", "sub", "subbed", subtype_strs[typesub],
			"sub-subscribed");
	MY_ASSERT(txt);
	register_unformatted(txt, "subaddr", subaddr);
	queuefilename = prepstdreply(txt, listdir,
			"$helpaddr$", subaddr, NULL);
	MY_ASSERT(queuefilename);
	close_text(txt);

	free(listaddr);
	free(listname);
	free(listfqdn);

	exec_or_die(mlmmjsend, "-l", "1", "-L", listdir, "-T", subaddr, "-F",
	    fromaddr, "-m", queuefilename, NULL);
}

static void subscribe_type(char *listdir, char *address, enum subtype typesub) {
	char *subfilename = NULL;
	char chstr[2], *subdir;
	char *subddirname = NULL;
	int groupwritable = 0, subfilefd;
	struct stat st;

	switch(typesub) {
		default:
		case SUB_NORMAL:
			subdir = "/subscribers.d/";
			break;
		case SUB_DIGEST:
			subdir = "/digesters.d/";
			break;
		case SUB_NOMAIL:
			subdir = "/nomailsubs.d/";
			break;
	}

	subddirname = concatstr(2, listdir, subdir);
	if (stat(subddirname, &st) == 0) {
		if(st.st_mode & S_IWGRP) {
			groupwritable = S_IRGRP|S_IWGRP;
			umask(S_IWOTH);
			setgid(st.st_gid);
		}
	}

	chstr[0] = address[0];
	chstr[1] = '\0';
	
	subfilename = concatstr(3, listdir, subdir, chstr);

	subfilefd = open(subfilename, O_RDWR|O_CREAT|O_APPEND,
				S_IRUSR|S_IWUSR|groupwritable);
	if(subfilefd == -1 && !lock(subfilefd, true)) {
		log_error(LOG_ARGS, "Could not open '%s'", subfilename);
		exit(EXIT_FAILURE);
	}

	dprintf(subfilefd, "%s\n", address);
	close(subfilefd);
}

int main(int argc, char **argv)
{
	char *listaddr, *listdelim, *listdir = NULL;
	char *mlmmjsend, *mlmmjunsub, *bindir;
	char *address = NULL, *modstr = NULL;
	const char *flag = NULL;
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

	CHECKFULLPATH(argv[0]);

	log_set_name(argv[0]);

	bindir = mydirname(argv[0]);
	mlmmjsend = concatstr(2, bindir, "/mlmmj-send");
	mlmmjunsub = concatstr(2, bindir, "/mlmmj-unsub");
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
			listdir = optarg;
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

	if(listdir == NULL) {
		errx(EXIT_FAILURE, "You have to specify -L\n"
		    "%s -h for help", argv[0]);
	}
	
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
		getaddrandtype(listdir, modstr, &address, &typesub);
		reasonsub = SUB_PERMIT;
	}

	/* Make the address lowercase */
	address = lowercase(address);

	/* get the list address */
	listaddr = getlistaddr(listdir);
	if(strncasecmp(listaddr, address, strlen(listaddr)) == 0)
		errx(EXIT_FAILURE, "Cannot subscribe the list address to the list");

	if(changeuid) {
		uid = getuid();
		if(!uid && stat(listdir, &st) == 0) {
			printf("Changing to uid %d, owner of %s.\n",
					(int)st.st_uid, listdir);
			if(setuid(st.st_uid) < 0) {
				perror("setuid");
				fprintf(stderr, "Continuing as uid %d\n",
				    (int)uid);
			}
		}
	}

	subbed = is_subbed(listdir, address, 1);

	if(subbed == typesub) {
		if(!nogensubscribed)
			generate_subscribed(listdir, address, mlmmjsend,
					typesub);
		return EXIT_SUCCESS;
	} else if(subbed != SUB_NONE) {
		reasonsub = SUB_SWITCH;
		/* If we want to subscribe to both, we can just subscribe the
		 * missing version, so don't unsub. */
		if (!(typesub == SUB_BOTH &&
				subbed != SUB_NOMAIL)) {
			if (subbed == SUB_BOTH) {
				if (typesub == SUB_NORMAL) flag = "-d";
				if (typesub == SUB_DIGEST) flag = "-N";
			}
			if (exec_and_wait(mlmmjunsub, "-L", listdir, "-q", "-a",
			    address, flag, NULL) != 0)
				log_error(LOG_ARGS, "not unsubscribed from "
				    "current version");
		}
	}

	listdelim = getlistdelim(listdir);

	if(subbed == SUB_NONE && subconfirm)
			generate_subconfirm(listdir, listaddr, listdelim,
			address, mlmmjsend, typesub, reasonsub);

	if(modstr == NULL && subbed == SUB_NONE && !force &&
			statctrl(listdir, "submod")) {
		moderate_sub(listdir, listaddr, listdelim,
				address, mlmmjsend, typesub, reasonsub);
	}

	if (typesub == SUB_BOTH) {
		if (subbed != SUB_NORMAL) {
			subscribe_type(listdir, address, SUB_NORMAL);
		}
		if (subbed != SUB_DIGEST) {
			subscribe_type(listdir, address, SUB_DIGEST);
		}
	} else if (!(subbed == SUB_BOTH && typesub != SUB_NOMAIL)) {
		subscribe_type(listdir, address, typesub);
	}

	if(confirmsub) {
		childpid = fork();

		if(childpid < 0) {
			log_error(LOG_ARGS, "Could not fork; owner not notified");
			confirm_sub(listdir, listaddr, listdelim, address,
					mlmmjsend, typesub, reasonsub);
		}
		
		if(childpid > 0) {
			do /* Parent waits for the child */
				pid = waitpid(childpid, &status, 0);
			while(pid == -1 && errno == EINTR);
		}

		/* child confirms subscription */
		if(childpid == 0)
			confirm_sub(listdir, listaddr, listdelim, address,
					mlmmjsend, typesub, reasonsub);
	}

	notifysub = !quiet && reasonsub != SUB_SWITCH &&
			statctrl(listdir, "notifysub");

	/* Notify list owner about subscription */
	if (notifysub)
		notify_sub(listdir, listaddr, listdelim, address, mlmmjsend,
				typesub, reasonsub);

	free(address);
	free(listaddr);
	free(listdelim);

	return EXIT_SUCCESS;
}
