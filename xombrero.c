/*
 * Copyright (c) 2010, 2011 Marco Peereboom <marco@peereboom.us>
 * Copyright (c) 2011 Stevan Andjelkovic <stevan@student.chalmers.se>
 * Copyright (c) 2010, 2011, 2012 Edd Barrett <vext01@gmail.com>
 * Copyright (c) 2011 Todd T. Fries <todd@fries.net>
 * Copyright (c) 2011 Raphael Graf <r@undefined.ch>
 * Copyright (c) 2011 Michal Mazurek <akfaew@jasminek.net>
 * Copyright (c) 2012 Josh Rickmar <jrick@devio.us>
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
#include "version.h"

char		*version = XOMBRERO_VERSION;

#ifdef XT_DEBUG
uint32_t		swm_debug = 0
			    | XT_D_MOVE
			    | XT_D_KEY
			    | XT_D_TAB
			    | XT_D_URL
			    | XT_D_CMD
			    | XT_D_NAV
			    | XT_D_DOWNLOAD
			    | XT_D_CONFIG
			    | XT_D_JS
			    | XT_D_FAVORITE
			    | XT_D_PRINTING
			    | XT_D_COOKIE
			    | XT_D_KEYBINDING
			    | XT_D_CLIP
			    | XT_D_BUFFERCMD
			    | XT_D_INSPECTOR
			    | XT_D_VISITED
			    | XT_D_HISTORY
			    ;
#endif

#ifdef USE_THREADS
GCRY_THREAD_OPTION_PTHREAD_IMPL;
#endif

char		*icons[] = {
	"xombreroicon16.png",
	"xombreroicon32.png",
	"xombreroicon48.png",
	"xombreroicon64.png",
	"xombreroicon128.png"
};

struct session {
	TAILQ_ENTRY(session)	entry;
	const gchar		*name;
};
TAILQ_HEAD(session_list, session);

struct undo {
	TAILQ_ENTRY(undo)	entry;
	gchar			*uri;
	GList			*history;
	int			back; /* Keeps track of how many back
				       * history items there are. */
};
TAILQ_HEAD(undo_tailq, undo);

struct command_entry {
	char				*line;
	TAILQ_ENTRY(command_entry)	entry;
};
TAILQ_HEAD(command_list, command_entry);

/* defines */
#define XT_CACHE_DIR		("cache")
#define XT_CERT_DIR		("certs")
#define XT_CERT_CACHE_DIR	("certs_cache")
#define XT_JS_DIR		("js")
#define XT_SESSIONS_DIR		("sessions")
#define XT_TEMP_DIR		("tmp")
#define XT_QMARKS_FILE		("quickmarks")
#define XT_SAVED_TABS_FILE	("main_session")
#define XT_RESTART_TABS_FILE	("restart_tabs")
#define XT_SOCKET_FILE		("socket")
#define XT_SAVE_SESSION_ID	("SESSION_NAME=")
#define XT_SEARCH_FILE		("search_history")
#define XT_COMMAND_FILE		("command_history")
#define XT_DLMAN_REFRESH	"10"
#define XT_MAX_URL_LENGTH	(4096) /* 1 page is atomic, don't make bigger */
#define XT_MAX_UNDO_CLOSE_TAB	(32)
#define XT_PRINT_EXTRA_MARGIN	10
#define XT_URL_REGEX		("^[[:blank:]]*[^[:blank:]]*([[:alnum:]-]+\\.)+[[:alnum:]-][^[:blank:]]*[[:blank:]]*$")
#define XT_INVALID_MARK		(-1) /* XXX this is a double, maybe use something else, like a nan */

/* colors */
#define XT_COLOR_RED		"#cc0000"
#define XT_COLOR_YELLOW		"#ffff66"
#define XT_COLOR_BLUE		"lightblue"
#define XT_COLOR_GREEN		"#99ff66"
#define XT_COLOR_WHITE		"white"
#define XT_COLOR_BLACK		"black"

#define XT_COLOR_CT_BACKGROUND	"#000000"
#define XT_COLOR_CT_INACTIVE	"#dddddd"
#define XT_COLOR_CT_ACTIVE	"#bbbb00"
#define XT_COLOR_CT_SEPARATOR	"#555555"

#define XT_COLOR_SB_SEPARATOR	"#555555"

/* CSS element names */
#define XT_CSS_NORMAL		""
#define XT_CSS_RED		"red"
#define XT_CSS_YELLOW		"yellow"
#define XT_CSS_GREEN		"green"
#define XT_CSS_BLUE		"blue"
#define XT_CSS_HIDDEN		"hidden"
#define XT_CSS_ACTIVE		"active"

#define XT_PROTO_DELIM		"://"

/* actions */
#define XT_MOVE_INVALID		(0)
#define XT_MOVE_DOWN		(1)
#define XT_MOVE_UP		(2)
#define XT_MOVE_BOTTOM		(3)
#define XT_MOVE_TOP		(4)
#define XT_MOVE_PAGEDOWN	(5)
#define XT_MOVE_PAGEUP		(6)
#define XT_MOVE_HALFDOWN	(7)
#define XT_MOVE_HALFUP		(8)
#define XT_MOVE_LEFT		(9)
#define XT_MOVE_FARLEFT		(10)
#define XT_MOVE_RIGHT		(11)
#define XT_MOVE_FARRIGHT	(12)
#define XT_MOVE_PERCENT		(13)
#define XT_MOVE_CENTER		(14)

#define XT_QMARK_SET		(0)
#define XT_QMARK_OPEN		(1)
#define XT_QMARK_TAB		(2)

#define XT_MARK_SET		(0)
#define XT_MARK_GOTO		(1)

#define XT_GO_UP_ROOT		(999)

#define XT_NAV_INVALID		(0)
#define XT_NAV_BACK		(1)
#define XT_NAV_FORWARD		(2)
#define XT_NAV_RELOAD		(3)
#define XT_NAV_STOP		(4)

#define XT_FOCUS_INVALID	(0)
#define XT_FOCUS_URI		(1)
#define XT_FOCUS_SEARCH		(2)

#define XT_SEARCH_INVALID	(0)
#define XT_SEARCH_NEXT		(1)
#define XT_SEARCH_PREV		(2)

#define XT_PASTE_CURRENT_TAB	(0)
#define XT_PASTE_NEW_TAB	(1)

#define XT_ZOOM_IN		(-1)
#define XT_ZOOM_OUT		(-2)
#define XT_ZOOM_NORMAL		(100)

#define XT_SES_DONOTHING	(0)
#define XT_SES_CLOSETABS	(1)

#define XT_PREFIX		(1<<0)
#define XT_USERARG		(1<<1)
#define XT_URLARG		(1<<2)
#define XT_INTARG		(1<<3)
#define XT_SESSARG		(1<<4)
#define XT_SETARG		(1<<5)

#define XT_HINT_NEWTAB		(1<<0)

#define XT_BUFCMD_SZ		(8)

#define XT_EJS_SHOW		(1<<0)

GtkWidget *	create_button(char *, char *, int);

void		recalc_tabs(void);
void		recolor_compact_tabs(void);
void		set_current_tab(int page_num);
gboolean	update_statusbar_position(GtkAdjustment*, gpointer);
void		marks_clear(struct tab *t);

/* globals */
extern char		*__progname;
char * const		*start_argv;
struct passwd		*pwd;
GtkWidget		*main_window;
GtkNotebook		*notebook;
GtkWidget		*tab_bar;
GtkWidget		*tab_bar_box;
GtkWidget		*arrow, *abtn;
GdkEvent		*fevent = NULL;
struct tab_list		tabs;
struct history_list	hl;
int			hl_purge_count = 0;
struct session_list	sessions;
struct domain_list	c_wl;
struct domain_list	js_wl;
struct domain_list	pl_wl;
struct domain_list	force_https;
struct domain_list	svil;
struct strict_transport_tree	st_tree;
struct undo_tailq	undos;
struct keybinding_list	kbl;
struct sp_list		spl;
struct user_agent_list	ua_list;
struct http_accept_list	ha_list;
struct cmd_alias_list	cal;
struct custom_uri_list	cul;
struct command_list	chl;
struct command_list	shl;
struct command_entry	*history_at;
struct command_entry	*search_at;
struct secviolation_list	svl;
struct set_reject_list	srl;
int			undo_count;
int			cmd_history_count = 0;
int			search_history_count = 0;
char			*global_search;
uint64_t		blocked_cookies = 0;
char			named_session[PATH_MAX];
GtkListStore		*completion_model;
GtkListStore		*buffers_store;
char			*stylesheet;

char			*qmarks[XT_NOQMARKS];
int			btn_down;	/* M1 down in any wv */
regex_t			url_re;		/* guess_search regex */

/* starts from 1 to catch atoi() failures when calling xtp_handle_dl() */
int			next_download_id = 1;

void			xxx_dir(char *);
int			icon_size_map(int);
void			activate_uri_entry_cb(GtkWidget*, struct tab *);

void
history_delete(struct command_list *l, int *counter)
{
	struct command_entry	*c;

	if (l == NULL || counter == NULL)
		return;

	c = TAILQ_LAST(l, command_list);
	if (c == NULL)
		return;

	TAILQ_REMOVE(l, c, entry);
	*counter -= 1;
	g_free(c->line);
	g_free(c);
}

void
history_add(struct command_list *list, char *file, char *l, int *counter)
{
	struct command_entry	*c;
	FILE			*f;

	if (list == NULL || l == NULL || counter == NULL)
		return;

	/* don't add the same line */
	c = TAILQ_FIRST(list);
	if (c)
		if (!strcmp(c->line + 1 /* skip space */, l))
			return;

	c = g_malloc0(sizeof *c);
	c->line = g_strdup_printf(" %s", l);

	*counter += 1;
	TAILQ_INSERT_HEAD(list, c, entry);

	if (*counter > 1000)
		history_delete(list, counter);

	if (history_autosave && file) {
		f = fopen(file, "w");
		if (f == NULL) {
			show_oops(NULL, "couldn't write history %s", file);
			return;
		}

		TAILQ_FOREACH_REVERSE(c, list, command_list, entry) {
			c->line[0] = ' ';
			fprintf(f, "%s\n", c->line);
		}

		fclose(f);
	}
}

int
history_read(struct command_list *list, char *file, int *counter)
{
	FILE			*f;
	char			*s, line[65536];

	if (list == NULL || file == NULL)
		return (1);

	f = fopen(file, "r");
	if (f == NULL) {
		startpage_add("couldn't open history file %s", file);
		return (1);
	}

	for (;;) {
		s = fgets(line, sizeof line, f);
		if (s == NULL || feof(f) || ferror(f))
			break;
		if ((s = strchr(line, '\n')) == NULL) {
			startpage_add("invalid history file %s", file);
			fclose(f);
			return (1);
		}
		*s = '\0';

		history_add(list, NULL, line + 1, counter);
	}

	fclose(f);

	return (0);
}

/* marks array storage. */
char
indextomark(int i)
{
	if (i < 0 || i >= XT_NOMARKS)
		return (0);

	return XT_MARKS[i];
}

int
marktoindex(char m)
{
	char *ret;

	if ((ret = strchr(XT_MARKS, m)) != NULL)
		return ret - XT_MARKS;

	return (-1);
}

/* quickmarks array storage. */
char
indextoqmark(int i)
{
	if (i < 0 || i >= XT_NOQMARKS)
		return (0);

	return XT_QMARKS[i];
}

int
qmarktoindex(char m)
{
	char *ret;

	if ((ret = strchr(XT_QMARKS, m)) != NULL)
		return ret - XT_QMARKS;

	return (-1);
}

int
is_g_object_setting(GObject *o, char *str)
{
	guint			n_props = 0, i;
	GParamSpec		**proplist;
	int			rv = 0;

	if (!G_IS_OBJECT(o))
		return (0);

	proplist = g_object_class_list_properties(G_OBJECT_GET_CLASS(o),
	    &n_props);

	for (i = 0; i < n_props; i++) {
		if (! strcmp(proplist[i]->name, str)) {
			rv = 1;
			break;
		}
	}

	g_free(proplist);
	return (rv);
}

struct tab *
get_current_tab(void)
{
	struct tab		*t;

	TAILQ_FOREACH(t, &tabs, entry) {
		if (t->tab_id == gtk_notebook_get_current_page(notebook))
			return (t);
	}

	warnx("%s: no current tab", __func__);

	return (NULL);
}

int
set_ssl_ca_file(struct settings *s, char *file)
{
	struct stat		sb;

	if (file == NULL || strlen(file) == 0)
		return (-1);
	if (stat(file, &sb)) {
		warnx("no CA file: %s", file);
		return (-1);
	}
	expand_tilde(ssl_ca_file, sizeof ssl_ca_file, file);
	g_object_set(session,
	    SOUP_SESSION_SSL_CA_FILE, ssl_ca_file,
	    SOUP_SESSION_SSL_STRICT, ssl_strict_certs,
	    (void *)NULL);
	return (0);
}

void
set_status(struct tab *t, gchar *fmt, ...)
{
	va_list	ap;
	gchar	*status;

	va_start(ap, fmt);

	status = g_strdup_vprintf(fmt, ap);

	gtk_entry_set_text(GTK_ENTRY(t->sbe.uri), status);

	if (!t->status)
		t->status = status;
	else if (strcmp(t->status, status)) {
		/* set new status */
		g_free(t->status);
		t->status = status;
	} else
		g_free(status);

	va_end(ap);
}

void
hide_cmd(struct tab *t)
{
	DNPRINTF(XT_D_CMD, "%s: tab %d\n", __func__, t->tab_id);

	history_at = NULL; /* just in case */
	search_at = NULL; /* just in case */
	gtk_widget_set_can_focus(t->cmd, FALSE);
	gtk_widget_hide(t->cmd);
}

void
show_cmd(struct tab *t)
{
	DNPRINTF(XT_D_CMD, "%s: tab %d\n", __func__, t->tab_id);

	/* without this you can't middle click in t->cmd to paste */
	gtk_entry_set_text(GTK_ENTRY(t->cmd), "");

	history_at = NULL;
	search_at = NULL;
	gtk_widget_hide(t->oops);
	gtk_widget_set_can_focus(t->cmd, TRUE);
	gtk_widget_show(t->cmd);
}

void
hide_buffers(struct tab *t)
{
	gtk_widget_hide(t->buffers);
	gtk_widget_set_can_focus(t->buffers, FALSE);
	gtk_list_store_clear(buffers_store);
}

enum {
	COL_ID		= 0,
	COL_FAVICON,
	COL_TITLE,
	NUM_COLS
};

int
sort_tabs_by_page_num(struct tab ***stabs)
{
	int			num_tabs = 0;
	struct tab		*t;

	num_tabs = gtk_notebook_get_n_pages(notebook);

	*stabs = g_malloc0(num_tabs * sizeof(struct tab *));

	TAILQ_FOREACH(t, &tabs, entry)
		(*stabs)[gtk_notebook_page_num(notebook, t->vbox)] = t;

	return (num_tabs);
}

void
buffers_make_list(void)
{
	int			i, num_tabs;
	const gchar		*title = NULL;
	GtkTreeIter		iter;
	struct tab		**stabs = NULL;

	num_tabs = sort_tabs_by_page_num(&stabs);

	for (i = 0; i < num_tabs; i++)
		if (stabs[i]) {
			gtk_list_store_append(buffers_store, &iter);
			title = get_title(stabs[i], FALSE);
			gtk_list_store_set(buffers_store, &iter,
			    COL_ID, i + 1, /* Enumerate the tabs starting from 1
					    * rather than 0. */
			    COL_FAVICON, gtk_image_get_pixbuf
			        (GTK_IMAGE(stabs[i]->tab_elems.favicon)),
			    COL_TITLE, title,
			    -1);
		}

	g_free(stabs);
}

void
show_buffers(struct tab *t)
{
	GtkTreeIter		iter;
	GtkTreeSelection	*sel;
	GtkTreePath		*path;
	int			index;

	if (gtk_widget_get_visible(GTK_WIDGET(t->buffers)))
		return;

	buffers_make_list();

	sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(t->buffers));
	index = gtk_notebook_get_current_page(notebook);
	path = gtk_tree_path_new_from_indices(index, -1);
	if (gtk_tree_model_get_iter(GTK_TREE_MODEL(buffers_store), &iter, path))
		gtk_tree_selection_select_iter(sel, &iter);
	gtk_tree_path_free(path);

	gtk_widget_show(t->buffers);
	gtk_widget_set_can_focus(t->buffers, TRUE);
	gtk_widget_grab_focus(GTK_WIDGET(t->buffers));
}

void
toggle_buffers(struct tab *t)
{
	if (gtk_widget_get_visible(t->buffers))
		hide_buffers(t);
	else
		show_buffers(t);
}

int
buffers(struct tab *t, struct karg *args)
{
	show_buffers(t);

	return (0);
}

int
set_scrollbar_visibility(struct tab *t, int visible)
{
#if GTK_CHECK_VERSION(3, 0, 0)
	GtkWidget		*h_scrollbar, *v_scrollbar;

	h_scrollbar = gtk_scrolled_window_get_hscrollbar(
	    GTK_SCROLLED_WINDOW(t->browser_win));
	v_scrollbar = gtk_scrolled_window_get_vscrollbar(
	    GTK_SCROLLED_WINDOW(t->browser_win));

	if (visible == 0) {
		gtk_widget_set_name(h_scrollbar, XT_CSS_HIDDEN);
		gtk_widget_set_name(v_scrollbar, XT_CSS_HIDDEN);
	} else {
		gtk_widget_set_name(h_scrollbar, "");
		gtk_widget_set_name(v_scrollbar, "");
	}

	return (0);
#else
	return (visible == 0);
#endif
}

void
hide_oops(struct tab *t)
{
	gtk_widget_hide(t->oops);
}

void
show_oops(struct tab *at, const char *fmt, ...)
{
	va_list			ap;
	char			*msg = NULL;
	struct tab		*t = NULL;

	if (fmt == NULL)
		return;

	if (at == NULL) {
		if ((t = get_current_tab()) == NULL)
			return;
	} else
		t = at;

	va_start(ap, fmt);
	if ((msg = g_strdup_vprintf(fmt, ap)) == NULL)
		errx(1, "show_oops failed");
	va_end(ap);

	gtk_entry_set_text(GTK_ENTRY(t->oops), msg);
	gtk_widget_hide(t->cmd);
	gtk_widget_show(t->oops);

	if (msg)
		g_free(msg);
}

char			work_dir[PATH_MAX];
char			certs_dir[PATH_MAX];
char			certs_cache_dir[PATH_MAX];
char			js_dir[PATH_MAX];
char			cache_dir[PATH_MAX];
char			sessions_dir[PATH_MAX];
char			temp_dir[PATH_MAX];
char			cookie_file[PATH_MAX];
char			*strict_transport_file = NULL;
SoupSession		*session;
SoupCookieJar		*s_cookiejar;
SoupCookieJar		*p_cookiejar;
char			rc_fname[PATH_MAX];

struct mime_type_list	mtl;
struct alias_list	aliases;

/* protos */
struct tab		*create_new_tab(char *, struct undo *, int, int);
void			delete_tab(struct tab *);
void			setzoom_webkit(struct tab *, int);
int			download_rb_cmp(struct download *, struct download *);
gboolean		cmd_execute(struct tab *t, char *str);

int
history_rb_cmp(struct history *h1, struct history *h2)
{
	return (strcmp(h1->uri, h2->uri));
}
RB_GENERATE(history_list, history, entry, history_rb_cmp);

int
domain_rb_cmp(struct domain *d1, struct domain *d2)
{
	return (strcmp(d1->d, d2->d));
}
RB_GENERATE(domain_list, domain, entry, domain_rb_cmp);

int
download_rb_cmp(struct download *e1, struct download *e2)
{
	return (e1->id < e2->id ? -1 : e1->id > e2->id);
}
RB_GENERATE(download_list, download, entry, download_rb_cmp);

int
secviolation_rb_cmp(struct secviolation *s1, struct secviolation *s2)
{
	return (s1->xtp_arg < s2->xtp_arg ? -1 : s1->xtp_arg > s2->xtp_arg);
}
RB_GENERATE(secviolation_list, secviolation, entry, secviolation_rb_cmp);

int
user_agent_rb_cmp(struct user_agent *ua1, struct user_agent *ua2)
{
	return (ua1->id < ua2->id ? -1 : ua1->id > ua2->id);
}
RB_GENERATE(user_agent_list, user_agent, entry, user_agent_rb_cmp);

int
http_accept_rb_cmp(struct http_accept *ha1, struct http_accept *ha2)
{
	return (ha1->id < ha2->id ? -1 : ha1->id > ha2->id);
}
RB_GENERATE(http_accept_list, http_accept, entry, http_accept_rb_cmp);

struct valid_url_types {
	char		*type;
} vut[] = {
	{ "http://" },
	{ "https://" },
	{ "ftp://" },
	{ "file://" },
	{ XT_URI_ABOUT },
	{ XT_XTP_STR },
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
match_alias(char *url_in)
{
	struct alias		*a;
	char			*arg;
	char			*url_out = NULL, *search, *enc_arg;
	char			**sv;

	search = g_strdup(url_in);
	arg = search;
	if (strsep(&arg, " \t") == NULL) {
		show_oops(NULL, "match_alias: NULL URL");
		goto done;
	}

	TAILQ_FOREACH(a, &aliases, entry) {
		if (!strcmp(search, a->a_name))
			break;
	}

	if (a != NULL) {
		DNPRINTF(XT_D_URL, "match_alias: matched alias %s\n",
		    a->a_name);
		enc_arg = soup_uri_encode(arg, XT_RESERVED_CHARS);
		sv = g_strsplit(a->a_uri, "%s", 2);
		if (arg != NULL)
			url_out = g_strjoinv(enc_arg, sv);
		else
			url_out = g_strjoinv("", sv);
		g_free(enc_arg);
		g_strfreev(sv);
	}
done:
	g_free(search);
	return (url_out);
}

char *
guess_url_type(char *url_in)
{
	struct stat		sb;
	char			cwd[PATH_MAX] = {0};
	char			*url_out = NULL, *enc_search = NULL;
	char			*path = NULL;
	char			**sv = NULL;
	int			i;


	/* substitute aliases */
	url_out = match_alias(url_in);
	if (url_out != NULL)
		return (url_out);

	/* see if we are an about page */
	if (!strncmp(url_in, XT_URI_ABOUT, XT_URI_ABOUT_LEN))
		for (i = 0; i < about_list_size(); i++)
			if (!strcmp(&url_in[XT_URI_ABOUT_LEN],
			    about_list[i].name)) {
				url_out = g_strdup(url_in);
				goto done;
			}

	if (guess_search && url_regex &&
	    !(g_str_has_prefix(url_in, "http://") ||
	    g_str_has_prefix(url_in, "https://"))) {
		if (regexec(&url_re, url_in, 0, NULL, 0)) {
			/* invalid URI so search instead */
			enc_search = soup_uri_encode(url_in, XT_RESERVED_CHARS);
			sv = g_strsplit(search_string, "%s", 2);
			url_out = g_strjoinv(enc_search, sv);
			g_free(enc_search);
			g_strfreev(sv);
			goto done;
		}
	}

	/* XXX not sure about this heuristic */
	if (stat(url_in, &sb) == 0) {
		if (url_in[0] == '/')
			url_out = g_filename_to_uri(url_in, NULL, NULL);
		else {
			if (getcwd(cwd, PATH_MAX) != NULL) {
				path = g_strdup_printf("%s" PS "%s", cwd,
				    url_in);
				url_out = g_filename_to_uri(path, NULL, NULL);
				g_free(path);
			}
		}
	} else
		url_out = g_strdup_printf("http://%s", url_in); /* guess http */
done:
	DNPRINTF(XT_D_URL, "guess_url_type: guessed %s\n", url_out);

	return (url_out);
}

void
set_normal_tab_meaning(struct tab *t)
{
	if (t == NULL)
		return;

	t->xtp_meaning = XT_XTP_TAB_MEANING_NORMAL;
	if (t->session_key != NULL) {
		g_free(t->session_key);
		t->session_key = NULL;
	}
}

void
load_uri(struct tab *t, gchar *uri)
{
	struct karg	args;
	gchar		*newuri = NULL;
	int		i;

	if (uri == NULL)
		return;

	/* Strip leading spaces. */
	while (*uri && isspace(*uri))
		uri++;

	if (strlen(uri) == 0) {
		blank(t, NULL);
		return;
	}

	set_normal_tab_meaning(t);

	if (valid_url_type(uri)) {
		newuri = guess_url_type(uri);
		uri = newuri;
	}

	if (!strncmp(uri, XT_URI_ABOUT, XT_URI_ABOUT_LEN)) {
		for (i = 0; i < about_list_size(); i++)
			if (!strcmp(&uri[XT_URI_ABOUT_LEN], about_list[i].name) &&
			    about_list[i].func != NULL) {
				bzero(&args, sizeof args);
				about_list[i].func(t, &args);
				gtk_widget_set_sensitive(GTK_WIDGET(t->stop),
				    FALSE);
				goto done;
			}
		show_oops(t, "invalid about page");
		goto done;
	}

	set_status(t, "Loading: %s", (char *)uri);
	marks_clear(t);
	webkit_web_view_load_uri(t->wv, uri);
done:
	if (newuri)
		g_free(newuri);
}

const gchar *
get_uri(struct tab *t)
{
	const gchar		*uri = NULL;

	if (webkit_web_view_get_load_status(t->wv) == WEBKIT_LOAD_FAILED &&
	    !t->download_requested)
		return (NULL);
	if (t->xtp_meaning == XT_XTP_TAB_MEANING_NORMAL) {
		uri = webkit_web_view_get_uri(t->wv);
	} else {
		/* use tmp_uri to make sure it is g_freed */
		if (t->tmp_uri)
			g_free(t->tmp_uri);
		t->tmp_uri = g_strdup_printf("%s%s", XT_URI_ABOUT,
		    about_list[t->xtp_meaning].name);
		uri = t->tmp_uri;
	}
	return uri;
}

const gchar *
get_title(struct tab *t, bool window)
{
	const gchar		*set = NULL, *title = NULL;
	WebKitLoadStatus	status = webkit_web_view_get_load_status(t->wv);

	if (status == WEBKIT_LOAD_PROVISIONAL ||
	    (status == WEBKIT_LOAD_FAILED && !t->download_requested) ||
	    t->xtp_meaning == XT_XTP_TAB_MEANING_BL)
		goto notitle;

	title = webkit_web_view_get_title(t->wv);
	if ((set = title ? title : get_uri(t)))
		return set;

notitle:
	set = window ? XT_NAME : "(untitled)";

	return set;
}

struct mime_type *
find_mime_type(char *mime_type)
{
	struct mime_type	*m, *def = NULL, *rv = NULL;

	TAILQ_FOREACH(m, &mtl, entry) {
		if (m->mt_default &&
		    !strncmp(mime_type, m->mt_type, strlen(m->mt_type)))
			def = m;

		if (m->mt_default == 0 && !strcmp(mime_type, m->mt_type)) {
			rv = m;
			break;
		}
	}

	if (rv == NULL)
		rv = def;

	return (rv);
}

/*
 * This only escapes the & and < characters, as per the discussion found here:
 * http://lists.apple.com/archives/Webkitsdk-dev/2007/May/msg00056.html
 */
char *
html_escape(const char *val)
{
	char			*s, *sp;
	char			**sv;

	if (val == NULL)
		return NULL;

	sv = g_strsplit(val, "&", -1);
	s = g_strjoinv("&amp;", sv);
	g_strfreev(sv);
	sp = s;
	sv = g_strsplit(val, "<", -1);
	s = g_strjoinv("&lt;", sv);
	g_strfreev(sv);
	g_free(sp);
	return (s);
}

struct domain *
wl_find_uri(const gchar *s, struct domain_list *wl)
{
	int			i;
	char			*ss;
	struct domain		*r;

	if (s == NULL || wl == NULL)
		return (NULL);

	if (!strncmp(s, "http://", strlen("http://")))
		s = &s[strlen("http://")];
	else if (!strncmp(s, "https://", strlen("https://")))
		s = &s[strlen("https://")];

	if (strlen(s) < 2)
		return (NULL);

	for (i = 0; i < strlen(s) + 1 /* yes er need this */; i++)
		/* chop string at first slash */
		if (s[i] == '/' || s[i] == ':' || s[i] == '\0') {
			ss = g_strdup(s);
			ss[i] = '\0';
			r = wl_find(ss, wl);
			g_free(ss);
			return (r);
		}

	return (NULL);
}

char *
js_ref_to_string(JSContextRef context, JSValueRef ref)
{
	char			*s = NULL;
	size_t			l;
	JSStringRef		jsref;

	jsref = JSValueToStringCopy(context, ref, NULL);
	if (jsref == NULL)
		return (NULL);

	l = JSStringGetMaximumUTF8CStringSize(jsref);
	s = g_malloc(l);
	if (s)
		JSStringGetUTF8CString(jsref, s, l);
	JSStringRelease(jsref);

	return (s);
}

#define XT_JS_DONE		("done;")
#define XT_JS_DONE_LEN		(strlen(XT_JS_DONE))
#define XT_JS_INSERT		("insert;")
#define XT_JS_INSERT_LEN	(strlen(XT_JS_INSERT))

int
run_script(struct tab *t, char *s)
{
	JSGlobalContextRef	ctx;
	WebKitWebFrame		*frame;
	JSStringRef		str;
	JSValueRef		val, exception;
	char			*es;

	DNPRINTF(XT_D_JS, "%s: tab %d %s\n", __func__,
	    t->tab_id, s == (char *)JS_HINTING ? "JS_HINTING" : s);

	frame = webkit_web_view_get_main_frame(t->wv);
	ctx = webkit_web_frame_get_global_context(frame);

	str = JSStringCreateWithUTF8CString(s);
	val = JSEvaluateScript(ctx, str, JSContextGetGlobalObject(ctx),
	    NULL, 0, &exception);
	JSStringRelease(str);

	DNPRINTF(XT_D_JS, "%s: val %p\n", __func__, val);
	if (val == NULL) {
		es = js_ref_to_string(ctx, exception);
		if (es) {
			DNPRINTF(XT_D_JS, "%s: exception %s\n", __func__, es);
			g_free(es);
		}
		return (1);
	} else {
		es = js_ref_to_string(ctx, val);
#if 0
		/* return values */
		if (!strncmp(es, XT_JS_DONE, XT_JS_DONE_LEN))
			; /* do nothing */
		if (!strncmp(es, XT_JS_INSERT, XT_JS_INSERT_LEN))
			; /* do nothing */
#endif
		if (es) {
			DNPRINTF(XT_D_JS, "%s: val %s\n", __func__, es);
			g_free(es);
		}
	}

	return (0);
}

int
run_script_locked(struct tab *t, char *s)
{
	int		rv;
#ifdef USE_THREADS
	gdk_threads_enter();
#endif
	rv = run_script(t, s);
#ifdef USE_THREADS
	GDK_FLUSH();
	gdk_threads_leave();
#endif
	return (rv);
}

void
enable_hints(struct tab *t)
{
	DNPRINTF(XT_D_JS, "%s: tab %d\n", __func__, t->tab_id);

	if (t->new_tab)
		run_script(t, "hints.createHints('', 'F');");
	else
		run_script(t, "hints.createHints('', 'f');");
	t->mode = XT_MODE_HINT;
}

void
disable_hints(struct tab *t)
{
	DNPRINTF(XT_D_JS, "%s: tab %d\n", __func__, t->tab_id);

	run_script(t, "hints.clearHints();");
	t->mode = XT_MODE_COMMAND;
	t->new_tab = 0;
}

int
passthrough(struct tab *t, struct karg *args)
{
	t->mode = XT_MODE_PASSTHROUGH;
	return (0);
}

int
modurl(struct tab *t, struct karg *args)
{
	const gchar		*uri = NULL;
	char			*u = NULL;

	/* XXX kind of a bad hack, but oh well */
	if (gtk_widget_has_focus(t->uri_entry)) {
		if ((uri = gtk_entry_get_text(GTK_ENTRY(t->uri_entry))) &&
		    (strlen(uri))) {
			u = g_strdup_printf("www.%s.com", uri);
			gtk_entry_set_text(GTK_ENTRY(t->uri_entry), u);
			g_free(u);
			activate_uri_entry_cb(t->uri_entry, t);
		}
	}
	return (0);
}

int
hint(struct tab *t, struct karg *args)
{

	DNPRINTF(XT_D_JS, "hint: tab %d args %d\n", t->tab_id, args->i);

	if (t->mode == XT_MODE_HINT) {
		if (args->i == XT_HINT_NEWTAB)
			t->new_tab = 1;
		enable_hints(t);
	} else
		disable_hints(t);

	return (0);
}

void
apply_style(struct tab *t)
{
	t->styled = 1;
	g_object_set(G_OBJECT(t->settings),
	    "user-stylesheet-uri", t->stylesheet, (char *)NULL);
}

void
remove_style(struct tab *t)
{
	t->styled = 0;
	g_object_set(G_OBJECT(t->settings),
	    "user-stylesheet-uri", NULL, (char *)NULL);
}

int
userstyle_cmd(struct tab *t, struct karg *args)
{
	char			script[PATH_MAX] = {'\0'};
	char			*script_uri;
	struct tab		*tt;

	DNPRINTF(XT_D_JS, "userstyle_cmd: tab %d\n", t->tab_id);

	if (args->s != NULL && strlen(args->s)) {
		expand_tilde(script, sizeof script, args->s);
		script_uri = g_filename_to_uri(script, NULL, NULL);
	} else
		script_uri = g_strdup(userstyle);

	if (script_uri == NULL)
		return (1);

	switch (args->i) {
	case XT_STYLE_CURRENT_TAB:
		if (t->styled && !strcmp(script_uri, t->stylesheet))
			remove_style(t);
		else {
			if (t->stylesheet)
				g_free(t->stylesheet);
			t->stylesheet = g_strdup(script_uri);
			apply_style(t);
		}
		break;
	case XT_STYLE_GLOBAL:
		if (userstyle_global && !strcmp(script_uri, t->stylesheet)) {
			userstyle_global = 0;
			TAILQ_FOREACH(tt, &tabs, entry)
				remove_style(tt);
		} else {
			userstyle_global = 1;

			/* need to save this stylesheet for new tabs */
			if (stylesheet)
				g_free(stylesheet);
			stylesheet = g_strdup(script_uri);

			TAILQ_FOREACH(tt, &tabs, entry) {
				if (tt->stylesheet)
					g_free(tt->stylesheet);
				tt->stylesheet = g_strdup(script_uri);
				apply_style(tt);
			}
		}
		break;
	}

	g_free(script_uri);

	return (0);
}

int
quit(struct tab *t, struct karg *args)
{
	if (save_global_history)
		save_global_history_to_disk(t);

	gtk_main_quit();

	return (1);
}

void
restore_sessions_list(void)
{
	DIR		*sdir = NULL;
	struct dirent	*dp = NULL;
	struct session	*s;
	int		reg;

	sdir = opendir(sessions_dir);
	if (sdir) {
		while ((dp = readdir(sdir)) != NULL) {
#if defined __MINGW32__
			reg = 1; /* windows only has regular files */
#else
			reg = dp->d_type == DT_REG;
#endif
			if (dp && reg) {
				s = g_malloc(sizeof(struct session));
				s->name = g_strdup(dp->d_name);
				TAILQ_INSERT_TAIL(&sessions, s, entry);
			}
		}
		closedir(sdir);
	}
}

int
open_tabs(struct tab *t, struct karg *a)
{
	char		file[PATH_MAX];
	FILE		*f = NULL;
	char		*uri = NULL;
	int		rv = 1;
	struct tab	*ti, *tt;

	if (a == NULL)
		goto done;

	ti = TAILQ_LAST(&tabs, tab_list);

	snprintf(file, sizeof file, "%s" PS "%s", sessions_dir, a->s);
	if ((f = fopen(file, "r")) == NULL)
		goto done;

	for (;;) {
		if ((uri = fparseln(f, NULL, NULL, "\0\0\0", 0)) == NULL)
			if (feof(f) || ferror(f))
				break;

		/* retrieve session name */
		if (uri && g_str_has_prefix(uri, XT_SAVE_SESSION_ID)) {
			strlcpy(named_session,
			    &uri[strlen(XT_SAVE_SESSION_ID)],
			    sizeof named_session);
			continue;
		}

		if (uri && strlen(uri))
			create_new_tab(uri, NULL, 1, -1);

		free(uri);
		uri = NULL;
	}

	/* close open tabs */
	if (a->i == XT_SES_CLOSETABS && ti != NULL) {
		for (;;) {
			tt = TAILQ_FIRST(&tabs);
			if (tt == NULL)
				break;
			if (tt != ti) {
				delete_tab(tt);
				continue;
			}
			delete_tab(tt);
			break;
		}
	}

	rv = 0;
done:
	if (f)
		fclose(f);

	return (rv);
}

int
restore_saved_tabs(void)
{
	char		file[PATH_MAX];
	int		unlink_file = 0;
	struct stat	sb;
	struct karg	a;
	int		rv = 0;

	snprintf(file, sizeof file, "%s" PS "%s",
	    sessions_dir, XT_RESTART_TABS_FILE);
	if (stat(file, &sb) == -1)
		a.s = XT_SAVED_TABS_FILE;
	else {
		unlink_file = 1;
		a.s = XT_RESTART_TABS_FILE;
	}

	a.i = XT_SES_DONOTHING;
	rv = open_tabs(NULL, &a);

	if (unlink_file)
		unlink(file);

	return (rv);
}

int
save_tabs(struct tab *t, struct karg *a)
{
	char			file[PATH_MAX];
	FILE			*f;
	int			num_tabs = 0, i;
	struct tab		**stabs = NULL;

	/* tab may be null here */

	if (a == NULL)
		return (1);
	if (a->s == NULL)
		snprintf(file, sizeof file, "%s" PS "%s",
		    sessions_dir, named_session);
	else
		snprintf(file, sizeof file, "%s" PS "%s", sessions_dir, a->s);

	if ((f = fopen(file, "w")) == NULL) {
		show_oops(t, "Can't open save_tabs file: %s", strerror(errno));
		return (1);
	}

	/* save session name */
	fprintf(f, "%s%s\n", XT_SAVE_SESSION_ID, named_session);

	/* Save tabs, in the order they are arranged in the notebook. */
	num_tabs = sort_tabs_by_page_num(&stabs);

	for (i = 0; i < num_tabs; i++)
		if (stabs[i]) {
			if (get_uri(stabs[i]) != NULL)
				fprintf(f, "%s\n", get_uri(stabs[i]));
			else if (gtk_entry_get_text(GTK_ENTRY(
			    stabs[i]->uri_entry)))
				fprintf(f, "%s\n", gtk_entry_get_text(GTK_ENTRY(
				    stabs[i]->uri_entry)));
		}

	g_free(stabs);

	/* try and make sure this gets to disk NOW. XXX Backup first? */
	if (fflush(f) != 0 || fsync(fileno(f)) != 0) {
		show_oops(t, "May not have managed to save session: %s",
		    strerror(errno));
	}

	fclose(f);

	return (0);
}

int
save_tabs_and_quit(struct tab *t, struct karg *args)
{
	struct karg		a;

	a.s = NULL;
	save_tabs(t, &a);
	quit(t, NULL);

	return (1);
}

void
expand_tilde(char *path, size_t len, const char *s)
{
	struct passwd		*pwd;
	int			i;
	char			user[LOGIN_NAME_MAX];
	const char		*sc = s;

	if (path == NULL || s == NULL)
		errx(1, "expand_tilde");

	if (s[0] != '~') {
		strlcpy(path, sc, len);
		return;
	}

	++s;
	for (i = 0; s[i] != PSC && s[i] != '\0'; ++i)
		user[i] = s[i];
	user[i] = '\0';
	s = &s[i];

	pwd = strlen(user) == 0 ? getpwuid(getuid()) : getpwnam(user);
	if (pwd == NULL)
		strlcpy(path, sc, len);
	else
		snprintf(path, len, "%s%s", pwd->pw_dir, s);
}

int
run_page_script(struct tab *t, struct karg *args)
{
	const gchar		*uri;
	char			*tmp, script[PATH_MAX];
	char			*sv[3];

	tmp = args->s != NULL && strlen(args->s) > 0 ? args->s : default_script;
	if (tmp[0] == '\0') {
		show_oops(t, "no script specified");
		return (1);
	}

	if ((uri = get_uri(t)) == NULL) {
		show_oops(t, "tab is empty, not running script");
		return (1);
	}

	expand_tilde(script, sizeof script, tmp);

	sv[0] = script;
	sv[1] = (char *)uri;
	sv[2] = NULL;
	if (!g_spawn_async(NULL, sv, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL,
	    NULL, NULL)) {
		show_oops(t, "%s: could not spawn process: %s %s", __func__,
		    sv[0], sv[1]);
		return (1);
	} else
		show_oops(t, "running: %s %s", sv[0], sv[1]);

	return (0);
}

int
yank_uri(struct tab *t, struct karg *args)
{
	const gchar		*uri;
	GtkClipboard		*clipboard, *primary;

	if ((uri = get_uri(t)) == NULL)
		return (1);

	primary = gtk_clipboard_get(GDK_SELECTION_PRIMARY);
	clipboard = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
	gtk_clipboard_set_text(primary, uri, -1);
	gtk_clipboard_set_text(clipboard, uri, -1);

	return (0);
}

int
paste_uri(struct tab *t, struct karg *args)
{
	GtkClipboard		*clipboard, *primary;
	gchar			*c = NULL, *p = NULL, *uri;
	int			i;

	clipboard = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
	primary = gtk_clipboard_get(GDK_SELECTION_PRIMARY);
	c = gtk_clipboard_wait_for_text(clipboard);
	p = gtk_clipboard_wait_for_text(primary);

	if (c || p) {
#ifdef __MINGW32__
		/* Windows try clipboard first */
		uri = c ? c : p;
#else
		/* UNIX try primary first */
		uri = p ? p : c;
#endif
		/* replace all newlines with spaces */
		for (i = 0; uri[i] != '\0'; ++i)
			if (uri[i] == '\n')
				uri[i] = ' ';

		while (*uri && isspace(*uri))
			uri++;
		if (strlen(uri) == 0) {
			show_oops(t, "empty paste buffer");
			goto done;
		}
		if (guess_search == 0 && valid_url_type(uri)) {
			/* we can be clever and paste this in search box */
			show_oops(t, "not a valid URL");
			goto done;
		}

		if (args->i == XT_PASTE_CURRENT_TAB)
			load_uri(t, uri);
		else if (args->i == XT_PASTE_NEW_TAB)
			create_new_tab(uri, NULL, 1, -1);
	}

done:
	if (c)
		g_free(c);
	if (p)
		g_free(p);

	return (0);
}

void
js_toggle_cb(GtkWidget *w, struct tab *t)
{
	struct karg		a;
	int			es, set;

	g_object_get(G_OBJECT(t->settings),
	    "enable-scripts", &es, (char *)NULL);
	es = !es;
	if (es)
		set = XT_WL_ENABLE;
	else
		set = XT_WL_DISABLE;

	a.i = set | XT_WL_TOPLEVEL;
	toggle_pl(t, &a);

	a.i = set | XT_WL_TOPLEVEL;
	toggle_cwl(t, &a);

	a.i = XT_WL_TOGGLE | XT_WL_TOPLEVEL | XT_WL_RELOAD;
	toggle_js(t, &a);
}

int
toggle_src(struct tab *t, struct karg *args)
{
	gboolean		mode;

	if (t == NULL)
		return (0);

	mode = webkit_web_view_get_view_source_mode(t->wv);
	webkit_web_view_set_view_source_mode(t->wv, !mode);
	webkit_web_view_reload(t->wv);

	return (0);
}

void
focus_webview(struct tab *t)
{
	if (t == NULL)
		return;

	/* only grab focus if we are visible */
	if (gtk_notebook_get_current_page(notebook) == t->tab_id)
		gtk_widget_grab_focus(GTK_WIDGET(t->wv));
}

int
focus(struct tab *t, struct karg *args)
{
	if (t == NULL || args == NULL)
		return (1);

	if (show_url == 0)
		return (0);

	if (args->i == XT_FOCUS_URI)
		gtk_widget_grab_focus(GTK_WIDGET(t->uri_entry));
	else if (args->i == XT_FOCUS_SEARCH)
		gtk_widget_grab_focus(GTK_WIDGET(t->search_entry));

	return (0);
}

int
connect_socket_from_uri(const gchar *uri, const gchar **error_str, char *domain,
    size_t domain_sz)
{
	SoupURI			*su = NULL;
	struct addrinfo		hints, *res = NULL, *ai;
	int			rv = -1, s = -1, on, error;
	char			port[8];
	static gchar		myerror[256]; /* this is not thread safe */

	myerror[0] = '\0';
	*error_str = myerror;
	if (uri && !g_str_has_prefix(uri, "https://")) {
		*error_str = "invalid URI";
		goto done;
	}

	su = soup_uri_new(uri);
	if (su == NULL) {
		*error_str = "invalid soup URI";
		goto done;
	}
	if (!SOUP_URI_VALID_FOR_HTTP(su)) {
		*error_str = "invalid HTTPS URI";
		goto done;
	}

	snprintf(port, sizeof port, "%d", su->port);
	bzero(&hints, sizeof(struct addrinfo));
	hints.ai_flags = AI_CANONNAME;
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if ((error = getaddrinfo(su->host, port, &hints, &res))) {
		snprintf(myerror, sizeof myerror, "getaddrinfo failed: %s",
		    gai_strerror(errno));
		goto done;
	}

	for (ai = res; ai; ai = ai->ai_next) {
		if (s != -1) {
			close(s);
			s = -1;
		}

		if (ai->ai_family != AF_INET && ai->ai_family != AF_INET6)
			continue;
		s = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
		if (s == -1)
			continue;
		if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &on,
		    sizeof(on)) == -1)
			continue;
		if (connect(s, ai->ai_addr, ai->ai_addrlen) == 0)
			break;
	}
	if (s == -1) {
		snprintf(myerror, sizeof myerror,
		    "could not obtain certificates from: %s",
		    su->host);
		goto done;
	}

	if (domain)
		strlcpy(domain, su->host, domain_sz);
	rv = s;
