/*-
 * Copyright(c) 2025 Baptiste Daroussin <bapt@FreeBSD.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#ifndef STRVIEW_H
#define STRVIEW_H

#include <ctype.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

/*
 * strview_t is a lightweight, non-owning view into a character buffer.
 * It pairs a pointer with a length, enabling zero-copy string operations
 * (splitting, trimming, comparing) without allocating memory.
 *
 * Create from a C string:
 *   strview_t sv = sv("hello world");
 *
 * Create from a buffer with explicit length:
 *   strview_t sv = { .ptr = buf, .len = n };
 */

typedef struct {
	const char *ptr;
	size_t len;
} strview_t;

/* Create a view from a nul-terminated C string (O(n) strlen) */
#define sv(s)                 ((strview_t){ .ptr = (s), .len = strlen(s) })

/* Accessors */
#define sv_data(sv)           ((sv).ptr)
#define sv_len(sv)            ((sv).len)
#define sv_empty(sv)          ((sv).len == 0)

/*
 * Compare two views for byte-for-byte equality.
 */
static inline bool
sv_equal(strview_t a, strview_t b)
{

	return (a.len == b.len && memcmp(a.ptr, b.ptr, a.len) == 0);
}

/*
 * Compare a view with a nul-terminated C string.
 */
static inline bool
sv_equal_c(strview_t sv, const char *s)
{
	size_t slen = strlen(s);

	return (sv.len == slen && memcmp(sv.ptr, s, slen) == 0);
}

/*
 * Split: return the first token delimited by `delim` and advance `sv`
 * past the delimiter.  If no delimiter is found the entire view is
 * returned and `sv` is advanced to the end (empty).
 *
 *   strview_t rem = sv("a,b,c");
 *   strview_t a   = sv_split(&rem, ',');  // a   = "a"
 *   strview_t b   = sv_split(&rem, ',');  // b   = "b"
 *   strview_t c   = sv_split(&rem, ',');  // c   = "c"
 *   sv_empty(rem)                          // true
 */
static inline strview_t
sv_split(strview_t *sv, char delim)
{
	for (size_t i = 0; i < sv->len; i++) {
		if (sv->ptr[i] == delim) {
			strview_t token = { .ptr = sv->ptr, .len = i };

			sv->ptr += i + 1;
			sv->len -= i + 1;
			return (token);
		}
	}
	/* No delimiter: consume everything */
	strview_t token = *sv;

	sv->ptr += sv->len;
	sv->len = 0;
	return (token);
}

/*
 * Trim leading and trailing whitespace (space, tab, \n, \r).
 */
static inline strview_t
sv_trim(strview_t sv)
{

	while (sv.len > 0 && (sv.ptr[0] == ' ' || sv.ptr[0] == '\t' ||
	    sv.ptr[0] == '\n' || sv.ptr[0] == '\r')) {
		sv.ptr++;
		sv.len--;
	}
	while (sv.len > 0 && (sv.ptr[sv.len - 1] == ' ' ||
	    sv.ptr[sv.len - 1] == '\t' || sv.ptr[sv.len - 1] == '\n' ||
	    sv.ptr[sv.len - 1] == '\r')) {
		sv.len--;
	}
	return (sv);
}

/*
 * Convert a view to an integer via strtol.
 * Returns 0 on success, -1 on failure.
 */
static inline int
sv_to_int(strview_t sv, int *val)
{
	char buf[64];

	if (sv.len >= sizeof(buf))
		return (-1);
	memcpy(buf, sv.ptr, sv.len);
	buf[sv.len] = '\0';

	char *endptr;
	long v = strtol(buf, &endptr, 10);

	if (endptr != buf + sv.len)
		return (-1);
	*val = (int)v;
	return (0);
}

/*
 * Convert a view to a malloc'd, nul-terminated C string.
 * The caller must free() the result.
 */
static inline char *
sv_to_c(strview_t sv)
{
	char *s = malloc(sv.len + 1);

	if (s == NULL)
		abort();
	memcpy(s, sv.ptr, sv.len);
	s[sv.len] = '\0';
	return (s);
}

#endif /* STRVIEW_H */