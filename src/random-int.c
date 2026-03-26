/* Copyright (C) 2002, 2003 Mads Martin Joergensen <mmj at mmj.dk>
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

#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <time.h>
#include <stdint.h>

#include "wrappers.h"
#include "config.h"

int random_int(void)
{
#ifndef HAVE_ARC4RANDOM_UNIFORM
	int devrandom;
	uint32_t val;

	devrandom = open("/dev/urandom", O_RDONLY);
	if (devrandom < 0)
		devrandom = open("/dev/random", O_RDONLY);

	if (devrandom >= 0) {
		readn(devrandom, &val, sizeof(val));
		close(devrandom);
		return (int)(val & 0x7fffffff);
	}

	/* Last resort fallback if no random device available */
	srand((unsigned int)time(NULL) ^ (unsigned int)getpid());
	return rand();
#else
	return arc4random_uniform(INT_MAX);
#endif
}
#if 0
int main(int argc, char **argv)
{
	int i;
	
	for(i = 0; i < 25; i++)
		printf("%i\n", random_int());
	
	return 0;
}
#endif
