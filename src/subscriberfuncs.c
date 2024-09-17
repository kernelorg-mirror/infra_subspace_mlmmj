/*
 * Copyright (C) 2003 Mads Martin Joergensen <mmj at mmj.dk>
 * Copyright (C) 2022-2023 Baptiste Daroussin <bapt@FreeBSD.org>
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

#include <sys/stat.h>
#include <sys/wait.h>

#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <err.h>

#include "mlmmj.h"
#include "subscriberfuncs.h"
#include "log_error.h"
#include "chomp.h"
#include "utils.h"
#include "prepstdreply.h"
#include "xmalloc.h"
#include "send_mail.h"
#include "strgen.h"
#include "send_help.h"
#include "statctrl.h"
#include "xstring.h"
#include "ctrlvalues.h"

char *subtype_strs[] = {
	"normal",
	"digest",
	"nomail",
	"file",
	"all",
	"both",
	"none"
};

char * subreason_strs[] = {
	"request",
	"confirm",
	"permit",
	"admin",
	"bouncing",
	"switch"
};

char *subtypes[] = {
	"SUB_NORMAL",
	"SUB_DIGEST",
	"SUB_NOMAIL",
	NULL,
	NULL,
	"SUB_BOTH",
	NULL,
};

bool
find_subscriber(int fd, const char *address)
{
	FILE *f;
	char *line = NULL;
	size_t linecap = 0;
	bool ret = false;

	f = fdopen(fd, "r");
	while (getline(&line, &linecap, f) > 0) {
		chomp(line);
		if (strcasecmp(address, line) == 0) {
			ret = true;
			break;
		}
	}
	free(line);
	fclose(f);
	return (ret);;
}

int
is_subbed_in(int dirfd, const char *subdirname, const char *address)
{
	int subread;
	DIR *subddir;
	struct dirent *dp;
	bool ret = false;

	if((subddir = fdopendir(dirfd)) == NULL) {
		log_error(LOG_ARGS, "Could not opendir(%s)", subdirname);
		exit(EXIT_FAILURE);
	}

	while((dp = readdir(subddir)) != NULL) {
		if(!strcmp(dp->d_name, "."))
			continue;
		if(!strcmp(dp->d_name, ".."))
			continue;

		subread = openat(dirfd, dp->d_name, O_RDONLY);
		if(subread < 0) {
			log_error(LOG_ARGS, "Could not open %s/%s",
			  subdirname, dp->d_name);
			continue;
		}

		ret = find_subscriber(subread, address);
		if (ret)
			break;
	}
	closedir(subddir);

	return ret;
}

enum subtype
is_subbed(int listfd, const char *address, bool both)
{
	enum subtype typesub = SUB_NONE;
	int fd;

	fd = openat(listfd, "subscribers.d", O_DIRECTORY);
	if (fd != -1 && is_subbed_in(fd, "subscribers.d", address)) {
		if (!both) return SUB_NORMAL;
		typesub = SUB_NORMAL;
	}

	fd = openat(listfd, "digesters.d", O_DIRECTORY);
	if (fd != -1 && is_subbed_in(fd, "digesters.d", address)) {
		if (typesub == SUB_NORMAL) return SUB_BOTH;
		return SUB_DIGEST;
	}

	fd = openat(listfd, "nomailsubs.d", O_DIRECTORY);
	if (fd != -1 && is_subbed_in(fd, "nomailsubs.d", address))
		return SUB_NOMAIL;

	return (typesub);
}

char *
get_subcookie_content(int listfd, bool unsub, const char *param)
{
	int dfd, fd;
	char *line = NULL;

	dfd = openat(listfd, unsub ? "unsubconf" : "subconf", O_DIRECTORY|O_CLOEXEC);
	if (dfd == -1) {
		log_error(LOG_ARGS, "Cannot open %ssubconf Ignoring mail", unsub ? "un" : "");
		return (NULL);
	}
	fd = openat(dfd, param, O_RDONLY);
	if (fd == -1) {
		close(dfd);
		return (NULL);
	}
	line = readlf(fd, true);
	if (line != NULL)
		unlinkat(dfd, param, 0);
	close(dfd);

	return (line);
}

void
notify_sub(struct ml *ml, const char *subaddr, enum subtype typesub,
    enum subreason reasonsub, bool sub)
{
	char *tostr;
	text *txt;
	char *queuefilename = NULL;
	const char *listtext = NULL;

	gen_addr(tostr, ml, "owner");

	switch(typesub) {
		default:
		case SUB_NORMAL:
			listtext = sub ? "notifysub" : "notifyunsub";
			break;
		case SUB_DIGEST:
			listtext = sub ? "notifysub-digest": "notifyunsub-digest";
			break;
		case SUB_NOMAIL:
			listtext = sub ? "notifysub-nomail": "notifyunsub-nomail";
			break;
		case SUB_BOTH:
			/* No legacy list text as feature didn't exist. */
			listtext = "notifysub";
			break;
	}

	txt = open_text(ml->fd, "notify", sub? "sub":"unsub",
			subreason_strs[reasonsub], subtype_strs[typesub],
			listtext);
	MY_ASSERT(txt);
	register_default_unformatted(txt, ml);
	register_unformatted(txt, "subaddr", subaddr);
	queuefilename = prepstdreply(txt, ml, "$listowner$", "$listowner$", NULL);
	MY_ASSERT(queuefilename);
	close_text(txt);

	send_help(ml, queuefilename, tostr);
	exit(EXIT_SUCCESS);
}

