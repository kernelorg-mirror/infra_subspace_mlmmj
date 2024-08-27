/*
 * Copyright (C) 2024 Baptiste Daroussin <bapt@FreeBSD.org>
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

#include <string.h>
#include <regex.h>
#include <errno.h>

#include "mlmmj.h" 
#include "xmalloc.h"

static char *action_strs[] = {
	"allowed",
	"sent",
	"denied",
	"moderated",
	"discarded"
};

actions_t
do_access(strlist *rules, strlist *headers, const char *from, char **msg)
{
	unsigned int match;
	char *rule_ptr;
	char errbuf[128];
	int err;
	actions_t act;
	unsigned int not;
	regex_t regexp;
	char *hdr;
	size_t i = 0;
	if (msg != NULL)
		*msg = NULL;

	if (rules == NULL)
		return (ACT_ALLOW);

	tll_foreach(*rules, it) {
		rule_ptr = it->item;
		if (strncmp(rule_ptr, "allow", 5) == 0) {
			rule_ptr += 5;
			act = ACT_ALLOW;
		} else if (strncmp(rule_ptr, "send", 4) == 0) {
			rule_ptr += 4;
			act = ACT_SEND;
		} else if (strncmp(rule_ptr, "deny", 4) == 0) {
			rule_ptr += 4;
			act = ACT_DENY;
		} else if (strncmp(rule_ptr, "moderate", 8) == 0) {
			rule_ptr += 8;
			act = ACT_MODERATE;
		} else if (strncmp(rule_ptr, "discard", 7) == 0) {
			rule_ptr += 7;
			act = ACT_DISCARD;
		} else {
			errno = 0;
			if (msg != NULL)
				xasprintf(msg, "Unable to parse rule #%d \"%s\":"
					" Missing action keyword. Denying post from \"%s\"",
					i, it->item, from);
			return ACT_DENY;
		}

		if (*rule_ptr == ' ') {
			rule_ptr++;
		} else if (*rule_ptr == '\0') {
			/* the rule is a keyword and no regexp */
			if (msg != NULL)
				xasprintf(msg, "access -"
					" A mail from \"%s\" was %s by rule #%d \"%s\"",
					from, action_strs[act], i, it->item);
			return act;
		} else {
			/* we must have space or end of string */
			errno = 0;
			if (msg != NULL)
				xasprintf(msg, "Unable to parse rule #%d \"%s\":"
					" Invalid character after action keyword."
					" Denying post from \"%s\"", i, it->item, from);
			return ACT_DENY;
		}

		if (*rule_ptr == '!') {
			rule_ptr++;
			not = 1;
		} else {
			not = 0;
		}

		/* remove unanchored ".*" from beginning of regexp to stop the
		 * regexp matching to loop so long time it seems like it's
		 * hanging */
		if (strncmp(rule_ptr, "^.*", 3) == 0) {
			rule_ptr += 3;
		}
		while (strncmp(rule_ptr, ".*", 2) == 0) {
			rule_ptr += 2;
		}

		if ((err = regcomp(&regexp, rule_ptr,
				REG_EXTENDED | REG_NOSUB | REG_ICASE))) {
			regerror(err, &regexp, errbuf, sizeof(errbuf));
			regfree(&regexp);
			errno = 0;
			if (msg != NULL)
				xasprintf(msg, "regcomp() failed for rule #%d \"%s\""
					" (message: '%s') (expression: '%s')"
					" Denying post from \"%s\"",
					i, it->item, errbuf, rule_ptr, from);
			return ACT_DENY;
		}

		match = 0;
		tll_foreach(*headers, header) {
			if (regexec(&regexp, header->item, 0, NULL, 0)
					== 0) {
				match = 1;
				hdr = header->item;
				break;
			}
		}

		regfree(&regexp);

		if (match != not) {
			if (match) {
				if (msg != NULL)
					xasprintf(msg, "access -"
						" A mail from \"%s\" with header \"%s\" was %s by"
						" rule #%d \"%s\"", from, hdr, action_strs[act],
						i, it->item);
			} else {
				if (msg != NULL)
					xasprintf(msg, "access -"
						" A mail from \"%s\" was %s by rule #%d \"%s\""
						" because no header matched.", from,
						action_strs[act], i, it->item);
			}
			return act;
		}
		i++;
	}

	xasprintf(msg, "access -"
			" A mail from \"%s\" didn't match any rules, and"
			" was denied by default.", from);
	return ACT_DENY;
}

