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

#ifndef CTRLVALUE_H
#define CTRLVALUE_H

#include <stdint.h>
#include <time.h>
#include <limits.h>

char *ctrlvalue(int ctrlfd, const char *ctrlstr);
char *ctrlcontent(const char *listdir, const char *ctrlstr);
char *textcontent(const char *listdir, const char *ctrlstr);
intmax_t ctrlim(int ctrlfd, const char *ctrlstr, intmax_t min,
    intmax_t max, intmax_t fallback);
uintmax_t ctrluim(int ctrlfd, const char *ctrlstr, uintmax_t min,
    uintmax_t max, uintmax_t fallback);
static inline int
ctrlint(int ctrlfd, const char *ctrlstr, int fallback)
{
	return (ctrlim(ctrlfd, ctrlstr, INT_MIN, INT_MAX, fallback));
}
static inline size_t
ctrlsizet(int ctrlfd, const char *ctrlstr, uintmax_t fallback)
{
	return (ctrluim(ctrlfd, ctrlstr, 0, SIZE_MAX, fallback));
}
static inline unsigned short
ctrlushort(int ctrlfd, const char *ctrlstr, unsigned short fallback)
{
	return (ctrluim(ctrlfd, ctrlstr, 0, USHRT_MAX, fallback));
}
time_t ctrltimet(int dfd, const char *ctrlstr, time_t fallback);
static inline long
ctrllong(int ctrlfd, const char *ctrlstr, unsigned short fallback)
{
	return (ctrlim(ctrlfd, ctrlstr, LONG_MIN, LONG_MAX, fallback));
}

#endif /* CTRLVALUE_H */
