/*
 * Copyright (C) 2004 Mads Martin Joergensen <mmj at mmj.dk>
 * Copyright (C) 2007 Morten K. Poulsen <morten at afdelingp.dk>
 * Copyright (C) 2011 Ben Schmidt <mail_ben_schmidt at yahoo.com.au>
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

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

#include "xmalloc.h"
#include "prepstdreply.h"
#include "statctrl.h"
#include "ctrlvalue.h"
#include "strgen.h"
#include "chomp.h"
#include "log_error.h"
#include "wrappers.h"
#include "mlmmj.h"
#include "unistr.h"
#include "sb.h"
#include "do_all_the_voodoo_here.h"

struct substitution {
	char *token;
	char *subst;
};

struct formatted {
	char *token;
	rewind_function rew;
	get_function get;
	void *state;
};

struct source;
typedef struct source source;
struct source {
	source *prev;
	char *upcoming;
	int processedlen;
	int processedwidth;
	char *prefix;
	int prefixlen;
	int prefixwidth;
	char *suffix;
	FILE *fp;
	struct formatted *fmt;
	int transparent;
	int limit;
};


struct conditional;
typedef struct conditional conditional;
struct conditional {
	int satisfied;
	int elsepart;
	conditional *outer;
};


enum conditional_target {
	ACTION,
	REASON,
	TYPE,
	CONTROL
};


enum wrap_mode {
	WRAP_WORD,
	WRAP_CHAR,
	WRAP_USER
};


enum width_reckoning {
	WIDTH_THIN,
	WIDTH_WIDE
};


struct text {
	char *action;
	char *reason;
	char *type;
	source *src;
	vec_t(struct substitution *) substs;
	char *mailname;
	vec_t(struct formatted *) fmts;
	int wrapindent;
	int wrapwidth;
	enum wrap_mode wrapmode;
	enum width_reckoning widthreckoning;
	char *zerowidth;
	conditional *cond;
	conditional *skip;
};


struct memory_lines_state {
	char *lines;
	char *pos;
	char *end;
};


struct file_lines_state {
	char *filename;
	FILE *fp;
	char truncate;
	char *line;
	size_t linecap;
	char *formatted_str;
	void *data;
};


memory_lines_state *
init_memory_lines(const char *lines)
{
	memory_lines_state *s = xcalloc(1, sizeof(memory_lines_state));
	char *l;

	l = s->lines = xstrdup(lines);
	chomp(s->lines);
	s->end = s->lines + strlen(s->lines);
	while (strsep(&l, "\n"));

	return (s);
}

void
rewind_memory_lines(void *state)
{
	memory_lines_state *s = (memory_lines_state *)state;
	if (s == NULL) return;
	s->pos = NULL;
}

const char *
get_memory_line(void *state)
{
	memory_lines_state *s = (memory_lines_state *)state;
	if (s == NULL) return NULL;

	if (s->pos == NULL) {
		s->pos = s->lines;
		return (s->pos);
	}

	s->pos += strlen(s->pos);
	if (s->pos >= s->end)
		return (NULL);
	while (s->pos[0] == '\0' && s->pos < s->end)
		s->pos++;
	return (s->pos);
}

void
finish_memory_lines(memory_lines_state *s)
{
	if (s == NULL) return;
	free(s->lines);
	free(s);
}


file_lines_state *
init_file_lines(const char *filename, char truncate)
{
	file_lines_state *s = xcalloc(1, sizeof(file_lines_state));
	s->filename = xstrdup(filename);
	s->truncate = truncate;
	return s;
}

file_lines_state
*init_file_lines_fd(int fd, char truncate)
{
	file_lines_state *s;

	if (fd == -1)
		return (NULL);
	s = xcalloc(1, sizeof(file_lines_state));
	s->fp = fdopen(fd, "r");
	if (s->fp == NULL) {
		close(fd);
		free(s);
		return (NULL);
	}
	s->truncate = truncate;
	return (s);
}

file_lines_state *
init_file_lines_data(int fd, char truncate, void *data)
{
	file_lines_state *s;

	s = init_file_lines_fd(fd, truncate);
	if (s == NULL)
		return (NULL);
	s->data = data;
	return (s);
}

void rewind_file_lines(void *state)
{
	file_lines_state *s = (file_lines_state *)state;
	if (s == NULL) return;
	if (s->filename != NULL) {
		s->fp = fopen(s->filename, "r");
		free(s->filename);
		s->filename = NULL;
	}
	if (s->fp != NULL)
		rewind(s->fp);
}

const char *get_file_line(void *state)
{
	file_lines_state *s = (file_lines_state *)state;
	char *end;
	if (s == NULL) return (NULL);
	if (s->fp == NULL) return (NULL);
	if (getline(&s->line, &s->linecap, s->fp) <= 0)
		return (NULL);
	if (s->truncate != '\0') {
		end = strchr(s->line, s->truncate);
		if (end == NULL) return NULL;
		*end = '\0';
	} else {
		chomp(s->line);
	}
	return s->line;
}

const char *
get_msgid_line(void *state)
{
	file_lines_state *s = (file_lines_state *)state;
	const char *id = get_file_line(state);
	if (id == NULL)
		return (NULL);
	if (s->data == NULL) {
		return (id);
	}
	struct ml *m = (struct ml *)s->data;
	struct mailhdr readhdrs[] = {
		{ "Message-ID", 0, NULL },
		{ NULL, 0, NULL }
	};
	int archive_fd = openat(m->fd, "archive", O_DIRECTORY|O_RDONLY);
	if (archive_fd == -1)
		return (id);
	int msgfd = openat(archive_fd, id, O_RDONLY);
	close(archive_fd);
	if (msgfd == -1)
		return (id);
	FILE *f = fdopen(msgfd, "r");
	if (f == NULL) {
		close(msgfd);
		return (id);
	}
	scan_headers(f, readhdrs, NULL, NULL);
	fclose(f);
	if (readhdrs[0].valuecount == 0) {
		free_mailhdrs(readhdrs);
		return (id);
	}
	free(s->formatted_str);
	xasprintf(&s->formatted_str, "%s, %s%s",
	    id, readhdrs[0].token, readhdrs[0].values[0]);
	free_mailhdrs(readhdrs);
	return (s->formatted_str);
}

void finish_file_lines(file_lines_state *s)
{
	if (s == NULL) return;
	free(s->line);
	free(s->formatted_str);
	if (s->fp != NULL) fclose(s->fp);
	if (s->filename != NULL) free(s->filename);
	free(s);
}


static char *filename_token(char *token)
{
	const char *pos = token;
	if (*pos == '\0') return NULL;
	while (
		(*pos >= '0' && *pos <= '9') ||
		(*pos >= 'A' && *pos <= 'Z') ||
		(*pos >= 'a' && *pos <= 'z') ||
		(*pos == '_') || (*pos == '-') ||
		(*pos == '.' && pos != token)
	) {
		pos++;
	}
	if (*pos != '\0') return NULL;
	return token;
}


static char *numeric_token(char *token)
{
	char *pos;
	if (*token == '\0') return NULL;
	for(pos = token; *pos != '\0'; pos++) {
		if(*pos >= '0' && *pos <= '9') continue;
		break;
	}
	if (*pos != '\0') return NULL;
	return token;
}

static size_t
do_substitute(sb_t *str, const char *line, int listfd, int ctrlfd, text *txt)
{
	const char *key, *endpos;
	char *value = NULL;
	char *token = NULL;
	struct substitution *subst;
	bool found = true;
	size_t len;
	if (*line != '$')
		return (0);
	line++;
	key = line;
	endpos = strchr(key, '$');
	if (endpos == NULL) {
		sb_cat_c(str, '$');
		return (0);
	}
	len = endpos - key;
	if (len == 8 && strncmp(key, "control ", 8) == 0) {
		token = xstrndup(key + 8, endpos - key + 8);
		if (strchr(token, '/') == NULL) /* ensure stay in the directory */
			value = ctrlcontent(ctrlfd, token);
	} else if (len == 5 && strncmp(key, "text ", 5) == 0) {
		token = xstrndup(key + 5, endpos - key + 5);
		if (strchr(token, '/') == NULL) /* ensure stay in the directory */
			value = textcontent(listfd, token);
	} else {
		found = false;
		vec_foreach(txt->substs, _i) {
			subst = txt->substs.d[_i];
			if (strlen (subst->token) == len &&
			    strncmp(key, subst->token, len) == 0) {
				sb_cat(str, subst->subst);
				found = true;
				break;
			}
		}
	}
	free(value);
	free(token);
	if (!found)
		sb_printf(str, "$%.*s$", (int)(endpos - key), key);
	return (len +1);
}

