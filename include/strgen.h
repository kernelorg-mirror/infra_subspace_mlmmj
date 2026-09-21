/*
 * Copyright (C) 2002, 2003 Mads Martin Joergensen <mmj at mmj.dk>
 * Copyright (C) 2023 Mads Martin Joergensen <mmj at mmj.dk>
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

#ifndef STRGEN_H
#define STRGEN_H

#include <stdbool.h>
#include <sys/types.h>

char *random_str(void);
char *random_plus_addr(const char *addr);
/*
 * Atomically create a uniquely-named file under dirfd.  Its name is
 * "<prefix><random>" (prefix may be NULL or empty); the file is opened
 * O_RDWR|O_CREAT|O_EXCL and the call retries on EEXIST.  On success
 * returns the open fd and, if name_out != NULL, a malloc'd copy of the
 * basename that was created (caller frees).  On other errors returns -1
 * with errno set and sets *name_out to NULL.  Use AT_FDCWD with an
 * absolute or list-relative prefix to create via an absolute path.
 */
int mkuniqfileat(int dirfd, const char *prefix, mode_t mode,
    char **name_out);
char *random_plus_addr(const char *addr);
char *genlistname(const char *listaddr);
const char *genlistfqdn(const char *listaddr);
char *hostnamestr(void);
char *mydirname(const char *path);
const char *mybasename(const char *path);
char *genmsgid(const char *fqdn);
char *gendatestr(void);
char *decode_qp(const char *qpstr, bool qformat);
bool splitlistaddr(const char *listaddr, char **listname, const char **listfqdn);
char *disarm_email(const char *from);

#endif /* STRGEN_H */
