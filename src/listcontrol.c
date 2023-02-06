/* Copyright (C) 2003 Mads Martin Joergensen <mmj at mmj.dk>
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
#include <strings.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <ctype.h>

#include "xmalloc.h"
#include "mlmmj.h"
#include "listcontrol.h"
#include "getlistdelim.h"
#include "strgen.h"
#include "prepstdreply.h"
#include "send_help.h"
#include "send_list.h"
#include "log_error.h"
#include "statctrl.h"
#include "mygetline.h"
#include "chomp.h"
#include "log_oper.h"
#include "ctrlvalues.h"
#include "subscriberfuncs.h"
#include "utils.h"


/* Must match the enum. CAREFUL when using commands that are substrings
 * of other commands. In that case the longest one have to be listed
 * first to match correctly. */
static struct ctrl_command ctrl_commands[] = {
	{ "subscribe-digest",   0, CTRL_SUBSCRIBE_DIGEST },
	{ "subscribe-nomail",   0, CTRL_SUBSCRIBE_NOMAIL },
	{ "subscribe-both",     0, CTRL_SUBSCRIBE_BOTH },
	{ "subscribe",          0, CTRL_SUBSCRIBE },
	{ "confsub-digest",     1, CTRL_CONFSUB_DIGEST },
	{ "confsub-nomail",     1, CTRL_CONFSUB_NOMAIL },
	{ "confsub-both",       1, CTRL_CONFSUB_BOTH },
	{ "confsub",            1, CTRL_CONFSUB },
	{ "unsubscribe-digest", 0, CTRL_UNSUBSCRIBE_DIGEST },
	{ "unsubscribe-nomail", 0, CTRL_UNSUBSCRIBE_NOMAIL },
	{ "unsubscribe",        0, CTRL_UNSUBSCRIBE },
	{ "confunsub-digest",   1, CTRL_CONFUNSUB_DIGEST },
	{ "confunsub-nomail",   1, CTRL_CONFUNSUB_NOMAIL },
	{ "confunsub",          1, CTRL_CONFUNSUB },
	{ "bounces",            1, CTRL_BOUNCES },
	{ "release",            1, CTRL_RELEASE },
	{ "reject",             1, CTRL_REJECT },
	{ "permit",             1, CTRL_PERMIT },
	{ "obstruct",           1, CTRL_OBSTRUCT },
	{ "moderate",           1, CTRL_MODERATE },
	{ "help",               0, CTRL_HELP },
	{ "faq",                0, CTRL_FAQ },
	{ "get",                1, CTRL_GET },
	{ "list",               0, CTRL_LIST }
};

#ifndef NELEM
#define NELEM(array) (sizeof(array) / sizeof((array)[0]))
#endif

struct ctrl_command *
get_ctrl_command(const char *controlstr, char **param)
{
	size_t cmdlen;
	unsigned int ctrl;

	for (ctrl=0; ctrl < NELEM(ctrl_commands); ctrl++) {
		cmdlen = strlen(ctrl_commands[ctrl].command);
		if (strncmp(controlstr, ctrl_commands[ctrl].command, cmdlen)
		    != 0)
			continue;
		if (ctrl_commands[ctrl].accepts_parameter) {
			if (controlstr[cmdlen] != '-') {
				errno = 0;
				log_error(LOG_ARGS, "Command \"%s\""
					" requires a parameter, but no"
					" parameter was given."
					" Ignoring mail",
					ctrl_commands[ctrl].command);
				return (NULL);
			}
			if (strchr(controlstr + cmdlen + 1, '/')) {
				errno = 0;
				log_error(LOG_ARGS, "Slash (/) in"
					" list control request,"
					" discarding mail");
				return (NULL);
			}
			*param = xstrdup(controlstr + cmdlen + 1);
		} else {
			if (controlstr[cmdlen] != '\0') {
				errno = 0;
				log_error(LOG_ARGS, "Command \"%s\""
					" does not accept a parameter,"
					" but a parameter was given."
					" Ignoring mail",
					ctrl_commands[ctrl].command);
				return (NULL);
			}
			*param = NULL;
		}
		return (&ctrl_commands[ctrl]);
	}

	return (NULL);
}

