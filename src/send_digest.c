/* Copyright (C) 2004, 2005 Morten K. Poulsen <morten at afdelingp.dk>
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

#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <ctype.h>

#include "mlmmj.h"
#include "send_digest.h"
#include "log_error.h"
#include "strgen.h"
#include "xmalloc.h"
#include "wrappers.h"
#include "prepstdreply.h"
#include "gethdrline.h"
#include "statctrl.h"
#include "unistr.h"
#include "utils.h"



thread_list_state *init_thread_list(
		const char *listdir, int firstindex, int lastindex)
{
	/* We use a static variable rather than dynamic allocation as
	 * there will never be two lists in use simultaneously */
	static thread_list_state s;
	s.listdir = listdir;
	s.firstindex = firstindex;
	s.lastindex = lastindex;
	s.num_threads = 0;
	s.threads = NULL;
	s.cur_thread = -1;
	return &s;
}


void rewind_thread_list(void * state)
{
	thread_list_state *s = (thread_list_state *)state;
	FILE *archivef;
	int i, j, thread_idx;
	char *line = NULL, *tmp, *subj, *from, *tmpfrom;
	size_t linecap = 0, tmplen;
	char *archivename;
	int num_threads = 0, threads_alloc = 0;
	struct digest_thread *threads = NULL;
	if (s->cur_thread != -1) {
		/* We have gathered the data already; just rewind */
		s->cur_thread = 0;
		s->cur_mail = -1;
		return;
	}

	for (i=s->firstindex; i<=s->lastindex; i++) {
		xasprintf(&archivename, "%s/archive/%d", s->listdir, i);
		archivef = fopen(archivename, "r");
		free(archivename);

		if (archivef == NULL)
			continue;

		subj = NULL;
		from = NULL;

		while (getline(&line, &linecap, archivef) > 0) {
			/* stop at end of headers */
			if (line[0] == '\n' ||
			    (line[0] == '\r' && line[1] == '\n'))
				break;
			if (strncasecmp(line, "Subject:", 8) == 0) {
				free(subj);
				subj = unistr_header_to_utf8(line + 8);
			} else if (strncasecmp(line, "From:", 5) == 0) {
				free(from);
				from = unistr_header_to_utf8(line + 5);
			}
		}

		if (!subj) {
			subj = xstrdup("no subject");
		}

		if (!from) {
			from = xstrdup("anonymous");
		}

		tmp = subj;
		for (;;) {
			if (isspace(*tmp)) {
				tmp++;
				continue;
			}
			if (strncasecmp(tmp, "Re:", 3) == 0) {
				tmp += 3;
				continue;
			}
			break;
		}
		/* tmp is now the clean subject */

		tmplen = strlen(tmp);
		thread_idx = -1;
		for (j=0; j<num_threads; j++) {
			if (strncmp(tmp, threads[j].subject, tmplen) == 0 &&
			    threads[j].subject[tmplen] == '\n') {
				thread_idx = j;
				break;
			}
		}
		if (thread_idx == -1) {
			if (num_threads == threads_alloc) {
				threads_alloc = threads_alloc ? threads_alloc * 2 : 16;
				threads = xrealloc(threads,
						threads_alloc*sizeof(struct digest_thread));
			}
			xasprintf(&(threads[num_threads].subject), "%s\n", tmp);
			threads[num_threads].num_mails = 0;
			threads[num_threads].mails = NULL;
			thread_idx = num_threads;
			num_threads++;
		}

		tmp = from;
		while (isspace(*tmp))
			tmp++;
		/* tmp is now left-trimmed from */

		threads[thread_idx].num_mails++;
		if (threads[thread_idx].num_mails > 1 &&
		    (threads[thread_idx].num_mails & (threads[thread_idx].num_mails - 1)) == 0) {
			/* power of 2: double the allocation */
			threads[thread_idx].mails = xrealloc(threads[thread_idx].mails,
					threads[thread_idx].num_mails*2*sizeof(struct digest_mail));
		} else if (threads[thread_idx].num_mails == 1) {
			threads[thread_idx].mails = xmalloc(sizeof(struct digest_mail));
		}
		threads[thread_idx].mails[threads[thread_idx].num_mails-1].idx = i;
		xasprintf(&tmpfrom, "      %d - %s\n", i, tmp);
		threads[thread_idx].mails[threads[thread_idx].num_mails-1].from = tmpfrom;

		free(subj);
		free(from);

		fclose(archivef);
	}
	free(line);

	s->num_threads = num_threads;
	s->threads = threads;
	s->cur_thread = 0;
	s->cur_mail = -1;
}