done:
	if (su)
		soup_uri_free(su);
	if (res)
		freeaddrinfo(res);
	if (rv == -1 && s != -1)
		close(s);

	return (rv);
}

#ifdef __MINGW32__
static ssize_t
custom_gnutls_push(void *s, const void *buf, size_t len)
{
	return send((size_t)s, buf, len, 0);
}

static ssize_t
custom_gnutls_pull(void *s, void *buf, size_t len)
{
	return recv((size_t)s, buf, len, 0);
}
#endif

int
stop_tls(gnutls_session_t gsession, gnutls_certificate_credentials_t xcred)
{
	if (gsession)
		gnutls_deinit(gsession);
	if (xcred)
		gnutls_certificate_free_credentials(xcred);

	return (0);
}

int
start_tls(const gchar **error_str, int s, gnutls_session_t *gs,
    gnutls_certificate_credentials_t *xc)
{
	gnutls_certificate_credentials_t xcred;
	gnutls_session_t	gsession;
	int			rv = 1;
	static gchar		myerror[1024]; /* this is not thread safe */

	if (gs == NULL || xc == NULL)
		goto done;

	myerror[0] = '\0';
	*gs = NULL;
	*xc = NULL;

	gnutls_certificate_allocate_credentials(&xcred);
	gnutls_certificate_set_x509_trust_file(xcred, ssl_ca_file,
	    GNUTLS_X509_FMT_PEM);

	gnutls_init(&gsession, GNUTLS_CLIENT);
	gnutls_priority_set_direct(gsession, "PERFORMANCE", NULL);
	gnutls_credentials_set(gsession, GNUTLS_CRD_CERTIFICATE, xcred);
	gnutls_transport_set_ptr(gsession, (gnutls_transport_ptr_t)(long)s);
#ifdef __MINGW32__
	/* sockets on windows don't use file descriptors */
	gnutls_transport_set_push_function(gsession, custom_gnutls_push);
	gnutls_transport_set_pull_function(gsession, custom_gnutls_pull);
#endif
	if ((rv = gnutls_handshake(gsession)) < 0) {
		snprintf(myerror, sizeof myerror,
		    "gnutls_handshake failed %d fatal %d %s",
		    rv,
		    gnutls_error_is_fatal(rv),
#if LIBGNUTLS_VERSION_MAJOR >= 2 && LIBGNUTLS_VERSION_MINOR >= 6
		    gnutls_strerror_name(rv));
#else
		    "GNUTLS version is too old to provide human readable error");
#endif
		stop_tls(gsession, xcred);
		goto done;
	}

	gnutls_credentials_type_t cred;
	cred = gnutls_auth_get_type(gsession);
	if (cred != GNUTLS_CRD_CERTIFICATE) {
		snprintf(myerror, sizeof myerror,
		    "gnutls_auth_get_type failed %d",
		    (int)cred);
		stop_tls(gsession, xcred);
		goto done;
	}

	*gs = gsession;
	*xc = xcred;
	rv = 0;
done:
	*error_str = myerror;
	return (rv);
}

int
get_connection_certs(gnutls_session_t gsession, gnutls_x509_crt_t **certs,
    size_t *cert_count)
{
	unsigned int		len;
	const gnutls_datum_t	*cl;
	gnutls_x509_crt_t	*all_certs;
	int			i, rv = 1;

	if (certs == NULL || cert_count == NULL)
		goto done;
	if (gnutls_certificate_type_get(gsession) != GNUTLS_CRT_X509)
		goto done;
	cl = gnutls_certificate_get_peers(gsession, &len);
	if (len == 0)
		goto done;

	all_certs = g_malloc(sizeof(gnutls_x509_crt_t) * len);
	for (i = 0; i < len; i++) {
		gnutls_x509_crt_init(&all_certs[i]);
		if (gnutls_x509_crt_import(all_certs[i], &cl[i],
		    GNUTLS_X509_FMT_PEM < 0)) {
			g_free(all_certs);
			goto done;
		    }
	}

	*certs = all_certs;
	*cert_count = len;
	rv = 0;
done:
	return (rv);
}

void
free_connection_certs(gnutls_x509_crt_t *certs, size_t cert_count)
{
	int			i;

	for (i = 0; i < cert_count; i++)
		gnutls_x509_crt_deinit(certs[i]);
	g_free(certs);
}

#if GTK_CHECK_VERSION(3, 0, 0)
void
statusbar_modify_attr(struct tab *t, const char *css_name)
{
	gtk_widget_set_name(t->sbe.ebox, css_name);
}
#else
void
statusbar_modify_attr(struct tab *t, const char *text, const char *base)
{
	GdkColor                c_text, c_base;

	gdk_color_parse(text, &c_text);
	gdk_color_parse(base, &c_base);

	gtk_widget_modify_bg(t->sbe.ebox, GTK_STATE_NORMAL, &c_base);
	gtk_widget_modify_base(t->sbe.uri, GTK_STATE_NORMAL, &c_base);
	gtk_widget_modify_text(t->sbe.uri, GTK_STATE_NORMAL, &c_text);
}
#endif

void
save_certs(struct tab *t, gnutls_x509_crt_t *certs,
    size_t cert_count, const char *domain, const char *dir)
{
	size_t			cert_buf_sz;
	char			cert_buf[64 * 1024], file[PATH_MAX];
	int			i;
	FILE			*f;

	if (t == NULL || certs == NULL || cert_count <= 0 || domain == NULL)
		return;

	snprintf(file, sizeof file, "%s" PS "%s", dir, domain);
	if ((f = fopen(file, "w")) == NULL) {
		show_oops(t, "Can't create cert file %s %s",
		    file, strerror(errno));
		return;
	}

	for (i = 0; i < cert_count; i++) {
		cert_buf_sz = sizeof cert_buf;
		if (gnutls_x509_crt_export(certs[i], GNUTLS_X509_FMT_PEM,
		    cert_buf, &cert_buf_sz)) {
			show_oops(t, "gnutls_x509_crt_export failed");
			goto done;
		}
		if (fwrite(cert_buf, cert_buf_sz, 1, f) != 1) {
			show_oops(t, "Can't write certs: %s", strerror(errno));
			goto done;
		}
	}

done:
	fclose(f);
}

enum cert_trust {
	CERT_LOCAL,
	CERT_TRUSTED,
	CERT_UNTRUSTED,
	CERT_BAD
};

enum cert_trust
load_compare_cert(const gchar *uri, const gchar **error_str, const char *dir)
{
	char			domain[8182], file[PATH_MAX];
	char			cert_buf[64 * 1024], r_cert_buf[64 * 1024];
	int			s = -1, i;
	unsigned int		error = 0;
	FILE			*f = NULL;
	size_t			cert_buf_sz, cert_count;
	enum cert_trust		rv = CERT_UNTRUSTED;
	static gchar		serr[80]; /* this isn't thread safe */
	gnutls_session_t	gsession;
	gnutls_x509_crt_t	*certs;
	gnutls_certificate_credentials_t xcred;

	DNPRINTF(XT_D_URL, "%s: %s\n", __func__, uri);

	serr[0] = '\0';
	*error_str = serr;
	if ((s = connect_socket_from_uri(uri, error_str, domain,
	    sizeof domain)) == -1)
		return (rv);

	DNPRINTF(XT_D_URL, "%s: fd %d\n", __func__, s);

	/* go ssl/tls */
	if (start_tls(error_str, s, &gsession, &xcred))
		goto done;
	DNPRINTF(XT_D_URL, "%s: got tls\n", __func__);

	/* verify certs in case cert file doesn't exist */
	if (gnutls_certificate_verify_peers2(gsession, &error) !=
	    GNUTLS_E_SUCCESS) {
		*error_str = "Invalid certificates";
		goto done;
	}

	/* get certs */
	if (get_connection_certs(gsession, &certs, &cert_count)) {
		*error_str = "Can't get connection certificates";
		goto done;
	}

	snprintf(file, sizeof file, "%s" PS "%s", dir, domain);
	if ((f = fopen(file, "r")) == NULL) {
		if (!error)
			rv = CERT_TRUSTED;
		goto freeit;
	}

	for (i = 0; i < cert_count; i++) {
		cert_buf_sz = sizeof cert_buf;
		if (gnutls_x509_crt_export(certs[i], GNUTLS_X509_FMT_PEM,
		    cert_buf, &cert_buf_sz)) {
			goto freeit;
		}
		if (fread(r_cert_buf, cert_buf_sz, 1, f) != 1 && !feof(f)) {
			rv = CERT_BAD; /* critical */
			goto freeit;
		}
		if (bcmp(r_cert_buf, cert_buf, cert_buf_sz)) {
			rv = CERT_BAD; /* critical */
			goto freeit;
		}
		rv = CERT_LOCAL;
	}

freeit:
	if (f)
		fclose(f);
	free_connection_certs(certs, cert_count);
done:
	/* we close the socket first for speed */
	if (s != -1)
		close(s);

	/* only complain if we didn't save it locally */
	if (strlen(ssl_ca_file) != 0 && error && rv != CERT_LOCAL) {
		strlcpy(serr, "Certificate exception(s): ", sizeof serr);
		if (error & GNUTLS_CERT_INVALID)
			strlcat(serr, "invalid, ", sizeof serr);
		if (error & GNUTLS_CERT_REVOKED)
			strlcat(serr, "revoked, ", sizeof serr);
		if (error & GNUTLS_CERT_SIGNER_NOT_FOUND)
			strlcat(serr, "signer not found, ", sizeof serr);
		if (error & GNUTLS_CERT_SIGNER_NOT_CA)
			strlcat(serr, "not signed by CA, ", sizeof serr);
		if (error & GNUTLS_CERT_INSECURE_ALGORITHM)
			strlcat(serr, "insecure algorithm, ", sizeof serr);
#if LIBGNUTLS_VERSION_MAJOR >= 2 && LIBGNUTLS_VERSION_MINOR >= 6
		if (error & GNUTLS_CERT_NOT_ACTIVATED)
			strlcat(serr, "not activated, ", sizeof serr);
		if (error & GNUTLS_CERT_EXPIRED)
			strlcat(serr, "expired, ", sizeof serr);
#endif
		for (i = strlen(serr) - 1; i > 0; i--)
			if (serr[i] == ',') {
				serr[i] = '\0';
				break;
			}
		*error_str = serr;
	}

	stop_tls(gsession, xcred);

	return (rv);
}

gnutls_x509_crt_t *
get_local_cert_chain(const char *uri, size_t *ncerts, const char **error_str,
    const char *dir)
{
	SoupURI			*su;
	unsigned char		cert_buf[64 * 1024] = {0};
	gnutls_datum_t		data;
	unsigned int		len = UINT_MAX;
	int			bytes_read;
	char			file[PATH_MAX];
	FILE			*f;
	gnutls_x509_crt_t	*certs;

	if ((su = soup_uri_new(uri)) == NULL) {
		*error_str = "Invalid URI";
		return (NULL);
	}

	snprintf(file, sizeof file, "%s" PS "%s", dir, su->host);
	if ((f = fopen(file, "r")) == NULL) {
		*error_str = "Could not read local cert";
		return (NULL);
	}

	bytes_read = fread(cert_buf, sizeof *cert_buf, sizeof cert_buf, f);
	if (bytes_read == 0) {
		*error_str = "Could not read local cert";
		return (NULL);
	}

	data.data = cert_buf;
	data.size = bytes_read;
	certs = g_malloc(sizeof *certs);
	*ncerts = INT_MAX;
	if (gnutls_x509_crt_list_import(certs, &len, &data,
	    GNUTLS_X509_FMT_PEM, 0) < 0) {
		*error_str = "Error reading local cert chain";
		return (NULL);
	}

	*ncerts = len;
	return (certs);
}


int
cert_cmd(struct tab *t, struct karg *args)
{
	const gchar		*uri, *error_str = NULL;
	char			domain[8182];
	int			s = -1;
	size_t			cert_count;
	gnutls_session_t	gsession;
	gnutls_x509_crt_t	*certs;
	gnutls_certificate_credentials_t xcred;
#if !GTK_CHECK_VERSION(3, 0, 0)
	GdkColor		color;
#endif

	if (t == NULL)
		return (1);

	if (args->s != NULL)
		uri = args->s;
	else if ((uri = get_uri(t)) == NULL) {
		show_oops(t, "Invalid URI");
		return (1);
	}

	/*
	 * if we're only showing the local certs, don't open a socket and get
	 * the remote certs
	 */
	if (args->i & XT_SHOW && args->i & XT_CACHE) {
		certs = get_local_cert_chain(uri, &cert_count, &error_str,
		    certs_cache_dir);
		if (error_str == NULL) {
			show_certs(t, certs, cert_count, "Certificate Chain");
			free_connection_certs(certs, cert_count);
		} else {
			show_oops(t, "%s", error_str);
			return (1);
		}
		return (0);
	}

	if ((s = connect_socket_from_uri(uri, &error_str, domain,
	    sizeof domain)) == -1) {
		show_oops(t, "%s", error_str);
		return (1);
	}

	/* go ssl/tls */
	if (start_tls(&error_str, s, &gsession, &xcred))
		goto done;

	/* get certs */
	if (get_connection_certs(gsession, &certs, &cert_count)) {
		show_oops(t, "get_connection_certs failed");
		goto done;
	}

	if (args->i & XT_SHOW)
		show_certs(t, certs, cert_count, "Certificate Chain");
	else if (args->i & XT_SAVE) {
		save_certs(t, certs, cert_count, domain, certs_dir);
#if GTK_CHECK_VERSION(3, 0, 0)
		gtk_widget_set_name(t->uri_entry, XT_CSS_BLUE);
		statusbar_modify_attr(t, XT_CSS_BLUE);
#else
		gdk_color_parse(XT_COLOR_BLUE, &color);
		gtk_widget_modify_base(t->uri_entry, GTK_STATE_NORMAL, &color);
		statusbar_modify_attr(t, XT_COLOR_BLACK, XT_COLOR_BLUE);
#endif
	} else if (args->i & XT_CACHE)
		save_certs(t, certs, cert_count, domain, certs_cache_dir);

	free_connection_certs(certs, cert_count);
done:
	/* we close the socket first for speed */
	if (s != -1)
		close(s);
	stop_tls(gsession, xcred);
	if (error_str && strlen(error_str))
		show_oops(t, "%s", error_str);
	return (0);
}

/*
 * args must be allocated dynamically as the thread that added this function
 * to the idle loop no longer exists
 */
gboolean
warn_cert_cache_differs_idle(struct karg *args)
{
	if (args == NULL) {
		show_oops(NULL, "%s: invalid parameters", __func__);
		/* return 0 to not re-add function to the idle loop */
		return (0);
	}
	xtp_page_sv((struct tab *)args->ptr, args);
	g_free(args);
	return (0);
}

int
check_cert_changes(struct tab *t, const char *uri)
{
	SoupURI			*soupuri = NULL;
	struct karg		args = {0};
	struct domain		*d = NULL;
	const char		*errstr = NULL;
	struct karg		*argsp;

	if (!(warn_cert_changes && g_str_has_prefix(uri, "https://")))
		return (0);

	switch (load_compare_cert(uri, &errstr, certs_cache_dir)) {
	case CERT_LOCAL:
		/* The cached certificate is identical */
		break;
	case CERT_TRUSTED:	/* FALLTHROUGH */
	case CERT_UNTRUSTED:
		/* cache new certificate */
		args.i = XT_CACHE;
		cert_cmd(t, &args);
		break;
	case CERT_BAD:
		if ((soupuri = soup_uri_new(uri)) == NULL ||
		    soupuri->host == NULL)
			break;
		if ((d = wl_find(soupuri->host, &svil)) != NULL)
			break;
		t->xtp_meaning = XT_XTP_TAB_MEANING_SV;
		argsp = g_malloc0(sizeof(struct karg));
		argsp->s = g_strdup((char *)uri);
		argsp->ptr = (void *)t;
		g_idle_add((GSourceFunc)warn_cert_cache_differs_idle, argsp);
		break;
	}

	if (soupuri)
		soup_uri_free(soupuri);
	return (0);
}

int
remove_cookie(int index)
{
	int			i, rv = 1;
	GSList			*cf;
	SoupCookie		*c;

	DNPRINTF(XT_D_COOKIE, "remove_cookie: %d\n", index);

	cf = soup_cookie_jar_all_cookies(s_cookiejar);

	for (i = 1; cf; cf = cf->next, i++) {
		if (i != index)
			continue;
		c = cf->data;
		print_cookie("remove cookie", c);
		soup_cookie_jar_delete_cookie(s_cookiejar, c);
		rv = 0;
		break;
	}

	soup_cookies_free(cf);

	return (rv);
}

int
remove_cookie_domain(int domain_id)
{
	int			domain_count, rv = 1;
	GSList			*cf;
	SoupCookie		*c;
	char *last_domain;

	DNPRINTF(XT_D_COOKIE, "remove_cookie_domain: %d\n", domain_id);

	last_domain = "";
	cf = soup_cookie_jar_all_cookies(s_cookiejar);

	for (domain_count = 0; cf; cf = cf->next) {
		c = cf->data;

		if (strcmp(last_domain, c->domain) != 0) {
			domain_count++;
			last_domain = c->domain;
		}

		if (domain_count < domain_id)
			continue;
		else if (domain_count > domain_id)
			break;

		print_cookie("remove cookie", c);
		soup_cookie_jar_delete_cookie(s_cookiejar, c);
		rv = 0;
	}

	soup_cookies_free(cf);

	return (rv);
}

int
remove_cookie_all()
{
	int			rv = 1;
	GSList			*cf;
	SoupCookie		*c;

	DNPRINTF(XT_D_COOKIE, "remove_cookie_all\n");

	cf = soup_cookie_jar_all_cookies(s_cookiejar);

	for (; cf; cf = cf->next) {
		c = cf->data;

		print_cookie("remove cookie", c);
		soup_cookie_jar_delete_cookie(s_cookiejar, c);
		rv = 0;
	}

	soup_cookies_free(cf);

	return (rv);
}

int
toplevel_cmd(struct tab *t, struct karg *args)
{
	js_toggle_cb(t->js_toggle, t);

	return (0);
}

int
can_go_back_for_real(struct tab *t)
{
	int			i;
	WebKitWebHistoryItem	*item;
	const gchar		*uri;

	if (t == NULL)
		return (FALSE);

	/* rely on webkit to make sure we can go backward when on an about page */
	uri = get_uri(t);
	if (uri == NULL || g_str_has_prefix(uri, "about:"))
		return (webkit_web_view_can_go_back(t->wv));

	/* the back/forwars list is stupid so help determine if we can go back */
	for (i = 0, item = webkit_web_back_forward_list_get_current_item(t->bfl);
	    item != NULL;
	    i--, item = webkit_web_back_forward_list_get_nth_item(t->bfl, i)) {
		if (strcmp(webkit_web_history_item_get_uri(item), uri))
			return (TRUE);
	}

	return (FALSE);
}

int
can_go_forward_for_real(struct tab *t)
{
	int			i;
	WebKitWebHistoryItem	*item;
	const gchar		*uri;

	if (t == NULL)
		return (FALSE);

	/* rely on webkit to make sure we can go forward when on an about page */
	uri = get_uri(t);
	if (uri == NULL || g_str_has_prefix(uri, "about:"))
		return (webkit_web_view_can_go_forward(t->wv));

	/* the back/forwars list is stupid so help selecting a different item */
	for (i = 0, item = webkit_web_back_forward_list_get_current_item(t->bfl);
	    item != NULL;
	    i++, item = webkit_web_back_forward_list_get_nth_item(t->bfl, i)) {
		if (strcmp(webkit_web_history_item_get_uri(item), uri))
			return (TRUE);
	}

	return (FALSE);
}

void
go_back_for_real(struct tab *t)
{
	int			i;
	WebKitWebHistoryItem	*item;
	const gchar		*uri;

	if (t == NULL)
		return;

	uri = get_uri(t);
	if (uri == NULL) {
		webkit_web_view_go_back(t->wv);
		return;
	}
	/* the back/forwars list is stupid so help selecting a different item */
	for (i = 0, item = webkit_web_back_forward_list_get_current_item(t->bfl);
	    item != NULL;
	    i--, item = webkit_web_back_forward_list_get_nth_item(t->bfl, i)) {
		if (strcmp(webkit_web_history_item_get_uri(item), uri)) {
			webkit_web_view_go_to_back_forward_item(t->wv, item);
			break;
		}
	}
}

void
go_forward_for_real(struct tab *t)
{
	int			i;
	WebKitWebHistoryItem	*item;
	const gchar		*uri;

	if (t == NULL)
		return;

	uri = get_uri(t);
	if (uri == NULL) {
		webkit_web_view_go_forward(t->wv);
		return;
	}
	/* the back/forwars list is stupid so help selecting a different item */
	for (i = 0, item = webkit_web_back_forward_list_get_current_item(t->bfl);
	    item != NULL;
	    i++, item = webkit_web_back_forward_list_get_nth_item(t->bfl, i)) {
		if (strcmp(webkit_web_history_item_get_uri(item), uri)) {
			webkit_web_view_go_to_back_forward_item(t->wv, item);
			break;
		}
	}
}

int
navaction(struct tab *t, struct karg *args)
{
	WebKitWebHistoryItem	*item;
	WebKitWebFrame		*frame;

	DNPRINTF(XT_D_NAV, "navaction: tab %d opcode %d\n",
	    t->tab_id, args->i);

	hide_oops(t);
	set_normal_tab_meaning(t);
	if (t->item) {
		if (args->i == XT_NAV_BACK)
			item = webkit_web_back_forward_list_get_current_item(t->bfl);
		else
			item = webkit_web_back_forward_list_get_forward_item(t->bfl);
		if (item == NULL)
			return (XT_CB_PASSTHROUGH);
		webkit_web_view_go_to_back_forward_item(t->wv, item);
		t->item = NULL;
		return (XT_CB_PASSTHROUGH);
	}

	switch (args->i) {
	case XT_NAV_BACK:
		marks_clear(t);
		go_back_for_real(t);
		break;
	case XT_NAV_FORWARD:
		marks_clear(t);
		go_forward_for_real(t);
		break;
	case XT_NAV_RELOAD:
		frame = webkit_web_view_get_main_frame(t->wv);
		webkit_web_frame_reload(frame);
		break;
	case XT_NAV_STOP:
		frame = webkit_web_view_get_main_frame(t->wv);
		webkit_web_frame_stop_loading(frame);
		break;
	}
	return (XT_CB_PASSTHROUGH);
}

int
move(struct tab *t, struct karg *args)
{
	GtkAdjustment		*adjust;
	double			pi, si, pos, ps, upper, lower, max;
	double			percent;

	switch (args->i) {
	case XT_MOVE_DOWN:
	case XT_MOVE_UP:
	case XT_MOVE_BOTTOM:
	case XT_MOVE_TOP:
	case XT_MOVE_PAGEDOWN:
	case XT_MOVE_PAGEUP:
	case XT_MOVE_HALFDOWN:
	case XT_MOVE_HALFUP:
	case XT_MOVE_PERCENT:
	case XT_MOVE_CENTER:
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
		t->mark[marktoindex('\'')] = gtk_adjustment_get_value(t->adjust_v);
		gtk_adjustment_set_value(adjust, max);
		break;
	case XT_MOVE_TOP:
	case XT_MOVE_FARLEFT:
		t->mark[marktoindex('\'')] = gtk_adjustment_get_value(t->adjust_v);
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
	case XT_MOVE_HALFDOWN:
		pos += pi / 2;
		gtk_adjustment_set_value(adjust, MIN(pos, max));
		break;
	case XT_MOVE_HALFUP:
		pos -= pi / 2;
		gtk_adjustment_set_value(adjust, MAX(pos, lower));
		break;
	case XT_MOVE_CENTER:
		t->mark[marktoindex('\'')] = gtk_adjustment_get_value(t->adjust_v);
		args->s = g_strdup("50.0");
		/* FALLTHROUGH */
	case XT_MOVE_PERCENT:
		t->mark[marktoindex('\'')] = gtk_adjustment_get_value(t->adjust_v);
		percent = atoi(args->s) / 100.0;
		pos = max * percent;
		if (pos < 0.0 || pos > max)
			break;
		gtk_adjustment_set_value(adjust, pos);
		break;
	default:
		return (XT_CB_PASSTHROUGH);
	}

	DNPRINTF(XT_D_MOVE, "move: new pos %f %f\n", pos, MIN(pos, max));

	return (XT_CB_HANDLED);
}

