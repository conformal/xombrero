/*
 * Copyright (c) 2010, 2011 Marco Peereboom <marco@peereboom.us>
 * Copyright (c) 2011 Stevan Andjelkovic <stevan@student.chalmers.se>
 * Copyright (c) 2010, 2011 Edd Barrett <vext01@gmail.com>
 * Copyright (c) 2011 Todd T. Fries <todd@fries.net>
 * Copyright (c) 2011 Raphael Graf <r@undefined.ch>
 * Copyright (c) 2011 Michal Mazurek <akfaew@jasminek.net>
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

#define XT_HISTORY_FILE		("history")
#define XT_MAX_HL_PURGE_COUNT	(1000) /* Purge the history for every
					* MAX_HL_PURGE_COUNT items inserted into
					* history and delete all items older
					* than MAX_HISTORY_AGE. */
#define XT_MAX_HISTORY_AGE	(60.0 * 60.0 * 24 * 14) /* 14 days */

int
purge_history(void)
{
	struct history		*h, *next;
	double			age = 0.0;

	DNPRINTF(XT_D_HISTORY, "%s: hl_purge_count = %d (%d is max)\n",
	    __func__, hl_purge_count, XT_MAX_HL_PURGE_COUNT);

	if (hl_purge_count == XT_MAX_HL_PURGE_COUNT) {
		hl_purge_count = 0;

		for (h = RB_MIN(history_list, &hl); h != NULL; h = next) {

			next = RB_NEXT(history_list, &hl, h);

			age = difftime(time(NULL), h->time);

			if (age > XT_MAX_HISTORY_AGE) {
				DNPRINTF(XT_D_HISTORY, "%s: removing %s (age %.1f)\n",
				    __func__, h->uri, age);

				RB_REMOVE(history_list, &hl, h);
				g_free(h->uri);
				g_free(h->title);
				g_free(h);
			} else {
				DNPRINTF(XT_D_HISTORY, "%s: keeping %s (age %.1f)\n",
				    __func__, h->uri, age);
			}
		}
	}

	return (0);
}

int
insert_history_item(const gchar *uri, const gchar *title, time_t time)
{
	struct history		*h;

	if (!(uri && strlen(uri) && title && strlen(title)))
		return (1);

	h = g_malloc(sizeof(struct history));
	h->uri = g_strdup(uri);
	h->title = g_strdup(title);
	h->time = time;

	DNPRINTF(XT_D_HISTORY, "%s: adding %s\n", __func__, h->uri);

	RB_INSERT(history_list, &hl, h);
	completion_add_uri(h->uri);
	hl_purge_count++;

	purge_history();
	update_history_tabs(NULL);

	return (0);
}

int
restore_global_history(void)
{
	char			file[PATH_MAX];
	FILE			*f;
	gchar			*uri, *title, *stime, *err = NULL;
	time_t			time;
	struct tm		tm;
	const char		delim[3] = {'\\', '\\', '\0'};

	snprintf(file, sizeof file, "%s/%s", work_dir, XT_HISTORY_FILE);

	if ((f = fopen(file, "r")) == NULL) {
		warnx("%s: fopen", __func__);
		return (1);
	}

	for (;;) {
		if ((uri = fparseln(f, NULL, NULL, delim, 0)) == NULL)
			if (feof(f) || ferror(f))
				break;

		if ((title = fparseln(f, NULL, NULL, delim, 0)) == NULL)
			if (feof(f) || ferror(f)) {
				err = "broken history file (title)";
				goto done;
			}

		if ((stime = fparseln(f, NULL, NULL, delim, 0)) == NULL)
			if (feof(f) || ferror(f)) {
				err = "broken history file (time)";
				goto done;
			}

		if (strptime(stime, "%a %b %d %H:%M:%S %Y", &tm) == NULL) {
			err = "strptime failed to parse time";
			goto done;
		}

		time = mktime(&tm);

		if (insert_history_item(uri, title, time)) {
			err = "failed to insert item";
			goto done;
		}

		free(uri);
		free(title);
		free(stime);
		uri = NULL;
		title = NULL;
		stime = NULL;
	}

done:
	if (err && strlen(err)) {
		warnx("%s: %s\n", __func__, err);
		free(uri);
		free(title);
		free(stime);

		return (1);
	}

	return (0);
}

int
save_global_history_to_disk(struct tab *t)
{
	char			file[PATH_MAX];
	FILE			*f;
	struct history		*h;

	snprintf(file, sizeof file, "%s/%s", work_dir, XT_HISTORY_FILE);

	if ((f = fopen(file, "w")) == NULL) {
		show_oops(t, "%s: global history file: %s",
		    __func__, strerror(errno));
		return (1);
	}

	RB_FOREACH_REVERSE(h, history_list, &hl) {
		if (h->uri && h->title && h->time)
			fprintf(f, "%s\n%s\n%s", h->uri, h->title,
			    ctime(&h->time));
	}

	fclose(f);

	return (0);
}

/* Marshall the internal record of visited URIs into a Javascript hash table in
 * string form. */
char *
color_visited_helper(void)
{
	char			*s = NULL;
	struct history		*h;

	RB_FOREACH_REVERSE(h, history_list, &hl) {
		if (s == NULL)
			s = g_strdup_printf("'%s':'dummy'", h->uri);
		else
			s = g_strjoin(",", s,
			    g_strdup_printf("'%s':'dummy'", h->uri), NULL);
	}

	s = g_strdup_printf("{%s}", s);

	DNPRINTF(XT_D_VISITED, "%s: s = %s\n", __func__, s);

	return (s);
}

int
color_visited(struct tab *t, char *visited)
{
	char		*s;

	if (t == NULL || visited == NULL) {
		show_oops(NULL, "%s: invalid parameters", __func__);
		return (1);
	}

	/* Create a string representing an annonymous Javascript function, which
	 * takes a hash table of visited URIs as an argument, goes through the
	 * links at the current web page and colors them if they indeed been
	 * visited.
	 */
	s = g_strconcat(
	    "(function(visitedUris) {",
	    "    for (var i = 0; i < document.links.length; i++)",
	    "        if (visitedUris[document.links[i].href])",
	    "            document.links[i].style.color = 'purple';",
	    "})",
	    /* Apply the annonymous function to the hash table containing
	     * visited URIs. */
	    g_strdup_printf("(%s);", visited),
	    NULL);

	run_script(t, s);
	g_free(s);
	g_free(visited);

	return (0);
}
