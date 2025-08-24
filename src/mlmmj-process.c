/*
 * Copyright (C) 2003, 2003, 2004 Mads Martin Joergensen <mmj at mmj.dk>
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
#include <libgen.h>
#include <regex.h>
#include <err.h>

#include "xmalloc.h"
#include "mlmmj.h"
#include "wrappers.h"
#include "find_email_adr.h"
#include "incindexfile.h"
#include "listcontrol.h"
#include "strgen.h"
#include "do_all_the_voodoo_here.h"
#include "log_error.h"
#include "statctrl.h"
#include "ctrlvalue.h"
#include "ctrlvalues.h"
#include "prepstdreply.h"
#include "subscriberfuncs.h"
#include "log_oper.h"
#include "unistr.h"
#include "chomp.h"
#include "mlmmj-process.h"
#include "utils.h"
#include "xstring.h"
#include "send_help.h"
#include "send_mail.h"

enum modreason {
	MODNONSUBPOSTS,
	MODNONMODPOSTS,
	ACCESS,
	MODERATED
};


static char *modreason_strs[] = {
	"modnonsubposts",
	"modnonmodposts",
	"access",
	"moderated"
};


static bool is_moderator(int listfd, const char *address,
		char **moderators) {
	char *line = NULL;
	size_t linecap = 0;
	int moderatorsfd, foundaddr = 0;
	xstring *str = NULL;
	FILE *fp;

	if((moderatorsfd = openat(listfd, "control/moderators", O_RDONLY)) < 0) {
		log_error(LOG_ARGS, "Could not open 'control/moderators'");
		exit(EXIT_FAILURE);
	}
	fp = fdopen(moderatorsfd, "r");

	while (getline(&line, &linecap, fp) > 0) {
		chomp(line);
		if(address && strcasecmp(line, address) == 0) {
			foundaddr = true;
			if (moderators == NULL)
				goto out;
		}
		if (moderators) {
			if (str == NULL)
				str = xstring_new();
			fprintf(str->fp, "%s\n", line);
		}
	}

	if (moderators)
		*moderators = xstring_get(str);
out:
	free(line);
	fclose(fp);
	return foundaddr;
}


static void newmoderated(struct ml *ml, const char *mailfilename,
		  const char *mlmmjsend, const char *efromsender,
		  const char *subject, const char *posteraddr,
		  enum modreason modreason)
{
	char *from, *moderators = NULL;
	char *replyto, *reject, *to;
	text *txt;
	memory_lines_state *mls;
	char *queuefilename = NULL;
	const char *efromismod = NULL;
	const char *mailbasename = mybasename(mailfilename);
	int notifymod = 0;
	int modemailsender = 0;
	struct mail mail;
#if 0
	printf("mailfilename = [%s], mailbasename = [%s]\n", mailfilename,
			                                     mailbasename);
#endif

	if(statctrl(ml->ctrlfd, "ifmodsendonlymodmoderate"))
		efromismod = efromsender;

	if(!is_moderator(ml->fd, efromismod, &moderators))
		efromismod = NULL;

	if(efromismod) mls = init_memory_lines(efromismod);
	else mls = init_memory_lines(moderators);

	free(moderators);

	gen_addr_cookie(replyto, ml, "release-", mailbasename);
	gen_addr_cookie(reject, ml, "reject-", mailbasename);

	gen_addr(from, ml, "owner");

	modemailsender = statctrl(ml->ctrlfd, "modemailsender");

	if(modemailsender) to = xstrdup(posteraddr);
	else xasprintf(&to, "%s-moderators@%s", ml->name, ml->fqdn);

	txt = open_text(ml->fd, "moderate", "post",
			modreason_strs[modreason], NULL, "moderation");
	MY_ASSERT(txt);
	register_default_unformatted(txt, ml);
	register_unformatted(txt, "subject", subject);
	register_unformatted(txt, "posteraddr", posteraddr);
	register_unformatted(txt, "releaseaddr", replyto);
	register_unformatted(txt, "rejectaddr", reject);
	register_formatted(txt, "moderators",
			rewind_memory_lines, get_memory_line, mls);
	register_originalmail(txt, mailfilename);
	queuefilename = prepstdreply(txt, ml, "$listowner$", to, replyto);
	MY_ASSERT(queuefilename);
	close_text(txt);

	/* we might need to exec more than one mlmmj-send */

	notifymod = !efromismod && statctrl(ml->ctrlfd,"notifymod");

	if (notifymod) {
		char *qfname;

		/* send mail to poster that the list is moderated */
		txt = open_text(ml->fd, "wait", "post",
				modreason_strs[modreason], NULL, "moderation-poster");
		MY_ASSERT(txt);
		register_default_unformatted(txt, ml);
		register_unformatted(txt, "subject", subject);
		register_unformatted(txt, "posteraddr", posteraddr);
		register_formatted(txt, "moderators",
				rewind_memory_lines, get_memory_line, mls);
		register_originalmail(txt, mailfilename);
		qfname = prepstdreply(txt, ml, "$listowner$", efromsender, NULL);
		MY_ASSERT(qfname);
		close_text(txt);

		finish_memory_lines(mls);

		memset(&mail, 0, sizeof(mail));
		mail.from = from;
		mail.to = efromsender;
		mail.fp = fopen(qfname, "r");
		if (send_single_mail(&mail, ml, false))
			unlink(qfname);
		fclose(mail.fp);
	}

	if (efromismod) {
		memset(&mail, 0, sizeof(mail));
		mail.from = from;
		mail.to = efromsender;
		mail.fp = fopen(queuefilename, "r");
		if (send_single_mail(&mail, ml, false))
			unlink(queuefilename);
		fclose(mail.fp);
		exit(EXIT_SUCCESS);
	}

	if(modemailsender) {
		memset(&mail, 0, sizeof(mail));
		mail.from = from;
		mail.to = to;
		mail.fp = fopen(queuefilename, "r");
		if (send_single_mail(&mail, ml, false))
			unlink(queuefilename);
		fclose(mail.fp);
		exit(EXIT_SUCCESS);
	}

	exec_or_die(mlmmjsend, "-l", "2", "-L", ml->dir, "-F", from,
	    "-m", queuefilename, NULL);
}