void
url_set_visibility(void)
{
	struct tab		*t;

	TAILQ_FOREACH(t, &tabs, entry)
		if (show_url == 0) {
			gtk_widget_hide(t->toolbar);
			focus_webview(t);
		} else
			gtk_widget_show(t->toolbar);
}

void
notebook_tab_set_visibility(void)
{
	if (show_tabs == 0) {
		gtk_widget_hide(tab_bar);
		gtk_notebook_set_show_tabs(notebook, FALSE);
	} else {
		if (tab_style == XT_TABS_NORMAL) {
			gtk_widget_hide(tab_bar);
			gtk_notebook_set_show_tabs(notebook, TRUE);
		} else if (tab_style == XT_TABS_COMPACT) {
			gtk_widget_show(tab_bar);
			gtk_notebook_set_show_tabs(notebook, FALSE);
		}
	}
}

void
statusbar_set_visibility(void)
{
	struct tab		*t;

	TAILQ_FOREACH(t, &tabs, entry){
		if (show_statusbar == 0)
			gtk_widget_hide(t->statusbar);
		else
			gtk_widget_show(t->statusbar);

		focus_webview(t);
	}
}

void
url_set(struct tab *t, int enable_url_entry)
{
	GdkPixbuf	*pixbuf;
	int		progress;

	show_url = enable_url_entry;

	if (enable_url_entry) {
		gtk_entry_set_icon_from_icon_name(GTK_ENTRY(t->sbe.uri),
		    GTK_ENTRY_ICON_PRIMARY, NULL);
		gtk_entry_set_progress_fraction(GTK_ENTRY(t->sbe.uri), 0);
	} else {
		pixbuf = gtk_entry_get_icon_pixbuf(GTK_ENTRY(t->uri_entry),
		    GTK_ENTRY_ICON_PRIMARY);
		progress =
		    gtk_entry_get_progress_fraction(GTK_ENTRY(t->uri_entry));
		gtk_entry_set_icon_from_pixbuf(GTK_ENTRY(t->sbe.uri),
		    GTK_ENTRY_ICON_PRIMARY, pixbuf);
		gtk_entry_set_progress_fraction(GTK_ENTRY(t->sbe.uri),
		    progress);
	}
}

int
fullscreen(struct tab *t, struct karg *args)
{
	DNPRINTF(XT_D_TAB, "%s: %p %d\n", __func__, t, args->i);

	if (t == NULL)
		return (XT_CB_PASSTHROUGH);

	if (show_url == 0) {
		url_set(t, 1);
		show_tabs = 1;
	} else {
		url_set(t, 0);
		show_tabs = 0;
	}

	url_set_visibility();
	notebook_tab_set_visibility();

	return (XT_CB_HANDLED);
}

int
statustoggle(struct tab *t, struct karg *args)
{
	DNPRINTF(XT_D_TAB, "%s: %p %d\n", __func__, t, args->i);

	if (show_statusbar == 1) {
		show_statusbar = 0;
		statusbar_set_visibility();
	} else if (show_statusbar == 0) {
		show_statusbar = 1;
		statusbar_set_visibility();
	}
	return (XT_CB_HANDLED);
}

int
urlaction(struct tab *t, struct karg *args)
{
	int			rv = XT_CB_HANDLED;

	DNPRINTF(XT_D_TAB, "%s: %p %d\n", __func__, t, args->i);

	if (t == NULL)
		return (XT_CB_PASSTHROUGH);

	switch (args->i) {
	case XT_URL_SHOW:
		if (show_url == 0) {
			url_set(t, 1);
			url_set_visibility();
		}
		break;
	case XT_URL_HIDE:
		if (show_url == 1) {
			url_set(t, 0);
			url_set_visibility();
		}
		break;
	}
	return (rv);
}

int
tabaction(struct tab *t, struct karg *args)
{
	int			rv = XT_CB_HANDLED;
	char			*url = args->s;
	struct undo		*u;
	struct tab		*tt, *tv;

	DNPRINTF(XT_D_TAB, "tabaction: %p %d\n", t, args->i);

	if (t == NULL)
		return (XT_CB_PASSTHROUGH);

	switch (args->i) {
	case XT_TAB_NEW:
		if (strlen(url) > 0)
			create_new_tab(url, NULL, 1, args->precount);
		else
			create_new_tab(NULL, NULL, 1, args->precount);
		break;
	case XT_TAB_DELETE:
		if (args->precount < 0)
			delete_tab(t);
		else
			TAILQ_FOREACH(tt, &tabs, entry)
				if (tt->tab_id == args->precount - 1) {
					delete_tab(tt);
					break;
				}
		break;
	case XT_TAB_DELQUIT:
		if (gtk_notebook_get_n_pages(notebook) > 1)
			delete_tab(t);
		else
			quit(t, args);
		break;
	case XT_TAB_ONLY:
		TAILQ_FOREACH_SAFE(tt, &tabs, entry, tv)
			if (t != tt)
				delete_tab(tt);
		break;
	case XT_TAB_OPEN:
		if (strlen(url) > 0)
			;
		else {
			rv = XT_CB_PASSTHROUGH;
			goto done;
		}
		load_uri(t, url);
		break;
	case XT_TAB_SHOW:
		if (show_tabs == 0) {
			show_tabs = 1;
			notebook_tab_set_visibility();
		}
		break;
	case XT_TAB_HIDE:
		if (show_tabs == 1) {
			show_tabs = 0;
			notebook_tab_set_visibility();
		}
		break;
	case XT_TAB_NEXTSTYLE:
		if (tab_style == XT_TABS_NORMAL) {
			tab_style = XT_TABS_COMPACT;
			recolor_compact_tabs();
		}
		else
			tab_style = XT_TABS_NORMAL;
		notebook_tab_set_visibility();
		break;
	case XT_TAB_UNDO_CLOSE:
		if (undo_count == 0) {
			DNPRINTF(XT_D_TAB, "%s: no tabs to undo close",
			    __func__);
			goto done;
		} else {
			undo_count--;
			u = TAILQ_FIRST(&undos);
			create_new_tab(u->uri, u, 1, -1);

			TAILQ_REMOVE(&undos, u, entry);
			g_free(u->uri);
			/* u->history is freed in create_new_tab() */
			g_free(u);
		}
		break;
	case XT_TAB_LOAD_IMAGES:

		if (!auto_load_images) {

			/* Enable auto-load images (this will load all
			 * previously unloaded images). */
			g_object_set(G_OBJECT(t->settings),
			    "auto-load-images", TRUE, (char *)NULL);
			webkit_web_view_set_settings(t->wv, t->settings);

			webkit_web_view_reload(t->wv);

			/* Webkit triggers an event when we change the setting,
			 * so we can't disable the auto-loading at once.
			 *
			 * Unfortunately, webkit does not tell us when it's done.
			 * Instead, we wait until the next request, and then
			 * disable autoloading again.
			 */
			t->load_images = TRUE;
		}
		break;
	default:
		rv = XT_CB_PASSTHROUGH;
		goto done;
	}

done:
	if (args->s) {
		g_free(args->s);
		args->s = NULL;
	}

	return (rv);
}

int
resizetab(struct tab *t, struct karg *args)
{
	if (t == NULL || args == NULL) {
		show_oops(NULL, "resizetab invalid parameters");
		return (XT_CB_PASSTHROUGH);
	}

	DNPRINTF(XT_D_TAB, "resizetab: tab %d %d\n",
	    t->tab_id, args->i);

	setzoom_webkit(t, args->i);

	return (XT_CB_HANDLED);
}

int
movetab(struct tab *t, struct karg *args)
{
	int			n, dest;

	if (t == NULL || args == NULL) {
		show_oops(NULL, "movetab invalid parameters");
		return (XT_CB_PASSTHROUGH);
	}

	DNPRINTF(XT_D_TAB, "movetab: tab %d opcode %d\n",
	    t->tab_id, args->i);

	if (args->i >= XT_TAB_INVALID)
		return (XT_CB_PASSTHROUGH);

	if (TAILQ_EMPTY(&tabs))
		return (XT_CB_PASSTHROUGH);

	n = gtk_notebook_get_n_pages(notebook);
	dest = gtk_notebook_get_current_page(notebook);

	switch (args->i) {
	case XT_TAB_NEXT:
		if (args->precount < 0)
			dest = dest == n - 1 ? 0 : dest + 1;
		else
			dest = args->precount - 1;

		break;
	case XT_TAB_PREV:
		if (args->precount < 0)
			dest -= 1;
		else
			dest -= args->precount % n;

		if (dest < 0)
			dest += n;

		break;
	case XT_TAB_FIRST:
		dest = 0;
		break;
	case XT_TAB_LAST:
		dest = n - 1;
		break;
	default:
		return (XT_CB_PASSTHROUGH);
	}

	if (dest < 0 || dest >= n)
		return (XT_CB_PASSTHROUGH);
	if (t->tab_id == dest) {
		DNPRINTF(XT_D_TAB, "movetab: do nothing\n");
		return (XT_CB_HANDLED);
	}

	set_current_tab(dest);

	return (XT_CB_HANDLED);
}

int cmd_prefix = 0;

struct prompt_sub {
	const char		*s;
	const char		*(*f)(struct tab *);
} subs[] = {
	{ "<uri>",	get_uri },
};

int
command(struct tab *t, struct karg *args)
{
	struct karg		a = {0};
	int			i;
	char			*s = NULL, *sp = NULL, *sl = NULL;
	gchar			**sv;

	if (t == NULL || args == NULL) {
		show_oops(NULL, "command invalid parameters");
		return (XT_CB_PASSTHROUGH);
	}

	switch (args->i) {
	case '/':
		s = "/";
		break;
	case '?':
		s = "?";
		break;
	case ':':
		if (cmd_prefix == 0) {
			if (args->s != NULL && strlen(args->s) != 0) {
				sp = g_strdup_printf(":%s", args->s);
				s = sp;
			} else
				s = ":";
		} else {
			sp = g_strdup_printf(":%d", cmd_prefix);
			s = sp;
			cmd_prefix = 0;
		}
		sl = g_strdup(s);
		if (sp) {
			g_free(sp);
			sp = NULL;
		}
		s = sl;
		for (i = 0; i < LENGTH(subs); ++i) {
			sv = g_strsplit(sl, subs[i].s, -1);
			if (sl)
				g_free(sl);
			sl = g_strjoinv(subs[i].f(t), sv);
			g_strfreev(sv);
			s = sl;
		}
		break;
	case '.':
		t->mode = XT_MODE_HINT;
		a.i = 0;
		hint(t, &a);
		s = ".";
		break;
	case ',':
		t->mode = XT_MODE_HINT;
		a.i = XT_HINT_NEWTAB;
		hint(t, &a);
		s = ",";
		break;
	default:
		show_oops(t, "command: invalid opcode %d", args->i);
		return (XT_CB_PASSTHROUGH);
	}

	DNPRINTF(XT_D_CMD, "%s: tab %d type %s\n", __func__, t->tab_id, s);

	show_cmd(t);
	gtk_widget_grab_focus(GTK_WIDGET(t->cmd));
#if GTK_CHECK_VERSION(3, 0, 0)
	gtk_widget_set_name(t->cmd, XT_CSS_NORMAL);
#else
	gtk_widget_modify_base(t->cmd, GTK_STATE_NORMAL,
	    &t->default_style->base[GTK_STATE_NORMAL]);
#endif
	gtk_entry_set_text(GTK_ENTRY(t->cmd), s);
	gtk_editable_set_position(GTK_EDITABLE(t->cmd), -1);

	if (sp)
		g_free(sp);
	if (sl)
		g_free(sl);

	return (XT_CB_HANDLED);
}

int
search(struct tab *t, struct karg *args)
{
	gboolean	d;

	if (t == NULL || args == NULL) {
		show_oops(NULL, "search invalid parameters");
		return (1);
	}

	switch (args->i) {
	case  XT_SEARCH_NEXT:
		d = t->search_forward;
		break;
	case  XT_SEARCH_PREV:
		d = !t->search_forward;
		break;
	default:
		return (XT_CB_PASSTHROUGH);
	}

	if (t->search_text == NULL) {
		if (global_search == NULL)
			return (XT_CB_PASSTHROUGH);
		else {
			d = t->search_forward = TRUE;
			t->search_text = g_strdup(global_search);
			webkit_web_view_mark_text_matches(t->wv, global_search, FALSE, 0);
			webkit_web_view_set_highlight_text_matches(t->wv, TRUE);
		}
	}

	DNPRINTF(XT_D_CMD, "search: tab %d opc %d forw %d text %s\n",
	    t->tab_id, args->i, t->search_forward, t->search_text);

	webkit_web_view_search_text(t->wv, t->search_text, FALSE, d, TRUE);

	return (XT_CB_HANDLED);
}

int
session_save(struct tab *t, char *filename)
{
	struct karg		a;
	int			rv = 1;
	struct session		*s;

	if (strlen(filename) == 0)
		goto done;

	if (filename[0] == '.' || filename[0] == '/')
		goto done;

	a.s = filename;
	if (save_tabs(t, &a))
		goto done;
	strlcpy(named_session, filename, sizeof named_session);

	/* add the new session to the list of sessions */
	s = g_malloc(sizeof(struct session));
	s->name = g_strdup(filename);
	TAILQ_INSERT_TAIL(&sessions, s, entry);

	rv = 0;
done:
	return (rv);
}

int
session_open(struct tab *t, char *filename)
{
	struct karg		a;
	int			rv = 1;

	if (strlen(filename) == 0)
		goto done;

	if (filename[0] == '.' || filename[0] == '/')
		goto done;

	a.s = filename;
	a.i = XT_SES_CLOSETABS;
	if (open_tabs(t, &a))
		goto done;

	strlcpy(named_session, filename, sizeof named_session);

	rv = 0;
done:
	return (rv);
}

int
session_delete(struct tab *t, char *filename)
{
	char			file[PATH_MAX];
	int			rv = 1;
	struct session		*s;

	if (strlen(filename) == 0)
		goto done;

	if (filename[0] == '.' || filename[0] == '/')
		goto done;

	snprintf(file, sizeof file, "%s" PS "%s", sessions_dir, filename);
	if (unlink(file))
		goto done;

	if (!strcmp(filename, named_session))
		strlcpy(named_session, XT_SAVED_TABS_FILE,
		    sizeof named_session);

	/* remove session from sessions list */
	TAILQ_FOREACH(s, &sessions, entry) {
		if (!strcmp(s->name, filename))
			break;
	}
	if (s == NULL)
		goto done;
	TAILQ_REMOVE(&sessions, s, entry);
	g_free((gpointer) s->name);
	g_free(s);

	rv = 0;
done:
	return (rv);
}

int
session_cmd(struct tab *t, struct karg *args)
{
	char			*filename = args->s;

	if (t == NULL)
		return (1);

	if (args->i & XT_SHOW)
		show_oops(t, "Current session: %s", named_session[0] == '\0' ?
		    XT_SAVED_TABS_FILE : named_session);
	else if (args->i & XT_SAVE) {
		if (session_save(t, filename)) {
			show_oops(t, "Can't save session: %s",
			    filename ? filename : "INVALID");
			goto done;
		}
	} else if (args->i & XT_OPEN) {
		if (session_open(t, filename)) {
			show_oops(t, "Can't open session: %s",
			    filename ? filename : "INVALID");
			goto done;
		}
	} else if (args->i & XT_DELETE) {
		if (session_delete(t, filename)) {
			show_oops(t, "Can't delete session: %s",
			    filename ? filename : "INVALID");
			goto done;
		}
	}
done:
	return (XT_CB_PASSTHROUGH);
}

int
script_cmd(struct tab *t, struct karg *args)
{
	struct stat		sb;
	FILE			*f = NULL;
	char			*buf = NULL;

	if (t == NULL)
		goto done;

	if ((f = fopen(args->s, "r")) == NULL) {
		show_oops(t, "Can't open script file: %s", args->s);
		goto done;
	}

	if (fstat(fileno(f), &sb) == -1) {
		show_oops(t, "Can't stat script file: %s", args->s);
		goto done;
	}

	buf = g_malloc0(sb.st_size + 1);
	if (fread(buf, 1, sb.st_size, f) != sb.st_size) {
		show_oops(t, "Can't read script file: %s", args->s);
		goto done;
	}

	DNPRINTF(XT_D_JS, "%s: about to run script\n", __func__);
	run_script(t, buf);

done:
	if (f)
		fclose(f);
	if (buf)
		g_free(buf);

	return (XT_CB_PASSTHROUGH);
}

/*
 * Make a hardcopy of the page
 */
int
print_page(struct tab *t, struct karg *args)
{
	WebKitWebFrame			*frame;
	GtkPageSetup			*ps;
	GtkPrintOperation		*op;
	GtkPrintOperationAction		action;
	GtkPrintOperationResult		print_res;
	GError				*g_err = NULL;
	int				marg_l, marg_r, marg_t, marg_b;

	DNPRINTF(XT_D_PRINTING, "%s:", __func__);

	ps = gtk_page_setup_new();
	op = gtk_print_operation_new();
	action = GTK_PRINT_OPERATION_ACTION_PRINT_DIALOG;
	frame = webkit_web_view_get_main_frame(t->wv);

	/* the default margins are too small, so we will bump them */
	marg_l = gtk_page_setup_get_left_margin(ps, GTK_UNIT_MM) +
	    XT_PRINT_EXTRA_MARGIN;
	marg_r = gtk_page_setup_get_right_margin(ps, GTK_UNIT_MM) +
	    XT_PRINT_EXTRA_MARGIN;
	marg_t = gtk_page_setup_get_top_margin(ps, GTK_UNIT_MM) +
	    XT_PRINT_EXTRA_MARGIN;
	marg_b = gtk_page_setup_get_bottom_margin(ps, GTK_UNIT_MM) +
	    XT_PRINT_EXTRA_MARGIN;

	/* set margins */
	gtk_page_setup_set_left_margin(ps, marg_l, GTK_UNIT_MM);
	gtk_page_setup_set_right_margin(ps, marg_r, GTK_UNIT_MM);
	gtk_page_setup_set_top_margin(ps, marg_t, GTK_UNIT_MM);
	gtk_page_setup_set_bottom_margin(ps, marg_b, GTK_UNIT_MM);

	gtk_print_operation_set_default_page_setup(op, ps);

	/* this appears to free 'op' and 'ps' */
	print_res = webkit_web_frame_print_full(frame, op, action, &g_err);

	/* check it worked */
	if (print_res == GTK_PRINT_OPERATION_RESULT_ERROR) {
		show_oops(NULL, "can't print: %s", g_err->message);
		g_error_free (g_err);
		return (1);
	}

	return (0);
}

int
go_home(struct tab *t, struct karg *args)
{
	load_uri(t, home);
	return (0);
}

int
set_encoding(struct tab *t, struct karg *args)
{
	const gchar	*e;

	if (args->s && strlen(g_strstrip(args->s)) == 0) {
		e = webkit_web_view_get_custom_encoding(t->wv);
		if (e == NULL)
			e = webkit_web_view_get_encoding(t->wv);
		show_oops(t, "encoding: %s", e ? e : "N/A");
	} else
		webkit_web_view_set_custom_encoding(t->wv, args->s);

	return (0);
}

int
restart(struct tab *t, struct karg *args)
{
	struct karg		a;

	a.s = XT_RESTART_TABS_FILE;
	save_tabs(t, &a);
	execvp(start_argv[0], start_argv);
	/* NOTREACHED */

	return (0);
}

char		*http_proxy_save; /* not a setting, used to toggle */

int
proxy_cmd(struct tab *t, struct karg *args)
{
	DNPRINTF(XT_D_CMD, "%s: tab %d\n", __func__, t->tab_id);

	if (t == NULL)
		return (1);

	/* setup */
	if (http_proxy) {
		if (http_proxy_save)
			g_free(http_proxy_save);
		http_proxy_save = g_strdup(http_proxy);
	}

	if (args->i & XT_PRXY_SHOW) {
		if (http_proxy)
			show_oops(t, "http_proxy = %s", http_proxy);
		else
			show_oops(t, "proxy is currently disabled");
	} else if (args->i & XT_PRXY_TOGGLE) {
		if (http_proxy_save == NULL && http_proxy == NULL) {
			show_oops(t, "can't toggle proxy");
			goto done;
		}
		if (http_proxy) {
			setup_proxy(NULL);
			show_oops(t, "http proxy disabled");
		} else {
			setup_proxy(http_proxy_save);
			show_oops(t, "http_proxy = %s", http_proxy);
		}
	}
done:
	return (XT_CB_PASSTHROUGH);
}
struct cmd {
	char		*cmd;
	int		level;
	int		(*func)(struct tab *, struct karg *);
	int		arg;
	int		type;
} cmds[] = {
	{ "command_mode",	0,	command_mode,		XT_MODE_COMMAND,	0 },
	{ "insert_mode",	0,	command_mode,		XT_MODE_INSERT,		0 },
	{ "command",		0,	command,		':',			0 },
	{ "search",		0,	command,		'/',			0 },
	{ "searchb",		0,	command,		'?',			0 },
	{ "hinting",		0,	command,		'.',			0 },
	{ "hinting_newtab",	0,	command,		',',			0 },
	{ "togglesrc",		0,	toggle_src,		0,			0 },
	{ "editsrc",		0,	edit_src,		0,			0 },
	{ "editelement",	0,	edit_element,		0,			0 },
	{ "passthrough",	0,	passthrough,		0,			0 },
	{ "modurl",		0,	modurl,			0,			0 },

	/* yanking and pasting */
	{ "yankuri",		0,	yank_uri,		0,			0 },
	{ "pasteuricur",	0,	paste_uri,		XT_PASTE_CURRENT_TAB,	0 },
	{ "pasteurinew",	0,	paste_uri,		XT_PASTE_NEW_TAB,	0 },

	/* search */
	{ "searchnext",		0,	search,			XT_SEARCH_NEXT,		0 },
	{ "searchprevious",	0,	search,			XT_SEARCH_PREV,		0 },

	/* focus */
	{ "focusaddress",	0,	focus,			XT_FOCUS_URI,		0 },
	{ "focussearch",	0,	focus,			XT_FOCUS_SEARCH,	0 },

	/* hinting */
	{ "hinting",		0,	hint,			0,			0 },
	{ "hinting_newtab",	0,	hint,			XT_HINT_NEWTAB,		0 },

	/* custom stylesheet */
	{ "userstyle",		0,	userstyle_cmd,		XT_STYLE_CURRENT_TAB,	XT_USERARG },
	{ "userstyle_global",	0,	userstyle_cmd,		XT_STYLE_GLOBAL,	XT_USERARG },

	/* navigation */
	{ "goback",		0,	navaction,		XT_NAV_BACK,		0 },
	{ "goforward",		0,	navaction,		XT_NAV_FORWARD,		0 },
	{ "reload",		0,	navaction,		XT_NAV_RELOAD,		0 },
	{ "stop",		0,	navaction,		XT_NAV_STOP,		0 },

	/* vertical movement */
	{ "scrolldown",		0,	move,			XT_MOVE_DOWN,		0 },
	{ "scrollup",		0,	move,			XT_MOVE_UP,		0 },
	{ "scrollbottom",	0,	move,			XT_MOVE_BOTTOM,		0 },
	{ "scrolltop",		0,	move,			XT_MOVE_TOP,		0 },
	{ "1",			0,	move,			XT_MOVE_TOP,		0 },
	{ "scrollhalfdown",	0,	move,			XT_MOVE_HALFDOWN,	0 },
	{ "scrollhalfup",	0,	move,			XT_MOVE_HALFUP,		0 },
	{ "scrollpagedown",	0,	move,			XT_MOVE_PAGEDOWN,	0 },
	{ "scrollpageup",	0,	move,			XT_MOVE_PAGEUP,		0 },
	/* horizontal movement */
	{ "scrollright",	0,	move,			XT_MOVE_RIGHT,		0 },
	{ "scrollleft",		0,	move,			XT_MOVE_LEFT,		0 },
	{ "scrollfarright",	0,	move,			XT_MOVE_FARRIGHT,	0 },
	{ "scrollfarleft",	0,	move,			XT_MOVE_FARLEFT,	0 },

	{ "favorites",		0,	xtp_page_fl,		XT_SHOW,		0 },
	{ "fav",		0,	xtp_page_fl,		XT_SHOW,		0 },
	{ "favedit",		0,	xtp_page_fl,		XT_SHOW|XT_DELETE,	0 },
	{ "favadd",		0,	add_favorite,		0,			0 },

	{ "qall",		0,	quit,			0,			0 },
	{ "quitall",		0,	quit,			0,			0 },
	{ "w",			0,	save_tabs,		0,			0 },
	{ "wq",			0,	save_tabs_and_quit,	0,			0 },
	{ "help",		0,	help,			0,			0 },
	{ "about",		0,	xtp_page_ab,		0,			0 },
	{ "stats",		0,	stats,			0,			0 },
	{ "version",		0,	xtp_page_ab,		0,			0 },

	/* js command */
	{ "js",			0,	js_cmd,			XT_SHOW | XT_WL_PERSISTENT | XT_WL_SESSION,	0 },
	{ "save",		1,	js_cmd,			XT_SAVE | XT_WL_FQDN,				0 },
	{ "domain",		2,	js_cmd,			XT_SAVE | XT_WL_TOPLEVEL,			0 },
	{ "fqdn",		2,	js_cmd,			XT_SAVE | XT_WL_FQDN,				0 },
	{ "show",		1,	js_cmd,			XT_SHOW | XT_WL_PERSISTENT | XT_WL_SESSION,	0 },
	{ "all",		2,	js_cmd,			XT_SHOW | XT_WL_PERSISTENT | XT_WL_SESSION,	0 },
	{ "persistent",		2,	js_cmd,			XT_SHOW | XT_WL_PERSISTENT,			0 },
	{ "session",		2,	js_cmd,			XT_SHOW | XT_WL_SESSION,			0 },
	{ "toggle",		1,	js_cmd,			XT_WL_TOGGLE | XT_WL_FQDN,			0 },
	{ "domain",		2,	js_cmd,			XT_WL_TOGGLE | XT_WL_TOPLEVEL,			0 },
	{ "fqdn",		2,	js_cmd,			XT_WL_TOGGLE | XT_WL_FQDN,			0 },

	/* cookie command */
	{ "cookie",		0,	cookie_cmd,		XT_SHOW | XT_WL_PERSISTENT | XT_WL_SESSION,	0 },
	{ "save",		1,	cookie_cmd,		XT_SAVE | XT_WL_FQDN,				0 },
	{ "domain",		2,	cookie_cmd,		XT_SAVE | XT_WL_TOPLEVEL,			0 },
	{ "fqdn",		2,	cookie_cmd,		XT_SAVE | XT_WL_FQDN,				0 },
	{ "show",		1,	cookie_cmd,		XT_SHOW | XT_WL_PERSISTENT | XT_WL_SESSION,	0 },
	{ "all",		2,	cookie_cmd,		XT_SHOW | XT_WL_PERSISTENT | XT_WL_SESSION,	0 },
	{ "persistent",		2,	cookie_cmd,		XT_SHOW | XT_WL_PERSISTENT,			0 },
	{ "session",		2,	cookie_cmd,		XT_SHOW | XT_WL_SESSION,			0 },
	{ "toggle",		1,	cookie_cmd,		XT_WL_TOGGLE | XT_WL_FQDN,			0 },
	{ "domain",		2,	cookie_cmd,		XT_WL_TOGGLE | XT_WL_TOPLEVEL,			0 },
	{ "fqdn",		2,	cookie_cmd,		XT_WL_TOGGLE | XT_WL_FQDN,			0 },
	{ "purge",		1,	cookie_cmd,		XT_DELETE,					0 },

	/* plugin command */
	{ "plugin",		0,	pl_cmd,			XT_SHOW | XT_WL_PERSISTENT | XT_WL_SESSION,	0 },
	{ "save",		1,	pl_cmd,			XT_SAVE | XT_WL_FQDN,				0 },
	{ "domain",		2,	pl_cmd,			XT_SAVE | XT_WL_TOPLEVEL,			0 },
	{ "fqdn",		2,	pl_cmd,			XT_SAVE | XT_WL_FQDN,				0 },
	{ "show",		1,	pl_cmd,			XT_SHOW | XT_WL_PERSISTENT | XT_WL_SESSION,	0 },
	{ "all",		2,	pl_cmd,			XT_SHOW | XT_WL_PERSISTENT | XT_WL_SESSION,	0 },
	{ "persistent",		2,	pl_cmd,			XT_SHOW | XT_WL_PERSISTENT,			0 },
	{ "session",		2,	pl_cmd,			XT_SHOW | XT_WL_SESSION,			0 },
	{ "toggle",		1,	pl_cmd,			XT_WL_TOGGLE | XT_WL_FQDN,			0 },
	{ "domain",		2,	pl_cmd,			XT_WL_TOGGLE | XT_WL_TOPLEVEL,			0 },
	{ "fqdn",		2,	pl_cmd,			XT_WL_TOGGLE | XT_WL_FQDN,			0 },

	/* https command */
	{ "https",		0,	https_cmd,		XT_SHOW | XT_WL_PERSISTENT | XT_WL_SESSION,	0 },
	{ "save",		1,	https_cmd,		XT_SAVE | XT_WL_FQDN,				0 },
	{ "domain",		2,	https_cmd,		XT_SAVE | XT_WL_TOPLEVEL,			0 },
	{ "fqdn",		2,	https_cmd,		XT_SAVE | XT_WL_FQDN,				0 },
	{ "show",		1,	https_cmd,		XT_SHOW | XT_WL_PERSISTENT | XT_WL_SESSION,	0 },
	{ "all",		2,	https_cmd,		XT_SHOW | XT_WL_PERSISTENT | XT_WL_SESSION,	0 },
	{ "persistent",		2,	https_cmd,		XT_SHOW | XT_WL_PERSISTENT,			0 },
	{ "session",		2,	https_cmd,		XT_SHOW | XT_WL_SESSION,			0 },
	{ "toggle",		1,	https_cmd,		XT_WL_TOGGLE | XT_WL_FQDN,			0 },
	{ "domain",		2,	https_cmd,		XT_WL_TOGGLE | XT_WL_TOPLEVEL,			0 },
	{ "fqdn",		2,	https_cmd,		XT_WL_TOGGLE | XT_WL_FQDN,			0 },

	/* toplevel (domain) command */
	{ "toplevel",		0,	toplevel_cmd,		XT_WL_TOGGLE | XT_WL_TOPLEVEL | XT_WL_RELOAD,	0 },
	{ "toggle",		1,	toplevel_cmd,		XT_WL_TOGGLE | XT_WL_TOPLEVEL | XT_WL_RELOAD,	0 },

	/* cookie jar */
	{ "cookiejar",		0,	xtp_page_cl,		0,			0 },

	/* cert command */
	{ "cert",		0,	cert_cmd,		XT_SHOW,		0 },
	{ "save",		1,	cert_cmd,		XT_SAVE,		0 },
	{ "show",		1,	cert_cmd,		XT_SHOW,		0 },

	{ "ca",			0,	ca_cmd,			0,			0 },
	{ "downloadmgr",	0,	xtp_page_dl,		0,			0 },
	{ "dl",			0,	xtp_page_dl,		0,			0 },
	{ "h",			0,	xtp_page_hl,		0,			0 },
	{ "history",		0,	xtp_page_hl,		0,			0 },
	{ "home",		0,	go_home,		0,			0 },
	{ "restart",		0,	restart,		0,			0 },
	{ "urlhide",		0,	urlaction,		XT_URL_HIDE,		0 },
	{ "urlshow",		0,	urlaction,		XT_URL_SHOW,		0 },
	{ "statustoggle",	0,	statustoggle,		0,			0 },
	{ "run_script",		0,	run_page_script,	0,			XT_USERARG },

	{ "print",		0,	print_page,		0,			0 },

	/* tabs */
	{ "focusin",		0,	resizetab,		XT_ZOOM_IN,		0 },
	{ "focusout",		0,	resizetab,		XT_ZOOM_OUT,		0 },
	{ "focusreset",		0,	resizetab,		XT_ZOOM_NORMAL,		0 },
	{ "q",			0,	tabaction,		XT_TAB_DELQUIT,		0 },
	{ "quit",		0,	tabaction,		XT_TAB_DELQUIT,		0 },
	{ "open",		0,	tabaction,		XT_TAB_OPEN,		XT_URLARG },
	{ "tabclose",		0,	tabaction,		XT_TAB_DELETE,		XT_PREFIX | XT_INTARG},
	{ "tabedit",		0,	tabaction,		XT_TAB_NEW,		XT_PREFIX | XT_URLARG },
	{ "tabfirst",		0,	movetab,		XT_TAB_FIRST,		0 },
	{ "tabhide",		0,	tabaction,		XT_TAB_HIDE,		0 },
	{ "tablast",		0,	movetab,		XT_TAB_LAST,		0 },
	{ "tabnew",		0,	tabaction,		XT_TAB_NEW,		XT_PREFIX | XT_URLARG },
	{ "tabnext",		0,	movetab,		XT_TAB_NEXT,		XT_PREFIX | XT_INTARG},
	{ "tabnextstyle",	0,	tabaction,		XT_TAB_NEXTSTYLE,	0 },
	{ "tabonly",		0,	tabaction,		XT_TAB_ONLY,		0 },
	{ "tabprevious",	0,	movetab,		XT_TAB_PREV,		XT_PREFIX | XT_INTARG},
	{ "tabrewind",		0,	movetab,		XT_TAB_FIRST,		0 },
	{ "tabshow",		0,	tabaction,		XT_TAB_SHOW,		0 },
	{ "tabs",		0,	buffers,		0,			0 },
	{ "tabundoclose",	0,	tabaction,		XT_TAB_UNDO_CLOSE,	0 },
	{ "buffers",		0,	buffers,		0,			0 },
	{ "ls",			0,	buffers,		0,			0 },
	{ "encoding",		0,	set_encoding,		0,			XT_USERARG },
	{ "loadimages",		0,	tabaction,		XT_TAB_LOAD_IMAGES,	0 },

	/* settings */
	{ "set",		0,	set,			0,			XT_SETARG },
	{ "runtime",		0,	xtp_page_rt,		0,			0 },

	{ "fullscreen",		0,	fullscreen,		0,			0 },
	{ "f",			0,	fullscreen,		0,			0 },

	/* sessions */
	{ "session",		0,	session_cmd,		XT_SHOW,		0 },
	{ "delete",		1,	session_cmd,		XT_DELETE,		XT_SESSARG },
	{ "open",		1,	session_cmd,		XT_OPEN,		XT_SESSARG },
	{ "save",		1,	session_cmd,		XT_SAVE,		XT_USERARG },
	{ "show",		1,	session_cmd,		XT_SHOW,		0 },

	/* external javascript */
	{ "script",		0,	script_cmd,		XT_EJS_SHOW,		XT_USERARG },

	/* inspector */
	{ "inspector",		0,	inspector_cmd,		XT_INS_SHOW,		0 },
	{ "show",		1,	inspector_cmd,		XT_INS_SHOW,		0 },
	{ "hide",		1,	inspector_cmd,		XT_INS_HIDE,		0 },

	/* proxy */
	{ "proxy",		0,	proxy_cmd,		XT_PRXY_SHOW,		0 },
	{ "show",		1,	proxy_cmd,		XT_PRXY_SHOW,		0 },
	{ "toggle",		1,	proxy_cmd,		XT_PRXY_TOGGLE,		0 },
};

