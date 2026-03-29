/*
 * Copyright (C) 2004, 2003, 2004 Mads Martin Joergensen <mmj at mmj.dk>
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
#include <unistd.h>
#include <fcntl.h>

#include "checkwait_smtpreply.h"
#include "mail-functions.h"
#include "getlistdelim.h"
#include "send_mail.h"
#include "log_error.h"
#include "init_sockfd.h"
#include "mlmmj.h"
#include "strgen.h"
#include "xmalloc.h"
#include "ctrlvalue.h"
#include "utils.h"

int
initsmtp(int *sockfd, const char *relayhost, unsigned short port, const char *heloname)
{
	int retval = 0;
	int try_ehlo = 1;
	char *reply = NULL;

	do {
		init_sockfd(sockfd, relayhost, port);

		if(*sockfd == -1) {
			retval = EBADF;
			break;
		}

		if((reply = checkwait_smtpreply(*sockfd, MLMMJ_CONNECT)) != NULL) {
			log_error(LOG_ARGS, "No proper greeting to our connect"
					"Reply: [%s]", reply);
			free(reply);
			retval = MLMMJ_CONNECT;
			/* FIXME: Queue etc. */
			break;
		}

		if (try_ehlo) {
			write_ehlo(*sockfd, heloname);
			if((reply = checkwait_smtpreply(*sockfd, MLMMJ_EHLO))
					== NULL) {
				/* EHLO successful don't try more */
				break;
			}

			/* RFC 1869 - 4.5. - In the case of any error response,
			 * the client SMTP should issue either the HELO or QUIT
			 * command.
			 * RFC 1869 - 4.5. - If the server SMTP recognizes the
			 * EHLO command, but the command argument is
			 * unacceptable, it will return code 501.
			 */
			if (strncmp(reply, "501", 3) == 0) {
				free(reply);
				/* Commmand unacceptable; we choose to QUIT but
				 * ignore any QUIT errors; return that EHLO was
				 * the error.
				 */
				endsmtp(sockfd);
				retval = MLMMJ_EHLO;
				break;
			}

			/* RFC 1869 - 4.6. - A server SMTP that conforms to RFC
			 * 821 but does not support the extensions specified
			 * here will not recognize the EHLO command and will
			 * consequently return code 500, as specified in RFC
			 * 821.  The server SMTP should stay in the same state
			 * after returning this code (see section 4.1.1 of RFC
			 * 821).  The client SMTP may then issue either a HELO
			 * or a QUIT command.
			 */

			if (reply[0] != '5') {
				free(reply);
				/* Server doesn't understand EHLO, but gives a
				 * broken response. Try with new connection.
				 */
				endsmtp(sockfd);
				try_ehlo = 0;
				continue;
			}

			free(reply);

			/* RFC 1869 - 4.7. - Other improperly-implemented
			 * servers will not accept a HELO command after EHLO has
			 * been sent and rejected.  In some cases, this problem
			 * can be worked around by sending a RSET after the
			 * failure response to EHLO, then sending the HELO.
			 */
			write_rset(*sockfd);
			reply = checkwait_smtpreply(*sockfd, MLMMJ_RSET);

			/* RFC 1869 - 4.7. - Clients that do this should be
			 * aware that many implementations will return a failure
			 * code (e.g., 503 Bad sequence of commands) in response
			 * to the RSET. This code can be safely ignored.
			 */
			free(reply);

			/* Try HELO on the same connection
			 */
		}

		write_helo(*sockfd, heloname);
		if((reply = checkwait_smtpreply(*sockfd, MLMMJ_HELO))
				== NULL) {
			/* EHLO successful don't try more */
			break;
		}
		if (try_ehlo) {
			free(reply);
			/* We reused a connection we tried EHLO on. Maybe
			 * that's why it failed. Try with new connection.
			 */
			endsmtp(sockfd);
			try_ehlo = 0;
			continue;
		}

		log_error(LOG_ARGS, "Error with HELO. Reply: "
				"[%s]. Check smtphelo configuration.",
				reply);
		free(reply);
		endsmtp(sockfd);
		retval = MLMMJ_HELO;
		break;

	} while (1);

	return retval;
}

int
endsmtp(int *sockfd)
{
	int retval = 0;
	char *reply = NULL;

	if(*sockfd == -1)
		return retval;
	
	write_quit(*sockfd);
	reply = checkwait_smtpreply(*sockfd, MLMMJ_QUIT);
	if(reply) {
		log_error(LOG_ARGS, "Mailserver would not let us QUIT. "
			  "We close the socket anyway though. "
			  "Mailserver reply = [%s]", reply);
		free(reply);
		retval = MLMMJ_QUIT;
	}

	close(*sockfd);
	*sockfd = -1;

	return retval;
}

