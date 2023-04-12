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

#pragma once

#include <sys/types.h>
#include <stdbool.h>
#include <stdio.h>
#include "mlmmj.h"

struct mail {
	const char *from;
	const char *to;
	const char *replyto;
	FILE *fp;
	bool addtohdr;
};

int newsmtp(struct ml *ml);
int initsmtp(int *sockfd, const char *relayhost, unsigned short port, const char *heloname);
int endsmtp(int *sockfd);
int send_mail(int sockfd, struct mail *mail, int listfd, int ctrlfd, bool bounce);
int do_bouncemail(int listfd, int ctrlfd, const char *from);
bool send_single_mail(struct mail *mail, struct ml *ml, bool bounce);
void save_queue(const char *queuefilename, struct mail *mail);
bool requeuemail(int listfd, int index, strlist *addrs, const char *addr);
char *bounce_from_adr(const char *recipient, const char *listadr, const char *listdelim, const char *mailfilename, int ctrlfd);
char *get_bounce_from_adr(const char *recipient, struct ml *ml, const char *mailfilename, int index);
int get_index_from_filename(const char *filename);

