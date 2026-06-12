/* Copyright (C) 2007 Sascha Sommer <ssommer at suse.de>
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

/* a version of mlmmj-receive that parses the mail on the fly and strips unwanted
   mime parts
   opens the files control/mimedeny and control/mimestrip for a list of mime
   types for body parts that should be denied or stripped.
   It adds an extra header X-ThisMailContainsUnwantedMimeParts: Y for mails that
   contain disallowed mimeparts and X-ThisMailContainsUnwantedMimeParts: N otherwise
*/

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <err.h>


#include "mlmmj.h"
#include "mygetline.h"
#include "gethdrline.h"
#include "strgen.h"
#include "chomp.h"
#include "ctrlvalue.h"
#include "ctrlvalues.h"
#include "utils.h"

#include "log_error.h"
#include "wrappers.h"
#include "xmalloc.h"
#include "utils.h"

#define UNWANTED_MIME_HDR "X-ThisMailContainsUnwantedMimeParts: N\n"

/* read all mail headers and save them in a strlist
 * check what to do with parts that contain the given mime_type
 *return values
 * 0: ok
 * 1: strip
 * sets deny to 1 if the entire mail should be denied
 */
#define MIME_OK 0
#define MIME_STRIP 1
static int read_hdrs(int fd, strlist *allhdrs, strlist* delmime, strlist* denymime,int* deny,char** boundary) {
	int result = MIME_OK;
	char* mime_type = NULL;
	char *tmp;
	/* read headers */
	while(1) {
		char* line = mygetline(fd);
		if(!line)        /* end of file and also end of headers */
			break;

		/* end of headers */
		if(line[0] == '\n'){
			free(line);
			break;
		}
		if(!allhdrs->len || ((line[0] != '\t') && (line[0] != ' '))) /* first header line or no more unfolding */
			vec_push(allhdrs, line);
		else{
			xasprintf(&tmp, "%s%s", vec_last(allhdrs), line);
			free(vec_pop(allhdrs));
			vec_push(allhdrs, tmp);
		}
		free(line);
	}
	parse_content_type(allhdrs,&mime_type,boundary);
	if(mime_type) {
		/* check if this part should be stripped */
		if(delmime && line_startwithany(mime_type, delmime, true))
			result = MIME_STRIP;
		/* check if the mail should be denied */
		if(denymime && line_startwithany(mime_type, denymime, true))
			*deny = 1;
		free(mime_type);
	}
	return result;
}

/* writes the mail headers if unwantedmime_hdrpos is not NULL an UNWANTED_MIME_HDR
 * is inserted and its position saved in unwantedmime_hdrpos
 * returns 0 on success
 */
static int write_hdrs(int outfd, strlist* hdrs,off_t* unwantedmime_hdrpos) {
	vec_foreach(*hdrs, _i) {
		if(dprintf(outfd, "%s", hdrs->d[_i]) < 0){
			log_error(LOG_ARGS, "Error when dumping headers");
			return -1;
		}
	}

	/* if this is not the header of an embedded part add the header that will
	   indicate if the mail contains unwanted mime parts */
	if(unwantedmime_hdrpos) {
		if(dprintf(outfd, "%s", UNWANTED_MIME_HDR) < 0){
			log_error(LOG_ARGS, "Error writting unwanted mime header");
			return -1;
		}
		/* get the current position so that we can update the header later */
		*unwantedmime_hdrpos = lseek(outfd,0,SEEK_CUR);
		if(*unwantedmime_hdrpos < 2){
			log_error(LOG_ARGS, "Error getting file position");
			return -1;
		}
		*unwantedmime_hdrpos -= 2;
	}

	/* write a single line feed to terminate the header part */
	if(dprintf(outfd, "\n") < 0) {
		log_error(LOG_ARGS,"Error writting end of hdrs.");
		return -1;
	}
	return 0;
}

/* set the unwanted mime_hdr to Y */
static int update_unwantedmime_hdr(int outfd,off_t unwantedmime_hdrpos) {
	/* seek to the header position */
	if(lseek(outfd,unwantedmime_hdrpos,SEEK_SET) < 0) {
		log_error(LOG_ARGS,"Error seeking to the unwantedmime_hdr");
		return -1;
	}

	/* update the header */
	if(dprintf(outfd, "Y\n") < 0){
		log_error(LOG_ARGS, "Error writting extra header");
		return -1;
	}

	/* seek back to the end of the mail */
	if(lseek(outfd,0,SEEK_END) < 0) {
		log_error(LOG_ARGS,"Error seeking to the mail end");
		return -1;
	}
	return 0;
}