static void substitute_one(char **line_p, char **pos_p, int *width_p,
			int listfd, int ctrlfd, text *txt)
{
	/* It is important for this function to leave the length of the
	 * processed portion unchanged, or increase it by just one ASCII
	 * character (for $$). */
	char *line = *line_p;
	char *pos = *pos_p;
	char *token = pos + 1;
	char *endpos;
	char *value = NULL;
	struct substitution *subst;

	endpos = strchr(token, '$');
	if (endpos == NULL) {
		(*pos_p)++;
		(*width_p)++;
		return;
	}

	*pos = '\0';
	*endpos = '\0';

	if(strncmp(token, "control ", 8) == 0) {
		token = filename_token(token + 8);
		if (token != NULL) value = ctrlcontent(ctrlfd, token);
	} else if(strncmp(token, "text ", 5) == 0) {
		token = filename_token(token + 5);
		if (token != NULL) value = textcontent(listfd, token);
	} else {
		vec_foreach(txt->substs, _i) {
			subst = txt->substs.d[_i];
			if(strcmp(token, subst->token) == 0) {
				value = xstrdup(subst->subst);
				break;
			}
		}
	}

	if (value != NULL) {
		xasprintf(&line, "%s%s%s", line, value, endpos + 1);
		*pos_p = line + (*pos_p - *line_p);
		if (strcmp(value, "$") == 0) {
			(*pos_p)++;
			(*width_p)++;
		}
		free(*line_p);
		*line_p = line;
		free(value);
	} else {
		*pos = '$';
		*endpos = '$';
		(*pos_p)++;
		(*width_p)++;
	}
}


char *substitute(const char *line, int listfd, int ctrlfd, text *txt)
{
	const char *pos = line;
	sb_t str = sb_init();

	while (*pos != '\0') {
		if (*pos != '$')
			sb_cat_c(&str, *pos);
		else
			pos += do_substitute(&str, pos, listfd, ctrlfd, txt);
		pos++;
	}

	return sb_get(&str);
}

text *open_text_file(int listfd, const char *filename)
{
	char *tmp;
	int fd;

	xasprintf(&tmp, "text/%s", filename);
	fd = openat(listfd, tmp, O_RDONLY);
	free(tmp);
	if (fd == -1) {
		xasprintf(&tmp, DEFAULTTEXTDIR"/default/%s", filename);
		fd = open(tmp, O_RDONLY);
		free(tmp);
	}
	if (fd == -1) {
		xasprintf(&tmp, DEFAULTTEXTDIR"/en/%s", filename);
		fd = open(tmp, O_RDONLY);
		free(tmp);
	}
	if (fd == -1)
		return (NULL);
	return (open_text_fd(fd));
}

text *open_text_fd(int fd)
{
	text *txt;

	txt = xcalloc(1, sizeof(text));
	txt->src = xcalloc(1, sizeof(source));
	txt->src->limit = -1;
	txt->wrapmode = WRAP_WORD;
	txt->widthreckoning = WIDTH_THIN;
	txt->src->fp = fdopen(fd, "r");
	if (txt->src->fp == NULL) {
		close(fd);
		free(txt->src);
		free(txt);
		return (NULL);
	}

	return (txt);
}

text *open_text(int listfd, const char *purpose, const char *action,
		   const char *reason, const char *type, const char *compat)
{
	size_t filenamelen, len;
	char *filename;
	text *txt;

	filenamelen = xasprintf(&filename, "%s-%s-%s-%s",
	    purpose != NULL ? purpose : "",
	    action != NULL ? action : "",
	    reason != NULL ? reason : "",
	    type != NULL ? type: "");
	do {
		if ((txt = open_text_file(listfd, filename)) != NULL) break;
		len = type ? strlen(type) : 0;
		filename[filenamelen-len-1] = '\0';
		if ((txt = open_text_file(listfd, filename)) != NULL) break;
		filename[filenamelen-len-1] = '-';
		filenamelen -= len + 1;
		len = reason ? strlen(reason) : 0;
		filename[filenamelen-len-1] = '\0';
		if ((txt = open_text_file(listfd, filename)) != NULL) break;
		filename[filenamelen-len-1] = '-';
		filenamelen -= len + 1;
		len = action ? strlen(action) : 0;
		filename[filenamelen-len-1] = '\0';
		if ((txt = open_text_file(listfd, filename)) != NULL) break;
		filename[filenamelen-len-1] = '-';
		if ((txt = open_text_file(listfd, compat)) != NULL) {
			free(filename);
			filename = xstrdup(compat);
			break;
		}
		log_error(LOG_ARGS, "Could not open listtext '%s'", filename);
		free(filename);
		return NULL;
	} while (0);

	txt->action = action != NULL ? xstrdup(action) : NULL;
	txt->reason = reason != NULL ? xstrdup(reason) : NULL;
	txt->type = type != NULL ? xstrdup(type) : NULL;

	return txt;
}


