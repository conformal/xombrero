/* $xxxterm$ */
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

/*
 * TODO:
 *	inverse color browsing
 *	tabs, alt-1..n to switch
 */

#include <stdio.h>
#include <stdlib.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <webkit/webkit.h>
#include <libsoup/soup.h>

GtkWidget		*mw;
GtkWidget		*uri_entry;
WebKitWebView		*wv;

void
activate_uri_entry_cb(GtkWidget* entry, gpointer data)
{
	const gchar			*uri = gtk_entry_get_text(GTK_ENTRY(entry));

	g_assert (uri);
	webkit_web_view_load_uri(wv, uri);
}

void
notify_load_status_cb(WebKitWebView* wview, GParamSpec* pspec, gpointer data)
{
	WebKitWebFrame		*frame;
	gchar			*uri;

	if (webkit_web_view_get_load_status(wview) == WEBKIT_LOAD_COMMITTED) {
		frame = webkit_web_view_get_main_frame(wview);
		uri = webkit_web_frame_get_uri(frame);
		if (uri)
			gtk_entry_set_text(GTK_ENTRY(uri_entry), uri);
	}
}

GtkWidget *
create_browser(void)
{
	GtkWidget		*w;
	
	w = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(w),
	    GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

	wv = WEBKIT_WEB_VIEW(webkit_web_view_new());
	gtk_container_add(GTK_CONTAINER(w), GTK_WIDGET(wv));

	g_signal_connect(wv, "notify::load-status",
	    G_CALLBACK(notify_load_status_cb), wv);

	return (w);
}

GtkWidget *
create_window(void)
{
	GtkWidget		*w;

	w = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_default_size(GTK_WINDOW(w), 800, 600);
	gtk_widget_set_name(w, "xxxterm");

	return (w);
}

GtkWidget *
create_toolbar(void)
{
	GtkWidget		*toolbar = gtk_toolbar_new();
	GtkToolItem		*i;

#if GTK_CHECK_VERSION(2,15,0)
	gtk_orientable_set_orientation(GTK_ORIENTABLE(toolbar),
	    GTK_ORIENTATION_HORIZONTAL);
#else
	gtk_toolbar_set_orientation(GTK_TOOLBAR(toolbar),
	    GTK_ORIENTATION_HORIZONTAL);
#endif
	gtk_toolbar_set_style(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_BOTH_HORIZ);

	i = gtk_tool_item_new();
	gtk_tool_item_set_expand(i, TRUE);
	uri_entry = gtk_entry_new();
	gtk_container_add(GTK_CONTAINER(i), uri_entry);
	g_signal_connect(G_OBJECT(uri_entry), "activate",
	    G_CALLBACK(activate_uri_entry_cb), NULL);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), i, -1);

	return (toolbar);
}

void
gui(void)
{
	GtkWidget		*vbox;
	
	vbox = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), create_toolbar(), FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), create_browser(), TRUE, TRUE, 0);

	mw = create_window();
	gtk_container_add(GTK_CONTAINER(mw), vbox);

	webkit_web_view_load_uri(wv, "http://www.peereboom.us/");

	gtk_widget_grab_focus(GTK_WIDGET(wv));
	gtk_widget_show_all(mw);
}

int
main(int argc, char *argv[])
{
	/* prepare gtk */
	gtk_init(&argc, &argv);
	if (!g_thread_supported())
		g_thread_init(NULL);

	gui();

	gtk_main();

	return (0);
}
