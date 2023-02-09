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
#include "getlistaddr.h"
#include "getlistdelim.h"
#include "listcontrol.h"
#include "strgen.h"
#include "do_all_the_voodoo_here.h"
#include "log_error.h"
#include "mygetline.h"
#include "statctrl.h"
#include "ctrlvalue.h"
#include "ctrlvalues.h"
#include "getlistaddr.h"
#include "prepstdreply.h"
#include "subscriberfuncs.h"
#include "log_oper.h"
#include "unistr.h"
#include "chomp.h"
#include "mlmmj-process.h"
#include "utils.h"

enum action {
	ALLOW,
	SEND,
	DENY,
	MODERATE,
	DISCARD
};


static char *action_strs[] = {
	"allowed",
	"sent",
	"denied",
	"moderated",
	"discarded"
};


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


static int is_moderator(const char *listdir, const char *address,
		char **moderators) {
	char *buf, *tmp, *moderatorsfilename;
	int moderatorsfd, foundaddr = 0;

	moderatorsfilename = concatstr(2, listdir, "/control/moderators");
	if((moderatorsfd = open(moderatorsfilename, O_RDONLY)) < 0) {
		log_error(LOG_ARGS, "Could not open '%s'", moderatorsfilename);
		free(moderatorsfilename);
		exit(EXIT_FAILURE);
	}
	free(moderatorsfilename);

	while((buf = mygetline(moderatorsfd))) {
		chomp(buf);
		if(address && strcasecmp(buf, address) == 0) {
			foundaddr = 1;
			if (!moderators) {
				close(moderatorsfd);
				free(buf);
				return foundaddr;
			}
		}
		if (moderators) {
			tmp = *moderators;
			*moderators = concatstr(3, *moderators, buf, "\n");
			free(tmp);
		}
		free(buf);
	}

	close(moderatorsfd);
	return foundaddr;
}


static void newmoderated(const char *listdir, const char *mailfilename,
		  const char *mlmmjsend, const char *efromsender,
		  const char *subject, const char *posteraddr,
		  enum modreason modreason, int listfd, int ctrlfd)
{
	char *from, *listfqdn, *listname, *moderators = NULL;
	char *replyto, *listaddr = getlistaddr(ctrlfd), *listdelim;
	text *txt;
	memory_lines_state *mls;
	char *queuefilename = NULL;
	const char *efromismod = NULL;
	const char *mailbasename = mybasename(mailfilename), *to, *reject;
	int notifymod = 0;
#if 0
	printf("mailfilename = [%s], mailbasename = [%s]\n", mailfilename,
			                                     mailbasename);
#endif
	listfqdn = genlistfqdn(listaddr);
	listname = genlistname(listaddr);

	if(statctrl(ctrlfd, "ifmodsendonlymodmoderate"))
		efromismod = efromsender;

	if(!is_moderator(listdir, efromismod, &moderators))
		efromismod = NULL;

	if(efromismod) mls = init_memory_lines(efromismod);
	else mls = init_memory_lines(moderators);

	free(moderators);

	listdelim = getlistdelim(ctrlfd);
	replyto = concatstr(6, listname, listdelim, "release-", mailbasename,
			    "@", listfqdn);
	reject = concatstr(6, listname, listdelim, "reject-", mailbasename,
			    "@", listfqdn);

	from = concatstr(4, listname, listdelim, "owner@", listfqdn);
	to = concatstr(3, listname, "-moderators@", listfqdn); /* FIXME JFA: Should this be converted? Why, why not? */

	free(listdelim);
	free(listname);
	free(listfqdn);

	txt = open_text(listdir, "moderate", "post",
			modreason_strs[modreason], NULL, "moderation");
	MY_ASSERT(txt);
	register_unformatted(txt, "subject", subject);
	register_unformatted(txt, "posteraddr", posteraddr);
	register_unformatted(txt, "moderateaddr", replyto); /* DEPRECATED */
	register_unformatted(txt, "releaseaddr", replyto);
	register_unformatted(txt, "rejectaddr", reject);
	register_unformatted(txt, "moderators", "%moderators%"); /* DEPRECATED */
	register_formatted(txt, "moderators",
			rewind_memory_lines, get_memory_line, mls);
	register_originalmail(txt, mailfilename);
	queuefilename = prepstdreply(txt, listdir, "$listowner$", to, replyto,
	    listfd, ctrlfd);
	MY_ASSERT(queuefilename);
	close_text(txt);