int do_bouncemail(int listfd, int ctrlfd, const char *from)
{
	/* expected format for the from: "something+anything-<number>-anything@anything" */
	char *tofree;
	char *myfrom = xstrdup(from);
	char *listdelim = getlistdelim(ctrlfd);
	char *addr, *num, *c;

	tofree = myfrom;
	if((c = strchr(myfrom, '@')) == NULL) {
		free(myfrom);
		free(listdelim);
		return 0; /* Success when malformed 'from' */
	}
	*c = '\0';
	num = strrchr(myfrom, '-');
	if (num == NULL) {
		free(tofree);
		return (0); /* Success when malformed 'from' */
	}
	num++;
	c = strstr(myfrom, listdelim);
	if (c == NULL) {
		free(tofree);
		return (0); /* Success when malformed 'from' */
	}
	myfrom = strchr(c, '-'); /* malformed entry with delimiter after the -num */
	if (myfrom == NULL) {
		free(tofree);
		return (0);
	}
	myfrom++;
	if (num <= myfrom) {
		free(tofree);
		return (0); /* Success when malformed 'from' */
	}
	addr = xstrndup(myfrom, num - myfrom - 1);
	free(listdelim);

	bouncemail(listfd, num, addr);
	free(tofree);
	free(addr);
	return 1;
}

