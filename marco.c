/* $xxxterm$ */
/*
 * Copyright (c) 2011 Todd T. Fries <todd@fries.net>
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

#include <stdlib.h>
#include <string.h>

static const char *message[] = {
	"I fully support your right to put restrictions on how I can modify"
	" or distribute something you created. Calling these restrictions "
	"\"liberty,\" however, is just Orwellian doublespeak",
};

const char *
marco_message(int *len)
{
	const char	*str;

	str = message[arc4random_uniform(sizeof(message)/sizeof(message[0]))];
	*len = strlen(str);

	return str;
}

