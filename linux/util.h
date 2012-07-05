/*
 * Copyright (c) 2010 Marco Peereboom <marco@peereboom.us>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#define FPARSELN_UNESCESC	0x01
#define FPARSELN_UNESCCONT	0x02
#define FPARSELN_UNESCCOMM	0x04
#define FPARSELN_UNESCREST	0x08
#define FPARSELN_UNESCALL	0x0f

size_t	strlcpy(char *, const char *, size_t);
size_t	strlcat(char *, const char *, size_t);

char   *fgetln(FILE *, size_t *);
char   *fparseln(FILE *, size_t *, size_t *, const char [3], int);

long long strtonum(const char *, long long, long long, const char **);

int	fmt_scaled(long long number, char *result);

#ifndef WAIT_ANY
#define WAIT_ANY		(-1)
#endif

/* there is no limit to ulrich drepper's crap */
#ifndef TAILQ_END
#define	TAILQ_END(head)			NULL
#endif

#ifndef TAILQ_FOREACH_SAFE
#define TAILQ_FOREACH_SAFE(var, head, field, tvar)                      \
	for ((var) = TAILQ_FIRST(head);                                 \
	    (var) != TAILQ_END(head) &&                                 \
	    ((tvar) = TAILQ_NEXT(var, field), 1);                       \
	    (var) = (tvar))
#endif

/*
 * fmt_scaled(3) specific flags. (from OpenBSD util.h)
 */
#define FMT_SCALED_STRSIZE	7	/* minus sign, 4 digits, suffix, null byte */

int getpeereid(int s, uid_t *euid, gid_t *gid);

