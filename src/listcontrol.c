/*
 * Copyright (C) 2003 Mads Martin Joergensen <mmj at mmj.dk>
 * Copyright (C) 2023-2025 Baptiste Daroussin <bapt@FreeBSD.org>
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
#include <sys/stat.h>
#include <sys/types.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <ctype.h>
#include <dirent.h>
#include <fcntl.h>

#include "xmalloc.h"
#include "mlmmj.h"
#include "archive.h"
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
	{ "subrelease",         true , true , true , CTRL_SUBRELEASE },
	{ "digestsubrelease",   true , true , true , CTRL_DIGESTSUBRELEASE },
	{ "nomailsubrelease",   true , true , true , CTRL_NOMAILSUBRELEASE },
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
	for (const char *p = email; *p != '\0'; p++) {
		if ((unsigned char)*p < 0x20) {
			errno = 0;
			log_error(LOG_ARGS, "%s request has "
			    "control character in From: header."
			    " Ignoring mail", log);
			return (false);
		}
	}
	return (true);
}

__dead2 static void
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

/*
 * Prepare a moderated message for release: rename it to .sending so that
 * it cannot be sent twice, and drop any stale .omit file. On success
 * *sendfnp and *omitp are set and must be freed by the caller, on failure
 * -1 is returned with errno set.
 */
static int
prepare_moderated_release(struct ml *ml, const char *param,
    char **sendfnp, char **omitp)
{
	char *modfn, *sendfn, *omitfn, *omit;
	int err;

	xasprintf(&modfn, "moderation/%s", param);
	xasprintf(&sendfn, "%s.sending", modfn);
	if (renameat(ml->fd, modfn, ml->fd, sendfn) != 0) {
		err = errno;
		free(sendfn);
		free(modfn);
		errno = err;
		return (-1);
	}

	xasprintf(&omitfn, "%s.omit", modfn);
	errno = 0;
	omit = ctrlvalue(ml->fd, omitfn);
	if (omit != NULL || errno != ENOENT)
		unlinkat(ml->fd, omitfn, 0);
	free(omitfn);
	free(modfn);

	*sendfnp = sendfn;
	*omitp = omit;
	return (0);
}

/*
 * Send a prepared moderated message through mlmmj-send. When wait is
 * false the current process is replaced and this function never returns.
 */
static void
send_moderated_release(struct ml *ml, const char *mlmmjsend,
    const char *sendfn, const char *omit, bool wait)
{
	char *tosend;

	xasprintf(&tosend, "%s/%s", ml->dir, sendfn);
	if (wait) {
		if (omit != NULL)
			exec_and_wait(mlmmjsend, "-L", ml->dir, "-o", omit,
			    "-m", tosend, NULL);
		else
			exec_and_wait(mlmmjsend, "-L", ml->dir, "-m", tosend,
			    NULL);
		free(tosend);
	} else {
		if (omit != NULL)
			exec_or_die(mlmmjsend, "-L", ml->dir, "-o", omit,
			    "-m", tosend, NULL);
		else
			exec_or_die(mlmmjsend, "-L", ml->dir, "-m", tosend,
			    NULL);
	}
}

/*
 * A moderated message queued for release by the subrelease flow.
 */
struct pending_release {
	char *sendfn;
	char *omit;
	struct timespec mtime;
};

typedef vec_t(struct pending_release) pending_releases_t;

static int
cmp_pending_release(const void *a, const void *b)
{
	const struct pending_release *pa = a;
	const struct pending_release *pb = b;

	if (pa->mtime.tv_sec != pb->mtime.tv_sec)
		return (pa->mtime.tv_sec < pb->mtime.tv_sec ? -1 : 1);
	if (pa->mtime.tv_nsec != pb->mtime.tv_nsec)
		return (pa->mtime.tv_nsec < pb->mtime.tv_nsec ? -1 : 1);
	return (strcmp(pa->sendfn, pb->sendfn));
}

/*
 * Queue every message held in moderation for the same sender, except the
 * one named by 'skipname' which the caller handles. Only plain moderated
 * mails are considered: .omit files, files already renamed to .sending and
 * the subscribe/ subdirectory are ignored.
 */
