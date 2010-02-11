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
 *	download files
 */

#include <stdio.h>
#include <stdlib.h>
#include <err.h>

#include <sys/queue.h>

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <webkit/webkit.h>
#include <libsoup/soup.h>

#define XT_DEBUG
/* #define XT_DEBUG */
#ifdef XT_DEBUG
#define DPRINTF(x...)		do { if (swm_debug) fprintf(stderr, x); } while (0)
#define DNPRINTF(n,x...)	do { if (swm_debug & n) fprintf(stderr, x); } while (0)
#define	XT_D_MOVE		0x0001
#define	XT_D_KEY		0x0002
#define	XT_D_TAB		0x0004
u_int32_t		swm_debug = 0
			    | XT_D_MOVE
			    | XT_D_KEY
			    | XT_D_TAB
			    ;
#else
#define DPRINTF(x...)
#define DNPRINTF(n,x...)
#endif

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

	/* adjustments for browser */
	GtkScrollbar		*sb_h;
	GtkScrollbar		*sb_v;
	GtkAdjustment		*adjust_h;
	GtkAdjustment		*adjust_v;

	WebKitWebView		*wv;
};
TAILQ_HEAD(tab_list, tab);

struct karg {
	int		i;
	char		*s;
};

/* actions */
#define XT_MOVE_INVALID		(0)
#define XT_MOVE_DOWN		(1)
#define XT_MOVE_UP		(2)
#define XT_MOVE_BOTTOM		(3)
#define XT_MOVE_TOP		(4)
#define XT_MOVE_PAGEDOWN	(5)
#define XT_MOVE_PAGEUP		(6)
#define XT_MOVE_LEFT		(7)
#define XT_MOVE_FARLEFT		(8)
#define XT_MOVE_RIGHT		(9)
#define XT_MOVE_FARRIGHT	(10)

#define XT_TAB_INVALID		(0)
#define XT_TAB_NEW		(1)
#define XT_TAB_DELETE		(2)

/* globals */
GtkWidget		*main_window;
GtkWidget		*notebook;
struct tab_list		tabs;

/* protos */
void			create_new_tab(char *, int);
void			delete_tab(struct tab *);

int
quit(struct tab *t, struct karg *args)
{
	gtk_main_quit();

	return (1);
}

int
move(struct tab *t, struct karg *args)
{
	GtkAdjustment		*adjust;
	double			pos;
	double			ps;
	double			upper;
	double			lower;
	double			max;

	switch (args->i) {
	case XT_MOVE_DOWN:
	case XT_MOVE_UP:
	case XT_MOVE_BOTTOM:
	case XT_MOVE_TOP:
	case XT_MOVE_PAGEDOWN:
	case XT_MOVE_PAGEUP:
		adjust = t->adjust_v;
		break;
	default:
		adjust = t->adjust_h;
		break;
	}

	pos = gtk_adjustment_get_value(adjust);
	ps = gtk_adjustment_get_page_size(adjust);
	upper = gtk_adjustment_get_upper(adjust);
	lower = gtk_adjustment_get_lower(adjust);
	max = upper - ps;

	DNPRINTF(XT_D_MOVE, "move opcode %d %s pos %f ps %f upper %f lower %f max %f\n",
	    args->i, adjust == t->adjust_h ? "horizontal" : "vertical", 
	    pos, ps, upper, lower, max);

	switch (args->i) {
	case XT_MOVE_DOWN:
	case XT_MOVE_RIGHT:
		pos += 24;
		gtk_adjustment_set_value(adjust, MIN(pos, max));
		break;
	case XT_MOVE_UP:
	case XT_MOVE_LEFT:
		pos -= 24;
		gtk_adjustment_set_value(adjust, MAX(pos, lower));
		break;
	case XT_MOVE_BOTTOM:
	case XT_MOVE_FARRIGHT:
		gtk_adjustment_set_value(adjust, max);
		break;
	case XT_MOVE_TOP:
	case XT_MOVE_FARLEFT:
		gtk_adjustment_set_value(adjust, lower);
		break;
	case XT_MOVE_PAGEDOWN:
		pos += ps;
		gtk_adjustment_set_value(adjust, MIN(pos, max));
		break;
	case XT_MOVE_PAGEUP:
		pos -= ps;
		gtk_adjustment_set_value(adjust, MAX(pos, lower));
		break;
	default:
		return (0); /* let webkit deal with it */
	}

	DNPRINTF(XT_D_MOVE, "new pos: %f  %f\n", pos, MIN(pos, max));

	return (1); /* handled */
}

int
tabaction(struct tab *t, struct karg *args)
{
	DNPRINTF(XT_D_TAB, "tabaction: %p %d\n", t, args->i);

	if (t == NULL)
		return (0);

	switch (args->i) {
	case XT_TAB_NEW:
		create_new_tab(NULL, 1);
		break;
	case XT_TAB_DELETE:
		delete_tab(t);
		break;
	default:
		return (0); /* let webkit deal with it */
	}

	return (1); /* handled */
}

