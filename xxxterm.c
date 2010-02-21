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
 *	config file
 *	inverse color browsing
 *	favs
 *	download files status
 *	multi letter commands
 *	pre and post counts for commands
 *	cookies
 *	proxy
 */

#include <stdio.h>
#include <stdlib.h>
#include <err.h>
#include <pwd.h>
#include <string.h>
#include <unistd.h>
#include <util.h>

#include <sys/queue.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <webkit/webkit.h>
#include <libsoup/soup.h>

static char		*version = "$xxxterm$";

#define XT_DEBUG
/* #define XT_DEBUG */
#ifdef XT_DEBUG
#define DPRINTF(x...)		do { if (swm_debug) fprintf(stderr, x); } while (0)
#define DNPRINTF(n,x...)	do { if (swm_debug & n) fprintf(stderr, x); } while (0)
#define	XT_D_MOVE		0x0001
#define	XT_D_KEY		0x0002
#define	XT_D_TAB		0x0004
#define	XT_D_URL		0x0008
#define	XT_D_CMD		0x0010
#define	XT_D_NAV		0x0020
#define	XT_D_DOWNLOAD		0x0040
#define	XT_D_CONFIG		0x0080
u_int32_t		swm_debug = 0
			    | XT_D_MOVE
			    | XT_D_KEY
			    | XT_D_TAB
			    | XT_D_URL
			    | XT_D_CMD
			    | XT_D_NAV
			    | XT_D_DOWNLOAD
			    | XT_D_CONFIG
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
	GtkWidget		*cmd;
	guint			tab_id;

	/* adjustments for browser */
	GtkScrollbar		*sb_h;
	GtkScrollbar		*sb_v;
	GtkAdjustment		*adjust_h;
	GtkAdjustment		*adjust_v;

	/* flags */
	int			focus_wv;
	int			ctrl_click;
	gchar			*hover;

	WebKitWebView		*wv;
};
TAILQ_HEAD(tab_list, tab);

struct karg {
	int		i;
	char		*s;
};

/* defines */
#define XT_CONF_FILE		("xxxterm.conf")
#define XT_CB_HANDLED		(TRUE)
#define XT_CB_PASSTHROUGH	(FALSE)

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

#define XT_TAB_PREV		(-2)
#define XT_TAB_NEXT		(-1)
#define XT_TAB_INVALID		(0)
#define XT_TAB_NEW		(1)
#define XT_TAB_DELETE		(2)
#define XT_TAB_DELQUIT		(3)
#define XT_TAB_OPEN		(4)

#define XT_NAV_INVALID		(0)
#define XT_NAV_BACK		(1)
#define XT_NAV_FORWARD		(2)

/* globals */
extern char		*__progname;
struct passwd		*pwd;
GtkWidget		*main_window;
GtkNotebook		*notebook;
struct tab_list		tabs;

/* settings */
int			showtabs = 1;	/* show tabs on notebook */
int			showurl = 1;	/* show url toolbar on notebook */
int			tabless = 0;	/* allow only 1 tab */

char			*home = "http://www.peereboom.us";

/* protos */
void			create_new_tab(char *, int);
void			delete_tab(struct tab *);

struct valid_url_types {
	char		*type;
} vut[] = {
	{ "http://" },
	{ "https://" },
	{ "ftp://" },
	{ "file://" },
};

int
valid_url_type(char *url)
{
	int			i;

	for (i = 0; i < LENGTH(vut); i++)
		if (!strncasecmp(vut[i].type, url, strlen(vut[i].type)))
			return (0);

	return (1);
}

char *
guess_url_type(char *url_in)
{
	struct stat		sb;
	char			*url_out = NULL;

	/* XXX not sure about this heuristic */
	if (stat(url_in, &sb) == 0) {
		if (asprintf(&url_out, "file://%s", url_in) == -1)
			err(1, "aprintf file");
	} else {
		/* guess http */
		if (asprintf(&url_out, "http://%s", url_in) == -1)
			err(1, "aprintf http");
	}

	if (url_out == NULL)
		err(1, "asprintf pointer");

	DNPRINTF(XT_D_URL, "guess_url_type: guessed %s\n", url_out);

	return (url_out);
}

