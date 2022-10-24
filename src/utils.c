/*
 * Copyright (C) 2021-2022 Baptiste Daroussin <bapt@FreeBSD.org>
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

#include <inttypes.h>
#include <ctype.h>
#include <errno.h>

#include "xmalloc.h"
#include "utils.h"

intmax_t
strtoim(const char *np, intmax_t minval, intmax_t maxval, const char **errpp)
{
	char *endp;
	intmax_t ret;

	*errpp = NULL;
	if (minval > maxval || np == NULL) {
		errno = EINVAL;
		*errpp = "invalid";
		return (0);
	}
	errno = 0;
	ret = strtoimax(np, &endp, 10);
	if (endp == np || *endp != '\0') {
		errno = EINVAL;
		*errpp = "invalid";
		return (0);
	}
	if (ret < minval) {
		errno = ERANGE;
		*errpp = "too small";
		return (0);
	}
	if (errno == ERANGE || ret > maxval) {
		errno = ERANGE;
		*errpp = "too large";
		return (0);
	}
	return (ret);
}

uintmax_t
strtouim(const char *np, uintmax_t minval, uintmax_t maxval, const char **errpp)
{
	char *endp;
	uintmax_t ret;

	*errpp = NULL;
	if (minval > maxval || np == NULL) {
		errno = EINVAL;
		*errpp = "invalid";
		return (0);
	}
	errno = 0;
	ret = strtoumax(np, &endp, 10);
	if (endp == np || *endp != '\0') {
		errno = EINVAL;
		*errpp = "invalid";
		return (0);
	}
	if (ret < minval) {
		errno = ERANGE;
		*errpp = "too small";
		return (0);
	}
	if (errno == ERANGE || ret > maxval) {
		errno = ERANGE;
		*errpp = "too large";
		return (0);
	}
	return (ret);
}

char *
lowercase(const char *address)
{
	char *loweraddress;
	int i = 0;

	loweraddress = xstrdup(address);
	while (loweraddress[i] != '\0') {
		loweraddress[i] = tolower(loweraddress[i]);
		i++;
	}
	return (loweraddress);
}
