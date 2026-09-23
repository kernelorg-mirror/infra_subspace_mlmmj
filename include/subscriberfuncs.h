/*
 * Copyright (C) 2003 Mads Martin Joergensen <mmj at mmj.dk>
 * Copyright (C) 2023-2025 Baptiste Daroussin <bapt@FreeBSD.org>
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

#pragma once

#include <stdbool.h>

struct subscription {
	bool send_welcome_email;
	const char *modstr;
	enum subreason reasonsub;
	enum subtype typesub;
	bool gensubscribed;
	bool subconfirm;
	bool force;
	bool quiet;
	const char *mlmmjsend;
};

bool find_subscriber(int fd, const char *address);
const char *find_addr_line(const char *start, const char *end,
    const char *address);
int is_subbed_in(int fd, const char *subddirname, const char *address);
enum subtype is_subbed(int listfd, const char *address, bool both);
char *get_subcookie_content(int listfd, bool unsub, const char *param);
void notify_sub(struct ml *ml, const char *subaddr, enum subtype typesub,
		enum subreason reasonsub, bool sub);
void generate_subscription(struct ml *ml, const char *subaddr, enum subtype typesub, bool sub);
void generate_subconfirm(struct ml *ml, const char *subaddr, enum subtype typesub,
    enum subreason reasonsub, bool sub);
void send_confirmation_mail(struct ml *ml, const char *subaddr, enum subtype typesub, enum subreason reasonsub, bool sub);
bool do_unsubscribe(struct ml *ml, const char *addr, enum subtype typesub, enum subreason reasonsub, bool inform_not_subscribed, bool confirm_unsubscription, bool quiet, bool send_goodbye_mail);
void mod_get_addr_and_type(struct ml *ml, const char *modstr, char **addrptr, enum subtype *subtypeptr);
bool do_subscribe(struct ml *ml, struct subscription *sub, const char *addr);
char *get_mail_sender(int mailfd);
char *autosubscribe_sender(struct ml *ml, int mailfd, enum subtype typesub);
