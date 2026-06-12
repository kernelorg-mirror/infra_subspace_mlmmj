/*
 * Copyright (C) 2004, 2003, 2004 Mads Martin Joergensen <mmj at mmj.dk>
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
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/wait.h>
#include <signal.h>
#include <libgen.h>
#include <syslog.h>
#include <stdarg.h>
#include <sys/mman.h>
#include <limits.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <err.h>

#include "xmalloc.h"
#include "mlmmj.h"
#include "mlmmj-send.h"
#include "mail-functions.h"
#include "incindexfile.h"
#include "chomp.h"
#include "checkwait_smtpreply.h"
#include "init_sockfd.h"
#include "strgen.h"
#include "log_error.h"
#include "wrappers.h"
#include "statctrl.h"
#include "ctrlvalue.h"
#include <stdbool.h>
#include "getaddrsfromfile.h"
#include "utils.h"
#include "send_mail.h"
#include "archive.h"

static size_t maxverprecips = MAXVERPRECIPS;
static volatile sig_atomic_t gotsigterm = 0;

void catch_sig_term(int sig MLMMJ_UNUSED)
{
	gotsigterm = 1;
}

int send_mail_verp(int sockfd, strlist *addrs, struct mail *mail,
    const char *verpextra)
{
	int retval;
	char *reply, *reply2;

	if(sockfd == -1)
		return EBADF;

	retval = write_mail_from(sockfd, mail->from, verpextra);
	if(retval) {
		log_error(LOG_ARGS, "Could not write MAIL FROM\n");
		return retval;
	}
	reply = checkwait_smtpreply(sockfd, MLMMJ_FROM);
	if(reply) {
		log_error(LOG_ARGS, "Error in MAIL FROM. Reply = [%s]",
				reply);
		free(reply);
		write_rset(sockfd);
		reply2 = checkwait_smtpreply(sockfd, MLMMJ_RSET);
		if (reply2 != NULL) free(reply2);
		return MLMMJ_FROM;
	}
	vec_foreach(*addrs, _i) {
		if(gotsigterm) {
			log_error(LOG_ARGS, "TERM signal received, "
					"shutting down.");
			return -1;
		}
		if(strchr(addrs->d[_i], '@') == NULL) {
			errno = 0;
			log_error(LOG_ARGS, "No @ in address, ignoring %s",
					addrs->d[_i]);
			continue;
		}
		retval = write_rcpt_to(sockfd, addrs->d[_i]);
		if(retval) {
			log_error(LOG_ARGS, "Could not write RCPT TO:\n");
			return retval;
		}

		reply = checkwait_smtpreply(sockfd, MLMMJ_RCPTTO);
		if(reply) {
			log_error(LOG_ARGS, "Error in RCPT TO. Reply = [%s]",
					reply);
			free(reply);
			return MLMMJ_RCPTTO;
		}
	}

	retval = write_data(sockfd);
	if(retval) {
		log_error(LOG_ARGS, "Could not write DATA\b");
		return retval;
	}

	reply = checkwait_smtpreply(sockfd, MLMMJ_DATA);
	if(reply) {
		log_error(LOG_ARGS, "Error with DATA. Reply = [%s]", reply);
		free(reply);
		write_rset(sockfd);
		reply2 = checkwait_smtpreply(sockfd, MLMMJ_RSET);
		if (reply2 != NULL) free(reply2);
		return MLMMJ_DATA;
	}

	rewind(mail->fp);
	retval = write_mailbody(sockfd, mail->fp, NULL);
	if(retval) {
		log_error(LOG_ARGS, "Could not write mail body\n");
		return retval;
	}

	retval = write_dot(sockfd);
	if(retval) {
		log_error(LOG_ARGS, "Could not write <CR><LF>.<CR><LF>\n");
		return retval;
	}

	reply = checkwait_smtpreply(sockfd, MLMMJ_DOT);
	if(reply) {
		log_error(LOG_ARGS, "Mailserver did not ack end of mail.\n"
				"<CR><LF>.<CR><LF> was written, to no"
				"avail. Reply = [%s]", reply);
		free(reply);
		write_rset(sockfd);
		reply2 = checkwait_smtpreply(sockfd, MLMMJ_RSET);
		if (reply2 != NULL) free(reply2);
		return MLMMJ_DOT;
	}

	return 0;
}

int send_mail_many_fd(int sockfd, struct mail *mail, struct ml *ml, int subfd,
    const char *archivefilename)
{
	int res, ret;
	strlist stl = vec_init();
	FILE *f = fdopen(subfd, "r");

	if (f == NULL) {
		log_error(LOG_ARGS, "Could not fdopen subscriber fd");
		close(subfd);
		return -1;
	}

	do {
		res = getaddrsfromfile(&stl, f, maxverprecips);
		if(stl.len == maxverprecips) {
			ret = send_mail_many_list(sockfd, mail, ml, &stl,
			    archivefilename);
			vec_free_and_free(&stl, free);
			if(ret < 0)
				return ret;
		}
	} while(res > 0);
	fclose(f);

	if(stl.len) {
		ret = send_mail_many_list(sockfd, mail, ml, &stl,
		    archivefilename);
		vec_free_and_free(&stl, free);
		return ret;
	}

	return 0;
}

int
send_mail_many_list(int sockfd, struct mail *mail, struct ml *ml,
    strlist *addrs, const char *archivefilename)
{
	int res = 0, status = 0;
	int index = archivefilename ?
	    get_index_from_filename(archivefilename) : -1;
	char *bounceaddr, *addr;

	for (size_t i = 0; i < addrs->len; i++) {
		addr = addrs->d[i];
		bounceaddr = NULL;
		if (strchr(addr, '@') == NULL) {
			errno = 0;
			log_error(LOG_ARGS, "No @ in address, ignoring %s",
			    addr);
			free(addr);
			continue;
		}
		if (gotsigterm && archivefilename) {
			/* we got SIGTERM, so save the addresses and bail */
			log_error(LOG_ARGS, "TERM signal received, "
			    "shutting down.");
			strlist remaining = { .d = &addrs->d[i + 1],
			    .len = addrs->len - i - 1 };
			status = requeuemail(ml->fd, index, &remaining, addr);
			for (size_t j = i + 1; j < addrs->len; j++)
				free(addrs->d[j]);
			free(addr);
			goto out;
		}
		if (mail->from == NULL) {
			bounceaddr = get_bounce_from_adr(addr, ml, index);
			mail->from = bounceaddr;
		}
		mail->to = addr;
		res = send_mail(sockfd, mail, ml->fd, ml->ctrlfd,
		    bounceaddr != NULL);
		if (bounceaddr != NULL) {
			free(bounceaddr);
			mail->from = NULL;
		}
		if (res && archivefilename) {
			/* we failed, so save the addresses and bail */
			strlist remaining = { .d = &addrs->d[i + 1],
			    .len = addrs->len - i - 1 };
			status = requeuemail(ml->fd, index, &remaining, addr);
			for (size_t j = i + 1; j < addrs->len; j++)
				free(addrs->d[j]);
			free(addr);
			goto out;
		}
		free(addr);
	}