#define	WS	"\n= \t"
void
config_parse(char *filename)
{
	FILE			*config;
	char			*line, *cp, *var, *val;
	size_t			 len, lineno = 0;

	DNPRINTF(XT_D_CONFIG, "config_parse: filename %s\n", filename);

	if (filename == NULL)
		return;

	if ((config = fopen(filename, "r")) == NULL) {
		warn("config_parse: cannot open %s", filename);
		return;
	}

	for (;;) {
		if ((line = fparseln(config, &len, &lineno, NULL, 0)) == NULL)
			if (feof(config))
				break;

		cp = line;
		cp += (long)strspn(cp, WS);
		if (cp[0] == '\0') {
			/* empty line */
			free(line);
			continue;
		}

		if ((var = strsep(&cp, WS)) == NULL || cp == NULL)
			break;

		cp += (long)strspn(cp, WS);
		if ((val = strsep(&cp, WS)) == NULL)
			break;

		DNPRINTF(XT_D_CONFIG, "config_parse: %s=%s\n",var ,val);

		/* get settings */
		if (!strcmp(var, "home"))
			home = strdup(val);
		else
			errx(1, "invalid conf file entry: %s=%s", var, val);

		free(line);
	}

	fclose(config);
}
int
quit(struct tab *t, struct karg *args)
{
	gtk_main_quit();

	return (1);
}

int
help(struct tab *t, struct karg *args)
{
	if (t == NULL)
		errx(1, "help");

	webkit_web_view_load_string(t->wv,
	    "<html><body><h1>XXXTerm</h1></body></html>",
	    NULL,
	    NULL,
	    NULL);

	return (0);
}

int
navaction(struct tab *t, struct karg *args)
{
	DNPRINTF(XT_D_NAV, "navaction: tab %d opcode %d\n",
	    t->tab_id, args->i);

	switch (args->i) {
	case XT_NAV_BACK:
		webkit_web_view_go_back(t->wv);
		break;
	case XT_NAV_FORWARD:
		webkit_web_view_go_forward(t->wv);
		break;
	}
	return (XT_CB_PASSTHROUGH);
}

int
move(struct tab *t, struct karg *args)
{
	GtkAdjustment		*adjust;
	double			pi, si, pos, ps, upper, lower, max;

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
	si = gtk_adjustment_get_step_increment(adjust);
	pi = gtk_adjustment_get_page_increment(adjust);
	max = upper - ps;

	DNPRINTF(XT_D_MOVE, "move: opcode %d %s pos %f ps %f upper %f lower %f "
	    "max %f si %f pi %f\n",
	    args->i, adjust == t->adjust_h ? "horizontal" : "vertical", 
	    pos, ps, upper, lower, max, si, pi);

	switch (args->i) {
	case XT_MOVE_DOWN:
	case XT_MOVE_RIGHT:
		pos += si;
		gtk_adjustment_set_value(adjust, MIN(pos, max));
		break;
	case XT_MOVE_UP:
	case XT_MOVE_LEFT:
		pos -= si;
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
		pos += pi;
		gtk_adjustment_set_value(adjust, MIN(pos, max));
		break;
	case XT_MOVE_PAGEUP:
		pos -= pi;
		gtk_adjustment_set_value(adjust, MAX(pos, lower));
		break;
	default:
		return (XT_CB_PASSTHROUGH);
	}

	DNPRINTF(XT_D_MOVE, "move: new pos %f %f\n", pos, MIN(pos, max));

	return (XT_CB_HANDLED);
}