void
generate_subscription(struct ml *ml, const char *subaddr,
    enum subtype typesub, bool sub)
{
	text *txt;
	char *queuefilename;

	txt = open_text(ml->fd, "deny", sub ? "sub" : "unsub",
	    sub ? "subbed" : "unsubbed", subtype_strs[typesub],
	    sub ? "sub-subscribed" : "unsub-notsubscribed");
	MY_ASSERT(txt);
	register_default_unformatted(txt, ml);
	register_unformatted(txt, "subaddr", subaddr);
	queuefilename = prepstdreply(txt, ml, "$helpaddr$", subaddr, NULL);
	MY_ASSERT(queuefilename);
	close_text(txt);

	send_help(ml, queuefilename, subaddr);
}

void
generate_subconfirm(struct ml *ml, const char *subaddr, enum subtype typesub,
    enum subreason reasonsub, bool sub)
{
	int subconffd;
	text *txt;
	char *queuefilename = NULL, *fromaddr;
	char *randomstr = NULL, *confirmaddr;
	const char *tmpstr, *listtext;
	int fd = openat(ml->fd, sub ? "subconf" : "unsubconf", O_DIRECTORY|O_CLOEXEC);
	struct mail mail = { 0 };

	if (fd == -1) {
		log_error(LOG_ARGS, "Cound not open(%s/%s)", ml->dir,
		    sub ? "subconf" : "unsubconf");
		exit(EXIT_FAILURE);
	}

	do {
		free(randomstr);
		randomstr = random_str();
		subconffd = openat(fd, randomstr, O_RDWR|O_CREAT|O_EXCL, S_IRUSR|S_IWUSR);
	} while ((subconffd < 0) && (errno == EEXIST));

	if(subconffd < 0) {
		log_error(LOG_ARGS, "Could not open '%s/%s/%s'", ml->dir,
		    sub ? "subconf" : "unsubconf", randomstr);
		free(randomstr);
		exit(EXIT_FAILURE);
	}
	dprintf(subconffd, "%s", subaddr);
	close(subconffd);

	gen_addr_cookie(fromaddr, ml, sub ? "bounces-confsub-" : "bounces-confunsub-", randomstr);

	switch(typesub) {
		default:
		case SUB_NORMAL:
			listtext = sub ? "sub-confirm" : "unsub-confirm";
			tmpstr = sub ? "confsub-" : "confunsub-";
			break;
		case SUB_DIGEST:
			listtext = sub ? "sub-confirm-digest" : "unsub-confirm-digest";
			tmpstr = sub ? "confsub-digest-" : "confunsub-digest-";
			break;
		case SUB_NOMAIL:
			listtext = sub ? "sub-confirm-nomail" : "unsub-confirm-nomail";
			tmpstr = sub ? "confsub-nomail-" : "confunsub-nomail-";
			break;
		case SUB_BOTH:
			/* No legacy list text as feature didn't exist. */
			listtext = sub ? "sub-confirm" : "unsub-confirm";
			tmpstr = sub ? "confsub-both-" : "confunsub-both-";
			break;
	}

	gen_addr_cookie(confirmaddr, ml, tmpstr, randomstr);
	free(randomstr);

	txt = open_text(ml->fd, "confirm", sub ? "sub" : "unsub",
			subreason_strs[reasonsub], subtype_strs[typesub],
			listtext);
	MY_ASSERT(txt);
	register_default_unformatted(txt, ml);
	register_unformatted(txt, "subaddr", subaddr);
	register_unformatted(txt, "confirmaddr", confirmaddr);
	queuefilename = prepstdreply(txt, ml, "$helpaddr$", subaddr, confirmaddr);
	MY_ASSERT(queuefilename);
	close_text(txt);

