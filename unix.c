/*
 * Copyright (c) 2010, 2011 Marco Peereboom <marco@peereboom.us>
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

#include <xombrero.h>

int
fork_exec(struct tab *t, char *argv0, const gchar *argv1, char *error, int loud)
{

	if (loud)
		show_oops(t, "running: %s %s", argv0, argv1);

	switch (fork()) {
	case -1:
		show_oops(t, error);
		return (1);
		/* NOTREACHED */
	case 0:
		break;
	default:
		return (0);
	}

	/* child */
	execlp(argv0, argv0, argv1, (void *)NULL);

	/* NOTREACHED */
	_exit(0);

	return (0);
}