char *
getparams(char *cmd, char *cmp)
{
	char			*rv = NULL;

	if (cmd && cmp) {
		if (!strncmp(cmd, cmp, strlen(cmp))) {
			rv = cmd + strlen(cmp);
			while (*rv == ' ')
				rv++;
			if (strlen(rv) == 0)
				rv = NULL;
		}
	}

	return (rv);
}

int
tabaction(struct tab *t, struct karg *args)
{
	int			rv = XT_CB_HANDLED;
	char			*url = NULL, *newuri = NULL;

	DNPRINTF(XT_D_TAB, "tabaction: %p %d %d\n", t, args->i, t->focus_wv);

	if (t == NULL)
		return (XT_CB_PASSTHROUGH);

	switch (args->i) {
	case XT_TAB_NEW:
		if ((url = getparams(args->s, "tabnew")))
			create_new_tab(url, 1);
		else
			create_new_tab(NULL, 1);
		break;
	case XT_TAB_DELETE:
		delete_tab(t);
		break;
	case XT_TAB_DELQUIT:
		if (gtk_notebook_get_n_pages(notebook) > 1)
			delete_tab(t);
		else
			quit(t, args);
		break;
	case XT_TAB_OPEN:
		if ((url = getparams(args->s, "open")) ||
		    ((url = getparams(args->s, "op"))) ||
		    ((url = getparams(args->s, "o"))))
			;
		else {
			rv = XT_CB_PASSTHROUGH;
			goto done;
		}

		if (valid_url_type(url)) {
			newuri = guess_url_type(url);
			url = newuri;
		}
		webkit_web_view_load_uri(t->wv, url);
		if (newuri)
			free(newuri);
		break;
	default:
		rv = XT_CB_PASSTHROUGH;
		goto done;
	}

done:
	if (args->s) {
		free(args->s);
		args->s = NULL;
	}

	return (rv);
}

int
movetab(struct tab *t, struct karg *args)
{
	struct tab		*tt;
	int			x;

	DNPRINTF(XT_D_TAB, "movetab: %p %d\n", t, args->i);

	if (t == NULL)
		return (XT_CB_PASSTHROUGH);

	if (args->i == XT_TAB_INVALID)
		return (XT_CB_PASSTHROUGH);

	if (args->i < XT_TAB_INVALID) {
		/* next or previous tab */
		if (TAILQ_EMPTY(&tabs))
			return (XT_CB_PASSTHROUGH);

		if (args->i == XT_TAB_NEXT)
			gtk_notebook_next_page(notebook);
		else
			gtk_notebook_prev_page(notebook);

		return (XT_CB_HANDLED);
	}

	/* jump to tab */
	x = args->i - 1;
	if (t->tab_id == x) {
		DNPRINTF(XT_D_TAB, "movetab: do nothing\n");
		return (XT_CB_HANDLED);
	}

	TAILQ_FOREACH(tt, &tabs, entry) {
		if (tt->tab_id == x) {
			gtk_notebook_set_current_page(notebook, x);
			DNPRINTF(XT_D_TAB, "movetab: going to %d\n", x);
			if (tt->focus_wv)
				gtk_widget_grab_focus(GTK_WIDGET(tt->wv));
		}
	}

	return (XT_CB_HANDLED);
}

int
command(struct tab *t, struct karg *args)
{
	DNPRINTF(XT_D_CMD, "command:\n");

	gtk_entry_set_text(GTK_ENTRY(t->cmd), ":");
	gtk_widget_show(t->cmd);
	gtk_widget_grab_focus(GTK_WIDGET(t->cmd));
	gtk_editable_set_position(GTK_EDITABLE(t->cmd), -1);

	return (XT_CB_HANDLED);
}