	mail.to = subaddr;
	mail.from = fromaddr;
	mail.fp = fopen(queuefilename, "r");
	if (!send_single_mail(&mail, ml, false))
		save_queue(queuefilename, &mail);
	else
		unlink(queuefilename);
	exit(EXIT_SUCCESS);
}

void
send_confirmation_mail(struct ml *ml, const char *subaddr, enum subtype typesub,
    enum subreason reasonsub, bool sub)
{
	text *txt;
	char *queuefilename;
	const char *listtext;

	switch(typesub) {
		default:
		case SUB_BOTH: /* FALLTHROUGH */
		case SUB_NORMAL:
			listtext = sub ? "sub-ok" : "unsub-ok";
			break;
		case SUB_DIGEST:
			listtext = sub ? "sub-ok-digest" : "unsub-ok-digest";
			break;
		case SUB_NOMAIL:
			listtext = sub ? "sub-ok-nomail" : "unsub-ok-nomail";
			break;
	}

	txt = open_text(ml->fd, "finish", sub ? "sub" : "unsub",
			subreason_strs[reasonsub], subtype_strs[typesub],
			listtext);
	MY_ASSERT(txt);
	register_default_unformatted(txt, ml);
	register_unformatted(txt, "subaddr", subaddr);
	queuefilename = prepstdreply(txt, ml, "$helpaddr$", subaddr, NULL);
	MY_ASSERT(queuefilename);
	close_text(txt);

	send_help_noexit(ml, queuefilename, subaddr);
}

bool
do_unsubscribe(struct ml *ml, const char *addr, enum subtype typesub,
     enum subreason reasonsub, bool inform_not_subscribed,
     bool confirm_unsubscription, bool quiet, bool send_goodbye_mail)
{
	char *address;
	bool subscribed = false;

	address = lowercase(addr);

	if (typesub == SUB_ALL) {
		subscribed = is_subbed(ml->fd, address, false) != SUB_NONE;
	} else {
		const char *subdir;
		int fd = open_subscriber_directory(ml->fd, typesub, &subdir);
		if (fd == -1) {
			log_error(LOG_ARGS, "Could not opendir(%s/%s)", ml->dir,
			    subdir);
			free(address);
			return (false);
		}
		subscribed = is_subbed_in(fd, subdir, address);
		close(fd);
	}

	if (!subscribed) {
		if (inform_not_subscribed)
			generate_subscription(ml, address, typesub, false);
		free(address);
		return (true);
	}
	if (confirm_unsubscription)
		generate_subconfirm(ml, address, typesub, reasonsub, false);

	unsubscribe(ml->fd, address, typesub);
	if (send_goodbye_mail)
		send_confirmation_mail(ml, address, typesub, reasonsub, false);

	/* Notify list owner about subscription */
	if (!quiet && statctrl(ml->ctrlfd, "notifysub"))
		notify_sub(ml, address, typesub, reasonsub, false);

	free(address);
	return (true);
}

void
mod_get_addr_and_type(struct ml *ml, const char *modstr, char **addrptr, enum subtype *subtypeptr)
{
	int fd, dfd;
	char *readtype, *modfilename = NULL;
	char *buf, *walk;
	size_t i;

	if (strncmp(modstr, "subscribe", 9) == 0)
			modstr += 9;

	dfd = openat(ml->fd, "moderation/subscribe", O_DIRECTORY);
	if (dfd != -1) {
		fd = openat(dfd, modstr, O_RDONLY);
	}
	if (dfd == -1 || fd == -1) {
		xasprintf(&modfilename, "moderation/subscribe%s", modstr);
		fd = openat(ml->fd, modfilename, O_RDONLY);
	}
	if(fd < 0) {
		log_error(LOG_ARGS, "Could not open "
		    "%s/moderation/subscribe/%s nor %s/%s", ml->dir, modstr,
		    ml->dir, modfilename);
		exit(EXIT_FAILURE);
	}
	walk = buf = readlf(fd, false);
	if (buf == NULL) {
		if (modfilename == NULL)
			log_error(LOG_ARGS, "Invalid %s/moderation/subscribe/%s",
			    ml->dir, modstr);
		else
			log_error(LOG_ARGS, "Invalid %s/%s", ml->dir,
			    modfilename);
		exit(EXIT_FAILURE);
	}
	*addrptr = xstrdup(strsep(&walk, "\n"));
	readtype = strsep(&walk, "\n");

	for (i = 0; i < NELEM(subtypes); i++) {
		if (subtypes[i] == NULL)
			continue;
		if (strcmp(subtypes[i], readtype) == 0) {
			*subtypeptr = i;
			break;
		}
	}

	if (i == NELEM(subtypes)) {
		log_error(LOG_ARGS, "Type %s not valid in %s/%s", readtype,
				ml->dir, modfilename);
		exit(EXIT_FAILURE);
	}

	free(buf);
	if (modfilename != NULL)
		unlinkat(ml->fd, modfilename, 0);
	else
		unlinkat(dfd, modstr, 0);
	if (dfd != -1)
		close(dfd);
	free(modfilename);
}

