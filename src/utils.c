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

#include <fcntl.h>
#include <sys/wait.h>
#include <sys/param.h>

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <inttypes.h>
#include <spawn.h>
#include <stdbool.h>
#include <unistd.h>

#include "xmalloc.h"
#include "utils.h"
#include "getlistdelim.h"
#include "chomp.h"

extern char **environ;

intmax_t
strtoim(const char *np, intmax_t minval, intmax_t maxval, const char **errpp)
{
	char *endp;
	intmax_t ret;

	if (errpp != NULL)
		*errpp = NULL;
	if (minval > maxval || np == NULL) {
		errno = EINVAL;
		if (errpp != NULL)
			*errpp = "invalid";
		return (0);
	}
	errno = 0;
	ret = strtoimax(np, &endp, 10);
	if (endp == np || *endp != '\0') {
		errno = EINVAL;
		if (errpp != NULL)
			*errpp = "invalid";
		return (0);
	}
	if (ret < minval) {
		errno = ERANGE;
		if (errpp != NULL)
			*errpp = "too small";
		return (0);
	}
	if (errno == ERANGE || ret > maxval) {
		errno = ERANGE;
		if (errpp != NULL)
			*errpp = "too large";
		return (0);
	}
	return (ret);
}

time_t
strtotimet(const char *np, const char **errpp)
{
	if (sizeof(time_t) == 4)
		return ((time_t)strtoim(np, INT_MIN, INT_MAX, errpp));
	return ((time_t)strtoim(np, LLONG_MIN, LLONG_MAX, errpp));
}

uintmax_t
strtouim(const char *np, uintmax_t minval, uintmax_t maxval, const char **errpp)
{
	char *endp;
	uintmax_t ret;

	if (errpp != NULL)
		*errpp = NULL;
	if (minval > maxval || np == NULL) {
		errno = EINVAL;
		if (errpp != NULL)
			*errpp = "invalid";
		return (0);
	}
	errno = 0;
	ret = strtoumax(np, &endp, 10);
	if (endp == np || *endp != '\0') {
		errno = EINVAL;
		if (errpp != NULL)
			*errpp = "invalid";
		return (0);
	}
	if (ret < minval) {
		errno = ERANGE;
		if (errpp != NULL)
			*errpp = "too small";
		return (0);
	}
	if (errno == ERANGE || ret > maxval) {
		errno = ERANGE;
		if (errpp != NULL)
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

void
exec_or_die(const char *arg, ...)
{
	va_list ap;
	const char **argv;
	int n;

	va_start(ap, arg);
	n = 1;
	while (va_arg(ap, char *) != NULL)
		n++;
	va_end(ap);
	argv = xmalloc((n +1) * sizeof(argv));
	va_start(ap, arg);
	argv[0] = arg;
	n = 1;
	while ((argv[n] = va_arg(ap, char *)) != NULL)
		n++;
	va_end(ap);
	execvp(argv[0], (char *const *)argv);
	err(EXIT_FAILURE, "Execution failed for '%s'", argv[0]);
}

int
exec_and_wait(const char *arg, ...)
{
	va_list ap;
	const char **argv;
	int n;
	pid_t p;
	int pstat;

	va_start(ap, arg);
	n = 1;
	while (va_arg(ap, char *) != NULL)
		n++;
	va_end(ap);
	argv = xmalloc((n +1) * sizeof(argv));
	va_start(ap, arg);
	argv[0] = arg;
	n = 1;
	while ((argv[n] = va_arg(ap, char *)) != NULL)
		n++;
	va_end(ap);

	if (0 != posix_spawnp(&p, argv[0], NULL, NULL,
				(char *const *) argv, environ)) {
		warn("Execution failed for '%s'", argv[0]);
		free(argv);
		return (-1);
	}
	while (waitpid(p, &pstat, 0) == -1) {
		if (errno != EINTR)
			warn("Could not wait for the execution of '%s'", argv[0]);
		free(argv);
		return (-1);
	}
	free(argv);

	return (WEXITSTATUS(pstat));
}

bool
lock(int fd, bool wr)
{
	struct flock lck;
	int r;

	if (fd == -1)
		return (false);
	lck.l_type = wr ? F_WRLCK : F_RDLCK;
	lck.l_whence = SEEK_SET;
	lck.l_start = 0;
	lck.l_len = 0;
	int i = 0;
	do {
		i++;
		if (i == 2) {
			warn("plop");
			exit(1);
		}
		r = fcntl(fd, F_SETLKW, &lck);
	} while (r < 0 && errno != EINTR);
	if (r == -1) {
		close(fd);
		return (false);
	}
	return (true);
}

char *
get_recipextra_from_env(int ctrlfd)
{
	const char *envstr;
	char *listdelim, *recipextra;

	/* address extension (the "foo" part of "user+foo@domain.tld") */
	/* qmail */
	if((envstr = getenv("DEFAULT")) != NULL)
		return (xstrdup(envstr));
	/* postfix */
	if((envstr = getenv("EXTENSION")) != NULL)
		return (xstrdup(envstr));
	/* exim */
	if ((envstr = getenv("LOCAL_PART_SUFFIX")) != NULL) {
		listdelim = getlistdelim(ctrlfd);
		if (strncmp(envstr, listdelim, strlen(listdelim)) == 0)
			recipextra = xstrdup(envstr + strlen(listdelim));
		else
			recipextra = xstrdup(envstr);
		return (recipextra);
	}
	return (NULL);
}

bool
addrmatch(const char *listaddr, const char *addr,
    const char *listdelim, char **recipextra)
{
	char *delim, *atsign;
	size_t len;

	if (recipextra)
		*recipextra = NULL;

	if (addr == NULL)
		return (false);

	if(strcasecmp(listaddr, addr) == 0)
		return (true);

	if (listdelim == NULL)
		return (false);

	delim = strstr(addr, listdelim);
	if (delim == NULL)
		return (false);

	len = delim - addr;
	if(strncasecmp(listaddr, addr, len) != 0)
		return (false);
	if(*(listaddr + len) != '@')
		return (false);

	delim += strlen(listdelim);

	atsign = strrchr(delim, '@');
	if (!atsign)
		return (false);

	if(strcasecmp(listaddr + len + 1, atsign + 1) != 0)
		return (false);

	if (recipextra) {
		len = atsign - delim;
		if (len > 0)
			*recipextra = xstrndup(delim, len);
	}

	return (true);
}

bool
find_email(strlist *list, strlist *matches, const char *delim, char **recipextra)
{
	tll_foreach(*list, lit) {
		tll_foreach(*matches, mit) {
			if (addrmatch(mit->item, lit->item, delim, recipextra))
				return true;
		}
	}
	return false;
}

char *
readlf(int fd, bool oneline)
{
	FILE *fp;
	size_t bufcap = 0;
	ssize_t buflen;
	char *buf = NULL;

	fp = fdopen(fd, "r");
	if (fp == NULL) {
		close(fd);
		return (NULL);
	}

	if (oneline) {
		buflen = getline(&buf, &bufcap, fp);
		if (buflen > 0)
			chomp(buf);
	} else {
		buflen = getdelim(&buf, &bufcap, EOF, fp);
	}
	fclose(fp);
	if (buflen < 0) {
		free(buf);
		buf = NULL;
	}
	return (buf);
}