	/* we might need to exec more than one mlmmj-send */
	
	notifymod = !efromismod && statctrl(ctrlfd,"notifymod");

	if (notifymod) {
		free(queuefilename);
		/* send mail to poster that the list is moderated */
		txt = open_text(listdir, "wait", "post",
				modreason_strs[modreason], NULL, "moderation-poster");
		MY_ASSERT(txt);
		register_unformatted(txt, "subject", subject);
		register_unformatted(txt, "posteraddr", posteraddr);
		register_unformatted(txt, "moderators", "%moderators%"); /* DEPRECATED */
		register_formatted(txt, "moderators",
				rewind_memory_lines, get_memory_line, mls);
		register_originalmail(txt, mailfilename);
		queuefilename = prepstdreply(txt, listdir,
				"$listowner$", efromsender, NULL, listfd, ctrlfd);
		MY_ASSERT(queuefilename);
		close_text(txt);

		finish_memory_lines(mls);

		exec_and_wait(mlmmjsend, "-l", "1", "-L", listdir, "-F", from,
		    "-m", queuefilename, "-T", efromsender, NULL);
	}

	if (efromismod)
		exec_or_die(mlmmjsend, "-l", "1", "-L", listdir, "-F", from,
		    "-m", queuefilename, "-T", efromsender, NULL);
	exec_or_die(mlmmjsend, "-l", "2", "-L", listdir, "-F", from,
	    "-m", queuefilename, NULL);
}


