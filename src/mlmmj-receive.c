/*
 * Copyright (C) 2002, 2003 Mads Martin Joergensen <mmj at mmj.dk>
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
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <libgen.h>
#include <err.h>

#include "mlmmj.h"
#include "wrappers.h"
#include "strgen.h"
#include "log_error.h"
#include "utils.h"
#include "xmalloc.h"
#include "mygetline.h"
#include "ctrlvalues.h"
#include "chomp.h"

extern char *optarg;

#define UNWANTED_MIME_HDR "X-ThisMailContainsUnwantedMimeParts: N\n"

/*
 * Read all mail headers from fd into the allhdrs strlist.
 * Parses Content-Type to extract mime_type and boundary.
 * Checks mime_type against denymime list.
 * Sets *deny to 1 if the mail should be denied.
 */
static void
read_headers(int fd, strlist *allhdrs,
    const strlist *denymime, int *deny, char **boundary)
{
	char *mime_type = NULL;
	char *tmp;

	while (1) {
		char *line = mygetline(fd);
		if (!line)        /* EOF, also end of headers */
			break;

		/* end of headers */
		if (line[0] == '\n') {
			free(line);
			break;
		}
		if (!allhdrs->len ||
		    ((line[0] != '\t') && (line[0] != ' '))) {
			/* first header or no unfolding */
			vec_push(allhdrs, line);
		} else {
			/* unfolding: append to previous header */
			xasprintf(&tmp, "%s%s", vec_last(allhdrs), line);
			free(vec_pop(allhdrs));
			vec_push(allhdrs, tmp);
			free(line);
		}
	}
	parse_content_type(allhdrs, &mime_type, boundary);
	if (mime_type) {
		if (denymime != NULL &&
		    line_startwithany(mime_type, denymime, true))
			*deny = 1;
		free(mime_type);
	}
}

/*
 * Write headers from hdrs strlist to outfd.
 * If unwantedmime_hdrpos is not NULL, insert the UNWANTED_MIME_HDR
 * and save the position of the 'N' character for later update.
 * Returns 0 on success, -1 on error.
 */
static int
write_headers(int outfd, const strlist *hdrs, off_t *unwantedmime_hdrpos)
{
	vec_foreach(*hdrs, _i) {
		if (dprintf(outfd, "%s", hdrs->d[_i]) < 0) {
			log_error(LOG_ARGS, "Error when dumping headers");
			return -1;
		}
	}

	if (unwantedmime_hdrpos != NULL) {
		if (dprintf(outfd, "%s", UNWANTED_MIME_HDR) < 0) {
			log_error(LOG_ARGS,
			    "Error writing unwanted mime header");
			return -1;
		}
		*unwantedmime_hdrpos = lseek(outfd, 0, SEEK_CUR);
		if (*unwantedmime_hdrpos < 2) {
			log_error(LOG_ARGS, "Error getting file position");
			return -1;
		}
		*unwantedmime_hdrpos -= 2;
	}

	if (dprintf(outfd, "\n") < 0) {
		log_error(LOG_ARGS, "Error writing end of headers.");
		return -1;
	}
	return 0;
}

/*
 * Seek back to the unwanted mime header position and update N -> Y.
 * Then seek back to end of file.
 */
static int
update_unwanted_mime_header(int outfd, off_t unwantedmime_hdrpos)
{
	if (lseek(outfd, unwantedmime_hdrpos, SEEK_SET) < 0) {
		log_error(LOG_ARGS, "Error seeking to the unwantedmime_hdr");
		return -1;
	}

	if (dprintf(outfd, "Y\n") < 0) {
		log_error(LOG_ARGS, "Error writing extra header");
		return -1;
	}

	if (lseek(outfd, 0, SEEK_END) < 0) {
		log_error(LOG_ARGS, "Error seeking to the mail end");
		return -1;
	}
	return 0;
}