static void
collect_sender_moderation(struct ml *ml, const char *sender,
    const char *skipname, pending_releases_t *pend)
{
	struct pending_release pending;
	struct dirent *dp;
	struct stat st;
	DIR *dir;
	char *filesender;
	int modfd, mfd;

	modfd = openat(ml->fd, "moderation", O_RDONLY|O_DIRECTORY|O_CLOEXEC);
	if (modfd == -1)
		return;
	dir = fdopendir(modfd);
	if (dir == NULL) {
		close(modfd);
		return;
	}

	while ((dp = readdir(dir)) != NULL) {
		if (dp->d_name[0] == '.')
			continue;
		if (strcmp(dp->d_name, skipname) == 0)
			continue;
		if (strstr(dp->d_name, ".sending") != NULL ||
		    strstr(dp->d_name, ".omit") != NULL)
			continue;
		/* moderation/ also holds the subscribe/ subdirectory */
		if (fstatat(modfd, dp->d_name, &st, 0) != 0 ||
		    !S_ISREG(st.st_mode))
			continue;

		mfd = openat(modfd, dp->d_name, O_RDONLY|O_CLOEXEC);
		if (mfd == -1)
			continue;
		filesender = get_mail_sender(mfd);
		if (filesender == NULL)
			continue;
		if (strcasecmp(filesender, sender) != 0) {
			free(filesender);
			continue;
		}
		free(filesender);

		if (prepare_moderated_release(ml, dp->d_name,
		    &pending.sendfn, &pending.omit) != 0)
			continue;
		pending.mtime = st.st_mtim;
		vec_push(pend, pending);
	}

	closedir(dir);
}

/*
 * Release every message held in moderation for the same sender, oldest
 * first so that a batch of mail (such as a patch series) keeps the order in
 * which it was received. Called from the subrelease flow when
 * control/subreleaseall is set. 'trigger' is the message the caller is
 * releasing, already renamed to .sending, and 'triggeromit' its omit value.
 */
static void
release_sender_moderation(struct ml *ml, const char *mlmmjsend,
    const char *sender, const char *trigger, const char *triggeromit)
{
	struct pending_release pending;
	pending_releases_t pend = vec_init();
	const char *base;
	struct stat st;
	size_t i;

	base = strrchr(trigger, '/');
	base = (base != NULL) ? base + 1 : trigger;

	/* The message the caller is releasing is part of the batch */
	if (fstatat(ml->fd, trigger, &st, 0) == 0) {
		pending.sendfn = xstrdup(trigger);
		pending.omit = (triggeromit != NULL) ?
		    xstrdup(triggeromit) : NULL;
		pending.mtime = st.st_mtim;
		vec_push(&pend, pending);
	}

	collect_sender_moderation(ml, sender, base, &pend);

	if (pend.len > 1)
		qsort(pend.d, pend.len, sizeof(pend.d[0]), cmp_pending_release);

	for (i = 0; i < pend.len; i++) {
		log_oper(ml->fd, OPLOGFNAME, "subreleaseall: released %s"
		    " from %s", pend.d[i].sendfn, sender);
		send_moderated_release(ml, mlmmjsend, pend.d[i].sendfn,
		    pend.d[i].omit, true);
		free(pend.d[i].sendfn);
		free(pend.d[i].omit);
	}
	vec_free(&pend);
}