struct {
	int			index;
	int			len;
	gchar			*list[256];
} cmd_status = {-1, 0};

gboolean
wv_release_button_cb(GtkWidget *btn, GdkEventButton *e, struct tab *t)
{

	if (e->type == GDK_BUTTON_RELEASE && e->button == 1)
		btn_down = 0;

	return (FALSE);
}

gboolean
wv_button_cb(GtkWidget *btn, GdkEventButton *e, struct tab *t)
{
	struct karg		a;
	WebKitHitTestResult	*hit_test_result;
	guint			context;

	hit_test_result = webkit_web_view_get_hit_test_result(t->wv, e);
	g_object_get(hit_test_result, "context", &context, NULL);

	hide_oops(t);
	hide_buffers(t);

	if (context & WEBKIT_HIT_TEST_RESULT_CONTEXT_EDITABLE)
		t->mode = XT_MODE_INSERT;
	else
		t->mode = XT_MODE_COMMAND;

	if (e->type == GDK_BUTTON_PRESS && e->button == 1)
		btn_down = 1;
	else if (e->type == GDK_BUTTON_PRESS && e->button == 8 /* btn 4 */) {
		/* go backward */
		a.i = XT_NAV_BACK;
		navaction(t, &a);

		return (TRUE);
	} else if (e->type == GDK_BUTTON_PRESS && e->button == 9 /* btn 5 */) {
		/* go forward */
		a.i = XT_NAV_FORWARD;
		navaction(t, &a);

		return (TRUE);
	}

	return (FALSE);
}

gboolean
tab_close_cb(GtkWidget *btn, GdkEventButton *e, struct tab *t)
{
	DNPRINTF(XT_D_TAB, "tab_close_cb: tab %d\n", t->tab_id);

	if (e->type == GDK_BUTTON_PRESS && e->button == 1)
		delete_tab(t);

	return (FALSE);
}

int
parse_custom_uri(struct tab *t, const char *uri)
{
	struct custom_uri	*u;
	int			handled = 0;
	char			*sv[3];

	TAILQ_FOREACH(u, &cul, entry) {
		if (strncmp(uri, u->uri, strlen(u->uri)))
			continue;

		handled = 1;
		sv[0] = u->cmd;
		sv[1] = (char *)uri;
		sv[2] = NULL;
		if (!g_spawn_async(NULL, sv, NULL, G_SPAWN_SEARCH_PATH, NULL,
		    NULL, NULL, NULL))
			show_oops(t, "%s: could not spawn process", __func__);
	}

	return (handled);
}

void
activate_uri_entry_cb(GtkWidget* entry, struct tab *t)
{
	const gchar		*uri = gtk_entry_get_text(GTK_ENTRY(entry));

	DNPRINTF(XT_D_URL, "activate_uri_entry_cb: %s\n", uri);

	if (t == NULL) {
		show_oops(NULL, "activate_uri_entry_cb invalid parameters");
		return;
	}

	if (uri == NULL) {
		show_oops(t, "activate_uri_entry_cb no uri");
		return;
	}

	uri += strspn(uri, "\t ");

	if (parse_custom_uri(t, uri))
		return;

	/* otherwise continue to load page normally */
	load_uri(t, (gchar *)uri);
	focus_webview(t);
}

void
activate_search_entry_cb(GtkWidget* entry, struct tab *t)
{
	const gchar		*search = gtk_entry_get_text(GTK_ENTRY(entry));
	char			*newuri = NULL;
	gchar			*enc_search;
	char			**sv;

	DNPRINTF(XT_D_URL, "activate_search_entry_cb: %s\n", search);

	if (t == NULL) {
		show_oops(NULL, "activate_search_entry_cb invalid parameters");
		return;
	}

	if (search_string == NULL || strlen(search_string) == 0) {
		show_oops(t, "no search_string");
		return;
	}

	set_normal_tab_meaning(t);

	enc_search = soup_uri_encode(search, XT_RESERVED_CHARS);
	sv = g_strsplit(search_string, "%s", 2);
	newuri = g_strjoinv(enc_search, sv);
	g_free(enc_search);
	g_strfreev(sv);

	marks_clear(t);
	load_uri(t, newuri);
	focus_webview(t);

	if (newuri)
		g_free(newuri);
}

void
check_and_set_cookie(const gchar *uri, struct tab *t)
{
	struct domain		*d = NULL;
	int			es = 0;

	if (uri == NULL || t == NULL)
		return;

	if ((d = wl_find_uri(uri, &c_wl)) == NULL)
		es = 0;
	else
		es = 1;

	DNPRINTF(XT_D_COOKIE, "check_and_set_cookie: %s %s\n",
	    es ? "enable" : "disable", uri);

	g_object_set(G_OBJECT(t->settings),
	    "enable-html5-local-storage", es, (char *)NULL);
	webkit_web_view_set_settings(t->wv, t->settings);
}

void
check_and_set_js(const gchar *uri, struct tab *t)
{
	struct domain		*d = NULL;
	int			es = 0;

	if (uri == NULL || t == NULL)
		return;

	if ((d = wl_find_uri(uri, &js_wl)) == NULL)
		es = 0;
	else
		es = 1;

	DNPRINTF(XT_D_JS, "check_and_set_js: %s %s\n",
	    es ? "enable" : "disable", uri);

	g_object_set(G_OBJECT(t->settings),
	    "enable-scripts", es, (char *)NULL);
	g_object_set(G_OBJECT(t->settings),
	    "javascript-can-open-windows-automatically", es, (char *)NULL);
	webkit_web_view_set_settings(t->wv, t->settings);

	button_set_stockid(t->js_toggle,
	    es ? GTK_STOCK_MEDIA_PLAY : GTK_STOCK_MEDIA_PAUSE);
}

void
check_and_set_pl(const gchar *uri, struct tab *t)
{
	struct domain		*d = NULL;
	int			es = 0;

	if (uri == NULL || t == NULL)
		return;

	if ((d = wl_find_uri(uri, &pl_wl)) == NULL)
		es = 0;
	else
		es = 1;

	DNPRINTF(XT_D_JS, "check_and_set_pl: %s %s\n",
	    es ? "enable" : "disable", uri);

	g_object_set(G_OBJECT(t->settings),
	    "enable-plugins", es, (char *)NULL);
	webkit_web_view_set_settings(t->wv, t->settings);
}

#if GTK_CHECK_VERSION(3, 0, 0)
/* A lot of this can be removed when gtk2 is dropped on the floor */
char *
get_css_name(const char *col_str)
{
	char			*name = NULL;

	if (!strcmp(col_str, XT_COLOR_WHITE))
		name = g_strdup(XT_CSS_NORMAL);
	else if (!strcmp(col_str, XT_COLOR_RED))
		name = g_strdup(XT_CSS_RED);
	else if (!strcmp(col_str, XT_COLOR_YELLOW))
		name = g_strdup(XT_CSS_YELLOW);
	else if (!strcmp(col_str, XT_COLOR_GREEN))
		name = g_strdup(XT_CSS_GREEN);
	else if (!strcmp(col_str, XT_COLOR_BLUE))
		name = g_strdup(XT_CSS_BLUE);
	return (name);
}
#endif

void
color_address_bar(gpointer p)
{
	struct tab		*tt, *t = p;
	gchar			*col_str = XT_COLOR_WHITE;
	const gchar		*uri, *u = NULL, *error_str = NULL;
#if GTK_CHECK_VERSION(3, 0, 0)
	char			*name;
#else
	GdkColor		color;
	gchar			*text, *base;
#endif

#ifdef USE_THREADS
	gdk_threads_enter();
#endif
	DNPRINTF(XT_D_URL, "%s:\n", __func__);

	/* make sure t still exists */
	if (t == NULL)
		goto done;
	TAILQ_FOREACH(tt, &tabs, entry)
		if (t == tt)
			break;
	if (t != tt)
		goto done;

	if ((uri = get_uri(t)) == NULL)
		goto white;
	u = g_strdup(uri);

#ifdef USE_THREADS
	GDK_FLUSH();
	gdk_threads_leave();
#endif

	col_str = XT_COLOR_YELLOW;
	switch (load_compare_cert(u, &error_str, certs_dir)) {
	case CERT_LOCAL:
		col_str = XT_COLOR_BLUE;
		break;
	case CERT_TRUSTED:
		col_str = (strlen(ssl_ca_file) == 0) ? XT_COLOR_RED
		    : XT_COLOR_GREEN;
		break;
	case CERT_UNTRUSTED:
		col_str = (strlen(ssl_ca_file) == 0) ? XT_COLOR_RED
		    : XT_COLOR_YELLOW;
		break;
	case CERT_BAD:
		col_str = XT_COLOR_RED;
		break;
	}

#ifdef USE_THREADS
	gdk_threads_enter();
#endif
	/* make sure t isn't deleted */
	TAILQ_FOREACH(tt, &tabs, entry)
		if (t == tt)
			break;
	if (t != tt)
		goto done;

#ifdef USE_THREADS
	/* test to see if the user navigated away and canceled the thread */
	if (t->thread != g_thread_self())
		goto done;
	if ((uri = get_uri(t)) == NULL) {
		t->thread = NULL;
		goto done;
	}
	if (strcmp(uri, u)) {
		/* make sure we are still the same url */
		t->thread = NULL;
		goto done;
	}
#endif
white:

	if (!strcmp(col_str, XT_COLOR_WHITE)) {
#if GTK_CHECK_VERSION(3, 0, 0)
		gtk_widget_set_name(t->uri_entry, XT_CSS_NORMAL);
		statusbar_modify_attr(t, XT_CSS_NORMAL);
#else
		text = gdk_color_to_string(
		    &t->default_style->text[GTK_STATE_NORMAL]);
		base = gdk_color_to_string(
		    &t->default_style->base[GTK_STATE_NORMAL]);
		gtk_widget_modify_base(t->uri_entry, GTK_STATE_NORMAL,
		    &t->default_style->base[GTK_STATE_NORMAL]);
		statusbar_modify_attr(t, text, base);
		g_free(text);
		g_free(base);
#endif
	} else {
#if GTK_CHECK_VERSION(3, 0, 0)
		name = get_css_name(col_str);
		gtk_widget_set_name(t->uri_entry, name);
		statusbar_modify_attr(t, name);
		g_free(name);
#else
		gdk_color_parse(col_str, &color);
		gtk_widget_modify_base(t->uri_entry, GTK_STATE_NORMAL, &color);
		statusbar_modify_attr(t, XT_COLOR_BLACK, col_str);
#endif
	}

	if (error_str && error_str[0] != '\0')
		show_oops(t, "%s", error_str);
#ifdef USE_THREADS
	t->thread = NULL;
#endif
done:
	/* t is invalid at this point */
	if (u)
		g_free((gpointer)u);
#ifdef USE_THREADS
	GDK_FLUSH();
	gdk_threads_leave();
#endif
}

void
check_certs(gpointer p)
{
	color_address_bar((struct tab *)p);
	check_cert_changes((struct tab *)p, get_uri((struct tab *)p));
}

void
show_ca_status(struct tab *t, const char *uri)
{
	gchar			*col_str = XT_COLOR_WHITE;
#if GTK_CHECK_VERSION(3, 0, 0)
	char			*name;
#else
	GdkColor		color;
	gchar			*text, *base;
#endif

	DNPRINTF(XT_D_URL, "show_ca_status: %d %s %s\n",
	    ssl_strict_certs, ssl_ca_file, uri);

	if (t == NULL)
		return;

	if (uri == NULL || g_str_has_prefix(uri, "http://") ||
	    !g_str_has_prefix(uri, "https://"))
		goto done;

#ifdef USE_THREADS
	/*
	 * It is not necessary to see if the thread is already running.
	 * If the thread is in progress setting it to something else aborts it
	 * on the way out.
	 */

	/* thread the coloring of the address bar */
	t->thread = g_thread_create((GThreadFunc)check_certs, t, TRUE, NULL);
#else
	check_certs(t);
#endif
	return;

done:
	if (col_str) {
		if (!strcmp(col_str, XT_COLOR_WHITE)) {
#if GTK_CHECK_VERSION(3, 0, 0)
			gtk_widget_set_name(t->uri_entry, XT_CSS_NORMAL);
			statusbar_modify_attr(t, XT_CSS_NORMAL);
#else
			text = gdk_color_to_string(
			    &t->default_style->text[GTK_STATE_NORMAL]);
			base = gdk_color_to_string(
			    &t->default_style->base[GTK_STATE_NORMAL]);
			gtk_widget_modify_base(t->uri_entry, GTK_STATE_NORMAL,
			    &t->default_style->base[GTK_STATE_NORMAL]);
			statusbar_modify_attr(t, text, base);
			g_free(text);
			g_free(base);
#endif
		} else {
#if GTK_CHECK_VERSION(3, 0, 0)
			name = get_css_name(col_str);
			gtk_widget_set_name(t->uri_entry, name);
			statusbar_modify_attr(t, name);
			g_free(name);
#else
			gdk_color_parse(col_str, &color);
			gtk_widget_modify_base(t->uri_entry, GTK_STATE_NORMAL, &color);
			statusbar_modify_attr(t, XT_COLOR_BLACK, col_str);
#endif
		}
	}
}

void
free_favicon(struct tab *t)
{
	DNPRINTF(XT_D_DOWNLOAD, "%s: down %p req %p\n",
	    __func__, t->icon_download, t->icon_request);

	if (t->icon_request)
		g_object_unref(t->icon_request);
	if (t->icon_dest_uri)
		g_free(t->icon_dest_uri);

	t->icon_request = NULL;
	t->icon_dest_uri = NULL;
}

void
xt_icon_from_name(struct tab *t, gchar *name)
{
	if (!enable_favicon_entry)
		return;

	gtk_entry_set_icon_from_icon_name(GTK_ENTRY(t->uri_entry),
	    GTK_ENTRY_ICON_PRIMARY, "text-html");
	if (show_url == 0)
		gtk_entry_set_icon_from_icon_name(GTK_ENTRY(t->sbe.uri),
		    GTK_ENTRY_ICON_PRIMARY, "text-html");
	else
		gtk_entry_set_icon_from_icon_name(GTK_ENTRY(t->sbe.uri),
		    GTK_ENTRY_ICON_PRIMARY, NULL);
}

void
xt_icon_from_pixbuf(struct tab *t, GdkPixbuf *pb)
{
	GdkPixbuf		*pb_scaled;

	if (gdk_pixbuf_get_width(pb) > 16 || gdk_pixbuf_get_height(pb) > 16)
		pb_scaled = gdk_pixbuf_scale_simple(pb, 16, 16,
		    GDK_INTERP_BILINEAR);
	else
		pb_scaled = pb;

	if (enable_favicon_entry) {

		/* Classic tabs. */
		gtk_entry_set_icon_from_pixbuf(GTK_ENTRY(t->uri_entry),
		    GTK_ENTRY_ICON_PRIMARY, pb_scaled);

		/* Minimal tabs. */
		if (show_url == 0) {
			gtk_entry_set_icon_from_pixbuf(GTK_ENTRY(t->sbe.uri),
			    GTK_ENTRY_ICON_PRIMARY, pb_scaled);
		} else
			gtk_entry_set_icon_from_icon_name(GTK_ENTRY(t->sbe.uri),
			    GTK_ENTRY_ICON_PRIMARY, NULL);
	}

	/* XXX: Only supports the minimal tabs atm. */
	if (enable_favicon_tabs)
		gtk_image_set_from_pixbuf(GTK_IMAGE(t->tab_elems.favicon),
		    pb_scaled);

	if (pb_scaled != pb)
		g_object_unref(pb_scaled);
}

void
xt_icon_from_file(struct tab *t, char *uri)
{
	GdkPixbuf		*pb;
	char			*file;

	if (g_str_has_prefix(uri, "file://"))
		file = g_filename_from_uri(uri, NULL, NULL);
	else
		file = g_strdup(uri);

	if (file == NULL)
		return;

	pb = gdk_pixbuf_new_from_file(file, NULL);
	if (pb) {
		xt_icon_from_pixbuf(t, pb);
		g_object_unref(pb);
	} else
		xt_icon_from_name(t, "text-html");

	g_free(file);
}

gboolean
is_valid_icon(char *file)
{
	gboolean		valid = 0;
	const char		*mime_type;
	GFileInfo		*fi;
	GFile			*gf;

	gf = g_file_new_for_path(file);
	fi = g_file_query_info(gf, G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE, 0,
	    NULL, NULL);
	mime_type = g_file_info_get_content_type(fi);
	valid = g_strcmp0(mime_type, "image/x-ico") == 0 ||
	    g_strcmp0(mime_type, "image/vnd.microsoft.icon") == 0 ||
	    g_strcmp0(mime_type, "image/png") == 0 ||
	    g_strcmp0(mime_type, "image/gif") == 0 ||
	    g_strcmp0(mime_type, "application/octet-stream") == 0;
	g_object_unref(fi);
	g_object_unref(gf);

	return (valid);
}

void
set_favicon_from_file(struct tab *t, char *uri)
{
	struct stat		sb;
	char			*file;

	if (t == NULL || uri == NULL)
		return;

	if (g_str_has_prefix(uri, "file://"))
		file = g_filename_from_uri(uri, NULL, NULL);
	else
		file = g_strdup(uri);

	if (file == NULL)
		return;

	DNPRINTF(XT_D_DOWNLOAD, "%s: loading %s\n", __func__, file);

	if (!stat(file, &sb)) {
		if (sb.st_size == 0 || !is_valid_icon(file)) {
			/* corrupt icon so trash it */
			DNPRINTF(XT_D_DOWNLOAD, "%s: corrupt icon %s\n",
			    __func__, file);
			unlink(file);
			/* no need to set icon to default here */
			goto done;
		}
	}
	xt_icon_from_file(t, file);
done:
	g_free(file);
}

void
favicon_download_status_changed_cb(WebKitDownload *download, GParamSpec *spec,
    WebKitWebView *wv)
{
	WebKitDownloadStatus	status = webkit_download_get_status(download);
	struct tab		*tt = NULL, *t = NULL;

	/*
	 * find the webview instead of passing in the tab as it could have been
	 * deleted from underneath us.
	 */
	TAILQ_FOREACH(tt, &tabs, entry) {
		if (tt->wv == wv) {
			t = tt;
			break;
		}
	}
	if (t == NULL)
		return;

	DNPRINTF(XT_D_DOWNLOAD, "%s: tab %d status %d\n",
	    __func__, t->tab_id, status);

	switch (status) {
	case WEBKIT_DOWNLOAD_STATUS_ERROR:
		/* -1 */
		t->icon_download = NULL;
		free_favicon(t);
		break;
	case WEBKIT_DOWNLOAD_STATUS_CREATED:
		/* 0 */
		break;
	case WEBKIT_DOWNLOAD_STATUS_STARTED:
		/* 1 */
		break;
	case WEBKIT_DOWNLOAD_STATUS_CANCELLED:
		/* 2 */
		DNPRINTF(XT_D_DOWNLOAD, "%s: freeing favicon %d\n",
		    __func__, t->tab_id);
		t->icon_download = NULL;
		free_favicon(t);
		break;
	case WEBKIT_DOWNLOAD_STATUS_FINISHED:
		/* 3 */

		DNPRINTF(XT_D_DOWNLOAD, "%s: setting icon to %s\n",
		    __func__, t->icon_dest_uri);
		set_favicon_from_file(t, t->icon_dest_uri);
		/* these will be freed post callback */
		t->icon_request = NULL;
		t->icon_download = NULL;
		break;
	default:
		break;
	}
}

void
abort_favicon_download(struct tab *t)
{
	DNPRINTF(XT_D_DOWNLOAD, "%s: down %p\n", __func__, t->icon_download);

#if !WEBKIT_CHECK_VERSION(1, 4, 0)
	if (t->icon_download) {
		g_signal_handlers_disconnect_by_func(G_OBJECT(t->icon_download),
		    G_CALLBACK(favicon_download_status_changed_cb), t->wv);
		webkit_download_cancel(t->icon_download);
		t->icon_download = NULL;
	} else
		free_favicon(t);
#endif

	xt_icon_from_name(t, "text-html");
}

void
notify_icon_loaded_cb(WebKitWebView *wv, gchar *uri, struct tab *t)
{
	DNPRINTF(XT_D_DOWNLOAD, "%s %s\n", __func__, uri);

	if (uri == NULL || t == NULL)
		return;

#if WEBKIT_CHECK_VERSION(1, 4, 0)
	/* take icon from WebKitIconDatabase */
	GdkPixbuf		*pb = NULL;

/* webkit_web_view_get_icon_pixbuf is depreciated in 1.8 */
#if WEBKIT_CHECK_VERSION(1, 8, 0)
	/*
	 * If the page was not loaded (for example, via ssl_strict_certs), do
	 * not attempt to get the webview's pixbuf.  This prevents a CRITICAL
	 * glib warning.
	 */
	if (wv && webkit_web_view_get_uri(wv))
		pb = webkit_web_view_try_get_favicon_pixbuf(wv, 0, 0);
#else
	if (wv && webkit_web_view_get_uri(wv))
		pb = webkit_web_view_get_icon_pixbuf(wv);
#endif
	if (pb) {
		xt_icon_from_pixbuf(t, pb);
		g_object_unref(pb);
	} else
		xt_icon_from_name(t, "text-html");
#elif WEBKIT_CHECK_VERSION(1, 1, 18)
	/* download icon to cache dir */
	gchar			*name_hash, file[PATH_MAX];
	struct stat		sb;

	if (t->icon_request) {
		DNPRINTF(XT_D_DOWNLOAD, "%s: download in progress\n", __func__);
		return;
	}

	/* check to see if we got the icon in cache */
	name_hash = g_compute_checksum_for_string(G_CHECKSUM_SHA256, uri, -1);
	snprintf(file, sizeof file, "%s" PS "%s.ico", cache_dir, name_hash);
	g_free(name_hash);

	if (!stat(file, &sb)) {
		if (sb.st_size > 0) {
			DNPRINTF(XT_D_DOWNLOAD, "%s: loading from cache %s\n",
			    __func__, file);
			set_favicon_from_file(t, file);
			return;
		}

		/* corrupt icon so trash it */
		DNPRINTF(XT_D_DOWNLOAD, "%s: corrupt icon %s\n",
		    __func__, file);
		unlink(file);
	}

	/* create download for icon */
	t->icon_request = webkit_network_request_new(uri);
	if (t->icon_request == NULL) {
		DNPRINTF(XT_D_DOWNLOAD, "%s: invalid uri %s\n",
		    __func__, uri);
		return;
	}

	t->icon_download = webkit_download_new(t->icon_request);
	if (t->icon_download == NULL)
		return;

	/* we have to free icon_dest_uri later */
	if ((t->icon_dest_uri = g_filename_to_uri(file, NULL, NULL)) == NULL)
		return;
	webkit_download_set_destination_uri(t->icon_download,
	    t->icon_dest_uri);

	if (webkit_download_get_status(t->icon_download) ==
	    WEBKIT_DOWNLOAD_STATUS_ERROR) {
		g_object_unref(t->icon_request);
		g_free(t->icon_dest_uri);
		t->icon_request = NULL;
		t->icon_dest_uri = NULL;
		return;
	}

	g_signal_connect(G_OBJECT(t->icon_download), "notify::status",
	    G_CALLBACK(favicon_download_status_changed_cb), t->wv);

	webkit_download_start(t->icon_download);
#endif
}

void
notify_load_status_cb(WebKitWebView* wview, GParamSpec* pspec, struct tab *t)
{
	const gchar		*uri = NULL;
	struct history		*h, find;
	struct karg		a;
#if !GTK_CHECK_VERSION(3, 0, 0)
	gchar			*text, *base;
#endif

	DNPRINTF(XT_D_URL, "notify_load_status_cb: %d  %s\n",
	    webkit_web_view_get_load_status(wview),
	    get_uri(t) ? get_uri(t) : "NOTHING");

	if (t == NULL) {
		show_oops(NULL, "notify_load_status_cb invalid parameters");
		return;
	}

	switch (webkit_web_view_get_load_status(wview)) {
	case WEBKIT_LOAD_PROVISIONAL:
		/* 0 */
		abort_favicon_download(t);
#if GTK_CHECK_VERSION(2, 20, 0)
		gtk_widget_show(t->spinner);
		gtk_spinner_start(GTK_SPINNER(t->spinner));
#endif
		t->download_requested = 0;

		gtk_widget_set_sensitive(GTK_WIDGET(t->stop), TRUE);

		/* assume we are a new address */
#if GTK_CHECK_VERSION(3, 0, 0)
		gtk_widget_set_name(t->uri_entry, XT_CSS_NORMAL);
		statusbar_modify_attr(t, XT_CSS_NORMAL);
#else
		text = gdk_color_to_string(
		    &t->default_style->text[GTK_STATE_NORMAL]);
		base = gdk_color_to_string(
		    &t->default_style->base[GTK_STATE_NORMAL]);
		gtk_widget_modify_base(t->uri_entry, GTK_STATE_NORMAL,
		    &t->default_style->base[GTK_STATE_NORMAL]);
		statusbar_modify_attr(t, text, base);
		g_free(text);
		g_free(base);
#endif

		/* DOM is changing, unreference the previous focused element */
#if WEBKIT_CHECK_VERSION(1, 5, 0)
		if (t->active)
			g_object_unref(t->active);
		t->active = NULL;
		if (t->active_text) {
			g_free(t->active_text);
			t->active_text = NULL;
		}
#endif

		/* take focus if we are visible */
		focus_webview(t);
		t->focus_wv = 1;

		marks_clear(t);
#ifdef USE_THREAD
		/* kill color thread */
		t->thread = NULL;
#endif
		break;

	case WEBKIT_LOAD_COMMITTED:
		/* 1 */
		uri = get_uri(t);
		if (uri == NULL)
			return;
		gtk_entry_set_text(GTK_ENTRY(t->uri_entry), uri);

		if (t->status) {
			g_free(t->status);
			t->status = NULL;
		}
		set_status(t, "Loading: %s", (char *)uri);

		/* check if js white listing is enabled */
		if (enable_plugin_whitelist)
			check_and_set_pl(uri, t);
		if (enable_cookie_whitelist)
			check_and_set_cookie(uri, t);
		if (enable_js_whitelist)
			check_and_set_js(uri, t);

		if (t->styled)
			apply_style(t);


		/* we know enough to autosave the session */
		if (session_autosave) {
			a.s = NULL;
			save_tabs(t, &a);
		}

		show_ca_status(t, uri);
		run_script(t, JS_HINTING);
		if (enable_autoscroll)
			run_script(t, JS_AUTOSCROLL);
		break;

	case WEBKIT_LOAD_FIRST_VISUALLY_NON_EMPTY_LAYOUT:
		/* 3 */
		if (color_visited_uris) {
			color_visited(t, color_visited_helper());

			/* This colors the links you middle-click (open in new
			 * tab) in the current tab. */
			if (t->tab_id != gtk_notebook_get_current_page(notebook) &&
			    (uri = get_uri(t)) != NULL)
				color_visited(get_current_tab(),
				    g_strdup_printf("{'%s' : 'dummy'}", uri));
		}
		break;

	case WEBKIT_LOAD_FINISHED:
		/* 2 */
		if ((uri = get_uri(t)) == NULL)
			return;

		if (!strncmp(uri, "http://", strlen("http://")) ||
		    !strncmp(uri, "https://", strlen("https://")) ||
		    !strncmp(uri, "file://", strlen("file://"))) {
			find.uri = (gchar *)uri;
			h = RB_FIND(history_list, &hl, &find);
			if (!h)
				insert_history_item(uri,
				    get_title(t, FALSE), time(NULL));
			else
				h->time = time(NULL);
		}

		if (statusbar_style == XT_STATUSBAR_URL)
			set_status(t, "%s", (char *)uri);
		else
			set_status(t, "%s", (char *)get_title(t, FALSE));
		gtk_widget_set_sensitive(GTK_WIDGET(t->stop), FALSE);
#if GTK_CHECK_VERSION(2, 20, 0)
		gtk_spinner_stop(GTK_SPINNER(t->spinner));
		gtk_widget_hide(t->spinner);
#endif
		break;

#if WEBKIT_CHECK_VERSION(1, 1, 18)
	case WEBKIT_LOAD_FAILED:
		/* 4 */
		if (!t->download_requested) {
			gtk_label_set_text(GTK_LABEL(t->label),
			    get_title(t, FALSE));
			gtk_label_set_text(GTK_LABEL(t->tab_elems.label),
			    get_title(t, FALSE));
			set_status(t, "%s", (char *)get_title(t, FALSE));
			gtk_window_set_title(GTK_WINDOW(main_window),
			    get_title(t, TRUE));
		} else {

		}
#endif
	default:
#if GTK_CHECK_VERSION(2, 20, 0)
		gtk_spinner_stop(GTK_SPINNER(t->spinner));
		gtk_widget_hide(t->spinner);
#endif
		gtk_widget_set_sensitive(GTK_WIDGET(t->stop), FALSE);
		break;
	}

	if (t->item)
		gtk_widget_set_sensitive(GTK_WIDGET(t->backward), TRUE);
	else
		gtk_widget_set_sensitive(GTK_WIDGET(t->backward),
		    can_go_back_for_real(t));

	gtk_widget_set_sensitive(GTK_WIDGET(t->forward),
	    can_go_forward_for_real(t));
}

void
notify_title_cb(WebKitWebView* wview, GParamSpec* pspec, struct tab *t)
{
	const gchar		*title = NULL, *win_title = NULL;

	title = get_title(t, FALSE);
	win_title = get_title(t, TRUE);
	if (title) {
		gtk_label_set_text(GTK_LABEL(t->label), title);
		gtk_label_set_text(GTK_LABEL(t->tab_elems.label), title);
	}

	if (win_title && t->tab_id == gtk_notebook_get_current_page(notebook))
		gtk_window_set_title(GTK_WINDOW(main_window), win_title);
}

char *
get_domain(const gchar *host)
{
	size_t		x;
	char		*p;

	/* handle silly domains like .co.uk */

	if ((x = strlen(host)) <= 6)
		return (g_strdup(host));

	if (host[x - 3] == '.' && host[x - 6] == '.') {
		x = x - 7;
		while (x > 0)
			if (host[x] != '.')
				x--;
			else
				return (g_strdup(&host[x + 1]));
	}

	/* find first . */
	p = g_strrstr(host, ".");
	if (p == NULL)
		return (g_strdup(""));
	p--;
	while (p > host)
		if (*p != '.')
			p--;
		else
			return (g_strdup(p + 1));

	return (g_strdup(host));
}

void
js_autorun(struct tab *t)
{
	SoupURI			*su = NULL;
	const gchar		*uri;
	size_t			got_default = 0, got_host = 0;
	struct stat		sb;
	char			deff[PATH_MAX], hostf[PATH_MAX];
	char			*js = NULL, *jsat, *domain = NULL;
	FILE			*deffile = NULL, *hostfile = NULL;

	if (enable_js_autorun == 0)
		return;

	uri = get_uri(t);
	if (uri &&
	    !(g_str_has_prefix(uri, "http://") ||
	    g_str_has_prefix(uri, "https://")))
		goto done;

	su = soup_uri_new(uri);
	if (su == NULL)
		goto done;
	if (!SOUP_URI_VALID_FOR_HTTP(su))
		goto done;

	DNPRINTF(XT_D_JS, "%s: host: %s domain: %s\n", __func__,
	    su->host, domain);
	domain = get_domain(su->host);

	snprintf(deff, sizeof deff, "%s" PS "default.js", js_dir);
	if ((deffile = fopen(deff, "r")) != NULL) {
		if (fstat(fileno(deffile), &sb) == -1) {
			show_oops(t, "can't stat default JS file");
			goto done;
		}
		got_default = sb.st_size;
	}

	/* try host first followed by domain */
	snprintf(hostf, sizeof hostf, "%s" PS "%s.js", js_dir, su->host);
	DNPRINTF(XT_D_JS, "trying file: %s\n", hostf);
	if ((hostfile = fopen(hostf, "r")) == NULL) {
		snprintf(hostf, sizeof hostf, "%s" PS "%s.js", js_dir, domain);
		DNPRINTF(XT_D_JS, "trying file: %s\n", hostf);
		if ((hostfile = fopen(hostf, "r")) == NULL)
			goto nofile;
	}
	DNPRINTF(XT_D_JS, "file: %s\n", hostf);
	if (fstat(fileno(hostfile), &sb) == -1) {
		show_oops(t, "can't stat %s JS file", hostf);
		goto done;
	}
	got_host = sb.st_size;

nofile:
	if (got_default + got_host == 0)
		goto done;

	js = g_malloc0(got_default + got_host + 1);
	jsat = js;

	if (got_default) {
		if (fread(js, got_default, 1, deffile) != 1) {
			show_oops(t, "default file read error");
			goto done;
		}
		jsat = js + got_default;
	}

	if (got_host) {
		if (fread(jsat, got_host, 1, hostfile) != 1) {
			show_oops(t, "host file read error");
			goto done;
		}
	}

	DNPRINTF(XT_D_JS, "%s: about to run script\n", __func__);
	run_script_locked(t, js);

done:
	if (su)
		soup_uri_free(su);
	if (js)
		g_free(js);
	if (deffile)
		fclose(deffile);
	if (hostfile)
		fclose(hostfile);
	if (domain)
		g_free(domain);
}

void
webview_load_finished_cb(WebKitWebView *wv, WebKitWebFrame *wf, struct tab *t)
{
	/* autorun some js if enabled */
	js_autorun(t);

	input_autofocus(t);

}

