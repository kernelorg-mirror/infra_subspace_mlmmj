/* Copyright (C) 2004 Mads Martin Joergensen <mmj at mmj.dk>
 * Copyright (C) 2006 Morten K. Poulsen <morten at afdelingp.dk>
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

#include <stdlib.h>
#include <unistd.h>

#include "xmalloc.h"
#include "gethdrline.h"
#include "strgen.h"
#include "wrappers.h"
#include "log_error.h"
#include "chomp.h"


char *gethdrline(FILE *f, char **unfolded)
{
	char *line = NULL, *retstr = NULL, *oldretstr = NULL, *oldunfolded = NULL;
	int ch;
	size_t linecap = 0;

	if (getline(&line, &linecap, f) <= 0) {
		if (unfolded != NULL)
			*unfolded = NULL;
		return NULL;
	}

	if (unfolded != NULL)
		*unfolded = xstrdup(line);

	chomp(line);

	/* end-of-headers */
	if (*line == '\0') {
		if (unfolded != NULL) {
			free(*unfolded);
			*unfolded = NULL;
		}
		free(line);
		return NULL;
	}

	retstr = xstrdup(line);
	for(;;) {
		/* look ahead one char to determine if we need to fold */
		ch = fgetc(f);
		if (ch == EOF)
			goto out;

		ungetc(ch, f);
		if ((ch != '\t') && (ch != ' '))  /* no more folding */
			goto out;

		if (getline(&line, &linecap, f) <= 0)
			goto out;

		if (unfolded != NULL) {
			oldunfolded = *unfolded;
			xasprintf(unfolded, "%s%s",
			    oldunfolded != NULL ? oldunfolded : "", line);
			free(oldunfolded);
		}

		chomp(line);
		oldretstr = retstr;
		xasprintf(&retstr, "%s%s",
		    oldretstr != NULL ? oldretstr : "", line);
		free(oldretstr);
	}
out:
	free(line);
	return (retstr);
}