int listcontrol(strlist *fromemails, strlist *rtoemails, struct ml *ml,
    const char *controlstr, const char *mlmmjsend, const char *mailname)
{
	char *bouncenr, *tmpstr;
	char *param = NULL, *moderatefilename, *gatekeepfilename;
	char *omit = NULL;
	char *c, *sendfilename;
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

	cc = get_ctrl_command(controlstr, &param);
	if (cc == NULL)
		return (-1);

	if (fromemails->len != 1 && cc->type != CTRL_BOUNCES) {
		errno = 0;
		log_error(LOG_ARGS, "Ignoring mail with invalid From: "
			"which was not a bounce: %d", fromemails->len);
		return -1;
	}
	
	if (fromemails->len >= 1)
		lc.fromemail = fromemails->d[0];
	lc.mlmmjsend = mlmmjsend;

	/* For subscribe commands, prefer Reply-To over From */
	if (rtoemails->len == 1 &&
	    (cc->type == CTRL_SUBSCRIBE ||
	     cc->type == CTRL_SUBSCRIBE_DIGEST ||
	     cc->type == CTRL_SUBSCRIBE_NOMAIL ||
	     cc->type == CTRL_SUBSCRIBE_BOTH)) {
		lc.fromemail = rtoemails->d[0];
	}

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
		/* fallthrough */
	/* listname+subscribe-nomail@domain.tld */
	case CTRL_SUBSCRIBE_NOMAIL:
		if (sub.typereq == NULL) {
			sub.typereq = "A subscribe-nomail";
			sub.ctrlstr = "nonomailsub";
			sub.typesub = SUB_NOMAIL;
			sub.type = "nomail";
			sub.typedeny = "sub-deny-nomail";
		}
		/* fallthrough */
	/* listname+subscribe-both@domain.tld */
	case CTRL_SUBSCRIBE_BOTH:
		if (sub.typereq == NULL) {
			sub.typereq = "A subscribe-both";
			sub.ctrlstr = "nodigestsub";
			sub.typesub = SUB_BOTH;
			sub.type = "both";
			sub.typedeny = "sub-deny-digest";
		}
		/* fallthrough */
	/* listname+subscribe@domain.tld */
	case CTRL_SUBSCRIBE:
		if (sub.typereq == NULL) {
			sub.typereq = "A subscribe";
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
		/* fallthrough */
	/* listname+subconf-nomail-COOKIE@domain.tld */
	case CTRL_CONFSUB_NOMAIL:
		if (subtypename == NULL) {
			subc.typesub = SUB_NOMAIL;
			subtypename = "nomail";
		}
		/* fallthrough */
	/* listname+subconf-both-COOKIE@domain.tld */
	case CTRL_CONFSUB_BOTH:
		if (subtypename == NULL) {
			subc.typesub = SUB_BOTH;
			subtypename = "both";
		}
		/* fallthrough */
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
		/* fallthrough */
	/* listname+unsubconf-nomail-COOKIE@domain.tld */
	case CTRL_CONFUNSUB_NOMAIL:
		if (ts == SUB_NONE) {
			ts = SUB_NOMAIL;
			subtypename = "-nomail";
		}
		/* fallthrough */
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
		char *lowered = lowercase(c);
		bret = bouncemail(ml->fd, lowered, bouncenr);
		free(lowered);
		if (bret == BOUNCE_DONE)
			save_lastbouncedmsg(ml->fd, c, mailname);
		if (bret == BOUNCE_FAIL)
			exit(EXIT_FAILURE);
		unlink(mailname);
		exit(EXIT_SUCCESS);
		break;

	/* listname+subrelease-COOKIE@domain.tld */
	case CTRL_SUBRELEASE:
		ts = SUB_NORMAL;
		/* fallthrough */
	/* listname+digestsubrelease-COOKIE@domain.tld */
	case CTRL_DIGESTSUBRELEASE:
		if (ts == SUB_NONE)
			ts = SUB_DIGEST;
		/* fallthrough */
	/* listname+nomailsubrelease-COOKIE@domain.tld */
	case CTRL_NOMAILSUBRELEASE:
		if (ts == SUB_NONE)
			ts = SUB_NOMAIL;
		/* fallthrough */
	/* listname+release-COOKIE@domain.tld */
	case CTRL_RELEASE:
	/* DEPRECATED: listname+moderate-COOKIE@domain.tld */
	/* DEPRECATED: listname+moderate-subscribeCOOKIE@domain.tld */
	case CTRL_MODERATE:
		/* Subscriber moderation; DEPRECATED */
		if (strncmp(param, "subscribe", 9) == 0) {
			tmpstr = xstrdup(param + 9);
			free(param);
			param = tmpstr;
			goto permit;
		}

		bool releaseall = false;

		if (prepare_moderated_release(ml, param, &sendfilename, &omit) != 0) {
			if (errno == ENOENT) {
				errno = 0;
				log_error(LOG_ARGS, "A release request was"
					" sent with a mismatching cookie."
					" Ignoring mail");
			} else {
				log_error(LOG_ARGS, "Could not rename to .sending");
			}
			return (-1);
		}
		if (ts != SUB_NONE) {
			/* subrelease: always subscribe with requested type */
			int mfd = openat(ml->fd, sendfilename, O_RDONLY);
			if (mfd == -1) {
				free(sendfilename);
				return (-1);
			}
			char *sender = autosubscribe_sender(ml, mfd, ts);
			releaseall = (sender != NULL &&
			    statctrl(ml->ctrlfd, "subreleaseall"));
			if (releaseall)
				release_sender_moderation(ml, mlmmjsend,
				    sender, sendfilename, omit);
			free(sender);
		} else if (statctrl(ml->ctrlfd, "autosubscribe")) {
			int mfd = openat(ml->fd, sendfilename, O_RDONLY);
			if (mfd == -1) {
				free(sendfilename);
				return (-1);
			}
			free(autosubscribe_sender(ml, mfd, SUB_NOMAIL));
		}

		log_oper(ml->fd, OPLOGFNAME, "%s released %s", lc.fromemail, param);

		if (releaseall) {
			/* The whole batch, including this mail, is sent */
			free(sendfilename);
			free(omit);
			break;
		}

		/* Does not return */
		send_moderated_release(ml, mlmmjsend, sendfilename, omit, false);
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
		exit(do_subscribe(ml, &subc, NULL) ? EXIT_SUCCESS : EXIT_FAILURE);
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
			fromemails->d[0], param);
		free(param);
		free(gatekeepfilename);
		break;

	/* listname+help@domain.tld */
	case CTRL_HELP:
		if (!is_valid_email(fromemails->d[0], "A help"))
			return -1;
		log_oper(ml->fd, OPLOGFNAME, "%s requested help",
				fromemails->d[0]);
		txt = open_text(ml->fd, "help", NULL, NULL, NULL, "listhelp");
		MY_ASSERT(txt);
		register_default_unformatted(txt, ml);
		queuefilename = prepstdreply(txt, ml,
				"$listowner$", fromemails->d[0], NULL);
		MY_ASSERT(queuefilename);
		close_text(txt);
		send_help(ml, queuefilename, fromemails->d[0]);
		break;

	/* listname+faq@domain.tld */
	case CTRL_FAQ:
		if (!is_valid_email(fromemails->d[0], "A faq"))
			return -1;
		log_oper(ml->fd, OPLOGFNAME, "%s requested faq",
		    fromemails->d[0]);
		txt = open_text(ml->fd, "faq", NULL, NULL, NULL, "listfaq");
		MY_ASSERT(txt);
		register_default_unformatted(txt, ml);
		queuefilename = prepstdreply(txt, ml,
				"$listowner$", fromemails->d[0], NULL);
		MY_ASSERT(queuefilename);
		close_text(txt);
		send_help(ml, queuefilename, fromemails->d[0]);
		break;

	/* listname+get-INDEX@domain.tld */
	case CTRL_GET:
		if (statctrl(ml->ctrlfd, "noget")) {
			errno = 0;
			log_error(LOG_ARGS, "A get request was sent to a list"
				" with the noget option set. Ignoring mail");
			free(param);
			return -1;
		}
		if (statctrl(ml->ctrlfd, "subonlyget")) {
			if (is_subbed(ml->fd, fromemails->d[0], 0) ==
					SUB_NONE) {
				errno = 0;
				log_error(LOG_ARGS, "A get request was sent"
					" from a non-subscribed address to a"
					" list with the subonlyget option set."
					" Ignoring mail");
				free(param);
				return -1;
			}
		}
		/* sanity check--is it all digits? */
		if (param[strspn(param, "0123456789")] != '\0') {
			errno = 0;
			log_error(LOG_ARGS, "The get request contained"
			    " non-digits in index. Ignoring mail");
			free(param);
			return -1;
		}
		int index = strtoim(param, 0, INT_MAX, NULL);
		int fd = archive_open(ml->fd, index);
		if (fd == -1) {
			log_error(LOG_ARGS, "Unable to open archive file");
			free(param);
			return -1;
		}
		struct mail mail = { 0 };
		char *bounceaddr;
		mail.to = fromemails->d[0];
		bounceaddr = get_bounce_from_adr(mail.to, ml, index);
		mail.from = bounceaddr;
		mail.fp = fdopen(fd, "r");
		if (mail.fp == NULL) {
			close(fd);
			free(bounceaddr);
			free(param);
			return -1;
		}
		log_oper(ml->fd, OPLOGFNAME, "%s got archive/%s",
				fromemails->d[0], param);
		if (!send_single_mail(&mail, ml, true)) {
			/* TODO: save to queue */
			fclose(mail.fp);
			free(bounceaddr);
			free(param);
			return -1;
		}
		fclose(mail.fp);
		free(bounceaddr);
		free(param);
		break;

	/* listname+list@domain.tld */
	case CTRL_LIST:
		if (statctrl(ml->ctrlfd, "nolistsubsemail"))
			return -1;
		const char *owner = fromemails->d[0];
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
