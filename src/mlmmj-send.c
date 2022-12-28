/* Copyright (C) 2004, 2003, 2004 Mads Martin Joergensen <mmj at mmj.dk>
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
#include <sys/fcntl.h>
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
#include <signal.h>
#include <err.h>

#include "xmalloc.h"
#include "mlmmj.h"
#include "mlmmj-send.h"
#include "mail-functions.h"
#include "incindexfile.h"
#include "chomp.h"
#include "checkwait_smtpreply.h"
#include "getlistaddr.h"
#include "getlistdelim.h"
#include "init_sockfd.h"
#include "strgen.h"
#include "log_error.h"
#include "mygetline.h"
#include "wrappers.h"
#include "statctrl.h"
#include "ctrlvalue.h"
#include "stdbool.h"
#include "getaddrsfromfd.h"
#include "utils.h"
#include "send_mail.h"

static int maxverprecips = MAXVERPRECIPS;
static int gotsigterm = 0;

void catch_sig_term(int sig)
{
	gotsigterm = 1;
}

int
get_index_from_filename(const char *filename)
{
	char *myfilename, *indexstr;
	int ret;
	size_t len;

	myfilename = xstrdup(filename);
	len = strlen(myfilename);
	if (len > 9 && (strcmp(myfilename + len - 9, "/mailfile") == 0)) {
		myfilename[len - 9] = '\0';
	}

	indexstr = strrchr(myfilename, '/');
	if (indexstr) {
		indexstr++;  /* skip the slash */
	} else {
		indexstr = myfilename;
	}

	ret = strtoim(indexstr, 0, INT_MAX, NULL);
	free(myfilename);

	return ret;
}

char *bounce_from_adr(const char *recipient, const char *listadr,
		      const char *listdelim, const char *mailfilename,
		      int ctrlfd)
{
	char *bounceaddr, *myrecipient, *mylistadr;
	char *listdomain, *a = NULL, *mymailfilename;
	char *staticbounceaddr, *staticbounceaddr_localpart;
	char *staticbounceaddr_domain;
	int index;

	mymailfilename = xstrdup(mailfilename);
	if (!mymailfilename) {
		return NULL;
	}

	index = get_index_from_filename(mymailfilename);

	myrecipient = xstrdup(recipient);
	if (!myrecipient) {
		free(mymailfilename);
		return NULL;
	}
	a = strchr(myrecipient, '@');
	if (a)
		*a = '=';

	mylistadr = xstrdup(listadr);
	if (!mylistadr) {
		free(mymailfilename);
		free(myrecipient);
		return NULL;
	}

	listdomain = strchr(mylistadr, '@');
	if (!listdomain) {
		free(mymailfilename);
		free(myrecipient);
		free(mylistadr);
		return NULL;
	}
	*listdomain++ = '\0';

	staticbounceaddr = ctrlvalue(ctrlfd, "staticbounceaddr");
	if (staticbounceaddr) {
		staticbounceaddr_localpart = genlistname(staticbounceaddr);
		staticbounceaddr_domain = genlistfqdn(staticbounceaddr);
	} else {
		staticbounceaddr_localpart = NULL;
		staticbounceaddr_domain = NULL;
	}

	if (staticbounceaddr) {
		xasprintf(&bounceaddr, "%s%s%s-bounces-%d-%s@%s",
			staticbounceaddr_localpart, listdelim, mylistadr,
			index, myrecipient, staticbounceaddr_domain);

		free(staticbounceaddr);
		free(staticbounceaddr_localpart);
		free(staticbounceaddr_domain);
	} else {
		xasprintf(&bounceaddr, "%s%sbounces-%d-%s@%s", mylistadr, listdelim,
			index, myrecipient, listdomain);
	}

	free(myrecipient);
	free(mylistadr);
	free(mymailfilename);

	return bounceaddr;
}

