/* Copyright (C) 2004 Mads Martin Joergensen <mmj at mmj.dk>
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
#include <unistd.h>

#include "xmalloc.h"
#include "mlmmj.h"
#include "gethdrline.h"
#include "strgen.h"
#include "ctrlvalue.h"
#include "do_all_the_voodoo_here.h"
#include "log_error.h"
#include "wrappers.h"

bool
findit(const char *line, const strlist *headers)
{
	size_t len;

	tll_foreach(*headers, it) {
		len = strlen(it->item);
		if(strncasecmp(line, it->item, len) == 0)
			return true;
	}

	return false;
}

void getinfo(const char *line, struct mailhdr *readhdrs)
{
	int i = 0;
	size_t tokenlen;

	while(readhdrs[i].token) {
		tokenlen = strlen(readhdrs[i].token);
		if(strncasecmp(line, readhdrs[i].token, tokenlen) == 0) {
			readhdrs[i].valuecount++;
			readhdrs[i].values =
				(char **)xrealloc(readhdrs[i].values,
				  readhdrs[i].valuecount * sizeof(char *));
			readhdrs[i].values[readhdrs[i].valuecount - 1] =
				xstrdup(line + tokenlen);
		}
		i++;
	}
}

int do_all_the_voodoo_here(int infd, int outfd, int hdrfd, int footfd,
		 const strlist *delhdrs, struct mailhdr *readhdrs,
		 strlist *allhdrs, const char *prefix, int replyto)
{
	char *hdrline, *unfolded, *unqp;
	bool hdrsadded = false;
	bool subject_present = false;
	FILE *f = fdopen(dup(infd), "r");

	for(;;) {
		hdrline = gethdrline(f, &unfolded);

		/* add extra headers before MIME* headers,
		   or after all headers */
		if(!hdrsadded &&
		    (hdrline == NULL || strncasecmp(hdrline, "mime", 4) == 0)) {
			if(hdrfd >= 0) {
				if(dumpfd2fd(hdrfd, outfd) < 0) {
					log_error(LOG_ARGS, "Could not "
						"add extra headers");
					free(hdrline);
					free(unfolded);
					return -1;
				}
				fsync(outfd);
			}
			hdrsadded = true;
		}

		/* end of headers */ 
		if(hdrline == NULL) {
			/* add Subject if none is present
			   and a prefix is defined */
			if (prefix && !subject_present) {
				dprintf(outfd, "Subject: %s\n", prefix);
				subject_present = true;
			}
			/* write LF */
			if(dprintf(outfd, "\n") < 0) {
				free(hdrline);
				free(unfolded);
				log_error(LOG_ARGS, "Error writing hdrs.");
				return -1;
			}
			free(hdrline);
			free(unfolded);
			break;
		}

		/* Do we want info from hdrs? Get it before it's gone */
		if(readhdrs)
			getinfo(hdrline, readhdrs);

		/* Snatch a copy of the header */
		tll_push_back(*allhdrs, xstrdup(hdrline));

		/* Add Subject: prefix if wanted */
		if(prefix) {
			if(strncasecmp(hdrline, "Subject:", 8) == 0) {
				subject_present = 1;
				unqp = decode_qp(hdrline + 8);
				if(strstr(hdrline + 8, prefix) == NULL &&
				   strstr(unqp, prefix) == NULL) {
					dprintf(outfd, "Subject: %s%s\n", prefix, hdrline + 8);
					free(hdrline);
					free(unqp);
					continue;
				}
				free(unqp);
			}
		}

		/* Should it be stripped? */
		if(!delhdrs || !findit(hdrline, delhdrs))
			dprintf(outfd, "%s", unfolded);

		/* Should Reply-To be added? */
		if(replyto) {
			if(strncasecmp(hdrline, "From:", 5) == 0) {
				dprintf(outfd, "Reply-To:%s\n", hdrline + 5);
			}
		}

		free(hdrline);
		free(unfolded);
	}

	lseek(infd, ftello(f), SEEK_SET);
	/* Just print the rest of the mail */
	if(dumpfd2fd(infd, outfd) < 0) {
		log_error(LOG_ARGS, "Error when dumping rest of mail");
		return -1;
	}
	fclose(f);

	/* No more, let's add the footer if one */
	if(footfd >= 0)
		if(dumpfd2fd(footfd, outfd) < 0) {
			log_error(LOG_ARGS, "Error when adding footer");
			return -1;
		}

	fsync(outfd);

	return 0;
}
