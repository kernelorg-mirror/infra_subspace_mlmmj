/* Copyright (C) 2004 Mads Martin Joergensen <mmj at mmj.dk>
 * Copyright (C) 2025 Baptiste Daroussin <bapt@FreeBSD.org>
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
#include "find_email_adr.h"
#include "utils.h"


/* read line and populate a struct to save a list important header see mlmmj-process.c:254 and mlmmj.h:97 */

void
scan_headers(FILE *f, struct mailhdr *readhdrs, strlist *allhdrs, strlist *allunfoldeds)
{
	char *hdrline, *unfolded;

	/* scan all headers and populate readhdrs , allhdrs , allunfoldeds */
	for (;;) {
	        /* hdrline contains the header in one line and unfolded contains the original data */
		hdrline = gethdrline(f, &unfolded);

		/* end of headers */ 
		if(hdrline == NULL) {
			break;
		}

		/* Do we want info from hdrs? Get it before it's gone */
		if(readhdrs)
			save_hdr_if_needed(hdrline, readhdrs);

		if (allhdrs != NULL) {
			/* Snatch a copy of the header */
			tll_push_back(*allhdrs, xstrdup(hdrline));
		}
		
		if (allunfoldeds != NULL) {
			/* Snatch a copy of unfolded */
			tll_push_back(*allunfoldeds, xstrdup(unfolded));
		}
		
		free(hdrline);
		free(unfolded);
	}
}

int do_all_the_voodoo_here(int infd, int outfd, int hdrfd, int footfd,
		 const strlist *delhdrs, struct mailhdr *readhdrs,
		 strlist *allhdrs, const char *prefix, int replyto)
{
	char *hdrline, *unfolded, *unqp;
	strlist allunfoldeds = tll_init();
	char *posteraddr = NULL;
	bool hdrsadded = false;
	bool subject_present = false;
	int dupfd = dup(infd);
	FILE *f = dupfd != -1 ? fdopen(dupfd, "r") : NULL;

	if (f == NULL) {
		if (dupfd != -1)
			close(dupfd);
		return -1;
	}

	scan_headers(f, readhdrs, allhdrs, &allunfoldeds);

	/* if we have custom headers , extract posteraddr from current header */
	if(hdrfd >= 0) {
		strlist fromemails = tll_init();
		if ( readhdrs[0].valuecount == 1 ) {
		        find_email_adr(readhdrs[0].values[0], &fromemails);
			if (tll_length(fromemails) > 0)
				posteraddr = xstrdup(tll_front(fromemails));
		}
		tll_free_and_free(fromemails, free);
	}
	/* scan all headers from allhdrs , and do the real things */
	tll_foreach(*allhdrs, a_header ) {

	        /* hdrline contains the unfolded value of the header and unfolded contains the original data */
	        hdrline=a_header->item ;
		unfolded=tll_pop_front(allunfoldeds) ;

		/* add extra headers before MIME* headers,
		   or after all headers */
		if(!hdrsadded &&
		    ( strncasecmp(hdrline, "mime", 4) == 0)) {
			if(hdrfd >= 0) {
			        if(process_headers_fd(hdrfd,outfd,posteraddr) < 0) {
					log_error(LOG_ARGS, "Could not "
						"add extra headers");
					tll_free_and_free(allunfoldeds, free);
					free(posteraddr);
					return -1;
				}
				fsync(outfd);
			}
			hdrsadded = true;
		}

		/* Add Subject: prefix if wanted */
		if(prefix) {
			if(strncasecmp(hdrline, "Subject:", 8) == 0) {
				subject_present = 1;
				unqp = decode_qp(hdrline + 8, false);
				if(strstr(hdrline + 8, prefix) == NULL &&
				   strstr(unqp, prefix) == NULL) {
					dprintf(outfd, "Subject: %s%s\n", prefix, hdrline + 8);
					free(unqp);
					continue;
				}
				free(unqp);
			}
		}

		/* Should it be stripped? */
		if(!delhdrs || !line_startwithany(hdrline, delhdrs, true))
			dprintf(outfd, "%s", unfolded);

		/* Should Reply-To be added? */
		if(replyto) {
			if(strncasecmp(hdrline, "From:", 5) == 0) {
				dprintf(outfd, "Reply-To:%s\n", hdrline + 5);
			}
		}
	}

	if(!hdrsadded ) {
		if(hdrfd >= 0) {
		  if(process_headers_fd(hdrfd,outfd,posteraddr) < 0) {
				log_error(LOG_ARGS, "Could not "
					"add extra headers");
				tll_free_and_free(allunfoldeds, free);
				free(posteraddr);
				return -1;
			}
			fsync(outfd);
		}
	}

	if (prefix && !subject_present) {
		dprintf(outfd, "Subject: %s\n", prefix);
		subject_present = true;
	}
	/* write LF */
	if(dprintf(outfd, "\n") < 0) {
	        tll_free_and_free(allunfoldeds, free);
		log_error(LOG_ARGS, "Error writing hdrs.");
		free(posteraddr);
		return -1;
	}
	
	tll_free_and_free(allunfoldeds, free);

	lseek(infd, ftello(f), SEEK_SET);
	/* Just print the rest of the mail */
	if(dumpfd2fd(infd, outfd) < 0) {
		log_error(LOG_ARGS, "Error when dumping rest of mail");
		free(posteraddr);
		return -1;
	}
	fclose(f);

	/* No more, let's add the footer if one */
	if(footfd >= 0)
		if(dumpfd2fd(footfd, outfd) < 0) {
			log_error(LOG_ARGS, "Error when adding footer");
			free(posteraddr);
			return -1;
		}

	fsync(outfd);

	free(posteraddr);
	return 0;
}