static void moderate_sub(struct ml *ml, const char *subaddr,
    struct subscription *sub)
{
	int fd, status;
	text *txt;
	memory_lines_state *mls;
	char *a = NULL, *queuefilename, *from;
	char *modfilename, *mods, *to, *replyto, *moderators = NULL;
	char *cookie, *obstruct;
	strlist *submods;
	const char *type;
	pid_t childpid, pid;
	xstring *str = NULL;
	int modfd = -1;

	type = subtypes[sub->typesub];

	for (;;) {
		cookie = random_str();
		xasprintf(&modfilename, "moderation/subscribe%s", cookie);
		fd = openat(ml->fd, modfilename, O_RDWR|O_CREAT|O_EXCL,
		    S_IRUSR|S_IWUSR);
		if (fd < 0) {
			if (errno == EEXIST) {
				free(cookie);
				free(modfilename);
				continue;
			}
			log_error(LOG_ARGS, "could not create %s"
					"ignoring request for: %s", subaddr);
			exit(EXIT_FAILURE);
		}
		break;
	}

	if (dprintf(fd, "%s\n%s\n", subaddr, type) < 0) {
		log_error(LOG_ARGS, "could not write to %s"
				"ignoring request for: %s", subaddr);
		exit(EXIT_FAILURE);
	}
	close(fd);

	submods = ctrlvalues(ml->ctrlfd, "submod");
	if (submods == NULL)
		return;
	/* check to see if there's adresses in the submod control file */
	tll_foreach(*submods, it)
		a = strchr(it->item, '@');

	/* no addresses in submod control file, use owner */
	if(a == NULL) {
		/* free the submods struct from above */
		tll_free_and_free(*submods, free);
		free(submods);
		submods = ctrlvalues(ml->ctrlfd, "owner");
		modfd = openat(ml->ctrlfd, "owner", O_RDONLY);
	}
	if (modfd == -1)
		modfd = openat(ml->ctrlfd, "submod", O_RDONLY);

	gen_addr(from, ml, "owner");
	xasprintf(&to, "%s-moderators@%s", ml->name, ml->fqdn);
	gen_addr_cookie(replyto, ml, "permit-", cookie);
	gen_addr_cookie(obstruct, ml, "obstruct-", cookie);
	free(cookie);
	tll_foreach(*submods, sm) {
		if (str == NULL)
			str = xstring_new();
		fprintf(str->fp, "%s\n", sm->item);
	}
	moderators = xstring_get(str);
	mls = init_memory_lines(moderators);
	free(moderators);

	txt = open_text(ml->fd,
			"gatekeep", "sub",
			subreason_strs[sub->reasonsub], subtype_strs[sub->typesub],
			"submod-moderator");
	MY_ASSERT(txt);
	register_default_unformatted(txt, ml);
	register_unformatted(txt, "subaddr", subaddr);
	register_unformatted(txt, "permitaddr", replyto);
	register_unformatted(txt, "obstructaddr", obstruct);
	register_formatted(txt, "gatekeepers",
			rewind_memory_lines, get_memory_line, mls);
	queuefilename = prepstdreply(txt, ml, "$listowner$", to, replyto);
	MY_ASSERT(queuefilename);
	close_text(txt);
	
	/* we might need to exec more than one mlmmj-send */

	if (statctrl(ml->ctrlfd, "nosubmodmails"))
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
		xasprintf(&mods, "%d", modfd);
		execl(sub->mlmmjsend, sub->mlmmjsend,
				"-a",
				"-l", "4",
				"-L", ml->dir,
				"-s", mods,
				"-F", from,
				"-R", replyto,
				"-m", queuefilename, (char *)NULL);
		log_error(LOG_ARGS, "execl() of '%s' failed", sub->mlmmjsend);
		exit(EXIT_FAILURE);
	}

	free(to);
	free(replyto);
	
	/* send mail to requester that the list is submod'ed */

	txt = open_text(ml->fd,
			"wait", "sub",
			subreason_strs[sub->reasonsub], subtype_strs[sub->typesub],
			"submod-requester");
	MY_ASSERT(txt);
	register_default_unformatted(txt, ml);
	register_unformatted(txt, "subaddr", subaddr);
	register_formatted(txt, "gatekeepers",
			rewind_memory_lines, get_memory_line, mls);
	queuefilename = prepstdreply(txt, ml, "$listowner$", subaddr, NULL);
	MY_ASSERT(queuefilename);
	close_text(txt);

	finish_memory_lines(mls);
	send_help(ml, queuefilename, subaddr);
}

