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
 *	favs
 */

#include <stdio.h>
#include <stdlib.h>
#include <err.h>

#include <sys/queue.h>

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <webkit/webkit.h>
#include <libsoup/soup.h>

#define LENGTH(x)		(sizeof x / sizeof x[0])
#define CLEAN(mask)		(mask & ~(GDK_MOD2_MASK) &	\
				    ~(GDK_BUTTON1_MASK) &	\
				    ~(GDK_BUTTON2_MASK) &	\
				    ~(GDK_BUTTON3_MASK) &	\
				    ~(GDK_BUTTON4_MASK) &	\
				    ~(GDK_BUTTON5_MASK))

struct tab {
	TAILQ_ENTRY(tab)	entry;
	GtkWidget		*vbox;
	GtkWidget		*label;
	GtkWidget		*uri_entry;
	GtkWidget		*toolbar;
	GtkWidget		*browser_win;
	WebKitWebView		*wv;
};
TAILQ_HEAD(tab_list, tab);

struct karg {
	int		i;
	char		*s;
};

/* globals */
GtkWidget		*main_window;
GtkWidget		*notebook;
struct tab_list		tabs;

int
quit(struct karg *args)
{
	gtk_main_quit();

	return (0);
}

int
command(struct karg *args)
{
	fprintf(stderr, "command\n");

	return (0);
}

struct key {
	guint		mask;
	guint		modkey;
	guint		key;
	int		(*func)(struct karg *);
	struct karg	arg;
} keys[] = {
	{ GDK_SHIFT_MASK,	0,	GDK_colon,	command,	{0} },
	{ GDK_CONTROL_MASK,	0,	GDK_q,		quit,		{0} },
};

void
activate_uri_entry_cb(GtkWidget* entry, gpointer data)
{
	const gchar		*uri = gtk_entry_get_text(GTK_ENTRY(entry));
	struct tab		*t;

	if (data == NULL)
		errx(1, "activate_uri_entry_cb");
	t = (struct tab *)data;

	g_assert(uri);
	webkit_web_view_load_uri(t->wv, uri);
}

void
notify_load_status_cb(WebKitWebView* wview, GParamSpec* pspec, gpointer data)
{
	WebKitWebFrame		*frame;
	const gchar		*uri;
	struct tab		*t;

	if (data == NULL)
		errx(1, "notify_load_status_cb");
	t = (struct tab *)data;

	if (webkit_web_view_get_load_status(wview) == WEBKIT_LOAD_COMMITTED) {
		frame = webkit_web_view_get_main_frame(wview);
		uri = webkit_web_frame_get_uri(frame);
		if (uri)
			gtk_entry_set_text(GTK_ENTRY(t->uri_entry), uri);
	}
}

void
webview_keypress_cb(WebKitWebView *webview, GdkEventKey *e, gpointer data)
{
	int			i;
	struct tab		*t;

	if (data == NULL)
		errx(1, "webview_keypress_cb");
	t = (struct tab *)data;

	fprintf(stderr, "keyval: 0x%x mask: 0x%x t %p\n", e->keyval, e->state, t);

	for (i = 0; i < LENGTH(keys); i++)
		if (e->keyval == keys[i].key && CLEAN(e->state) == keys[i].mask)
			keys[i].func(&keys[i].arg);
}

GtkWidget *
create_browser(struct tab *t)
{
	GtkWidget		*w;

	if (t == NULL)
		errx(1, "create_browser");

	w = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(w),
	    GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

	t->wv = WEBKIT_WEB_VIEW(webkit_web_view_new());
	gtk_container_add(GTK_CONTAINER(w), GTK_WIDGET(t->wv));

	g_signal_connect(t->wv, "notify::load-status",
	    G_CALLBACK(notify_load_status_cb), t);

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
create_toolbar(struct tab *t)
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
	t->uri_entry = gtk_entry_new();
	gtk_container_add(GTK_CONTAINER(i), t->uri_entry);
	g_signal_connect(G_OBJECT(t->uri_entry), "activate",
	    G_CALLBACK(activate_uri_entry_cb), t);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), i, -1);

	return (toolbar);
}

void
create_new_tab(char *title)
{
	struct tab		*t;

	t = g_malloc0(sizeof *t);
	TAILQ_INSERT_TAIL(&tabs, t, entry);

	t->label = gtk_label_new(title);
	t->vbox = gtk_vbox_new(FALSE, 0);
	t->toolbar = create_toolbar(t);
	t->browser_win = create_browser(t);
	gtk_box_pack_start(GTK_BOX(t->vbox), t->toolbar, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(t->vbox), t->browser_win, TRUE, TRUE, 0);
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), t->vbox, t->label);

	g_object_connect((GObject*)t->wv,
	    "signal::key-press-event", (GCallback)webview_keypress_cb, t,
	    NULL);

	gtk_widget_grab_focus(GTK_WIDGET(t->wv));
	webkit_web_view_load_uri(t->wv, title);
}

void
create_canvas(void)
{
	GtkWidget		*vbox;
	
	vbox = gtk_vbox_new(FALSE, 0);
	notebook = gtk_notebook_new();

	gtk_box_pack_start(GTK_BOX(vbox), notebook, TRUE, TRUE, 0);

	main_window = create_window();
	gtk_container_add(GTK_CONTAINER(main_window), vbox);

	create_new_tab("http://www.dell.com");
	create_new_tab("http://www.peereboom.us");

	gtk_widget_show_all(main_window);
}

int
main(int argc, char *argv[])
{
	TAILQ_INIT(&tabs);

	/* prepare gtk */
	gtk_init(&argc, &argv);
	if (!g_thread_supported())
		g_thread_init(NULL);

	create_canvas();

	gtk_main();

	return (0);
}
