/* Copyright (C) 2003 Mads Martin Joergensen <mmj at mmj.dk>
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

#ifndef LISTCONTROL_H
#define LISTCONTROL_H

#include "mlmmj.h"

enum ctrl_e {
	CTRL_SUBSCRIBE_DIGEST,
	CTRL_SUBSCRIBE_NOMAIL,
	CTRL_SUBSCRIBE_BOTH,
	CTRL_SUBSCRIBE,
	CTRL_CONFSUB_DIGEST,
	CTRL_CONFSUB_NOMAIL,
	CTRL_CONFSUB_BOTH,
	CTRL_CONFSUB,
	CTRL_UNSUBSCRIBE_DIGEST,
	CTRL_UNSUBSCRIBE_NOMAIL,
	CTRL_UNSUBSCRIBE,
	CTRL_CONFUNSUB_DIGEST,
	CTRL_CONFUNSUB_NOMAIL,
	CTRL_CONFUNSUB,
	CTRL_BOUNCES,
	CTRL_RELEASE,
	CTRL_REJECT,
	CTRL_PERMIT,
	CTRL_OBSTRUCT,
	CTRL_MODERATE,
	CTRL_HELP,
	CTRL_FAQ,
	CTRL_GET,
	CTRL_LIST,
};

struct ctrl_command {
	char *command;
	bool accepts_parameter;
	bool valid_when_closed_list;
	bool valid_when_closed_sub;
	enum ctrl_e type;
};

struct ctrl_command *get_ctrl_command(const char *controlstr, char **param);

int listcontrol(strlist *fromemails, struct ml *ml,
		const char *controlstr, const char *mlmmjsub,
		const char *mlmmjsend, const char *mailname);

#endif /* LISTCONTROL_H */