static void
subscribe_type(int listfd, char *address, enum subtype typesub)
{
	int dirfd;;
	char chstr[2];
	const char *subdir;
	int groupwritable = 0, subfilefd;
	struct stat st;

	dirfd = open_subscriber_directory(listfd, typesub, &subdir);
	if (dirfd == -1)
		err(EXIT_FAILURE, "cannot open(%s)", subdir);
	if (fstat(dirfd, &st) == 0) {
		if(st.st_mode & S_IWGRP) {
			groupwritable = S_IRGRP|S_IWGRP;
			umask(S_IWOTH);
			setgid(st.st_gid);
		}
	}

	chstr[0] = address[0];
	chstr[1] = '\0';

	subfilefd = openat(dirfd, chstr, O_RDWR|O_CREAT|O_APPEND,
				S_IRUSR|S_IWUSR|groupwritable);
	if(subfilefd == -1 && !lock(subfilefd, true)) {
		log_error(LOG_ARGS, "Could not open '%s/%s'", subdir, chstr);
		exit(EXIT_FAILURE);
	}

	dprintf(subfilefd, "%s\n", address);
	close(dirfd);
	close(subfilefd);
}

bool
do_subscribe(struct ml *ml, struct subscription *sub, const char *addr)
{
	char *address = NULL;
	enum subtype subbed;

	if (addr != NULL)
		address = lowercase(addr);

	if (sub->modstr != NULL) {
		mod_get_addr_and_type(ml, sub->modstr, &address, &sub->typesub);
		sub->reasonsub = SUB_PERMIT;
	}

	if (address == NULL)
		errx(EXIT_FAILURE, "No address to subscribe found");

	if(strncasecmp(ml->addr, address, strlen(ml->addr)) == 0)
		errx(EXIT_FAILURE, "Cannot subscribe the list address to the list");

	subbed = is_subbed(ml->fd, address, 1);

	if (subbed == sub->typesub) {
		if (sub->gensubscribed)
			generate_subscription(ml, address, sub->typesub, true);
		free(address);
		return (true);
	}
	if (subbed != SUB_NONE) {
		sub->reasonsub = SUB_SWITCH;
		/* If we want to subscribe to both, we can just subscribe the
		 * missing version, so don't unsub-> */
		if (!(sub->typesub == SUB_BOTH &&
				subbed != SUB_NOMAIL)) {
			enum subtype ts = SUB_ALL;
			if (subbed == SUB_BOTH) {
				if (sub->typesub == SUB_NORMAL) ts = SUB_DIGEST;
				if (sub->typesub == SUB_DIGEST) ts = SUB_NORMAL;
			}
			if (!unsubscribe(ml->fd, address, ts))
				log_error(LOG_ARGS, "not unsubscribed from "
				    "current version");
		}
	}

	if (subbed == SUB_NONE && sub->subconfirm)
		generate_subconfirm(ml, address, sub->typesub, sub->reasonsub,
		    true);

	if(sub->modstr == NULL && subbed == SUB_NONE && !sub->force &&
			statctrl(ml->ctrlfd, "submod")) {
		moderate_sub(ml, address, sub);
	}

	if (sub->typesub == SUB_BOTH) {
		if (subbed != SUB_NORMAL) {
			subscribe_type(ml->fd, address, SUB_NORMAL);
		}
		if (subbed != SUB_DIGEST) {
			subscribe_type(ml->fd, address, SUB_DIGEST);
		}
	} else if (!(subbed == SUB_BOTH && sub->typesub != SUB_NOMAIL)) {
		subscribe_type(ml->fd, address, sub->typesub);
	}

	if (sub->send_welcome_email)
		send_confirmation_mail(ml, address, sub->typesub,
		    sub->reasonsub, true);

	bool notifysub = !sub->quiet && sub->reasonsub != SUB_SWITCH &&
			statctrl(ml->ctrlfd, "notifysub");

	/* Notify list owner about subscription */
	if (notifysub)
		notify_sub(ml, address, sub->typesub, sub->reasonsub, true);

	free(address);
	return (true);
}
