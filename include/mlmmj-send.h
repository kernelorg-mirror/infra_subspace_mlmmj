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

#ifndef MMJML_SEND_H
#define MMJML_SEND_H

#include <sys/types.h>
#include <stdbool.h>
#include "send_mail.h"

int send_mail_many_fd(int sockfd, struct mail *mail, int subfd,
		   const char *listaddr, const char *listdelim,
		   const char *archivefilename, int listfd);
int send_mail_many_list(int sockfd, struct mail *mail, struct strlist *addrs,
		   const char *listaddr, const char *listdelim,
		   const char *archivefilename, int listfd);
int send_mail_verp(int sockfd, struct strlist *addrs, struct mail *mail, const char *extra);

#endif /* MMJML_SEND_H */