out:
	/*
	 * The caller will call vec_free_and_free on this strlist.
	 * Since we already freed each address as we went, reset the
	 * length to prevent double-free.
	 */
	addrs->len = 0;
	return (status);
}

static void print_help(const char *prg)
{
	printf("Usage: %s [-L /path/to/list -m /path/to/mail | -l listctrl]\n"
	       "       [-a] [-D] [-F sender@example.org] [-h] [-o address@example.org]\n"
	       "       [-r 127.0.0.1] [-R reply@example.org] [-s /path/to/subscribers]\n"
	       "       [-T recipient@example.org] [-V]\n"
	       " -a: Don't archive the mail\n"
	       " -D: Don't delete the mail after it's sent\n"
	       " -F: What to use as MAIL FROM:\n"
	       " -h: This help\n"
	       " -l: List control variable:\n", prg);
	printf("    '1' means 'send a single mail'\n"
	       "    '2' means 'mail to moderators'\n"
	       "    '3' means 'resend failed list mail'\n"
	       "    '4' means 'send to file with recipients'\n"
	       "    '5' means 'bounceprobe'\n"
	       "    '6' means 'single listmail to single recipient'\n"
	       "    '7' means 'digest'\n");
	printf(" -L: Full path to list directory\n"
	       " -m: Full path to mail file\n"
	       " -o: Address to omit from distribution (normal mail only)\n"
	       " -r: Relayhost IP address (defaults to 127.0.0.1)\n"
	       " -R: What to use as Reply-To: header\n"
	       " -s: Subscribers file name\n"
	       " -T: What to use as RCPT TO:\n"
	       " -V: Print version\n");
	exit(EXIT_SUCCESS);
}