int listcontrol(strlist *fromemails, const char *listdir,
		const char *controlstr, const char *mlmmjsub,
		const char *mlmmjunsub, const char *mlmmjsend,
		const char *mailname, int listfd, int ctrlfd)
{
	char *bouncenr, *tmpstr;
	char *param = NULL, *conffilename, *moderatefilename, *gatekeepfilename;
	char *omitfilename;
	char *omit = NULL;
	char *c, *archivefilename, *sendfilename;
	struct stat stbuf;
	bool closedlist, closedlistsub, nosubconfirm, subonlyget;
	int tmpfd, noget;
	strlist *owners;
	text *txt;
	char *queuefilename;
	const char *subtype = NULL;
	const char *subtypename = NULL;
	bounce_t bret;
	struct ctrl_command *cc;

	/* A closed list doesn't allow subscribtion and unsubscription */
	closedlist = statctrl(ctrlfd, "closedlist");

	/* A closed list "sub" only dissallows subscription, not unsub. */
	closedlistsub = statctrl(ctrlfd, "closedlistsub");

	nosubconfirm = statctrl(ctrlfd, "nosubconfirm");
	
#if 0
	log_error(LOG_ARGS, "controlstr = [%s]\n", controlstr);
	log_error(LOG_ARGS, "tll_front(*fromemails) = [%s]\n",
			tll_front(*fromemails));
#endif
	cc = get_ctrl_command(controlstr, &param);
	if (cc == NULL)
		return (-1);

	if(tll_length(*fromemails) != 1 && cc->type != CTRL_BOUNCES) {
		errno = 0;
		log_error(LOG_ARGS, "Ignoring mail with invalid From: "
				"which was not a bounce: %d", tll_length(*fromemails));
		return -1;
	}

	/* We only need the control mail when bouncing, to save bounced msg */
	if(cc->type != CTRL_BOUNCES)
		unlink(mailname);

	switch (cc->type) {

	/* listname+subscribe-digest@domain.tld */
	case CTRL_SUBSCRIBE_DIGEST:
		if (closedlist || closedlistsub) {
			errno = 0;
			log_error(LOG_ARGS, "A subscribe-digest request was"
				" sent to a closed list. Ignoring mail");
			return -1;
		}
		if (!strchr(tll_front(*fromemails), '@')) {
			/* Not a valid From: address */
			errno = 0;
			log_error(LOG_ARGS, "A subscribe-digest request was"
				" sent with an invalid From: header."
				" Ignoring mail");
			return -1;
		}
		if (statctrl(ctrlfd, "nodigestsub")) {
			errno = 0;
			log_error(LOG_ARGS, "A subscribe-digest request was"
				" denied");
			txt = open_text(listdir, "deny", "sub", "disabled",
					"digest", "sub-deny-digest");
			MY_ASSERT(txt);
			register_unformatted(txt, "subaddr",
					tll_front(*fromemails));
			queuefilename = prepstdreply(txt, listdir,
					"$listowner$",
					tll_front(*fromemails), NULL, listfd, ctrlfd);
			MY_ASSERT(queuefilename);
			close_text(txt);
			send_help(listdir, queuefilename,
					tll_front(*fromemails), mlmmjsend, ctrlfd);
			return -1;
		}
		log_oper(listfd, OPLOGFNAME, "mlmmj-sub: request for digest"
			" subscription from %s", tll_front(*fromemails));
		exec_or_die(mlmmjsub, "-L", listdir, "-a",
		    tll_front(*fromemails), "-d", "-r", "-c",
		    (nosubconfirm ? NULL : "-C"), NULL);
		break;

	/* listname+subscribe-nomail@domain.tld */
	case CTRL_SUBSCRIBE_NOMAIL:
		if (closedlist || closedlistsub) {
			errno = 0;
			log_error(LOG_ARGS, "A subscribe-nomail request was"
				" sent to a closed list. Ignoring mail");
			return -1;
		}
		if (!strchr(tll_front(*fromemails), '@')) {
			/* Not a valid From: address */
			errno = 0;
			log_error(LOG_ARGS, "A subscribe-nomail request was"
				" sent with an invalid From: header."
				" Ignoring mail");
			return -1;
		}
		if (statctrl(ctrlfd, "nonomailsub")) {
			errno = 0;
			log_error(LOG_ARGS, "A subscribe-nomail request was"
				" denied");
			txt = open_text(listdir, "deny", "sub", "disabled",
					"nomail", "sub-deny-nomail");
			MY_ASSERT(txt);
			register_unformatted(txt, "subaddr",
					tll_front(*fromemails));
			queuefilename = prepstdreply(txt, listdir,
					"$listowner$",
					tll_front(*fromemails), NULL, listfd, ctrlfd);
			MY_ASSERT(queuefilename);
			close_text(txt);
			send_help(listdir, queuefilename,
					tll_front(*fromemails), mlmmjsend, ctrlfd);
			return -1;
		}
		log_oper(listfd, OPLOGFNAME, "mlmmj-sub: request for nomail"
			" subscription from %s", tll_front(*fromemails));
		exec_or_die(mlmmjsub, "-L", listdir, "-a",
		    tll_front(*fromemails), "-n", "-r", "-c",
		    (nosubconfirm ? NULL : "-C"), NULL);
		break;

	/* listname+subscribe-both@domain.tld */
	case CTRL_SUBSCRIBE_BOTH:
		if (closedlist || closedlistsub) {
			errno = 0;
			log_error(LOG_ARGS, "A subscribe-both request was"
				" sent to a closed list. Ignoring mail");
			return -1;
		}
		if (!strchr(tll_front(*fromemails), '@')) {
			/* Not a valid From: address */
			errno = 0;
			log_error(LOG_ARGS, "A subscribe-both request was"
				" sent with an invalid From: header."
				" Ignoring mail");
			return -1;
		}
		if (statctrl(ctrlfd, "nodigestsub")) {
			errno = 0;
			log_error(LOG_ARGS, "A subscribe-both request was"
				" denied");
			txt = open_text(listdir, "deny", "sub", "disabled",
					"both", "sub-deny-digest");
			MY_ASSERT(txt);
			register_unformatted(txt, "subaddr",
					tll_front(*fromemails));
			queuefilename = prepstdreply(txt, listdir,
					"$listowner$",
					tll_front(*fromemails), NULL, listfd, ctrlfd);
			MY_ASSERT(queuefilename);
			close_text(txt);
			send_help(listdir, queuefilename,
					tll_front(*fromemails), mlmmjsend, ctrlfd);
			return -1;
		}
		log_oper(listfd, OPLOGFNAME, "mlmmj-sub: request for both"
			" subscription from %s", tll_front(*fromemails));
		exec_or_die(mlmmjsub, "-L", listdir, "-a",
		    tll_front(*fromemails), "-b", "-r", "-c",
		    (nosubconfirm ? NULL : "-C"), NULL);
		break;

	/* listname+subscribe@domain.tld */
	case CTRL_SUBSCRIBE:
		if (closedlist || closedlistsub) {
			errno = 0;
			log_error(LOG_ARGS, "A subscribe request was"
				" sent to a closed list. Ignoring mail");
			return -1;
		}
		if (!strchr(tll_front(*fromemails), '@')) {
			/* Not a valid From: address */
			errno = 0;
			log_error(LOG_ARGS, "A subscribe request was"
				" sent with an invalid From: header."
				" Ignoring mail");
			return -1;
		}
		log_oper(listfd, OPLOGFNAME, "mlmmj-sub: request for regular"
			" subscription from %s", tll_front(*fromemails));
		exec_or_die(mlmmjsub, "-L", listdir, "-a",
		    tll_front(*fromemails), "-r", "-c",
		    (nosubconfirm ? NULL : "-C"), NULL);
		break;

	/* listname+subconf-digest-COOKIE@domain.tld */
	case CTRL_CONFSUB_DIGEST:
		subtype = "-d";
		subtypename = "digest";
		/* FALLTHROUH */
	/* listname+subconf-nomail-COOKIE@domain.tld */
	case CTRL_CONFSUB_NOMAIL:
		if (subtype == NULL) {
			subtype = "-n";
			subtypename = "nomail";
		}
		/* FALLTHROUH */
	/* listname+subconf-both-COOKIE@domain.tld */
	case CTRL_CONFSUB_BOTH:
		if (subtype == NULL) {
			subtype = "-b";
			subtypename = "both";
		}
		conffilename = concatstr(3, listdir, "/subconf/", param);
		free(param);
		if((tmpfd = open(conffilename, O_RDONLY)) < 0) {
			/* invalid COOKIE */
			errno = 0;
			log_error(LOG_ARGS, "A subconf request was"
				" sent with a mismatching cookie."
				" Ignoring mail");
			return -1;
		}
		tmpstr = mygetline(tmpfd);
		chomp(tmpstr);
		close(tmpfd);
		log_oper(listfd, OPLOGFNAME, "mlmmj-sub: %s confirmed"
			" subscription to %s", tmpstr, subtypename);
		exec_or_die(mlmmjsub, "-L", listdir, "-a", tmpstr, "-n",
		    "-R", "-c", NULL);
		break;

	/* listname+subconf-COOKIE@domain.tld */
	case CTRL_CONFSUB:
		conffilename = concatstr(3, listdir, "/subconf/", param);
		free(param);
		if((tmpfd = open(conffilename, O_RDONLY)) < 0) {
			/* invalid COOKIE */
			errno = 0;
			log_error(LOG_ARGS, "A subconf request was"
				" sent with a mismatching cookie."
				" Ignoring mail");
			return -1;
		}
		tmpstr = mygetline(tmpfd);
		chomp(tmpstr);
		close(tmpfd);
		unlink(conffilename);
		log_oper(listfd, OPLOGFNAME, "mlmmj-sub: %s confirmed"
			" subscription to regular list", tmpstr);
		exec_or_die(mlmmjsub, "-L", listdir, "-a", tmpstr,
		    "-R", "-c", NULL);
		break;

	/* DEPRECATED: listname+unsubscribe-digest@domain.tld */
	case CTRL_UNSUBSCRIBE_DIGEST:
	/* DEPRECATED: listname+unsubscribe-nomail@domain.tld */
	case CTRL_UNSUBSCRIBE_NOMAIL:
	/* listname+unsubscribe@domain.tld */
	case CTRL_UNSUBSCRIBE:
		if (closedlist) {
			errno = 0;
			log_error(LOG_ARGS, "An unsubscribe request was"
				" sent to a closed list. Ignoring mail");
			return -1;
		}
		if (!strchr(tll_front(*fromemails), '@')) {
			/* Not a valid From: address */
			errno = 0;
			log_error(LOG_ARGS, "An unsubscribe request was"
				" sent with an invalid From: header."
				" Ignoring mail");
			return -1;
		}
		log_oper(listfd, OPLOGFNAME, "mlmmj-unsub: %s requests"
			" unsubscribe", tll_front(*fromemails));
		exec_or_die(mlmmjunsub, "-L", listdir, "-a",
		    tll_front(*fromemails), "-r",
		    (nosubconfirm ? "-c" : "-C"), NULL);
		break;

	/* listname+unsubconf-digest-COOKIE@domain.tld */
	case CTRL_CONFUNSUB_DIGEST:
		if (closedlist) {
			errno = 0;
			log_error(LOG_ARGS, "An unsubconf-digest request was"
				" sent to a closed list. Ignoring mail");
			return -1;
		}
		conffilename = concatstr(3, listdir, "/unsubconf/", param);
		free(param);
		if((tmpfd = open(conffilename, O_RDONLY)) < 0) {
			/* invalid COOKIE */
			errno = 0;
			log_error(LOG_ARGS, "An unsubconf-digest request was"
				" sent with a mismatching cookie."
				" Ignoring mail");
			return -1;
		}
		tmpstr = mygetline(tmpfd);
		close(tmpfd);
		chomp(tmpstr);
		unlink(conffilename);
		log_oper(listfd, OPLOGFNAME, "mlmmj-unsub: %s confirmed"
			" unsubscribe from digest", tmpstr);
		exec_or_die(mlmmjunsub, "-L", listdir, "-a", tmpstr, "-d",
		    "-R", "-c", NULL);
		break;

	/* listname+unsubconf-nomail-COOKIE@domain.tld */
	case CTRL_CONFUNSUB_NOMAIL:
		if (closedlist) {
			errno = 0;
			log_error(LOG_ARGS, "An unsubconf-nomail request was"
				" sent to a closed list. Ignoring mail");
			return -1;
		}
		conffilename = concatstr(3, listdir, "/unsubconf/", param);
		free(param);
		if((tmpfd = open(conffilename, O_RDONLY)) < 0) {
			/* invalid COOKIE */
			errno = 0;
			log_error(LOG_ARGS, "An unsubconf-nomail request was"
				" sent with a mismatching cookie."
				" Ignoring mail");
			return -1;
		}
		tmpstr = mygetline(tmpfd);
		close(tmpfd);
		chomp(tmpstr);
		unlink(conffilename);
		log_oper(listfd, OPLOGFNAME, "mlmmj-unsub: %s confirmed"
			" unsubscribe from nomail", tmpstr);
		exec_or_die(mlmmjunsub, "-L", listdir, "-a", tmpstr, "-n",
		    "-R", "-c", NULL);
		break;

	/* listname+unsubconf-COOKIE@domain.tld */
	case CTRL_CONFUNSUB:
		if (closedlist) {
			errno = 0;
			log_error(LOG_ARGS, "An unsubconf request was"
				" sent to a closed list. Ignoring mail");
			return -1;
		}
		conffilename = concatstr(3, listdir, "/unsubconf/", param);
		free(param);
		if((tmpfd = open(conffilename, O_RDONLY)) < 0) {
			/* invalid COOKIE */
			errno = 0;
			log_error(LOG_ARGS, "An unsubconf request was"
				" sent with a mismatching cookie."
				" Ignoring mail");
			return -1;
		}
		tmpstr = mygetline(tmpfd);
		close(tmpfd);
		chomp(tmpstr);
		unlink(conffilename);
		log_oper(listfd, OPLOGFNAME, "mlmmj-unsub: %s confirmed"
			" unsubscribe from regular list", tmpstr);
		exec_or_die(mlmmjunsub, "-L", listdir, "-a", tmpstr, "-R", "-c",
		    NULL);
		break;

	/* listname+bounces-INDEX-user=example.tld@domain.tld */
	case CTRL_BOUNCES:
		listfd = open(listdir, O_DIRECTORY);
		bouncenr = param;
		c = strchr(param, '-');
		if (!c) { /* Exec with dsn parsing, since the addr is missing */
			c = dsnparseaddr(mailname);
			if (c == NULL)
				exit(EXIT_SUCCESS);
			char *tmp = strrchr(c, '@');
			MY_ASSERT(tmp);
			*tmp = '=';
		} else {
			*c++ = '\0';
		}
		bret = bouncemail(listfd, lowercase(c), bouncenr);
		if (bret == BOUNCE_DONE)
			save_lastbouncedmsg(listfd, c, mailname);
		if (bouncemail(listfd, c, bouncenr) == BOUNCE_FAIL)
			exit(EXIT_FAILURE);
		if (bret == BOUNCE_OK)
			unlink(mailname);
		unlink(mailname);
		exit(bret == BOUNCE_FAIL ? EXIT_FAILURE : EXIT_SUCCESS);
		break;

	/* listname+release-COOKIE@domain.tld */
	case CTRL_RELEASE:
	/* DEPRECATED: listname+moderate-COOKIE@domain.tld */
	/* DEPRECATED: listname+moderate-subscribeCOOKIE@domain.tld */
	case CTRL_MODERATE:
		/* Subscriber moderation; DEPRECATED */
		if(strncmp(param, "subscribe", 9) == 0) {
			tmpstr = xstrdup(param + 9);
			free(param);
			param = tmpstr;
			goto permit;
		}

		moderatefilename = concatstr(3, listdir, "/moderation/", param);
		if(stat(moderatefilename, &stbuf) < 0) {
			free(moderatefilename);
			/* no mail to moderate */
			errno = 0;
			log_error(LOG_ARGS, "A release request was"
				" sent with a mismatching cookie."
				" Ignoring mail");
			return -1;
		}

		sendfilename = concatstr(2, moderatefilename, ".sending");
		/* Rename it to avoid mail being sent twice */
		if(rename(moderatefilename, sendfilename) < 0) {
			log_error(LOG_ARGS, "Could not rename to .sending");
			exit(EXIT_FAILURE);
		}

		omitfilename = concatstr(2, moderatefilename, ".omit");
		if(stat(omitfilename, &stbuf) == 0) {
			tmpfd = open(omitfilename, O_RDONLY);
			if(tmpfd < 0) {
				log_error(LOG_ARGS, "Could not open %s",
						omitfilename);
			} else {
				omit = mygetline(tmpfd);
				close(tmpfd);
				chomp(omit);
			}
			unlink(omitfilename);
			free(omitfilename);
		}

		free(moderatefilename);

		log_oper(listfd, OPLOGFNAME, "%s released %s",
				tll_front(*fromemails), param);

		if (omit != NULL)
			exec_or_die(mlmmjsend, "-L", listdir, "-o", omit,
			    "-m", sendfilename, NULL);
		exec_or_die(mlmmjsend, "-L", listdir, "-m", sendfilename, NULL);
		break;

	/* listname+reject-COOKIE@domain.tld */
	case CTRL_REJECT:
		moderatefilename = concatstr(3, listdir, "/moderation/", param);
		if(stat(moderatefilename, &stbuf) < 0) {
			free(moderatefilename);
			/* no mail to moderate */
			errno = 0;
			log_error(LOG_ARGS, "A reject request was"
				" sent with a mismatching cookie."
				" Ignoring mail");
			return -1;
		}
		log_oper(listfd, OPLOGFNAME, "%s rejected %s",
			tll_front(*fromemails), param);
		free(param);
		if (unlink(moderatefilename) != 0) {
			log_error(LOG_ARGS, "Could not unlink %s",
					moderatefilename);
			free(moderatefilename);
			exit(EXIT_FAILURE);
		}
		free(moderatefilename);
		break;

	/* listname+permit-COOKIE@domain.tld */
	case CTRL_PERMIT:
permit:
		gatekeepfilename = concatstr(3, listdir,
				"/moderation/subscribe", param);
		if(stat(gatekeepfilename, &stbuf) < 0) {
			free(gatekeepfilename);
			/* no mail to moderate */
			errno = 0;
			log_error(LOG_ARGS, "A permit request was"
				" sent with a mismatching cookie."
				" Ignoring mail");
			return -1;
		}
		log_oper(listfd, OPLOGFNAME, "%s permitted %s",
			tll_front(*fromemails), param);
		exec_or_die(mlmmjsub, "-L", listdir, "-m", param, "-c", NULL);
		break;

	/* listname+obstruct-COOKIE@domain.tld */
	case CTRL_OBSTRUCT:
		gatekeepfilename = concatstr(3, listdir,
				"/moderation/subscribe", param);
		if(stat(gatekeepfilename, &stbuf) < 0) {
			free(gatekeepfilename);
			/* no mail to moderate */
			errno = 0;
			log_error(LOG_ARGS, "An obstruct request was"
				" sent with a mismatching cookie."
				" Ignoring mail");
			return -1;
		}
		log_oper(listfd, OPLOGFNAME, "%s obstructed %s",
			tll_front(*fromemails), param);
		free(param);
		if (unlink(gatekeepfilename) != 0) {
			log_error(LOG_ARGS, "Could not unlink %s",
					gatekeepfilename);
			free(gatekeepfilename);
			exit(EXIT_FAILURE);
		}
		free(gatekeepfilename);
		break;

	/* listname+help@domain.tld */
	case CTRL_HELP:
		if(!strchr(tll_front(*fromemails), '@')) {
			/* Not a valid From: address */
			errno = 0;
			log_error(LOG_ARGS, "A help request was"
				" sent with an invalid From: header."
				" Ignoring mail");
			return -1;
		}
		log_oper(listfd, OPLOGFNAME, "%s requested help",
				tll_front(*fromemails));
		txt = open_text(listdir, "help", NULL, NULL, NULL, "listhelp");
		MY_ASSERT(txt);
		queuefilename = prepstdreply(txt, listdir,
				"$listowner$", tll_front(*fromemails), NULL, listfd, ctrlfd);
		MY_ASSERT(queuefilename);
		close_text(txt);
		send_help(listdir, queuefilename,
				tll_front(*fromemails), mlmmjsend, ctrlfd);
		break;

       /* listname+faq@domain.tld */
        case CTRL_FAQ:
               if(!strchr(tll_front(*fromemails), '@')) {
                       /* Not a valid From: address */
                       errno = 0;
                       log_error(LOG_ARGS, "A faq request was"
                               " sent with an invalid From: header."
                               " Ignoring mail");
                       return -1;
               }
               log_oper(listfd, OPLOGFNAME, "%s requested faq",
                               tll_front(*fromemails));
		txt = open_text(listdir, "faq", NULL, NULL, NULL, "listfaq");
		MY_ASSERT(txt);
		queuefilename = prepstdreply(txt, listdir,
				"$listowner$", tll_front(*fromemails), NULL, listfd, ctrlfd);
		MY_ASSERT(queuefilename);
		close_text(txt);
		send_help(listdir, queuefilename,
				tll_front(*fromemails), mlmmjsend, ctrlfd);
               break;

	/* listname+get-INDEX@domain.tld */
	case CTRL_GET:
		noget = statctrl(ctrlfd, "noget");
		if(noget) {
			errno = 0;
			log_error(LOG_ARGS, "A get request was sent to a list"
				" with the noget option set. Ignoring mail");
			return -1;
		}
		subonlyget = statctrl(ctrlfd, "subonlyget");
		if(subonlyget) {
			if(is_subbed(listfd, tll_front(*fromemails), 0) ==
					SUB_NONE) {
				errno = 0;
				log_error(LOG_ARGS, "A get request was sent"
					" from a non-subscribed address to a"
					" list with the subonlyget option set."
					" Ignoring mail");
				return -1;
			}
		}
		/* sanity check--is it all digits? */
		for(c = param; *c != '\0'; c++) {
			if(!isdigit((int)*c)) {
				errno = 0;
				log_error(LOG_ARGS, "The get request contained"
					" non-digits in index. Ignoring mail");
				return -1;
			}
		}
		archivefilename = concatstr(3, listdir, "/archive/",
						param);
		if(stat(archivefilename, &stbuf) < 0) {
			log_error(LOG_ARGS, "Unable to open archive file");
			exit(EXIT_FAILURE);
		}
		log_oper(listfd, OPLOGFNAME, "%s got archive/%s",
				tll_front(*fromemails), archivefilename);
		exec_or_die(mlmmjsend, "-T", tll_front(*fromemails), "-L",
		    listdir, "-l", "6", "-m", archivefilename, "-a", "-D",
		    NULL);
		break;

	/* listname+list@domain.tld */
	case CTRL_LIST:
		if(statctrl(ctrlfd, "nolistsubsemail"))
			return -1;
		const char *owner = NULL;
		owners = ctrlvalues(ctrlfd, "owner");
		if (owners != NULL) {
			tll_foreach(*owners, it) {
				if(strcasecmp(tll_front(*fromemails),
							it->item) == 0) {
					log_oper(listfd, OPLOGFNAME,
							"%s requested sub list",
					tll_front(*fromemails));
					owner = it->item;
					break;
				}
			}
		}
		if (owner != NULL) {
			errno = 0;
			log_error(LOG_ARGS, "A list request was sent to the"
				" list from a non-owner address."
				" Ignoring mail");
			return -1;
		} else {
			send_list(listdir, owner, mlmmjsend, listfd, ctrlfd);
		}
		if (owners != NULL)
			tll_free_and_free(*owners, free);
		free(owners);
		break;

	/* listname+???@domain.tld */
	default:
		errno = 0;
		log_error(LOG_ARGS, "Unknown command \"%s\". Ignoring mail",
								controlstr);
		return -1;

	}

	return 0;
}
