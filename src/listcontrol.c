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

#include <sys/param.h>
#include <sys/types.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>

#include "xmalloc.h"
#include "mlmmj.h"
#include "listcontrol.h"
#include "prepstdreply.h"
#include "send_help.h"
#include "send_list.h"
#include "send_mail.h"
#include "log_error.h"
#include "statctrl.h"
#include "chomp.h"
#include "log_oper.h"
#include "ctrlvalues.h"
#include "ctrlvalue.h"
#include "subscriberfuncs.h"
#include "utils.h"


/* Must match the enum. CAREFUL when using commands that are substrings
 * of other commands. In that case the longest one have to be listed
 * first to match correctly. */

/* A closed list doesn't allow subscribtion and unsubscription
   A closed list "sub" only dissallows subscription, not unsub. */

static struct ctrl_command ctrl_commands[] = {
	{ "subscribe-digest",   false, false, false, CTRL_SUBSCRIBE_DIGEST },
	{ "subscribe-nomail",   false, false, false, CTRL_SUBSCRIBE_NOMAIL },
	{ "subscribe-both",     false, false, false, CTRL_SUBSCRIBE_BOTH },
	{ "subscribe",          false, false, false, CTRL_SUBSCRIBE },
	{ "confsub-digest",     true , false, true , CTRL_CONFSUB_DIGEST },
	{ "confsub-nomail",     true , false, true , CTRL_CONFSUB_NOMAIL },
	{ "confsub-both",       true , false, true , CTRL_CONFSUB_BOTH },
	{ "confsub",            true , false, true , CTRL_CONFSUB },
	{ "unsubscribe-digest", false, false, true , CTRL_UNSUBSCRIBE_DIGEST },
	{ "unsubscribe-nomail", false, false, true , CTRL_UNSUBSCRIBE_NOMAIL },
	{ "unsubscribe",        false, false, true , CTRL_UNSUBSCRIBE },
	{ "confunsub-digest",   true , false, true , CTRL_CONFUNSUB_DIGEST },
	{ "confunsub-nomail",   true , false, true , CTRL_CONFUNSUB_NOMAIL },
	{ "confunsub",          true , false, true , CTRL_CONFUNSUB },
	{ "bounces",            true , true , true , CTRL_BOUNCES },
	{ "release",            true , true , true , CTRL_RELEASE },
	{ "reject",             true , true , true , CTRL_REJECT },
	{ "permit",             true , true , true , CTRL_PERMIT },
	{ "obstruct",           true , true , true , CTRL_OBSTRUCT },
	{ "moderate",           true , true , true , CTRL_MODERATE },
	{ "help",               false, true , true , CTRL_HELP },
	{ "faq",                false, true , true , CTRL_FAQ },
	{ "get",                true , true , true , CTRL_GET },
	{ "list",               false, true , true , CTRL_LIST }
};

struct listcontrol {
	const char *fromemail;
	const char *mlmmjsend;
	bool subconfirm;
};

struct sub {
	const char *typereq;
	const char *ctrlstr;
	const char *type;
	const char *typedeny;
	enum subtype typesub;
};

static bool
is_valid_email(const char *email, const char *log)
{
	if (strchr(email, '@') == NULL) {
		errno = 0;
		log_error(LOG_ARGS, "%s request was "
			    " sent with an invalid From: header."
			    " Ignoring mail", log);
		return (false);
	}
	return (true);
}

