/*
 * Copyright (C) 2002, 2003, 2004 Mads Martin Joergensen <mmj at mmj.dk>
 * Copyright (C) 2022-2024 Baptiste Daroussin <bapt@FreeBSD.org>
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

#ifndef MLMMJ_GENERIC_INCLUDES
#define MLMMJ_GENERIC_INCLUDES

#include "config.h"
#include <stdbool.h>
#include <time.h>
#include <tllist.h>

#ifndef __unused
#define __unused __attribute__((__unused__))
#endif

#ifndef NELEM
#define NELEM(array) (sizeof(array) / sizeof((array)[0]))
#endif

#define RELAYHOST "127.0.0.1"
#define READ_BUFSIZE 2048
#define DEFAULT_RECIPDELIM "+"  /* Default recipient delimiter */
#define MODREQLIFE 604800 /* How long time will moderation requests be kept?
			   * 604800s is 7 days */
#define DISCARDEDLIFE 604800 /* How long time will discarded mails be kept?
			      * 604800s is 7 days */
#define CONFIRMLIFE 604800 /* How long time will (un)sub confirmations be kept?
			    * 604800s is 7 days */
#define BOUNCELIFE 432000 /* How long time can addresses bounce before
			     unsubscription happens? 432000s is 5 days
			     Tweakable with control/bouncelife */
#define WAITPROBE 43200   /* How long do we wait for a bounce of the probe
			     mail before concluding the address is no longer
			     bouncing? 43200 is 12 hours */
#define MAINTD_SLEEP 7200 /* How long between maintenance runs when
			     mlmmj-maintd runs daemonized? 7200s is 2 hours */
#define MAINTD_LOGFILE "mlmmj-maintd.lastrun.log"

#define DIGESTINTERVAL 604800  /* How long do we collect mails for digests
				* 604800s is 7 days */
#define DIGESTMAXMAILS 50 /* How many mails can accumulate before we send the
			   * digest */
#define DIGESTMIMETYPE "digest" /* Which sub-type of multipart to use when
				 * sending digest mails */
#define OPLOGFNAME "mlmmj.operation.log" /* logfile to log operations */
#define OPLOGSIZE 524288

#define gen_addr(addr, _ml, _cmd) \
    xasprintf(&addr, "%s%s%s@%s", (_ml)->name, (_ml)->delim, _cmd, (_ml)->fqdn)
#define gen_addr_cookie(addr, _ml, _cmd, _cookie) \
    xasprintf(&addr, "%s%s%s%s@%s", (_ml)->name, (_ml)->delim, _cmd, _cookie, (_ml)->fqdn);

struct ml {
	const char *dir;
	char *delim;
	char *name;
	const char *fqdn;
	char *addr;
	int fd;
	int ctrlfd;
};

typedef tll(char *) strlist;

typedef enum bounce {
	BOUNCE_OK,
	BOUNCE_FAIL,
	BOUNCE_DONE
} bounce_t;

#define MAXVERPRECIPS 100

struct mailhdr {
	const char *token;
	int valuecount;
	char **values;
};

/* Has to go here, since it's used in many places */

enum subtype {
	SUB_NORMAL = 0,
	SUB_DIGEST,
	SUB_NOMAIL,
	SUB_FILE, /* For single files (moderator, owner etc.) */
	SUB_ALL, /* For listing or unsubscribing all kinds of subscribers */
	SUB_BOTH, /* For normal+digest subscription */
	SUB_NONE /* For when an address is not subscribed at all */
};

extern char *subtype_strs[7]; /* count matches enum above; defined in subscriberfuncs.c */

enum subreason {
	SUB_REQUEST = 0,
	SUB_CONFIRM,
	SUB_PERMIT,
	SUB_ADMIN,
	SUB_BOUNCING,
	SUB_SWITCH
};

extern char * subreason_strs[6]; /* count matches enum above; defined in subscriberfuncs.c */

void print_version(const char *prg);
bool parse_lastdigest(char *line, long *lastindex, time_t *lasttime,
    long *lastissue, const char **errstr);
time_t extract_bouncetime(char *line, const char **errstr);
int open_subscriber_directory(int listfd, enum subtype typesub, const char **dir);
bool unsubscribe(int listfd, const char *address, enum subtype typesub);
bounce_t bouncemail(int listfd, const char *theaddress, const char *identifier);
void save_lastbouncedmsg(int listfd, const char *address, const char *mailname);
char *dsnparseaddr(const char *mailname);
int open_listdir(const char *listdir, bool usercheck);
void ml_init(struct ml *ml);
bool ml_open(struct ml *ml, bool checkuser);
void ml_close(struct ml *ml);
bool send_probe(struct ml *ml, const char *addr);

#define MY_ASSERT(expression) if (!(expression)) { \
			errno = 0; \
			log_error(LOG_ARGS, "assertion failed"); \
			exit(EXIT_FAILURE); \
		}

#define CHECKFULLPATH(name) if(strchr(name, '/') == NULL) { \
			errx(EXIT_FAILURE, "All mlmmj binaries have to " \
			    "be invoked with full path,\n" \
			    "e.g. /usr/local/bin/%s", name); \
			};

#endif /* MLMMJ_GENERIC_INCLUDES */