void
webview_progress_changed_cb(WebKitWebView *wv, int progress, struct tab *t)
{
	gtk_entry_set_progress_fraction(GTK_ENTRY(t->sbe.uri),
	    progress == 100 ? 0 : (double)progress / 100);
	gtk_entry_set_progress_fraction(GTK_ENTRY(t->uri_entry),
	    progress == 100 ? 0 : (double)progress / 100);

	update_statusbar_position(NULL, NULL);
}

int
strict_transport_rb_cmp(struct strict_transport *a, struct strict_transport *b)
{
	char			*p1, *p2;
	int			l1, l2;

	/* compare strings from the end */
	l1 = strlen(a->host);
	l2 = strlen(b->host);

	p1 = a->host + l1;
	p2 = b->host + l2;
	for (; *p1 == *p2 && p1 > a->host && p2 > b->host;
	    p1--, p2--)
		;

	/*
	 * Check if we need to do pattern expansion,
	 * or if we're just keeping the tree in order
	 */
	if (a->flags & XT_STS_FLAGS_EXPAND &&
	    b->flags & XT_STS_FLAGS_INCLUDE_SUBDOMAINS) {
		/* Check if we're matching the
		 * 'host.xyz' part in '*.host.xyz'
		 */
		if (p2 == b->host && (p1 == a->host || *(p1-1) == '.')) {
			return (0);
		}
	}

	if (p1 == a->host && p2 == b->host)
		return (0);
	if (p1 == a->host)
		return (1);
	if (p2 == b->host)
		return (-1);

	if (*p1 < *p2)
		return (-1);
	if (*p1 > *p2)
		return (1);

	return (0);
}
RB_GENERATE(strict_transport_tree, strict_transport, entry,
    strict_transport_rb_cmp);

int
strict_transport_add(const char *domain, time_t timeout, int subdomains)
{
	struct strict_transport	*d, find;
	time_t			now;
	FILE			*f;

	if (enable_strict_transport == FALSE)
		return (0);

	DPRINTF("strict_transport_add(%s,%" PRIi64 ",%d)\n", domain,
	    (uint64_t)timeout, subdomains);

	now = time(NULL);
	if (timeout < now)
		return (0);

	find.host = (char *)domain;
	find.flags = 0;
	d = RB_FIND(strict_transport_tree, &st_tree, &find);

	/* update flags */
	if (d) {
		/* check if update is needed */
		if (d->timeout == timeout &&
		    (d->flags & XT_STS_FLAGS_INCLUDE_SUBDOMAINS) == subdomains)
			return (0);

		d->timeout = timeout;
		if (subdomains)
			d->flags |= XT_STS_FLAGS_INCLUDE_SUBDOMAINS;

		/* We're still initializing */
		if (strict_transport_file == NULL)
			return (0);

		if ((f = fopen(strict_transport_file, "w")) == NULL) {
			show_oops(NULL,
			    "can't open strict-transport rules file");
			return (1);
		}

		fprintf(f, "# Generated file - do not update unless you know "
		    "what you're doing\n");
		RB_FOREACH(d, strict_transport_tree, &st_tree) {
			if (d->timeout < now)
				continue;
			fprintf(f, "%s\t%" PRIi64 "\t%d\n", d->host,
			    (uint64_t)d->timeout,
			    d->flags & XT_STS_FLAGS_INCLUDE_SUBDOMAINS);
		}
		fclose(f);
	} else {
		d = g_malloc(sizeof *d);
		d->host= g_strdup(domain);
		d->timeout = timeout;
		if (subdomains)
			d->flags = XT_STS_FLAGS_INCLUDE_SUBDOMAINS;
		else
			d->flags = 0;
		RB_INSERT(strict_transport_tree, &st_tree, d);

		/* We're still initializing */
		if (strict_transport_file == NULL)
			return (0);

		if ((f = fopen(strict_transport_file, "a+")) == NULL) {
			show_oops(NULL,
			    "can't open strict-transport rules file");
			return (1);
		}

		fseek(f, 0, SEEK_END);
		fprintf(f,"%s\t%" PRIi64 "\t%d\n", d->host, (uint64_t)timeout,
		    subdomains);
		fclose(f);
	}
	return (0);
}

int
strict_transport_check(const char *host)
{
	static struct strict_transport	*d = NULL;
	struct strict_transport		find;

	if (enable_strict_transport == FALSE)
		return (0);

	find.host = (char *)host;

	/* match for domains that include subdomains */
	find.flags = XT_STS_FLAGS_EXPAND;

	/* First, check if we're already at the right node */
	if (d != NULL && strict_transport_rb_cmp(&find, d) == 0) {
		return (1);
	}

	d = RB_FIND(strict_transport_tree, &st_tree, &find);
	if (d != NULL)
		return (1);

	return (0);
}

int
strict_transport_init()
{
	char			file[PATH_MAX];
	char			delim[3];
	char			*rule;
	char			*ptr;
	FILE			*f;
	size_t			len;
	time_t			timeout, now;
	int			subdomains;

	snprintf(file, sizeof file, "%s" PS "%s", work_dir, XT_STS_FILE);
	if ((f = fopen(file, "r")) == NULL) {
		strict_transport_file = g_strdup(file);
		return (0);
	}

	delim[0] = '\\';
	delim[1] = '\\';
	delim[2] = '#';
	rule = NULL;
	now = time(NULL);

	for (;;) {
		if ((rule = fparseln(f, &len, NULL, delim, 0)) == NULL) {
			if (!feof(f) || ferror(f))
				goto corrupt_file;
			else
				break;
		}

		/* get second entry */
		if ((ptr = strpbrk(rule, " \t")) == NULL)
			goto corrupt_file;

		*ptr++ = '\0';
		timeout = atoi(ptr);

		/* get third entry */
		if ((ptr = strpbrk(ptr, " \t")) == NULL)
			goto corrupt_file;

		ptr ++;
		subdomains = atoi(ptr);

		if (timeout > now)
			strict_transport_add(rule, timeout, subdomains);
		free(rule);
	}

	fclose(f);
	strict_transport_file = g_strdup(file);
	return (0);

corrupt_file:
	startpage_add("strict-transport rules file ('%s') is corrupt", file);
	if (rule)
		free(rule);
	fclose(f);
	return (1);
}

int
force_https_check(const char *uri)
{
	struct domain		*d = NULL;

	if (uri == NULL)
		return (0);

	if ((d = wl_find_uri(uri, &force_https)) == NULL)
		return (0);
	else
		return (1);
}

void
strict_transport_security_cb(SoupMessage *msg, gpointer data)
{
	SoupURI		*uri;
	const char	*sts;
	char		*ptr;
	time_t		timeout = 0;
	int		subdomains = FALSE;

	if (msg == NULL)
		return;

	sts = soup_message_headers_get_one(msg->response_headers,
	    "Strict-Transport-Security");
	uri = soup_message_get_uri(msg);

	if (sts == NULL || uri == NULL)
		return;

	if ((ptr = strcasestr(sts, "max-age="))) {
		ptr += strlen("max-age=");
		timeout = atoll(ptr);
	} else
		return; /* malformed header - max-age must be included */

	if ((ptr = strcasestr(sts, "includeSubDomains")))
		subdomains = TRUE;

	strict_transport_add(uri->host, timeout + time(NULL), subdomains);
}

void
session_rq_cb(SoupSession *s, SoupMessage *msg, SoupSocket *socket,
    gpointer data)
{
	SoupURI			*dest;
	SoupURI			*ref_uri;
	const char		*ref;

	char			*ref_suffix;
	char			*dest_suffix;

	if (s == NULL || msg == NULL)
		return;

	if (enable_strict_transport) {
		soup_message_add_header_handler(msg, "finished",
		    "Strict-Transport-Security",
		    G_CALLBACK(strict_transport_security_cb), NULL);
	}

	if (referer_mode == XT_REFERER_ALWAYS)
		return;

	/* Check if referer is set - and what the user requested for referers */
	ref = soup_message_headers_get_one(msg->request_headers, "Referer");
	if (ref) {
		DNPRINTF(XT_D_NAV, "session_rq_cb: Referer: %s\n", ref);
		switch (referer_mode) {
		case XT_REFERER_NEVER:
			DNPRINTF(XT_D_NAV, "session_rq_cb: removing referer\n");
			soup_message_headers_remove(msg->request_headers,
			    "Referer");
			break;
		case XT_REFERER_SAME_DOMAIN:
			ref_uri = soup_uri_new(ref);
			dest = soup_message_get_uri(msg);

			ref_suffix = tld_get_suffix(ref_uri->host);
			dest_suffix = tld_get_suffix(dest->host);

			if (dest && ref_suffix && dest_suffix &&
			    strcmp(ref_suffix, dest_suffix) != 0) {
				soup_message_headers_remove(msg->request_headers,
				    "Referer");
				DNPRINTF(XT_D_NAV, "session_rq_cb: removing "
				    "referer (not same domain) (suffixes: %s - %s)\n",
				    ref_suffix, dest_suffix);
			}
			soup_uri_free(ref_uri);
			break;
		case XT_REFERER_SAME_FQDN:
			ref_uri = soup_uri_new(ref);
			dest = soup_message_get_uri(msg);
			if (dest && strcmp(ref_uri->host, dest->host) != 0) {
				soup_message_headers_remove(msg->request_headers,
				    "Referer");
				DNPRINTF(XT_D_NAV, "session_rq_cb: removing "
				    "referer (not same fqdn) (should be %s)\n",
				    dest->host);
			}
			soup_uri_free(ref_uri);
			break;
		case XT_REFERER_CUSTOM:
			DNPRINTF(XT_D_NAV, "session_rq_cb: setting referer "
			    "to %s\n", referer_custom);
			soup_message_headers_replace(msg->request_headers,
			    "Referer", referer_custom);
			break;
		}
	}
}

int
webview_npd_cb(WebKitWebView *wv, WebKitWebFrame *wf,
    WebKitNetworkRequest *request, WebKitWebNavigationAction *na,
    WebKitWebPolicyDecision *pd, struct tab *t)
{
	WebKitWebNavigationReason	reason;
	struct user_agent		ua_find, *ua;
	struct domain			*d = NULL;
	char				*uri;

	if (t == NULL) {
		show_oops(NULL, "webview_npd_cb invalid parameters");
		return (FALSE);
	}

	DNPRINTF(XT_D_NAV, "webview_npd_cb: ctrl_click %d %s\n",
	    t->ctrl_click,
	    webkit_network_request_get_uri(request));

	uri = (char *)webkit_network_request_get_uri(request);

	if (!auto_load_images && t->load_images) {

		/* Disable autoloading of images, now that we're done loading
		 * them. */
		g_object_set(G_OBJECT(t->settings),
		    "auto-load-images", FALSE, (char *)NULL);
		webkit_web_view_set_settings(t->wv, t->settings);

		t->load_images = FALSE;
	}

	/* If this is an xtp url, we don't load anything else. */
	if (parse_xtp_url(t, uri))
		return (TRUE);

	if (parse_custom_uri(t, uri))
		return (TRUE);

	if ((t->mode == XT_MODE_HINT && t->new_tab) || t->ctrl_click) {
		t->ctrl_click = 0;
		create_new_tab(uri, NULL, ctrl_click_focus, -1);
		webkit_web_policy_decision_ignore(pd);
		return (TRUE); /* we made the decission */
	}

	/* Change user agent if more than one has been given. */
	if (!RB_EMPTY(&ua_list)) {
		ua_find.id = t->user_agent_id;

		if ((ua = RB_FIND(user_agent_list, &ua_list, &ua_find)) == NULL) {
			ua_find.id = 0;
			t->user_agent_id = 1;
			user_agent = RB_FIND(user_agent_list, &ua_list, &ua_find);
		} else {
			++t->user_agent_id;
			user_agent = ua;
		}

		g_free(t->user_agent);
		t->user_agent = g_strdup(user_agent->value);

		DNPRINTF(XT_D_NAV, "user-agent: %s\n", t->user_agent);

		g_object_set(G_OBJECT(t->settings),
			"user-agent", t->user_agent, (char *)NULL);

		webkit_web_view_set_settings(wv, t->settings);
	}

	/*
	 * This is a little hairy but it comes down to this:
	 * when we run in whitelist mode we have to assist the browser in
	 * opening the URL that it would have opened in a new tab.
	 */
	reason = webkit_web_navigation_action_get_reason(na);
	if (reason == WEBKIT_WEB_NAVIGATION_REASON_LINK_CLICKED) {
		set_normal_tab_meaning(t);
		if (enable_scripts == 0 && enable_cookie_whitelist == 1)
			if (uri && (d = wl_find_uri(uri, &js_wl)) == NULL)
				load_uri(t, uri);
		webkit_web_policy_decision_use(pd);
		return (TRUE); /* we made the decision */
	}

	return (FALSE);
}

void
webview_rrs_cb(WebKitWebView *wv, WebKitWebFrame *wf, WebKitWebResource *res,
    WebKitNetworkRequest *request, WebKitNetworkResponse *response,
    struct tab *t)
{
	SoupMessage		*msg = NULL;
	SoupURI			*uri = NULL;
	struct http_accept	ha_find, *ha = NULL;
	const char		*accept = NULL;
	char			*uri_s = NULL;

	msg = webkit_network_request_get_message(request);
	if (!msg) return;

	uri = soup_message_get_uri(msg);
	if (!uri)
		return;
	uri_s = soup_uri_to_string(uri, FALSE);

	if (strcmp(uri->scheme, SOUP_URI_SCHEME_HTTP) == 0) {
		if (strict_transport_check(uri->host) ||
		    force_https_check(uri_s)) {
			DNPRINTF(XT_D_NAV, "webview_rrs_cb: force https for %s\n",
					uri->host);
			soup_uri_set_scheme(uri, SOUP_URI_SCHEME_HTTPS);
		}
	}

	if (do_not_track)
		soup_message_headers_append(msg->request_headers, "DNT", "1");

	/* Round-robin through HTTP Accept headers if any have been set */
	if (!RB_EMPTY(&ha_list)) {
		accept = soup_message_headers_get_list(msg->request_headers,
		    "Accept");
		if (accept == NULL ||
		    strncmp(accept, "text/html", strlen("text/html")))
			goto done;

		ha_find.id = t->http_accept_id;
		ha = RB_FIND(http_accept_list, &ha_list, &ha_find);
		if (ha == NULL) {
			ha_find.id = 0;
			t->http_accept_id = 1;
			http_accept = RB_FIND(http_accept_list, &ha_list,
			    &ha_find);
		} else {
			++t->http_accept_id;
			http_accept = ha;
		}

		soup_message_headers_replace(msg->request_headers, "Accept",
		    http_accept->value);
	}
done:
	if (uri_s)
		g_free(uri_s);
}

WebKitWebView *
webview_cwv_cb(WebKitWebView *wv, WebKitWebFrame *wf, struct tab *t)
{
	struct tab		*tt;
	struct domain		*d = NULL;
	const gchar		*uri;
	WebKitWebView		*webview = NULL;
	int			x = 1;

	DNPRINTF(XT_D_NAV, "webview_cwv_cb: %s\n",
	    webkit_web_view_get_uri(wv));

	if (tabless) {
		/* open in current tab */
		webview = t->wv;
	} else if (enable_scripts == 0 && enable_js_whitelist == 1) {
		uri = webkit_web_view_get_uri(wv);
		if (uri && (d = wl_find_uri(uri, &js_wl)) == NULL)
			return (NULL);

		if (t->ctrl_click) {
			x = ctrl_click_focus;
			t->ctrl_click = 0;
		}
		tt = create_new_tab(NULL, NULL, x, -1);
		webview = tt->wv;
	} else if (enable_scripts == 1) {
		if (t->ctrl_click) {
			x = ctrl_click_focus;
			t->ctrl_click = 0;
		}
		tt = create_new_tab(NULL, NULL, x, -1);
		webview = tt->wv;
	}

	return (webview);
}

gboolean
webview_closewv_cb(WebKitWebView *wv, struct tab *t)
{
	const gchar		*uri;
	struct domain		*d = NULL;

	DNPRINTF(XT_D_NAV, "webview_close_cb: %d\n", t->tab_id);

	if (enable_scripts == 0 && enable_cookie_whitelist == 1) {
		uri = webkit_web_view_get_uri(wv);
		if (uri && (d = wl_find_uri(uri, &js_wl)) == NULL)
			return (FALSE);

		delete_tab(t);
	} else if (enable_scripts == 1)
		delete_tab(t);

	return (TRUE);
}

int
webview_event_cb(GtkWidget *w, GdkEventButton *e, struct tab *t)
{
	/* we can not eat the event without throwing gtk off so defer it */

	/* catch middle click */
	if (e->type == GDK_BUTTON_RELEASE && e->button == 2) {
		t->ctrl_click = 1;
		goto done;
	}

	/* catch ctrl click */
	if (e->type == GDK_BUTTON_RELEASE &&
	    CLEAN(e->state) == GDK_CONTROL_MASK)
		t->ctrl_click = 1;
	else
		t->ctrl_click = 0;
done:
	return (XT_CB_PASSTHROUGH);
}

int
run_mimehandler(struct tab *t, char *mime_type, WebKitNetworkRequest *request)
{
	struct mime_type	*m;
	char			*sv[3];
	GError			*gerr = NULL;

	m = find_mime_type(mime_type);
	if (m == NULL)
		return (1);
	if (m->mt_download)
		return (1);

	sv[0] = m->mt_action;
	sv[1] = (char *)webkit_network_request_get_uri(request);
	sv[2] = NULL;

	/* ignore donothing from example config */
	if (m->mt_action && !strcmp(m->mt_action, "donothing"))
		return (0);

	if (!g_spawn_async(NULL, sv, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL,
	    NULL, &gerr))
		show_oops(t, "%s: could not spawn process (%s)", __func__,
		    gerr ? gerr->message : "N/A");
	return (0);
}

char *
get_mime_type(const char *uri)
{
	GFileInfo		*fi;
	GFile			*gf;
	const gchar		*m;
	char			*mime_type = NULL;
	char			*file;

	if (uri == NULL) {
		show_oops(NULL, "%s: invalid parameters", __func__);
		return (NULL);
	}

	if (g_str_has_prefix(uri, "file://"))
		file = g_filename_from_uri(uri, NULL, NULL);
	else
		file = g_strdup(uri);

	if (file == NULL)
		return (NULL);

	gf = g_file_new_for_path(file);
	fi = g_file_query_info(gf, G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE, 0,
	    NULL, NULL);
	if ((m = g_file_info_get_content_type(fi)) != NULL)
		mime_type = g_strdup(m);
	g_object_unref(fi);
	g_object_unref(gf);
	g_free(file);

	return (mime_type);
}

int
run_download_mimehandler(char *mime_type, char *file)
{
	struct mime_type	*m;
	char			*sv[3];

	m = find_mime_type(mime_type);
	if (m == NULL)
		return (1);

	sv[0] = m->mt_action;
	sv[1] = file;
	sv[2] = NULL;
	if (!g_spawn_async(NULL, sv, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL,
	    NULL, NULL)) {
		show_oops(NULL, "%s: could not spawn process: %s %s", __func__,
		    sv[0], sv[1]);
		return (1);
	}
	return (0);
}

void
download_status_changed_cb(WebKitDownload *download, GParamSpec *spec,
    WebKitWebView *wv)
{
	WebKitDownloadStatus	status;
	const char		*uri;
	char			*file = NULL;
	char			*mime = NULL;

	if (download == NULL)
		return;
	status = webkit_download_get_status(download);
	if (status != WEBKIT_DOWNLOAD_STATUS_FINISHED)
		return;

	if (download_notifications)
		show_oops(NULL, "Download of '%s' finished",
		    basename((char *)webkit_download_get_destination_uri(download)));
	uri = webkit_download_get_destination_uri(download);
	if (uri == NULL)
		return;
	mime = get_mime_type(uri);
	if (mime == NULL)
		return;

	if (g_str_has_prefix(uri, "file://"))
		file = g_filename_from_uri(uri, NULL, NULL);
	else
		file = g_strdup(uri);

	if (file == NULL)
		goto done;

	run_download_mimehandler((char *)mime, file);

done:
	g_free(mime);
}

int
webview_mimetype_cb(WebKitWebView *wv, WebKitWebFrame *frame,
    WebKitNetworkRequest *request, char *mime_type,
    WebKitWebPolicyDecision *decision, struct tab *t)
{
	if (t == NULL) {
		show_oops(NULL, "webview_mimetype_cb invalid parameters");
		return (FALSE);
	}

	DNPRINTF(XT_D_DOWNLOAD, "webview_mimetype_cb: tab %d mime %s\n",
	    t->tab_id, mime_type);

	if (run_mimehandler(t, mime_type, request) == 0) {
		webkit_web_policy_decision_ignore(decision);
		focus_webview(t);
		return (TRUE);
	}

	if (webkit_web_view_can_show_mime_type(wv, mime_type) == FALSE) {
		webkit_web_policy_decision_download(decision);
		return (TRUE);
	}

	return (FALSE);
}

int
download_start(struct tab *t, struct download *d, int flag)
{
	WebKitNetworkRequest	*req;
	struct stat		sb;
	const gchar		*suggested_name;
	gchar			*filename = NULL;
	char			*uri = NULL;
	char			*path = NULL;
	int			ret = TRUE;
	int			i;

	if (d == NULL || t == NULL) {
		show_oops(NULL, "%s invalid parameters", __func__);
		return (FALSE);
	}

	suggested_name = webkit_download_get_suggested_filename(d->download);
	if (suggested_name == NULL)
		return (FALSE); /* abort download */

	i = 0;
	do {
		if (filename) {
			g_free(filename);
			filename = NULL;
		}
		if (i) {
			g_free(uri);
			uri = NULL;
			filename = g_strdup_printf("%d%s", i, suggested_name);
		}
#ifdef __MINGW32__
		/* XXX using urls doesn't work properly in windows? */
		uri = g_strdup_printf("%s\\%s", download_dir, i ?
		    filename : suggested_name);
#else
		path = g_strdup_printf("%s" PS "%s", download_dir, i ?
		    filename : suggested_name);
		if ((uri = g_filename_to_uri(path, NULL, NULL)) == NULL)
			break;	/* XXX */
#endif
		i++;
#ifdef __MINGW32__
	} while (!stat(uri, &sb));
#else
	} while (!stat(path, &sb));
#endif

	DNPRINTF(XT_D_DOWNLOAD, "%s: tab %d filename %s "
	    "local %s\n", __func__, t->tab_id, filename, uri);

	/* if we're restarting the download, or starting
	 * it after doing something else, we need to recreate
	 * the download request.
	 */
	if (flag == XT_DL_RESTART) {
		req = webkit_network_request_new(webkit_download_get_uri(d->download));
		webkit_download_cancel(d->download);
		g_object_unref(d->download);
		d->download = webkit_download_new(req);
	}

	webkit_download_set_destination_uri(d->download, uri);

	if (webkit_download_get_status(d->download) ==
	    WEBKIT_DOWNLOAD_STATUS_ERROR) {
		show_oops(t, "%s: download failed to start", __func__);
		ret = FALSE;
		show_oops(t, "Download Failed");
	} else {
		/* connect "download first" mime handler */
		g_signal_connect(G_OBJECT(d->download), "notify::status",
		    G_CALLBACK(download_status_changed_cb), NULL);

		/* get from history */
		g_object_ref(d->download);
		show_oops(t, "Download of '%s' started...",
		    basename((char *)webkit_download_get_destination_uri(d->download)));
	}

	if (flag != XT_DL_START)
		webkit_download_start(d->download);

	DNPRINTF(XT_D_DOWNLOAD, "download status : %d",
	    webkit_download_get_status(d->download));

	/* sync other download manager tabs */
	update_download_tabs(NULL);

	if (uri)
		g_free(uri);
	if (path)
		g_free(path);
	if (filename)
		g_free(filename);

	return (ret);
}

int
download_ask_cb(struct tab *t, GdkEventKey *e, gpointer data)
{
	struct download *d = data;

	/* unset mode */
	t->mode_cb = NULL;
	t->mode_cb_data = NULL;

	if (!e || !d) {
		e->keyval = GDK_Escape;
		return (XT_CB_PASSTHROUGH);
	}

	DPRINTF("download_ask_cb: User pressed %c\n", e->keyval);
	if (e->keyval == 'y' || e->keyval == 'Y' || e->keyval == GDK_Return)
		/* We need to do a RESTART, because we're not calling from
		 * webview_download_cb
		 */
		download_start(t, d, XT_DL_RESTART);

	/* for all other keyvals, we just let the download be */
	e->keyval = GDK_Escape;
	return (XT_CB_HANDLED);
}

int
download_ask(struct tab *t, struct download *d)
{
	const gchar		*suggested_name;

	suggested_name = webkit_download_get_suggested_filename(d->download);
	if (suggested_name == NULL)
		return (FALSE); /* abort download */

	show_oops(t, "download file %s [y/n] ?", suggested_name);
	t->mode_cb = download_ask_cb;
	t->mode_cb_data = d;

	return (TRUE);
}

int
webview_download_cb(WebKitWebView *wv, WebKitDownload *wk_download,
    struct tab *t)
{
	const gchar		*suggested_name;
	struct download		*download_entry;
	int			ret = TRUE;

	if (wk_download == NULL || t == NULL) {
		show_oops(NULL, "%s invalid parameters", __func__);
		return (FALSE);
	}

	suggested_name = webkit_download_get_suggested_filename(wk_download);
	if (suggested_name == NULL)
		return (FALSE); /* abort download */

	download_entry = g_malloc(sizeof(struct download));
	download_entry->download = wk_download;
	download_entry->tab = t;
	download_entry->id = next_download_id++;
	RB_INSERT(download_list, &downloads, download_entry);
	t->download_requested = 1;

	if (download_mode == XT_DM_START)
		ret = download_start(t, download_entry, XT_DL_START);
	else if (download_mode == XT_DM_ASK)
		ret = download_ask(t, download_entry);
	else if (download_mode == XT_DM_ADD)
		show_oops(t, "added %s to download manager",
		    suggested_name);

	/* sync other download manager tabs */
	update_download_tabs(NULL);

	/*
	 * NOTE: never redirect/render the current tab before this
	 * function returns. This will cause the download to never start.
	 */
	return (ret); /* start download */
}

void
webview_hover_cb(WebKitWebView *wv, gchar *title, gchar *uri, struct tab *t)
{
	DNPRINTF(XT_D_KEY, "webview_hover_cb: %s %s\n", title, uri);

	if (t == NULL) {
		show_oops(NULL, "webview_hover_cb");
		return;
	}

	if (uri)
		set_status(t, "Link: %s", uri);
	else {
		if (statusbar_style == XT_STATUSBAR_URL) {
			const gchar *page_uri;

			if ((page_uri = get_uri(t)) != NULL)
				set_status(t, "%s", page_uri);
		} else
			set_status(t, "%s", (char *)get_title(t, FALSE));
	}
}

int
mark(struct tab *t, struct karg *arg)
{
	char                    mark;
	int                     index;
	double			pos;

	mark = arg->s[1];
	if ((index = marktoindex(mark)) == -1)
		return (-1);

	if (arg->i == XT_MARK_SET)
		t->mark[index] = gtk_adjustment_get_value(t->adjust_v);
	else if (arg->i == XT_MARK_GOTO) {
		if (t->mark[index] == XT_INVALID_MARK) {
			show_oops(t, "mark '%c' does not exist", mark);
			return (-1);
		}
		/* XXX t->mark[index] can be bigger than the maximum if ajax or
		   something changes the document size */
		pos = gtk_adjustment_get_value(t->adjust_v);
		gtk_adjustment_set_value(t->adjust_v, t->mark[index]);
		t->mark[marktoindex('\'')] = pos;
	}

	return (0);
}

void
marks_clear(struct tab *t)
{
	int			i;

	for (i = 0; i < LENGTH(t->mark); i++)
		t->mark[i] = XT_INVALID_MARK;
}

int
qmarks_load(void)
{
	char			file[PATH_MAX];
	char			*line = NULL, *p;
	int			index, i;
	FILE			*f;
	size_t			linelen;

	snprintf(file, sizeof file, "%s" PS "%s", work_dir, XT_QMARKS_FILE);
	if ((f = fopen(file, "r+")) == NULL) {
		show_oops(NULL, "Can't open quickmarks file: %s", strerror(errno));
		return (1);
	}

	for (i = 1; ; i++) {
		if ((line = fparseln(f, &linelen, NULL, NULL, 0)) == NULL)
			break;
		if (strlen(line) == 0 || line[0] == '#') {
			free(line);
			line = NULL;
			continue;
		}

		p = strtok(line, " \t");

		if (p == NULL || strlen(p) != 1 ||
		    (index = qmarktoindex(*p)) == -1) {
			warnx("corrupt quickmarks file, line %d", i);
			break;
		}

		p = strtok(NULL, " \t");
		if (qmarks[index] != NULL)
			g_free(qmarks[index]);
		qmarks[index] = g_strdup(p);
	}

	fclose(f);

	return (0);
}

int
qmarks_save(void)
{
	char			 file[PATH_MAX];
	int			 i;
	FILE			*f;

	snprintf(file, sizeof file, "%s" PS "%s", work_dir, XT_QMARKS_FILE);
	if ((f = fopen(file, "r+")) == NULL) {
		show_oops(NULL, "Can't open quickmarks file: %s", strerror(errno));
		return (1);
	}

	for (i = 0; i < XT_NOQMARKS; i++)
		if (qmarks[i] != NULL)
			fprintf(f, "%c %s\n", indextoqmark(i), qmarks[i]);

	fclose(f);

	return (0);
}

int
qmark(struct tab *t, struct karg *arg)
{
	char		mark;
	int		index;

	mark = arg->s[strlen(arg->s)-1];
	index = qmarktoindex(mark);
	if (index == -1)
		return (-1);

	switch (arg->i) {
	case XT_QMARK_SET:
		if (qmarks[index] != NULL) {
			g_free(qmarks[index]);
			qmarks[index] = NULL;
		}

		qmarks_load(); /* sync if multiple instances */
		qmarks[index] = g_strdup(get_uri(t));
		qmarks_save();
		break;
	case XT_QMARK_OPEN:
		if (qmarks[index] != NULL)
			load_uri(t, qmarks[index]);
		else {
			show_oops(t, "quickmark \"%c\" does not exist",
			    mark);
			return (-1);
		}
		break;
	case XT_QMARK_TAB:
		if (qmarks[index] != NULL)
			create_new_tab(qmarks[index], NULL, 1, -1);
		else {
			show_oops(t, "quickmark \"%c\" does not exist",
			    mark);
			return (-1);
		}
		break;
	}

	return (0);
}

int
go_up(struct tab *t, struct karg *args)
{
	int		levels;
	int		lastidx;
	char		*uri;
	char		*tmp;
	char		*p;

	if (args->i == XT_GO_UP_ROOT)
		levels = XT_GO_UP_ROOT;
	else if ((levels = atoi(args->s)) == 0)
		levels = 1;

	uri = g_strdup(get_uri(t));
	if (uri == NULL)
		return (1);

	if ((tmp = strstr(uri, XT_PROTO_DELIM)) == NULL)
		return (1);

	tmp += strlen(XT_PROTO_DELIM);

	/* it makes no sense to strip the last slash from ".../dir/", skip it */
	lastidx = strlen(tmp) - 1;
	if (lastidx > 0) {
		if (tmp[lastidx] == '/')
			tmp[lastidx] = '\0';
	}

	while (levels--) {
		p = strrchr(tmp, '/');
		if (p == tmp) { /* Are we at the root of a file://-path? */
			p[1] = '\0';
			break;
		} else if (p != NULL)
			*p = '\0';
		else
			break;
	}

	load_uri(t, uri);
	g_free(uri);

	return (0);
}

int
gototab(struct tab *t, struct karg *args)
{
	int		tab;
	struct karg	arg = {0, NULL, -1};

	tab = atoi(args->s);

	if (args->i == 0)
		arg.i = XT_TAB_NEXT;
	else
		arg.i = args->i;

	arg.precount = tab;

	movetab(t, &arg);

	return (0);
}

int
zoom_amount(struct tab *t, struct karg *arg)
{
	struct karg	narg = {0, NULL, -1};

	narg.i = atoi(arg->s);
	resizetab(t, &narg);

	return (0);
}

int
flip_colon(struct tab *t, struct karg *arg)
{
	struct karg	narg = {0, NULL, -1};
	char		*p;

	if (t == NULL || arg == NULL)
		return (1);

	p = strstr(arg->s, ":");
	if (p == NULL)
		return (1);
	*p = '\0';

	narg.i = ':';
	narg.s = arg->s;
	command(t, &narg);

	return (0);
}

/* buffer commands receive the regex that triggered them in arg.s */
char bcmd[XT_BUFCMD_SZ];
struct buffercmd {
	char		*regex;
	int		precount;
#define XT_PRE_NO	(0)
#define XT_PRE_YES	(1)
#define XT_PRE_MAYBE	(2)
	char		*cmd;
	int		(*func)(struct tab *, struct karg *);
	int		arg;
	regex_t		cregex;
} buffercmds[] = {
	{ "^[0-9]*gu$",		XT_PRE_MAYBE,	"gu",	go_up,		0 },
	{ "^gU$",		XT_PRE_NO,	"gU",	go_up,		XT_GO_UP_ROOT },
	{ "^gg$",		XT_PRE_NO,	"gg",	move,		XT_MOVE_TOP },
	{ "^gG$",		XT_PRE_NO,	"gG",	move,		XT_MOVE_BOTTOM },
	{ "^[0-9]+%$",		XT_PRE_YES,	"%",	move,		XT_MOVE_PERCENT },
	{ "^zz$",		XT_PRE_NO,	"zz",	move,		XT_MOVE_CENTER },
	{ "^gh$",		XT_PRE_NO,	"gh",	go_home,	0 },
	{ "^m[a-zA-Z0-9]$",	XT_PRE_NO,	"m",	mark,		XT_MARK_SET },
	{ "^['][a-zA-Z0-9']$",	XT_PRE_NO,	"'",	mark,		XT_MARK_GOTO },
	{ "^[0-9]+t$",		XT_PRE_YES,	"t",	gototab,	0 },
	{ "^g0$",		XT_PRE_YES,	"g0",	movetab,	XT_TAB_FIRST },
	{ "^g[$]$",		XT_PRE_YES,	"g$",	movetab,	XT_TAB_LAST },
	{ "^[0-9]*gt$",		XT_PRE_YES,	"t",	movetab,	XT_TAB_NEXT },
	{ "^[0-9]*gT$",		XT_PRE_YES,	"T",	movetab,	XT_TAB_PREV },
	{ "^M[a-zA-Z0-9]$",	XT_PRE_NO,	"M",	qmark,		XT_QMARK_SET },
	{ "^go[a-zA-Z0-9]$",	XT_PRE_NO,	"go",	qmark,		XT_QMARK_OPEN },
	{ "^gn[a-zA-Z0-9]$",	XT_PRE_NO,	"gn",	qmark,		XT_QMARK_TAB },
	{ "^ZR$",		XT_PRE_NO,	"ZR",	restart,	0 },
	{ "^ZZ$",		XT_PRE_NO,	"ZZ",	quit,		0 },
	{ "^zi$",		XT_PRE_NO,	"zi",	resizetab,	XT_ZOOM_IN },
	{ "^zo$",		XT_PRE_NO,	"zo",	resizetab,	XT_ZOOM_OUT },
	{ "^z0$",		XT_PRE_NO,	"z0",	resizetab,	XT_ZOOM_NORMAL },
	{ "^[0-9]+Z$",		XT_PRE_YES,	"Z",	zoom_amount,	0 },
	{ "^[0-9]+:$",		XT_PRE_YES,	":",	flip_colon,	0 },
};