/* inherent to GTK not all keys will be caught at all times */
struct key {
	guint		mask;
	guint		modkey;
	guint		key;
	int		(*func)(struct tab *, struct karg *);
	struct karg	arg;
} keys[] = {
	{ GDK_SHIFT_MASK,	0,	GDK_colon,	command,	{0} },
	{ GDK_CONTROL_MASK,	0,	GDK_q,		quit,		{0} },

	/* navigation */
	{ 0,			0,	GDK_BackSpace,	navaction,	{.i = XT_NAV_BACK} },
	{ GDK_MOD1_MASK,	0,	GDK_Left,	navaction,	{.i = XT_NAV_BACK} },
	{ GDK_SHIFT_MASK,	0,	GDK_BackSpace,	navaction,	{.i = XT_NAV_FORWARD} },
	{ GDK_MOD1_MASK,	0,	GDK_Right,	navaction,	{.i = XT_NAV_FORWARD} },

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
	{ GDK_CONTROL_MASK,	0,	GDK_1,		movetab,	{.i = 1} },
	{ GDK_CONTROL_MASK,	0,	GDK_2,		movetab,	{.i = 2} },
	{ GDK_CONTROL_MASK,	0,	GDK_3,		movetab,	{.i = 3} },
	{ GDK_CONTROL_MASK,	0,	GDK_4,		movetab,	{.i = 4} },
	{ GDK_CONTROL_MASK,	0,	GDK_5,		movetab,	{.i = 5} },
	{ GDK_CONTROL_MASK,	0,	GDK_6,		movetab,	{.i = 6} },
	{ GDK_CONTROL_MASK,	0,	GDK_7,		movetab,	{.i = 7} },
	{ GDK_CONTROL_MASK,	0,	GDK_8,		movetab,	{.i = 8} },
	{ GDK_CONTROL_MASK,	0,	GDK_9,		movetab,	{.i = 9} },
	{ GDK_CONTROL_MASK,	0,	GDK_0,		movetab,	{.i = 10} },
};

struct cmd {
	char		*cmd;
	int		params;
	int		(*func)(struct tab *, struct karg *);
	struct karg	arg;
} cmds[] = {
	{ "q!",			0,	quit,			{0} },
	{ "qa",			0,	quit,			{0} },
	{ "qa!",		0,	quit,			{0} },
	{ "help",		0,	help,			{0} },

	/* tabs */
	{ "o",			1,	tabaction,		{.i = XT_TAB_OPEN} },
	{ "op",			1,	tabaction,		{.i = XT_TAB_OPEN} },
	{ "open",		1,	tabaction,		{.i = XT_TAB_OPEN} },
	{ "tabnew",		1,	tabaction,		{.i = XT_TAB_NEW} },
	{ "tabedit",		0,	tabaction,		{.i = XT_TAB_NEW} },
	{ "tabe",		0,	tabaction,		{.i = XT_TAB_NEW} },
	{ "tabclose",		0,	tabaction,		{.i = XT_TAB_DELETE} },
	{ "quit",		0,	tabaction,		{.i = XT_TAB_DELQUIT} },
	{ "q",			0,	tabaction,		{.i = XT_TAB_DELQUIT} },
	/* XXX add count to these commands and add tabl and friends */
	{ "tabprevious",	0,	movetab,		{.i = XT_TAB_PREV} },
	{ "tabp",		0,	movetab,		{.i = XT_TAB_PREV} },
	{ "tabnext",		0,	movetab,		{.i = XT_TAB_NEXT} },
	{ "tabn",		0,	movetab,		{.i = XT_TAB_NEXT} },
};

void
focus_uri_entry_cb(GtkWidget* w, GtkDirectionType direction, struct tab *t)
{
	DNPRINTF(XT_D_URL, "focus_uri_entry_cb: tab %d focus_wv %d\n",
	    t->tab_id, t->focus_wv);

	if (t == NULL)
		errx(1, "focus_uri_entry_cb");

	/* focus on wv instead */
	if (t->focus_wv)
		gtk_widget_grab_focus(GTK_WIDGET(t->wv));
}

