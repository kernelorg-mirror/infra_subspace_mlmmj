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

#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>

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
	FILE *f;
	char *line = NULL;
	size_t linecap = 0;

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
	f = fdopen(fd, "r");
	if (getline(&line, &linecap, f) > 0) {
		chomp(line);
		fclose(f);
		unlinkat(dfd, param, 0);
		close(dfd);
		return (line);
	}
	close(dfd);
	fclose(f);

	return (NULL);
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
	register_unformatted(txt, sub ? "newsub" : "oldsub", subaddr); /* DEPRECATED */
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
	register_unformatted(txt, "confaddr", confirmaddr); /* DEPRECATED */
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