void close_source(text *txt)
{
	source *tmp;
	if (txt->src->fp != NULL) fclose(txt->src->fp);
	if (txt->src->prefix != NULL) free(txt->src->prefix);
	if (txt->src->suffix != NULL) free(txt->src->suffix);
	tmp = txt->src;
	txt->src = txt->src->prev;
	free(tmp);
}


void register_unformatted(text *txt, const char *token, const char *replacement)
{
	struct substitution * subst = xmalloc(sizeof(struct substitution));
	subst->token = xstrdup(token);
	subst->subst = xstrdup(replacement);
	vec_push(&txt->substs, subst);
}

void
register_default_unformatted(text *txt, struct ml *ml)
{
	char *tmp;

	register_unformatted(txt, "", "$");
	register_unformatted(txt, "listaddr", ml->addr);
	xasprintf(&tmp, "%s%s", ml->name, ml->delim);
	register_unformatted(txt, "list+", tmp);
	register_unformatted(txt, "list", ml->name);
	register_unformatted(txt, "domain", ml->fqdn);
	free(tmp);
	gen_addr(tmp, ml, "owner");
	register_unformatted(txt, "listowner", tmp);
	free(tmp);
	gen_addr(tmp, ml, "help");
	register_unformatted(txt, "helpaddr", tmp);
	free(tmp);
	gen_addr(tmp, ml, "faq");
	register_unformatted(txt, "faqaddr", tmp);
	free(tmp);
	gen_addr(tmp, ml, "get-N");
	register_unformatted(txt, "listgetN", tmp);
	free(tmp);
	gen_addr(tmp, ml, "unsubscribe");
	register_unformatted(txt, "listunsubaddr", tmp);
	free(tmp);
	gen_addr(tmp, ml, "unsubscribe-digest");
	register_unformatted(txt, "digestunsubaddr", tmp);
	free(tmp);
	gen_addr(tmp, ml, "unsubscribe-nomail");
	register_unformatted(txt, "nomailunsubaddr", tmp);
	free(tmp);
	gen_addr(tmp, ml, "subscribe");
	register_unformatted(txt, "listsubaddr", tmp);
	free(tmp);
	gen_addr(tmp, ml, "subscribe-digest");
	register_unformatted(txt, "digestsubaddr", tmp);
	free(tmp);
	gen_addr(tmp, ml, "subscribe-nomail");
	register_unformatted(txt, "nomailsubaddr", tmp);
	free(tmp);
}

void register_originalmail(text *txt, const char *mailname)
{
	txt->mailname = xstrdup(mailname);
}


void register_formatted(text *txt, const char *token,
		rewind_function rew, get_function get, void *state)
{
	struct formatted * fmt = xmalloc(sizeof(struct formatted));
	fmt->token = xstrdup(token);
	fmt->rew = rew;
	fmt->get = get;
	fmt->state = state;
	vec_push(&txt->fmts, fmt);
}


static void begin_new_source_file(text *txt, char **line_p, char **pos_p,
		int *width_p, const char *filename, int transparent) {
	char *line = *line_p;
	char *pos = *pos_p;
	char *tmp = NULL, *esc;
	size_t tmpcap = 0;
	source *src;
	FILE *fp;
	int i;

	/* Save any later lines for use after finishing the source */
	while (*pos != '\0' && *pos != '\r' && *pos != '\n') pos++;
	if (*pos == '\r') pos++;
	if (*pos == '\n') pos++;
	if (*pos != '\0') {
		txt->src->upcoming = xstrdup(pos);
		txt->src->processedlen = 0;
		txt->src->processedwidth = 0;
	}

	fp = fopen(filename, "r");
	if (fp == NULL) {
		/* Act as if the source were an empty line */
		**pos_p = '\0';
		return;
	}

	src = xmalloc(sizeof(source));
	src->prev = txt->src;
	src->upcoming = NULL;
	src->prefixlen = strlen(line);
	src->prefixwidth = *width_p;
	src->prefix = xmalloc((*width_p + 1) * sizeof(char));
	for (tmp = src->prefix, i = 0; i < *width_p; tmp++, i++) *tmp = ' ';
	*tmp = '\0';
	src->suffix = NULL;
	src->fp = fp;
	src->fmt = NULL;
	src->transparent = transparent;
	src->limit = -1;
	txt->src = src;
	tmp = NULL;
	if (getline(&tmp, &tmpcap, fp) <= 0) {
		close_source(txt);
		**pos_p = '\0';
		return;
	}
	if (!transparent) {
		esc = unistr_escaped_to_utf8(tmp);
		free(tmp);
		tmp = esc;
	}
	xasprintf(&line, "%s%s", line, tmp);
	*pos_p = line + (*pos_p - *line_p);
	free(*line_p);
	*line_p = line;
	free(tmp);
}


static void begin_new_formatted_source(text *txt, char **line_p, char **pos_p,
		int *width_p, char *suffix, struct formatted *fmt, int transparent) {
	char *line = *line_p;
	char *pos = *pos_p;
	const char *str;
	source *src;

	/* Save any later lines for use after finishing the source */
	while (*pos != '\0' && *pos != '\r' && *pos != '\n') pos++;
	if (*pos == '\r') pos++;
	if (*pos == '\n') pos++;
	if (*pos != '\0') {
		txt->src->upcoming = xstrdup(pos);
		txt->src->processedlen = 0;
		txt->src->processedwidth = 0;
	}

	(*fmt->rew)(fmt->state);

	src = xmalloc(sizeof(source));
	src->prev = txt->src;
	src->upcoming = NULL;
	if (*line == '\0') {
		src->prefix = NULL;
	} else {
		src->prefix = xstrdup(line);
	}
	src->prefixlen = strlen(line);
	src->prefixwidth = *width_p;
	if (*suffix == '\0' || *suffix == '\r' || *suffix == '\n') {
		src->suffix = NULL;
	} else {
		src->suffix = xstrdup(suffix);
	}
	src->fp = NULL;
	src->fmt = fmt;
	src->transparent = transparent;
	src->limit = -1;
	txt->src = src;
	str = (*fmt->get)(fmt->state);
	if (str == NULL) {
		close_source(txt);
		**line_p = '\0';
		*pos_p = *line_p;
		*width_p = 0;
		return;
	}
	if (!transparent) str = unistr_escaped_to_utf8(str);
	xasprintf(&line, "%s%s", line, str);
	/* The suffix will be added back in get_processed_text_line() */
	*pos_p = line + strlen(*line_p);
	free(*line_p);
	*line_p = line;
}