static void
lc_sub(struct listcontrol *lc, struct ml *ml, struct sub *sub)
{
	if (!is_valid_email(lc->fromemail, sub->typereq))
		exit(EXIT_FAILURE);

	if (sub->ctrlstr != NULL && statctrl(ml->ctrlfd, sub->ctrlstr)) {
		char *queuefilename;
		text *txt;
		errno = 0;
		log_error(LOG_ARGS, "%s request was denied", sub->typereq);
		txt = open_text(ml->fd, "deny", "sub", "disabled",
		    sub->type, sub->typedeny);
		MY_ASSERT(txt);
		register_default_unformatted(txt, ml);
		register_unformatted(txt, "subaddr", lc->fromemail);
		queuefilename = prepstdreply(txt, ml, "$listowner$",
		    lc->fromemail, NULL);
		MY_ASSERT(queuefilename);
		close_text(txt);
		send_help(ml, queuefilename, lc->fromemail);
	}
	log_oper(ml->fd, OPLOGFNAME, "mlmmj-sub: request for %s"
	    " subscription from %s", sub->type, lc->fromemail);
	struct subscription subc = { 0 };
	subc.subconfirm = lc->subconfirm;
	subc.send_welcome_email = true;
	subc.reasonsub = SUB_REQUEST;
	subc.typesub = sub->typesub;
	subc.gensubscribed = true;
	exit(do_subscribe(ml, &subc, lc->fromemail) ? EXIT_SUCCESS : EXIT_FAILURE);
}

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