static int parse_body(int infd,int outfd, strlist* delmime, strlist* denymime,
			int* deny,char* boundary){
	int strip = 0;
	char* line;
	while((line = mygetline(infd))) {
		if(boundary && !strncmp(line,boundary,strlen(boundary))){
			strip = 0;
			/* check if the boundary is the beginning of a new part */
			if(strncmp(line + strlen(boundary),"--",2)){
				strlist hdrs = vec_init();
				char* new_boundary = NULL;
				/* check if this part should be stripped */
				if(read_hdrs(infd, &hdrs,delmime,denymime,deny,&new_boundary) == MIME_STRIP)
					strip = 1;
				else {
					/* write boundary */
					if(dprintf(outfd, "%s", line) < 0){
						log_error(LOG_ARGS, "Error writting boundary");
						return -1;
					}
					/* write hdr */
					if(write_hdrs(outfd, &hdrs, NULL) < 0){
						log_error(LOG_ARGS, "Error writting hdrs");
						return -1;
					}
					/* parse embedded part if a new boundary was found */
					if(new_boundary && parse_body(infd,outfd,delmime,denymime,deny,new_boundary) != 0) {
						log_error(LOG_ARGS, "Could not parse embedded part");
						return -1;
					}
				}
				vec_free_and_free(&hdrs, free);
				if(new_boundary)
					free(new_boundary);
			}else{
				/* write end of part */
				if(dprintf(outfd, "%s", line) < 0){
					log_error(LOG_ARGS, "Error writting hdrs");
					return -1;
				}
				/* and leave */
				free(line);
				break;
			}
		}else {
			if(!strip) { /* write the current line */
				if(dprintf(outfd, "%s", line) < 0){
					log_error(LOG_ARGS, "Error when dumping line");
					return -1;
				}
			}
		}
		free(line);
	}
	return 0;
}

/* read a mail stripping unwanted parts */
static int dump_mail(int infd, int outfd,char* listdir) {
	strlist hdrs = vec_init();
	strlist *delmime;
	strlist *denymime;
	char* boundary=NULL;
	int deny = 0;
	int result, ctrlfd;
	off_t unwantedmime_hdr_pos = 0;
	char *dir;

	xasprintf(&dir, "%s/control", listdir);
	ctrlfd = open(dir, O_RDONLY|O_CLOEXEC);

	/* get list control values */
	delmime = ctrlvalues(ctrlfd, "mimestrip");
	denymime = ctrlvalues(ctrlfd, "mimedeny");

	/* read mail header */
	result = read_hdrs(infd, &hdrs,delmime,denymime,&deny,&boundary);
	/* write mail header */
	if(write_hdrs(outfd,&hdrs,&unwantedmime_hdr_pos) < 0) {
		log_error(LOG_ARGS, "Could not write mail headers");
		return -1;
	}

	/* free mail header */
	vec_free_and_free(&hdrs, free);

	if(result == MIME_OK && !deny) {
		/* try to parse the mail */
		if(parse_body(infd,outfd,delmime,denymime,&deny,boundary) != 0) {
			log_error(LOG_ARGS, "Could not parse mail");
			return -1;
		}
		free(boundary);
	}else
		deny = 1;

	/* dump rest of mail */
        if(dumpfd2fd(infd, outfd) != 0) {
		log_error(LOG_ARGS, "Could not receive mail");
		return -1;
        }

	/* update header */
	if(deny) {
		if(update_unwantedmime_hdr(outfd,unwantedmime_hdr_pos) != 0) {
			log_error(LOG_ARGS, "Could not update header");
			return -1;
		}
	}

	/* free mime types */
	if(delmime) {
		vec_free_and_free(delmime, free);
		free(delmime);
	}
	if(denymime) {
		vec_free_and_free(denymime, free);
		free(denymime);
	}
	return 0;
}

static void print_help(const char *prg)
{
        printf("Usage: %s -L /path/to/listdir [-h] [-V] [-P] [-F]\n"
	       " -h: This help\n"
	       " -F: Don't fork in the background\n"
	       " -L: Full path to list directory\n"
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

	do {
		free(randomstr);
		randomstr = random_str();
		fd = openat(incfd, randomstr, O_RDWR|O_CREAT|O_EXCL, S_IRUSR|S_IWUSR);
	} while(fd < 0 && errno == EEXIST);

	xasprintf(&infilename, "%s/incoming/%s", listdir, randomstr);
	free(randomstr);

	if(fd < 0) {
		log_error(LOG_ARGS, "could not create mail file in "
				    "%s/incoming directory", listdir);
		free(infilename);
		exit(EXIT_FAILURE);
	}

	if(dump_mail(fileno(stdin), fd, listdir) != 0) {
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