int send_mail_verp(int sockfd, struct strlist *addrs, struct mail *mail,
    const char *verpextra)
{
	int retval, i;
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
	for(i = 0; i < addrs->count; i++) {
		if(gotsigterm) {
			log_error(LOG_ARGS, "TERM signal received, "
					"shutting down.");
			return -1;
		}
		if(strchr(addrs->strs[i], '@') == NULL) {
			errno = 0;
			log_error(LOG_ARGS, "No @ in address, ignoring %s",
					addrs->strs[i]);
			continue;
		}
		retval = write_rcpt_to(sockfd, addrs->strs[i]);
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

	if(mail->inmem) {
		retval = dprintf(sockfd, "%s", mail->hdrs);
		if(retval < 0) {
			log_error(LOG_ARGS, "Could not write mailheaders.\n");
			return retval;
		}
		retval = dprintf(sockfd, "%s", mail->body);
		if(retval < 0) {
			log_error(LOG_ARGS, "Could not write mailbody.\n");
			return retval;
		}
	} else {
		retval = write_mailbody_from_map(sockfd, mail->map, mail->size, NULL);
		if(retval) {
			log_error(LOG_ARGS, "Could not write mail\n");
			return retval;
		}
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

int send_mail_many_fd(int sockfd, struct mail *mail, int subfd,
		      const char *listaddr, const char *listdelim,
		      const char *archivefilename, int listfd, int ctrlfd)
{
	int res, ret, i;
	struct strlist stl;

	stl.strs = NULL;
	stl.count = 0;

	do {
		res = getaddrsfromfd(&stl, subfd, maxverprecips);
		if(stl.count == maxverprecips) {
			ret = send_mail_many_list(sockfd, mail, &stl,
					listaddr, listdelim, archivefilename,
					listfd, ctrlfd);
			for(i = 0; i < stl.count; i++)
				free(stl.strs[i]);
			if(ret < 0)
				return ret;
			stl.count = 0;
		}
	} while(res > 0);

	if(stl.count) {
		ret = send_mail_many_list(sockfd, mail, &stl, listaddr, listdelim,
				archivefilename, listfd, ctrlfd);
		for(i = 0; i < stl.count; i++)
			free(stl.strs[i]);
		stl.count = 0;
		return ret;
	}

	return 0;
}

int requeuemail(int listfd, int index, struct strlist *addrs,
		int addrcount)
{
	int addrfd, i;
	char *dirname, *addrfilename;
	
	xasprintf(&dirname, "requeue/%d", index);
	if(mkdirat(listfd, dirname, 0750) < 0 && errno != EEXIST) {
		log_error(LOG_ARGS, "Could not mkdir(%s) for "
				"requeueing. Mail cannot "
				"be requeued.", dirname);
		free(dirname);
		return -1;
	}
	addrfilename = concatstr(2, dirname, "/subscribers");
	free(dirname);
	addrfd = openat(listfd, addrfilename, O_WRONLY|O_CREAT|O_APPEND,
			S_IRUSR|S_IWUSR);
	if(addrfd < 0) {
		log_error(LOG_ARGS, "Could not open %s",
				addrfilename);
		free(addrfilename);
		return -1;
	} else {
		/* Dump the remaining addresses. We dump the remaining before
		 * we write the failing address to ensure the potential good
		 * ones will be tried first when mlmmj-maintd sends out mails
		 * that have been requeued. addrcount was so far we were */
		for(i = addrcount + 1; i < addrs->count; i++) {
			if(dprintf(addrfd, "%s\n", addrs->strs[i]) < 0) {
				log_error(LOG_ARGS, "Could not add [%s] "
						    "to requeue file", addrs->strs[i]);
				return -1;
			}
		}
		if(dprintf(addrfd, "%s\n", addrs->strs[addrcount]) < 0) {
			log_error(LOG_ARGS, "Could not add [%s] to requeue "
					"file", addrs->strs[addrcount]);
			return -1;
		}
	}
	free(addrfilename);
	close(addrfd);

	return 0;
}

int send_mail_many_list(int sockfd, struct mail *mail, struct strlist *addrs,
		   const char *listaddr, const char *listdelim,
		   const char *archivefilename, int listfd, int ctrlfd)
{
	int res = 0, i, status, index;
	char *bounceaddr, *addr;

	for(i = 0; i < addrs->count; i++) {
		addr = addrs->strs[i];
		if(strchr(addr, '@') == NULL) {
			errno = 0;
			log_error(LOG_ARGS, "No @ in address, ignoring %s",
					addr);
			continue;
		}
		if(gotsigterm && listaddr && archivefilename) {
			/* we got SIGTERM, so save the addresses and bail */
			log_error(LOG_ARGS, "TERM signal received, "
						"shutting down.");
			index = get_index_from_filename(archivefilename);
			status = requeuemail(listfd, index, addrs, i);
			return status;
		}
		if (mail->from == NULL) {
			bounceaddr = bounce_from_adr(addr, listaddr, listdelim,
			    archivefilename, ctrlfd);
			mail->from = bounceaddr;
		}
		res = send_mail(sockfd, mail, listfd, bounceaddr != NULL);
		free(bounceaddr);
		if(res && listaddr && archivefilename) {
			/* we failed, so save the addresses and bail */
			index = get_index_from_filename(archivefilename);
			status = requeuemail(listfd, index, addrs, i);
			return status;
		}
	}
	return 0;
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
	size_t len = 0, hdrslen, bodylen;
	int sockfd = -1, mailfd = 0, opt, mindex = 0, subfd = 0, tmpfd, i;
	int deletewhensent = 1, sendres = 0, digest = 0;
	bool archive = true, ctrlarchive;
	int res;
	char *listaddr = NULL, *listdelim = NULL;
	char *mailfilename = NULL, *subfilename = NULL, *omit = NULL;
	char *bounceaddr = NULL;
	char *relayhost = NULL, *archivefilename = NULL, *tmpstr;
	char *subddirname = NULL, *listdir = NULL;
	char listctrl = 0;
	char *probefile, *a;
	char *verp = NULL;
	char *listname, *listfqdn, *verpfrom;
	char *reply, *smtphelo, *requeuefilename;
	ssize_t memmailsize = 0;
	DIR *subddir;
	struct dirent *dp;
	struct stat st;
	uid_t uid;
	struct strlist stl;
	unsigned short smtpport;
	struct sigaction sigact;
	int listfd, ctrlfd;
	struct mail mail;

	memset(&mail, 0, sizeof(mail));
	log_set_name(argv[0]);

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
			listdir = optarg;
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

	if(mailfilename == NULL || (listdir == NULL && listctrl == '0')) {
		errx(EXIT_FAILURE, "You have to specify -m and -L or -l\n"
		    "%s -h for help", argv[0]);
	}
	if (listdir != NULL) {
		listfd = open(listdir, O_DIRECTORY|O_CLOEXEC);
		if (listfd == -1)
			err(EXIT_FAILURE, "Cannot open(%s)", listdir);
		ctrlfd = openat(listfd, "control", O_DIRECTORY|O_CLOEXEC);
		if (ctrlfd == -1)
			err(EXIT_FAILURE, "Cannot open(%s/control)", listdir);
	}

	/* Lets make sure no random user tries to send mail to the list */
	if(listdir) {
		if(stat(listdir, &st) == 0) {
			uid = getuid();
			if(uid && uid != st.st_uid) {
				log_error(LOG_ARGS,
					"Have to invoke either as root "
					"or as the user owning listdir");
				errx(EXIT_FAILURE,
					"Have to invoke either as root "
					"or as the user owning listdir");
			}
		} else {
			log_error(LOG_ARGS, "Could not stat %s", listdir);
			exit(EXIT_FAILURE);
		}
	}

	/* get the list address */
	if(listctrl == '1' && (bounceaddr == NULL || mail.to == NULL)) {
		errx(EXIT_FAILURE, "With -l 1 you need -F and -T");
	}

	if((listctrl == '2' && (listdir == NULL || bounceaddr == NULL))) {
		errx(EXIT_FAILURE, "With -l 2 you need -L and -F");
	}

	if((listctrl == '7' && listdir == NULL)) {
		errx(EXIT_FAILURE, "With -l 7 you need -L");
	}

	if (listdir) {
		verp = ctrlvalue(ctrlfd, "verp");
		if(verp == NULL)
			if(statctrl(ctrlfd, "verp") == 1)
				verp = xstrdup("");

		if(verp && statctrl(ctrlfd, "maxverprecips"))
			maxverprecips = ctrlint(ctrlfd, "maxverprecips", MAXVERPRECIPS);
	}

	stl.strs = NULL;
	stl.count = 0;

	if(listdir && listctrl != '5')
		listaddr = fgetlistaddr(listfd);
	
	/* initialize file with mail to send */

	if((mailfd = open(mailfilename, O_RDWR, 0644)) < 0 &&
	    !lock(mailfd, true)) {
	        log_error(LOG_ARGS, "Could not open '%s'", mailfilename);
		exit(EXIT_FAILURE);
	}

	if(fstat(mailfd, &st) < 0) {
		log_error(LOG_ARGS, "Could not stat mailfd");
		exit(EXIT_FAILURE);
	}

	if (listdir) {
		memmailsize = ctrlsizet(ctrlfd, "memorymailsize", MEMORYMAILSIZE);
		ctrlarchive = statctrl(ctrlfd, "noarchive");
	}

	if(st.st_size > memmailsize) {
		errno = 0;
		log_error(LOG_ARGS, "Not preparing in memory. "
				    "Mail is %ld bytes", (long)st.st_size);
	} else
		mail.inmem = true;

	mail.map = mmap(0, st.st_size, PROT_READ, MAP_SHARED, mailfd, 0);
	if(mail.map == MAP_FAILED) {
		log_error(LOG_ARGS, "Could not mmap mailfd");
		exit(EXIT_FAILURE);
	}
	mail.size = st.st_size;

	if(mail.inmem) {
		mail.hdrs = get_preppedhdrs_from_map(mail.map, &hdrslen);
		if(mail.hdrs == NULL) {
			log_error(LOG_ARGS, "Could not prepare headers");
			exit(EXIT_FAILURE);
		}
		mail.body = get_prepped_mailbody_from_map(mail.map, st.st_size,
						     &bodylen);
		if(mail.body == NULL) {
			log_error(LOG_ARGS, "Could not prepare mailbody");
			free(mail.hdrs);
			exit(EXIT_FAILURE);
		}
	}

	if(listdir)
		listdelim = fgetlistdelim(listfd);

	switch(listctrl) {
	case '1': /* A single mail is to be sent, do nothing */
	case '5':
		break;
	case '2': /* Moderators */
		subfilename = concatstr(2, listdir, "/control/moderators");
		if((subfd = open(subfilename, O_RDONLY)) < 0) {
			log_error(LOG_ARGS, "Could not open '%s':",
					    subfilename);
			free(mail.hdrs);
			free(mail.body);
			free(subfilename);
			free(listdelim);
			/* No moderators is no error. Could be the sysadmin
			 * likes to do it manually.
			 */
			exit(EXIT_SUCCESS);
		}
		break;
	case '3':
		mail.addtohdr = statctrl(ctrlfd, "addtohdr");
	case '4': /* sending mails to subfile */
		if((subfd = open(subfilename, O_RDONLY)) < 0) {
			log_error(LOG_ARGS, "Could not open '%s':",
					    subfilename);
			free(mail.hdrs);
			free(mail.body);
			free(listdelim);
			exit(EXIT_FAILURE);
		}
		break;
	case '6':
		deletewhensent = 0;
		archivefilename = xstrdup(mailfilename);
		bounceaddr = bounce_from_adr(mail.to, listaddr, listdelim,
						archivefilename, ctrlfd);
		break;
	default: /* normal list mail -- now handled when forking */
		mail.addtohdr = statctrl(ctrlfd, "addtohdr");
		break;
	}

	/* initialize the archive filename */
	if(archive) {
		mindex = incindexfile((const char *)listdir);
		len = strlen(listdir) + 9 + 20;
		archivefilename = xmalloc(len);
		snprintf(archivefilename, len, "%s/archive/%d", listdir,
			 mindex);
	}

	if(!relayhost)
		relayhost = ctrlvalue(ctrlfd, "relayhost");
	if(!relayhost)
		relayhost = xstrdup(RELAYHOST);

	smtpport = ctrlushort(ctrlfd, "smtpport", 25);
	if ((smtphelo = ctrlvalue(ctrlfd, "smtphelo")) == NULL) {
		smtphelo = hostnamestr();
	}

	switch(listctrl) {
	case '1': /* A single mail is to be sent */
	case '6':
		initsmtp(&sockfd, relayhost, smtpport, smtphelo);
		mail.from = bounceaddr;
		if(send_mail(sockfd, &mail,listfd, false)) {
			close(sockfd);
			sockfd = -1;
			/* error, so keep it in the queue */
			deletewhensent = 0;
			/* dump data we want when resending first check
			 * if it already exists. In that case continue */
			tmpstr = concatstr(2, mailfilename, ".mailfrom");
			if(stat(tmpstr, &st) == 0) {
				free(tmpstr);
				break;
			}
			tmpfd = open(tmpstr, O_WRONLY|O_CREAT|O_TRUNC|O_SYNC,
						S_IRUSR|S_IWUSR);
			free(tmpstr);
			if(tmpfd >= 0) {
				dprintf(tmpfd, "%s", bounceaddr);
			}
			close(tmpfd);
			tmpstr = concatstr(2, mailfilename, ".reciptto");
			if(stat(tmpstr, &st) == 0) {
				free(tmpstr);
				break;
			}
			tmpfd = open(tmpstr, O_WRONLY|O_CREAT|O_TRUNC|O_SYNC,
						S_IRUSR|S_IWUSR);
			free(tmpstr);
			if(tmpfd >= 0) {
				dprintf(tmpfd, "%s", mail.to);
			}
			close(tmpfd);
			if(mail.replyto) {
				tmpstr = concatstr(2, mailfilename,
						      ".reply-to");
				if(stat(tmpstr, &st) == 0) {
					free(tmpstr);
					break;
				}
				tmpfd = open(tmpstr, O_WRONLY|O_CREAT|O_TRUNC|O_SYNC,
							S_IRUSR|S_IWUSR);
				free(tmpstr);
				if(tmpfd >= 0) {
					dprintf(tmpfd, "%s", mail.replyto);
				}
				close(tmpfd);
			}
		} else {
			endsmtp(&sockfd);
		}
		break;
	case '2': /* Moderators */
		initsmtp(&sockfd, relayhost, smtpport, smtphelo);
		mail.from = bounceaddr;
		mail.replyto = NULL;
		if(send_mail_many_fd(sockfd, &mail, subfd, NULL, NULL, NULL, listfd, ctrlfd)) {
			close(sockfd);
			sockfd = -1;
		} else {
			endsmtp(&sockfd);
		}
		break;
	case '3': /* resending earlier failed mails */
		initsmtp(&sockfd, relayhost, smtpport, smtphelo);
		mail.from = NULL;
		mail.replyto = NULL;
		if(send_mail_many_fd(sockfd, &mail, subfd, listaddr, listdelim,
				mailfilename, listfd, ctrlfd)) {
			close(sockfd);
			sockfd = -1;
		} else {
			endsmtp(&sockfd);
		}
		unlink(subfilename);
		break;
	case '4': /* send mails to owner */
		initsmtp(&sockfd, relayhost, smtpport, smtphelo);
		mail.from = bounceaddr;
		mail.replyto = NULL;
		if(send_mail_many_fd(sockfd, &mail, subfd, listaddr, listdelim,
				mailfilename, listfd, ctrlfd)) {
			close(sockfd);
			sockfd = -1;
		} else {
			endsmtp(&sockfd);
		}
		break;
	case '5': /* bounceprobe - handle relayhost local users bouncing*/
		initsmtp(&sockfd, relayhost, smtpport, smtphelo);
		mail.from = bounceaddr;
		if(send_mail(sockfd, &mail, listfd, false)) {
			close(sockfd);
			sockfd = -1;
			/* error, so remove the probefile */
			tmpstr = xstrdup(mail.to);
			a = strchr(tmpstr, '@');
			MY_ASSERT(a);
			*a = '=';
			probefile = concatstr(4, listdir, "/bounce/", tmpstr,
					"-probe");
			unlink(probefile);
			free(probefile);
			free(tmpstr);
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
			subddirname = concatstr(2, listdir, "/subscribers.d/");
		} else {
			subddirname = concatstr(2, listdir, "/digesters.d/");
		}
		if((subddir = opendir(subddirname)) == NULL) {
			log_error(LOG_ARGS, "Could not opendir(%s)",
					    subddirname);
			free(listdelim);
			free(subddirname);
			free(mail.hdrs);
			free(mail.body);
			exit(EXIT_FAILURE);
		}

		listdelim = getlistdelim(listdir);
		listname = genlistname(listaddr);	
		listfqdn = genlistfqdn(listaddr);	
		xasprintf(&verpfrom, "%s%sbounces-%d@%s", listname, listdelim,
		    mindex, listfqdn);
		free(listname);
		free(listfqdn);

		if(digest)
			verp = NULL;

		if(verp && (strcmp(verp, "postfix") == 0)) {
			free(verp);
			verp = xstrdup("XVERP=-=");
		}

		if(mail.addtohdr && verp) {
			log_error(LOG_ARGS, "Cannot use VERP and add "
					"To: header. Not sending with "
					"VERP.");
			verp = NULL;
		}
		
		if(verp) {
			initsmtp(&sockfd, relayhost, smtpport, smtphelo);
			if(sockfd > -1) {
			    if(write_mail_from(sockfd, verpfrom, verp)) {
				log_error(LOG_ARGS,
					    "Could not write VERP MAIL FROM. "
					    "Not sending with VERP.");
				verp = NULL;
			    } else {
				reply = checkwait_smtpreply(sockfd, MLMMJ_FROM);
				if(reply) {
					log_error(LOG_ARGS,
						"Mailserver did not "
						"accept VERP MAIL FROM. "
						"Not sending with VERP.");
					free(reply);
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
			    verp = NULL;
			}
		}

		while((dp = readdir(subddir)) != NULL) {
			if(!strcmp(dp->d_name, "."))
				continue;
			if(!strcmp(dp->d_name, ".."))
				continue;
			subfilename = concatstr(2, subddirname, dp->d_name);
			if((subfd = open(subfilename, O_RDONLY)) < 0) {
				log_error(LOG_ARGS, "Could not open '%s'",
						    subfilename);
				free(subfilename);
				continue;
			}
			do {
				i = stl.count;
				res = getaddrsfromfd(&stl, subfd,
						maxverprecips);
				if(omit != NULL) {
					while(i < stl.count) {
						if(strcmp(stl.strs[i], omit)
							== 0) {
						    free(stl.strs[i]);
						    stl.count--;
						    while (i < stl.count) {
							stl.strs[i] =
								stl.strs[i+1];
							i++;
						    }
						    stl.strs[stl.count] = NULL;
						    break;
						}
						i++;
					}
				}
				if(stl.count == maxverprecips) {
					initsmtp(&sockfd, relayhost, smtpport, smtphelo);
					if(verp) {
						mail.from = verpfrom;
						mail.replyto = NULL;
						sendres = send_mail_verp(
								sockfd, &stl,
								&mail,
								verp);
						if(sendres)
							requeuemail(listfd,
								mindex,
								&stl, 0);
					} else {
						mail.from = NULL;
						mail.replyto = NULL;
						sendres = send_mail_many_list(
								sockfd, &mail,
								&stl, listaddr,
								listdelim,
								archivefilename,
								listfd, ctrlfd);
					}
				    	if (sendres) {
					    	close(sockfd);
					    	sockfd = -1;
				    	} else {
					    	endsmtp(&sockfd);
				    	}
					for(i = 0; i < stl.count; i++)
						free(stl.strs[i]);
					stl.count = 0;
				}
			} while(res > 0);
			free(subfilename);
			close(subfd);

		}
		if(stl.count) {
			initsmtp(&sockfd, relayhost, smtpport, smtphelo);
			if(verp) {
				mail.from = verpfrom;
				mail.replyto = NULL;
				sendres = send_mail_verp(sockfd, &stl, &mail, verp);
				if(sendres)
					requeuemail(listfd, mindex, &stl, 0);
			} else {
				mail.from = NULL;
				mail.replyto = NULL;
				sendres = send_mail_many_list(sockfd, &mail,
						&stl, listaddr, listdelim,
						archivefilename, listfd, ctrlfd);
			}
			if (sendres) {
				close(sockfd);
				sockfd = -1;
			} else {
				endsmtp(&sockfd);
			}
			for(i = 0; i < stl.count; i++)
				free(stl.strs[i]);
			stl.count = 0;
		}
		free(stl.strs);
		free(verpfrom);
		closedir(subddir);
		free(subddirname);
		break;
	}
	
	free(listdelim);
	free(mail.hdrs);
	free(mail.body);
	munmap(mail.map, mail.size);
	close(mailfd);
	free(verp);
	free(smtphelo);

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
				listdir, mindex);
			if(stat(requeuefilename, &st) < 0) {
				/* Nothing was requeued and we don't keep
				 * mail for a noarchive list. */
				unlink(mailfilename);
			} else {
				free(requeuefilename);
				xasprintf(&requeuefilename,
					"%s/requeue/%d/mailfile",
					listdir, mindex);
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
