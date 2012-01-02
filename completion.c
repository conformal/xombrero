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

gboolean
completion_select_cb(GtkEntryCompletion *widget, GtkTreeModel *model,
    GtkTreeIter *iter, struct tab *t)
{
	gchar			*value;

	gtk_tree_model_get(model, iter, 0, &value, -1);
	load_uri(t, value);
	g_free(value);

	return (FALSE);
}

gboolean
completion_hover_cb(GtkEntryCompletion *widget, GtkTreeModel *model,
    GtkTreeIter *iter, struct tab *t)
{
	gchar			*value;

	gtk_tree_model_get(model, iter, 0, &value, -1);
	gtk_entry_set_text(GTK_ENTRY(t->uri_entry), value);
	gtk_editable_set_position(GTK_EDITABLE(t->uri_entry), -1);
	g_free(value);

	return (TRUE);
}

void
completion_add_uri(const gchar *uri)
{
	GtkTreeIter		iter;

	/* add uri to list_store */
	gtk_list_store_append(completion_model, &iter);
	gtk_list_store_set(completion_model, &iter, 0, uri, -1);
}

gboolean
completion_match(GtkEntryCompletion *completion, const gchar *key,
    GtkTreeIter *iter, gpointer user_data)
{
	gchar			*value;
	gboolean		match = FALSE;

	gtk_tree_model_get(GTK_TREE_MODEL(completion_model), iter, 0, &value,
	    -1);

	if (value == NULL)
		return FALSE;

	match = match_uri(value, key);

	g_free(value);
	return (match);
}

void
completion_add(struct tab *t)
{
	/* enable completion for tab */
	t->completion = gtk_entry_completion_new();
	gtk_entry_completion_set_text_column(t->completion, 0);
	gtk_entry_set_completion(GTK_ENTRY(t->uri_entry), t->completion);
	gtk_entry_completion_set_model(t->completion,
	    GTK_TREE_MODEL(completion_model));
	gtk_entry_completion_set_match_func(t->completion, completion_match,
	    NULL, NULL);
	gtk_entry_completion_set_minimum_key_length(t->completion, 1);
	gtk_entry_completion_set_inline_selection(t->completion, TRUE);
	g_signal_connect(G_OBJECT (t->completion), "match-selected",
	    G_CALLBACK(completion_select_cb), t);
	g_signal_connect(G_OBJECT (t->completion), "cursor-on-match",
	    G_CALLBACK(completion_hover_cb), t);
}