const char *get_thread_list_line(void * state)
{
	thread_list_state *s = (thread_list_state *)state;

	if (s->cur_thread >= s->num_threads) return NULL;

	if (s->cur_mail == -1) {
		s->cur_mail = 0;
		return s->threads[s->cur_thread].subject;
	}

	if (s->cur_mail >= s->threads[s->cur_thread].num_mails) {
		s->cur_thread++;
		s->cur_mail = -1;
		return "\n";
	}

	return s->threads[s->cur_thread].mails[s->cur_mail++].from;
}


void finish_thread_list(thread_list_state * s)
{
	int i, j;
	if (s->threads == NULL) return;
	for (i=0; i<s->num_threads; i++) {
		free(s->threads[i].subject);
		for (j=0; j<s->threads[i].num_mails; j++) {
			free(s->threads[i].mails[j].from);
		}
		free(s->threads[i].mails);
	}
	free(s->threads);
	s->threads = NULL;
}


int send_digest(struct ml *ml, int firstindex, int lastindex,
		int issue, const char *addr, const char *mlmmjsend)
{
	int i, fd, archivefd = -1, hdrfd = -1;
	int retval = -1;
	size_t len;
	text *txt = NULL;
	char buf[100];
	char *tmp, *queuename = NULL, *archivename, *subject = NULL, *line = NULL;
	char *boundary = NULL;
	thread_list_state *tls;
	FILE *f = NULL;

	if (addr) {
		errno = 0;
		log_error(LOG_ARGS, "send_digest() does not support sending "
				"digest mails to only one recipient yet");
		return -1;
	}

	if (firstindex > lastindex)
		return -1;

	do {
		tmp = random_str();
		free(queuename);
		xasprintf(&queuename, "%s/queue/%s", ml->dir, tmp);
		free(tmp);
		fd = open(queuename, O_RDWR|O_CREAT|O_EXCL, S_IRUSR|S_IWUSR);
	} while ((fd < 0) && (errno == EEXIST));

	if (fd < 0) {
		log_error(LOG_ARGS, "Could not open digest queue file '%s'",
				queuename);
		free(queuename);
		return -1;
	}

	hdrfd = openat(ml->ctrlfd, "customheaders", O_RDONLY);
	boundary = random_str();

	txt = open_text_file(ml->fd, "digest");
	if (txt == NULL) {
		log_error(LOG_ARGS, "Could not open listtext 'digest'");
		goto fallback_subject;
	}
	register_default_unformatted(txt, ml);

	snprintf(buf, sizeof(buf), "%d", firstindex);
	register_unformatted(txt, "digestfirst", buf);

	snprintf(buf, sizeof(buf), "%d", lastindex);
	register_unformatted(txt, "digestlast", buf);

	if (lastindex == firstindex) {
		snprintf(buf, sizeof(buf), "%d", firstindex);
	} else {
		snprintf(buf, sizeof(buf), "%d-%d", firstindex, lastindex);
	}
	register_unformatted(txt, "digestinterval", buf);

	snprintf(buf, sizeof(buf), "%d", issue);
	register_unformatted(txt, "digestissue", buf);

	tls = init_thread_list(ml->dir, firstindex, lastindex);
	register_formatted(txt, "digestthreads", rewind_thread_list,
			get_thread_list_line, tls);

	line = get_processed_text_line(txt, true, ml);

	if (line == NULL) {
		log_error(LOG_ARGS, "No content in digest listtext");
		goto fallback_subject;
	}

	tmp = line;
	len = 0;
	while (*tmp && *tmp != ':') {
		tmp++;
		len++;
	}
	if (!*tmp) {
		log_error(LOG_ARGS, "No subject or invalid "
				"subject in digest listtext");
		goto fallback_subject;
	}
	tmp++;
	len++;
	if (strncasecmp(line, "Subject:", len) == 0) {
		tmp = unistr_utf8_to_header(tmp);
		xasprintf(&subject, "Subject: %s", tmp);
		free(tmp);

		/* Skip the empty line after the subject */
		line = get_processed_text_line(txt, true, ml);
		if (line == NULL || *line != '\0') {
			log_error(LOG_ARGS, "Too many headers "
					"in digest listtext");
			goto fallback_subject;
		}

		free(line);
		line = NULL;
	} else {
		log_error(LOG_ARGS, "No subject or invalid "
				"subject in digest listtext");
		goto fallback_subject;
	}

fallback_subject:
	if (subject == NULL) {
		if (lastindex == firstindex) {
			snprintf(buf, sizeof(buf), "%d", firstindex);
		} else {
			snprintf(buf, sizeof(buf), "%d-%d", firstindex, lastindex);
		}
		tmp = xstrdup(buf);
		snprintf(buf, sizeof(buf), "Digest of %s issue %d (%s)",
				ml->addr, issue, tmp);
		subject = unistr_utf8_to_header(buf);
		free(tmp);
	}

	{
		int dupfd = dup(fd);
		if (dupfd == -1) goto cleanup;
		f = fdopen(dupfd, "w");
		if (f == NULL) { close(dupfd); goto cleanup; }
	}

	if (fprintf(f, "From: %s%shelp@%s"
	    "\nMIME-Version: 1.0"
	    "\nContent-Type: multipart/" DIGESTMIMETYPE "; "
	    "boundary=%s"
	    "\nSubject: %s\n",
	    ml->name, ml->delim, ml->fqdn, boundary, subject) < 0) {
		log_error(LOG_ARGS, "Could not write digest headers to '%s'",
				queuename);
		goto cleanup;
	}

	if (hdrfd >= 0) {
		fflush(f);
		if (dumpfd2fd(hdrfd, fd) < 0) {
			log_error(LOG_ARGS, "Could not write digest headers "
					"to '%s'", queuename);
			goto cleanup;
		}
		close(hdrfd);
		hdrfd = -1;
	}

	if (fprintf(f, "\n") < 0) {
		log_error(LOG_ARGS, "Could not write digest headers to '%s'",
				queuename);
		goto cleanup;
	}

	if (txt != NULL && !statctrl(ml->ctrlfd, "nodigesttext")) {
		if (fprintf(f, "\n--%s"
		    "\nContent-Type: text/plain; charset=UTF-8"
		    "\nContent-Transfer-Encoding: 8bit"
		    "\n\n", boundary) < 0) {
			log_error(LOG_ARGS, "Could not write digest text/plain"
					" part headers to '%s'", queuename);
			goto cleanup;
		}

		for (;;) {
			line = get_processed_text_line(txt, false, ml);
			if (line == NULL) break;
			if (fprintf(f, "%s\n", line) < 0) {
				log_error(LOG_ARGS, "Could not write"
						" std mail");
				break;
			}
			free(line);
			line = NULL;
		}

		finish_thread_list(tls);
		close_text(txt);
		txt = NULL;
	} else if (txt != NULL) {
		finish_thread_list(tls);
		close_text(txt);
		txt = NULL;
	}

	free(line);
	line = NULL;

	for (i = firstindex; i <= lastindex; i++) {
		xasprintf(&archivename, "archive/%d", i);
		archivefd = openat(ml->fd, archivename, O_RDONLY);
		free(archivename);

		if (archivefd < 0)
			continue;

		if (fprintf(f, "\n--%s"
		    "\nContent-Type: message/rfc822"
		    "\nContent-Disposition: inline; filename=\"%s_%s.eml\"\n\n",
		    boundary, ml->name, buf) < 0) {
			log_error(LOG_ARGS, "Could not write digest part "
					"headers for archive index %d to "
					"'%s'", i, queuename);
			goto cleanup;
		}

		fflush(f);
		if (dumpfd2fd(archivefd, fd) < 0) {
			log_error(LOG_ARGS, "Could not write digest part %d "
					"to '%s'", i, queuename);
			goto cleanup;
		}
		close(archivefd);
		archivefd = -1;
	}

	if (fprintf(f, "\n--%s--\n", boundary) < 0) {
		log_error(LOG_ARGS, "Could not write digest end to '%s'",
				queuename);
		goto cleanup;
	}

	fclose(f);
	f = NULL;
	close(fd);
	fd = -1;

	exec_and_wait(mlmmjsend, "-l", "7", "-L", ml->dir, "-m", queuename,
	    NULL);
	retval = 0;

cleanup:
	if (f != NULL)
		fclose(f);
	if (fd >= 0) {
		close(fd);
		unlink(queuename);
	}
	if (archivefd >= 0)
		close(archivefd);
	if (hdrfd >= 0)
		close(hdrfd);
	if (txt != NULL) {
		finish_thread_list(tls);
		close_text(txt);
	}
	free(subject);
	free(line);
	free(boundary);
	free(queuename);

	return retval;
}
