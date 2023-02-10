/*
 * Copyright (C) 2005 Mads Martin Joergensen <mmj at mmj.dk>
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
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>

#include "xmalloc.h"
#include "mlmmj.h"
#include "send_list.h"
#include "strgen.h"
#include "log_error.h"
#include "chomp.h"
#include "wrappers.h"
#include "mygetline.h"
#include "prepstdreply.h"
#include "utils.h"


struct subs_list_state {
	int listfd;
	char *dirname;
	DIR *dirp;
	FILE *fp;
	int dfd;
	char *line;
	size_t linecap;
	bool used;
};


struct subs_list_state *init_subs_list(int listfd, const char *dirname)
{
	struct subs_list_state *s = xcalloc(1, sizeof(struct subs_list_state));
	s->listfd = listfd;
	s->dirname = xstrdup(dirname);
	return s;
}


void rewind_subs_list(void *state)
{
	struct subs_list_state *s = (struct subs_list_state *)state;
	if (s == NULL) return;
	if (s->dirp != NULL) closedir(s->dirp);
	s->dfd = openat(s->listfd, s->dirname, O_DIRECTORY|O_CLOEXEC);
	if (s->dfd == -1 || (s->dirp = fdopendir(s->dfd)) == NULL)
		log_error(LOG_ARGS, "Could not opendir(%s);\n", s->dirname);
	s->used = true;
}


const char *get_sub(void *state)
{
	struct subs_list_state *s = (struct subs_list_state *)state;
	struct dirent *dp;
	int fd;

	if (s == NULL) return NULL;
	if (s->dirp == NULL) return NULL;

	if (s->line != NULL) {
		free(s->line);
		s->line = NULL;
	}

	for (;;) {
		if (s->fp == NULL) {
			dp = readdir(s->dirp);
			if (dp == NULL) {
				closedir(s->dirp);
				s->dirp = NULL;
				return NULL;
			}
			if ((strcmp(dp->d_name, "..") == 0) ||
			    (strcmp(dp->d_name, ".") == 0))
					continue;
			fd = openat(s->dfd, dp->d_name, O_RDONLY);
			if(fd < 0 || (s->fp = fdopen(fd, "r")) == NULL) {
				log_error(LOG_ARGS,
				    "Could not open %s/%s for reading",
				    s->dirname, dp->d_name);
				continue;
			}
		}
		if (getline(&s->line, &s->linecap, s->fp) <= 0) {
				fclose(s->fp);
				s->fp = NULL;
				continue;
		}
		chomp(s->line);
		return (s->line);
	}
}

void finish_subs_list(struct subs_list_state *s)
{
	if (s == NULL) return;
	if (s->line != NULL) free(s->line);
	if (s->fp != NULL) fclose(s->fp);
	if (s->dirp != NULL) closedir(s->dirp);
	free(s->dirname);
	free(s);
}


void print_subs(int fd, struct subs_list_state *s)
{
	const char *sub;
	rewind_subs_list(s);
	while ((sub = get_sub(s)) != NULL)
		dprintf(fd, "%s\n", sub);
}


void send_list(struct ml *ml, const char *emailaddr, const char *mlmmjsend)
{
	text *txt;
	struct subs_list_state *normalsls, *digestsls, *nomailsls;
	char *queuefilename, *fromaddr;
	int fd;

	gen_addr(fromaddr, ml, "bounces-help");

	normalsls = init_subs_list(ml->fd, "subscribers.d");
	digestsls = init_subs_list(ml->fd, "digesters.d");
	nomailsls = init_subs_list(ml->fd, "nomailsubs.d");

	txt = open_text(ml->fd, "list", NULL, NULL, subtype_strs[SUB_ALL],
			"listsubs");
	MY_ASSERT(txt);
	register_default_unformatted(txt, ml->delim, ml->addr);
	register_formatted(txt, "listsubs",
			rewind_subs_list, get_sub, normalsls);
	register_formatted(txt, "normalsubs",
			rewind_subs_list, get_sub, normalsls);
	register_formatted(txt, "digestsubs",
			rewind_subs_list, get_sub, digestsls);
	register_formatted(txt, "nomailsubs",
			rewind_subs_list, get_sub, nomailsls);
	queuefilename = prepstdreply(txt, ml->dir, "$listowner$", emailaddr,
	    NULL, ml->fd, ml->ctrlfd);
	MY_ASSERT(queuefilename);
	close_text(txt);

	/* DEPRECATED */
	/* Add lists manually if they weren't encountered in the list text */
	if (!normalsls->used && !digestsls->used && !nomailsls->used) {
		fd = open(queuefilename, O_WRONLY|O_APPEND);
		if(fd < 0) {
			log_error(LOG_ARGS, "Could not open sub list mail");
			exit(EXIT_FAILURE);
		}
		print_subs(fd, normalsls);
		dprintf(fd, "\n-- \n");
		print_subs(fd, nomailsls);
		dprintf(fd, "\n-- \n");
		print_subs(fd, digestsls);
		dprintf(fd, "\n-- \nend of output\n");
		close(fd);
	}

	finish_subs_list(normalsls);
	finish_subs_list(digestsls);
	finish_subs_list(nomailsls);

	exec_or_die(mlmmjsend, "-l", "1", "-L", ml->dir, "-T", emailaddr, "-F",
	    fromaddr, "-m", queuefilename, NULL);
}