void
activate_uri_entry_cb(GtkWidget* entry, struct tab *t)
{
	const gchar		*uri = gtk_entry_get_text(GTK_ENTRY(entry));
	char			*newuri = NULL;

	DNPRINTF(XT_D_URL, "activate_uri_entry_cb: %s\n", uri);

	if (t == NULL)
		errx(1, "activate_uri_entry_cb");

	if (uri == NULL)
		errx(1, "uri");

	if (valid_url_type((char *)uri)) {
		newuri = guess_url_type((char *)uri);
		uri = newuri;
	}

	webkit_web_view_load_uri(t->wv, uri);
	gtk_widget_grab_focus(GTK_WIDGET(t->wv));

	if (newuri)
		free(newuri);
}

void
notify_load_status_cb(WebKitWebView* wview, GParamSpec* pspec, struct tab *t)
{
	WebKitWebFrame		*frame;
	const gchar		*uri;

	if (t == NULL)
		errx(1, "notify_load_status_cb");

	switch (webkit_web_view_get_load_status(wview)) {
	case WEBKIT_LOAD_COMMITTED:
		frame = webkit_web_view_get_main_frame(wview);
		uri = webkit_web_frame_get_uri(frame);
		if (uri)
			gtk_entry_set_text(GTK_ENTRY(t->uri_entry), uri);
		t->focus_wv = 1;

		/* take focus if we are visible */
		if (gtk_notebook_get_current_page(notebook) == t->tab_id)
			gtk_widget_grab_focus(GTK_WIDGET(t->wv));
		break;
	case WEBKIT_LOAD_FIRST_VISUALLY_NON_EMPTY_LAYOUT:
		uri = webkit_web_view_get_title(wview);
		if (uri == NULL) {
			frame = webkit_web_view_get_main_frame(wview);
			uri = webkit_web_frame_get_uri(frame);
		}
		gtk_label_set_text(GTK_LABEL(t->label), uri);
		break;
	case WEBKIT_LOAD_PROVISIONAL:
	case WEBKIT_LOAD_FINISHED:
	case WEBKIT_LOAD_FAILED:
	default:
		break;
	}
}

int
webview_npd_cb(WebKitWebView *wv, WebKitWebFrame *wf,
    WebKitNetworkRequest *request, WebKitWebNavigationAction *na,
    WebKitWebPolicyDecision *pd, struct tab *t)
{
	char			*uri;

	if (t == NULL)
		errx(1, "webview_npd_cb");

	DNPRINTF(XT_D_NAV, "webview_npd_cb: %s\n",
	    webkit_network_request_get_uri(request));

	if (t->ctrl_click) {
		uri = (char *)webkit_network_request_get_uri(request);
		create_new_tab(uri, 0);
		t->ctrl_click = 0;
		webkit_web_policy_decision_ignore(pd);

		return (TRUE); /* we made the decission */
	}

	return (FALSE);
}

int
webview_event_cb(GtkWidget *w, GdkEventButton *e, struct tab *t)
{
	/* we can not eat the event without throwing gtk off so defer it */

	/* catch ctrl click */
	if (e->type == GDK_BUTTON_RELEASE && 
	    CLEAN(e->state) == GDK_CONTROL_MASK)
		t->ctrl_click = 1;
	else
		t->ctrl_click = 0;

	return (XT_CB_PASSTHROUGH);
}

int
webview_mimetype_cb(WebKitWebView *wv, WebKitWebFrame *frame,
    WebKitNetworkRequest *request, char *mime_type,
    WebKitWebPolicyDecision *decision, struct tab *t)
{
	if (t == NULL)
		errx(1, "webview_mimetype_cb");

	DNPRINTF(XT_D_DOWNLOAD, "webview_mimetype_cb: tab %d mime %s\n",
	    t->tab_id, mime_type);

	if (webkit_web_view_can_show_mime_type(wv, mime_type) == FALSE) {
		webkit_web_policy_decision_download(decision);
		return (TRUE);
	}

	return (FALSE);
}

