/*
 * Copyright (C) 2004 Morten K. Poulsen <morten at afdelingp.dk>
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

#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <sys/types.h>
#include <unistd.h>

#include "xmalloc.h"
#include "log_error.h"
#include "config.h"

#ifdef HAVE_SYSLOG_H
#include <syslog.h>
#endif

static char *log_name = NULL;
static bool syslog_is_open = false;
#ifdef HAVE_SYSLOG
static int syslog_logopt = LOG_PID|LOG_CONS;
#endif

static void
_openlog(bool reopen)
{
#ifdef HAVE_SYSLOG
	if (!reopen) {
		if (syslog_is_open)
			return;
	} else {
		if (!syslog_is_open)
			closelog();
	}
	openlog(log_name, syslog_logopt, LOG_MAIL);
	syslog_is_open = true;
#endif
}

void log_set_name(const char* name)
{
	free(log_name);
	log_name = xstrdup(name);

	_openlog(true);
}

void
log_activate_stderr(void)
{
#ifdef HAVE_SYSLOG
	syslog_logopt |= LOG_PERROR;
	_openlog(true);
#endif
}

void
log_set_namef(const char *fmt, ...)
{
	char *buf;
	va_list ap;
	int i;

	va_start(ap, fmt);
	i = vasprintf(&buf, fmt, ap);
	va_end(ap);
	if (i < 0 || buf == NULL)
		abort();

	free(log_name);
	log_name = buf;

	_openlog(true);
}

void log_free_name()
{
	free(log_name);
}

void
log_err(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
#ifdef HAVE_SYSLOG
	_openlog(false);
	vsyslog(LOG_ERR, fmt, ap);
#else
	char *buf;
	i = vasprintf(&buf, fmt, ap);
	fprintf(stderr, "%s[%d]: %s\n", log_name, (int) getpid(), buf);
	free(buf);
#endif
	va_end(ap);
}

void log_error(const char *file, int line, const char *errstr,
	const char *fmt, ...)
{
	char *buf;
	va_list ap;
	int i;

	va_start(ap, fmt);
	i = vasprintf(&buf, fmt, ap);
	va_end(ap);

	if (i < 0 || buf == NULL)
		abort();

	if (!log_name) log_name = "mlmmj-UNKNOWN";

#ifdef HAVE_SYSLOG
	_openlog(true);
	syslog(LOG_ERR, "%s:%d: %s: %s", file, line, buf, errstr);
#else
	fprintf(stderr, "%s[%d]: %s:%d: %s: %s\n", log_name, (int)getpid(),
			file, line, buf, errstr);
#endif
	free(buf);
}