/*
 * Recursively walk a multipart MIME body, writing everything to outfd.
 * Checks nested Content-Type headers against the deny list.
 * Returns 0 on success, -1 on error.
 */
static int
parse_body(int infd, int outfd, const strlist *denymime,
    int *deny, const char *boundary)
{
	char *line;
	size_t blen;

	if (boundary == NULL)
		return 0;

	blen = strlen(boundary);

	while ((line = mygetline(infd))) {
		/* Boundary lines start with "--" followed by the boundary */
		if (strncmp(line, "--", 2) == 0 &&
		    strncmp(line + 2, boundary, blen) == 0) {
			/* check if this is a closing boundary
			 * (trailing "--" after the boundary value) */
			if (strncmp(line + 2 + blen, "--", 2)) {
				strlist hdrs = vec_init();
				char *new_boundary = NULL;

				read_headers(infd, &hdrs, denymime,
				    deny, &new_boundary);

				/* write the boundary */
				if (dprintf(outfd, "%s", line) < 0) {
					log_error(LOG_ARGS,
					    "Error writing boundary");
					vec_free_and_free(&hdrs, free);
					free(new_boundary);
					free(line);
					return -1;
				}
				/* write the part headers */
				if (write_headers(outfd, &hdrs, NULL) < 0) {
					log_error(LOG_ARGS,
					    "Error writing part hdrs");
					vec_free_and_free(&hdrs, free);
					free(new_boundary);
					free(line);
					return -1;
				}
				vec_free_and_free(&hdrs, free);
				/* recurse into embedded multipart */
				if (new_boundary &&
				    parse_body(infd, outfd, denymime,
				    deny, new_boundary) != 0) {
					free(new_boundary);
					free(line);
					return -1;
				}
				free(new_boundary);
			} else {
				/* closing boundary --, write it */
				if (dprintf(outfd, "%s", line) < 0) {
					log_error(LOG_ARGS,
					    "Error writing closing boundary");
					free(line);
					return -1;
				}
				free(line);
				break;
			}
		} else {
			/* regular body line, pass through */
			if (dprintf(outfd, "%s", line) < 0) {
				log_error(LOG_ARGS,
				    "Error writing body line");
				free(line);
				return -1;
			}
		}
		free(line);
	}
	return 0;
}

/*
 * Main mail processing function: reads the mail from infd, checks MIME
 * types against control/mimedeny file, writes the mail to outfd, and adds
 * an X-ThisMailContainsUnwantedMimeParts: N/Y header.
 *
 * If no control/mimedeny file exists, falls back to simple dumpfd2fd.
 * Returns 0 on success, -1 on error.
 */
static int
process_mail(int infd, int outfd, int listfd)
{
	strlist hdrs = vec_init();
	strlist *denymime = NULL;
	char *boundary = NULL;
	int deny = 0, ctrlfd;
	off_t unwantedmime_hdr_pos = 0;

	/* Open control directory and read control/mimedeny */
	ctrlfd = openat(listfd, "control", O_DIRECTORY|O_RDONLY|O_CLOEXEC);
	if (ctrlfd != -1) {
		denymime = ctrlvalues(ctrlfd, "mimedeny");
		close(ctrlfd);
	}

	/* If no mimedeny file, just pass through */
	if (denymime == NULL)
		return dumpfd2fd(infd, outfd);

	/* Read mail headers */
	read_headers(infd, &hdrs, denymime, &deny, &boundary);

	/* Write mail headers (with unwanted mime header) */
	if (write_headers(outfd, &hdrs, &unwantedmime_hdr_pos) < 0) {
		log_error(LOG_ARGS, "Could not write mail headers");
		goto cleanup;
	}

	vec_free_and_free(&hdrs, free);

	/* If not already denied by the top-level headers, check the body
	 * (for multipart messages, nested parts may match denymime) */
	if (!deny) {
		if (parse_body(infd, outfd, denymime, &deny,
		    boundary) != 0) {
			log_error(LOG_ARGS, "Could not parse mail");
			goto cleanup;
		}
	}
	free(boundary);

	/* Dump the rest of the mail (anything not consumed by parse_body) */
	if (dumpfd2fd(infd, outfd) != 0) {
		log_error(LOG_ARGS, "Could not receive mail");
		goto cleanup;
	}

	/* Update the N -> Y header if unwanted MIME parts were found */
	if (deny) {
		if (update_unwanted_mime_header(outfd,
		    unwantedmime_hdr_pos) != 0) {
			log_error(LOG_ARGS, "Could not update header");
			goto cleanup;
		}
	}

cleanup:
	if (denymime) {
		vec_free_and_free(denymime, free);
		free(denymime);
	}
	return 0;
}