int
webview_download_cb(WebKitWebView *wv, WebKitDownload *download, struct tab *t)
{
	const gchar		*filename;
	char			*uri = NULL;

	if (download == NULL || t == NULL)
		errx(1, "webview_download_cb: invalid pointers");

	filename = webkit_download_get_suggested_filename(download);
	if (filename == NULL)
		return (FALSE); /* abort download */

	if (asprintf(&uri, "file://%s/%s", pwd->pw_dir, filename) == -1)
		err(1, "aprintf uri");

	DNPRINTF(XT_D_DOWNLOAD, "webview_download_cb: tab %d filename %s "
	    "local %s\n",
	    t->tab_id, filename, uri);

	webkit_download_set_destination_uri(download, uri);

	if (uri)
		free(uri);

	webkit_download_start(download);

	return (TRUE); /* start download */
}

void
webview_hover_cb(WebKitWebView *wv, gchar *title, gchar *uri, struct tab *t)
{
	DNPRINTF(XT_D_KEY, "webview_hover_cb: %s %s\n", title, uri);

	if (t == NULL)
		errx(1, "webview_hover_cb");

	if (uri) {
		if (t->hover) {
			free(t->hover);
			t->hover = NULL;
		}
		t->hover = strdup(uri);
	} else if (t->hover) {
		free(t->hover);
		t->hover = NULL;
	}
}

int
webview_keypress_cb(GtkWidget *w, GdkEventKey *e, struct tab *t)
{
	int			i;

	/* don't use w directly; use t->whatever instead */

	if (t == NULL)
		errx(1, "webview_keypress_cb");

	DNPRINTF(XT_D_KEY, "webview_keypress_cb: keyval 0x%x mask 0x%x t %p\n",
	    e->keyval, e->state, t);

	for (i = 0; i < LENGTH(keys); i++)
		if (e->keyval == keys[i].key && CLEAN(e->state) ==
		    keys[i].mask) {
			keys[i].func(t, &keys[i].arg);
			return (XT_CB_HANDLED);
		}

	return (XT_CB_PASSTHROUGH);
}

int
cmd_keypress_cb(GtkEntry *w, GdkEventKey *e, struct tab *t)
{
	int			rv = XT_CB_HANDLED;
	const gchar		*c = gtk_entry_get_text(w);

	if (t == NULL)
		errx(1, "cmd_keypress_cb");

	DNPRINTF(XT_D_CMD, "cmd_keypress_cb: keyval 0x%x mask 0x%x t %p\n",
	    e->keyval, e->state, t);

	/* sanity */
	if (c == NULL)
		e->keyval = GDK_Escape;
	else if (c[0] != ':')
		e->keyval = GDK_Escape;

	switch (e->keyval) {
	case GDK_BackSpace:
		if (strcmp(c, ":"))
			break;
		/* FALLTHROUGH */
	case GDK_Escape:
		gtk_widget_hide(t->cmd);
		gtk_widget_grab_focus(GTK_WIDGET(t->wv));
		goto done;
	}

	rv = XT_CB_PASSTHROUGH;
done:
	return (rv);
}

int
cmd_focusout_cb(GtkWidget *w, GdkEventFocus *e, struct tab *t)
{
	if (t == NULL)
		errx(1, "cmd_focusout_cb");

	DNPRINTF(XT_D_CMD, "cmd_focusout_cb: tab %d focus_wv %d\n",
	    t->tab_id, t->focus_wv);

	/* abort command when losing focus */
	gtk_widget_hide(t->cmd);
	if (t->focus_wv)
		gtk_widget_grab_focus(GTK_WIDGET(t->wv));
	else
		gtk_widget_grab_focus(GTK_WIDGET(t->uri_entry));

	return (XT_CB_PASSTHROUGH);
}