int main(int argc, char **argv)
{
	int sockfd = -1, opt, mindex = 0, subfd = 0;
	int deletewhensent = 1, sendres = 0, digest = 0;
	bool archive = true, ctrlarchive = true;
	int res;
	char *mailfilename = NULL, *subfilename = NULL, *omit = NULL;
	char *bounceaddr = NULL;
	char *relayhost = NULL, *archivefilename = NULL;
	const char *subddirname = NULL;
	char listctrl = 0;
	char *verp = NULL;
	char *verpfrom;
	char *reply, *requeuefilename;
	DIR *subddir;
	struct dirent *dp;
	struct stat st;
	strlist stl = vec_init();
	struct sigaction sigact;
	struct mail mail = { 0 };
	struct ml ml;
	int subdirfd, mailfd;
	const char *errp;

	log_set_name(argv[0]);
	ml_init(&ml);

	/* install signal handler for SIGTERM */
	sigact.sa_handler = catch_sig_term;
	sigemptyset(&sigact.sa_mask);
	sigact.sa_flags = 0;
	if(sigaction(SIGTERM, &sigact, NULL) < 0)
		log_error(LOG_ARGS, "Could not install SIGTERM handler!");

	while ((opt = getopt(argc, argv, "aVDhm:l:L:R:F:T:r:s:o:")) != -1){
		switch(opt) {
		case 'a':
			archive = false;
			break;
		case 'D':
			deletewhensent = 0;
			break;
		case 'F':
			bounceaddr = optarg;
			break;
		case 'h':
			print_help(argv[0]);
			break;
		case 'l':
			if (*optarg <= '0' || *optarg > '7')
				errx(EXIT_FAILURE, "-l only accept the "
				    "following arguments: 1, 2, 3, 4, 5, 6 or"
				    " 7");
			listctrl = *optarg;
			archive = false;
			break;
		case 'L':
			ml.dir = optarg;
			break;
		case 'm':
			mailfilename = optarg;
			break;
		case 'o':
			omit = optarg;
			break;
		case 'r':
			relayhost = optarg;
			break;
		case 'R':
			mail.replyto = optarg;
			break;
		case 's':
			subfilename = optarg;
			break;
		case 'T':
			mail.to = optarg;
			break;
		case 'V':
			print_version(argv[0]);
			exit(EXIT_SUCCESS);
		}
	}

	if(mailfilename == NULL || (ml.dir == NULL && listctrl == '0')) {
		errx(EXIT_FAILURE, "You have to specify -m and -L or -l\n"
		    "%s -h for help", argv[0]);
	}
	if (ml.dir != NULL) {
		if (!ml_open(&ml, false))
			exit(EXIT_FAILURE);
	}

	if (listctrl == '1' || listctrl == '5' || listctrl == '6')
		errx(EXIT_FAILURE, "-l %c is not supported anymore", listctrl);

	if((listctrl == '2' && (ml.dir == NULL || bounceaddr == NULL))) {
		errx(EXIT_FAILURE, "With -l 2 you need -L and -F");
	}

	if((listctrl == '7' && ml.dir == NULL)) {
		errx(EXIT_FAILURE, "With -l 7 you need -L");
	}

	if (ml.dir) {
		verp = ctrlvalue(ml.ctrlfd, "verp");
		if(verp == NULL)
			if(statctrl(ml.ctrlfd, "verp") == 1)
				verp = xstrdup("");

		if(verp && statctrl(ml.ctrlfd, "maxverprecips"))
			maxverprecips = ctrlsizet(ml.ctrlfd, "maxverprecips", MAXVERPRECIPS);
	}

	/* initialize file with mail to send */

	mailfd = strtoim(mailfilename, 0, INT_MAX, &errp);
	if (errp == NULL) {
		/* cannot delete a mail sent from file descriptor */
		deletewhensent = false;
	}

	if (errp != NULL && ((mailfd = open(mailfilename, O_RDWR)) == -1 ||
	    !lock(mailfd, true))) {
		if (ml.dir != NULL && mailfd == -1) {
			if ((mailfd = openat(ml.fd, mailfilename, O_RDWR)) == -1 ||
				!lock(mailfd, true)) {
				log_error(LOG_ARGS, "Could not open '%s'", mailfilename);
				exit(EXIT_FAILURE);
			}
		} else {
			log_error(LOG_ARGS, "Could not open '%s'", mailfilename);
			exit(EXIT_FAILURE);
		}
	}
	mail.fp = fdopen(mailfd, "r");
	if (mail.fp == NULL) {
		log_error(LOG_ARGS, "Could not fdopen '%s'", mailfilename);
		close(mailfd);
		exit(EXIT_FAILURE);
	}

	if (ml.dir)
		ctrlarchive = statctrl(ml.ctrlfd, "noarchive");

	switch(listctrl) {
	case '2': /* Moderators */
		if((subfd = openat(ml.ctrlfd, "moderators", O_RDONLY)) < 0) {
			log_error(LOG_ARGS, "Could not open '%s':",
					    subfilename);
			/* No moderators is no error. Could be the sysadmin
			 * likes to do it manually.
			 */
			exit(EXIT_SUCCESS);
		}
		break;
	case '3':
		mail.addtohdr = statctrl(ml.ctrlfd, "addtohdr");
		mail.xforwardedto = statctrl(ml.ctrlfd, "xforwardedto");
		mail.xsignedrecipient = statctrl(ml.ctrlfd, "dara");
		/* FALLTHROUGH */
	case '4': /* sending mails to subfile */
		subfd = strtoim(subfilename, 0, INT_MAX, &errp);
		if (errp != NULL && (subfd = open(subfilename, O_RDONLY)) < 0) {
			log_error(LOG_ARGS, "Could not open '%s':",
					    subfilename);
			exit(EXIT_FAILURE);
		}
		break;
	default: /* normal list mail -- now handled when forking */
		mail.addtohdr = statctrl(ml.ctrlfd, "addtohdr");
		mail.xforwardedto = statctrl(ml.ctrlfd, "xforwardedto");
		mail.xsignedrecipient = statctrl(ml.ctrlfd, "dara");
		break;
	}

	/* initialize the archive filename */
	if(archive) {
		char *relpath;
		mindex = incindexfile(ml.fd);
		relpath = archive_path_write(ml.fd, ml.ctrlfd, mindex);
		xasprintf(&archivefilename, "%s/%s", ml.dir, relpath);
		free(relpath);
	}

	switch(listctrl) {
	case '2': /* Moderators */
		sockfd = newsmtp(&ml, relayhost);
		mail.from = bounceaddr;
		mail.replyto = NULL;
		if(send_mail_many_fd(sockfd, &mail, &ml, subfd, NULL)) {
			close(sockfd);
			sockfd = -1;
		} else {
			endsmtp(&sockfd);
		}
		break;
	case '3': /* resending earlier failed mails */
		sockfd = newsmtp(&ml, relayhost);
		mail.from = NULL;
		mail.replyto = NULL;
		sendres = send_mail_many_fd(sockfd, &mail, &ml, subfd, mailfilename);
		if (sendres) {
			/* NB: On partial failure, keeping the original
			 * subscribers file risks duplicate deliveries for
			 * recipients that already succeeded. This is a
			 * deliberate trade-off to avoid losing addresses
			 * on total connection failure. */
			close(sockfd);
			sockfd = -1;
		} else {
			endsmtp(&sockfd);
			/* Only unlink if subfilename was a path, not a fd number */
			if (errp != NULL)
				unlink(subfilename);
		}
		break;
	case '4': /* send mails to owner */
		sockfd = newsmtp(&ml, relayhost);
		mail.from = bounceaddr;
		mail.replyto = NULL;
		if(send_mail_many_fd(sockfd, &mail, &ml, subfd, mailfilename)) {
			close(sockfd);
			sockfd = -1;
		} else {
			endsmtp(&sockfd);
		}
		break;
	case '7':
		digest = 1;
		mail.addtohdr = true;
		archivefilename = "digest";
		/* fall through */
	default: /* normal list mail */
		if (!digest) {
			subddirname = "subscribers.d";
		} else {
			subddirname = "digesters.d";
		}
		subdirfd = openat(ml.fd, subddirname, O_DIRECTORY|O_CLOEXEC|O_RDONLY);
		if (subdirfd == -1 || (subddir = fdopendir(subdirfd)) == NULL) {
			log_error(LOG_ARGS, "Could not opendir(%s/%s)", ml.dir,
					    subddirname);
			exit(EXIT_FAILURE);
		}

		xasprintf(&verpfrom, "%s%sbounces-%d@%s", ml.name, ml.delim,
		    mindex, ml.fqdn);

		if(digest) {
			free(verp);
			verp = NULL;
		}

		if(verp && (strcmp(verp, "postfix") == 0)) {
			free(verp);
			verp = xstrdup("XVERP=-=");
		}

		if((mail.addtohdr || mail.xforwardedto || mail.xsignedrecipient) && verp) {
			log_error(LOG_ARGS, "Cannot use VERP with per-recipient "
					"headers. Not sending with VERP.");
			free(verp);
			verp = NULL;
		}
		
		if(verp) {
			sockfd = newsmtp(&ml, relayhost);
			if(sockfd > -1) {
			    if(write_mail_from(sockfd, verpfrom, verp)) {
				log_error(LOG_ARGS,
					    "Could not write VERP MAIL FROM. "
					    "Not sending with VERP.");
				free(verp);
				verp = NULL;
			    } else {
				reply = checkwait_smtpreply(sockfd, MLMMJ_FROM);
				if(reply) {
					log_error(LOG_ARGS,
						"Mailserver did not "
						"accept VERP MAIL FROM. "
						"Not sending with VERP.");
					free(reply);
					free(verp);
					verp = NULL;
				}
			    }
			    /* We can't be in SMTP DATA state or anything like
			     * that, so should be able to safely QUIT. */
			    endsmtp(&sockfd);
			} else {
			    log_error(LOG_ARGS,
				    "Could not connect to "
				    "write VERP MAIL FROM. "
				    "Not sending with VERP.");
			    free(verp);
			    verp = NULL;
			}
		}

		while((dp = readdir(subddir)) != NULL) {
			int fd;
			FILE *f;
			if(!strcmp(dp->d_name, "."))
				continue;
			if(!strcmp(dp->d_name, ".."))
				continue;
			fd = openat(subdirfd, dp->d_name, O_RDONLY);
			if (fd == -1 || (f = fdopen(fd, "r")) == NULL) {
				log_error(LOG_ARGS, "Could not open '%s/%s/%s'",
				    ml.dir, subddirname, dp->d_name);
				continue;
			}
			do {
				res = getaddrsfromfile(&stl, f, maxverprecips);
				if(omit != NULL) {
					vec_foreach(stl, _j)
						if (strcmp(stl.d[_j], omit) == 0) {
							vec_remove_and_free(&stl, _j, free);
							_j--;
						}
				}
				if(stl.len == maxverprecips) {
					sockfd = newsmtp(&ml, relayhost);
					if(verp) {
						mail.from = verpfrom;
						mail.replyto = NULL;
						sendres = send_mail_verp(
								sockfd, &stl,
								&mail,
								verp);
						if(sendres)
							requeuemail(ml.fd,
								mindex,
								&stl, NULL);
					} else {
						mail.from = NULL;
						mail.replyto = NULL;
						sendres = send_mail_many_list( sockfd, &mail, &ml, &stl, archivefilename);
					}
					if (sendres) {
						close(sockfd);
						sockfd = -1;
					} else {
						endsmtp(&sockfd);
					}
					vec_free_and_free(&stl, free);
				}
			} while(res > 0);
			fclose(f);
		}
		if(stl.len) {
			sockfd = newsmtp(&ml, relayhost);
			if(verp) {
				mail.from = verpfrom;
				mail.replyto = NULL;
				sendres = send_mail_verp(sockfd, &stl, &mail, verp);
				if(sendres)
					requeuemail(ml.fd, mindex, &stl, NULL);
			} else {
				mail.from = NULL;
				mail.replyto = NULL;
				sendres = send_mail_many_list(sockfd, &mail, &ml, &stl, archivefilename);
			}
			if (sendres) {
				close(sockfd);
				sockfd = -1;
			} else {
				endsmtp(&sockfd);
			}
			vec_free_and_free(&stl, free);
		}
		free(verpfrom);
		closedir(subddir);
		break;
	}

	free(verp);

	if(archive) {
		if(!ctrlarchive) {
			if(rename(mailfilename, archivefilename) < 0) {
				log_error(LOG_ARGS,
						"Could not rename(%s,%s);",
						mailfilename,
						archivefilename);
			}
		} else {
			xasprintf(&requeuefilename, "%s/requeue/%d",
				ml.dir, mindex);
			if(stat(requeuefilename, &st) < 0) {
				/* Nothing was requeued and we don't keep
				 * mail for a noarchive list. */
				unlink(mailfilename);
			} else {
				free(requeuefilename);
				xasprintf(&requeuefilename,
					"%s/requeue/%d/mailfile",
					ml.dir, mindex);
				if (rename(mailfilename, requeuefilename) < 0) {
					log_error(LOG_ARGS,
							"Could not rename(%s,%s);",
							mailfilename,
							requeuefilename);
				}
			}
			free(requeuefilename);
		}
		free(archivefilename);
	} else if(deletewhensent)
		unlink(mailfilename);

	return EXIT_SUCCESS;
}
