/*-
 * Copyright(c) 2026 Baptiste Daroussin <bapt@FreeBSD.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 *
 */

#include "sb.h"

void
sb_grow(sb_t *sb, size_t needed)
{
	if (needed <= sb->cap)
		return;
	size_t newcap = sb->cap ? sb->cap * 2 : 512;
	while (newcap < needed)
		newcap *= 2;
	char *nd = realloc(sb->d, newcap);
	if (nd == NULL)
		abort();
	sb->d = nd;
	sb->cap = newcap;
}

void
sb_cat(sb_t *sb, const char *s)
{
	size_t slen = strlen(s);
	sb_grow(sb, sb->len + slen + 1);
	memcpy(sb->d + sb->len, s, slen + 1);
	sb->len += slen;
}

void
sb_cat_n(sb_t *sb, const char *s, size_t n)
{
	if (n == 0)
		return;
	sb_grow(sb, sb->len + n + 1);
	memcpy(sb->d + sb->len, s, n);
	sb->len += n;
	sb->d[sb->len] = '\0';
}

void
sb_cat_c(sb_t *sb, char c)
{
	sb_grow(sb, sb->len + 2);
	sb->d[sb->len++] = c;
	sb->d[sb->len] = '\0';
}

void
sb_printf(sb_t *sb, const char *fmt, ...)
{
	va_list ap;
	int n;

	if (sb->cap == 0)
		sb_grow(sb, 512);

	for (;;) {
		size_t remain = sb->cap - sb->len;
		va_start(ap, fmt);
		n = vsnprintf(sb->d + sb->len, remain, fmt, ap);
		va_end(ap);

		if (n < 0)
			abort();

		if ((size_t)n < remain) {
			sb->len += (size_t)n;
			return;
		}

		sb_grow(sb, sb->len + (size_t)n + 1);
	}
}

void
sb_fini(sb_t *sb)
{
	if (sb == NULL)
		return;
	free(sb->d);
	sb->d = NULL;
	sb->len = 0;
	sb->cap = 0;
}

void
sb_reset(sb_t *sb)
{
	if (sb == NULL)
		return;
	sb->len = 0;
	if (sb->d)
		sb->d[0] = '\0';
}

char *
sb_get(sb_t *sb)
{
	if (sb == NULL)
		return (NULL);
	if (sb->d == NULL) {
		sb_grow(sb, 1);
		sb->d[0] = '\0';
	}
	char *ret = sb->d;
	sb->d = NULL;
	sb->len = 0;
	sb->cap = 0;
	return (ret);
}

char *
sb_str(sb_t *sb)
{
	if (sb->d == NULL) {
		sb_grow(sb, 1);
		sb->d[0] = '\0';
	}
	return (sb->d);
}