static void print_help(const char *prg)
{
	printf("Usage: %s -L /path/to/list\n"
	       "       -m /path/to/mail [-h] [-P] [-V]\n"
	       " -h: This help\n"
	       " -L: Full path to list directory\n"
	       " -m: Full path to mail file\n"
	       " -P: Don't execute mlmmj-send\n"
	       " -V: Print version\n", prg);

	exit(EXIT_SUCCESS);
}

int main(int argc, char **argv)
{
	int i, opt, moderated = 0, send = 0;
	enum modreason modreason;
	int hdrfd, footfd, rawmailfd, donemailfd, omitfd;
	bool addr_in_to_or_cc, notmetoo, replyto;
	bool findaddress = false, intocc = false, noprocess = false;
	int maxmailsize = 0;
	bool subonlypost, modonlypost, modnonsubposts, foundaddr = false;
	char *mailfile = NULL, *donemailname = NULL;
	char *randomstr = NULL, *mqueuename, *omitfilename;
	char *mlmmjsend;
	char *bindir, *subjectprefix, *discardname;
	text *txt;
	char *queuefilename, *recipextra = NULL, *owner = NULL;
	char *subject = NULL, *posteraddr = NULL;
	char *envstr;
	const char *efrom = "";
	struct stat st;
	strlist fromemails = tll_init();
	strlist originalfromemails = tll_init();
	strlist toemails = tll_init();
	strlist ccemails = tll_init();
	strlist rpemails = tll_init();
	strlist dtemails = tll_init();
	strlist *testfrom = NULL;
	strlist *access_rules = NULL;
	strlist *list_rules = NULL;
	strlist *delheaders = NULL;
	strlist allheaders = tll_init();
	strlist *listaddrs = NULL;
	struct mailhdr readhdrs[] = {
		{ "From:", 0, NULL },
		{ "To:", 0, NULL },
		{ "Cc:", 0, NULL },
		{ "Return-Path:", 0, NULL },
		{ "Delivered-To:", 0, NULL },
		{ "Subject:", 0, NULL },
		{ "X-Original-From:", 0, NULL },
		{ NULL, 0, NULL }
	};
	struct ml ml;

	CHECKFULLPATH(argv[0]);
	ml_init(&ml);

	log_set_name(argv[0]);

	bindir = mydirname(argv[0]);
	xasprintf(&mlmmjsend, "%s/mlmmj-send", bindir);
	free(bindir);

	while ((opt = getopt(argc, argv, "hVPm:L:")) != -1) {
		switch(opt) {
		case 'L':
			ml.dir = optarg;
			break;
		case 'm':
			mailfile = optarg;
			break;
		case 'h':
			print_help(argv[0]);
			break;
		case 'P':
			noprocess = true;
			break;
		case 'V':
			print_version(argv[0]);
			exit(EXIT_SUCCESS);
		}
	}

	if(ml.dir == NULL || mailfile == NULL) {
		errx(EXIT_FAILURE, "You have to specify -L and -m\n"
		    "%s -h for help", argv[0]);
	}

	if (!ml_open(&ml, true))
		exit(EXIT_FAILURE);

        do {
                free(donemailname);
                free(randomstr);
                randomstr = random_str();
		xasprintf(&donemailname, "%s/queue/%s", ml.dir, randomstr);

                donemailfd = open(donemailname, O_RDWR|O_CREAT|O_EXCL,
						S_IRUSR|S_IWUSR);

        } while ((donemailfd < 0) && (errno == EEXIST));

	if(donemailfd < 0) {
		log_error(LOG_ARGS, "could not create %s", donemailname);
		free(donemailname);
		exit(EXIT_FAILURE);
	}
#if 0
	log_error(LOG_ARGS, "donemailname = [%s]\n", donemailname);
#endif
	if((rawmailfd = open(mailfile, O_RDONLY)) < 0) {
		unlink(donemailname);
		free(donemailname);
		log_error(LOG_ARGS, "could not open() input mail file");
		exit(EXIT_FAILURE);
	}

	/* hdrfd is checked in do_all_the_voodoo_here(), because the
	 * customheaders file might not exist */
	hdrfd = openat(ml.ctrlfd, "customheaders", O_RDONLY);

	/* footfd is checked in do_all_the_voodoo_here(), see above */
	footfd = openat(ml.ctrlfd, "footer", O_RDONLY);

	delheaders = ctrlvalues(ml.ctrlfd, "delheaders");
	if(delheaders == NULL)
		delheaders = xcalloc(1, sizeof(*delheaders));

	tll_push_back(*delheaders, xstrdup("From "));
	tll_push_back(*delheaders, xstrdup("Return-Path:"));

	subjectprefix = ctrlvalue(ml.ctrlfd, "prefix");

	replyto = statctrl(ml.ctrlfd, "replyto");

	if(do_all_the_voodoo_here(rawmailfd, donemailfd, hdrfd, footfd,
				delheaders, readhdrs,
				&allheaders, subjectprefix, replyto) < 0) {
		log_error(LOG_ARGS, "Error in do_all_the_voodoo_here");
		exit(EXIT_FAILURE);
	}

	tll_free_and_free(*delheaders, free);

	close(rawmailfd);
	close(donemailfd);

	if(hdrfd >= 0)
		close(hdrfd);
	if(footfd >= 0)
		close(footfd);

	/* To: addresses */
	for(i = 0; i < readhdrs[1].valuecount; i++) {
		find_email_adr(readhdrs[1].values[i], &toemails);
	}

	/* Cc: addresses */
	for(i = 0; i < readhdrs[2].valuecount; i++) {
		find_email_adr(readhdrs[2].values[i], &ccemails);
	}

	/* Delivered-To: addresses */
	for(i = 0; i < readhdrs[4].valuecount; i++) {
		find_email_adr(readhdrs[4].values[i], &dtemails);
	}

	recipextra = get_recipextra_from_env(ml.ctrlfd);
	if (recipextra == NULL) {
		findaddress = true;
	}
	addr_in_to_or_cc = !statctrl(ml.ctrlfd, "tocc");

	if(addr_in_to_or_cc || findaddress) {
		listaddrs = ctrlvalues(ml.ctrlfd, "listaddress");
		if (listaddrs == NULL) {
			log_error(LOG_ARGS, "list address is not defined");
			err(EXIT_FAILURE, "list address is not defined");
		}
		findaddress = !find_email(&dtemails, listaddrs, ml.delim,
		    recipextra != NULL ? NULL : &recipextra);
	}
	if(addr_in_to_or_cc || findaddress) {
		intocc = find_email(&toemails, listaddrs, ml.delim,
		    recipextra != NULL ? NULL : &recipextra);
		if (!intocc)
			intocc = find_email(&ccemails, listaddrs, ml.delim,
			    recipextra != NULL ? NULL : &recipextra);
	}
	if (listaddrs)
		tll_free_and_free(*listaddrs, free);
	free(listaddrs);

	if (recipextra && *recipextra == '\0') {
		free(recipextra);
		recipextra = NULL;
	}

	/* From: addresses */
	for(i = 0; i < readhdrs[0].valuecount; i++) {
		find_email_adr(readhdrs[0].values[i], &fromemails);
	}
	/* X-Original-From: addresses */
	for(i = 0; i < readhdrs[6].valuecount; i++) {
		find_email_adr(readhdrs[6].values[i], &originalfromemails);
	}

	/* discard malformed mail with invalid From: unless it's a bounce */
	if(tll_length(fromemails) != 1 &&
			(recipextra == NULL ||
			strncmp(recipextra, "bounces", 7) != 0)) {
		tll_foreach(fromemails, it)
			printf("fromemails.emaillist[] = %s\n",
					it->item);
		xasprintf(&discardname, "%s/queue/discarded/%s", ml.dir, randomstr);
		log_error(LOG_ARGS, "Discarding %s due to invalid From:",
				mailfile);
		tll_foreach(fromemails, it)
			log_error(LOG_ARGS, "fromemails.emaillist[] = %s\n",
					it->item);
		rename(mailfile, discardname);
		unlink(donemailname);
		free(donemailname);
		free(discardname);
		free(randomstr);
		/* TODO: free emailstructs */
		exit(EXIT_SUCCESS);
	}
	/* The only time posteraddr will remain unset is when the mail is a
	 * bounce, so the mail will be processed by listcontrol() and the
	 * program will terminate before posteraddr is used. */
	if (tll_length(fromemails) > 0)
			posteraddr = tll_front(fromemails);

	/* Return-Path: addresses */
	for(i = 0; i < readhdrs[3].valuecount; i++) {
		find_email_adr(readhdrs[3].values[i], &rpemails);
	}

	/* envelope from */
	if((envstr = getenv("SENDER")) != NULL) {
		/* qmail, postfix, exim */
		efrom = envstr;
	} else if(tll_length(rpemails) >= 1) {
		/* the (first) Return-Path: header */
		efrom = tll_front(rpemails);
	}

	/* Subject: */
	if (readhdrs[5].valuecount)
			subject = unistr_header_to_utf8(readhdrs[5].values[0]);
	if (!subject) subject = xstrdup("");

	if(recipextra) {
		int ownfd = openat(ml.ctrlfd, "owner", O_RDONLY);
		xasprintf(&owner, "%d", ownfd);
		if(strcmp(recipextra, "owner") == 0) {
			/* Why is this here, and not in listcontrol() ?
			 * -- mortenp 20060409 */
			/* strip envelope from before resending */
			tll_push_back(*delheaders, xstrdup("From "));
			tll_push_back(*delheaders, xstrdup("Return-Path:"));

			if((rawmailfd = open(mailfile, O_RDONLY)) < 0) {
				log_error(LOG_ARGS, "could not open() "
						    "input mail file");
				exit(EXIT_FAILURE);
			}
			if((donemailfd = open(donemailname,
						O_WRONLY|O_TRUNC)) < 0) {
				log_error(LOG_ARGS, "could not open() "
						    "output mail file");
				exit(EXIT_FAILURE);
			}
			if(do_all_the_voodoo_here(rawmailfd, donemailfd, -1,
					-1, delheaders,
					NULL, &allheaders, NULL, 0) < 0) {
				log_error(LOG_ARGS, "do_all_the_voodoo_here");
				exit(EXIT_FAILURE);
			}
			tll_free_and_free(*delheaders, free);
			close(rawmailfd);
			close(donemailfd);
			unlink(mailfile);
			log_oper(ml.fd, OPLOGFNAME, "mlmmj-process: sending"
					" mail from %s to owner",
					efrom);
			exec_or_die(mlmmjsend, "-l", "4", "-L", ml.dir, "-F",
			    efrom, "-s", owner, "-a", "-m", donemailname, NULL);
		}
		unlink(mailfile);
		if (tll_length(originalfromemails) > 0)
			testfrom = &originalfromemails;
		else
			testfrom = &fromemails;
		listcontrol(testfrom, &ml, recipextra,
			    mlmmjsend, donemailname);

		return EXIT_SUCCESS;
	}

	/* checking incoming mail's size */
	errno = 0;
	maxmailsize = ctrlsizet(ml.ctrlfd, "maxmailsize", 0);
	if(errno != ENOENT) {
		if(stat(donemailname, &st) < 0) {
			log_error(LOG_ARGS, "stat(%s,..) failed", donemailname);
			exit(EXIT_FAILURE);
		}

		if(st.st_size > maxmailsize) {
			char *maxmailsizestr;
			if (statctrl(ml.ctrlfd, "nomaxmailsizedenymails")) {
				errno = 0;
				log_error(LOG_ARGS, "Discarding %s due to"
						" size limit (%d bytes too big)",
						donemailname, (st.st_size - maxmailsize));
				unlink(donemailname);
				unlink(mailfile);
				free(donemailname);
				exit(EXIT_SUCCESS);
			}

			txt = open_text(ml.fd, "deny", "post",
					"maxmailsize", NULL, "maxmailsize");
			MY_ASSERT(txt);
			register_default_unformatted(txt, &ml);
			register_unformatted(txt, "subject", subject);
			register_unformatted(txt, "posteraddr", posteraddr);
			xasprintf(&maxmailsizestr, "%d", maxmailsize);
			register_unformatted(txt, "maxmailsize", maxmailsizestr);
			register_originalmail(txt, donemailname);
			queuefilename = prepstdreply(txt, &ml, "$listowner$",
			    posteraddr, NULL);
			MY_ASSERT(queuefilename);
			close_text(txt);
			unlink(donemailname);
			unlink(mailfile);
			free(donemailname);
			free(maxmailsizestr);
			send_help(&ml, queuefilename, posteraddr);
		}
	}

	free(delheaders);

	if(*efrom == '\0') { /* don't send mails with <> in From
					     to the list */
		xasprintf(&discardname, "%s/queue/discarded/%s", ml.dir, randomstr);
		errno = 0;
		log_error(LOG_ARGS, "Discarding %s due to missing envelope"
				" from address", mailfile);
		rename(mailfile, discardname);
		unlink(donemailname);
		free(donemailname);
		free(discardname);
		free(randomstr);
		/* TODO: free emailstructs */
		exit(EXIT_FAILURE);
	}

	unlink(mailfile);

	if(addr_in_to_or_cc && !intocc) {
		/* Don't send a mail about denial to the list, but silently
		 * discard and exit. Also don't in case of it being turned off
		 */
		if ((strcasecmp(ml.addr, posteraddr) == 0) ||
				statctrl(ml.ctrlfd, "notoccdenymails")) {
			log_error(LOG_ARGS, "Discarding %s because list"
					" address was not in To: or Cc:,"
					" and From: was the list or"
					" notoccdenymails was set",
					mailfile);
			unlink(donemailname);
			free(donemailname);
			exit(EXIT_SUCCESS);
		}
		txt = open_text(ml.fd, "deny", "post",
				"tocc", NULL, "notintocc");
		MY_ASSERT(txt);
		register_default_unformatted(txt, &ml);
		register_unformatted(txt, "subject", subject);
		register_unformatted(txt, "posteraddr", posteraddr);
		register_originalmail(txt, donemailname);
		queuefilename = prepstdreply(txt, &ml,
		    "$listowner$", posteraddr, NULL);
		MY_ASSERT(queuefilename)
		close_text(txt);
		unlink(donemailname);
		free(donemailname);
		send_help(&ml, queuefilename, posteraddr);
	}

	access_rules = ctrlvalues(ml.ctrlfd, "access");
	if (access_rules != NULL) {
		actions_t accret;
		char *error = NULL;
		char *qualifier = NULL;
		/* Don't send a mail about denial to the list, but silently
		 * discard and exit. Also do this in case it's turned off */
		accret = do_access(access_rules, &allheaders,
					posteraddr, &error, &qualifier);
		if (error != NULL) {
			log_oper(ml.fd, OPLOGFNAME, "%s", error);
			free(error);
		}
		if (accret == ACT_DENY) {
			if ((strcasecmp(ml.addr, posteraddr) == 0) ||
				    statctrl(ml.ctrlfd, "noaccessdenymails")) {
				log_error(LOG_ARGS, "Discarding %s because"
					" it was denied by an access"
					" rule, and From: was the list"
					" address or noaccessdenymails"
					" was set",
					mailfile);
				unlink(donemailname);
				free(donemailname);
				exit(EXIT_SUCCESS);
			}
			txt = open_text(ml.fd, "deny", "post",
					"access", qualifier, "access");
			MY_ASSERT(txt);
			register_default_unformatted(txt, &ml);
			register_unformatted(txt, "subject", subject);
			register_unformatted(txt, "posteraddr", posteraddr);
			register_originalmail(txt, donemailname);
			queuefilename = prepstdreply(txt, &ml,
					"$listowner$", posteraddr, NULL);
			MY_ASSERT(queuefilename)
			close_text(txt);
			unlink(donemailname);
			free(donemailname);
			free(randomstr);
			send_help(&ml, queuefilename, posteraddr);
		} else if (accret == ACT_MODERATE) {
			moderated = 1;
			modreason = ACCESS;
		} else if (accret == ACT_DISCARD) {
			xasprintf(&discardname, "%s/queue/discarded/%s", ml.dir,
			    randomstr);
			free(randomstr);
                	if(rename(donemailname, discardname) < 0) {
				log_error(LOG_ARGS, "could not rename(%s,%s)",
					    donemailname, discardname);
				free(donemailname);
				free(discardname);
				exit(EXIT_FAILURE);
			}
			free(donemailname);
			free(discardname);
                	exit(EXIT_SUCCESS);
		} else if (accret == ACT_SEND) {
			send = 1;
		} else if (accret == ACT_ALLOW) {
			/* continue processing as normal */
		}
		free(qualifier);
	}

	list_rules = ctrlvalues(ml.ctrlfd, "send");
	if (list_rules != NULL) {
		tll_foreach(*list_rules, lr) {
			if (strcasecmp(posteraddr, lr->item) == 0) {
				send = 1;
				break;
			}
		}
	}

	subonlypost = statctrl(ml.ctrlfd, "subonlypost");
	modonlypost = statctrl(ml.ctrlfd, "modonlypost");
	modnonsubposts = statctrl(ml.ctrlfd, "modnonsubposts");
	/* modnonsubposts implies subonlypost if modonlypost is not set */
	if (modnonsubposts && !modonlypost) subonlypost = 1;

	if(!send && (subonlypost || modonlypost || modnonsubposts)) {
		/* Don't send a mail about denial to the list, but silently
		 * discard and exit. */
		char *testaddr = posteraddr;
		if (tll_length(originalfromemails) > 0)
			testaddr = tll_front(originalfromemails);
		if (strcasecmp(ml.addr, testaddr) == 0) {
			log_error(LOG_ARGS, "Discarding %s because"
					" there are sender restrictions but"
					" From: was the list address",
					mailfile);
			unlink(donemailname);
			free(donemailname);
			exit(EXIT_SUCCESS);
		}
		if(subonlypost) {
			foundaddr = (is_subbed(ml.fd, testaddr, 0) !=
					SUB_NONE);
		} else if (modonlypost) {
			foundaddr = is_moderator(ml.fd, testaddr, NULL);
		}
		if(!foundaddr) {
			if(modnonsubposts) {
			    moderated = 1;
			    if (subonlypost)
				modreason = MODNONSUBPOSTS;
			    else if (modonlypost)
				modreason = MODNONMODPOSTS;
			} else {
			    if((subonlypost &&
				    statctrl(ml.ctrlfd, "nosubonlydenymails")) ||
				    (modonlypost &&
				    statctrl(ml.ctrlfd, "nomodonlydenymails"))) {
				log_error(LOG_ARGS, "Discarding %s because"
					" no{sub|mod}onlydenymails was set",
					mailfile);
				unlink(donemailname);
				free(donemailname);
				exit(EXIT_SUCCESS);
			    }
			    if (subonlypost) {
				txt = open_text(ml.fd, "deny", "post",
					"subonlypost", NULL, "subonlypost");
			    } else if (modonlypost) {
				txt = open_text(ml.fd, "deny", "post",
					"modonlypost", NULL, NULL);
			    }
			    MY_ASSERT(txt);
			    register_default_unformatted(txt, &ml);
			    register_unformatted(txt, "subject", subject);
			    register_unformatted(txt, "posteraddr", testaddr);
			    register_originalmail(txt, donemailname);
			    queuefilename = prepstdreply(txt, &ml,
				    "$listowner$", testaddr, NULL);
			    MY_ASSERT(queuefilename)
			    close_text(txt);
			    unlink(donemailname);
			    free(donemailname);
			    send_help(&ml, queuefilename, testaddr);
			}
		}
	}

	if(!send && !moderated) {
		if(statctrl(ml.ctrlfd, "moderated")) {
			moderated = 1;
			modreason = MODERATED;
		}
	}

	notmetoo = statctrl(ml.ctrlfd, "notmetoo");

	if(moderated) {
		xasprintf(&mqueuename, "%s/moderation/%s", ml.dir, randomstr);
		free(randomstr);
		if(rename(donemailname, mqueuename) < 0) {
			log_error(LOG_ARGS, "could not rename(%s,%s)",
					    donemailname, mqueuename);
			free(donemailname);
			free(mqueuename);
			exit(EXIT_FAILURE);
		}
		free(donemailname);
		if (notmetoo) {
			xasprintf(&omitfilename, "%s.omit", mqueuename);
			omitfd = open(omitfilename, O_RDWR|O_CREAT|O_EXCL, S_IRUSR|S_IWUSR);
			if (omitfd < 0) {
				log_error(LOG_ARGS, "could not open %s",
					    	    omitfilename);
				free(mqueuename);
				free(omitfilename);
				exit(EXIT_FAILURE);
			}
			free(omitfilename);
			if(dprintf(omitfd, "%s", posteraddr) < 0) {
				log_error(LOG_ARGS,
						"could not write omit file");
				free(mqueuename);
				exit(EXIT_FAILURE);
			}
			fsync(omitfd);
			close(omitfd);
		}
		newmoderated(&ml, mqueuename, mlmmjsend, efrom, subject,
		    posteraddr, modreason);
		return EXIT_SUCCESS;
	}

	free(randomstr);

	if(noprocess) {
		free(donemailname);
		/* XXX: toemails and ccemails etc. have to be free() */
		exit(EXIT_SUCCESS);
	}

	if (notmetoo)
		exec_or_die(mlmmjsend, "-L", ml.dir, "-o", posteraddr,
		    "-m", donemailname, NULL);
	exec_or_die(mlmmjsend, "-L", ml.dir, "-m", donemailname, NULL);
}