void
buffercmd_init(void)
{
	int			i;

	for (i = 0; i < LENGTH(buffercmds); i++)
		if (regcomp(&buffercmds[i].cregex, buffercmds[i].regex,
		    REG_EXTENDED | REG_NOSUB))
			startpage_add("invalid buffercmd regex %s",
			    buffercmds[i].regex);
}

void
buffercmd_abort(struct tab *t)
{
	int			i;

	if (t == NULL)
		return;

	DNPRINTF(XT_D_BUFFERCMD, "%s: clearing buffer\n", __func__);

	for (i = 0; i < LENGTH(bcmd); i++)
		bcmd[i] = '\0';

	cmd_prefix = 0; /* clear prefix for non-buffer commands */
	gtk_label_set_text(GTK_LABEL(t->sbe.buffercmd), bcmd);
}

void
buffercmd_execute(struct tab *t, struct buffercmd *cmd)
{
	struct karg		arg = {0, NULL, -1};

	arg.i = cmd->arg;
	arg.s = g_strdup(bcmd);

	DNPRINTF(XT_D_BUFFERCMD, "buffercmd_execute: buffer \"%s\" "
	    "matches regex \"%s\", executing\n", bcmd, cmd->regex);
	cmd->func(t, &arg);

	if (arg.s)
		g_free(arg.s);

	buffercmd_abort(t);
}

gboolean
buffercmd_addkey(struct tab *t, guint keyval)
{
	int			i, c, match ;
	char			s[XT_BUFCMD_SZ];

	if (gtk_widget_get_visible(GTK_WIDGET(t->buffers))) {
		buffercmd_abort(t);
		return (XT_CB_PASSTHROUGH);
	}

	if (keyval == GDK_Escape) {
		buffercmd_abort(t);
		return (XT_CB_HANDLED);
	}

	/* key with modifier or non-ascii character */
	if (!isascii(keyval)) {
		/*
		 * XXX this looks wrong but fixes some sites like
		 * http://www.seslisozluk.com/
		 * that eat a shift or ctrl and end putting default focus in js
		 * instead of ignoring the keystroke
		 * so instead of return (XT_CB_PASSTHROUGH); eat the key
		 */
		return (XT_CB_HANDLED);
	}

	DNPRINTF(XT_D_BUFFERCMD, "buffercmd_addkey: adding key \"%c\" "
	    "to buffer \"%s\"\n", keyval, bcmd);

	for (i = 0; i < LENGTH(bcmd); i++)
		if (bcmd[i] == '\0') {
			bcmd[i] = keyval;
			break;
		}

	/* buffer full, ignore input */
	if (i >= LENGTH(bcmd) -1) {
		DNPRINTF(XT_D_BUFFERCMD, "buffercmd_addkey: buffer full\n");
		buffercmd_abort(t);
		return (XT_CB_HANDLED);
	}

	gtk_label_set_text(GTK_LABEL(t->sbe.buffercmd), bcmd);

	/* find exact match */
	for (i = 0; i < LENGTH(buffercmds); i++)
		if (regexec(&buffercmds[i].cregex, bcmd,
		    (size_t) 0, NULL, 0) == 0) {
			buffercmd_execute(t, &buffercmds[i]);
			goto done;
		}

	/* find non exact matches to see if we need to abort ot not */
	for (i = 0, match = 0; i < LENGTH(buffercmds); i++) {
		DNPRINTF(XT_D_BUFFERCMD, "trying: %s\n", bcmd);
		c = -1;
		s[0] = '\0';
		if (buffercmds[i].precount == XT_PRE_MAYBE) {
			if (isdigit(bcmd[0])) {
				if (sscanf(bcmd, "%d%s", &c, s) == 0)
					continue;
			} else {
				c = 0;
				if (sscanf(bcmd, "%s", s) == 0)
					continue;
			}
		} else if (buffercmds[i].precount == XT_PRE_YES) {
			if (sscanf(bcmd, "%d%s", &c, s) == 0)
				continue;
		} else {
			if (sscanf(bcmd, "%s", s) == 0)
				continue;
		}
		if (c == -1 && buffercmds[i].precount)
			continue;
		if (!strncmp(s, buffercmds[i].cmd, strlen(s)))
			match++;

		DNPRINTF(XT_D_BUFFERCMD, "got[%d] %d <%s>: %d %s\n",
		    i, match, buffercmds[i].cmd, c, s);
	}
	if (match == 0) {
		DNPRINTF(XT_D_BUFFERCMD, "aborting: %s\n", bcmd);
		buffercmd_abort(t);
	}

done:
	return (XT_CB_HANDLED);
}

/*
 * XXX we were seeing a bunch of focus issues with the toplevel
 * main_window losing its is-active and has-toplevel-focus properties.
 * This is the most correct and portable solution we could come up with
 * without relying on calling internal GTK functions (which we
 * couldn't link to in Linux).
 */
void
fake_focus_in(GtkWidget *w)
{
	if (fevent == NULL) {
		fevent = gdk_event_new(GDK_FOCUS_CHANGE);
		fevent->focus_change.window =
		    gtk_widget_get_window(main_window);
		fevent->focus_change.type = GDK_FOCUS_CHANGE;
		fevent->focus_change.in = TRUE;
	}
	gtk_widget_send_focus_change(main_window, fevent);
}

gboolean
handle_keypress(struct tab *t, GdkEventKey *e, int entry)
{
	struct karg		args;
	struct key_binding	*k;

	/*
	 * This sometimes gets randomly unset for whatever reason in GTK3.
	 * If we're handling a keypress, the main window's is-active propery
	 * *must* be true, or else many things will break.
	 */
#if GTK_CHECK_VERSION(3, 0, 0)
	fake_focus_in(main_window);
#endif

	/* handle keybindings if buffercmd is empty.
	   if not empty, allow commands like C-n */
	if (bcmd[0] == '\0' || ((e->state & (CTRL | MOD1)) != 0))
		TAILQ_FOREACH(k, &kbl, entry)
			if (e->keyval == k->key
			    && (entry ? k->use_in_entry : 1)) {
				/* when we are edditing eat ctrl/mod keys */
				if (edit_mode == XT_EM_VI &&
				    t->mode == XT_MODE_INSERT &&
				    (e->state & CTRL || e->state & MOD1))
					return (XT_CB_PASSTHROUGH);

				if (k->mask == 0) {
					if ((e->state & (CTRL | MOD1)) == 0)
						goto runcmd;
				} else if ((e->state & k->mask) == k->mask) {
					goto runcmd;
				}
			}

	if (!entry && ((e->state & (CTRL | MOD1)) == 0))
		return (buffercmd_addkey(t, e->keyval));

	return (XT_CB_PASSTHROUGH);

runcmd:
	if (k->cmd[0] == ':') {
		args.i = ':';
		args.s = &k->cmd[1];
		return (command(t, &args));
	} else
		return (cmd_execute(t, k->cmd));
}

int
wv_keypress_cb(GtkEntry *w, GdkEventKey *e, struct tab *t)
{
	char			s[2];

	/* don't use w directly; use t->whatever instead */

	if (t == NULL) {
		show_oops(NULL, "wv_keypress_cb");
		return (XT_CB_PASSTHROUGH);
	}

	hide_oops(t);

	if (t->mode_cb)
		return (t->mode_cb(t, e, t->mode_cb_data));

	DNPRINTF(XT_D_KEY, "wv_keypress_cb: mode %d keyval 0x%x mask "
	    "0x%x tab %d\n", t->mode, e->keyval, e->state, t->tab_id);

	/* Hide buffers, if they are visible, with escape. */
	if (gtk_widget_get_visible(GTK_WIDGET(t->buffers)) &&
	    CLEAN(e->state) == 0 && e->keyval == GDK_Escape) {
		gtk_widget_grab_focus(GTK_WIDGET(t->wv));
		hide_buffers(t);
		return (XT_CB_HANDLED);
	}

	if (t->mode == XT_MODE_HINT)
		return (XT_CB_HANDLED);

	if ((CLEAN(e->state) == 0 && e->keyval == GDK_Tab) ||
	    (CLEAN(e->state) == SHFT && e->keyval == GDK_Tab))
		/* something focussy is about to happen */
		return (XT_CB_PASSTHROUGH);

	/* check if we are some sort of text input thing in the dom */
	input_check_mode(t);

	if (t->mode == XT_MODE_HINT) {
		/* XXX make sure cmd entry is enabled */
		return (XT_CB_HANDLED);
	} else if (t->mode == XT_MODE_PASSTHROUGH) {
		if (CLEAN(e->state) == 0 && e->keyval == GDK_Escape)
			t->mode = XT_MODE_COMMAND;
		return (XT_CB_PASSTHROUGH);
	} else if (t->mode == XT_MODE_COMMAND) {
		/* prefix input */
		snprintf(s, sizeof s, "%c", e->keyval);
		if (CLEAN(e->state) == 0 && isdigit(s[0]))
			cmd_prefix = 10 * cmd_prefix + atoi(s);
		return (handle_keypress(t, e, 0));
	} else {
		/* insert mode */
		return (handle_keypress(t, e, 1));
	}

	/* NOTREACHED */
	return (XT_CB_PASSTHROUGH);
}

gboolean
hint_continue(struct tab *t)
{
	const gchar		*c = gtk_entry_get_text(GTK_ENTRY(t->cmd));
	char			*s;
	const gchar		*errstr = NULL;
	gboolean		rv = TRUE;
	int			i;

	if (!(c[0] == '.' || c[0] == ','))
		goto done;
	if (strlen(c) == 1) {
		/* XXX should not happen */
		rv = TRUE;
		goto done;
	}

	if (isdigit(c[1])) {
		/* numeric input */
		i = strtonum(&c[1], 1, 4096, &errstr);
		if (errstr) {
			show_oops(t, "invalid numerical hint %s", &c[1]);
			goto done;
		}
		s = g_strdup_printf("hints.updateHints(%d);", i);
		run_script(t, s);
		g_free(s);
	} else {
		/* alphanumeric input */
		s = g_strdup_printf("hints.createHints('%s', '%c');",
		    &c[1], c[0] == '.' ? 'f' : 'F');
		run_script(t, s);
		g_free(s);
	}

	rv = FALSE;
done:
	return (rv);
}

gboolean
search_continue(struct tab *t)
{
	const gchar		*c = gtk_entry_get_text(GTK_ENTRY(t->cmd));
	gboolean		rv = FALSE;

	if (c[0] == ':' || c[0] == '.' || c[0] == ',')
		goto done;
	if (strlen(c) == 1) {
		webkit_web_view_unmark_text_matches(t->wv);
		goto done;
	}

	if (c[0] == '/')
		t->search_forward = TRUE;
	else if (c[0] == '?')
		t->search_forward = FALSE;
	else
		goto done;

	rv = TRUE;
done:
	return (rv);
}

gboolean
search_cb(struct tab *t)
{
	const gchar		*c = gtk_entry_get_text(GTK_ENTRY(t->cmd));
#if !GTK_CHECK_VERSION(3, 0, 0)
	GdkColor		color;
#endif

	if (search_continue(t) == FALSE)
		goto done;

	/* search */
	if (webkit_web_view_search_text(t->wv, &c[1], FALSE, t->search_forward,
	    TRUE) == FALSE) {
		/* not found, mark red */
#if GTK_CHECK_VERSION(3, 0, 0)
		gtk_widget_set_name(t->cmd, XT_CSS_RED);
#else
		gdk_color_parse(XT_COLOR_RED, &color);
		gtk_widget_modify_base(t->cmd, GTK_STATE_NORMAL, &color);
#endif
		/* unmark and remove selection */
		webkit_web_view_unmark_text_matches(t->wv);
		/* my kingdom for a way to unselect text in webview */
	} else {
		/* found, highlight all */
		webkit_web_view_unmark_text_matches(t->wv);
		webkit_web_view_mark_text_matches(t->wv, &c[1], FALSE, 0);
		webkit_web_view_set_highlight_text_matches(t->wv, TRUE);
#if GTK_CHECK_VERSION(3, 0, 0)
		gtk_widget_set_name(t->cmd, XT_CSS_NORMAL);
#else
		gtk_widget_modify_base(t->cmd, GTK_STATE_NORMAL,
		    &t->default_style->base[GTK_STATE_NORMAL]);
#endif
	}
done:
	t->search_id = 0;
	return (FALSE);
}

int
cmd_keyrelease_cb(GtkEntry *w, GdkEventKey *e, struct tab *t)
{
	const gchar		*c = gtk_entry_get_text(w);

	if (t == NULL) {
		show_oops(NULL, "cmd_keyrelease_cb invalid parameters");
		return (XT_CB_PASSTHROUGH);
	}

	DNPRINTF(XT_D_CMD, "cmd_keyrelease_cb: keyval 0x%x mask 0x%x tab %d\n",
	    e->keyval, e->state, t->tab_id);

	/* hinting */
	if (!(e->keyval == GDK_Tab || e->keyval == GDK_ISO_Left_Tab)) {
		if (hint_continue(t) == FALSE)
			goto done;
	}

	/* search */
	if (search_continue(t) == FALSE)
		goto done;

	/* if search length is > 4 then no longer play timeout games */
	if (strlen(c) > 4) {
		if (t->search_id) {
			g_source_remove(t->search_id);
			t->search_id = 0;
		}
		search_cb(t);
		goto done;
	}

	/* reestablish a new timer if the user types fast */
	if (t->search_id)
		g_source_remove(t->search_id);
	t->search_id = g_timeout_add(250, (GSourceFunc)search_cb, (gpointer)t);

done:
	return (XT_CB_PASSTHROUGH);
}

gboolean
match_uri(const gchar *uri, const gchar *key) {
	gchar			*voffset;
	size_t			len;
	gboolean		match = FALSE;

	len = strlen(key);

	if (!strncmp(key, uri, len))
		match = TRUE;
	else {
		voffset = strstr(uri, "/") + 2;
		if (!strncmp(key, voffset, len))
			match = TRUE;
		else if (g_str_has_prefix(voffset, "www.")) {
			voffset = voffset + strlen("www.");
			if (!strncmp(key, voffset, len))
				match = TRUE;
		}
	}

	return (match);
}

gboolean
match_session(const gchar *name, const gchar *key) {
	char		*sub;

	sub = strcasestr(name, key);

	return sub == name;
}

void
cmd_getlist(int id, char *key)
{
	int			i,  dep, c = 0;
	struct history		*h;
	struct session		*s;

	if (id >= 0) {
		if (cmds[id].type & XT_URLARG) {
			RB_FOREACH_REVERSE(h, history_list, &hl)
				if (match_uri(h->uri, key)) {
					cmd_status.list[c] = (char *)h->uri;
					if (++c > 255)
						break;
				}
			cmd_status.len = c;
			return;
		} else if (cmds[id].type & XT_SESSARG) {
			TAILQ_FOREACH(s, &sessions, entry)
				if (match_session(s->name, key)) {
					cmd_status.list[c] = (char *)s->name;
					if (++c > 255)
						break;
				}
			cmd_status.len = c;
			return;
		} else if (cmds[id].type & XT_SETARG) {
			for (i = 0; i < get_settings_size(); i++)
				if (!strncmp(key, get_setting_name(i),
				    strlen(key)))
					cmd_status.list[c++] =
					    get_setting_name(i);
			cmd_status.len = c;
			return;
		}
	}

	dep = (id == -1) ? 0 : cmds[id].level + 1;

	for (i = id + 1; i < LENGTH(cmds); i++) {
		if (cmds[i].level < dep)
			break;
		if (cmds[i].level == dep && !strncmp(key, cmds[i].cmd,
		    strlen(key)) && !isdigit(cmds[i].cmd[0]))
			cmd_status.list[c++] = cmds[i].cmd;

	}

	cmd_status.len = c;
}

char *
cmd_getnext(int dir)
{
	cmd_status.index += dir;

	if (cmd_status.index < 0)
		cmd_status.index = cmd_status.len - 1;
	else if (cmd_status.index >= cmd_status.len)
		cmd_status.index = 0;

	return cmd_status.list[cmd_status.index];
}

int
cmd_tokenize(char *s, char *tokens[])
{
	int			i = 0;
	char			*tok, *last = NULL;
	size_t			len = strlen(s);
	bool			blank;

	blank = len == 0 || (len > 0 && s[len - 1] == ' ');
	for (tok = strtok_r(s, " ", &last); tok && i < 3;
	    tok = strtok_r(NULL, " ", &last), i++)
		tokens[i] = tok;

	if (blank && i < 3)
		tokens[i++] = "";

	return (i);
}

void
cmd_complete(struct tab *t, char *str, int dir)
{
	GtkEntry		*w = GTK_ENTRY(t->cmd);
	int			i, j, levels, c = 0, dep = 0, parent = -1;
	int			matchcount = 0;
	char			*tok, *match, *s = g_strdup(str);
	char			*tokens[3];
	char			res[XT_MAX_URL_LENGTH + 32] = ":";
	char			*sc = s;

	DNPRINTF(XT_D_CMD, "%s: complete %s\n", __func__, str);

	/* copy prefix*/
	for (i = 0; isdigit(s[i]); i++)
		res[i + 1] = s[i];

	for (; isspace(s[i]); i++)
		 res[i + 1] = s[i];

	s += i;

	levels = cmd_tokenize(s, tokens);

	for (i = 0; i < levels - 1; i++) {
		tok = tokens[i];
		matchcount = 0;
		for (j = c; j < LENGTH(cmds); j++) {
			if (cmds[j].level < dep)
				break;
			if (cmds[j].level == dep && !strncmp(tok, cmds[j].cmd,
			    strlen(tok))) {
				matchcount++;
				c = j + 1;
				if (strlen(tok) == strlen(cmds[j].cmd)) {
					matchcount = 1;
					break;
				}
			}
		}

		if (matchcount == 1) {
			strlcat(res, tok, sizeof res);
			strlcat(res, " ", sizeof res);
			dep++;
		} else {
			g_free(sc);
			return;
		}

		parent = c - 1;
	}

	if (cmd_status.index == -1)
		cmd_getlist(parent, tokens[i]);

	if (cmd_status.len > 0) {
		match = cmd_getnext(dir);
		strlcat(res, match, sizeof res);
		gtk_entry_set_text(w, res);
		gtk_editable_set_position(GTK_EDITABLE(w), -1);
	}

	g_free(sc);
}

char *
parse_prefix_and_alias(const char *str, int *prefix)
{
	struct cmd_alias	*c;
	char			*s = g_strdup(str), *sc;

	g_strstrip(s);
	sc = s;

	if (isdigit(s[0])) {
		sscanf(s, "%d", prefix);
		while (isdigit(s[0]) || isspace(s[0]))
			++s;
	}

	TAILQ_FOREACH(c, &cal, entry) {
		if (strncmp(s, c->alias, strlen(c->alias)))
			continue;

		if (strlen(s) == strlen(c->alias)) {
			g_free(sc);
			return (g_strdup(c->cmd));
		}

		if (!isspace(s[strlen(c->alias)]))
			continue;

		s = g_strdup_printf("%s %s", c->cmd, &s[strlen(c->alias) + 1]);
		g_free(sc);
		return (s);
	}
	s = g_strdup(s);
	g_free(sc);
	return (s);
}

gboolean
cmd_execute(struct tab *t, char *str)
{
	struct cmd		*cmd = NULL;
	char			*tok, *last = NULL, *s = str;
	int			j = 0, len, c = 0, dep = 0, matchcount = 0;
	int			prefix = -1, rv = XT_CB_PASSTHROUGH;
	struct karg		arg = {0, NULL, -1};

	s = parse_prefix_and_alias(s, &prefix);

	for (tok = strtok_r(s, " ", &last); tok;
	    tok = strtok_r(NULL, " ", &last)) {
		matchcount = 0;
		for (j = c; j < LENGTH(cmds); j++) {
			if (cmds[j].level < dep)
				break;
			len = (tok[strlen(tok) - 1] == '!') ? strlen(tok) - 1 :
			    strlen(tok);
			if (cmds[j].level == dep &&
			    !strncmp(tok, cmds[j].cmd, len)) {
				matchcount++;
				c = j + 1;
				cmd = &cmds[j];
				if (len == strlen(cmds[j].cmd)) {
					matchcount = 1;
					break;
				}
			}
		}
		if (matchcount == 1) {
			if (cmd->type > 0)
				goto execute_cmd;
			dep++;
		} else {
			show_oops(t, "Invalid command: %s", str);
			goto done;
		}
	}
execute_cmd:
	if (cmd == NULL) {
		show_oops(t, "Empty command");
		goto done;
	}
	arg.i = cmd->arg;

	if (prefix != -1)
		arg.precount = prefix;
	else if (cmd_prefix > 0)
		arg.precount = cmd_prefix;

	if (j > 0 && !(cmd->type & XT_PREFIX) && arg.precount > -1) {
		show_oops(t, "No prefix allowed: %s", str);
		goto done;
	}
	if (cmd->type > 1)
		arg.s = last ? g_strdup(last) : g_strdup("");
	if (cmd->type & XT_INTARG && last && strlen(last) > 0) {
		if (arg.s == NULL) {
			show_oops(t, "Invalid command");
			goto done;
		}
		arg.precount = atoi(arg.s);
		if (arg.precount <= 0) {
			if (arg.s[0] == '0')
				show_oops(t, "Zero count");
			else
				show_oops(t, "Trailing characters");
			goto done;
		}
	}

	DNPRINTF(XT_D_CMD, "%s: prefix %d arg %s\n",
	    __func__, arg.precount, arg.s);

	cmd->func(t, &arg);

	rv = XT_CB_HANDLED;
done:
	if (j > 0)
		cmd_prefix = 0;
	g_free(s);
	if (arg.s)
		g_free(arg.s);

	return (rv);
}

int
save_runtime_setting(const char *name, const char *val)
{
	struct stat		sb;
	FILE			*f;
	size_t			linelen;
	int			found = 0;
	char			file[PATH_MAX];
	char			delim[3] = { '\0', '\0', '\0' };
	char			*line, *lt, *start;
	char			*contents, *tmp;

	if (runtime_settings == NULL || strlen(runtime_settings) == 0)
		return (-1);

	snprintf(file, sizeof file, "%s" PS "%s", work_dir, runtime_settings);
	if (stat(file, &sb) || (f = fopen(file, "r+")) == NULL)
		return (-1);
	lt = g_strdup_printf("%s=%s", name, val);
	contents = g_strdup("");
	while (!feof(f)) {
		line = fparseln(f, &linelen, NULL, delim, 0);
		if (line == NULL || linelen == 0)
			continue;
		tmp = contents;
		start = g_strdup_printf("%s=", name);
		if (strstr(line, start) == NULL)
			contents = g_strdup_printf("%s%s\n", contents, line);
		else {
			found = 1;
			contents = g_strdup_printf("%s%s\n", contents, lt);
		}
		g_free(tmp);
		g_free(start);
		free(line);
		line = NULL;
	}
	if (found == 0) {
		tmp = contents;
		contents = g_strdup_printf("%s%s\n", contents, lt);
		g_free(tmp);
	}
	if ((f = freopen(file, "w", f)) == NULL)
		return (-1);
	else {
		fputs(contents, f);
		fclose(f);
	}
	g_free(lt);
	g_free(contents);

	return (0);
}

int
entry_focus_cb(GtkWidget *w, GdkEvent e, struct tab *t)
{
	/*
	 * This sometimes gets randomly unset for whatever reason in GTK3,
	 * causing a GtkEntry's text cursor becomes invisible.  When we focus
	 * a GtkEntry, be sure to manually reset the main window's is-active
	 * property so the cursor is shown correctly.
	 */
#if GTK_CHECK_VERSION(3, 0, 0)
	fake_focus_in(main_window);
#endif
	return (XT_CB_PASSTHROUGH);
}

int
entry_key_cb(GtkEntry *w, GdkEventKey *e, struct tab *t)
{
	if (t == NULL) {
		show_oops(NULL, "entry_key_cb invalid parameters");
		return (XT_CB_PASSTHROUGH);
	}

	DNPRINTF(XT_D_CMD, "entry_key_cb: keyval 0x%x mask 0x%x tab %d\n",
	    e->keyval, e->state, t->tab_id);

	hide_oops(t);

	if (e->keyval == GDK_Escape) {
		/* don't use focus_webview(t) because we want to type :cmds */
		gtk_widget_grab_focus(GTK_WIDGET(t->wv));
	}

	return (handle_keypress(t, e, 1));
}

struct command_entry *
history_prev(struct command_list *l, struct command_entry *at)
{
	if (at == NULL)
		at = TAILQ_LAST(l, command_list);
	else {
		at = TAILQ_PREV(at, command_list, entry);
		if (at == NULL)
			at = TAILQ_LAST(l, command_list);
	}

	return (at);
}

struct command_entry *
history_next(struct command_list *l, struct command_entry *at)
{
	if (at == NULL)
		at = TAILQ_FIRST(l);
	else {
		at = TAILQ_NEXT(at, entry);
		if (at == NULL)
			at = TAILQ_FIRST(l);
	}

	return (at);
}

int
cmd_keypress_cb(GtkEntry *w, GdkEventKey *e, struct tab *t)
{
	int			rv = XT_CB_HANDLED;
	const gchar		*c = gtk_entry_get_text(w);
	char			*s;

	if (t == NULL) {
		show_oops(NULL, "cmd_keypress_cb parameters");
		return (XT_CB_PASSTHROUGH);
	}

	DNPRINTF(XT_D_CMD, "cmd_keypress_cb: keyval 0x%x mask 0x%x tab %d\n",
	    e->keyval, e->state, t->tab_id);

	/* sanity */
	if (c == NULL)
		e->keyval = GDK_Escape;
	else if (!(c[0] == ':' || c[0] == '/' || c[0] == '?' ||
	    c[0] == '.' || c[0] == ','))
		e->keyval = GDK_Escape;

	if (e->keyval != GDK_Tab && e->keyval != GDK_Shift_L &&
	    e->keyval != GDK_ISO_Left_Tab)
		cmd_status.index = -1;

	switch (e->keyval) {
	case GDK_Tab:
		if (c[0] == ':')
			cmd_complete(t, (char *)&c[1], 1);
		else if (c[0] == '.' || c[0] == ',')
			run_script(t, "hints.focusNextHint();");
		goto done;
	case GDK_ISO_Left_Tab:
		if (c[0] == ':')
			cmd_complete(t, (char *)&c[1], -1);
		else if (c[0] == '.' || c[0] == ',')
			run_script(t, "hints.focusPreviousHint();");
		goto done;
	case GDK_Down:
		if (c[0] == '?') {
			if ((search_at = history_next(&shl, search_at))) {
				search_at->line[0] = c[0];
				gtk_entry_set_text(w, search_at->line);
				gtk_editable_set_position(GTK_EDITABLE(w), -1);
			}
		} else if (c[0] == '/') {
			if ((search_at = history_prev(&shl, search_at))) {
				search_at->line[0] = c[0];
				gtk_entry_set_text(w, search_at->line);
				gtk_editable_set_position(GTK_EDITABLE(w), -1);
			}
		} else if (c[0] == ':') {
			if ((history_at = history_prev(&chl, history_at))) {
				history_at->line[0] = c[0];
				gtk_entry_set_text(w, history_at->line);
				gtk_editable_set_position(GTK_EDITABLE(w), -1);
			}
		}
		goto done;
	case GDK_Up:
		if (c[0] == '/') {
			if ((search_at = history_next(&shl, search_at))) {
				search_at->line[0] = c[0];
				gtk_entry_set_text(w, search_at->line);
				gtk_editable_set_position(GTK_EDITABLE(w), -1);
			}
		} else if (c[0] == '?') {
			if ((search_at = history_prev(&shl, search_at))) {
				search_at->line[0] = c[0];
				gtk_entry_set_text(w, search_at->line);
				gtk_editable_set_position(GTK_EDITABLE(w), -1);
			}
		} if (c[0] == ':') {
			if ((history_at = history_next(&chl, history_at))) {
				history_at->line[0] = c[0];
				gtk_entry_set_text(w, history_at->line);
				gtk_editable_set_position(GTK_EDITABLE(w), -1);
			}
		}
		goto done;
	case GDK_BackSpace:
		if (!(!strcmp(c, ":") || !strcmp(c, "/") || !strcmp(c, "?") ||
		    !strcmp(c, ".") || !strcmp(c, ","))) {
			/* see if we are doing hinting and reset it */
			if (c[0] == '.' || c[0] == ',') {
				/* recreate hints */
				s = g_strdup_printf("hints.createHints('', "
				    "'%c');", c[0] == '.' ? 'f' : 'F');
				run_script(t, s);
				g_free(s);
			}
			break;
		}

		/* FALLTHROUGH */
	case GDK_Escape:
		hide_cmd(t);

		/* cancel search */
		if (c != NULL && (c[0] == '/' || c[0] == '?'))
			webkit_web_view_unmark_text_matches(t->wv);

		/* no need to cancel hints */
		goto done;
	}

	rv = XT_CB_PASSTHROUGH;
done:
	return (rv);
}

void
wv_popup_activ_cb(GtkMenuItem *menu, struct tab *t)
{
	GtkAction		*a = NULL;
	GtkClipboard		*clipboard, *primary;
	const gchar		*name, *uri;

	a = gtk_activatable_get_related_action(GTK_ACTIVATABLE(menu));
	if (a == NULL)
		return;
	name = gtk_action_get_name(a);

	DNPRINTF(XT_D_CMD, "wv_popup_activ_cb: tab %d action %s\n",
	    t->tab_id, name);

	/*
	 * context-menu-action-3    copy link location
	 * context-menu-action-7    copy image address
	 * context-menu-action-2030 copy video link location
	 */

	/* copy actions */
	if ((g_strcmp0(name, "context-menu-action-3") == 0) ||
	    (g_strcmp0(name, "context-menu-action-7") == 0) ||
	    (g_strcmp0(name, "context-menu-action-2030") == 0)) {
		clipboard = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
		primary = gtk_clipboard_get(GDK_SELECTION_PRIMARY);
		uri = gtk_clipboard_wait_for_text(clipboard);
		if (uri == NULL)
			return;
		gtk_clipboard_set_text(primary, uri, -1);
	}
}

void
wv_popup_cb(WebKitWebView *wview, GtkMenu *menu, struct tab *t)
{
	GList			*items, *l;

	DNPRINTF(XT_D_CMD, "wv_popup_cb: tab %d\n", t->tab_id);

	items = gtk_container_get_children(GTK_CONTAINER(menu));
	for (l = items; l; l = l->next)
		g_signal_connect(l->data, "activate",
		    G_CALLBACK(wv_popup_activ_cb), t);
	g_list_free(items);
}

void
cmd_popup_cb(GtkEntry *entry, GtkMenu *menu, struct tab *t)
{
	/* popup menu enabled */
	t->popup = 1;
}

int
cmd_focusout_cb(GtkWidget *w, GdkEventFocus *e, struct tab *t)
{
	if (t == NULL) {
		show_oops(NULL, "cmd_focusout_cb invalid parameters");
		return (XT_CB_PASSTHROUGH);
	}

	DNPRINTF(XT_D_CMD, "cmd_focusout_cb: tab %d popup %d\n",
	    t->tab_id, t->popup);

	/* if popup is enabled don't lose focus */
	if (t->popup) {
		t->popup = 0;
		return (XT_CB_PASSTHROUGH);
	}

	hide_cmd(t);
	hide_oops(t);
	disable_hints(t);

	return (XT_CB_PASSTHROUGH);
}

void
cmd_hide_cb(GtkWidget *w, struct tab *t)
{
	if (t == NULL) {
		show_oops(NULL, "%s: invalid parameters", __func__);
		return;
	}

	if (show_url == 0 || t->focus_wv)
		focus_webview(t);
	else
		gtk_widget_grab_focus(GTK_WIDGET(t->uri_entry));
}

void
cmd_activate_cb(GtkEntry *entry, struct tab *t)
{
	char			*s;
	const gchar		*c = gtk_entry_get_text(entry);

	if (t == NULL) {
		show_oops(NULL, "cmd_activate_cb invalid parameters");
		return;
	}

	DNPRINTF(XT_D_CMD, "cmd_activate_cb: tab %d %s\n", t->tab_id, c);

	/* sanity */
	if (c == NULL)
		goto done;
	else if (!(c[0] == ':' || c[0] == '/' || c[0] == '?' ||
	    c[0] == '.' || c[0] == ','))
		goto done;
	if (strlen(c) < 1)
		goto done;
	s = (char *)&c[1];

	if (c[0] == '/' || c[0] == '?') {
		/* see if there is a timer pending */
		if (t->search_id) {
			g_source_remove(t->search_id);
			t->search_id = 0;
			search_cb(t);
		}

		if (t->search_text) {
			g_free(t->search_text);
			t->search_text = NULL;
		}

		t->search_text = g_strdup(s);
		if (global_search)
			g_free(global_search);
		global_search = g_strdup(s);
		t->search_forward = c[0] == '/';

		history_add(&shl, search_file, s, &search_history_count);
	} else if (c[0] == '.' || c[0] == ',') {
		run_script(t, "hints.fire();");
		/* XXX history for link following? */
	} else if (c[0] == ':') {
		history_add(&chl, command_file, s, &cmd_history_count);
		/* can't call hide_cmd after cmd_execute */
		hide_cmd(t);
		cmd_execute(t, s);
		return;
	}

done:
	hide_cmd(t);
}

void
backward_cb(GtkWidget *w, struct tab *t)
{
	struct karg		a;

	if (t == NULL) {
		show_oops(NULL, "backward_cb invalid parameters");
		return;
	}

	DNPRINTF(XT_D_NAV, "backward_cb: tab %d\n", t->tab_id);

	a.i = XT_NAV_BACK;
	navaction(t, &a);
}

void
forward_cb(GtkWidget *w, struct tab *t)
{
	struct karg		a;

	if (t == NULL) {
		show_oops(NULL, "forward_cb invalid parameters");
		return;
	}

	DNPRINTF(XT_D_NAV, "forward_cb: tab %d\n", t->tab_id);

	a.i = XT_NAV_FORWARD;
	navaction(t, &a);
}

void
home_cb(GtkWidget *w, struct tab *t)
{
	if (t == NULL) {
		show_oops(NULL, "home_cb invalid parameters");
		return;
	}

	DNPRINTF(XT_D_NAV, "home_cb: tab %d\n", t->tab_id);

	load_uri(t, home);
}

