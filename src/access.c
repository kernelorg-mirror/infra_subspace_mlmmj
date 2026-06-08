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

struct action_keyword {
	const char *keyword;
	actions_t action;
};

static const struct action_keyword action_keywords[] = {
	{ "allow",    ACT_ALLOW },
	{ "send",     ACT_SEND },
	{ "deny",     ACT_DENY },
	{ "moderate", ACT_MODERATE },
	{ "discard",  ACT_DISCARD },
};

static bool
parse_action(const char **rule_ptr, actions_t *act)
{
	for (size_t i = 0; i < NELEM(action_keywords); i++) {
		size_t len = strlen(action_keywords[i].keyword);
		if (strncmp(*rule_ptr, action_keywords[i].keyword, len) == 0) {
			*rule_ptr += len;
			*act = action_keywords[i].action;
			return (true);
		}
	}
	return (false);
}

actions_t
do_access(strlist *rules, strlist *headers, const char *from, char **msg, char **qualifier)
{
	unsigned int match;
	char *rule_ptr;
	char errbuf[128];
	int err;
	actions_t act;
	unsigned int not;
	regex_t regexp;
	char *hdr;
	if (msg != NULL)
		*msg = NULL;
	if (qualifier != NULL)
		*qualifier = NULL;

	if (rules == NULL)
		return (ACT_ALLOW);

	vec_foreach(*rules, _i) {
		rule_ptr = rules->d[_i];
		if (!parse_action((const char **)&rule_ptr, &act)) {

			errno = 0;
			if (msg != NULL)
				xasprintf(msg, "Unable to parse rule #%zu \"%s\":"
					" Missing action keyword. Denying post from \"%s\"",
					_i, rules->d[_i], from);
			return ACT_DENY;
		}

		if (*rule_ptr == ' ') {
			rule_ptr++;
		} else if (*rule_ptr == '-') {
			rule_ptr++;
			const char *qual = rule_ptr;
			while (*rule_ptr != ' ' && *rule_ptr != '\0')
				rule_ptr++;
			if (rule_ptr - qual != 0 && qualifier != NULL)
				*qualifier = xstrndup(qual, rule_ptr - qual);
			rule_ptr++;
		} else if (*rule_ptr == '\0') {
			/* the rule is a keyword and no regexp */
			if (msg != NULL)
				xasprintf(msg, "access -"
					" A mail from \"%s\" was %s by rule #%zu \"%s\"",
					from, action_strs[act], _i, rules->d[_i]);
			return act;
		} else {
			/* we must have space or end of string */
			errno = 0;
			if (msg != NULL)
				xasprintf(msg, "Unable to parse rule #%zu \"%s\":"
					" Invalid character after action keyword."
					" Denying post from \"%s\"", _i, rules->d[_i], from);
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
				xasprintf(msg, "regcomp() failed for rule #%zu \"%s\""
					" (message: '%s') (expression: '%s')"
					" Denying post from \"%s\"",
					_i, rules->d[_i], errbuf, rule_ptr, from);
			return ACT_DENY;
		}

		match = 0;
		vec_foreach(*headers, _j) {
			if (regexec(&regexp, headers->d[_j], 0, NULL, 0)
					== 0) {
				match = 1;
				hdr = headers->d[_j];
				break;
			}
		}

		regfree(&regexp);

		if (match != not) {
			if (match) {
				if (msg != NULL)
					xasprintf(msg, "access -"
						" A mail from \"%s\" with header \"%s\" was %s by"
						" rule #%zu \"%s\"", from, hdr, action_strs[act],
						_i, rules->d[_i]);
			} else {
				if (msg != NULL)
					xasprintf(msg, "access -"
						" A mail from \"%s\" was %s by rule #%zu \"%s\""
						" because no header matched.", from,
						action_strs[act], _i, rules->d[_i]);
			}
			return act;
		}
	}

	if (msg != NULL)
		xasprintf(msg, "access -"
				" A mail from \"%s\" didn't match any rules, and"
				" was denied by default.", from);
	return ACT_DENY;
}