static int handle_conditional(text *txt, char **line_p, char **pos_p,
		int *skipwhite_p, char *token,
		int neg, enum conditional_target tgt, int multi,
		int ctrlfd)
{
	/* This function handles a conditional directive and returns a boolean
	 * (0 or 1) representing whether it was successfully handled or not.
	 * The conditional should already have been identified, and the type of
	 * conditional, whether it is negative, whether multiple parameters are
	 * acceptable, and the position of the first parameter should be passed
	 * in.
	*/
	char *line = *line_p;
	char *pos;
	int satisfied = 0;
	int matches;
	conditional *cond;

	if (txt->skip == NULL) {
		for (;;) {
			pos = token;
			if (*pos == '\0') break;
			while (
				(*pos >= '0' && *pos <= '9') ||
				(*pos >= 'A' && *pos <= 'Z') ||
				(*pos >= 'a' && *pos <= 'z') ||
				(*pos == '_') || (*pos == '-') ||
				(*pos == '.' && pos != token)
			) {
				pos++;
			}
			if (*pos == ' ') {
				*pos = '\0';
			} else {
				multi = 0;
			}
			if (*pos != '\0') return 1;

			matches = 0;
			if (tgt == ACTION) {
				if (txt->action == NULL) return 1;
				if (strcasecmp(token, txt->action) == 0)
						matches = 1;
			} else if (tgt == REASON) {
				if (txt->reason == NULL) return 1;
				if (strcasecmp(token, txt->reason) == 0)
						matches = 1;
			} else if (tgt == TYPE) {
				if (txt->type == NULL) return 1;
				if (strcasecmp(token, txt->type) == 0)
						matches = 1;
			} else if (tgt == CONTROL) {
				if (statctrl(ctrlfd, token))
						matches = 1;
			}
			if ((matches && !neg) || (!matches && neg)) {
				satisfied = 1;
				break;
			}

			if (!multi) break;
			*pos = ' ';
			token = pos + 1;
		}
	} else {
		/* We consider nested conditionals as successful while skipping
		 * text so they don't register themselves as the reason for
		 * skipping, nor trigger swallowing blank lines */
		satisfied = 1;
		pos = token + 1;
		while (*pos != '\0') pos++;
		multi = 0;
	}

	cond = xmalloc(sizeof(conditional));
	cond->satisfied = satisfied;
	cond->elsepart = 0;
	cond->outer = txt->cond;
	txt->cond = cond;
	if (!satisfied) txt->skip = cond;

	if (multi) {
		*pos = ' ';
		pos++;
		while (*pos != '\0') pos++;
	}
	pos++;
	if (*skipwhite_p) {
		while (*pos == ' ' || *pos == '\t') pos++;
	}
	xasprintf(&line, "%s%s", line, pos);
	*pos_p = line + (*pos_p - *line_p);
	free(*line_p);
	*line_p = line;

	return 0;
}


