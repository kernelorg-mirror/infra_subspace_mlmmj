/*
 * Copyright (C) 2004 Mads Martin Joergensen <mmj at mmj.dk>
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

#ifndef MLMMJ_MAINTD_H
#define MLMMJ_MAINTD_H

#include <sys/types.h>

int delolder(int fd, const char *dirname, time_t than);
int clean_moderation(int fd);
int clean_discarded(int fd);
int resend_queue(int fd, const char *mlmmjsend, const char *listdir);
int resend_requeue(int fd, const char *mlmmjsend, const char *listdir);
int clean_nolongerbouncing(int fd);
int probe_bouncers(int fd, const char *mlmmjbounce, const char *listdir);
int unsub_bouncers(int fd, const char *mlmmjunsub, const char *listdir);
int run_digests(int fd, const char *mlmmjsend, const char *listdir);

#endif /* MLMMJ_MAINTD_H */