static void print_help(const char *prg)
{
	printf("Usage: %s -L /path/to/listdir\n"
	       "       [-s sender@example.org] [-e extension] [-h] [-V] [-P] [-F]\n"
	       " -h: This help\n"
	       " -F: Don't fork in the background\n"
	       " -L: Full path to list directory\n"
	       " -s: Specify sender address\n"
	       " -e: The foo part of user+foo@example.org\n"
	       " -P: Don't execute mlmmj-process\n"
	       " -V: Print version\n", prg);
	exit(EXIT_SUCCESS);
}

int main(int argc, char **argv)
{
	char *infilename = NULL, *listdir = NULL;
	char *randomstr = NULL;
	char *mlmmjprocess, *bindir;
	int fd, opt, noprocess = 0, nofork = 0;
	int incfd, listfd;

	CHECKFULLPATH(argv[0]);
	
	log_set_name(argv[0]);

	bindir = mydirname(argv[0]);
	xasprintf(&mlmmjprocess, "%s/mlmmj-process", bindir);
	free(bindir);

	while ((opt = getopt(argc, argv, "hPVL:s:e:F")) != -1) {
		switch(opt) {
		case 'h':
			print_help(argv[0]);
			break;
		case 'L':
			listdir = optarg;
			break;
		case 's':
			setenv("SENDER", optarg, 1);
			break;
		case 'e':
			setenv("EXTENSION", optarg, 1);
			break;
		case 'P':
			noprocess = 1;
			break;
		case 'F':
			nofork = 1;
			break;
		case 'V':
			print_version(argv[0]);
			exit(0);
		}
	}

	if(listdir == NULL) {
		errx(EXIT_FAILURE, "You have to specify -L\n"
		    "%s -h for help", argv[0]);
	}

	listfd = open_listdir(listdir, true);
	incfd = openat(listfd, "incoming", O_DIRECTORY|O_CLOEXEC);
	if (incfd == -1)
		err(EXIT_FAILURE, "Cannot open(%s/incoming)", listdir);

	fd = mkuniqfileat(incfd, "", S_IRUSR|S_IWUSR, &randomstr);
	if (fd < 0) {
		log_error(LOG_ARGS, "could not create mail file in "
				    "%s/incoming directory", listdir);
		free(randomstr);
		exit(EXIT_FAILURE);
	}

	xasprintf(&infilename, "%s/incoming/%s", listdir, randomstr);
	free(randomstr);

	if(process_mail(STDIN_FILENO, fd, listfd) != 0) {
		log_error(LOG_ARGS, "Could not receive mail");
		exit(EXIT_FAILURE);
	}

	fsync(fd);
	close(fd);

	if(noprocess) {
		free(infilename);
		exit(EXIT_SUCCESS);
	}

	/*
	 * Now we fork so we can exit with success since it could potentially
	 * take a long time for mlmmj-send to finish delivering the mails and
	 * returning, making it susceptible to getting a SIGKILL from the
	 * mailserver invoking mlmmj-receive.
	 */
	if (!nofork)
		daemon(1, 0);

	exec_or_die(mlmmjprocess, "-L", listdir, "-m", infilename, NULL);
}
