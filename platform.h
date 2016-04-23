#ifndef XT_PLATFORM_H
#define XT_PLATFORM_H

/* platform.h -- Platform-specific code.

   This file is a union of stuff in freebsd/, osx/, linux/ and netbsd/
   directories.  Instead of different subdirectory for each file, this
   header, via  preprocessor logic, provides the  needed functionality
   on each platform.  This simplifies build.
 */
#ifdef __OpenBSD__
#include <util.h>
#endif	/* ifdef __OpenBSD__ */

#if defined(__FreeBSD__) || defined(__DragonflyBSD__)
#include <libutil.h>
#endif /* if defined(__FreeBSD__) || defined(__DragonflyBSD__) */

#ifndef __OpenBSD__
typedef enum {
	NONE = 0, KILO = 1, MEGA = 2, GIGA = 3, TERA = 4, PETA = 5, EXA = 6
} unit_type;

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>

/* These three arrays MUST be in sync!  XXX make a struct */
static unit_type units[] = { NONE, KILO, MEGA, GIGA, TERA, PETA, EXA };
static char scale_chars[] = "BKMGTPE";
static long long scale_factors[] = {
	1LL,
	1024LL,
	1024LL*1024,
	1024LL*1024*1024,
	1024LL*1024*1024*1024,
	1024LL*1024*1024*1024*1024,
	1024LL*1024*1024*1024*1024*1024,
};
#define	SCALE_LENGTH (sizeof(units)/sizeof(units[0]))

#define MAX_DIGITS (SCALE_LENGTH * 3)	/* XXX strlen(sprintf("%lld", -1)? */


#define FMT_SCALED_STRSIZE	7	/* minus sign, 4 digits, suffix, null byte */

int scan_scaled(char *scaled, long long *result);
int fmt_scaled(long long number, char *result);
#endif	/* ifndef __OpenBSD__ */

#if defined(__NetBSD__)||defined(__linux__)||defined(__APPLE__)
#include <errno.h>
#include <limits.h>
#include <stdlib.h>

#define INVALID 	1
#define TOOSMALL 	2
#define TOOLARGE 	3

long long
strtonum(const char *numstr, long long minval, long long maxval,
	 const char **errstrp);
#endif	/* if defined(__NetBSD__)||defined(__linux__)||defined(__APPLE__) */

#ifdef __linux__
#include <sys/types.h>
#include <sys/cdefs.h>
#include <sys/socket.h>

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FPARSELN_UNESCESC	0x01
#define FPARSELN_UNESCCONT	0x02
#define FPARSELN_UNESCCOMM	0x04
#define FPARSELN_UNESCREST	0x08
#define FPARSELN_UNESCALL	0x0f

size_t	strlcpy(char *, const char *, size_t);
size_t	strlcat(char *, const char *, size_t);

char   *fgetln(FILE *, size_t *);
char   *fparseln(FILE *, size_t *, size_t *, const char [3], int);
#ifndef WAIT_ANY
#define WAIT_ANY		(-1)
#endif	/* ifndef WAIT_ANY */

/* there is no limit to ulrich drepper's crap */
#ifndef TAILQ_END
#define	TAILQ_END(head)			NULL
#endif	/* ifndef TAILQ_END */

#ifndef TAILQ_FOREACH_SAFE
#define TAILQ_FOREACH_SAFE(var, head, field, tvar)                      \
	for ((var) = TAILQ_FIRST(head);                                 \
	    (var) != TAILQ_END(head) &&                                 \
	    ((tvar) = TAILQ_NEXT(var, field), 1);                       \
	    (var) = (tvar))
#endif	/* ifndef TAILQ_FOREACH_SAFE */

int getpeereid(int s, uid_t *euid, gid_t *gid);
#endif	/* ifdef __linux__ */

#endif /* ifndef XT_PLATFORM_H */