int
command(struct tab *t, struct karg *args)
{
	return (0);
}

struct key {
	guint		mask;
	guint		modkey;
	guint		key;
	int		(*func)(struct tab *, struct karg *);
	struct karg	arg;
} keys[] = {
	{ GDK_SHIFT_MASK,	0,	GDK_colon,	command,	{0} },
	{ GDK_CONTROL_MASK,	0,	GDK_q,		quit,		{0} },

	/* vertical movement */
	{ 0,			0,	GDK_j,		move,		{.i = XT_MOVE_DOWN} },
	{ 0,			0,	GDK_Down,	move,		{.i = XT_MOVE_DOWN} },
	{ 0,			0,	GDK_Up,		move,		{.i = XT_MOVE_UP} },
	{ 0,			0,	GDK_k,		move,		{.i = XT_MOVE_UP} },
	{ GDK_SHIFT_MASK,	0,	GDK_G,		move,		{.i = XT_MOVE_BOTTOM} },
	{ 0,			0,	GDK_End,	move,		{.i = XT_MOVE_BOTTOM} },
	{ 0,			0,	GDK_Home,	move,		{.i = XT_MOVE_TOP} },
	{ 0,			GDK_g,	GDK_g,		move,		{.i = XT_MOVE_TOP} }, /* XXX make this work */
	{ 0,			0,	GDK_space,	move,		{.i = XT_MOVE_PAGEDOWN} },
	{ 0,			0,	GDK_Page_Down,	move,		{.i = XT_MOVE_PAGEDOWN} },
	{ 0,			0,	GDK_Page_Up,	move,		{.i = XT_MOVE_PAGEUP} },
	/* horizontal movement */
	{ 0,			0,	GDK_l,		move,		{.i = XT_MOVE_RIGHT} },
	{ 0,			0,	GDK_Right,	move,		{.i = XT_MOVE_RIGHT} },
	{ 0,			0,	GDK_Left,	move,		{.i = XT_MOVE_LEFT} },
	{ 0,			0,	GDK_h,		move,		{.i = XT_MOVE_LEFT} },
	{ GDK_SHIFT_MASK,	0,	GDK_dollar,	move,		{.i = XT_MOVE_FARRIGHT} },
	{ 0,			0,	GDK_0,		move,		{.i = XT_MOVE_FARLEFT} },

	/* tabs */
	{ GDK_CONTROL_MASK,	0,	GDK_t,		tabaction,	{.i = XT_TAB_NEW} },
	{ GDK_CONTROL_MASK,	0,	GDK_w,		tabaction,	{.i = XT_TAB_DELETE} },
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

	DNPRINTF(XT_D_KEY, "keyval: 0x%x mask: 0x%x t %p\n",
	    e->keyval, e->state, t);

	for (i = 0; i < LENGTH(keys); i++)
		if (e->keyval == keys[i].key && CLEAN(e->state) == keys[i].mask)
			keys[i].func(t, &keys[i].arg);
}

GtkWidget *
create_browser(struct tab *t)
{
	GtkWidget		*w;

	if (t == NULL)
		errx(1, "create_browser");

	t->sb_h = GTK_SCROLLBAR(gtk_hscrollbar_new(NULL));
	t->sb_v = GTK_SCROLLBAR(gtk_vscrollbar_new(NULL));
	t->adjust_h = gtk_range_get_adjustment(GTK_RANGE(t->sb_h));
	t->adjust_v = gtk_range_get_adjustment(GTK_RANGE(t->sb_v));

	w = gtk_scrolled_window_new(t->adjust_h, t->adjust_v);
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
	gtk_window_set_wmclass(GTK_WINDOW(w), "xxxterm", "xxxterm");

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
delete_tab(struct tab *t)
{
	DNPRINTF(XT_D_TAB, "delete_tab: %p\n", t);

	if (t == NULL)
		return;

	gtk_widget_destroy(t->vbox);
	g_free(t);
}

void
create_new_tab(char *title, int focus)
{
	struct tab		*t;
	int			last, load = 1;

	DNPRINTF(XT_D_TAB, "create_new_tab: title %s focus %d\n", title, focus);

	t = g_malloc0(sizeof *t);
	TAILQ_INSERT_TAIL(&tabs, t, entry);

	if (title == NULL) {
		title = "(untitled)";
		load = 0;
	}
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

	if (focus) {
		gtk_widget_show_all(main_window);
		last = gtk_notebook_get_n_pages(GTK_NOTEBOOK(notebook)) - 1;
		gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook), last);
		DNPRINTF(XT_D_TAB, "going to tab: %d\n", last);
		gtk_widget_grab_focus(GTK_WIDGET(t->wv));
	}

	if (load)
		webkit_web_view_load_uri(t->wv, title);
	else
		gtk_widget_grab_focus(GTK_WIDGET(t->uri_entry));
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

	create_new_tab("http://www.peereboom.us", 1);
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