static enum action do_access(strlist *rule_strs, strlist *hdrs,
		const char *from, int listfd)
{
	unsigned int match;
	char *rule_ptr;
	char errbuf[128];
	int err;
	enum action act;
	unsigned int not;
	regex_t regexp;
	char *hdr;
	size_t i = 0;

	tll_foreach(*rule_strs, it) {
		rule_ptr = it->item;
		if (strncmp(rule_ptr, "allow", 5) == 0) {
			rule_ptr += 5;
			act = ALLOW;
		} else if (strncmp(rule_ptr, "send", 4) == 0) {
			rule_ptr += 4;
			act = SEND;
		} else if (strncmp(rule_ptr, "deny", 4) == 0) {
			rule_ptr += 4;
			act = DENY;
		} else if (strncmp(rule_ptr, "moderate", 8) == 0) {
			rule_ptr += 8;
			act = MODERATE;
		} else if (strncmp(rule_ptr, "discard", 7) == 0) {
			rule_ptr += 7;
			act = DISCARD;
		} else {
			errno = 0;
			log_error(LOG_ARGS, "Unable to parse rule #%d \"%s\":"
					" Missing action keyword. Denying post from \"%s\"",
					i, it->item, from);
			log_oper(listfd, OPLOGFNAME, "Unable to parse rule #%d \"%s\":"
					" Missing action keyword. Denying post from \"%s\"",
					i, it->item, from);
			return DENY;
		}

		if (*rule_ptr == ' ') {
			rule_ptr++;
		} else if (*rule_ptr == '\0') {
			/* the rule is a keyword and no regexp */
			log_oper(listfd, OPLOGFNAME, "mlmmj-process: access -"
					" A mail from \"%s\" was %s by rule #%d \"%s\"",
					from, action_strs[act], i, it->item);
			return act;
		} else {
			/* we must have space or end of string */
			errno = 0;
			log_error(LOG_ARGS, "Unable to parse rule #%d \"%s\":"
					" Invalid character after action keyword."
					" Denying post from \"%s\"", i, it->item, from);
			log_oper(listfd, OPLOGFNAME, "Unable to parse rule #%d \"%s\":"
					" Invalid character after action keyword."
					" Denying post from \"%s\"", i, it->item, from);
			return DENY;
		}

		if (*rule_ptr == '!') {
			rule_ptr++;
			not = 1;
		} else {
			not = 0;
		}

		/* remove unanchored ".*" from beginning of regexp to stop the
		 * regexp matching to loop so long time it seems like it's
		 * hanging */
		if (strncmp(rule_ptr, "^.*", 3) == 0) {
			rule_ptr += 3;
		}
		while (strncmp(rule_ptr, ".*", 2) == 0) {
			rule_ptr += 2;
		}

		if ((err = regcomp(&regexp, rule_ptr,
				REG_EXTENDED | REG_NOSUB | REG_ICASE))) {
			regerror(err, &regexp, errbuf, sizeof(errbuf));
			regfree(&regexp);
			errno = 0;
			log_error(LOG_ARGS, "regcomp() failed for rule #%d \"%s\""
					" (message: '%s') (expression: '%s')"
					" Denying post from \"%s\"",
					i, it->item, errbuf, rule_ptr, from);
			log_oper(listfd, OPLOGFNAME, "regcomp() failed for rule"
					" #%d \"%s\" (message: '%s') (expression: '%s')"
					" Denying post from \"%s\"",
					i, it->item, errbuf, rule_ptr, from);
			return DENY;
		}

		match = 0;
		tll_foreach(*hdrs, header) {
			if (regexec(&regexp, header->item, 0, NULL, 0)
					== 0) {
				match = 1;
				hdr = header->item;
				break;
			}
		}

		regfree(&regexp);

		if (match != not) {
			if (match) {
				log_oper(listfd, OPLOGFNAME, "mlmmj-process: access -"
						" A mail from \"%s\" with header \"%s\" was %s by"
						" rule #%d \"%s\"", from, hdr, action_strs[act],
						i, it->item);
			} else {
				log_oper(listfd, OPLOGFNAME, "mlmmj-process: access -"
						" A mail from \"%s\" was %s by rule #%d \"%s\""
						" because no header matched.", from,
						action_strs[act], i, it->item);
			}
			return act;
		}
		i++;
	}

	log_oper(listfd, OPLOGFNAME, "mlmmj-process: access -"
			" A mail from \"%s\" didn't match any rules, and"
			" was denied by default.", from);
	return DENY;
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
	int i, opt, noprocess = 0, moderated = 0, send = 0;
	enum modreason modreason;
	int hdrfd, footfd, rawmailfd, donemailfd, omitfd;
	bool addr_in_to_or_cc, notmetoo;
	bool findaddress = false, intocc = false;
	int maxmailsize = 0;
	bool subonlypost, modonlypost, modnonsubposts, foundaddr = 0;
	char *listdir = NULL, *mailfile = NULL, *donemailname = NULL;
	char *randomstr = NULL, *mqueuename, *omitfilename;
	char *mlmmjsend, *mlmmjsub, *mlmmjunsub;
	char *bindir, *subjectprefix, *discardname, *listaddr, *listdelim = NULL;
	char *listfqdn, *listname, *fromaddr;
	text *txt;
	char *queuefilename, *recipextra = NULL, *owner = NULL;
	char *maxmailsizestr;
	char *subject = NULL, *posteraddr = NULL;
	char *envstr, *efrom;
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
	int listfd, ctrlfd;

	CHECKFULLPATH(argv[0]);

	log_set_name(argv[0]);

	bindir = mydirname(argv[0]);
	mlmmjsend = concatstr(2, bindir, "/mlmmj-send");
	mlmmjsub = concatstr(2, bindir, "/mlmmj-sub");
	mlmmjunsub = concatstr(2, bindir, "/mlmmj-unsub");
	free(bindir);

	while ((opt = getopt(argc, argv, "hVPm:L:")) != -1) {
		switch(opt) {
		case 'L':
			listdir = optarg;
			break;
		case 'm':
			mailfile = optarg;
			break;
		case 'h':
			print_help(argv[0]);
			break;
		case 'P':
			noprocess = 1;
			break;
		case 'V':
			print_version(argv[0]);
			exit(EXIT_SUCCESS);
		}
	}

	if(listdir == NULL || mailfile == NULL) {
		errx(EXIT_FAILURE, "You have to specify -L and -m\n"
		    "%s -h for help", argv[0]);
	}

	listfd = open_listdir(listdir, true);
	ctrlfd = openat(listfd, "control", O_DIRECTORY|O_CLOEXEC);
	if (ctrlfd == -1)
		err(EXIT_FAILURE, "Cannot open(%s/control)", listdir);

        do {
                free(donemailname);
                free(randomstr);
                randomstr = random_str();
                donemailname = concatstr(3, listdir, "/queue/", randomstr);

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
	hdrfd = openat(ctrlfd, "customheaders", O_RDONLY);

	/* footfd is checked in do_all_the_voodoo_here(), see above */
	footfd = openat(ctrlfd, "footer", O_RDONLY);

	delheaders = ctrlvalues(ctrlfd, "delheaders");
	if(delheaders == NULL)
		delheaders = xcalloc(1, sizeof(*delheaders));

	tll_push_back(*delheaders, xstrdup("From "));
	tll_push_back(*delheaders, xstrdup("Return-Path:"));

	subjectprefix = ctrlvalue(ctrlfd, "prefix");

	if(do_all_the_voodoo_here(rawmailfd, donemailfd, hdrfd, footfd,
				delheaders, readhdrs,
				&allheaders, subjectprefix) < 0) {
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

	recipextra = get_recipextra_from_env(ctrlfd);
	if (recipextra == NULL) {
		findaddress = true;
		listdelim = getlistdelim(ctrlfd);
	}
	addr_in_to_or_cc = !statctrl(ctrlfd, "tocc");

	if(addr_in_to_or_cc || findaddress) {
		listaddrs = ctrlvalues(ctrlfd, "listaddress");
		if (listaddrs == NULL) {
			log_error(LOG_ARGS, "list address is not defined");
			err(EXIT_FAILURE, "list address is not defined");
		}
		tll_foreach(dtemails, it) {
			tll_foreach(*listaddrs, la) {
				if (addrmatch(la->item, it->item,
				    listdelim,
				    recipextra != NULL ? NULL : &recipextra)) {
					findaddress = 0;
					free(listdelim);
					listdelim = NULL;
					break;
				}
			}
		}
	}
	if(addr_in_to_or_cc || findaddress) {
		tll_foreach(toemails, it) {
			tll_foreach(*listaddrs, la) {
				if(addrmatch(la->item, it->item,
				listdelim,
				recipextra != NULL ? NULL : &recipextra)) {
					intocc = true;
					break;
				}
			}
		}
		if (!intocc)
			tll_foreach(ccemails, it) {
				tll_foreach(*listaddrs, la) {
					if(addrmatch(la->item, it->item,
						listdelim,
					   recipextra != NULL ? NULL : &recipextra)) {
						intocc = true;
						break;
					}
				}
			}
	}
	if (listdelim) {
		free(listdelim);
		listdelim = NULL;
	}
	if (listaddrs)
		tll_free_and_free(*listaddrs, free);
	free(listaddrs);

	if(recipextra && (strlen(recipextra) == 0)) {
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
		discardname = concatstr(3, listdir,
				"/queue/discarded/", randomstr);
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
		efrom = xstrdup(envstr);
	} else if(tll_length(rpemails) >= 1) {
		/* the (first) Return-Path: header */
		efrom = xstrdup(tll_front(rpemails));
	} else {
		efrom = xstrdup("");
	}

	/* Subject: */
	if (readhdrs[5].valuecount)
			subject = unistr_header_to_utf8(readhdrs[5].values[0]);
	if (!subject) subject = xstrdup("");

	if(recipextra) {
		owner = concatstr(2, listdir, "/control/owner");
		if(owner && strcmp(recipextra, "owner") == 0) {
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
					NULL, &allheaders, NULL) < 0) {
				log_error(LOG_ARGS, "do_all_the_voodoo_here");
				exit(EXIT_FAILURE);
			}
			tll_free_and_free(*delheaders, free);
			close(rawmailfd);
			close(donemailfd);
			unlink(mailfile);
			log_oper(listfd, OPLOGFNAME, "mlmmj-process: sending"
					" mail from %s to owner",
					efrom);
			exec_or_die(mlmmjsend, "-l", "4", "-L", listdir, "-F",
			    efrom, "-s", owner, "-a", "-m", donemailname, NULL);
		}
#if 0
		log_error(LOG_ARGS, "listcontrol(from, %s, %s, %s, %s, %s, %s, %s)\n", listdir, toemails.emaillist[0], mlmmjsub, mlmmjunsub, mlmmjsend, mlmmjbounce, donemailname);
#endif
		unlink(mailfile);
		if (tll_length(originalfromemails) > 0)
			testfrom = &originalfromemails;
		else
			testfrom = &fromemails;
		listcontrol(testfrom, listdir, recipextra,
			    mlmmjsub, mlmmjunsub, mlmmjsend,
			    donemailname, listfd, ctrlfd);

		return EXIT_SUCCESS;
	}

	listaddr = getlistaddr(ctrlfd);

	/* checking incoming mail's size */
	maxmailsize = ctrlsizet(ctrlfd, "maxmailsize", 0);
	maxmailsizestr = ctrlvalue(ctrlfd, "maxmailsize");
	if(maxmailsizestr) {
		maxmailsize = atol(maxmailsizestr);
		if(stat(donemailname, &st) < 0) {
			log_error(LOG_ARGS, "stat(%s,..) failed", donemailname);
			exit(EXIT_FAILURE);
		}

		if(st.st_size > maxmailsize) {

			if (statctrl(ctrlfd, "nomaxmailsizedenymails")) {
				errno = 0;
				log_error(LOG_ARGS, "Discarding %s due to"
						" size limit (%d bytes too big)",
						donemailname, (st.st_size - maxmailsize));
				unlink(donemailname);
				unlink(mailfile);
				free(donemailname);
				free(maxmailsizestr);
				exit(EXIT_SUCCESS);
			}

			listdelim = getlistdelim(ctrlfd);
			listname = genlistname(listaddr);
			listfqdn = genlistfqdn(listaddr);
			fromaddr = concatstr(4, listname, listdelim,
					"bounces-help@", listfqdn);
			txt = open_text(listdir, "deny", "post",
					"maxmailsize", NULL, "maxmailsize");
			MY_ASSERT(txt);
			register_unformatted(txt, "subject", subject);
			register_unformatted(txt, "posteraddr", posteraddr);
			register_unformatted(txt, "maxmailsize", maxmailsizestr);
			register_originalmail(txt, donemailname);
			queuefilename = prepstdreply(txt, listdir,
			    "$listowner$", posteraddr, NULL, listfd, ctrlfd);
			MY_ASSERT(queuefilename);
			close_text(txt);
			free(listdelim);
			free(listname);
			free(listfqdn);
			unlink(donemailname);
			unlink(mailfile);
			free(donemailname);
			free(maxmailsizestr);
			exec_or_die(mlmmjsend, "-l", "1", "-L", listdir, "-T",
			    posteraddr, "-F", fromaddr, "-m", queuefilename,
			    NULL);
		}
	}

	free(delheaders);

	if(strcmp(efrom, "") == 0) { /* don't send mails with <> in From
					     to the list */
		discardname = concatstr(3, listdir,
				"/queue/discarded/",
				randomstr);
		errno = 0;
		log_error(LOG_ARGS, "Discarding %s due to missing envelope"
				" from address", mailfile);
		rename(mailfile, discardname);
		unlink(donemailname);
		free(donemailname);
		free(discardname);
		free(randomstr);
		/* TODO: free emailstructs */
		exit(EXIT_SUCCESS);
	}

	unlink(mailfile);

	if(addr_in_to_or_cc && !intocc) {
		/* Don't send a mail about denial to the list, but silently
		 * discard and exit. Also don't in case of it being turned off
		 */
		if ((strcasecmp(listaddr, posteraddr) == 0) ||
				statctrl(ctrlfd, "notoccdenymails")) {
			log_error(LOG_ARGS, "Discarding %s because list"
					" address was not in To: or Cc:,"
					" and From: was the list or"
					" notoccdenymails was set",
					mailfile);
			free(listaddr);
			unlink(donemailname);
			free(donemailname);
			exit(EXIT_SUCCESS);
		}
		listdelim = getlistdelim(ctrlfd);
		listname = genlistname(listaddr);
		listfqdn = genlistfqdn(listaddr);
		fromaddr = concatstr(4, listname, listdelim, "bounces-help@",
				     listfqdn);
		txt = open_text(listdir, "deny", "post",
				"tocc", NULL, "notintocc");
		MY_ASSERT(txt);
		register_unformatted(txt, "subject", subject);
		register_unformatted(txt, "posteraddr", posteraddr);
		register_originalmail(txt, donemailname);
		queuefilename = prepstdreply(txt, listdir,
		    "$listowner$", posteraddr, NULL, listfd, ctrlfd);
		MY_ASSERT(queuefilename)
		close_text(txt);
		free(listdelim);
		free(listname);
		free(listfqdn);
		unlink(donemailname);
		free(donemailname);
		exec_or_die(mlmmjsend, "-l", "1", "-L", listdir, "-T",
		    posteraddr, "-F", fromaddr, "-m", queuefilename, NULL);
	}

	access_rules = ctrlvalues(ctrlfd, "access");
	if (access_rules != NULL) {
		enum action accret;
		/* Don't send a mail about denial to the list, but silently
		 * discard and exit. Also do this in case it's turned off */
		accret = do_access(access_rules, &allheaders,
					posteraddr, listfd);
		if (accret == DENY) {
			if ((strcasecmp(listaddr, posteraddr) == 0) ||
				    statctrl(ctrlfd, "noaccessdenymails")) {
				log_error(LOG_ARGS, "Discarding %s because"
					" it was denied by an access"
					" rule, and From: was the list"
					" address or noaccessdenymails"
					" was set",
					mailfile);
				free(listaddr);
				unlink(donemailname);
				free(donemailname);
				exit(EXIT_SUCCESS);
			}
			listdelim = getlistdelim(ctrlfd);
			listname = genlistname(listaddr);
			listfqdn = genlistfqdn(listaddr);
			fromaddr = concatstr(4, listname, listdelim,
					"bounces-help@", listfqdn);
			txt = open_text(listdir, "deny", "post",
					"access", NULL, "access");
			MY_ASSERT(txt);
			register_unformatted(txt, "subject", subject);
			register_unformatted(txt, "posteraddr", posteraddr);
			register_originalmail(txt, donemailname);
			queuefilename = prepstdreply(txt, listdir,
					"$listowner$", posteraddr, NULL, listfd, ctrlfd);
			MY_ASSERT(queuefilename)
			close_text(txt);
			free(listaddr);
			free(listdelim);
			free(listname);
			free(listfqdn);
			unlink(donemailname);
			free(donemailname);
			free(randomstr);
			exec_or_die(mlmmjsend, "-l", "1", "-L", listdir, "-T",
			    posteraddr, "-F", fromaddr, "-m", queuefilename,
			    NULL);
		} else if (accret == MODERATE) {
			moderated = 1;
			modreason = ACCESS;
		} else if (accret == DISCARD) {
	                discardname = concatstr(3, listdir,
                                "/queue/discarded/", randomstr);
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
		} else if (accret == SEND) {
			send = 1;
		} else if (accret == ALLOW) {
			/* continue processing as normal */
		}
	}

	list_rules = ctrlvalues(ctrlfd, "send");
	if (list_rules != NULL) {
		tll_foreach(*list_rules, lr) {
			if (strcasecmp(posteraddr, lr->item) == 0) {
				send = 1;
				break;
			}
		}
	}

	subonlypost = statctrl(ctrlfd, "subonlypost");
	modonlypost = statctrl(ctrlfd, "modonlypost");
	modnonsubposts = statctrl(ctrlfd, "modnonsubposts");
	/* modnonsubposts implies subonlypost if modonlypost is not set */
	if (modnonsubposts && !modonlypost) subonlypost = 1;

	if(!send && (subonlypost || modonlypost || modnonsubposts)) {
		/* Don't send a mail about denial to the list, but silently
		 * discard and exit. */
		char *testaddr = posteraddr;
		if (tll_length(originalfromemails) > 0)
			testaddr = tll_front(originalfromemails);
		if (strcasecmp(listaddr, testaddr) == 0) {
			log_error(LOG_ARGS, "Discarding %s because"
					" there are sender restrictions but"
					" From: was the list address",
					mailfile);
			free(listaddr);
			unlink(donemailname);
			free(donemailname);
			exit(EXIT_SUCCESS);
		}
		if(subonlypost) {
			foundaddr = (is_subbed(listfd, testaddr, 0) !=
					SUB_NONE);
		} else if (modonlypost) {
			foundaddr = is_moderator(listdir, testaddr, NULL);
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
				    statctrl(ctrlfd, "nosubonlydenymails")) ||
				    (modonlypost &&
				    statctrl(ctrlfd, "nomodonlydenymails"))) {
				log_error(LOG_ARGS, "Discarding %s because"
					" no{sub|mod}onlydenymails was set",
					mailfile);
				free(listaddr);
				unlink(donemailname);
				free(donemailname);
				exit(EXIT_SUCCESS);
			    }
			    listdelim = getlistdelim(ctrlfd);
			    listname = genlistname(listaddr);
			    listfqdn = genlistfqdn(listaddr);
			    fromaddr = concatstr(4, listname, listdelim,
				    "bounces-help@", listfqdn);
			    if (subonlypost) {
				txt = open_text(listdir, "deny", "post",
					"subonlypost", NULL, "subonlypost");
			    } else if (modonlypost) {
				txt = open_text(listdir, "deny", "post",
					"modonlypost", NULL, NULL);
			    }
			    MY_ASSERT(txt);
			    register_unformatted(txt, "subject", subject);
			    register_unformatted(txt, "posteraddr", testaddr);
			    register_originalmail(txt, donemailname);
			    queuefilename = prepstdreply(txt, listdir,
				    "$listowner$", testaddr, NULL, listfd, ctrlfd);
			    MY_ASSERT(queuefilename)
			    close_text(txt);
			    free(listaddr);
			    free(listdelim);
			    free(listname);
			    free(listfqdn);
			    unlink(donemailname);
			    free(donemailname);
				exec_or_die(mlmmjsend, "-L", listdir, "-l", "1",
				    "-T", testaddr, "-F", fromaddr, "-m",
				    queuefilename, NULL);
			}
		}
	}

	if(!send && !moderated) {
		if(statctrl(ctrlfd, "moderated")) {
			moderated = 1;
			modreason = MODERATED;
		}
	}

	notmetoo = statctrl(ctrlfd, "notmetoo");

	if(moderated) {
		mqueuename = concatstr(3, listdir, "/moderation/",
				       randomstr);
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
			omitfilename = concatstr(2, mqueuename, ".omit");
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
		newmoderated(listdir, mqueuename,
				mlmmjsend, efrom, subject, posteraddr,
				modreason, listfd, ctrlfd);
		return EXIT_SUCCESS;
	}

	free(randomstr);

	if(noprocess) {
		free(donemailname);
		/* XXX: toemails and ccemails etc. have to be free() */
		exit(EXIT_SUCCESS);
	}

	if (notmetoo)
		exec_or_die(mlmmjsend, "-L", listdir, "-o", posteraddr,
		    "-m", donemailname, NULL);
	exec_or_die(mlmmjsend, "-L", listdir, "-m", donemailname, NULL);
}