void
cmd_activate_cb(GtkEntry *entry, struct tab *t)
{
	int			i;
	char			*s;
	const gchar		*c = gtk_entry_get_text(entry);

	if (t == NULL)
		errx(1, "cmd_activate_cb");

	DNPRINTF(XT_D_CMD, "cmd_activate_cb: tab %d %s\n", t->tab_id, c);

	/* sanity */
	if (c == NULL)
		goto done;
	else if (c[0] != ':')
		goto done;
	if (strlen(c) < 2)
		goto done;
	s = (char *)&c[1];

	for (i = 0; i < LENGTH(cmds); i++)
		if (cmds[i].params) {
			if (!strncmp(s, cmds[i].cmd, strlen(cmds[i].cmd))) {
				cmds[i].arg.s = strdup(s);
				cmds[i].func(t, &cmds[i].arg);
			}
		} else {
			if (!strcmp(s, cmds[i].cmd))
				cmds[i].func(t, &cmds[i].arg);
		}

done:
	gtk_widget_hide(t->cmd);
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
	gtk_window_set_wmclass(GTK_WINDOW(w), "xxxterm", "XXXTerm");

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

	TAILQ_REMOVE(&tabs, t, entry);
	if (TAILQ_EMPTY(&tabs))
		create_new_tab(NULL, 1);

	gtk_widget_destroy(t->vbox);
	g_free(t);
}

void
create_new_tab(char *title, int focus)
{
	struct tab		*t;
	int			load = 1;
	char			*newuri = NULL;

	DNPRINTF(XT_D_TAB, "create_new_tab: title %s focus %d\n", title, focus);

	if (tabless && !TAILQ_EMPTY(&tabs)) {
		DNPRINTF(XT_D_TAB, "create_new_tab: new tab rejected\n");
		return;
	}

	t = g_malloc0(sizeof *t);
	TAILQ_INSERT_TAIL(&tabs, t, entry);

	if (title == NULL) {
		title = "(untitled)";
		load = 0;
	} else {
		if (valid_url_type(title)) {
			newuri = guess_url_type(title);
			title = newuri;
		}
	}

	t->vbox = gtk_vbox_new(FALSE, 0);

	/* label for tab */
	t->label = gtk_label_new(title);
	gtk_widget_set_size_request(t->label, 100, -1);

	/* toolbar */
	t->toolbar = create_toolbar(t);
	gtk_box_pack_start(GTK_BOX(t->vbox), t->toolbar, FALSE, FALSE, 0);

	/* browser */
	t->browser_win = create_browser(t);
	gtk_box_pack_start(GTK_BOX(t->vbox), t->browser_win, TRUE, TRUE, 0);

	/* command entry */
	t->cmd = gtk_entry_new();
	gtk_entry_set_inner_border(GTK_ENTRY(t->cmd), NULL);
	gtk_entry_set_has_frame(GTK_ENTRY(t->cmd), FALSE);
	gtk_box_pack_end(GTK_BOX(t->vbox), t->cmd, FALSE, FALSE, 0);

	/* and show it all */
	gtk_widget_show_all(t->vbox);
	t->tab_id = gtk_notebook_append_page(notebook, t->vbox,
	    t->label);

	g_object_connect((GObject*)t->cmd,
	    "signal::key-press-event", (GCallback)cmd_keypress_cb, t,
	    "signal::focus-out-event", (GCallback)cmd_focusout_cb, t,
	    "signal::activate", (GCallback)cmd_activate_cb, t,
	    NULL);

	g_object_connect((GObject*)t->wv,
	    "signal-after::key-press-event", (GCallback)webview_keypress_cb, t,
	    /* "signal::hovering-over-link", (GCallback)webview_hover_cb, t, */
	    "signal::download-requested", (GCallback)webview_download_cb, t,
	    "signal::mime-type-policy-decision-requested", (GCallback)webview_mimetype_cb, t,
	    "signal::navigation-policy-decision-requested", (GCallback)webview_npd_cb, t,
	    "signal::event", (GCallback)webview_event_cb, t,
	    NULL);

	/* hijack the unused keys as if we were the browser */
	g_object_connect((GObject*)t->toolbar,
	    "signal-after::key-press-event", (GCallback)webview_keypress_cb, t,
	    NULL);

	g_signal_connect(G_OBJECT(t->uri_entry), "focus",
	    G_CALLBACK(focus_uri_entry_cb), t);

	/* hide stuff */
	gtk_widget_hide(t->cmd);
	if (showurl == 0)
		gtk_widget_hide(t->toolbar);

	if (focus) {
		gtk_notebook_set_current_page(notebook, t->tab_id);
		DNPRINTF(XT_D_TAB, "create_new_tab: going to tab: %d\n",
		    t->tab_id);
	}

	if (load)
		webkit_web_view_load_uri(t->wv, title);
	else
		gtk_widget_grab_focus(GTK_WIDGET(t->uri_entry));

	if (newuri)
		free(newuri);
}