static int handle_directive(text *txt, char **line_p, char **pos_p,
		int *width_p, int *skipwhite_p,
		int conditionalsonly, struct ml *ml)
{
	/* This function returns 1 to swallow a preceding blank line, i.e. if
	 * we just finished processing a failed conditional without an else
	 * part, -1 if we did nothing due to only processing conditionals, and
	 * 0 otherwise. */
	char *line = *line_p;
	char *pos = *pos_p;
	char *token = pos + 1;
	char *endpos;
	char *filename;
	int limit;
	struct formatted *fmt;
	conditional *cond;
	int swallow;

	endpos = strchr(token, '%');
	if (endpos == NULL) {
		if (conditionalsonly) return -1;
		(*pos_p)++;
		return 0;
	}

	*pos = '\0';
	*endpos = '\0';

	if(strncmp(token, "ifaction ", 9) == 0) {
		token += 9;
		if (handle_conditional(txt, line_p, pos_p, skipwhite_p, token,
				0, ACTION, 1, ml->ctrlfd) == 0) return 0;
	} else if(strncmp(token, "ifreason ", 9) == 0) {
		token += 9;
		if (handle_conditional(txt, line_p, pos_p, skipwhite_p, token,
				0, REASON, 1, ml->ctrlfd) == 0) return 0;
	} else if(strncmp(token, "iftype ", 7) == 0) {
		token += 7;
		if (handle_conditional(txt, line_p, pos_p, skipwhite_p, token,
				0, TYPE, 1, ml->ctrlfd) == 0) return 0;
	} else if(strncmp(token, "ifcontrol ", 10) == 0) {
		token += 10;
		if (handle_conditional(txt, line_p, pos_p, skipwhite_p, token,
				0, CONTROL, 1, ml->ctrlfd) == 0) return 0;
	} else if(strncmp(token, "ifnaction ", 10) == 0) {
		token += 10;
		if (handle_conditional(txt, line_p, pos_p, skipwhite_p, token,
				1, ACTION, 0, ml->ctrlfd) == 0) return 0;
	} else if(strncmp(token, "ifnreason ", 10) == 0) {
		token += 10;
		if (handle_conditional(txt, line_p, pos_p, skipwhite_p, token,
				1, REASON, 0, ml->ctrlfd) == 0) return 0;
	} else if(strncmp(token, "ifntype ", 8) == 0) {
		token += 8;
		if (handle_conditional(txt, line_p, pos_p, skipwhite_p, token,
				1, TYPE, 0, ml->ctrlfd) == 0) return 0;
	} else if(strncmp(token, "ifncontrol ", 11) == 0) {
		token += 11;
		if (handle_conditional(txt, line_p, pos_p, skipwhite_p, token,
				1, CONTROL, 1, ml->ctrlfd) == 0) return 0;
	} else if(strcmp(token, "else") == 0) {
		if (txt->cond != NULL) {
			if (txt->skip == txt->cond) txt->skip = NULL;
			else if (txt->skip == NULL) txt->skip = txt->cond;
			txt->cond->elsepart = 1;
			endpos++;
			if (*skipwhite_p) {
				while (*endpos == ' ' || *endpos == '\t')
						endpos++;
			}
			xasprintf(&line, "%s%s", line, endpos);
			*pos_p = line + (*pos_p - *line_p);
			free(*line_p);
			*line_p = line;
			return 0;
		}
	} else if(strcmp(token, "endif") == 0) {
		if (txt->cond != NULL) {
			if (txt->skip == txt->cond) txt->skip = NULL;
			cond = txt->cond;
			swallow = (!cond->satisfied && !cond->elsepart)?1:0;
			txt->cond = cond->outer;
			free(cond);
			endpos++;
			if (*skipwhite_p) {
				while (*endpos == ' ' || *endpos == '\t')
						endpos++;
			}
			xasprintf(&line, "%s%s", line, endpos);
			*pos_p = line + (*pos_p - *line_p);
			free(*line_p);
			*line_p = line;
			return swallow;
		}
	}

	if (conditionalsonly) {
		*pos = '%';
		*endpos = '%';
		return -1;
	}

	if (txt->skip != NULL) {
		/* We don't process anything but conditionals if we're
		 * already skipping text in one. */
		*pos = '%';
		*endpos = '%';
		(*pos_p)++;
		(*width_p)++;
		return 0;
	}

	*skipwhite_p = 0;

	if(strcmp(token, "") == 0) {
		xasprintf(&line, "%s%%%s", line, endpos + 1);
		*pos_p = line + (*pos_p - *line_p) + 1;
		(*width_p)++;
		free(*line_p);
		*line_p = line;
		return 0;
	} else if(strcmp(token, "^") == 0) {
		if (txt->src->prefixlen != 0) {
			line[txt->src->prefixlen] = '\0';
			xasprintf(&line, "%s%s", line, endpos + 1);
			*width_p = txt->src->prefixwidth;
		} else {
			line = xstrdup(endpos + 1);
			*width_p = 0;
		}
		*pos_p = line;
		free(*line_p);
		*line_p = line;
		return 0;
	} else if(strcmp(token, "comment") == 0 || strcmp(token, "$") == 0 ) {
		pos = endpos + 1;
		while (*pos != '\0' && *pos != '\r' && *pos != '\n') pos++;
		xasprintf(&line, "%s%s", line, pos);
		*pos_p = line + (*pos_p - *line_p);
		free(*line_p);
		*line_p = line;
		return 0;
	} else if(strncmp(token, "wrap", 4) == 0) {
		token += 4;
		limit = 0;
		if (*token == '\0') {
			limit = 76;
		} else if (*token == ' ') {
			token = numeric_token(token + 1);
			if (token != NULL) limit = atol(token);
		}
		if (limit != 0) {
			txt->wrapindent = *width_p;
			txt->wrapwidth = limit;
			xasprintf(&line, "%s%s", line, endpos + 1);
			*pos_p = line + (*pos_p - *line_p);
			free(*line_p);
			*line_p = line;
			return 0;
		}
	} else if(strcmp(token, "nowrap") == 0) {
		txt->wrapwidth = 0;
		xasprintf(&line, "%s%s", line, endpos + 1);
		*pos_p = line + (*pos_p - *line_p);
		free(*line_p);
		*line_p = line;
		return 0;
	} else if(strcmp(token, "ww") == 0 ||
			strcmp(token, "wordwrap") == 0 ||
			strcmp(token, "cw") == 0 ||
			strcmp(token, "charwrap") == 0 ||
			strcmp(token, "uw") == 0 ||
			strcmp(token, "userwrap") == 0) {
		if (*token == 'w') txt->wrapmode = WRAP_WORD;
		if (*token == 'c') txt->wrapmode = WRAP_CHAR;
		if (*token == 'u') txt->wrapmode = WRAP_USER;
		xasprintf(&line, "%s%s", line, endpos + 1);
		*pos_p = line + (*pos_p - *line_p);
		free(*line_p);
		*line_p = line;
		return 0;
	} else if(strcmp(token, "thin") == 0) {
		txt->widthreckoning = WIDTH_THIN;
		xasprintf(&line, "%s%s", line, endpos + 1);
		*pos_p = line + (*pos_p - *line_p);
		free(*line_p);
		*line_p = line;
		return 0;
	} else if(strcmp(token, "wide") == 0) {
		txt->widthreckoning = WIDTH_WIDE;
		xasprintf(&line, "%s%s", line, endpos + 1);
		*pos_p = line + (*pos_p - *line_p);
		free(*line_p);
		*line_p = line;
		return 0;
	} else if(strncmp(token, "zero ", 5) == 0) {
		token += 5;
		if (txt->zerowidth != NULL) free(txt->zerowidth);
		txt->zerowidth = xstrdup(token);
		xasprintf(&line, "%s%s", line, endpos + 1);
		*pos_p = line + (*pos_p - *line_p);
		free(*line_p);
		*line_p = line;
		return 0;
	} else if(strncmp(token, "control ", 8) == 0) {
		token = filename_token(token + 8);
		if (token != NULL) {
			xasprintf(&filename, "%s/control/%s", ml->dir, token);
			begin_new_source_file(txt,
					line_p, pos_p, width_p, filename, 0);
			free(filename);
			return 0;
		}
	} else if(strncmp(token, "text ", 5) == 0) {
		token = filename_token(token + 5);
		if (token != NULL) {
			xasprintf(&filename, "%s/text/%s", ml->dir, token);
			begin_new_source_file(txt,
					line_p, pos_p, width_p, filename, 0);
			free(filename);
			return 0;
		}
	} else if(strncmp(token, "originalmail", 12) == 0 &&
			txt->mailname != NULL) {
		token += 12;
		limit = 0;
		if (*token == '\0') {
			limit = -1;
		} else if (*token == ' ') {
			token = numeric_token(token + 1);
			if (token != NULL) limit = atol(token);
		} else {
			token = numeric_token(token);
			if (token != NULL) limit = atol(token);
		}
		if (limit != 0) {
			begin_new_source_file(txt, line_p, pos_p, width_p,
					txt->mailname, 1);
			if (limit == -1) txt->src->limit = -1;
			else txt->src->limit = limit - 1;
			return 0;
		}
	}
	if (token == NULL) {
		/* We have encountered a directive, but not been able to deal
		 * with it, so just advance through the string. */
		*pos = '%';
		*endpos = '%';
		(*pos_p)++;
		(*width_p)++;
		return 0;
	}

	vec_foreach(txt->fmts, _i) {
		fmt = txt->fmts.d[_i];
		if (strcmp(token, fmt->token) == 0) {
			begin_new_formatted_source(txt, line_p, pos_p, width_p,
					endpos + 1, fmt, 0);
			return 0;
		}
	}

	/* No recognised directive; just advance through the string. */
	*pos = '%';
	*endpos = '%';
	(*pos_p)++;
	(*width_p)++;
	return 0;
}


