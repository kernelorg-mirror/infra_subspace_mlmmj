/*
 * Copyright (C) 2004, 2003, 2004 Mads Martin Joergensen <mmj at mmj.dk>
 * Copyright (C) 2022 Baptiste Daroussin <bapt@FreeBSD.org>
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
#include <unistd.h>

#include "checkwait_smtpreply.h"
#include "mail-functions.h"
#include "send_mail.h"
#include "log_error.h"
#include "init_sockfd.h"
#include "mlmmj.h"
#include "xmalloc.h"

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
				"[%s]", reply);
		free(reply);
		/* FIXME: quit and tell admin to configure
		 * correctly */
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
		printf("reply from quit: %s\n", reply);
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