void
stop_cb(GtkWidget *w, struct tab *t)
{
	WebKitWebFrame		*frame;

	if (t == NULL) {
		show_oops(NULL, "stop_cb invalid parameters");
		return;
	}

	DNPRINTF(XT_D_NAV, "stop_cb: tab %d\n", t->tab_id);

	frame = webkit_web_view_get_main_frame(t->wv);
	if (frame == NULL) {
		show_oops(t, "stop_cb: no frame");
		return;
	}

	webkit_web_frame_stop_loading(frame);
	abort_favicon_download(t);
}

void
setup_webkit(struct tab *t)
{
	if (is_g_object_setting(G_OBJECT(t->settings), "enable-dns-prefetching"))
		g_object_set(G_OBJECT(t->settings), "enable-dns-prefetching",
		    FALSE, (char *)NULL);
	else
		warnx("webkit does not have \"enable-dns-prefetching\" property");
	g_object_set(G_OBJECT(t->settings),
	    "user-agent", t->user_agent, (char *)NULL);
	g_object_set(G_OBJECT(t->settings),
	    "enable-scripts", enable_scripts, (char *)NULL);
	g_object_set(G_OBJECT(t->settings),
	    "enable-plugins", enable_plugins, (char *)NULL);
	g_object_set(G_OBJECT(t->settings),
	    "javascript-can-open-windows-automatically", enable_scripts,
	    (char *)NULL);
	g_object_set(G_OBJECT(t->settings),
	    "enable-html5-database", FALSE, (char *)NULL);
	g_object_set(G_OBJECT(t->settings),
	    "enable-html5-local-storage", enable_localstorage, (char *)NULL);
	g_object_set(G_OBJECT(t->settings),
	    "enable_spell_checking", enable_spell_checking, (char *)NULL);
	g_object_set(G_OBJECT(t->settings),
	    "spell_checking_languages", spell_check_languages, (char *)NULL);
	g_object_set(G_OBJECT(t->settings),
	    "enable-developer-extras", TRUE, (char *)NULL);
	g_object_set(G_OBJECT(t->wv),
	    "full-content-zoom", TRUE, (char *)NULL);
	g_object_set(G_OBJECT(t->settings),
	    "auto-load-images", auto_load_images, (char *)NULL);
	if (is_g_object_setting(G_OBJECT(t->settings),
	    "enable-display-of-insecure-content"))
		g_object_set(G_OBJECT(t->settings),
		    "enable-display-of-insecure-content",
		    allow_insecure_content, (char *)NULL);
	if (is_g_object_setting(G_OBJECT(t->settings),
	    "enable-running-of-insecure-content"))
		g_object_set(G_OBJECT(t->settings),
		    "enable-running-of-insecure-content",
		    allow_insecure_scripts, (char *)NULL);

	webkit_web_view_set_settings(t->wv, t->settings);
}

gboolean
update_statusbar_position(GtkAdjustment* adjustment, gpointer data)
{
	struct tab	*ti, *t = NULL;
	gdouble		view_size, value, max;
	gchar		*position;

	TAILQ_FOREACH(ti, &tabs, entry)
		if (ti->tab_id == gtk_notebook_get_current_page(notebook)) {
			t = ti;
			break;
		}

	if (t == NULL)
		return FALSE;

	if (adjustment == NULL)
		adjustment = gtk_scrolled_window_get_vadjustment(
		    GTK_SCROLLED_WINDOW(t->browser_win));

	view_size = gtk_adjustment_get_page_size(adjustment);
	value = gtk_adjustment_get_value(adjustment);
	max = gtk_adjustment_get_upper(adjustment) - view_size;

	if (max == 0)
		position = g_strdup("All");
	else if (value == max)
		position = g_strdup("Bot");
	else if (value == 0)
		position = g_strdup("Top");
	else
		position = g_strdup_printf("%d%%", (int) ((value / max) * 100));

	gtk_label_set_text(GTK_LABEL(t->sbe.position), position);
	g_free(position);

	return (TRUE);
}

GtkWidget *
create_window(const gchar *name)
{
	GtkWidget		*w;

	w = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	if (window_maximize)
		gtk_window_maximize(GTK_WINDOW(w));
	else
		gtk_window_set_default_size(GTK_WINDOW(w), window_width, window_height);
	gtk_widget_set_name(w, name);
	gtk_window_set_wmclass(GTK_WINDOW(w), name, "Xombrero");

	return (w);
}

GtkWidget *
create_browser(struct tab *t)
{
	GtkWidget		*w;
	gchar			*strval;
	GtkAdjustment		*adjustment;

	if (t == NULL) {
		show_oops(NULL, "create_browser invalid parameters");
		return (NULL);
	}

	w = gtk_scrolled_window_new(NULL, NULL);
	gtk_widget_set_can_focus(w, FALSE);
	t->adjust_h = gtk_scrolled_window_get_hadjustment(
	    GTK_SCROLLED_WINDOW(w));
	t->adjust_v = gtk_scrolled_window_get_vadjustment(
	    GTK_SCROLLED_WINDOW(w));
#if !GTK_CHECK_VERSION(3, 0, 0)
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(w),
	    GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
#endif


	t->wv = WEBKIT_WEB_VIEW(webkit_web_view_new());
	gtk_container_add(GTK_CONTAINER(w), GTK_WIDGET(t->wv));

	/* set defaults */
	t->settings = webkit_web_settings_new();

	g_object_set(t->settings, "default-encoding", encoding, (char *)NULL);

	if (user_agent == NULL) {
		g_object_get(G_OBJECT(t->settings), "user-agent", &strval,
		    (char *)NULL);
		t->user_agent = g_strdup_printf("%s %s+", strval, version);
		g_free(strval);
	} else
		t->user_agent = g_strdup(user_agent->value);

	t->stylesheet = g_strdup(stylesheet);
	t->load_images = auto_load_images;

	adjustment =
	    gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(w));
	g_signal_connect(G_OBJECT(adjustment), "value-changed",
	    G_CALLBACK(update_statusbar_position), NULL);

	setup_webkit(t);
	setup_inspector(t);

	return (w);
}

GtkWidget *
create_kiosk_toolbar(struct tab *t)
{
	GtkWidget		*toolbar = NULL, *b;

#if GTK_CHECK_VERSION(3, 0, 0)
	b = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_widget_set_name(GTK_WIDGET(b), "toolbar");
#else
	b = gtk_hbox_new(FALSE, 0);
#endif
	toolbar = b;
	gtk_container_set_border_width(GTK_CONTAINER(toolbar), 0);

	/* backward button */
	t->backward = create_button("Back", GTK_STOCK_GO_BACK, 0);
	gtk_widget_set_sensitive(t->backward, FALSE);
	g_signal_connect(G_OBJECT(t->backward), "clicked",
	    G_CALLBACK(backward_cb), t);
	gtk_box_pack_start(GTK_BOX(b), t->backward, TRUE, TRUE, 0);

	/* forward button */
	t->forward = create_button("Forward", GTK_STOCK_GO_FORWARD, 0);
	gtk_widget_set_sensitive(t->forward, FALSE);
	g_signal_connect(G_OBJECT(t->forward), "clicked",
	    G_CALLBACK(forward_cb), t);
	gtk_box_pack_start(GTK_BOX(b), t->forward, TRUE, TRUE, 0);

	/* home button */
	t->gohome = create_button("Home", GTK_STOCK_HOME, 0);
	gtk_widget_set_sensitive(t->gohome, true);
	g_signal_connect(G_OBJECT(t->gohome), "clicked",
	    G_CALLBACK(home_cb), t);
	gtk_box_pack_start(GTK_BOX(b), t->gohome, TRUE, TRUE, 0);

	/* create widgets but don't use them */
	t->uri_entry = gtk_entry_new();
	g_signal_connect(G_OBJECT(t->uri_entry), "focus-in-event",
	    G_CALLBACK(entry_focus_cb), t);
#if !GTK_CHECK_VERSION(3, 0, 0)
	t->default_style = gtk_rc_get_style(t->uri_entry);
#endif
	t->stop = create_button("Stop", GTK_STOCK_STOP, 0);
	t->js_toggle = create_button("JS-Toggle", enable_scripts ?
	    GTK_STOCK_MEDIA_PLAY : GTK_STOCK_MEDIA_PAUSE, 0);

	return (toolbar);
}

GtkWidget *
create_toolbar(struct tab *t)
{
	GtkWidget		*toolbar = NULL, *b;

#if GTK_CHECK_VERSION(3, 0, 0)
	b = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_widget_set_name(GTK_WIDGET(b), "toolbar");
#else
	b = gtk_hbox_new(FALSE, 0);
#endif
	toolbar = b;
	gtk_container_set_border_width(GTK_CONTAINER(toolbar), 0);

	/* backward button */
	t->backward = create_button("Back", GTK_STOCK_GO_BACK, 0);
	gtk_widget_set_sensitive(t->backward, FALSE);
	g_signal_connect(G_OBJECT(t->backward), "clicked",
	    G_CALLBACK(backward_cb), t);
	gtk_box_pack_start(GTK_BOX(b), t->backward, FALSE, FALSE, 0);

	/* forward button */
	t->forward = create_button("Forward",GTK_STOCK_GO_FORWARD, 0);
	gtk_widget_set_sensitive(t->forward, FALSE);
	g_signal_connect(G_OBJECT(t->forward), "clicked",
	    G_CALLBACK(forward_cb), t);
	gtk_box_pack_start(GTK_BOX(b), t->forward, FALSE, FALSE, 0);

	/* stop button */
	t->stop = create_button("Stop", GTK_STOCK_STOP, 0);
	gtk_widget_set_sensitive(t->stop, FALSE);
	g_signal_connect(G_OBJECT(t->stop), "clicked", G_CALLBACK(stop_cb), t);
	gtk_box_pack_start(GTK_BOX(b), t->stop, FALSE, FALSE, 0);

	/* JS button */
	t->js_toggle = create_button("JS-Toggle", enable_scripts ?
	    GTK_STOCK_MEDIA_PLAY : GTK_STOCK_MEDIA_PAUSE, 0);
	gtk_widget_set_sensitive(t->js_toggle, TRUE);
	g_signal_connect(G_OBJECT(t->js_toggle), "clicked",
	    G_CALLBACK(js_toggle_cb), t);
	gtk_box_pack_start(GTK_BOX(b), t->js_toggle, FALSE, FALSE, 0);

	t->uri_entry = gtk_entry_new();
	g_signal_connect(G_OBJECT(t->uri_entry), "activate",
	    G_CALLBACK(activate_uri_entry_cb), t);
	g_signal_connect(G_OBJECT(t->uri_entry), "key-press-event",
	    G_CALLBACK(entry_key_cb), t);
	g_signal_connect(G_OBJECT(t->uri_entry), "focus-in-event",
	    G_CALLBACK(entry_focus_cb), t);
	completion_add(t);
	gtk_box_pack_start(GTK_BOX(b), t->uri_entry, TRUE, TRUE, 0);

	/* search entry */
	t->search_entry = gtk_entry_new();
	gtk_entry_set_width_chars(GTK_ENTRY(t->search_entry), 30);
	g_signal_connect(G_OBJECT(t->search_entry), "activate",
	    G_CALLBACK(activate_search_entry_cb), t);
	g_signal_connect(G_OBJECT(t->search_entry), "key-press-event",
	    G_CALLBACK(entry_key_cb), t);
	g_signal_connect(G_OBJECT(t->search_entry), "focus-in-event",
	    G_CALLBACK(entry_focus_cb), t);
	gtk_box_pack_start(GTK_BOX(b), t->search_entry, FALSE, FALSE, 0);

#if !GTK_CHECK_VERSION(3, 0, 0)
	t->default_style = gtk_rc_get_style(t->uri_entry);
#endif

	return (toolbar);
}

GtkWidget *
create_buffers(struct tab *t)
{
	GtkCellRenderer		*renderer;
	GtkWidget		*view;

	view = gtk_tree_view_new();

	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(view), FALSE);

	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_insert_column_with_attributes
	    (GTK_TREE_VIEW(view), -1, "Id", renderer, "text", COL_ID, (char *)NULL);

	renderer = gtk_cell_renderer_pixbuf_new();
	gtk_tree_view_insert_column_with_attributes
	    (GTK_TREE_VIEW(view), -1, "Favicon", renderer, "pixbuf", COL_FAVICON,
	    (char *)NULL);

	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_insert_column_with_attributes
	    (GTK_TREE_VIEW(view), -1, "Title", renderer, "text", COL_TITLE,
	    (char *)NULL);

	gtk_tree_view_set_model
	    (GTK_TREE_VIEW(view), GTK_TREE_MODEL(buffers_store));

	return (view);
}

void
row_activated_cb(GtkTreeView *view, GtkTreePath *path,
    GtkTreeViewColumn *col, struct tab *t)
{
	GtkTreeIter		iter;
	guint			id;

	gtk_widget_grab_focus(GTK_WIDGET(t->wv));

	if (gtk_tree_model_get_iter(GTK_TREE_MODEL(buffers_store), &iter,
	    path)) {
		gtk_tree_model_get
		    (GTK_TREE_MODEL(buffers_store), &iter, COL_ID, &id, -1);
		set_current_tab(id - 1);
	}

	hide_buffers(t);
}

/* after tab reordering/creation/removal */
void
recalc_tabs(void)
{
	struct tab		*t;

	TAILQ_FOREACH(t, &tabs, entry)
		t->tab_id = gtk_notebook_page_num(notebook, t->vbox);
}

void
update_statusbar_tabs(struct tab *t)
{
	int			tab_id, max_tab_id;
	char			s[16];

	if (t != NULL)
		tab_id = t->tab_id;
	else
		tab_id = gtk_notebook_get_current_page(notebook);

	max_tab_id = gtk_notebook_get_n_pages(notebook);
	snprintf(s, sizeof s, "%d/%d", tab_id + 1, max_tab_id);

	if (t == NULL)
		t = get_current_tab();

	if (t != NULL)
		gtk_label_set_text(GTK_LABEL(t->sbe.tabs), s);
}

/* after active tab change */
void
recolor_compact_tabs(void)
{
	struct tab		*t;
	int			curid = 0;
#if !GTK_CHECK_VERSION(3, 0, 0)
	GdkColor		color_active, color_inactive;

	gdk_color_parse(XT_COLOR_CT_ACTIVE, &color_active);
	gdk_color_parse(XT_COLOR_CT_INACTIVE, &color_inactive);
#endif
	curid = gtk_notebook_get_current_page(notebook);

	TAILQ_FOREACH(t, &tabs, entry) {
#if GTK_CHECK_VERSION(3, 0, 0)
		if (t->tab_id == curid)
			gtk_widget_set_name(t->tab_elems.label, XT_CSS_ACTIVE);
		else
			gtk_widget_set_name(t->tab_elems.label, "");
#else
		if (t->tab_id == curid)
			gtk_widget_modify_fg(t->tab_elems.label,
			    GTK_STATE_NORMAL, &color_active);
		else
			gtk_widget_modify_fg(t->tab_elems.label,
			    GTK_STATE_NORMAL, &color_inactive);
#endif
	}
}

void
set_current_tab(int page_num)
{
	buffercmd_abort(get_current_tab());
	gtk_notebook_set_current_page(notebook, page_num);
	recolor_compact_tabs();
	update_statusbar_tabs(NULL);
}

int
undo_close_tab_save(struct tab *t)
{
	int			m, n;
	const gchar		*uri;
	struct undo		*u1, *u2;
	GList			*items;
	WebKitWebHistoryItem	*item;

	if ((uri = get_uri(t)) == NULL)
		return (1);

	u1 = g_malloc0(sizeof(struct undo));
	u1->uri = g_strdup(uri);

	t->bfl = webkit_web_view_get_back_forward_list(t->wv);

	m = webkit_web_back_forward_list_get_forward_length(t->bfl);
	n = webkit_web_back_forward_list_get_back_length(t->bfl);
	u1->back = n;

	/* forward history */
	items = webkit_web_back_forward_list_get_forward_list_with_limit(t->bfl, m);

	while (items) {
		item = items->data;
		u1->history = g_list_prepend(u1->history,
		    webkit_web_history_item_copy(item));
		items = g_list_next(items);
	}

	/* current item */
	if (m) {
		item = webkit_web_back_forward_list_get_current_item(t->bfl);
		u1->history = g_list_prepend(u1->history,
		    webkit_web_history_item_copy(item));
	}

	/* back history */
	items = webkit_web_back_forward_list_get_back_list_with_limit(t->bfl, n);

	while (items) {
		item = items->data;
		u1->history = g_list_prepend(u1->history,
		    webkit_web_history_item_copy(item));
		items = g_list_next(items);
	}

	TAILQ_INSERT_HEAD(&undos, u1, entry);

	if (undo_count > XT_MAX_UNDO_CLOSE_TAB) {
		u2 = TAILQ_LAST(&undos, undo_tailq);
		TAILQ_REMOVE(&undos, u2, entry);
		g_free(u2->uri);
		g_list_free(u2->history);
		g_free(u2);
	} else
		undo_count++;

	return (0);
}

void
delete_tab(struct tab *t)
{
	struct karg		a;

	DNPRINTF(XT_D_TAB, "delete_tab: %p\n", t);

	if (t == NULL)
		return;

	/*
	 * no need to join thread here because it won't access t on completion
	 */

	TAILQ_REMOVE(&tabs, t, entry);
	buffercmd_abort(t);

	/* Halt all webkit activity. */
	abort_favicon_download(t);
	webkit_web_view_stop_loading(t->wv);

	/* Save the tab, so we can undo the close. */
	undo_close_tab_save(t);

	/* just in case */
	if (t->search_id)
		g_source_remove(t->search_id);

	/* session key */
	if (t->session_key)
		g_free(t->session_key);

	/* inspector */
	bzero(&a, sizeof a);
	a.i = XT_INS_CLOSE;
	inspector_cmd(t, &a);

	if (browser_mode == XT_BM_KIOSK) {
		gtk_widget_destroy(t->uri_entry);
		gtk_widget_destroy(t->stop);
		gtk_widget_destroy(t->js_toggle);
	}

	g_object_unref(t->completion);

	gtk_widget_destroy(t->tab_elems.eventbox);
	gtk_widget_destroy(t->vbox);

	g_free(t->user_agent);
	g_free(t->stylesheet);
	g_free(t->tmp_uri);
	g_free(t->status);
	g_free(t);
	t = NULL;

	if (TAILQ_EMPTY(&tabs)) {
		if (browser_mode == XT_BM_KIOSK)
			create_new_tab(home, NULL, 1, -1);
		else
			create_new_tab(NULL, NULL, 1, -1);
	}

	/* recreate session */
	if (session_autosave) {
		bzero(&a, sizeof a);
		a.s = NULL;
		save_tabs(NULL, &a);
	}

	recalc_tabs();
	recolor_compact_tabs();
}

void
update_statusbar_zoom(struct tab *t)
{
	gfloat			zoom;
	char			s[16] = { '\0' };

	g_object_get(G_OBJECT(t->wv), "zoom-level", &zoom, (char *)NULL);
	if ((zoom <= 0.99 || zoom >= 1.01))
		snprintf(s, sizeof s, "%d%%", (int)(zoom * 100));
	gtk_label_set_text(GTK_LABEL(t->sbe.zoom), s);
}

void
setzoom_webkit(struct tab *t, int adjust)
{
#define XT_ZOOMPERCENT		0.04

	gfloat			zoom;

	if (t == NULL) {
		show_oops(NULL, "setzoom_webkit invalid parameters");
		return;
	}

	g_object_get(G_OBJECT(t->wv), "zoom-level", &zoom, (char *)NULL);
	if (adjust == XT_ZOOM_IN)
		zoom += XT_ZOOMPERCENT;
	else if (adjust == XT_ZOOM_OUT)
		zoom -= XT_ZOOMPERCENT;
	else if (adjust > 0)
		zoom = default_zoom_level + adjust / 100.0 - 1.0;
	else {
		show_oops(t, "setzoom_webkit invalid zoom value");
		return;
	}

	if (zoom < XT_ZOOMPERCENT)
		zoom = XT_ZOOMPERCENT;
	g_object_set(G_OBJECT(t->wv), "zoom-level", zoom, (char *)NULL);
	update_statusbar_zoom(t);
}

gboolean
tab_clicked_cb(GtkWidget *widget, GdkEventButton *event, gpointer data)
{
	struct tab *t = (struct tab *) data;

	DNPRINTF(XT_D_TAB, "tab_clicked_cb: tab: %d\n", t->tab_id);

	switch (event->button) {
	case 1:
		set_current_tab(t->tab_id);
		break;
	case 2:
		delete_tab(t);
		break;
	}

	return TRUE;
}

void
append_tab(struct tab *t)
{
	if (t == NULL)
		return;

	TAILQ_INSERT_TAIL(&tabs, t, entry);
	t->tab_id = gtk_notebook_append_page(notebook, t->vbox, t->tab_content);
}

GtkWidget *
create_sbe(void)
{
	GtkWidget		*sbe;

	sbe = gtk_label_new(NULL);
	gtk_widget_set_can_focus(GTK_WIDGET(sbe), FALSE);
	gtk_widget_modify_font(GTK_WIDGET(sbe), statusbar_font);
	return (sbe);
}

int
add_sbe(GtkWidget *box, char flag, int *used, GtkWidget *sbe)
{
	if (box == NULL || used == NULL || sbe == NULL) {
		DPRINTF("%s: invalid parameters", __func__);
		return (1);
	}

	if (*used) {
		warnx("flag \"%c\" specified more than "
		    "once in statusbar_elems\n", flag);
		return (1);
	}

	gtk_box_pack_start(GTK_BOX(box), sbe, FALSE, FALSE, 0);
	*used = 1;

	return (0);
}

int
statusbar_create(struct tab *t)
{
	GtkWidget		*box;	/* container for statusbar elems */
	GtkWidget		*sep;
	char			*p;
	int			sbe_P = 0, sbe_B = 0, sbe_Z = 0, sbe_T = 0,
				sbe_p = 0;
#if !GTK_CHECK_VERSION(3, 0, 0)
	GdkColor		color;
#endif

	if (t == NULL) {
		DPRINTF("%s: invalid parameters", __func__);
		return (1);
	}

#if GTK_CHECK_VERSION(3, 0, 0)
	t->statusbar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_widget_set_name(GTK_WIDGET(t->statusbar), "statusbar");
#else
	t->statusbar = gtk_hbox_new(FALSE, 0);
	box = gtk_hbox_new(FALSE, 0);
#endif
	t->sbe.ebox = gtk_event_box_new();

	gtk_widget_set_can_focus(GTK_WIDGET(t->statusbar), FALSE);
	gtk_widget_set_can_focus(GTK_WIDGET(t->sbe.ebox), FALSE);
	gtk_widget_set_can_focus(GTK_WIDGET(box), FALSE);

	gtk_box_set_spacing(GTK_BOX(box), 10);
	gtk_box_pack_start(GTK_BOX(t->statusbar), t->sbe.ebox, TRUE, TRUE, 0);
	gtk_container_add(GTK_CONTAINER(t->sbe.ebox), box);

	/* create these widgets only if specified in statusbar_elems */
	t->sbe.uri = gtk_entry_new();
	gtk_widget_set_can_focus(GTK_WIDGET(t->sbe.uri), FALSE);
	gtk_widget_modify_font(GTK_WIDGET(t->sbe.uri), statusbar_font);
#if !GTK_CHECK_VERSION(3, 0, 0)
	gtk_entry_set_inner_border(GTK_ENTRY(t->sbe.uri), NULL);
	gtk_entry_set_has_frame(GTK_ENTRY(t->sbe.uri), FALSE);
#endif	
	t->sbe.position = create_sbe();
	t->sbe.zoom = create_sbe();
	t->sbe.buffercmd = create_sbe();
	t->sbe.tabs = create_sbe();
	t->sbe.proxy = create_sbe();

#if GTK_CHECK_VERSION(3, 0, 0)
	statusbar_modify_attr(t, XT_CSS_NORMAL);
#else
	statusbar_modify_attr(t, XT_COLOR_WHITE, XT_COLOR_BLACK);
#endif

	gtk_box_pack_start(GTK_BOX(box), t->sbe.uri, TRUE, TRUE, 0);

	/*
	 * gtk widgets cannot be added to a box twice. The sbe_* variables
	 * make sure of this
	 */
	for (p = statusbar_elems; *p != '\0'; p++) {
		switch (*p) {
		case '|':
#if GTK_CHECK_VERSION(3, 0, 0)
			sep = gtk_separator_new(GTK_ORIENTATION_VERTICAL);
#else
			sep = gtk_vseparator_new();
			gdk_color_parse(XT_COLOR_SB_SEPARATOR, &color);
			gtk_widget_modify_bg(sep, GTK_STATE_NORMAL, &color);
#endif
			gtk_box_pack_start(GTK_BOX(box), sep, FALSE, FALSE, 0);
			break;
		case 'P':
			add_sbe(box, *p, &sbe_P, t->sbe.position);
			break;
		case 'B':
			add_sbe(box, *p, &sbe_B, t->sbe.buffercmd);
			break;
		case 'Z':
			add_sbe(box, *p, &sbe_Z, t->sbe.zoom);
			break;
		case 'T':
			add_sbe(box, *p, &sbe_T, t->sbe.tabs);
			break;
		case 'p':
			if (add_sbe(box, *p, &sbe_p, t->sbe.proxy) == 0)
				if (proxy_uri)
					gtk_entry_set_text(
					    GTK_ENTRY(t->sbe.proxy), "proxy");
			break;
		default:
			warnx("illegal flag \"%c\" in statusbar_elems\n", *p);
			break;
		}
	}

	gtk_box_pack_end(GTK_BOX(t->vbox), t->statusbar, FALSE, FALSE, 0);

	return (0);
}

struct tab *
create_new_tab(char *title, struct undo *u, int focus, int position)
{
	struct tab			*t;
	int				load = 1, id;
	GtkWidget			*b, *bb;
	WebKitWebHistoryItem		*item;
	GList				*items;
#if !GTK_CHECK_VERSION(3, 0, 0)
	GdkColor			color;
#endif

	DNPRINTF(XT_D_TAB, "create_new_tab: title %s focus %d\n", title, focus);

	if (tabless && !TAILQ_EMPTY(&tabs)) {
		DNPRINTF(XT_D_TAB, "create_new_tab: new tab rejected\n");
		return (NULL);
	}

	t = g_malloc0(sizeof *t);

	if (title == NULL) {
		title = "(untitled)";
		load = 0;
	}

#if GTK_CHECK_VERSION(3, 0, 0)
	t->vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	b = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_widget_set_name(t->vbox, "vbox");
#else
	t->vbox = gtk_vbox_new(FALSE, 0);
	b = gtk_hbox_new(FALSE, 0);
#endif
	gtk_widget_set_can_focus(t->vbox, FALSE);

	/* label + button for tab */
	t->tab_content = b;
	gtk_widget_set_can_focus(t->tab_content, FALSE);

	t->user_agent_id = 0;
	t->http_accept_id = 0;

#if WEBKIT_CHECK_VERSION(1, 5, 0)
	t->active = NULL;
#endif

#if GTK_CHECK_VERSION(2, 20, 0)
	t->spinner = gtk_spinner_new();
#endif
	t->label = gtk_label_new(title);
	bb = create_button("Close", GTK_STOCK_CLOSE, 1);
	gtk_label_set_max_width_chars(GTK_LABEL(t->label), 20);
	gtk_label_set_ellipsize(GTK_LABEL(t->label), PANGO_ELLIPSIZE_END);
	gtk_label_set_line_wrap(GTK_LABEL(t->label), FALSE);
	gtk_widget_set_size_request(b, 130, 0);

	/*
	 * this is a total hack and most likely breaks with other styles but
	 * is necessary so the text doesn't bounce around when the spinner is
	 * shown/hidden
	 */
#if GTK_CHECK_VERSION(3, 0, 0)
	gtk_widget_set_size_request(t->label, 95, 0);
#else
	gtk_widget_set_size_request(t->label, 100, 0);
#endif

	gtk_box_pack_start(GTK_BOX(b), bb, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(b), t->label, FALSE, FALSE, 0);
#if GTK_CHECK_VERSION(2, 20, 0)
	gtk_box_pack_end(GTK_BOX(b), t->spinner, FALSE, FALSE, 0);
#endif

	/* toolbar */
	if (browser_mode == XT_BM_KIOSK) {
		t->toolbar = create_kiosk_toolbar(t);
		gtk_box_pack_start(GTK_BOX(t->vbox), t->toolbar, FALSE, FALSE,
		    0);
	} else {
		t->toolbar = create_toolbar(t);
		gtk_box_pack_start(GTK_BOX(t->vbox), t->toolbar, FALSE, FALSE,
		    0);
	}

	/* marks */
	marks_clear(t);

	/* browser */
	t->browser_win = create_browser(t);
	set_scrollbar_visibility(t, show_scrollbars);
	gtk_box_pack_start(GTK_BOX(t->vbox), t->browser_win, TRUE, TRUE, 0);

	/* oops message for user feedback */
	t->oops = gtk_entry_new();
	gtk_entry_set_inner_border(GTK_ENTRY(t->oops), NULL);
	gtk_entry_set_has_frame(GTK_ENTRY(t->oops), FALSE);
	gtk_widget_set_can_focus(GTK_WIDGET(t->oops), FALSE);
#if GTK_CHECK_VERSION(3, 0, 0)
	gtk_widget_set_name(t->oops, XT_CSS_RED);
#else
	gdk_color_parse(XT_COLOR_RED, &color);
	gtk_widget_modify_base(t->oops, GTK_STATE_NORMAL, &color);
#endif
	gtk_box_pack_end(GTK_BOX(t->vbox), t->oops, FALSE, FALSE, 0);
	gtk_widget_modify_font(GTK_WIDGET(t->oops), oops_font);

	/* command entry */
	t->cmd = gtk_entry_new();
	g_signal_connect(G_OBJECT(t->cmd), "focus-in-event",
	    G_CALLBACK(entry_focus_cb), t);
	gtk_entry_set_inner_border(GTK_ENTRY(t->cmd), NULL);
	gtk_entry_set_has_frame(GTK_ENTRY(t->cmd), FALSE);
	gtk_box_pack_end(GTK_BOX(t->vbox), t->cmd, FALSE, FALSE, 0);
	gtk_widget_modify_font(GTK_WIDGET(t->cmd), cmd_font);

	/* status bar */
	statusbar_create(t);

	/* buffer list */
	t->buffers = create_buffers(t);
	gtk_box_pack_end(GTK_BOX(t->vbox), t->buffers, FALSE, FALSE, 0);

	/* xtp meaning is normal by default */
	set_normal_tab_meaning(t);

	/* set empty favicon */
	xt_icon_from_name(t, "text-html");

	/* and show it all */
	gtk_widget_show_all(b);
	gtk_widget_show_all(t->vbox);

	if (!fancy_bar) {
		gtk_widget_hide(t->backward);
		gtk_widget_hide(t->forward);
		gtk_widget_hide(t->stop);
		gtk_widget_hide(t->js_toggle);
	}
	if (!fancy_bar || (search_string == NULL || strlen(search_string) == 0))
		gtk_widget_hide(t->search_entry);

	/* compact tab bar */
	t->tab_elems.label = gtk_label_new(title);
	t->tab_elems.favicon = gtk_image_new();

	t->tab_elems.eventbox = gtk_event_box_new();
	gtk_widget_set_name(t->tab_elems.eventbox, "compact_tab");
#if GTK_CHECK_VERSION(3, 0, 0)
	t->tab_elems.box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);

	gtk_label_set_ellipsize(GTK_LABEL(t->tab_elems.label),
	    PANGO_ELLIPSIZE_END);
	gtk_widget_override_font(t->tab_elems.label, tabbar_font);
	gtk_widget_set_halign(t->tab_elems.label, GTK_ALIGN_START);
	gtk_widget_set_valign(t->tab_elems.label, GTK_ALIGN_START);
#else
	t->tab_elems.box = gtk_hbox_new(FALSE, 0);

	gtk_label_set_width_chars(GTK_LABEL(t->tab_elems.label), 1);
	gtk_misc_set_alignment(GTK_MISC(t->tab_elems.label), 0.0, 0.0);
	gtk_misc_set_padding(GTK_MISC(t->tab_elems.label), 4.0, 4.0);
	gtk_widget_modify_font(GTK_WIDGET(t->tab_elems.label), tabbar_font);

	gdk_color_parse(XT_COLOR_CT_BACKGROUND, &color);
	gtk_widget_modify_bg(t->tab_elems.eventbox, GTK_STATE_NORMAL, &color);
	gdk_color_parse(XT_COLOR_CT_INACTIVE, &color);
	gtk_widget_modify_fg(t->tab_elems.label, GTK_STATE_NORMAL, &color);
#endif

	gtk_box_pack_start(GTK_BOX(t->tab_elems.box), t->tab_elems.favicon, FALSE,
	    FALSE, 0);
	gtk_box_pack_start(GTK_BOX(t->tab_elems.box), t->tab_elems.label, TRUE,
	    TRUE, 0);
	gtk_container_add(GTK_CONTAINER(t->tab_elems.eventbox),
	    t->tab_elems.box);

	gtk_box_pack_start(GTK_BOX(tab_bar_box), t->tab_elems.eventbox, TRUE,
	    TRUE, 0);
	gtk_widget_show_all(t->tab_elems.eventbox);

	if (append_next == 0 || gtk_notebook_get_n_pages(notebook) == 0)
		append_tab(t);
	else {
		id = position >= 0 ? position :
		    gtk_notebook_get_current_page(notebook) + 1;
		if (id > gtk_notebook_get_n_pages(notebook))
			append_tab(t);
		else {
			TAILQ_INSERT_TAIL(&tabs, t, entry);
			gtk_notebook_insert_page(notebook, t->vbox, b, id);
			gtk_box_reorder_child(GTK_BOX(tab_bar_box),
			    t->tab_elems.eventbox, id);
			recalc_tabs();
		}
	}

#if GTK_CHECK_VERSION(2, 20, 0)
	/* turn spinner off if we are a new tab without uri */
	if (!load) {
		gtk_spinner_stop(GTK_SPINNER(t->spinner));
		gtk_widget_hide(t->spinner);
	}