char *get_processed_text_line(text *txt, bool headers, struct ml *ml)
{
	char *line;
	const char *item;
	char *pos;
	char *tmp;
	char *prev = NULL;
	int len, width, i;
	int processedlen = 0, processedwidth = 0;
	int wrapindentlen = -1;
	int incision, linebreak, linebreakwidth;
	int directive, inhibitbreak;
	int peeking = 0; /* for a failed conditional without an else */
	int skipwhite; /* skip whitespace after a conditional directive */
	int swallow;
	char utf8char[5] = {0, 0, 0, 0, 0};

	for (;;) {
		line = NULL;
		while (txt->src != NULL) {
			if (txt->src->upcoming != NULL) {
				if (prev != NULL) {
					/* If wrapping, we are going to swallow
					 * leading whitespace anyway, which is
					 * what the prefix will always be, so
					 * we needn't include it, nor the
					 * wrapindent; wrapindentlen is also
					 * already set from the previous
					 * iteration. */
					line = txt->src->upcoming;
					txt->src->upcoming = NULL;
					break;
				}
				/* Join the prefix, wrapindent and upcoming
				 * line. */
				len = strlen(txt->src->upcoming);
				processedlen = txt->src->processedlen;
				processedwidth = txt->src->processedwidth;
				if (txt->src->prefixwidth != 0) {
					/* prefixlen may be true for an existing
					 * prefix, not the one in txt->src, so
					 * set it afresh. */
					txt->src->prefixlen =
						strlen(txt->src->prefix);
					len += txt->src->prefixlen;
					processedlen += txt->src->prefixlen;
					processedwidth += txt->src->prefixwidth;
				}
				if (txt->wrapwidth != 0) {
					/* wrapindent is a width, but includes
					 * the prefix; the excess we make up
					 * with just spaces though, so one byte
					 * per character. */
					len += txt->wrapindent -
						txt->src->prefixwidth;
					processedlen += txt->wrapindent -
						txt->src->prefixwidth;
					processedwidth += txt->wrapindent -
						txt->src->prefixwidth;
				}
				line = xmalloc((len + 1) * sizeof(char));
				if (txt->src->prefixwidth != 0) {
					strcpy(line, txt->src->prefix);
					pos = line + txt->src->prefixlen;
				} else {
					pos = line;
				}
				if (txt->wrapwidth != 0) {
					i = txt->wrapindent -
						txt->src->prefixwidth;
					for (; i > 0; i--) *pos++ = ' ';
					wrapindentlen = pos - line;
				}
				strcpy(pos, txt->src->upcoming);
				free(txt->src->upcoming);
				txt->src->upcoming = NULL;
				break;
			}
			if (txt->src->limit != 0) {
				tmp = NULL;
				if (txt->src->fp != NULL) {
					size_t linecap = 0;
					if (getline(&tmp, &linecap, txt->src->fp) <= 0) {
						free(tmp);
						tmp = NULL;
					}
				} else if (txt->src->fmt != NULL) {
					item = (*txt->src->fmt->get)(
						txt->src->fmt->state);
					if (item != NULL)
						tmp = xstrdup(item);
				}
				if (txt->src->limit > 0) txt->src->limit--;
				if (tmp == NULL) {
					txt->src->upcoming = NULL;
				} else if (txt->src->transparent) {
					txt->src->upcoming = tmp;
					txt->src->processedlen = 0;
					txt->src->processedwidth = 0;
				} else {
					txt->src->upcoming =
						unistr_escaped_to_utf8(tmp);
					txt->src->processedlen = 0;
					txt->src->processedwidth = 0;
					free(tmp);
				}
			} else {
				txt->src->upcoming = NULL;
			}
			if (txt->src->upcoming != NULL) continue;
			close_source(txt);
		}
		if (line == NULL) {
			if (peeking) return xstrdup("");
			if (prev != NULL) return prev;
			return NULL;
		}

		if (prev != NULL) {
			/* Wrapping; join and start processing at the new bit,
			 * which is always unprocessed. */
			len = strlen(prev);
			pos = prev + len - 1;
			/* The width remains set from the previous iteration. */
			if (txt->wrapmode == WRAP_WORD) {
				while (pos >= prev + wrapindentlen &&
						(*pos == ' ' || *pos == '\t')) {
					pos--;
					len--;
					width--;
				}
			}
			pos++;
			*pos = '\0';
			pos = line;
			while (*pos == ' ' || *pos == '\t') pos++;
			if ((*pos == '\r' || *pos == '\n' || *pos == '\0') &&
					txt->skip == NULL) {
				/* Empty/white line; stop wrapping, finish
				   the last line and save the empty/white
				   line for later. */
				txt->wrapwidth = 0;
				txt->src->upcoming = line;
				txt->src->processedlen = 0;
				txt->src->processedwidth = 0;
				line = prev;
				pos = line + len;
				skipwhite = 0;
			} else {
				if (*prev == '\0') {
					tmp = xstrdup(pos);
				} else {
					if (txt->wrapmode == WRAP_WORD &&
							len > wrapindentlen) {
						xasprintf(&tmp, "%s %s", prev, pos);
						len++;
						width++;
					} else {
						xasprintf(&tmp, "%s%s", prev, pos);
					}
				}
				free(line);
				line = tmp;
				free(prev);
				pos = line + len;
				skipwhite = 1;
			}
			/* We can always line-break where the input had one */
			linebreak = len;
			linebreakwidth = width;
			prev = NULL;
		} else {
			/* Not wrapping; start processing where we left off;
			 * there can't be any break opportunities in the
			 * processed part, and if it looks like there are, they
			 * must have been inhibited so aren't really. */
			pos = line + processedlen;
			len = processedlen;
			width = processedwidth;
			linebreak = 0;
			linebreakwidth = 0;
			skipwhite = 0;
		}

		if (txt->skip != NULL) {
			incision = len;
		} else {
			incision = -1;
		}
		directive = 0;
		inhibitbreak = 0;
		while (*pos != '\0') {
			if (txt->wrapwidth != 0 && width >= txt->wrapwidth &&
					!peeking && linebreak > wrapindentlen &&
					linebreak < len)
					break;
			if ((unsigned char)*pos > 0xbf && txt->skip == NULL &&
					txt->wrapmode == WRAP_CHAR &&
					!inhibitbreak) {
				linebreak = len;
				linebreakwidth = width;
			}
			if (*pos == '\r') {
				*pos = '\0';
				pos++;
				if (*pos == '\n') pos++;
				if (*pos == '\0') break;
				txt->src->upcoming = xstrdup(pos);
				txt->src->processedlen = 0;
				txt->src->processedwidth = 0;
				break;
			} else if (*pos == '\n') {
				*pos = '\0';
				pos++;
				if (*pos == '\0') break;
				txt->src->upcoming = xstrdup(pos);
				txt->src->processedlen = 0;
				txt->src->processedwidth = 0;
				break;
			} else if (*pos == ' ') {
				if (txt->skip == NULL &&
						txt->wrapmode != WRAP_USER &&
						!inhibitbreak) {
					linebreak = len + 1;
					linebreakwidth = width + 1;
				}
				inhibitbreak = 0;
			} else if (*pos == '\t') {
				/* Avoid breaking due to peeking */
				inhibitbreak = 0;
			} else if (txt->src->transparent) {
				/* Do nothing if the file is to be included
			 	 * transparently */
				if (peeking && txt->skip == NULL) break;
				inhibitbreak = 0;
			} else if (*pos == '\\' && txt->skip == NULL) {
				if (peeking) break;
				if (*(pos + 1) == '/') {
					linebreak = len;
					linebreakwidth = width;
					tmp = pos + 2;
					inhibitbreak = 0;
				} else if (*(pos + 1) == '=') {
					tmp = pos + 2;
					/* Ensure we don't wrap the next
					 * character */
					inhibitbreak = 1;
				} else {
					/* Includes space and backslash */
					tmp = pos + 1;
					/* Ensure we don't wrap a space */
					if (*(pos+1) == ' ') inhibitbreak = 1;
					else inhibitbreak = 0;
				}
				*pos = '\0';
				xasprintf(&tmp, "%s%s", line, tmp);
				pos = tmp + len;
				free(line);
				line = tmp;
				skipwhite = 0;
				continue;
			} else if (*pos == '$' && txt->skip == NULL) {
				if (peeking) break;
				substitute_one(&line, &pos, &width, ml->fd, ml->ctrlfd, txt);
				if (len != pos - line) {
					/* Cancel any break inhibition if the
					 * length changed (which will be
					 * because of $$) */
					inhibitbreak = 0;
					len = pos - line;
				}
				skipwhite = 0;
				/* The function sets up for the next character
				 * to process, so continue straight away. */
				continue;
			} else if (*pos == '%') {
				directive = 1;
				swallow = handle_directive(txt, &line, &pos,
						&width, &skipwhite,
						peeking, ml);
				if (swallow == 1) peeking = 0;
				if (swallow == -1) break;
				if (txt->skip != NULL) {
					if (incision == -1) {
						/* We have to cut a bit out
						 * later */
						incision = pos - line;
					}
				} else {
					if (incision != -1) {
					    /* Time to cut */
					    if (pos - line != incision) {
						line[incision] = '\0';
						xasprintf(&tmp, "%s%s", line, pos);
						pos = tmp + incision;
						free(line);
						line = tmp;
					    }
					    incision = -1;
					}
				}
				if (len != pos - line) {
					/* Cancel any break inhibition if the
					 * length changed (which will be
					 * because of %% or %^% or an empty
					 * list) */
					inhibitbreak = 0;
					len = pos - line;
				}
				if (txt->wrapwidth != 0 &&
					    wrapindentlen == -1) {
					/* Started wrapping. */
					wrapindentlen = len;
				}
				/* handle_directive() sets up for the next
				 * character to process, so continue straight
				 * away. */
				continue;
			} else if (peeking && txt->skip == NULL) {
				break;
			}
			if (txt->skip == NULL) {
				len++;
				if ((unsigned char)*pos < 0x80) {
					width++;
				} else if ((unsigned char)*pos > 0xbf) {
					/* Not a UTF-8 continuation byte. */
					if (txt->zerowidth != NULL) {
					    tmp = pos;
					    utf8char[0] = *tmp++;
					    for (i = 1; i < 4; i++, tmp++) {
						if ((unsigned char)*tmp<0x80 ||
						      (unsigned char)*tmp>0xbf)
						      break;
						utf8char[i] = *tmp;
					    }
					    utf8char[i] = '\0';
					    if (strstr(txt->zerowidth, utf8char)
						    == NULL) {
						width++;
						if (txt->widthreckoning ==
						      WIDTH_WIDE)
						      width++;
					    }
					} else {
					    width++;
					    if (txt->widthreckoning ==
						    WIDTH_WIDE)
						    width++;
					}
				}
			}
			pos++;
			skipwhite = 0;
		}

		if (incision == 0) {
			/* The whole line was skipped; nothing to return yet;
			 * keep reading */
			incision = -1;
			free(line);
			continue;
		}

		if (incision != -1) {
			/* Time to cut */
			if (pos - line != incision) {
				line[incision] = '\0';
				tmp = xstrdup(line);
				pos = tmp + incision;
				free(line);
				line = tmp;
			}
			incision = -1;
		}

		if (txt->wrapwidth != 0 && !peeking) {
			if (width < txt->wrapwidth ||
					linebreak <= wrapindentlen ||
					linebreak >= len) {
				prev = line;
				continue;
			}
			if (linebreak != 0) {
				if (txt->wrapmode == WRAP_WORD &&
					line[linebreak-1] == ' ')
					line[linebreak-1] = '\0';
				if (line[linebreak] == '\0') linebreak = 0;
			}
			if (linebreak != 0) {
				if (txt->src->upcoming == NULL) {
				    tmp = xstrdup(line + linebreak);
				} else {
				    /* If something's coming up, it's because
				     * it was a new line. */
				    if (*(line + linebreak) != '\0') {
					xasprintf(&tmp, "%s\n%s",
					    line + linebreak, txt->src->upcoming);
					free(txt->src->upcoming);
				    } else {
				    	tmp = txt->src->upcoming;
				    }
				}
				txt->src->upcoming = tmp;
				txt->src->processedlen = len - linebreak;
				txt->src->processedwidth =
						width - linebreakwidth;
			}
			line[linebreak] = '\0';
			tmp = xstrdup(line);
			free(line);
			line = tmp;
		} else {
			if (directive) {
				pos = line;
				while (*pos == ' ' || *pos == '\t') pos++;
				if (*pos == '\0') {
					/* Omit whitespace-only line with
					 * directives */
					free(line);
					continue;
				}
			}
			if (*line == '\0' && !headers && !peeking) {
				/* Non-wrapped bona fide blank line that isn't
				 * ending the headers; peek ahead to check it's
				 * not followed by an unsatisfied conditional
				 * without an else */
				peeking = 1;
				free(line);
				continue;
			} else if (peeking) {
				/* We found something; return preceding blank
				 * line */
				if (txt->src->upcoming == NULL) {
					txt->src->upcoming = line;
					txt->src->processedlen = len;
					txt->src->processedwidth = width;
				} else {
					tmp = txt->src->upcoming;
					xasprintf(&txt->src->upcoming, "%s\n%s",
					    line, txt->src->upcoming);
					txt->src->processedlen = len;
					txt->src->processedwidth = width;
					free(line);
					free(tmp);
				}
				line = xstrdup("");
			}
		}

		if (txt->src->suffix != NULL) {
			xasprintf(&tmp, "%s%s", line, txt->src->suffix);
			free(line);
			return tmp;
		} else {
			return line;
		}
	}
}