void
notebook_switchpage_cb(GtkNotebook *nb, GtkNotebookPage *nbp, guint pn,
    gpointer *udata)
{
	struct tab		*t;

	DNPRINTF(XT_D_TAB, "notebook_switchpage_cb: tab: %d\n", pn);

	TAILQ_FOREACH(t, &tabs, entry) {
		if (t->tab_id == pn) {
			DNPRINTF(XT_D_TAB, "notebook_switchpage_cb: going to "
			    "%d\n", pn);
			gtk_widget_hide(t->cmd);
		}
	}
}

void
create_canvas(void)
{
	GtkWidget		*vbox;
	
	vbox = gtk_vbox_new(FALSE, 0);
	notebook = GTK_NOTEBOOK(gtk_notebook_new());
	if (showtabs == 0)
		gtk_notebook_set_show_tabs(GTK_NOTEBOOK(notebook), FALSE);

	gtk_box_pack_start(GTK_BOX(vbox), GTK_WIDGET(notebook), TRUE, TRUE, 0);

	g_object_connect((GObject*)notebook,
	    "signal::switch-page", (GCallback)notebook_switchpage_cb, NULL,
	    NULL);

	main_window = create_window();
	gtk_container_add(GTK_CONTAINER(main_window), vbox);
	gtk_widget_show_all(main_window);
}

void
usage(void)
{
	fprintf(stderr,
	    "%s [-STVt][-f file] url ...\n", __progname);
	exit(0);
}

int
main(int argc, char *argv[])
{
	char			conf[PATH_MAX] = { '\0' };
	int			c, focus = 1;

	while ((c = getopt(argc, argv, "STVf:t")) != -1) {
		switch (c) {
		case 'S':
			showurl = 0;
			break;
		case 'T':
			showtabs = 0;
			break;
		case 'V':
			errx(0 , "Version: %s", version);
			break;
		case 'f':
			strlcpy(conf, optarg, sizeof(conf));
			break;
		case 't':
			tabless = 1;
			break;
		default:
			usage();
			/* NOTREACHED */
		}
	}
	argc -= optind;
	argv += optind;

	TAILQ_INIT(&tabs);

	/* prepare gtk */
	gtk_init(&argc, &argv);
	if (!g_thread_supported())
		g_thread_init(NULL);

	pwd = getpwuid(getuid());
	if (pwd == NULL)
		errx(1, "invalid user %d", getuid());

	/* read config file */
	if (strlen(conf) == 0)
		snprintf(conf, sizeof conf, "%s/.%s", pwd->pw_dir,
		    XT_CONF_FILE);
	config_parse(conf);

	create_canvas();

	while (argc) {
		create_new_tab(argv[0], focus);
		focus = 0;

		argc--;
		argv++;
	}
	if (focus == 1)
		create_new_tab(home, 1);

	gtk_main();

	return (0);
}
