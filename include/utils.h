/*
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

#pragma once

#include <sys/file.h>
#include <stdint.h>
#include <stdbool.h>

#include <time.h>
#include <fcntl.h>

#include "mlmmj.h"

char *lowercase(const char *);
intmax_t strtoim(const char *np, intmax_t minval, intmax_t maxval,
    const char **errpp);
uintmax_t strtouim(const char *, uintmax_t, uintmax_t, const char **);
void exec_or_die(const char *arg, ...);
int exec_and_wait(const char *arg, ...);
time_t strtotimet(const char *np, const char **errpp);
bool lock(int fd, bool write);
bool find_email(strlist *list, strlist *matches, const char *delim, char **recipextra);
char *readlf(int fd, bool online);
