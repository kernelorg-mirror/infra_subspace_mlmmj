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

enum ctrl_e {
	CTRL_SUBSCRIBE_DIGEST,
	CTRL_SUBSCRIBE_NOMAIL,
	CTRL_SUBSCRIBE_BOTH,
	CTRL_SUBSCRIBE,
	CTRL_CONFSUB_DIGEST,
	CTRL_CONFSUB_NOMAIL,
	CTRL_CONFSUB_BOTH,
	CTRL_CONFSUB,
	CTRL_UNSUBSCRIBE_DIGEST,
	CTRL_UNSUBSCRIBE_NOMAIL,
	CTRL_UNSUBSCRIBE,
	CTRL_CONFUNSUB_DIGEST,
	CTRL_CONFUNSUB_NOMAIL,
	CTRL_CONFUNSUB,
	CTRL_BOUNCES,
	CTRL_RELEASE,
	CTRL_REJECT,
	CTRL_PERMIT,
	CTRL_OBSTRUCT,
	CTRL_MODERATE,
	CTRL_HELP,
	CTRL_FAQ,
	CTRL_GET,
	CTRL_LIST,
	CTRL_END  /* end marker, must be last */
};

struct ctrl_command {
	char *command;
	unsigned int accepts_parameter;
};

/* Must match the enum. CAREFUL when using commands that are substrings
 * of other commands. In that case the longest one have to be listed
 * first to match correctly. */
static struct ctrl_command ctrl_commands[] = {
	{ "subscribe-digest",   0 },
	{ "subscribe-nomail",   0 },
	{ "subscribe-both",     0 },
	{ "subscribe",          0 },
	{ "confsub-digest",     1 },
	{ "confsub-nomail",     1 },
	{ "confsub-both",       1 },
	{ "confsub",            1 },
	{ "unsubscribe-digest", 0 },
	{ "unsubscribe-nomail", 0 },
	{ "unsubscribe",        0 },
	{ "confunsub-digest",   1 },
	{ "confunsub-nomail",   1 },
	{ "confunsub",          1 },
	{ "bounces",            1 },
	{ "release",            1 },
	{ "reject",             1 },
	{ "permit",             1 },
	{ "obstruct",           1 },
	{ "moderate",           1 },
	{ "help",               0 },
	{ "faq",                0 },
	{ "get",                1 },
	{ "list",               0 }
};


