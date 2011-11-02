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

#include "xxxterm.h"

static const char *message[] = {
	"I fully support your right to put restrictions on how I can modify"
	" or distribute something you created. Calling these restrictions "
	"\"liberty,\" however, is just Orwellian doublespeak",
	"GET OFF MY INTERNET LAWN!!!!",
	"here is your list of restrictions -> freedom!!!!",
	"i am all for it being free; i am all for not allowing abuse",
	"webkit 1.4 is a huge step backwards",
	"i honestly dont get how people equate gpl with liberty or freedom",
	"I have no idea where you came up with the idea that you have to "
	"continuously patch OpenBSD. Last time I cared about that was when "
	"I was still dicking around with Linux.",
	"Problem with discussion is that it doesn't add any code to cvs.",
	"I can insult you some more but I am not sure you'd get it.",
	"the mouse is for copy/paste and the interwebs",
	"You could run openbsd and be done with it. Unlike linux is doesn't "
	"suck so that helps that decision.",
	"Twitter is the dumbest fucking thing that has ever happened to the Internet.",
	"[speaking of {open,libre}office] it is an obese crack whore with herpes boils "
	"that are about to explode"
};

const char *
marco_message(int *len)
{
	const char	*str;

	str = message[arc4random_uniform(sizeof(message)/sizeof(message[0]))];
	*len = strlen(str);

	return str;
}

int
marco(struct tab *t, struct karg *args)
{
	char			*page, line[64 * 1024];
	int			len;

	if (t == NULL)
		show_oops(NULL, "marco invalid parameters");

	line[0] = '\0';
	snprintf(line, sizeof line, "%s", marco_message(&len));

	page = get_html_page("Marco Sez...", line, "", 0);

	load_webkit_string(t, page, XT_URI_ABOUT_MARCO);
	g_free(page);

	return (0);
}