static void
substitution_free(struct substitution *subst)
{
	free(subst->token);
	free(subst->subst);
	free(subst);
}

static void
formatted_free(struct formatted *fmt)
{
	free(fmt->token);
	free(fmt);
}

void close_text(text *txt)
{
	conditional *cond;
	while (txt->src != NULL) {
		close_source(txt);
	}
	vec_free_and_free(&txt->substs, substitution_free);
	if (txt->mailname != NULL) free(txt->mailname);
	vec_free_and_free(&txt->fmts, formatted_free);
	while (txt->cond != NULL) {
		cond = txt->cond;
		txt->cond = txt->cond->outer;
		free(cond);
	}
	free(txt);
}

bool
prepstdreply_to(text *txt, struct ml *ml, const char *from, const char *to,
    const char *replyto, FILE *tof, const char *msgid)
{
	bool ret = false;
	char *tmp, *line;
	int i;
	size_t len;
	char *headers[10] = { NULL }; /* relies on NULL to flag end */

	for (i=0; i<6; i++) {
		tmp = xstrdup("randomN");
		tmp[6] = '0' + i;
		char *str = random_str();
		register_unformatted(txt, tmp, str);
		free(tmp);
		free(str);
	}

	tmp = substitute(from, ml->fd, ml->ctrlfd, txt);
	xasprintf(&headers[0], "From: %s", tmp);
	free(tmp);
	tmp = substitute(to, ml->fd, ml->ctrlfd, txt);
	xasprintf(&headers[1], "To: %s", tmp);
	free(tmp);
	if (msgid)
		headers[2] = xstrdup(msgid);
	else
		headers[2] = genmsgid(ml->fqdn);
	chomp(headers[2]);
	headers[3] = gendatestr();
	chomp(headers[3]);
	headers[4] = xstrdup("Subject: mlmmj administrivia");
	headers[5] = xstrdup("MIME-Version: 1.0");
	headers[6] = xstrdup("Content-Type: text/plain; charset=utf-8");
	headers[7] = xstrdup("Content-Transfer-Encoding: 8bit");

	if(replyto) {
		tmp = substitute(replyto, ml->fd, ml->fd, txt);
		xasprintf(&headers[8], "Reply-To: %s", tmp);
		free(tmp);
	}

	for(;;) {
		line = get_processed_text_line(txt, true, ml);
		if (!line) {
			log_error(LOG_ARGS, "No body in listtext");
			break;
		}
		if (*line == '\0') {
			/* end of headers */
			free(line);
			line = NULL;
			break;
		}
		if (*line == ' ' || *line == '\t') {
			/* line beginning with linear whitespace is a
			   continuation of previous header line */
			if(fprintf(tof, "%s\n", line) < 0) {
				log_error(LOG_ARGS, "Could not write std mail");
				free(line);
				goto freeandreturn;
			}
		} else {
			tmp = line;
			len = 0;
			while (*tmp && *tmp != ':') {
				tmp++;
				len++;
			}
			if (!*tmp) {
				log_error(LOG_ARGS, "No headers or invalid "
						"header in listtext");
				break;
			}
			tmp++;
			len++;
			/* remove the standard header if one matches */
			for (i=0; headers[i] != NULL; i++) {
				if (strncasecmp(line, headers[i], len) == 0) {
					free(headers[i]);
					while (headers[i] != NULL) {
						headers[i] = headers[i+1];
						i++;
					}
					break;
				}
			}
			if (strncasecmp(line, "Subject:", len) == 0) {
				tmp = unistr_utf8_to_header(tmp);
				free(line);
				xasprintf(&line, "Subject: %s", tmp);
				free(tmp);
			}
			if(fprintf(tof, "%s\n", line) < 0) {
				log_error(LOG_ARGS, "Could not write std mail");
				free(line);
				goto freeandreturn;
			}
		}
		free(line);
		line = NULL;
	}

	for (i=0; headers[i] != NULL; i++) {
		if(fprintf(tof, "%s\n", headers[i]) < 0) {
			log_error(LOG_ARGS, "Could not write std mail");
			if (line)
				free(line);
			goto freeandreturn;
		}
	}

	/* end the headers */
	if(fprintf(tof, "\n") < 0) {
		log_error(LOG_ARGS, "Could not write std mail");
		if (line)
			free(line);
		goto freeandreturn;
	}

	if (line == NULL) {
		line = get_processed_text_line(txt, false, ml);
	}
	while(line) {
			if(fprintf(tof, "%s\n", line) < 0) {
				log_error(LOG_ARGS, "Could not write std mail");
				goto freeandreturn;
			}
		free(line);
		line = get_processed_text_line(txt, false, ml);
	}

	ret = true;

freeandreturn:
	for (i = 0; i < 10; i++)
		free(headers[i]);

	return ret;
}

char *prepstdreply(text *txt, struct ml *ml, const char *from, const char *to, const char *replyto)
{
	int outfd;
	FILE *outf;
	char *qprefix, *retstr = NULL;

	xasprintf(&qprefix, "%s/queue/", ml->dir);
	outfd = mkuniqfileat(AT_FDCWD, qprefix, S_IRUSR|S_IWUSR, &retstr);
	free(qprefix);

	if(outfd < 0) {
		log_error(LOG_ARGS, "Could not open std mail in %s/queue",
		    ml->dir);
		free(retstr);
		close_text(txt);
		return NULL;
	}
	outf = fdopen(outfd, "w");
	if (outf == NULL) {
		close(outfd);
		unlink(retstr);
		free(retstr);
		close_text(txt);
		return NULL;
	}
	if (!prepstdreply_to(txt, ml, from, to, replyto, outf, NULL)) {
		fclose(outf);
		unlink(retstr);
		free(retstr);
		close_text(txt);
		return (NULL);
	}
	fclose(outf);
	return (retstr);
}