int listcontrol(struct email_container *fromemails, const char *listdir,
		const char *controlstr, const char *mlmmjsub,
		const char *mlmmjunsub, const char *mlmmjsend,
		const char *mlmmjbounce, const char *mailname)
{
	char *bouncenr, *tmpstr;
	char *param = NULL, *conffilename, *moderatefilename, *gatekeepfilename;
	char *omitfilename;
	char *omit = NULL;
	char *c, *archivefilename, *sendfilename;
	struct stat stbuf;
	bool closedlist, closedlistsub, nosubconfirm, subonlyget;
	int tmpfd, noget, i;
	size_t cmdlen;
	unsigned int ctrl;
	struct strlist *owners;
	int owner_idx;
	text *txt;
	char *queuefilename;
	const char *subtype = NULL;
	const char *subtypename = NULL;
	
	/* A closed list doesn't allow subscribtion and unsubscription */
	closedlist = statctrl(listdir, "closedlist");

	/* A closed list "sub" only dissallows subscription, not unsub. */
	closedlistsub = statctrl(listdir, "closedlistsub");

	nosubconfirm = statctrl(listdir, "nosubconfirm");
	
#if 0
	log_error(LOG_ARGS, "controlstr = [%s]\n", controlstr);
	log_error(LOG_ARGS, "fromemails->emaillist[0] = [%s]\n",
			fromemails->emaillist[0]);
#endif
	for (ctrl=0; ctrl<CTRL_END; ctrl++) {
		cmdlen = strlen(ctrl_commands[ctrl].command);
		if (strncmp(controlstr, ctrl_commands[ctrl].command, cmdlen)
				== 0) {

			if (ctrl_commands[ctrl].accepts_parameter) {
				if (controlstr[cmdlen] != '-') {
					errno = 0;
					log_error(LOG_ARGS, "Command \"%s\""
						" requires a parameter, but no"
						" parameter was given."
						" Ignoring mail",
						ctrl_commands[ctrl].command);
					return -1;
                                }
				param = xstrdup(controlstr + cmdlen + 1);
				MY_ASSERT(param);
				if (strchr(param, '/')) {
					errno = 0;
					log_error(LOG_ARGS, "Slash (/) in"
						" list control request,"
						" discarding mail");
					return -1;
				}

			} else {
				if (controlstr[cmdlen] != '\0') {
					errno = 0;
					log_error(LOG_ARGS, "Command \"%s\""
						" does not accept a parameter,"
						" but a parameter was given."
						" Ignoring mail",
						ctrl_commands[ctrl].command);
					return -1;
				}
				param = NULL;
			}

			break;

		}
	}
	
	/* Only allow mails with bad From: header to be bounce mails */
	if(fromemails->emailcount != 1 && ctrl != CTRL_BOUNCES) {
		errno = 0;
		log_error(LOG_ARGS, "Ignoring mail with invalid From: "
				"which was not a bounce: %d", fromemails->emailcount);
		return -1;
	}

	/* We only need the control mail when bouncing, to save bounced msg */
	if(ctrl != CTRL_BOUNCES)
		unlink(mailname);

	switch (ctrl) {

	/* listname+subscribe-digest@domain.tld */
	case CTRL_SUBSCRIBE_DIGEST:
		if (closedlist || closedlistsub) {
			errno = 0;
			log_error(LOG_ARGS, "A subscribe-digest request was"
				" sent to a closed list. Ignoring mail");
			return -1;
		}
		if (!strchr(fromemails->emaillist[0], '@')) {
			/* Not a valid From: address */
			errno = 0;
			log_error(LOG_ARGS, "A subscribe-digest request was"
				" sent with an invalid From: header."
				" Ignoring mail");
			return -1;
		}
		if (statctrl(listdir, "nodigestsub")) {
			errno = 0;
			log_error(LOG_ARGS, "A subscribe-digest request was"
				" denied");
			txt = open_text(listdir, "deny", "sub", "disabled",
					"digest", "sub-deny-digest");
			MY_ASSERT(txt);
			register_unformatted(txt, "subaddr",
					fromemails->emaillist[0]);
			queuefilename = prepstdreply(txt, listdir,
					"$listowner$",
					fromemails->emaillist[0], NULL);
			MY_ASSERT(queuefilename);
			close_text(txt);
			send_help(listdir, queuefilename,
					fromemails->emaillist[0], mlmmjsend);
			return -1;
		}
		log_oper(listdir, OPLOGFNAME, "mlmmj-sub: request for digest"
					" subscription from %s",
					fromemails->emaillist[0]);
		exec_or_die(mlmmjsub, "-L", listdir, "-a",
		    fromemails->emaillist[0], "-d", "-r", "-c",
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
		if (!strchr(fromemails->emaillist[0], '@')) {
			/* Not a valid From: address */
			errno = 0;
			log_error(LOG_ARGS, "A subscribe-nomail request was"
				" sent with an invalid From: header."
				" Ignoring mail");
			return -1;
		}
		if (statctrl(listdir, "nonomailsub")) {
			errno = 0;
			log_error(LOG_ARGS, "A subscribe-nomail request was"
				" denied");
			txt = open_text(listdir, "deny", "sub", "disabled",
					"nomail", "sub-deny-nomail");
			MY_ASSERT(txt);
			register_unformatted(txt, "subaddr",
					fromemails->emaillist[0]);
			queuefilename = prepstdreply(txt, listdir,
					"$listowner$",
					fromemails->emaillist[0], NULL);
			MY_ASSERT(queuefilename);
			close_text(txt);
			send_help(listdir, queuefilename,
					fromemails->emaillist[0], mlmmjsend);
			return -1;
		}
		log_oper(listdir, OPLOGFNAME, "mlmmj-sub: request for nomail"
					" subscription from %s",
					fromemails->emaillist[0]);
		exec_or_die(mlmmjsub, "-L", listdir, "-a",
		    fromemails->emaillist[0], "-n", "-r", "-c",
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
		if (!strchr(fromemails->emaillist[0], '@')) {
			/* Not a valid From: address */
			errno = 0;
			log_error(LOG_ARGS, "A subscribe-both request was"
				" sent with an invalid From: header."
				" Ignoring mail");
			return -1;
		}
		if (statctrl(listdir, "nodigestsub")) {
			errno = 0;
			log_error(LOG_ARGS, "A subscribe-both request was"
				" denied");
			txt = open_text(listdir, "deny", "sub", "disabled",
					"both", "sub-deny-digest");
			MY_ASSERT(txt);
			register_unformatted(txt, "subaddr",
					fromemails->emaillist[0]);
			queuefilename = prepstdreply(txt, listdir,
					"$listowner$",
					fromemails->emaillist[0], NULL);
			MY_ASSERT(queuefilename);
			close_text(txt);
			send_help(listdir, queuefilename,
					fromemails->emaillist[0], mlmmjsend);
			return -1;
		}
		log_oper(listdir, OPLOGFNAME, "mlmmj-sub: request for both"
					" subscription from %s",
					fromemails->emaillist[0]);
		exec_or_die(mlmmjsub, "-L", listdir, "-a",
		    fromemails->emaillist[0], "-b", "-r", "-c",
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
		if (!strchr(fromemails->emaillist[0], '@')) {
			/* Not a valid From: address */
			errno = 0;
			log_error(LOG_ARGS, "A subscribe request was"
				" sent with an invalid From: header."
				" Ignoring mail");
			return -1;
		}
		log_oper(listdir, OPLOGFNAME, "mlmmj-sub: request for regular"
					" subscription from %s",
					fromemails->emaillist[0]);
		exec_or_die(mlmmjsub, "-L", listdir, "-a",
		    fromemails->emaillist[0], "-r", "-c",
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
		log_oper(listdir, OPLOGFNAME, "mlmmj-sub: %s confirmed"
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
		log_oper(listdir, OPLOGFNAME, "mlmmj-sub: %s confirmed"
					" subscription to regular list",
					tmpstr);
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
		if (!strchr(fromemails->emaillist[0], '@')) {
			/* Not a valid From: address */
			errno = 0;
			log_error(LOG_ARGS, "An unsubscribe request was"
				" sent with an invalid From: header."
				" Ignoring mail");
			return -1;
		}
		log_oper(listdir, OPLOGFNAME, "mlmmj-unsub: %s requests"
					" unsubscribe",
					fromemails->emaillist[0]);
		exec_or_die(mlmmjunsub, "-L", listdir, "-a",
		    fromemails->emaillist[0], "-r",
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
		log_oper(listdir, OPLOGFNAME, "mlmmj-unsub: %s confirmed"
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
		log_oper(listdir, OPLOGFNAME, "mlmmj-unsub: %s confirmed"
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
		log_oper(listdir, OPLOGFNAME, "mlmmj-unsub: %s confirmed"
					" unsubscribe from regular list",
					tmpstr);
		exec_or_die(mlmmjunsub, "-L", listdir, "-a", tmpstr, "-R", "-c",
		    NULL);
		break;

	/* listname+bounces-INDEX-user=example.tld@domain.tld */
	case CTRL_BOUNCES:
		bouncenr = param;
		c = strchr(param, '-');
		if (!c) { /* Exec with dsn parsing, since the addr is missing */
			exec_or_die(mlmmjbounce, "-L", listdir, "-m", mailname,
			    "-n", bouncenr, "-d", NULL);
		}
		*c++ = '\0';
		exec_or_die(mlmmjbounce, "-L", listdir, "-a", c, "-m", mailname,
		    "-n", bouncenr, NULL);
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

		log_oper(listdir, OPLOGFNAME, "%s released %s",
				fromemails->emaillist[0], param);

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
		log_oper(listdir, OPLOGFNAME, "%s rejected %s",
			fromemails->emaillist[0], param);
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
		log_oper(listdir, OPLOGFNAME, "%s permitted %s",
			fromemails->emaillist[0], param);
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
		log_oper(listdir, OPLOGFNAME, "%s obstructed %s",
			fromemails->emaillist[0], param);
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
		if(!strchr(fromemails->emaillist[0], '@')) {
			/* Not a valid From: address */
			errno = 0;
			log_error(LOG_ARGS, "A help request was"
				" sent with an invalid From: header."
				" Ignoring mail");
			return -1;
		}
		log_oper(listdir, OPLOGFNAME, "%s requested help",
				fromemails->emaillist[0]);
		txt = open_text(listdir, "help", NULL, NULL, NULL, "listhelp");
		MY_ASSERT(txt);
		queuefilename = prepstdreply(txt, listdir,
				"$listowner$", fromemails->emaillist[0], NULL);
		MY_ASSERT(queuefilename);
		close_text(txt);
		send_help(listdir, queuefilename,
				fromemails->emaillist[0], mlmmjsend);
		break;

       /* listname+faq@domain.tld */
        case CTRL_FAQ:
               if(!strchr(fromemails->emaillist[0], '@')) {
                       /* Not a valid From: address */
                       errno = 0;
                       log_error(LOG_ARGS, "A faq request was"
                               " sent with an invalid From: header."
                               " Ignoring mail");
                       return -1;
               }
               log_oper(listdir, OPLOGFNAME, "%s requested faq",
                               fromemails->emaillist[0]);
		txt = open_text(listdir, "faq", NULL, NULL, NULL, "listfaq");
		MY_ASSERT(txt);
		queuefilename = prepstdreply(txt, listdir,
				"$listowner$", fromemails->emaillist[0], NULL);
		MY_ASSERT(queuefilename);
		close_text(txt);
		send_help(listdir, queuefilename,
				fromemails->emaillist[0], mlmmjsend);
               break;

	/* listname+get-INDEX@domain.tld */
	case CTRL_GET:
		noget = statctrl(listdir, "noget");
		if(noget) {
			errno = 0;
			log_error(LOG_ARGS, "A get request was sent to a list"
				" with the noget option set. Ignoring mail");
			return -1;
		}
		subonlyget = statctrl(listdir, "subonlyget");
		if(subonlyget) {
			if(is_subbed(listdir, fromemails->emaillist[0], 0) ==
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
		log_oper(listdir, OPLOGFNAME, "%s got archive/%s",
				fromemails->emaillist[0], archivefilename);
		exec_or_die(mlmmjsend, "-T", fromemails->emaillist[0], "-L",
		    listdir, "-l", "6", "-m", archivefilename, "-a", "-D",
		    NULL);
		break;

	/* listname+list@domain.tld */
	case CTRL_LIST:
		if(statctrl(listdir, "nolistsubsemail"))
			return -1;
		owner_idx = -1;
		owners = ctrlvalues(listdir, "owner");
		if (owners != NULL) {
			for(i = 0; i < owners->count; i++) {
				if(strcasecmp(fromemails->emaillist[0],
							owners->strs[i]) == 0) {
					log_oper(listdir, OPLOGFNAME,
							"%s requested sub list",
					fromemails->emaillist[0]);
					owner_idx = i;
					break;
				}
			}
		}
		if (owner_idx == -1) {
			errno = 0;
			log_error(LOG_ARGS, "A list request was sent to the"
				" list from a non-owner address."
				" Ignoring mail");
			return -1;
		} else {
			send_list(listdir, owners->strs[owner_idx], mlmmjsend);
		}
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
