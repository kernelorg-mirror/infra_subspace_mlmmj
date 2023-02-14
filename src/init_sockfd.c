/* Copyright (C) 2002, 2003 Mads Martin Joergensen <mmj at mmj.dk>
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

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <inttypes.h>
#include <netdb.h>

#include "init_sockfd.h"
#include "log_error.h"

void init_sockfd(int *sockfd, const char *relayhost, unsigned short port)
{
	int on, sd = -1;
	struct addrinfo *ai = NULL, *curai, hints;
	char srv[NI_MAXSERV];
	*sockfd = -1;

	memset(&hints, 0, sizeof(hints));
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_family = PF_UNSPEC;
	snprintf(srv, sizeof(srv), "%d", port);
	if (getaddrinfo(relayhost, srv, &hints, &ai) != 0) {
		log_error(LOG_ARGS, "Unable to lookup for relayhost %s:%s",
		    relayhost, srv);
		return;
	}

	for (curai = ai; curai != NULL; curai = curai->ai_next) {
		if ((sd = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol)) < 0) {
			continue;
		}
		if (connect(sd, ai->ai_addr, ai->ai_addrlen) != 0) {
			close(sd);
			sd = -1;
			continue;
		}
		break;
	}
	freeaddrinfo(ai);
	if (sd == -1) {
		log_error(LOG_ARGS, "Could not connect to %s", relayhost);
		return;
	}
	*sockfd = sd;

	on = 1;
	if(setsockopt(*sockfd, SOL_SOCKET, SO_KEEPALIVE, (void *)&on,
				sizeof(on)) < 0)
		log_error(LOG_ARGS, "Could not set SO_KEEPALIVE");
}