int
send_mail(int sockfd, struct mail *mail, int listfd, int ctrlfd, bool bounce)
{
	int retval = 0;
	char *reply, *reply2;

	if(sockfd == -1)
		return EBADF;

	if (mail->to == NULL) {
		errno = 0;
		log_error(LOG_ARGS, "No 'to' address, ignoring");
		return (0);
	}

	if(strchr(mail->to, '@') == NULL) {
		errno = 0;
		log_error(LOG_ARGS, "No @ in address, ignoring %s",
		    mail->to);
		return 0;
	}

	if (mail->from == NULL) {
		errno = 0;
		log_error(LOG_ARGS, "No from address, ignoring");
		return 0;
	}

	if (mail->fp == NULL) {
		errno = 0;
		log_error(LOG_ARGS, "No mail to send, ignoring");
		return 0;
	}
	rewind(mail->fp);

	retval = write_mail_from(sockfd, mail->from, "");
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
	retval = write_rcpt_to(sockfd, mail->to);
	if(retval) {
		log_error(LOG_ARGS, "Could not write RCPT TO:\n");
		return retval;
	}

	reply = checkwait_smtpreply(sockfd, MLMMJ_RCPTTO);
	if(reply) {
		write_rset(sockfd);
		reply2 = checkwait_smtpreply(sockfd, MLMMJ_RSET);
		if (reply2 != NULL) free(reply2);
		if(bounce && ((reply[0] == '4') || (reply[0] == '5'))
				&& (reply[1] == '5')) {
			free(reply);
			return do_bouncemail(listfd, ctrlfd, mail->from);
		} else {
			log_error(LOG_ARGS, "Error in RCPT TO. RCPT = [%s],"
			    " Reply = [%s]", mail->from, reply);
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

	if(mail->replyto) {
		retval = write_replyto(sockfd, mail->replyto);
		if(retval) {
			log_error(LOG_ARGS, "Could not write reply-to addr.\n");
			return retval;
		}
	}

	if (mail->xforwardedto) {
		if (dprintf(sockfd, "X-Forwarded-To: %s\r\n", mail->to) < 0) {
			log_error(LOG_ARGS, "Could not write X-Forwarded-To header\n");
			return -1;
		}
	}
	if (mail->xsignedrecipient) {
		if (dprintf(sockfd, "X-Signed-Recipient: i=1; %s\r\n", mail->to) < 0) {
			log_error(LOG_ARGS, "Could not write X-Signed-Recipient header\n");
			return -1;
		}
	}

	retval = write_mailbody(sockfd, mail->fp,
	    mail->addtohdr ? mail->to : NULL);
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

int
newsmtp(struct ml *ml, const char *rhost)
{
	int sockfd;
	char *relayhost = NULL, *smtphelo;
	unsigned short smtpport = ctrlushort(ml->ctrlfd, "smtpport", 25);

	if (rhost != NULL)
		relayhost = xstrdup(rhost);
	if (relayhost == NULL)
		relayhost = ctrlvalue(ml->ctrlfd, "relayhost");
	if (relayhost == NULL)
		relayhost = xstrdup(RELAYHOST);
	smtphelo = ctrlvalue(ml->ctrlfd, "smtphelo");
	if (smtphelo == NULL)
		smtphelo = hostnamestr();
	if (initsmtp(&sockfd, relayhost, smtpport, smtphelo) != 0) {
		free(relayhost);
		free(smtphelo);
		return (-1);
	}
	free(relayhost);
	free(smtphelo);
	return (sockfd);
}

static bool
save_file(const char *name, const char *ext, const char *content)
{
	char *tmpstr;
	int fd;

	xasprintf(&tmpstr, "%s.%s", name, ext);
	fd = open(tmpstr, O_WRONLY|O_CREAT|O_EXCL|O_SYNC, S_IRUSR|S_IWUSR);
	if (fd == -1) {
		free(tmpstr);
		return (false);
	}
	dprintf(fd, "%s", content);
	close(fd);
	free(tmpstr);
	return (true);
}

void
save_queue(const char *queuefilename, struct mail *mail)
{
	if (!save_file(queuefilename, "mailfrom", mail->from))
		return;
	if (!save_file(queuefilename, "reciptto", mail->to))
		return;
	if (mail->replyto != NULL)
		save_file(queuefilename, "reply-to", mail->replyto);
}

bool
send_single_mail(struct mail *mail, struct ml *ml, bool bounce)
{
	int sockfd;

	sockfd = newsmtp(ml, NULL);
	if (sockfd == -1)
		return (false);
	if (send_mail(sockfd, mail, ml->fd, ml->ctrlfd, bounce)) {
		endsmtp(&sockfd);
		return (false);
	}
	endsmtp(&sockfd);
	return (true);
}

bool
requeuemail(int listfd, int index, strlist *addrs, const char *addr)
{
	int addrfd, dfd;
	char *dirname;

	if ((addrs == NULL || addrs->len == 0) && addr == NULL)
		return (false);
	xasprintf(&dirname, "requeue/%d", index);
	if(mkdirat(listfd, dirname, 0750) < 0 && errno != EEXIST) {
		log_error(LOG_ARGS, "Could not mkdir(%s) for "
				"requeueing. Mail cannot "
				"be requeued.", dirname);
		free(dirname);
		return (false);
	}
	dfd = openat(listfd, dirname, O_DIRECTORY);
	if(dfd < 0) {
		log_error(LOG_ARGS, "Could not open directory %s",
		    dirname);
		free(dirname);
		return (false);
	}
	addrfd = openat(dfd, "subscribers", O_WRONLY|O_CREAT|O_APPEND,
			S_IRUSR|S_IWUSR);
	if(addrfd < 0) {
		log_error(LOG_ARGS, "Could not open %s/subscribers",
		    dirname);
		free(dirname);
		close(dfd);
		return (false);
	}
	free(dirname);
	close(dfd);
	/* Dump the remaining addresses. We dump the remaining before
	 * we write the failing address to ensure the potential good
	 * ones will be tried first when mlmmj-maintd sends out mails
	 * that have been requeued. addrcount was so far we were */
	{
		FILE *addrf = fdopen(addrfd, "a");
		if (addrf == NULL) {
			close(addrfd);
			return (false);
		}
		if (addrs != NULL) {
			vec_foreach(*addrs, _i) {
				fprintf(addrf, "%s\n", addrs->d[_i]);
			}
		}
		if (addr != NULL)
			fprintf(addrf, "%s\n", addr);
		fclose(addrf);
	}

	return (true);
}

char *
get_bounce_from_adr(const char *recipient, struct ml *ml, int index)
{
	char *bounceaddr, *myrecipient;
	char *a = NULL;
	char *staticbounceaddr, *staticbounceaddr_localpart = NULL;
	const char *staticbounceaddr_domain = NULL;

	myrecipient = xstrdup(recipient);
	a = strchr(myrecipient, '@');
	if (a)
		*a = '=';

	staticbounceaddr = ctrlvalue(ml->ctrlfd, "staticbounceaddr");
	if (staticbounceaddr) {
		staticbounceaddr_localpart = genlistname(staticbounceaddr);
		staticbounceaddr_domain = genlistfqdn(staticbounceaddr);
	}

	if (staticbounceaddr) {
		xasprintf(&bounceaddr, "%s%s%s-bounces-%d-%s@%s",
			staticbounceaddr_localpart, ml->delim, ml->name,
			index, myrecipient, staticbounceaddr_domain);

		free(staticbounceaddr);
		free(staticbounceaddr_localpart);
	} else {
		xasprintf(&bounceaddr, "%s%sbounces-%d-%s@%s", ml->name,
		    ml->delim, index, myrecipient, ml->fqdn);
	}

	free(myrecipient);

	return bounceaddr;
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