int listcontrol(strlist *fromemails, struct ml *ml, const char *controlstr,
    const char *mlmmjsend, const char *mailname)
{
	char *bouncenr, *tmpstr;
	char *param = NULL, *moderatefilename, *gatekeepfilename;
	char *omitfilename;
	char *omit = NULL;
	char *c, *archivefilename, *sendfilename, *tosend;
	text *txt;
	char *queuefilename;
	enum subtype ts = SUB_NONE;
	const char *subtypename = NULL;
	bounce_t bret;
	struct ctrl_command *cc;
	struct listcontrol lc = { 0 };
	struct sub sub = { 0 };
	struct subscription subc = { 0 };

	lc.subconfirm = !statctrl(ml->ctrlfd, "nosubconfirm");

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
	
	if (tll_length(*fromemails) >= 1)
		lc.fromemail = tll_front(*fromemails);
	lc.mlmmjsend = mlmmjsend;

	/* We only need the control mail when bouncing, to save bounced msg */
	if (cc->type != CTRL_BOUNCES)
		unlink(mailname);

	if (statctrl(ml->ctrlfd, "closedlist") && !cc->valid_when_closed_list) {
		errno = 0;
		log_error(LOG_ARGS, "A %s request was sent to a closed list. "
		    "Ignoring mail", cc->command);
		return -1;
	}

	if (statctrl(ml->ctrlfd, "closedlistsub") && !cc->valid_when_closed_sub) {
		errno = 0;
		log_error(LOG_ARGS, "A %s request was sent to a subscription "
		    "closed list. Ignoring mail", cc->command);
		return -1;
	}
	switch (cc->type) {

	/* listname+subscribe-digest@domain.tld */
	case CTRL_SUBSCRIBE_DIGEST:
		sub.typereq = "A subscribe-digest";
		sub.ctrlstr = "nodigestsub";
		sub.type = "digest";
		sub.typesub = SUB_DIGEST;
		sub.typedeny = "sub-deny-digest";
		__attribute__ ((fallthrough));
	/* listname+subscribe-nomail@domain.tld */
	case CTRL_SUBSCRIBE_NOMAIL:
		if (sub.typereq == NULL) {
			sub.typereq = "A subscribe-nomail";
			sub.ctrlstr = "nomailsub";
			sub.typesub = SUB_NOMAIL;
			sub.type = "nomail";
			sub.typedeny = "sub-deny-nomail";
		}
		__attribute__ ((fallthrough));
	/* listname+subscribe-both@domain.tld */
	case CTRL_SUBSCRIBE_BOTH:
		if (sub.typereq == NULL) {
			sub.typereq = "A subscribe-both";
			sub.ctrlstr = "nodigestsub";
			sub.typesub = SUB_BOTH;
			sub.type = "both";
			sub.typedeny = "sub-deny-digest";
		}
		__attribute__ ((fallthrough));
	/* listname+subscribe@domain.tld */
	case CTRL_SUBSCRIBE:
		if (sub.typereq == NULL) {
			sub.typereq = "A subcribe";
			sub.ctrlstr = NULL;
			sub.type = "regular";
			sub.typesub = SUB_NORMAL;
		}
		lc_sub(&lc, ml, &sub);
		break;

	/* listname+subconf-digest-COOKIE@domain.tld */
	case CTRL_CONFSUB_DIGEST:
		subc.typesub = SUB_DIGEST;
		subtypename = "digest";
		__attribute__ ((fallthrough));
	/* listname+subconf-nomail-COOKIE@domain.tld */
	case CTRL_CONFSUB_NOMAIL:
		if (subtypename == NULL) {
			subc.typesub = SUB_NOMAIL;
			subtypename = "nomail";
		}
		__attribute__ ((fallthrough));
	/* listname+subconf-both-COOKIE@domain.tld */
	case CTRL_CONFSUB_BOTH:
		if (subtypename == NULL) {
			subc.typesub = SUB_BOTH;
			subtypename = "both";
		}
		__attribute__ ((fallthrough));
	/* listname+subconf-COOKIE@domain.tld */
	case CTRL_CONFSUB:
		if (subtypename == NULL) {
			subc.typesub = SUB_NORMAL;
			subtypename = "regular list";
		}
		tmpstr = get_subcookie_content(ml->fd, false, param);
		if (tmpstr == NULL) {
			/* invalid COOKIE */
			errno = 0;
			log_error(LOG_ARGS, "A subconf request was"
				" sent with a mismatching cookie."
				" Ignoring mail");
			return -1;
		}
		subc.send_welcome_email = true;
		subc.reasonsub = SUB_CONFIRM;
		subc.gensubscribed = true;
		subc.mlmmjsend = lc.mlmmjsend;
		log_oper(ml->fd, OPLOGFNAME, "mlmmj-sub: %s confirmed"
			" subscription to %s", tmpstr, subtypename);
		exit(do_subscribe(ml, &subc, tmpstr) ? EXIT_SUCCESS : EXIT_FAILURE);

	/* DEPRECATED: listname+unsubscribe-digest@domain.tld */
	case CTRL_UNSUBSCRIBE_DIGEST:
	/* DEPRECATED: listname+unsubscribe-nomail@domain.tld */
	case CTRL_UNSUBSCRIBE_NOMAIL:
	/* listname+unsubscribe@domain.tld */
	case CTRL_UNSUBSCRIBE:
		if (!is_valid_email(lc.fromemail, "An unsubscribe"))
			return -1;
		log_oper(ml->fd, OPLOGFNAME, "mlmmj-unsub: %s requests"
			" unsubscribe", lc.fromemail);
		do_unsubscribe(ml, lc.fromemail, SUB_ALL, SUB_REQUEST,
		    false, lc.subconfirm, false, lc.subconfirm);
		exit(EXIT_SUCCESS);
		break;

	/* listname+unsubconf-digest-COOKIE@domain.tld */
	case CTRL_CONFUNSUB_DIGEST:
		ts = SUB_DIGEST;
		subtypename = "-digest";
		__attribute__ ((fallthrough));
	/* listname+unsubconf-nomail-COOKIE@domain.tld */
	case CTRL_CONFUNSUB_NOMAIL:
		if (ts == SUB_NONE) {
			ts = SUB_NOMAIL;
			subtypename = "-nomail";
		}
		__attribute__ ((fallthrough));
	/* listname+unsubconf-COOKIE@domain.tld */
	case CTRL_CONFUNSUB:
		if (ts == SUB_NONE) {
			ts = SUB_ALL;
			subtypename = "";
		}
		tmpstr = get_subcookie_content(ml->fd, true, param);
		if (tmpstr == NULL) {
			/* invalid COOKIE */
			errno = 0;
			log_error(LOG_ARGS, "An unsubconf%s request was"
				" sent with a mismatching cookie."
				" Ignoring mail", subtypename);
			return -1;
		}
		do_unsubscribe(ml, tmpstr, ts, SUB_CONFIRM,
		    false, false, false, true);
		exit(EXIT_SUCCESS);
		break;

	/* listname+bounces-INDEX-user=example.tld@domain.tld */
	case CTRL_BOUNCES:
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
		bret = bouncemail(ml->fd, lowercase(c), bouncenr);
		if (bret == BOUNCE_DONE)
			save_lastbouncedmsg(ml->fd, c, mailname);
		if (bouncemail(ml->fd, c, bouncenr) == BOUNCE_FAIL)
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

		xasprintf(&moderatefilename, "moderation/%s", param);
		xasprintf(&sendfilename, "%s.sending", moderatefilename);
		/* Rename it to avoid mail being sent twice */
		if(renameat(ml->fd, moderatefilename, ml->fd, sendfilename) != 0) {
			if (errno == ENOENT) {
				errno = 0;
				log_error(LOG_ARGS, "A release request was"
					" sent with a mismatching cookie."
					" Ignoring mail");
			} else {
				log_error(LOG_ARGS, "Could not rename to .sending");
			}
			free(sendfilename);
			free(moderatefilename);
			return (-1);
		}

		xasprintf(&omitfilename, "%s.omit", moderatefilename);
		errno = 0;
		omit = ctrlvalue(ml->fd, omitfilename);
		if (omit != NULL || errno != ENOENT)
			unlinkat(ml->fd, omitfilename, 0);
		free(omitfilename);
		free(moderatefilename);

		log_oper(ml->fd, OPLOGFNAME, "%s released %s", lc.fromemail, param);

		xasprintf(&tosend, "%s/%s", ml->dir, sendfilename);
		if (omit != NULL)
			exec_or_die(mlmmjsend, "-L", ml->dir, "-o", omit,
			    "-m", tosend, NULL);
		exec_or_die(mlmmjsend, "-L", ml->dir, "-m", tosend, NULL);
		break;

	/* listname+reject-COOKIE@domain.tld */
	case CTRL_REJECT:
		xasprintf(&moderatefilename, "moderation/%s", param);
		if (unlinkat(ml->fd, moderatefilename, 0) != 0) {
			if (errno == ENOENT) {
				errno = 0;
				log_error(LOG_ARGS, "A reject request was"
					" sent with a mismatching cookie."
					" Ignoring mail");
			} else {
				log_error(LOG_ARGS, "Could not unlink %s",
				    moderatefilename);
			}
			free(moderatefilename);
			return -1;
		}
		log_oper(ml->fd, OPLOGFNAME, "%s rejected %s", lc.fromemail, param);
		free(moderatefilename);
		xasprintf(&moderatefilename, "moderation/%s.omit", param);
		unlinkat(ml->fd, moderatefilename, 0);
		free(moderatefilename);
		free(param);
		break;

	/* listname+permit-COOKIE@domain.tld */
	case CTRL_PERMIT:
permit:
		xasprintf(&gatekeepfilename, "moderation/subscribe%s", param);
		if (faccessat(ml->fd, gatekeepfilename, F_OK, 0) < 0) {
			free(gatekeepfilename);
			/* no mail to moderate */
			errno = 0;
			log_error(LOG_ARGS, "A permit request was"
				" sent with a mismatching cookie."
				" Ignoring mail");
			return -1;
		}
		log_oper(ml->fd, OPLOGFNAME, "%s permitted %s",
			lc.fromemail, param);
		subc.modstr = param;
		subc.send_welcome_email = true;
		subc.gensubscribed = true;
		subc.reasonsub = SUB_PERMIT;
		subc.mlmmjsend = lc.mlmmjsend;
		exit(do_subscribe(ml, &subc, tmpstr) ? EXIT_SUCCESS : EXIT_FAILURE);
		break;

	/* listname+obstruct-COOKIE@domain.tld */
	case CTRL_OBSTRUCT:
		xasprintf(&gatekeepfilename, "moderation/subscribe%s", param);
		if (unlinkat(ml->fd, gatekeepfilename, 0) != 0) {
			if (errno == ENOENT) {
				errno = 0;
				log_error(LOG_ARGS, "An obstruct request was"
					" sent with a mismatching cookie."
					" Ignoring mail");
			} else {
				log_error(LOG_ARGS, "Could not unlink %s/%s",
				    ml->dir, gatekeepfilename);
			}
			free(param);
			free(gatekeepfilename);
			return -1;
		}
		log_oper(ml->fd, OPLOGFNAME, "%s obstructed %s",
			tll_front(*fromemails), param);
		free(param);
		free(gatekeepfilename);
		break;

	/* listname+help@domain.tld */
	case CTRL_HELP:
		if (!is_valid_email(tll_front(*fromemails), "A help"))
			return -1;
		log_oper(ml->fd, OPLOGFNAME, "%s requested help",
				tll_front(*fromemails));
		txt = open_text(ml->fd, "help", NULL, NULL, NULL, "listhelp");
		MY_ASSERT(txt);
		register_default_unformatted(txt, ml);
		queuefilename = prepstdreply(txt, ml,
				"$listowner$", tll_front(*fromemails), NULL);
		MY_ASSERT(queuefilename);
		close_text(txt);
		send_help(ml, queuefilename, tll_front(*fromemails));
		break;

	/* listname+faq@domain.tld */
	case CTRL_FAQ:
		if (!is_valid_email(tll_front(*fromemails), "A faq"))
			return -1;
		log_oper(ml->fd, OPLOGFNAME, "%s requested faq",
		    tll_front(*fromemails));
		txt = open_text(ml->fd, "faq", NULL, NULL, NULL, "listfaq");
		MY_ASSERT(txt);
		register_default_unformatted(txt, ml);
		queuefilename = prepstdreply(txt, ml,
				"$listowner$", tll_front(*fromemails), NULL);
		MY_ASSERT(queuefilename);
		close_text(txt);
		send_help(ml, queuefilename, tll_front(*fromemails));
		break;

	/* listname+get-INDEX@domain.tld */
	case CTRL_GET:
		if (statctrl(ml->ctrlfd, "noget")) {
			errno = 0;
			log_error(LOG_ARGS, "A get request was sent to a list"
				" with the noget option set. Ignoring mail");
			return -1;
		}
		if (statctrl(ml->ctrlfd, "subonlyget")) {
			if(is_subbed(ml->fd, tll_front(*fromemails), 0) ==
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
		if (param[strspn(param, "0123456789")] != '\0') {
			errno = 0;
			log_error(LOG_ARGS, "The get request contained"
			    " non-digits in index. Ignoring mail");
			return -1;
		}
		int index = strtoim(param, 0, INT_MAX, NULL);
		xasprintf(&archivefilename, "archive/%d", index);
		int fd = openat(ml->fd, archivefilename, O_RDONLY|O_CLOEXEC);
		if (fd == -1) {
			log_error(LOG_ARGS, "Unable to open archive file");
			free(archivefilename);
			return -1;
		}
		struct mail mail = { 0 };
		mail.to = tll_front(*fromemails);
		mail.from = get_bounce_from_adr(mail.to, ml, index);
		mail.fp = fdopen(fd, "r");
		log_oper(ml->fd, OPLOGFNAME, "%s got archive/%s",
				tll_front(*fromemails), param);
		if (!send_single_mail(&mail, ml, true)) {
			/* TODO: save to queue */
			return (-1);
		}
		break;

	/* listname+list@domain.tld */
	case CTRL_LIST:
		if(statctrl(ml->ctrlfd, "nolistsubsemail"))
			return -1;
		const char *owner = tll_front(*fromemails);
		if (!ctrlvalues_contains(ml->ctrlfd, "owner", owner, false)) {
			errno = 0;
			log_error(LOG_ARGS, "A list request was sent to the"
				" list from a non-owner address."
				" Ignoring mail");
			return -1;
		}
		send_list(ml, owner);
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