#endif
	/* make notebook tabs reorderable */
	gtk_notebook_set_tab_reorderable(notebook, t->vbox, TRUE);

	/* compact tabs clickable */
	g_signal_connect(G_OBJECT(t->tab_elems.eventbox),
	    "button_press_event", G_CALLBACK(tab_clicked_cb), t);

	g_object_connect(G_OBJECT(t->cmd),
	    "signal::button-release-event", G_CALLBACK(cmd_keyrelease_cb), t,
	    "signal::key-press-event", G_CALLBACK(cmd_keypress_cb), t,
	    "signal::key-release-event", G_CALLBACK(cmd_keyrelease_cb), t,
	    "signal::focus-out-event", G_CALLBACK(cmd_focusout_cb), t,
	    "signal::activate", G_CALLBACK(cmd_activate_cb), t,
	    "signal::populate-popup", G_CALLBACK(cmd_popup_cb), t,
	    "signal::hide", G_CALLBACK(cmd_hide_cb), t,
	    (char *)NULL);

	/* reuse wv_button_cb to hide oops */
	g_object_connect(G_OBJECT(t->oops),
	    "signal::button_press_event", G_CALLBACK(wv_button_cb), t,
	    (char *)NULL);

	g_signal_connect(t->buffers,
	    "row-activated", G_CALLBACK(row_activated_cb), t);
	g_object_connect(G_OBJECT(t->buffers),
	    "signal::key-press-event", G_CALLBACK(wv_keypress_cb), t, (char *)NULL);

	g_object_connect(G_OBJECT(t->wv),
	    "signal::key-press-event", G_CALLBACK(wv_keypress_cb), t,
	    "signal::hovering-over-link", G_CALLBACK(webview_hover_cb), t,
	    "signal::download-requested", G_CALLBACK(webview_download_cb), t,
	    "signal::mime-type-policy-decision-requested", G_CALLBACK(webview_mimetype_cb), t,
	    "signal::navigation-policy-decision-requested", G_CALLBACK(webview_npd_cb), t,
	    "signal::new-window-policy-decision-requested", G_CALLBACK(webview_npd_cb), t,
	    "signal::resource-request-starting", G_CALLBACK(webview_rrs_cb), t,
	    "signal::create-web-view", G_CALLBACK(webview_cwv_cb), t,
	    "signal::close-web-view", G_CALLBACK(webview_closewv_cb), t,
	    "signal::event", G_CALLBACK(webview_event_cb), t,
	    "signal::load-finished", G_CALLBACK(webview_load_finished_cb), t,
	    "signal::load-progress-changed", G_CALLBACK(webview_progress_changed_cb), t,
	    "signal::icon-loaded", G_CALLBACK(notify_icon_loaded_cb), t,
	    "signal::button_press_event", G_CALLBACK(wv_button_cb), t,
	    "signal::button_release_event", G_CALLBACK(wv_release_button_cb), t,
	    "signal::populate-popup", G_CALLBACK(wv_popup_cb), t,
	    (char *)NULL);
	g_signal_connect(t->wv,
	    "notify::load-status", G_CALLBACK(notify_load_status_cb), t);
	g_signal_connect(t->wv,
	    "notify::title", G_CALLBACK(notify_title_cb), t);

	/* hijack the unused keys as if we were the browser */
	//g_object_connect(G_OBJECT(t->toolbar),
	//    "signal-after::key-press-event", G_CALLBACK(wv_keypress_after_cb), t,
	//    (char *)NULL);

	g_signal_connect(G_OBJECT(bb), "button_press_event",
	    G_CALLBACK(tab_close_cb), t);

	/* setup history */
	t->bfl = webkit_web_view_get_back_forward_list(t->wv);
	/* restore the tab's history */
	if (u && u->history) {
		items = u->history;
		while (items) {
			item = items->data;
			webkit_web_back_forward_list_add_item(t->bfl, item);
			items = g_list_next(items);
		}

		item = g_list_nth_data(u->history, u->back);
		if (item)
			webkit_web_view_go_to_back_forward_item(t->wv, item);

		g_list_free(items);
		g_list_free(u->history);
	} else
		webkit_web_back_forward_list_clear(t->bfl);

	/* check and show url and statusbar */
	url_set_visibility();
	statusbar_set_visibility();

	if (focus) {
		set_current_tab(t->tab_id);
		DNPRINTF(XT_D_TAB, "create_new_tab: going to tab: %d\n",
		    t->tab_id);

		if (load) {
			gtk_entry_set_text(GTK_ENTRY(t->uri_entry), title);
			load_uri(t, title);
		} else {
			if (show_url == 1)
				gtk_widget_grab_focus(GTK_WIDGET(t->uri_entry));
			else
				focus_webview(t);
		}
	} else if (load)
		load_uri(t, title);

	if (userstyle_global)
		apply_style(t);

	recolor_compact_tabs();
	setzoom_webkit(t, XT_ZOOM_NORMAL);
	return (t);
}

void
notebook_switchpage_cb(GtkNotebook *nb, GtkWidget *nbp, guint pn,
    gpointer *udata)
{
	struct tab		*t;
	const gchar		*uri;

	DNPRINTF(XT_D_TAB, "notebook_switchpage_cb: tab: %d\n", pn);

	if (gtk_notebook_get_current_page(notebook) == -1)
		recalc_tabs();

	TAILQ_FOREACH(t, &tabs, entry) {
		if (t->tab_id == pn) {
			DNPRINTF(XT_D_TAB, "notebook_switchpage_cb: going to "
			    "%d\n", pn);

			uri = get_title(t, TRUE);
			gtk_window_set_title(GTK_WINDOW(main_window), uri);

			hide_cmd(t);
			hide_oops(t);
			hide_buffers(t);

			if (t->focus_wv) {
				/* can't use focus_webview here */
				gtk_widget_grab_focus(GTK_WIDGET(t->wv));
			}
			update_statusbar_tabs(t);
			break;
		}
	}
}

void
notebook_pagereordered_cb(GtkNotebook *nb, GtkWidget *nbp, guint pn,
    gpointer *udata)
{
	struct tab		*t = NULL, *tt;

	recalc_tabs();

	TAILQ_FOREACH(tt, &tabs, entry)
		if (tt->tab_id == pn) {
			t = tt;
			break;
		}
	if (t == NULL)
		return;
	DNPRINTF(XT_D_TAB, "page_reordered_cb: tab: %d\n", t->tab_id);

	gtk_box_reorder_child(GTK_BOX(tab_bar_box), t->tab_elems.eventbox,
	    t->tab_id);

	update_statusbar_tabs(t);
}

void
menuitem_response(struct tab *t)
{
	gtk_notebook_set_current_page(notebook, t->tab_id);
}

int
destroy_menu(GtkWidget *w, GdkEventFocus *e, void *notused)
{
	gtk_widget_destroy(w);
	return (XT_CB_PASSTHROUGH);
}

gboolean
arrow_cb(GtkWidget *w, GdkEventButton *event, gpointer user_data)
{
	GtkWidget		*menu, *menu_items;
	GdkEventButton		*bevent;
	struct tab		**stabs = NULL;
	int			i, num_tabs;
	const gchar		*uri;

	if (event->type == GDK_BUTTON_PRESS) {
		bevent = (GdkEventButton *) event;
		menu = gtk_menu_new();

		num_tabs = sort_tabs_by_page_num(&stabs);
		for (i = 0; i < num_tabs; ++i) {
			if (stabs[i] == NULL)
				continue;
			if ((uri = get_uri(stabs[i])) == NULL)
				/* XXX make sure there is something to print */
				/* XXX add gui pages in here to look purdy */
				uri = "(untitled)";
			menu_items = gtk_menu_item_new_with_label(uri);
			gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_items);
			gtk_widget_show(menu_items);

			g_signal_connect_swapped((menu_items),
			    "activate", G_CALLBACK(menuitem_response),
			    (gpointer)stabs[i]);
		}
		g_free(stabs);

		gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL,
		    bevent->button, bevent->time);

		g_object_connect(G_OBJECT(menu),
		    "signal::hide", G_CALLBACK(destroy_menu), NULL,
		    (char *)NULL);

		return (TRUE /* eat event */);
	}

	return (FALSE /* propagate */);
}

int
icon_size_map(int iconsz)
{
	if (iconsz <= GTK_ICON_SIZE_INVALID ||
	    iconsz > GTK_ICON_SIZE_DIALOG)
		return (GTK_ICON_SIZE_SMALL_TOOLBAR);

	return (iconsz);
}

GtkWidget *
create_button(char *name, char *stockid, int size)
{
	GtkWidget		*button, *image;
	int			gtk_icon_size;
#if !GTK_CHECK_VERSION(3, 0, 0)
	gchar			*newstyle;
#endif

#if !GTK_CHECK_VERSION(3, 0, 0)
	newstyle = g_strdup_printf(
	    "style \"%s-style\"\n"
	    "{\n"
	    "  GtkWidget::focus-padding = 0\n"
	    "  GtkWidget::focus-line-width = 0\n"
	    "  xthickness = 0\n"
	    "  ythickness = 0\n"
	    "}\n"
	    "widget \"*.%s\" style \"%s-style\"", name, name, name);
	gtk_rc_parse_string(newstyle);
	g_free(newstyle);
#endif
	button = gtk_button_new();
	gtk_widget_set_can_focus(button, FALSE);
	gtk_button_set_focus_on_click(GTK_BUTTON(button), FALSE);
	gtk_icon_size = icon_size_map(size ? size : icon_size);

	image = gtk_image_new_from_stock(stockid, gtk_icon_size);
	gtk_container_set_border_width(GTK_CONTAINER(button), 1);
	gtk_container_add(GTK_CONTAINER(button), GTK_WIDGET(image));
	gtk_widget_set_name(button, name);
	gtk_button_set_relief(GTK_BUTTON(button), GTK_RELIEF_NONE);

	return (button);
}

void
button_set_stockid(GtkWidget *button, char *stockid)
{
	GtkWidget		*image;

	image = gtk_image_new_from_stock(stockid, icon_size_map(icon_size));
	gtk_button_set_image(GTK_BUTTON(button), image);
}

void
create_canvas(void)
{
	GtkWidget		*vbox;
	GList			*l = NULL;
	GdkPixbuf		*pb;
	char			file[PATH_MAX];
	int			i;
#if !GTK_CHECK_VERSION(3, 0, 0)
	GdkColor		color;
#endif

#if GTK_CHECK_VERSION(3, 0, 0)
	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
#else
	vbox = gtk_vbox_new(FALSE, 0);
#endif
	gtk_box_set_spacing(GTK_BOX(vbox), 0);
	gtk_widget_set_can_focus(vbox, FALSE);
	notebook = GTK_NOTEBOOK(gtk_notebook_new());
#if !GTK_CHECK_VERSION(3, 0, 0)
	/* XXX seems to be needed with gtk+2 */
	g_object_set(G_OBJECT(notebook), "tab-border", 0, NULL);
#endif
	gtk_notebook_set_scrollable(notebook, TRUE);
	gtk_notebook_set_show_border(notebook, FALSE);
	gtk_widget_set_can_focus(GTK_WIDGET(notebook), FALSE);

	abtn = gtk_button_new();
	gtk_widget_set_can_focus(abtn, FALSE);
	arrow = gtk_arrow_new(GTK_ARROW_DOWN, GTK_SHADOW_NONE);
	gtk_widget_set_name(abtn, "Arrow");
	gtk_container_add(GTK_CONTAINER(abtn), arrow);
	gtk_widget_set_size_request(abtn, -1, 20);

#if GTK_CHECK_VERSION(2, 20, 0)
	gtk_notebook_set_action_widget(notebook, abtn, GTK_PACK_END);
#endif
	/* compact tab bar */
	tab_bar = gtk_event_box_new();
#if GTK_CHECK_VERSION(3, 0, 0)
	gtk_widget_set_name(tab_bar, "tab_bar");
	tab_bar_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
#else
	gdk_color_parse(XT_COLOR_CT_SEPARATOR, &color);
	gtk_widget_modify_bg(tab_bar, GTK_STATE_NORMAL, &color);
	tab_bar_box = gtk_hbox_new(TRUE, 0);
#endif
	gtk_container_add(GTK_CONTAINER(tab_bar), tab_bar_box);
	gtk_box_set_homogeneous(GTK_BOX(tab_bar_box), TRUE);
	gtk_box_set_spacing(GTK_BOX(tab_bar_box), 2);

	gtk_box_pack_start(GTK_BOX(vbox), tab_bar, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), GTK_WIDGET(notebook), TRUE, TRUE, 0);

	g_object_connect(G_OBJECT(notebook),
	    "signal::switch-page", G_CALLBACK(notebook_switchpage_cb), NULL,
	    (char *)NULL);
	g_object_connect(G_OBJECT(notebook),
	    "signal::page-reordered", G_CALLBACK(notebook_pagereordered_cb),
	    NULL, (char *)NULL);
	g_signal_connect(G_OBJECT(abtn), "button_press_event",
	    G_CALLBACK(arrow_cb), NULL);

	main_window = create_window("xombrero");
	gtk_container_add(GTK_CONTAINER(main_window), vbox);
	g_signal_connect(G_OBJECT(main_window), "delete_event",
	    G_CALLBACK(gtk_main_quit), NULL);
#if GTK_CHECK_VERSION(3, 0, 0)
	gtk_window_set_has_resize_grip(GTK_WINDOW(main_window), FALSE);
#endif

	/* icons */
	for (i = 0; i < LENGTH(icons); i++) {
		snprintf(file, sizeof file, "%s" PS "%s", resource_dir, icons[i]);
		pb = gdk_pixbuf_new_from_file(file, NULL);
		l = g_list_append(l, pb);
	}
	gtk_window_set_default_icon_list(l);

	gtk_widget_show_all(abtn);
	gtk_widget_show_all(main_window);
	notebook_tab_set_visibility();
}

#ifndef XT_SOCKET_DISABLE
int
send_cmd_to_socket(char *cmd)
{
	int			s, len, rv = 1;
	struct sockaddr_un	sa;

	if ((s = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
		warnx("%s: socket", __func__);
		return (rv);
	}

	sa.sun_family = AF_UNIX;
	snprintf(sa.sun_path, sizeof(sa.sun_path), "%s" PS "%s",
	    work_dir, XT_SOCKET_FILE);
	len = SUN_LEN(&sa);

	if (connect(s, (struct sockaddr *)&sa, len) == -1) {
		warnx("%s: connect", __func__);
		goto done;
	}

	if (send(s, cmd, strlen(cmd) + 1, 0) == -1) {
		warnx("%s: send", __func__);
		goto done;
	}

	rv = 0;
done:
	close(s);
	return (rv);
}

gboolean
socket_watcher(GIOChannel *source, GIOCondition condition, gpointer data)
{
	int			s, n;
	char			str[XT_MAX_URL_LENGTH];
	socklen_t		t = sizeof(struct sockaddr_un);
	struct sockaddr_un	sa;
	struct passwd		*p;
	uid_t			uid;
	gid_t			gid;
	struct tab		*tt;
	gint			fd = g_io_channel_unix_get_fd(source);

	if ((s = accept(fd, (struct sockaddr *)&sa, &t)) == -1) {
		warn("accept");
		return (FALSE);
	}

	if (getpeereid(s, &uid, &gid) == -1) {
		warn("getpeereid");
		return (FALSE);
	}
	if (uid != getuid() || gid != getgid()) {
		warnx("unauthorized user");
		return (FALSE);
	}

	p = getpwuid(uid);
	if (p == NULL) {
		warnx("not a valid user");
		return (FALSE);
	}

	n = recv(s, str, sizeof(str), 0);
	if (n <= 0)
		return (TRUE);

	tt = get_current_tab();
	cmd_execute(tt, str);
	return (TRUE);
}

int
is_running(void)
{
	int			s, len, rv = 1;
	struct sockaddr_un	sa;

	if ((s = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
		warn("is_running: socket");
		return (-1);
	}

	sa.sun_family = AF_UNIX;
	snprintf(sa.sun_path, sizeof(sa.sun_path), "%s" PS "%s",
	    work_dir, XT_SOCKET_FILE);
	len = SUN_LEN(&sa);

	/* connect to see if there is a listener */
	if (connect(s, (struct sockaddr *)&sa, len) == -1)
		rv = 0; /* not running */
	else
		rv = 1; /* already running */

	close(s);

	return (rv);
}

int
build_socket(void)
{
	int			s, len;
	struct sockaddr_un	sa;

	if ((s = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
		warn("build_socket: socket");
		return (-1);
	}

	sa.sun_family = AF_UNIX;
	snprintf(sa.sun_path, sizeof(sa.sun_path), "%s" PS "%s",
	    work_dir, XT_SOCKET_FILE);
	len = SUN_LEN(&sa);

	/* connect to see if there is a listener */
	if (connect(s, (struct sockaddr *)&sa, len) == -1) {
		/* no listener so we will */
		unlink(sa.sun_path);

		if (bind(s, (struct sockaddr *)&sa, len) == -1) {
			warn("build_socket: bind");
			goto done;
		}

		if (listen(s, 1) == -1) {
			warn("build_socket: listen");
			goto done;
		}

		return (s);
	}

done:
	close(s);
	return (-1);
}
#endif

void
xxx_dir(char *dir)
{
	struct stat		sb;

	if (stat(dir, &sb)) {
#if defined __MINGW32__
		if (mkdir(dir) == -1)
#else
		if (mkdir(dir, S_IRWXU) == -1)
#endif
			err(1, "mkdir %s", dir);
		if (stat(dir, &sb))
			err(1, "stat %s", dir);
	}
	if (S_ISDIR(sb.st_mode) == 0)
		errx(1, "%s not a dir", dir);
#if !defined __MINGW32__
	if (((sb.st_mode & (S_IRWXU | S_IRWXG | S_IRWXO))) != S_IRWXU) {
		warnx("fixing invalid permissions on %s", dir);
		if (chmod(dir, S_IRWXU) == -1)
			err(1, "chmod %s", dir);
	}
#endif
}

void
usage(void)
{
	fprintf(stderr,
	    "%s [-nSTVt][-f file][-s session] url ...\n", __progname);
	exit(0);
}

GStaticRecMutex		my_gdk_mtx = G_STATIC_REC_MUTEX_INIT;
volatile int		mtx_depth;
int			mtx_complain;

/*
 * The linux flash plugin violates the gdk locking mechanism.
 * Work around the issue by using a recursive mutex with some match applied
 * to see if we hit a buggy condition.
 *
 * The following code is painful so just don't read it.  It really doesn't
 * make much sense but seems to work.
 */
void
mtx_lock(void)
{
#ifdef XT_DEBUG
	char		*s = NULL;
#endif

	g_static_rec_mutex_lock(&my_gdk_mtx);
	if (my_gdk_mtx.depth <= 0) {
#ifdef XT_DEBUG
		s = "lock <= 0";
#endif
		g_static_rec_mutex_lock(&my_gdk_mtx);
		goto complain;
	} else if (my_gdk_mtx.depth != 1) {
#ifdef XT_DEBUG
		s = "lock != 1";
#endif
		do {
			g_static_rec_mutex_unlock(&my_gdk_mtx);
		} while (my_gdk_mtx.depth > 1);
		goto complain;
	}
	return;

complain:
	if (mtx_complain == 0) {
		DNPRINTF(XT_D_MTX, "buggy mutex implementation detected(%s), "
		    "work around implemented", s);
		mtx_complain = 1;
	}
}

void
mtx_unlock(void)
{
#ifdef XT_DEBUG
	char		*s = NULL;
#endif

	if (my_gdk_mtx.depth <= 0) {
#ifdef XT_DEBUG
		s = "unlock <= 0";
#endif
		goto complain;
	} else if (my_gdk_mtx.depth != 1) {
#ifdef XT_DEBUG
		s = "unlock != 1";
#endif
		g_static_rec_mutex_unlock_full(&my_gdk_mtx);
		goto complain;
	}
	g_static_rec_mutex_unlock(&my_gdk_mtx);
	return;

complain:
	if (mtx_complain == 0) {
		DNPRINTF(XT_D_MTX, "buggy mutex implementation detected(%s), "
		    "work around implemented", s);
		mtx_complain = 1;
	}
}

#if GTK_CHECK_VERSION(3, 0, 0)
void
setup_css(void)
{
	GtkCssProvider		*provider;
	GdkDisplay		*display;
	GdkScreen		*screen;
	GFile			*file;
	char			path[PATH_MAX];
#if defined __MINGW32__
	GtkCssProvider		*windows_hacks;
#endif

	provider = gtk_css_provider_new();
	display = gdk_display_get_default();
	screen = gdk_display_get_default_screen(display);
	snprintf(path, sizeof path, "%s" PS "%s", resource_dir, XT_CSS_FILE);
	file = g_file_new_for_path(path);
	gtk_css_provider_load_from_file(provider, file, NULL);
	gtk_style_context_add_provider_for_screen(screen,
	    GTK_STYLE_PROVIDER(provider),
	    GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
#if defined __MINGW32__
	windows_hacks = gtk_css_provider_new();
	gtk_css_provider_load_from_data(windows_hacks,
	    ".notebook tab {\n"
	    "	border-width: 0px;\n"
	    "}", -1, NULL);
	gtk_style_context_add_provider_for_screen(screen,
	    GTK_STYLE_PROVIDER(windows_hacks),
	    GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
#endif
	g_object_unref(file);
}
#endif

void
welcome(void)
{
	startpage_add("<b>Welcome to xombrero %s!</b><p>", version);
	startpage_add("Details at "
	    "<a href=https://opensource.conformal.com/wiki/xombrero>xombrero "
	    "wiki page</a><p>");
}

int
main(int argc, char **argv)
{
	struct stat		sb;
	int			c, optn = 0, opte = 0, focus = 1;
	char			conf[PATH_MAX] = { '\0' };
	char			file[PATH_MAX];
	char			sodversion[32];
	char			*env_proxy = NULL;
	char			*path;
	FILE			*f = NULL;
	struct karg		a;

	start_argv = (char * const *)argv;

	if (os_init)
		os_init();

	/* prepare gtk */
#ifdef USE_THREADS
#if !defined __MINGW32__
	/* http://web.archiveorange.com/archive/v/UsPjxkX5PsaXBIoOjqxf */
	XInitThreads();
#endif
	/* http://developer.gnome.org/gdk/stable/gdk-Threads.html */
	g_thread_init(NULL);
	gdk_threads_set_lock_functions(mtx_lock, mtx_unlock);
	gdk_threads_init();
	gdk_threads_enter();

	/* http://www.gnupg.org/documentation/manuals/gcrypt/Multi_002dThreading.html */
	gcry_control (GCRYCTL_SET_THREAD_CBS, &gcry_threads_pthread);
#endif
	gtk_init(&argc, &argv);

	gnutls_global_init();

	strlcpy(named_session, XT_SAVED_TABS_FILE, sizeof named_session);

	RB_INIT(&hl);
	RB_INIT(&js_wl);
	RB_INIT(&pl_wl);
	RB_INIT(&force_https);
	RB_INIT(&downloads);
	RB_INIT(&st_tree);
	RB_INIT(&svl);
	RB_INIT(&svil);
	RB_INIT(&ua_list);

	TAILQ_INIT(&sessions);
	TAILQ_INIT(&tabs);
	TAILQ_INIT(&mtl);
	TAILQ_INIT(&aliases);
	TAILQ_INIT(&undos);
	TAILQ_INIT(&kbl);
	TAILQ_INIT(&spl);
	TAILQ_INIT(&chl);
	TAILQ_INIT(&shl);
	TAILQ_INIT(&cul);
	TAILQ_INIT(&srl);

#ifndef XT_RESOURCE_LIMITS_DISABLE
	struct rlimit		rlp;
	char			*cmd = NULL;
	int			s;
	GIOChannel		*channel;

	/* fiddle with ulimits */
	if (getrlimit(RLIMIT_NOFILE, &rlp) == -1)
		warn("getrlimit");
	else {
		/* just use them all */
		rlp.rlim_cur = rlp.rlim_max;
		if (setrlimit(RLIMIT_NOFILE, &rlp) == -1)
			warn("setrlimit");
		if (getrlimit(RLIMIT_NOFILE, &rlp) == -1)
			warn("getrlimit");
		else if (rlp.rlim_cur < 1024)
			startpage_add("%s requires at least 1024 "
			    "(2048 recommended) file " "descriptors, "
			    "currently it has up to %d available",
			   __progname, rlp.rlim_cur);
	}
#endif

	while ((c = getopt(argc, argv, "STVf:s:tne")) != -1) {
		switch (c) {
		case 'S':
			show_url = 0;
			break;
		case 'T':
			show_tabs = 0;
			break;
		case 'V':
#ifdef XOMBRERO_BUILDSTR
			errx(0 , "Version: %s Build: %s",
			    version, XOMBRERO_BUILDSTR);
#else
			errx(0 , "Version: %s", version);
#endif
			break;
		case 'f':
			strlcpy(conf, optarg, sizeof(conf));
			break;
		case 's':
			strlcpy(named_session, optarg, sizeof(named_session));
			break;
		case 't':
			tabless = 1;
			break;
		case 'n':
			optn = 1;
			break;
		case 'e':
			opte = 1;
			break;
		default:
			usage();
			/* NOTREACHED */
		}
	}
	argc -= optind;
	argv += optind;

	init_keybindings();

	pwd = getpwuid(getuid());
	if (pwd == NULL)
		errx(1, "invalid user %d", getuid());

	/* set download dir */
	if (strlen(download_dir) == 0)
		strlcpy(download_dir, pwd->pw_dir, sizeof download_dir);

	/* compile buffer command regexes */
	buffercmd_init();

	/* set default dynamic string settings */
	home = g_strdup(XT_DS_HOME);
	search_string = g_strdup(XT_DS_SEARCH_STRING);
	strlcpy(runtime_settings, "runtime", sizeof runtime_settings);
	cmd_font_name = g_strdup(XT_DS_CMD_FONT_NAME);
	oops_font_name = g_strdup(XT_DS_OOPS_FONT_NAME);
	statusbar_font_name = g_strdup(XT_DS_STATUSBAR_FONT_NAME);
	tabbar_font_name = g_strdup(XT_DS_TABBAR_FONT_NAME);
	statusbar_elems = g_strdup("BP");
	spell_check_languages = g_strdup(XT_DS_SPELL_CHECK_LANGUAGES);
	encoding = g_strdup(XT_DS_ENCODING);
	spell_check_languages = g_strdup(XT_DS_SPELL_CHECK_LANGUAGES);
	path = g_strdup_printf("%s" PS "style.css", resource_dir);
	userstyle = g_filename_to_uri(path, NULL, NULL);
	g_free(path);
	stylesheet = g_strdup(userstyle);

	/* set statically allocated (struct special) settings */
	if (strlen(default_script) == 0)
		expand_tilde(default_script, sizeof default_script,
		    XT_DS_DEFAULT_SCRIPT);
	if (strlen(ssl_ca_file) == 0)
		expand_tilde(ssl_ca_file, sizeof ssl_ca_file,
		    XT_DS_SSL_CA_FILE);

	/* soup session */
	session = webkit_get_default_session();

	/* read config file */
	if (strlen(conf) == 0)
		snprintf(conf, sizeof conf, "%s" PS ".%s",
		    pwd->pw_dir, XT_CONF_FILE);
	config_parse(conf, 0);

	/* read preloaded HSTS list */
	if (preload_strict_transport) {
		snprintf(conf, sizeof conf, "%s" PS "%s",
		    resource_dir, XT_HSTS_PRELOAD_FILE);
		config_parse(conf, 0);
	}

	/* init fonts */
	cmd_font = pango_font_description_from_string(cmd_font_name);
	oops_font = pango_font_description_from_string(oops_font_name);
	statusbar_font = pango_font_description_from_string(statusbar_font_name);
	tabbar_font = pango_font_description_from_string(tabbar_font_name);

	/* working directory */
	if (strlen(work_dir) == 0)
		snprintf(work_dir, sizeof work_dir, "%s" PS "%s",
		    pwd->pw_dir, XT_DIR);
	xxx_dir(work_dir);

	/* icon cache dir */
	snprintf(cache_dir, sizeof cache_dir, "%s" PS "%s", work_dir, XT_CACHE_DIR);
	xxx_dir(cache_dir);

	/* certs dir */
	snprintf(certs_dir, sizeof certs_dir, "%s" PS "%s", work_dir, XT_CERT_DIR);
	xxx_dir(certs_dir);

	/* cert changes dir */
	snprintf(certs_cache_dir, sizeof certs_cache_dir, "%s" PS "%s",
	    work_dir, XT_CERT_CACHE_DIR);
	xxx_dir(certs_cache_dir);

	/* sessions dir */
	snprintf(sessions_dir, sizeof sessions_dir, "%s" PS "%s",
	    work_dir, XT_SESSIONS_DIR);
	xxx_dir(sessions_dir);

	/* js dir */
	snprintf(js_dir, sizeof js_dir, "%s" PS "%s", work_dir, XT_JS_DIR);
	xxx_dir(js_dir);

	/* temp dir */
	snprintf(temp_dir, sizeof temp_dir, "%s" PS "%s", work_dir, XT_TEMP_DIR);
	xxx_dir(temp_dir);

	/* runtime settings that can override config file */
	if (runtime_settings[0] != '\0')
		config_parse(runtime_settings, 1);

	/* download dir */
	if (!strcmp(download_dir, pwd->pw_dir))
		strlcat(download_dir, PS "downloads", sizeof download_dir);
	xxx_dir(download_dir);

	/* first start file */
	snprintf(file, sizeof file, "%s" PS "%s", work_dir, XT_SOD_FILE);
	if (stat(file, &sb)) {
		warnx("start of day file doesn't exist, creating it");
		if ((f = fopen(file, "w")) == NULL)
			err(1, "startofday");
		if (fputs(version, f) == EOF)
			err(1, "fputs");
		fclose(f);

		/* welcome user */
		welcome();
	} else {
		if ((f = fopen(file, "r+")) == NULL)
			err(1, "startofday");
		if (fgets(sodversion, sizeof sodversion, f) == NULL)
			err(1, "fgets");
		sodversion[strcspn(sodversion, "\n")] = '\0';
		if (strcmp(version, sodversion)) {
			if ((f = freopen(file, "w", f)) == NULL)
				err(1, "startofday");
			if (fputs(version, f) == EOF)
				err(1, "fputs");

			/* upgrade, say something smart */
			welcome();
		}
		fclose(f);
	}

	/* favorites file */
	snprintf(file, sizeof file, "%s" PS "%s", work_dir, XT_FAVS_FILE);
	if (stat(file, &sb)) {
		warnx("favorites file doesn't exist, creating it");
		if ((f = fopen(file, "w")) == NULL)
			err(1, "favorites");
		fclose(f);
	}

	/* quickmarks file */
	snprintf(file, sizeof file, "%s" PS "%s", work_dir, XT_QMARKS_FILE);
	if (stat(file, &sb)) {
		warnx("quickmarks file doesn't exist, creating it");
		if ((f = fopen(file, "w")) == NULL)
			err(1, "quickmarks");
		fclose(f);
	}

	/* search history */
	if (history_autosave) {
		snprintf(search_file, sizeof search_file, "%s" PS "%s",
		    work_dir, XT_SEARCH_FILE);
		if (stat(search_file, &sb)) {
			warnx("search history file doesn't exist, creating it");
			if ((f = fopen(search_file, "w")) == NULL)
				err(1, "search_history");
			fclose(f);
		}
		history_read(&shl, search_file, &search_history_count);
	}

	/* command history */
	if (history_autosave) {
		snprintf(command_file, sizeof command_file, "%s" PS "%s",
		    work_dir, XT_COMMAND_FILE);
		if (stat(command_file, &sb)) {
			warnx("command history file doesn't exist, creating it");
			if ((f = fopen(command_file, "w")) == NULL)
				err(1, "command_history");
			fclose(f);
		}
		history_read(&chl, command_file, &cmd_history_count);
	}

	/* cookies */
	setup_cookies();

	/* guess_search regex */
	if (url_regex == NULL)
		url_regex = g_strdup(XT_URL_REGEX);
	if (url_regex)
		if (regcomp(&url_re, url_regex, REG_EXTENDED | REG_NOSUB))
			startpage_add("invalid url regex %s", url_regex);

	/* proxy */
	env_proxy = getenv("http_proxy");
	if (env_proxy)
		setup_proxy(env_proxy);
	else {
		env_proxy = getenv("HTTP_PROXY");
		if (env_proxy)
			setup_proxy(env_proxy);
		else
			setup_proxy(http_proxy);
	}

	/* the user can optionally have the proxy disabled at startup */
	if ((http_proxy_starts_enabled == 0) && (http_proxy != NULL)) {
		http_proxy_save = g_strdup(http_proxy);
		setup_proxy(NULL);
	}

#ifndef XT_SOCKET_DISABLE
	if (opte) {
		send_cmd_to_socket(argv[0]);
		exit(0);
	}
#else
	opte = opte; /* shut mingw up */
#endif
	/* set some connection parameters */
	g_object_set(session, "max-conns", max_connections, (char *)NULL);
	g_object_set(session, "max-conns-per-host", max_host_connections,
	    (char *)NULL);
	g_object_set(session, SOUP_SESSION_SSL_STRICT, ssl_strict_certs,
	    (char *)NULL);

	g_signal_connect(session, "request-queued", G_CALLBACK(session_rq_cb),
	    NULL);

#ifndef XT_SOCKET_DISABLE
	/* see if there is already a xombrero running */
	if (single_instance && is_running()) {
		optn = 1;
		warnx("already running");
	}

	if (optn) {
		while (argc) {
			cmd = g_strdup_printf("%s %s", "tabnew", argv[0]);
			send_cmd_to_socket(cmd);
			if (cmd)
				g_free(cmd);

			argc--;
			argv++;
		}
		exit(0);
	}
#else
	optn = optn; /* shut mingw up */
#endif
	/* tld list */
	tld_tree_init();

	if (enable_strict_transport)
		strict_transport_init();

	/* uri completion */
	completion_model = gtk_list_store_new(1, G_TYPE_STRING);

	/* buffers */
	buffers_store = gtk_list_store_new
	    (NUM_COLS, G_TYPE_UINT, GDK_TYPE_PIXBUF, G_TYPE_STRING);

	qmarks_load();

	/* go graphical */
	create_canvas();
	notebook_tab_set_visibility();

	if (save_global_history)
		restore_global_history();

	/* restore session list */
	restore_sessions_list();

	if (!strcmp(named_session, XT_SAVED_TABS_FILE))
		restore_saved_tabs();
	else {
		a.s = named_session;
		a.i = XT_SES_DONOTHING;
		open_tabs(NULL, &a);
	}

	/* see if we have an exception */
	if (!TAILQ_EMPTY(&spl)) {
		create_new_tab("about:startpage", NULL, focus, -1);
		focus = 0;
	}

	while (argc) {
		create_new_tab(argv[0], NULL, focus, -1);
		focus = 0;

		argc--;
		argv++;
	}

	if (TAILQ_EMPTY(&tabs))
		create_new_tab(home, NULL, 1, -1);
#ifndef XT_SOCKET_DISABLE
	if (enable_socket)
		if ((s = build_socket()) != -1) {
			channel = g_io_channel_unix_new(s);
			g_io_add_watch(channel, G_IO_IN, socket_watcher, NULL);
		}
#endif

#if GTK_CHECK_VERSION(3, 0, 0)
	setup_css();
#endif

	gtk_main();

#ifdef USE_THREADS
	GDK_FLUSH();
	gdk_threads_leave();
	g_static_rec_mutex_unlock_full(&my_gdk_mtx); /* just in case */
#endif

	gnutls_global_deinit();

	if (url_regex)
		regfree(&url_re);

	return (0);
}
