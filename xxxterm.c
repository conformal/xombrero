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

char		*version = XXXTERM_VERSION;

/* hooked functions */
void		(*_soup_cookie_jar_add_cookie)(SoupCookieJar *, SoupCookie *);
void		(*_soup_cookie_jar_delete_cookie)(SoupCookieJar *,
		    SoupCookie *);

#ifdef XT_DEBUG
u_int32_t		swm_debug = 0
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
			    ;
#endif

#ifdef USE_THREADS
GCRY_THREAD_OPTION_PTHREAD_IMPL;
#endif

char		*icons[] = {
	"xxxtermicon16.png",
	"xxxtermicon32.png",
	"xxxtermicon48.png",
	"xxxtermicon64.png",
	"xxxtermicon128.png"
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

struct sp {
	char			*line;
	TAILQ_ENTRY(sp)		entry;
};
TAILQ_HEAD(sp_list, sp);

struct command_entry {
	char				*line;
	TAILQ_ENTRY(command_entry)	entry;
};
TAILQ_HEAD(command_list, command_entry);

/* defines */
#define XT_DIR			(".xxxterm")
#define XT_CACHE_DIR		("cache")
#define XT_CERT_DIR		("certs/")
#define XT_SESSIONS_DIR		("sessions/")
#define XT_CONF_FILE		("xxxterm.conf")
#define XT_QMARKS_FILE		("quickmarks")
#define XT_SAVED_TABS_FILE	("main_session")
#define XT_RESTART_TABS_FILE	("restart_tabs")
#define XT_SOCKET_FILE		("socket")
#define XT_HISTORY_FILE		("history")
#define XT_REJECT_FILE		("rejected.txt")
#define XT_COOKIE_FILE		("cookies.txt")
#define XT_SAVE_SESSION_ID	("SESSION_NAME=")
#define XT_SEARCH_FILE		("search_history")
#define XT_COMMAND_FILE		("command_history")
#define XT_DLMAN_REFRESH	"10"
#define XT_MAX_URL_LENGTH	(4096) /* 1 page is atomic, don't make bigger */
#define XT_MAX_UNDO_CLOSE_TAB	(32)
#define XT_RESERVED_CHARS	"$&+,/:;=?@ \"<>#%%{}|^~[]`"
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

#define XT_QMARK_SET		(0)
#define XT_QMARK_OPEN		(1)
#define XT_QMARK_TAB		(2)

#define XT_MARK_SET		(0)
#define XT_MARK_GOTO		(1)

#define XT_TAB_LAST		(-4)
#define XT_TAB_FIRST		(-3)
#define XT_TAB_PREV		(-2)
#define XT_TAB_NEXT		(-1)
#define XT_TAB_INVALID		(0)
#define XT_TAB_NEW		(1)
#define XT_TAB_DELETE		(2)
#define XT_TAB_DELQUIT		(3)
#define XT_TAB_OPEN		(4)
#define XT_TAB_UNDO_CLOSE	(5)
#define XT_TAB_SHOW		(6)
#define XT_TAB_HIDE		(7)
#define XT_TAB_NEXTSTYLE	(8)

#define XT_NAV_INVALID		(0)
#define XT_NAV_BACK		(1)
#define XT_NAV_FORWARD		(2)
#define XT_NAV_RELOAD		(3)

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

#define XT_URL_SHOW		(1)
#define XT_URL_HIDE		(2)

#define XT_CMD_OPEN		(0)
#define XT_CMD_OPEN_CURRENT	(1)
#define XT_CMD_TABNEW		(2)
#define XT_CMD_TABNEW_CURRENT	(3)

#define XT_STATUS_NOTHING	(0)
#define XT_STATUS_LINK		(1)
#define XT_STATUS_URI		(2)
#define XT_STATUS_LOADING	(3)

#define XT_SES_DONOTHING	(0)
#define XT_SES_CLOSETABS	(1)

#define XT_BM_NORMAL		(0)
#define XT_BM_WHITELIST		(1)
#define XT_BM_KIOSK		(2)

#define XT_PREFIX		(1<<0)
#define XT_USERARG		(1<<1)
#define XT_URLARG		(1<<2)
#define XT_INTARG		(1<<3)
#define XT_SESSARG		(1<<4)
#define XT_SETARG		(1<<5)

#define XT_HINT_NEWTAB		(1<<0)

#define XT_MODE_INSERT		(0)
#define XT_MODE_COMMAND		(1)

#define XT_TABS_NORMAL		0
#define XT_TABS_COMPACT		1

#define XT_BUFCMD_SZ		(8)

#define XT_EJS_SHOW		(1<<0)

/* mime types */
struct mime_type {
	char			*mt_type;
	char			*mt_action;
	int			mt_default;
	int			mt_download;
	TAILQ_ENTRY(mime_type)	entry;
};
TAILQ_HEAD(mime_type_list, mime_type);

/* uri aliases */
struct alias {
	char			*a_name;
	char			*a_uri;
	TAILQ_ENTRY(alias)	 entry;
};
TAILQ_HEAD(alias_list, alias);

/* settings that require restart */
int		tabless = 0;	/* allow only 1 tab */
int		enable_socket = 0;
int		single_instance = 0; /* only allow one xxxterm to run */
int		fancy_bar = 1;	/* fancy toolbar */
int		browser_mode = XT_BM_NORMAL;
int		enable_localstorage = 1;
char		*statusbar_elems = NULL;

/* runtime settings */
int		show_tabs = 1;	/* show tabs on notebook */
int		tab_style = XT_TABS_NORMAL; /* tab bar style */
int		show_url = 1;	/* show url toolbar on notebook */
int		show_statusbar = 0; /* vimperator style status bar */
int		ctrl_click_focus = 0; /* ctrl click gets focus */
int		cookies_enabled = 1; /* enable cookies */
int		read_only_cookies = 0; /* enable to not write cookies */
int		enable_scripts = 1;
int		enable_plugins = 1;
gfloat		default_zoom_level = 1.0;
char		default_script[PATH_MAX];
int		window_height = 768;
int		window_width = 1024;
int		icon_size = 2; /* 1 = smallest, 2+ = bigger */
int		refresh_interval = 10; /* download refresh interval */
int		enable_plugin_whitelist = 0;
int		enable_cookie_whitelist = 0;
int		enable_js_whitelist = 0;
int		session_timeout = 3600; /* cookie session timeout */
int		cookie_policy = SOUP_COOKIE_JAR_ACCEPT_ALWAYS;
char		*ssl_ca_file = NULL;
char		*resource_dir = NULL;
gboolean	ssl_strict_certs = FALSE;
int		append_next = 1; /* append tab after current tab */
char		*home = NULL;
char		*search_string = NULL;
char		*http_proxy = NULL;
char		download_dir[PATH_MAX];
char		runtime_settings[PATH_MAX]; /* override of settings */
int		allow_volatile_cookies = 0;
int		save_global_history = 0; /* save global history to disk */
char		*user_agent = NULL;
int		save_rejected_cookies = 0;
int		session_autosave = 0;
int		guess_search = 0;
int		dns_prefetch = FALSE;
gint		max_connections = 25;
gint		max_host_connections = 5;
gint		enable_spell_checking = 0;
char		*spell_check_languages = NULL;
int		xterm_workaround = 0;
char		*url_regex = NULL;
int		history_autosave = 0;
char		search_file[PATH_MAX];
char		command_file[PATH_MAX];
char		*encoding = NULL;
int		autofocus_onload = 0;

char		*cmd_font_name = NULL;
char		*oops_font_name = NULL;
char		*statusbar_font_name = NULL;
char		*tabbar_font_name = NULL;
PangoFontDescription *cmd_font;
PangoFontDescription *oops_font;
PangoFontDescription *statusbar_font;
PangoFontDescription *tabbar_font;
char		*qmarks[XT_NOMARKS];

int		btn_down;	/* M1 down in any wv */
regex_t		url_re;		/* guess_search regex */

struct settings;
struct key_binding;
int		set_browser_mode(struct settings *, char *);
int		set_cookie_policy(struct settings *, char *);
int		set_download_dir(struct settings *, char *);
int		set_default_script(struct settings *, char *);
int		set_runtime_dir(struct settings *, char *);
int		set_tab_style(struct settings *, char *);
int		set_work_dir(struct settings *, char *);
int		add_alias(struct settings *, char *);
int		add_mime_type(struct settings *, char *);
int		add_cookie_wl(struct settings *, char *);
int		add_js_wl(struct settings *, char *);
int		add_pl_wl(struct settings *, char *);
int		add_kb(struct settings *, char *);
void		button_set_stockid(GtkWidget *, char *);
GtkWidget *	create_button(char *, char *, int);

char		*get_browser_mode(struct settings *);
char		*get_cookie_policy(struct settings *);
char		*get_download_dir(struct settings *);
char		*get_default_script(struct settings *);
char		*get_runtime_dir(struct settings *);
char		*get_tab_style(struct settings *);
char		*get_work_dir(struct settings *);
void		startpage_add(const char *, ...);

void		walk_alias(struct settings *, void (*)(struct settings *,
		    char *, void *), void *);
void		walk_cookie_wl(struct settings *, void (*)(struct settings *,
		    char *, void *), void *);
void		walk_js_wl(struct settings *, void (*)(struct settings *,
		    char *, void *), void *);
void		walk_pl_wl(struct settings *, void (*)(struct settings *,
		    char *, void *), void *);
void		walk_kb(struct settings *, void (*)(struct settings *, char *,
		    void *), void *);
void		walk_mime_type(struct settings *, void (*)(struct settings *,
		    char *, void *), void *);

void		recalc_tabs(void);
void		recolor_compact_tabs(void);
void		set_current_tab(int page_num);
gboolean	update_statusbar_position(GtkAdjustment*, gpointer);
void		marks_clear(struct tab *t);

int		set_http_proxy(char *);

struct special {
	int		(*set)(struct settings *, char *);
	char		*(*get)(struct settings *);
	void		(*walk)(struct settings *,
			    void (*cb)(struct settings *, char *, void *),
			    void *);
};

struct special		s_browser_mode = {
	set_browser_mode,
	get_browser_mode,
	NULL
};

struct special		s_cookie = {
	set_cookie_policy,
	get_cookie_policy,
	NULL
};

struct special		s_alias = {
	add_alias,
	NULL,
	walk_alias
};

struct special		s_mime = {
	add_mime_type,
	NULL,
	walk_mime_type
};

struct special		s_js = {
	add_js_wl,
	NULL,
	walk_js_wl
};

struct special		s_pl = {
	add_pl_wl,
	NULL,
	walk_pl_wl
};

struct special		s_kb = {
	add_kb,
	NULL,
	walk_kb
};

struct special		s_cookie_wl = {
	add_cookie_wl,
	NULL,
	walk_cookie_wl
};

struct special		s_default_script = {
	set_default_script,
	get_default_script,
	NULL
};

struct special		s_download_dir = {
	set_download_dir,
	get_download_dir,
	NULL
};

struct special		s_work_dir = {
	set_work_dir,
	get_work_dir,
	NULL
};

struct special		s_tab_style = {
	set_tab_style,
	get_tab_style,
	NULL
};

struct settings {
	char		*name;
	int		type;
#define XT_S_INVALID	(0)
#define XT_S_INT	(1)
#define XT_S_STR	(2)
#define XT_S_FLOAT	(3)
	uint32_t	flags;
#define XT_SF_RESTART	(1<<0)
#define XT_SF_RUNTIME	(1<<1)
	int		*ival;
	char		**sval;
	struct special	*s;
	gfloat		*fval;
	int		(*activate)(char *);
} rs[] = {
	{ "allow_volatile_cookies",	XT_S_INT, 0,		&allow_volatile_cookies, NULL, NULL },
	{ "append_next",		XT_S_INT, 0,		&append_next, NULL, NULL },
	{ "autofocus_onload",		XT_S_INT, 0,		&autofocus_onload, NULL, NULL },
	{ "browser_mode",		XT_S_INT, 0, NULL, NULL,&s_browser_mode },
	{ "cookie_policy",		XT_S_INT, 0, NULL, NULL,&s_cookie },
	{ "cookies_enabled",		XT_S_INT, 0,		&cookies_enabled, NULL, NULL },
	{ "ctrl_click_focus",		XT_S_INT, 0,		&ctrl_click_focus, NULL, NULL },
	{ "default_zoom_level",		XT_S_FLOAT, 0,		NULL, NULL, NULL, &default_zoom_level },
	{ "default_script",		XT_S_STR, 0, NULL, NULL,&s_default_script },
	{ "download_dir",		XT_S_STR, 0, NULL, NULL,&s_download_dir },
	{ "enable_cookie_whitelist",	XT_S_INT, 0,		&enable_cookie_whitelist, NULL, NULL },
	{ "enable_js_whitelist",	XT_S_INT, 0,		&enable_js_whitelist, NULL, NULL },
	{ "enable_plugin_whitelist",	XT_S_INT, 0,		&enable_plugin_whitelist, NULL, NULL },
	{ "enable_localstorage",	XT_S_INT, 0,		&enable_localstorage, NULL, NULL },
	{ "enable_plugins",		XT_S_INT, 0,		&enable_plugins, NULL, NULL },
	{ "enable_scripts",		XT_S_INT, 0,		&enable_scripts, NULL, NULL },
	{ "enable_socket",		XT_S_INT, XT_SF_RESTART,&enable_socket, NULL, NULL },
	{ "enable_spell_checking",	XT_S_INT, 0,		&enable_spell_checking, NULL, NULL },
	{ "encoding",			XT_S_STR, 0, NULL,	&encoding, NULL },
	{ "fancy_bar",			XT_S_INT, XT_SF_RESTART,&fancy_bar, NULL, NULL },
	{ "guess_search",		XT_S_INT, 0,		&guess_search, NULL, NULL },
	{ "history_autosave",		XT_S_INT, 0,		&history_autosave, NULL, NULL },
	{ "home",			XT_S_STR, 0, NULL,	&home, NULL },
	{ "http_proxy",			XT_S_STR, 0, NULL,	&http_proxy, NULL, NULL, set_http_proxy },
	{ "icon_size",			XT_S_INT, 0,		&icon_size, NULL, NULL },
	{ "max_connections",		XT_S_INT, XT_SF_RESTART,&max_connections, NULL, NULL },
	{ "max_host_connections",	XT_S_INT, XT_SF_RESTART,&max_host_connections, NULL, NULL },
	{ "read_only_cookies",		XT_S_INT, 0,		&read_only_cookies, NULL, NULL },
	{ "refresh_interval",		XT_S_INT, 0,		&refresh_interval, NULL, NULL },
	{ "resource_dir",		XT_S_STR, 0, NULL,	&resource_dir, NULL },
	{ "search_string",		XT_S_STR, 0, NULL,	&search_string, NULL },
	{ "save_global_history",	XT_S_INT, XT_SF_RESTART,&save_global_history, NULL, NULL },
	{ "save_rejected_cookies",	XT_S_INT, XT_SF_RESTART,&save_rejected_cookies, NULL, NULL },
	{ "session_timeout",		XT_S_INT, 0,		&session_timeout, NULL, NULL },
	{ "session_autosave",		XT_S_INT, 0,		&session_autosave, NULL, NULL },
	{ "single_instance",		XT_S_INT, XT_SF_RESTART,&single_instance, NULL, NULL },
	{ "show_tabs",			XT_S_INT, 0,		&show_tabs, NULL, NULL },
	{ "show_url",			XT_S_INT, 0,		&show_url, NULL, NULL },
	{ "show_statusbar",		XT_S_INT, 0,		&show_statusbar, NULL, NULL },
	{ "spell_check_languages",	XT_S_STR, 0, NULL, &spell_check_languages, NULL },
	{ "ssl_ca_file",		XT_S_STR, 0, NULL,	&ssl_ca_file, NULL },
	{ "ssl_strict_certs",		XT_S_INT, 0,		&ssl_strict_certs, NULL, NULL },
	{ "statusbar_elems",		XT_S_STR, 0, NULL,	&statusbar_elems, NULL },
	{ "tab_style",			XT_S_STR, 0, NULL, NULL,&s_tab_style },
	{ "url_regex",			XT_S_STR, 0, NULL,	&url_regex, NULL },
	{ "user_agent",			XT_S_STR, 0, NULL,	&user_agent, NULL },
	{ "window_height",		XT_S_INT, 0,		&window_height, NULL, NULL },
	{ "window_width",		XT_S_INT, 0,		&window_width, NULL, NULL },
	{ "work_dir",			XT_S_STR, 0, NULL, NULL,&s_work_dir },
	{ "xterm_workaround",		XT_S_INT, 0,		&xterm_workaround, NULL, NULL },

	/* font settings */
	{ "cmd_font",			XT_S_STR, 0, NULL, &cmd_font_name, NULL },
	{ "oops_font",			XT_S_STR, 0, NULL, &oops_font_name, NULL },
	{ "statusbar_font",		XT_S_STR, 0, NULL, &statusbar_font_name, NULL },
	{ "tabbar_font",		XT_S_STR, 0, NULL, &tabbar_font_name, NULL },

	/* runtime settings */
	{ "alias",			XT_S_STR, XT_SF_RUNTIME, NULL, NULL, &s_alias },
	{ "cookie_wl",			XT_S_STR, XT_SF_RUNTIME, NULL, NULL, &s_cookie_wl },
	{ "js_wl",			XT_S_STR, XT_SF_RUNTIME, NULL, NULL, &s_js },
	{ "keybinding",			XT_S_STR, XT_SF_RUNTIME, NULL, NULL, &s_kb },
	{ "mime_type",			XT_S_STR, XT_SF_RUNTIME, NULL, NULL, &s_mime },
	{ "pl_wl",			XT_S_STR, XT_SF_RUNTIME, NULL, NULL, &s_pl },
};

/* globals */
extern char		*__progname;
char			**start_argv;
struct passwd		*pwd;
GtkWidget		*main_window;
GtkNotebook		*notebook;
GtkWidget		*tab_bar;
GtkWidget		*arrow, *abtn;
struct tab_list		tabs;
struct history_list	hl;
struct session_list	sessions;
struct domain_list	c_wl;
struct domain_list	js_wl;
struct domain_list	pl_wl;
struct undo_tailq	undos;
struct keybinding_list	kbl;
struct sp_list		spl;
struct command_list	chl;
struct command_list	shl;
struct command_entry	*history_at;
struct command_entry	*search_at;
int			undo_count;
int			cmd_history_count = 0;
int			search_history_count = 0;
char			*global_search;
long long unsigned int	blocked_cookies = 0;
char			named_session[PATH_MAX];
GtkListStore		*completion_model;
GtkListStore		*buffers_store;

/* starts from 1 to catch atoi() failures when calling xtp_handle_dl() */
int			next_download_id = 1;

void			xxx_dir(char *);
int			icon_size_map(int);
void			completion_add(struct tab *);
void			completion_add_uri(const gchar *);

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

/* marks and quickmarks array storage.
 * first a-z, then A-Z, then 0-9 */
char
indextomark(int i)
{
	if (i < 0)
		return (0);

	if (i >= 0 && i <= 'z' - 'a')
		return 'a' + i;

	i -= 'z' - 'a' + 1;
	if (i >= 0 && i <= 'Z' - 'A')
		return 'A' + i;

	i -= 'Z' - 'A' + 1;
	if (i >= 10)
		return (0);

	return i + '0';
}

int
marktoindex(char m)
{
	int ret = 0;

	if (m >= 'a' && m <= 'z')
		return ret + m - 'a';

	ret += 'z' - 'a' + 1;
	if (m >= 'A' && m <= 'Z')
		return ret + m - 'A';

	ret += 'Z' - 'A' + 1;
	if (m >= '0' && m <= '9')
		return ret + m - '0';

	return (-1);
}


void
sigchild(int sig)
{
	int			saved_errno, status;
	pid_t			pid;

	saved_errno = errno;

	while ((pid = waitpid(WAIT_ANY, &status, WNOHANG)) != 0) {
		if (pid == -1) {
			if (errno == EINTR)
				continue;
			if (errno != ECHILD) {
				/*
				clog_warn("sigchild: waitpid:");
				*/
			}
			break;
		}

		if (WIFEXITED(status)) {
			if (WEXITSTATUS(status) != 0) {
				/*
				clog_warnx("sigchild: child exit status: %d",
				    WEXITSTATUS(status));
				*/
			}
		} else {
			/*
			clog_warnx("sigchild: child is terminated abnormally");
			*/
		}
	}

	errno = saved_errno;
}

int
is_g_object_setting(GObject *o, char *str)
{
	guint			n_props = 0, i;
	GParamSpec		**proplist;

	if (! G_IS_OBJECT(o))
		return (0);

	proplist = g_object_class_list_properties(G_OBJECT_GET_CLASS(o),
	    &n_props);

	for (i=0; i < n_props; i++) {
		if (! strcmp(proplist[i]->name, str))
			return (1);
	}
	return (0);
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

void
set_status(struct tab *t, gchar *s, int status)
{
	gchar *type = NULL;

	if (s == NULL)
		return;

	switch (status) {
	case XT_STATUS_LOADING:
		type = g_strdup_printf("Loading: %s", s);
		s = type;
		break;
	case XT_STATUS_LINK:
		type = g_strdup_printf("Link: %s", s);
		if (!t->status)
			t->status = g_strdup(gtk_entry_get_text(
			    GTK_ENTRY(t->sbe.statusbar)));
		s = type;
		break;
	case XT_STATUS_URI:
		type = g_strdup_printf("%s", s);
		if (!t->status) {
			t->status = g_strdup(type);
		}
		s = type;
		if (!t->status)
			t->status = g_strdup(s);
		break;
	case XT_STATUS_NOTHING:
		/* FALL THROUGH */
	default:
		break;
	}
	gtk_entry_set_text(GTK_ENTRY(t->sbe.statusbar), s);
	if (type)
		g_free(type);
}

void
hide_cmd(struct tab *t)
{
	history_at = NULL; /* just in case */
	search_at = NULL; /* just in case */
	gtk_widget_hide(t->cmd);
}

void
show_cmd(struct tab *t)
{
	history_at = NULL;
	search_at = NULL;
	gtk_widget_hide(t->oops);
	gtk_widget_show(t->cmd);
}

void
hide_buffers(struct tab *t)
{
	gtk_widget_hide(t->buffers);
	gtk_list_store_clear(buffers_store);
}

enum {
	COL_ID		= 0,
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
			    COL_TITLE, title,
			    -1);
		}

	g_free(stabs);
}

void
show_buffers(struct tab *t)
{
	buffers_make_list();
	gtk_widget_show(t->buffers);
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
	if (vasprintf(&msg, fmt, ap) == -1)
		errx(1, "show_oops failed");
	va_end(ap);

	gtk_entry_set_text(GTK_ENTRY(t->oops), msg);
	gtk_widget_hide(t->cmd);
	gtk_widget_show(t->oops);

	if (msg)
		free(msg);
}

char *
get_as_string(struct settings *s)
{
	char			*r = NULL;

	if (s == NULL)
		return (NULL);

	if (s->s) {
		if (s->s->get)
			r = s->s->get(s);
		else
			warnx("get_as_string skip %s\n", s->name);
	} else if (s->type == XT_S_INT)
		r = g_strdup_printf("%d", *s->ival);
	else if (s->type == XT_S_STR)
		r = g_strdup(*s->sval);
	else if (s->type == XT_S_FLOAT)
		r = g_strdup_printf("%f", *s->fval);
	else
		r = g_strdup_printf("INVALID TYPE");

	return (r);
}

void
settings_walk(void (*cb)(struct settings *, char *, void *), void *cb_args)
{
	int			i;
	char			*s;

	for (i = 0; i < LENGTH(rs); i++) {
		if (rs[i].s && rs[i].s->walk)
			rs[i].s->walk(&rs[i], cb, cb_args);
		else {
			s = get_as_string(&rs[i]);
			cb(&rs[i], s, cb_args);
			g_free(s);
		}
	}
}

int
set_browser_mode(struct settings *s, char *val)
{
	if (!strcmp(val, "whitelist")) {
		browser_mode = XT_BM_WHITELIST;
		allow_volatile_cookies	= 0;
		cookie_policy = SOUP_COOKIE_JAR_ACCEPT_NO_THIRD_PARTY;
		cookies_enabled = 1;
		enable_cookie_whitelist = 1;
		enable_plugin_whitelist = 1;
		enable_plugins = 0;
		read_only_cookies = 0;
		save_rejected_cookies = 0;
		session_timeout = 3600;
		enable_scripts = 0;
		enable_js_whitelist = 1;
		enable_localstorage = 0;
	} else if (!strcmp(val, "normal")) {
		browser_mode = XT_BM_NORMAL;
		allow_volatile_cookies = 0;
		cookie_policy = SOUP_COOKIE_JAR_ACCEPT_ALWAYS;
		cookies_enabled = 1;
		enable_cookie_whitelist = 0;
		enable_plugin_whitelist = 0;
		enable_plugins = 1;
		read_only_cookies = 0;
		save_rejected_cookies = 0;
		session_timeout = 3600;
		enable_scripts = 1;
		enable_js_whitelist = 0;
		enable_localstorage = 1;
	} else if (!strcmp(val, "kiosk")) {
		browser_mode = XT_BM_KIOSK;
		allow_volatile_cookies = 0;
		cookie_policy = SOUP_COOKIE_JAR_ACCEPT_ALWAYS;
		cookies_enabled = 1;
		enable_cookie_whitelist = 0;
		enable_plugin_whitelist = 0;
		enable_plugins = 1;
		read_only_cookies = 0;
		save_rejected_cookies = 0;
		session_timeout = 3600;
		enable_scripts = 1;
		enable_js_whitelist = 0;
		enable_localstorage = 1;
		show_tabs = 0;
		tabless = 1;
	} else
		return (1);

	return (0);
}

char *
get_browser_mode(struct settings *s)
{
	char			*r = NULL;

	if (browser_mode == XT_BM_WHITELIST)
		r = g_strdup("whitelist");
	else if (browser_mode == XT_BM_NORMAL)
		r = g_strdup("normal");
	else if (browser_mode == XT_BM_KIOSK)
		r = g_strdup("kiosk");
	else
		return (NULL);

	return (r);
}

int
set_cookie_policy(struct settings *s, char *val)
{
	if (!strcmp(val, "no3rdparty"))
		cookie_policy = SOUP_COOKIE_JAR_ACCEPT_NO_THIRD_PARTY;
	else if (!strcmp(val, "accept"))
		cookie_policy = SOUP_COOKIE_JAR_ACCEPT_ALWAYS;
	else if (!strcmp(val, "reject"))
		cookie_policy = SOUP_COOKIE_JAR_ACCEPT_NEVER;
	else
		return (1);

	return (0);
}

char *
get_cookie_policy(struct settings *s)
{
	char			*r = NULL;

	if (cookie_policy == SOUP_COOKIE_JAR_ACCEPT_NO_THIRD_PARTY)
		r = g_strdup("no3rdparty");
	else if (cookie_policy == SOUP_COOKIE_JAR_ACCEPT_ALWAYS)
		r = g_strdup("accept");
	else if (cookie_policy == SOUP_COOKIE_JAR_ACCEPT_NEVER)
		r = g_strdup("reject");
	else
		return (NULL);

	return (r);
}

char *
get_default_script(struct settings *s)
{
	if (default_script[0] == '\0')
		return (0);
	return (g_strdup(default_script));
}

int
set_default_script(struct settings *s, char *val)
{
	if (val[0] == '~')
		snprintf(default_script, sizeof default_script, "%s/%s",
		    pwd->pw_dir, &val[1]);
	else
		strlcpy(default_script, val, sizeof default_script);

	return (0);
}

char *
get_download_dir(struct settings *s)
{
	if (download_dir[0] == '\0')
		return (0);
	return (g_strdup(download_dir));
}

int
set_download_dir(struct settings *s, char *val)
{
	if (val[0] == '~')
		snprintf(download_dir, sizeof download_dir, "%s/%s",
		    pwd->pw_dir, &val[1]);
	else
		strlcpy(download_dir, val, sizeof download_dir);

	return (0);
}

char			work_dir[PATH_MAX];
char			certs_dir[PATH_MAX];
char			cache_dir[PATH_MAX];
char			sessions_dir[PATH_MAX];
char			cookie_file[PATH_MAX];
SoupURI			*proxy_uri = NULL;
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
int			run_script(struct tab *, char *);
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

char *
get_work_dir(struct settings *s)
{
	if (work_dir[0] == '\0')
		return (0);
	return (g_strdup(work_dir));
}

int
set_work_dir(struct settings *s, char *val)
{
	if (val[0] == '~')
		snprintf(work_dir, sizeof work_dir, "%s/%s",
		    pwd->pw_dir, &val[1]);
	else
		strlcpy(work_dir, val, sizeof work_dir);

	return (0);
}

char *
get_tab_style(struct settings *s)
{
	if (tab_style == XT_TABS_NORMAL)
		return (g_strdup("normal"));
	else
		return (g_strdup("compact"));
}

int
set_tab_style(struct settings *s, char *val)
{
	if (!strcmp(val, "normal"))
		tab_style = XT_TABS_NORMAL;
	else if (!strcmp(val, "compact"))
		tab_style = XT_TABS_COMPACT;
	else
		return (1);

	return (0);
}

int
download_rb_cmp(struct download *e1, struct download *e2)
{
	return (e1->id < e2->id ? -1 : e1->id > e2->id);
}
RB_GENERATE(download_list, download, entry, download_rb_cmp);

struct valid_url_types {
	char		*type;
} vut[] = {
	{ "http://" },
	{ "https://" },
	{ "ftp://" },
	{ "file://" },
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

void
print_cookie(char *msg, SoupCookie *c)
{
	if (c == NULL)
		return;

	if (msg)
		DNPRINTF(XT_D_COOKIE, "%s\n", msg);
	DNPRINTF(XT_D_COOKIE, "name     : %s\n", c->name);
	DNPRINTF(XT_D_COOKIE, "value    : %s\n", c->value);
	DNPRINTF(XT_D_COOKIE, "domain   : %s\n", c->domain);
	DNPRINTF(XT_D_COOKIE, "path     : %s\n", c->path);
	DNPRINTF(XT_D_COOKIE, "expires  : %s\n",
	    c->expires ? soup_date_to_string(c->expires, SOUP_DATE_HTTP) : "");
	DNPRINTF(XT_D_COOKIE, "secure   : %d\n", c->secure);
	DNPRINTF(XT_D_COOKIE, "http_only: %d\n", c->http_only);
	DNPRINTF(XT_D_COOKIE, "====================================\n");
}

void
walk_alias(struct settings *s,
    void (*cb)(struct settings *, char *, void *), void *cb_args)
{
	struct alias		*a;
	char			*str;

	if (s == NULL || cb == NULL) {
		show_oops(NULL, "walk_alias invalid parameters");
		return;
	}

	TAILQ_FOREACH(a, &aliases, entry) {
		str = g_strdup_printf("%s --> %s", a->a_name, a->a_uri);
		cb(s, str, cb_args);
		g_free(str);
	}
}

char *
match_alias(char *url_in)
{
	struct alias		*a;
	char			*arg;
	char			*url_out = NULL, *search, *enc_arg;

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
		if (arg != NULL) {
			enc_arg = soup_uri_encode(arg, XT_RESERVED_CHARS);
			url_out = g_strdup_printf(a->a_uri, enc_arg);
			g_free(enc_arg);
		} else
			url_out = g_strdup_printf(a->a_uri, "");
	}
done:
	g_free(search);
	return (url_out);
}

char *
guess_url_type(char *url_in)
{
	struct stat		sb;
	char			*url_out = NULL, *enc_search = NULL;
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
			url_out = g_strdup_printf(search_string, enc_search);
			g_free(enc_search);
			goto done;
		}
	}

	/* XXX not sure about this heuristic */
	if (stat(url_in, &sb) == 0)
		url_out = g_strdup_printf("file://%s", url_in);
	else
		url_out = g_strdup_printf("http://%s", url_in); /* guess http */
done:
	DNPRINTF(XT_D_URL, "guess_url_type: guessed %s\n", url_out);

	return (url_out);
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

	t->xtp_meaning = XT_XTP_TAB_MEANING_NORMAL;

	if (valid_url_type(uri)) {
		newuri = guess_url_type(uri);
		uri = newuri;
	}

	if (!strncmp(uri, XT_URI_ABOUT, XT_URI_ABOUT_LEN)) {
		for (i = 0; i < about_list_size(); i++)
			if (!strcmp(&uri[XT_URI_ABOUT_LEN], about_list[i].name)) {
				bzero(&args, sizeof args);
				about_list[i].func(t, &args);
				gtk_widget_set_sensitive(GTK_WIDGET(t->stop),
				    FALSE);
				goto done;
			}
		show_oops(t, "invalid about page");
		goto done;
	}

	set_status(t, (char *)uri, XT_STATUS_LOADING);
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

	if (webkit_web_view_get_load_status(t->wv) == WEBKIT_LOAD_FAILED)
		return NULL;
	if (t->xtp_meaning == XT_XTP_TAB_MEANING_NORMAL) {
		uri = webkit_web_view_get_uri(t->wv);
	} else {
		/* use tmp_uri to make sure it is g_freed */
		if (t->tmp_uri)
			g_free(t->tmp_uri);
		t->tmp_uri =g_strdup_printf("%s%s", XT_URI_ABOUT,
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

	if (status == WEBKIT_LOAD_PROVISIONAL || status == WEBKIT_LOAD_FAILED ||
	    t->xtp_meaning == XT_XTP_TAB_MEANING_BL)
		goto notitle;

	title = webkit_web_view_get_title(t->wv);
	if ((set = title ? title : get_uri(t)))
		return set;

notitle:
	set = window ? XT_NAME : "(untitled)";

	return set;
}

int
add_alias(struct settings *s, char *line)
{
	char			*l, *alias;
	struct alias		*a = NULL;

	if (s == NULL || line == NULL) {
		show_oops(NULL, "add_alias invalid parameters");
		return (1);
	}

	l = line;
	a = g_malloc(sizeof(*a));

	if ((alias = strsep(&l, " \t,")) == NULL || l == NULL) {
		show_oops(NULL, "add_alias: incomplete alias definition");
		goto bad;
	}
	if (strlen(alias) == 0 || strlen(l) == 0) {
		show_oops(NULL, "add_alias: invalid alias definition");
		goto bad;
	}

	a->a_name = g_strdup(alias);
	a->a_uri = g_strdup(l);

	DNPRINTF(XT_D_CONFIG, "add_alias: %s for %s\n", a->a_name, a->a_uri);

	TAILQ_INSERT_TAIL(&aliases, a, entry);

	return (0);
bad:
	if (a)
		g_free(a);
	return (1);
}

int
add_mime_type(struct settings *s, char *line)
{
	char			*mime_type;
	char			*l;
	struct mime_type	*m = NULL;
	int			downloadfirst = 0;

	/* XXX this could be smarter */

	if (line == NULL || strlen(line) == 0) {
		show_oops(NULL, "add_mime_type invalid parameters");
		return (1);
	}

	l = line;
	if (*l == '@') {
		downloadfirst = 1;
		l++;
	}
	m = g_malloc(sizeof(*m));

	if ((mime_type = strsep(&l, " \t,")) == NULL || l == NULL) {
		show_oops(NULL, "add_mime_type: invalid mime_type");
		goto bad;
	}
	if (mime_type[strlen(mime_type) - 1] == '*') {
		mime_type[strlen(mime_type) - 1] = '\0';
		m->mt_default = 1;
	} else
		m->mt_default = 0;

	if (strlen(mime_type) == 0 || strlen(l) == 0) {
		show_oops(NULL, "add_mime_type: invalid mime_type");
		goto bad;
	}

	m->mt_type = g_strdup(mime_type);
	m->mt_action = g_strdup(l);
	m->mt_download = downloadfirst;

	DNPRINTF(XT_D_CONFIG, "add_mime_type: type %s action %s default %d\n",
	    m->mt_type, m->mt_action, m->mt_default);

	TAILQ_INSERT_TAIL(&mtl, m, entry);

	return (0);
bad:
	if (m)
		g_free(m);
	return (1);
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

void
walk_mime_type(struct settings *s,
    void (*cb)(struct settings *, char *, void *), void *cb_args)
{
	struct mime_type	*m;
	char			*str;

	if (s == NULL || cb == NULL) {
		show_oops(NULL, "walk_mime_type invalid parameters");
		return;
	}

	TAILQ_FOREACH(m, &mtl, entry) {
		str = g_strdup_printf("%s%s --> %s",
		    m->mt_type,
		    m->mt_default ? "*" : "",
		    m->mt_action);
		cb(s, str, cb_args);
		g_free(str);
	}
}

void
wl_add(char *str, struct domain_list *wl, int handy)
{
	struct domain		*d;
	int			add_dot = 0;
	char			*p;

	if (str == NULL || wl == NULL || strlen(str) < 2)
		return;

	DNPRINTF(XT_D_COOKIE, "wl_add in: %s\n", str);

	/* treat *.moo.com the same as .moo.com */
	if (str[0] == '*' && str[1] == '.')
		str = &str[1];
	else if (str[0] == '.')
		str = &str[0];
	else
		add_dot = 1;

	/* slice off port number */
	p = g_strrstr(str, ":");
	if (p)
		*p = '\0';

	d = g_malloc(sizeof *d);
	if (add_dot)
		d->d = g_strdup_printf(".%s", str);
	else
		d->d = g_strdup(str);
	d->handy = handy;

	if (RB_INSERT(domain_list, wl, d))
		goto unwind;

	DNPRINTF(XT_D_COOKIE, "wl_add: %s\n", d->d);
	return;
unwind:
	if (d) {
		if (d->d)
			g_free(d->d);
		g_free(d);
	}
}

int
add_cookie_wl(struct settings *s, char *entry)
{
	wl_add(entry, &c_wl, 1);
	return (0);
}

void
walk_cookie_wl(struct settings *s,
    void (*cb)(struct settings *, char *, void *), void *cb_args)
{
	struct domain		*d;

	if (s == NULL || cb == NULL) {
		show_oops(NULL, "walk_cookie_wl invalid parameters");
		return;
	}

	RB_FOREACH_REVERSE(d, domain_list, &c_wl)
		cb(s, d->d, cb_args);
}

void
walk_js_wl(struct settings *s,
    void (*cb)(struct settings *, char *, void *), void *cb_args)
{
	struct domain		*d;

	if (s == NULL || cb == NULL) {
		show_oops(NULL, "walk_js_wl invalid parameters");
		return;
	}

	RB_FOREACH_REVERSE(d, domain_list, &js_wl)
		cb(s, d->d, cb_args);
}

int
add_js_wl(struct settings *s, char *entry)
{
	wl_add(entry, &js_wl, 1 /* persistent */);
	return (0);
}

void
walk_pl_wl(struct settings *s,
    void (*cb)(struct settings *, char *, void *), void *cb_args)
{
	struct domain		*d;

	if (s == NULL || cb == NULL) {
		show_oops(NULL, "walk_pl_wl invalid parameters");
		return;
	}

	RB_FOREACH_REVERSE(d, domain_list, &pl_wl)
		cb(s, d->d, cb_args);
}

int
add_pl_wl(struct settings *s, char *entry)
{
	wl_add(entry, &pl_wl, 1 /* persistent */);
	return (0);
}

struct domain *
wl_find(const gchar *search, struct domain_list *wl)
{
	int			i;
	struct domain		*d = NULL, dfind;
	gchar			*s = NULL;

	if (search == NULL || wl == NULL)
		return (NULL);
	if (strlen(search) < 2)
		return (NULL);

	if (search[0] != '.')
		s = g_strdup_printf(".%s", search);
	else
		s = g_strdup(search);

	for (i = strlen(s) - 1; i >= 0; i--) {
		if (s[i] == '.') {
			dfind.d = &s[i];
			d = RB_FIND(domain_list, wl, &dfind);
			if (d)
				goto done;
		}
	}

done:
	if (s)
		g_free(s);

	return (d);
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

int
settings_add(char *var, char *val)
{
	int i, rv, *p;
	gfloat *f;
	char **s;

	/* get settings */
	for (i = 0, rv = 0; i < LENGTH(rs); i++) {
		if (strcmp(var, rs[i].name))
			continue;

		if (rs[i].s) {
			if (rs[i].s->set(&rs[i], val))
				errx(1, "invalid value for %s: %s", var, val);
			rv = 1;
			break;
		} else
			switch (rs[i].type) {
			case XT_S_INT:
				p = rs[i].ival;
				*p = atoi(val);
				rv = 1;
				break;
			case XT_S_STR:
				s = rs[i].sval;
				if (s == NULL)
					errx(1, "invalid sval for %s",
					    rs[i].name);
				if (*s)
					g_free(*s);
				*s = g_strdup(val);
				rv = 1;
				break;
			case XT_S_FLOAT:
				f = rs[i].fval;
				*f = atof(val);
				rv = 1;
				break;
			case XT_S_INVALID:
			default:
				errx(1, "invalid type for %s", var);
			}
		break;
	}
	return (rv);
}

#define WS	"\n= \t"
void
config_parse(char *filename, int runtime)
{
	FILE			*config, *f;
	char			*line, *cp, *var, *val;
	size_t			len, lineno = 0;
	int			handled;
	char			file[PATH_MAX];
	struct stat		sb;

	DNPRINTF(XT_D_CONFIG, "config_parse: filename %s\n", filename);

	if (filename == NULL)
		return;

	if (runtime && runtime_settings[0] != '\0') {
		snprintf(file, sizeof file, "%s/%s",
		    work_dir, runtime_settings);
		if (stat(file, &sb)) {
			warnx("runtime file doesn't exist, creating it");
			if ((f = fopen(file, "w")) == NULL)
				err(1, "runtime");
			fprintf(f, "# AUTO GENERATED, DO NOT EDIT\n");
			fclose(f);
		}
	} else
		strlcpy(file, filename, sizeof file);

	if ((config = fopen(file, "r")) == NULL) {
		warn("config_parse: cannot open %s", filename);
		return;
	}

	for (;;) {
		if ((line = fparseln(config, &len, &lineno, NULL, 0)) == NULL)
			if (feof(config) || ferror(config))
				break;

		cp = line;
		cp += (long)strspn(cp, WS);
		if (cp[0] == '\0') {
			/* empty line */
			free(line);
			continue;
		}

		if ((var = strsep(&cp, WS)) == NULL || cp == NULL)
			startpage_add("invalid configuration file entry: %s",
			    line);
		else {
			cp += (long)strspn(cp, WS);

			if ((val = strsep(&cp, "\0")) == NULL)
				break;

			DNPRINTF(XT_D_CONFIG, "config_parse: %s=%s\n",
			    var, val);
			handled = settings_add(var, val);

			if (handled == 0)
				startpage_add("invalid configuration file entry"
				    ": %s=%s", var, val);
		}

		free(line);
	}

	fclose(config);
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

void
disable_hints(struct tab *t)
{
	DNPRINTF(XT_D_JS, "%s\n", __func__);

	run_script(t, "hints.clearHints();");
	t->hints_on = 0;
	t->new_tab = 0;
}

void
enable_hints(struct tab *t)
{
	DNPRINTF(XT_D_JS, "%s\n", __func__);

	run_script(t, JS_HINTING);

	if (t->new_tab)
		run_script(t, "hints.createHints('', 'F');");
	else
		run_script(t, "hints.createHints('', 'f');");
	t->hints_on = 1;
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

	DNPRINTF(XT_D_JS, "run_script: tab %d %s\n",
	    t->tab_id, s == (char *)JS_HINTING ? "JS_HINTING" : s);

	/* a bit silly but it prevents some heartburn */
	if (t->script_init == 0) {
		t->script_init = 1;
		run_script(t, JS_HINTING);
	}

	frame = webkit_web_view_get_main_frame(t->wv);
	ctx = webkit_web_frame_get_global_context(frame);

	str = JSStringCreateWithUTF8CString(s);
	val = JSEvaluateScript(ctx, str, JSContextGetGlobalObject(ctx),
	    NULL, 0, &exception);
	JSStringRelease(str);

	DNPRINTF(XT_D_JS, "run_script: val %p\n", val);
	if (val == NULL) {
		es = js_ref_to_string(ctx, exception);
		if (es) {
			/* show_oops(t, "script exception: %s", es); */
			DNPRINTF(XT_D_JS, "run_script: exception %s\n", es);
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
			DNPRINTF(XT_D_JS, "run_script: val %s\n", es);
			g_free(es);
		}
	}

	return (0);
}

int
hint(struct tab *t, struct karg *args)
{

	DNPRINTF(XT_D_JS, "hint: tab %d args %d\n", t->tab_id, args->i);

	if (t->hints_on == 0) {
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
	g_object_set(G_OBJECT(t->settings),
	    "user-stylesheet-uri", t->stylesheet, (char *)NULL);
}

int
userstyle(struct tab *t, struct karg *args)
{
	DNPRINTF(XT_D_JS, "userstyle: tab %d\n", t->tab_id);

	if (t->styled) {
		t->styled = 0;
		g_object_set(G_OBJECT(t->settings),
		    "user-stylesheet-uri", NULL, (char *)NULL);
	} else {
		t->styled = 1;
		apply_style(t);
	}
	return (0);
}

/*
 * Doesn't work fully, due to the following bug:
 * https://bugs.webkit.org/show_bug.cgi?id=51747
 */
int
restore_global_history(void)
{
	char			file[PATH_MAX];
	FILE			*f;
	struct history		*h;
	gchar			*uri;
	gchar			*title;
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
				free(uri);
				warnx("%s: broken history file\n", __func__);
				return (1);
			}

		if (uri && strlen(uri) && title && strlen(title)) {
			webkit_web_history_item_new_with_data(uri, title);
			h = g_malloc(sizeof(struct history));
			h->uri = g_strdup(uri);
			h->title = g_strdup(title);
			RB_INSERT(history_list, &hl, h);
			completion_add_uri(h->uri);
		} else {
			warnx("%s: failed to restore history\n", __func__);
			free(uri);
			free(title);
			return (1);
		}

		free(uri);
		free(title);
		uri = NULL;
		title = NULL;
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
		if (h->uri && h->title)
			fprintf(f, "%s\n%s\n", h->uri, h->title);
	}

	fclose(f);

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
	DIR *sdir               = NULL;
	struct dirent *dp       = NULL;
	struct session		*s;

	sdir = opendir(sessions_dir);
	if (sdir) {
		while ((dp = readdir(sdir)) != NULL)
			if (dp->d_type == DT_REG) {
				s = g_malloc(sizeof(struct session));
				s->name = g_strdup(dp->d_name);
				TAILQ_INSERT_TAIL(&sessions, s, entry);
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

	snprintf(file, sizeof file, "%s/%s", sessions_dir, a->s);
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

	snprintf(file, sizeof file, "%s/%s",
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
		snprintf(file, sizeof file, "%s/%s",
		    sessions_dir, named_session);
	else
		snprintf(file, sizeof file, "%s/%s", sessions_dir, a->s);

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

int
run_page_script(struct tab *t, struct karg *args)
{
	const gchar		*uri;
	char			*tmp, script[PATH_MAX];

	tmp = args->s != NULL && strlen(args->s) > 0 ? args->s : default_script;
	if (tmp[0] == '\0') {
		show_oops(t, "no script specified");
		return (1);
	}

	if ((uri = get_uri(t)) == NULL) {
		show_oops(t, "tab is empty, not running script");
		return (1);
	}

	if (tmp[0] == '~')
		snprintf(script, sizeof script, "%s/%s",
		    pwd->pw_dir, &tmp[1]);
	else
		strlcpy(script, tmp, sizeof script);

	switch (fork()) {
	case -1:
		show_oops(t, "can't fork to run script");
		return (1);
		/* NOTREACHED */
	case 0:
		break;
	default:
		return (0);
	}

	/* child */
	execlp(script, script, uri, (void *)NULL);

	_exit(0);

	/* NOTREACHED */

	return (0);
}

int
yank_uri(struct tab *t, struct karg *args)
{
	const gchar		*uri;
	GtkClipboard		*clipboard;

	if ((uri = get_uri(t)) == NULL)
		return (1);

	clipboard = gtk_clipboard_get(GDK_SELECTION_PRIMARY);
	gtk_clipboard_set_text(clipboard, uri, -1);

	return (0);
}

int
paste_uri(struct tab *t, struct karg *args)
{
	GtkClipboard		*clipboard;
	GdkAtom			atom = gdk_atom_intern("CUT_BUFFER0", FALSE);
	gint			len;
	gchar			*p = NULL, *uri;

	/* try primary clipboard first */
	clipboard = gtk_clipboard_get(GDK_SELECTION_PRIMARY);
	p = gtk_clipboard_wait_for_text(clipboard);

	/* if it failed get whatever text is in cut_buffer0 */
	if (p == NULL && xterm_workaround)
		if (gdk_property_get(gdk_get_default_root_window(),
		    atom,
		    gdk_atom_intern("STRING", FALSE),
		    0,
		    1024 * 1024 /* picked out of my butt */,
		    FALSE,
		    NULL,
		    NULL,
		    &len,
		    (guchar **)&p)) {
			/* yes sir, we need to NUL the string */
			p[len] = '\0';
		}

	if (p) {
		uri = p;
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
	if (p)
		g_free(p);

	return (0);
}

gchar *
find_domain(const gchar *s, int toplevel)
{
	SoupURI			*uri;
	gchar			*ret, *p;

	if (s == NULL)
		return (NULL);

	uri = soup_uri_new(s);

	if (uri == NULL || !SOUP_URI_VALID_FOR_HTTP(uri)) {
		return (NULL);
	}

	if (toplevel && !isdigit(uri->host[strlen(uri->host) - 1])) {
		if ((p = strrchr(uri->host, '.')) != NULL) {
			while(--p >= uri->host && *p != '.');
			p++;
		} else
			p = uri->host;
	} else
		p = uri->host;

	ret = g_strdup_printf(".%s", p);

	soup_uri_free(uri);

	return (ret);
}

int
toggle_cwl(struct tab *t, struct karg *args)
{
	struct domain		*d;
	const gchar		*uri;
	char			*dom = NULL;
	int			es;

	if (args == NULL)
		return (1);

	uri = get_uri(t);
	dom = find_domain(uri, args->i & XT_WL_TOPLEVEL);

	if (uri == NULL || dom == NULL ||
	    webkit_web_view_get_load_status(t->wv) == WEBKIT_LOAD_FAILED) {
		show_oops(t, "Can't toggle domain in cookie white list");
		goto done;
	}
	d = wl_find(dom, &c_wl);

	if (d == NULL)
		es = 0;
	else
		es = 1;

	if (args->i & XT_WL_TOGGLE)
		es = !es;
	else if ((args->i & XT_WL_ENABLE) && es != 1)
		es = 1;
	else if ((args->i & XT_WL_DISABLE) && es != 0)
		es = 0;

	if (es)
		/* enable cookies for domain */
		wl_add(dom, &c_wl, 0);
	else {
		/* disable cookies for domain */
		if (d)
			RB_REMOVE(domain_list, &c_wl, d);
	}

	if (args->i & XT_WL_RELOAD)
		webkit_web_view_reload(t->wv);

done:
	g_free(dom);
	return (0);
}

int
toggle_js(struct tab *t, struct karg *args)
{
	int			es;
	const gchar		*uri;
	struct domain		*d;
	char			*dom = NULL;

	if (args == NULL)
		return (1);

	g_object_get(G_OBJECT(t->settings),
	    "enable-scripts", &es, (char *)NULL);
	if (args->i & XT_WL_TOGGLE)
		es = !es;
	else if ((args->i & XT_WL_ENABLE) && es != 1)
		es = 1;
	else if ((args->i & XT_WL_DISABLE) && es != 0)
		es = 0;
	else
		return (1);

	uri = get_uri(t);
	dom = find_domain(uri, args->i & XT_WL_TOPLEVEL);

	if (uri == NULL || dom == NULL ||
	    webkit_web_view_get_load_status(t->wv) == WEBKIT_LOAD_FAILED) {
		show_oops(t, "Can't toggle domain in JavaScript white list");
		goto done;
	}

	if (es) {
		button_set_stockid(t->js_toggle, GTK_STOCK_MEDIA_PLAY);
		wl_add(dom, &js_wl, 0 /* session */);
	} else {
		d = wl_find(dom, &js_wl);
		if (d)
			RB_REMOVE(domain_list, &js_wl, d);
		button_set_stockid(t->js_toggle, GTK_STOCK_MEDIA_PAUSE);
	}
	g_object_set(G_OBJECT(t->settings),
	    "enable-scripts", es, (char *)NULL);
	g_object_set(G_OBJECT(t->settings),
	    "javascript-can-open-windows-automatically", es, (char *)NULL);
	webkit_web_view_set_settings(t->wv, t->settings);

	if (args->i & XT_WL_RELOAD)
		webkit_web_view_reload(t->wv);
done:
	if (dom)
		g_free(dom);
	return (0);
}

int
toggle_pl(struct tab *t, struct karg *args)
{
	int			es;
	const gchar		*uri;
	struct domain		*d;
	char			*dom = NULL;

	if (args == NULL)
		return (1);

	g_object_get(G_OBJECT(t->settings),
	    "enable-plugins", &es, (char *)NULL);
	if (args->i & XT_WL_TOGGLE)
		es = !es;
	else if ((args->i & XT_WL_ENABLE) && es != 1)
		es = 1;
	else if ((args->i & XT_WL_DISABLE) && es != 0)
		es = 0;
	else
		return (1);

	uri = get_uri(t);
	dom = find_domain(uri, args->i & XT_WL_TOPLEVEL);

	if (uri == NULL || dom == NULL ||
	    webkit_web_view_get_load_status(t->wv) == WEBKIT_LOAD_FAILED) {
		show_oops(t, "Can't toggle domain in plugins white list");
		goto done;
	}

	if (es)
		wl_add(dom, &pl_wl, 0 /* session */);
	else {
		d = wl_find(dom, &pl_wl);
		if (d)
			RB_REMOVE(domain_list, &pl_wl, d);
	}
	g_object_set(G_OBJECT(t->settings),
	    "enable-plugins", es, (char *)NULL);
	webkit_web_view_set_settings(t->wv, t->settings);

	if (args->i & XT_WL_RELOAD)
		webkit_web_view_reload(t->wv);
done:
	if (dom)
		g_free(dom);
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
startpage(struct tab *t, struct karg *args)
{
	char			*page, *body, *b;
	struct sp		*s;

	if (t == NULL)
		show_oops(NULL, "startpage invalid parameters");

	body = g_strdup_printf("<b>Startup Exception(s):</b><p>");

	TAILQ_FOREACH(s, &spl, entry) {
		b = body;
		body = g_strdup_printf("%s%s<br>", body, s->line);
		g_free(b);
	}

	page = get_html_page("Startup Exception", body, "", 0);
	g_free(body);

	load_webkit_string(t, page, XT_URI_ABOUT_STARTPAGE);
	g_free(page);

	return (0);
}

void
startpage_add(const char *fmt, ...)
{
	va_list			ap;
	char			*msg;
	struct sp		*s;

	if (fmt == NULL)
		return;

	va_start(ap, fmt);
	if (vasprintf(&msg, fmt, ap) == -1)
		errx(1, "startpage_add failed");
	va_end(ap);

	s = g_malloc0(sizeof *s);
	s->line = msg;

	TAILQ_INSERT_TAIL(&spl, s, entry);
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
	if ((rv = gnutls_handshake(gsession)) < 0) {
		snprintf(myerror, sizeof myerror,
		    "gnutls_handshake failed %d fatal %d %s",
		    rv,
		    gnutls_error_is_fatal(rv),
		    gnutls_strerror_name(rv));
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

void
statusbar_modify_attr(struct tab *t, const char *text, const char *base)
{
	GdkColor                c_text, c_base;

	gdk_color_parse(text, &c_text);
	gdk_color_parse(base, &c_base);

	gtk_widget_modify_text(t->sbe.statusbar, GTK_STATE_NORMAL, &c_text);
	gtk_widget_modify_text(t->sbe.buffercmd, GTK_STATE_NORMAL, &c_text);
	gtk_widget_modify_text(t->sbe.zoom, GTK_STATE_NORMAL, &c_text);
	gtk_widget_modify_text(t->sbe.position, GTK_STATE_NORMAL, &c_text);

	gtk_widget_modify_base(t->sbe.statusbar, GTK_STATE_NORMAL, &c_base);
	gtk_widget_modify_base(t->sbe.buffercmd, GTK_STATE_NORMAL, &c_base);
	gtk_widget_modify_base(t->sbe.zoom, GTK_STATE_NORMAL, &c_base);
	gtk_widget_modify_base(t->sbe.position, GTK_STATE_NORMAL, &c_base);
}

void
save_certs(struct tab *t, gnutls_x509_crt_t *certs,
    size_t cert_count, char *domain)
{
	size_t			cert_buf_sz;
	char			cert_buf[64 * 1024], file[PATH_MAX];
	int			i;
	FILE			*f;
	GdkColor		color;

	if (t == NULL || certs == NULL || cert_count <= 0 || domain == NULL)
		return;

	snprintf(file, sizeof file, "%s/%s", certs_dir, domain);
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

	/* not the best spot but oh well */
	gdk_color_parse("lightblue", &color);
	gtk_widget_modify_base(t->uri_entry, GTK_STATE_NORMAL, &color);
	statusbar_modify_attr(t, XT_COLOR_BLACK, "lightblue");
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
load_compare_cert(const gchar *uri, const gchar **error_str)
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

	snprintf(file, sizeof file, "%s/%s", certs_dir, domain);
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
		if (fread(r_cert_buf, cert_buf_sz, 1, f) != 1) {
			rv = CERT_BAD; /* critical */
			goto freeit;
		}
		if (bcmp(r_cert_buf, cert_buf, sizeof cert_buf_sz)) {
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
	if (error && rv != CERT_LOCAL) {
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
		if (error & GNUTLS_CERT_NOT_ACTIVATED)
			strlcat(serr, "not activated, ", sizeof serr);
		if (error & GNUTLS_CERT_EXPIRED)
			strlcat(serr, "expired, ", sizeof serr);
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

	if (t == NULL)
		return (1);

	if (ssl_ca_file == NULL) {
		show_oops(t, "Can't open CA file: %s", ssl_ca_file);
		return (1);
	}

	if ((uri = get_uri(t)) == NULL) {
		show_oops(t, "Invalid URI");
		return (1);
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
	else if (args->i & XT_SAVE)
		save_certs(t, certs, cert_count, domain);

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
wl_show(struct tab *t, struct karg *args, char *title, struct domain_list *wl)
{
	struct domain		*d;
	char			*tmp, *body;

	body = g_strdup("");

	/* p list */
	if (args->i & XT_WL_PERSISTENT) {
		tmp = body;
		body = g_strdup_printf("%s<h2>Persistent</h2>", body);
		g_free(tmp);
		RB_FOREACH(d, domain_list, wl) {
			if (d->handy == 0)
				continue;
			tmp = body;
			body = g_strdup_printf("%s%s<br/>", body, d->d);
			g_free(tmp);
		}
	}

	/* s list */
	if (args->i & XT_WL_SESSION) {
		tmp = body;
		body = g_strdup_printf("%s<h2>Session</h2>", body);
		g_free(tmp);
		RB_FOREACH(d, domain_list, wl) {
			if (d->handy == 1)
				continue;
			tmp = body;
			body = g_strdup_printf("%s%s<br/>", body, d->d);
			g_free(tmp);
		}
	}

	tmp = get_html_page(title, body, "", 0);
	g_free(body);
	if (wl == &js_wl)
		load_webkit_string(t, tmp, XT_URI_ABOUT_JSWL);
	else if (wl == &c_wl)
		load_webkit_string(t, tmp, XT_URI_ABOUT_COOKIEWL);
	else
		load_webkit_string(t, tmp, XT_URI_ABOUT_PLUGINWL);
	g_free(tmp);
	return (0);
}

#define XT_WL_INVALID		(0)
#define XT_WL_JAVASCRIPT	(1)
#define XT_WL_COOKIE		(2)
#define XT_WL_PLUGIN		(3)
int
wl_save(struct tab *t, struct karg *args, int list)
{
	char			file[PATH_MAX], *lst_str = NULL;
	FILE			*f;
	char			*line = NULL, *lt = NULL, *dom = NULL;
	size_t			linelen;
	const gchar		*uri;
	struct karg		a;
	struct domain		*d;
	GSList			*cf;
	SoupCookie		*ci, *c;

	if (t == NULL || args == NULL)
		return (1);

	if (runtime_settings[0] == '\0')
		return (1);

	snprintf(file, sizeof file, "%s/%s", work_dir, runtime_settings);
	if ((f = fopen(file, "r+")) == NULL)
		return (1);

	switch (list) {
	case XT_WL_JAVASCRIPT:
		lst_str = "JavaScript";
		lt = g_strdup_printf("js_wl=%s", dom);
		break;
	case XT_WL_COOKIE:
		lst_str = "Cookie";
		lt = g_strdup_printf("cookie_wl=%s", dom);
		break;
	case XT_WL_PLUGIN:
		lst_str = "Plugin";
		lt = g_strdup_printf("pl_wl=%s", dom);
		break;
	default:
		show_oops(t, "Invalid list id: %d", list);
		return (1);
	}

	uri = get_uri(t);
	dom = find_domain(uri, args->i & XT_WL_TOPLEVEL);
	if (uri == NULL || dom == NULL ||
	    webkit_web_view_get_load_status(t->wv) == WEBKIT_LOAD_FAILED) {
		show_oops(t, "Can't add domain to %s white list", lst_str);
		goto done;
	}

	while (!feof(f)) {
		line = fparseln(f, &linelen, NULL, NULL, 0);
		if (line == NULL)
			continue;
		if (!strcmp(line, lt))
			goto done;
		free(line);
		line = NULL;
	}

	fprintf(f, "%s\n", lt);

	a.i = XT_WL_ENABLE;
	a.i |= args->i;
	switch (list) {
	case XT_WL_JAVASCRIPT:
		d = wl_find(dom, &js_wl);
		if (!d) {
			settings_add("js_wl", dom);
			d = wl_find(dom, &js_wl);
		}
		toggle_js(t, &a);
		break;

	case XT_WL_COOKIE:
		d = wl_find(dom, &c_wl);
		if (!d) {
			settings_add("cookie_wl", dom);
			d = wl_find(dom, &c_wl);
		}
		toggle_cwl(t, &a);

		/* find and add to persistent jar */
		cf = soup_cookie_jar_all_cookies(s_cookiejar);
		for (;cf; cf = cf->next) {
			ci = cf->data;
			if (!strcmp(dom, ci->domain) ||
			    !strcmp(&dom[1], ci->domain)) /* deal with leading . */ {
				c = soup_cookie_copy(ci);
				_soup_cookie_jar_add_cookie(p_cookiejar, c);
			}
		}
		soup_cookies_free(cf);
		break;

	case XT_WL_PLUGIN:
		d = wl_find(dom, &pl_wl);
		if (!d) {
			settings_add("pl_wl", dom);
			d = wl_find(dom, &pl_wl);
		}
		toggle_pl(t, &a);
		break;
	default:
		abort(); /* can't happen */
	}
	if (d)
		d->handy = 1;

done:
	if (line)
		free(line);
	if (dom)
		g_free(dom);
	if (lt)
		g_free(lt);
	fclose(f);

	return (0);
}

int
js_show_wl(struct tab *t, struct karg *args)
{
	args->i = XT_SHOW | XT_WL_PERSISTENT | XT_WL_SESSION;
	wl_show(t, args, "JavaScript White List", &js_wl);

	return (0);
}

int
cookie_cmd(struct tab *t, struct karg *args)
{
	if (args->i & XT_SHOW)
		wl_show(t, args, "Cookie White List", &c_wl);
	else if (args->i & XT_WL_TOGGLE) {
		args->i |= XT_WL_RELOAD;
		toggle_cwl(t, args);
	} else if (args->i & XT_SAVE) {
		args->i |= XT_WL_RELOAD;
		wl_save(t, args, XT_WL_COOKIE);
	} else if (args->i & XT_DELETE)
		show_oops(t, "'cookie delete' currently unimplemented");

	return (0);
}

int
js_cmd(struct tab *t, struct karg *args)
{
	if (args->i & XT_SHOW)
		wl_show(t, args, "JavaScript White List", &js_wl);
	else if (args->i & XT_SAVE) {
		args->i |= XT_WL_RELOAD;
		wl_save(t, args, XT_WL_JAVASCRIPT);
	} else if (args->i & XT_WL_TOGGLE) {
		args->i |= XT_WL_RELOAD;
		toggle_js(t, args);
	} else if (args->i & XT_DELETE)
		show_oops(t, "'js delete' currently unimplemented");

	return (0);
}

int
pl_show_wl(struct tab *t, struct karg *args)
{
	args->i = XT_SHOW | XT_WL_PERSISTENT | XT_WL_SESSION;
	wl_show(t, args, "Plugin White List", &pl_wl);

	return (0);
}

int
pl_cmd(struct tab *t, struct karg *args)
{
	if (args->i & XT_SHOW)
		wl_show(t, args, "Plugin White List", &pl_wl);
	else if (args->i & XT_SAVE) {
		args->i |= XT_WL_RELOAD;
		wl_save(t, args, XT_WL_PLUGIN);
	} else if (args->i & XT_WL_TOGGLE) {
		args->i |= XT_WL_RELOAD;
		toggle_pl(t, args);
	} else if (args->i & XT_DELETE)
		show_oops(t, "'plugin delete' currently unimplemented");

	return (0);
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

	t->xtp_meaning = XT_XTP_TAB_MEANING_NORMAL;
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
	case XT_MOVE_HALFDOWN:
		pos += pi / 2;
		gtk_adjustment_set_value(adjust, MIN(pos, max));
		break;
	case XT_MOVE_HALFUP:
		pos -= pi / 2;
		gtk_adjustment_set_value(adjust, MAX(pos, lower));
		break;
	case XT_MOVE_PERCENT:
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

	TAILQ_FOREACH(t, &tabs, entry)
		if (show_statusbar == 0) {
			gtk_widget_hide(t->statusbar_box);
			focus_webview(t);
		} else
			gtk_widget_show(t->statusbar_box);
}

void
url_set(struct tab *t, int enable_url_entry)
{
	GdkPixbuf	*pixbuf;
	int		progress;

	show_url = enable_url_entry;

	if (enable_url_entry) {
		gtk_entry_set_icon_from_icon_name(GTK_ENTRY(t->sbe.statusbar),
		    GTK_ENTRY_ICON_PRIMARY, NULL);
		gtk_entry_set_progress_fraction(GTK_ENTRY(t->sbe.statusbar), 0);
	} else {
		pixbuf = gtk_entry_get_icon_pixbuf(GTK_ENTRY(t->uri_entry),
		    GTK_ENTRY_ICON_PRIMARY);
		progress =
		    gtk_entry_get_progress_fraction(GTK_ENTRY(t->uri_entry));
		gtk_entry_set_icon_from_pixbuf(GTK_ENTRY(t->sbe.statusbar),
		    GTK_ENTRY_ICON_PRIMARY, pixbuf);
		gtk_entry_set_progress_fraction(GTK_ENTRY(t->sbe.statusbar),
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
	struct tab		*tt;

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

int
command_mode(struct tab *t, struct karg *args)
{
	if (args->i == XT_MODE_COMMAND)
		run_script(t, "hints.clearFocus();");
	else
		run_script(t, "hints.focusInput();");
	return (XT_CB_HANDLED);
}

int
command(struct tab *t, struct karg *args)
{
	char			*s = NULL, *ss = NULL;
	GdkColor		color;
	const gchar		*uri;
	struct karg		a;

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
		if (cmd_prefix == 0)
			s = ":";
		else {
			ss = g_strdup_printf(":%d", cmd_prefix);
			s = ss;
			cmd_prefix = 0;
		}
		break;
	case '.':
		bzero(&a, sizeof a);
		a.i = 0;
		hint(t, &a);
		s = ".";
		break;
	case ',':
		bzero(&a, sizeof a);
		a.i = XT_HINT_NEWTAB;
		hint(t, &a);
		s = ",";
		break;
	case XT_CMD_OPEN:
		s = ":open ";
		break;
	case XT_CMD_TABNEW:
		s = ":tabnew ";
		break;
	case XT_CMD_OPEN_CURRENT:
		s = ":open ";
		/* FALL THROUGH */
	case XT_CMD_TABNEW_CURRENT:
		if (!s) /* FALL THROUGH? */
			s = ":tabnew ";
		if ((uri = get_uri(t)) != NULL) {
			ss = g_strdup_printf("%s%s", s, uri);
			s = ss;
		}
		break;
	default:
		show_oops(t, "command: invalid opcode %d", args->i);
		return (XT_CB_PASSTHROUGH);
	}

	DNPRINTF(XT_D_CMD, "command: type %s\n", s);

	gtk_entry_set_text(GTK_ENTRY(t->cmd), s);
	gdk_color_parse(XT_COLOR_WHITE, &color);
	gtk_widget_modify_base(t->cmd, GTK_STATE_NORMAL, &color);
	show_cmd(t);
	gtk_widget_grab_focus(GTK_WIDGET(t->cmd));
	gtk_editable_set_position(GTK_EDITABLE(t->cmd), -1);

	if (ss)
		g_free(ss);

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

struct settings_args {
	char		**body;
	int		i;
};

void
print_setting(struct settings *s, char *val, void *cb_args)
{
	char			*tmp, *color;
	struct settings_args	*sa = cb_args;

	if (sa == NULL)
		return;

	if (s->flags & XT_SF_RUNTIME)
		color = "#22cc22";
	else
		color = "#cccccc";

	tmp = *sa->body;
	*sa->body = g_strdup_printf(
	    "%s\n<tr>"
	    "<td style='background-color: %s; width: 10%%;word-break:break-all'>%s</td>"
	    "<td style='background-color: %s; width: 20%%;word-break:break-all'>%s</td>",
	    *sa->body,
	    color,
	    s->name,
	    color,
	    val
	    );
	g_free(tmp);
	sa->i++;
}

int
set_show(struct tab *t, struct karg *args)
{
	char			*body, *page, *tmp;
	int			i = 1;
	struct settings_args	sa;

	bzero(&sa, sizeof sa);
	sa.body = &body;

	/* body */
	body = g_strdup_printf("<div align='center'><table><tr>"
	    "<th align='left'>Setting</th>"
	    "<th align='left'>Value</th></tr>\n");

	settings_walk(print_setting, &sa);
	i = sa.i;

	/* small message if there are none */
	if (i == 1) {
		tmp = body;
		body = g_strdup_printf("%s\n<tr><td style='text-align:center'"
		    "colspan='2'>No settings</td></tr>\n", body);
		g_free(tmp);
	}

	tmp = body;
	body = g_strdup_printf("%s</table></div>", body);
	g_free(tmp);

	page = get_html_page("Settings", body, "", 0);

	g_free(body);

	load_webkit_string(t, page, XT_URI_ABOUT_SET);

	g_free(page);

	return (XT_CB_PASSTHROUGH);
}

int
set(struct tab *t, struct karg *args)
{
	char			*p, *val;
	int			i;

	if (args == NULL || args->s == NULL)
		return (set_show(t, args));

	/* strip spaces */
	p = g_strstrip(args->s);

	if (strlen(p) == 0)
		return (set_show(t, args));

	/* we got some sort of string */
	val = g_strrstr(p, "=");
	if (val) {
		*val++ = '\0';
		val = g_strchomp(val);
		p = g_strchomp(p);

		for (i = 0; i < LENGTH(rs); i++) {
			if (strcmp(rs[i].name, p))
				continue;

			if (rs[i].activate) {
				if (rs[i].activate(val))
					show_oops(t, "%s invalid value %s",
					    p, val);
				else
					show_oops(t, ":set %s = %s", p, val);
				goto done;
			} else {
				show_oops(t, "not a runtime option: %s", p);
				goto done;
			}
		}
		show_oops(t, "unknown option: %s", p);
	} else {
		p = g_strchomp(p);

		for (i = 0; i < LENGTH(rs); i++) {
			if (strcmp(rs[i].name, p))
				continue;

			/* XXX this could use some cleanup */
			switch (rs[i].type) {
			case XT_S_INT:
				if (rs[i].ival)
					show_oops(t, "%s = %d",
					    rs[i].name, *rs[i].ival);
				else if (rs[i].s && rs[i].s->get)
					show_oops(t, "%s = %s",
					    rs[i].name,
					    rs[i].s->get(&rs[i]));
				else if (rs[i].s && rs[i].s->get == NULL)
					show_oops(t, "%s = ...", rs[i].name);
				else
					show_oops(t, "%s = ", rs[i].name);
				break;
			case XT_S_FLOAT:
				if (rs[i].fval)
					show_oops(t, "%s = %f",
					    rs[i].name, *rs[i].fval);
				else if (rs[i].s && rs[i].s->get)
					show_oops(t, "%s = %s",
					    rs[i].name,
					    rs[i].s->get(&rs[i]));
				else if (rs[i].s && rs[i].s->get == NULL)
					show_oops(t, "%s = ...", rs[i].name);
				else
					show_oops(t, "%s = ", rs[i].name);
				break;
			case XT_S_STR:
				if (rs[i].sval && *rs[i].sval)
					show_oops(t, "%s = %s",
					    rs[i].name, *rs[i].sval);
				else if (rs[i].s && rs[i].s->get)
					show_oops(t, "%s = %s",
					    rs[i].name,
					    rs[i].s->get(&rs[i]));
				else if (rs[i].s && rs[i].s->get == NULL)
					show_oops(t, "%s = ...", rs[i].name);
				else
					show_oops(t, "%s = ", rs[i].name);
				break;
			default:
				show_oops(t, "unknown type for %s", rs[i].name);
				goto done;
			}

			goto done;
		}
		show_oops(t, "unknown option: %s", p);
	}
done:
	return (XT_CB_PASSTHROUGH);
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

	snprintf(file, sizeof file, "%s/%s", sessions_dir, filename);
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
	JSGlobalContextRef	ctx;
	WebKitWebFrame		*frame;
	JSStringRef		str;
	JSValueRef		val, exception;
	char			*es;
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

	/* this code needs to be redone */
	frame = webkit_web_view_get_main_frame(t->wv);
	ctx = webkit_web_frame_get_global_context(frame);

	str = JSStringCreateWithUTF8CString(buf);
	val = JSEvaluateScript(ctx, str, JSContextGetGlobalObject(ctx),
	    NULL, 0, &exception);
	JSStringRelease(str);

	DNPRINTF(XT_D_JS, "run_script: val %p\n", val);
	if (val == NULL) {
		es = js_ref_to_string(ctx, exception);
		if (es) {
			show_oops(t, "script exception: %s", es);
			g_free(es);
		}
		goto done;
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
			show_oops(t, "script complete return value: '%s'", es);
			g_free(es);
		} else
			show_oops(t, "script complete: without a return value");
	}

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

#define CTRL	GDK_CONTROL_MASK
#define MOD1	GDK_MOD1_MASK
#define SHFT	GDK_SHIFT_MASK

/* inherent to GTK not all keys will be caught at all times */
/* XXX sort key bindings */
struct key_binding {
	char				*cmd;
	guint				mask;
	guint				use_in_entry;
	guint				key;
	TAILQ_ENTRY(key_binding)	entry;	/* in bss so no need to init */
} keys[] = {
	{ "command_mode",	0,	0,	GDK_Escape	},
	{ "insert_mode",	0,	0,	GDK_i		},
	{ "cookiejar",		MOD1,	0,	GDK_j		},
	{ "downloadmgr",	MOD1,	0,	GDK_d		},
	{ "history",		MOD1,	0,	GDK_h		},
	{ "print",		CTRL,	0,	GDK_p		},
	{ "search",		0,	0,	GDK_slash	},
	{ "searchb",		0,	0,	GDK_question	},
	{ "statustoggle",	CTRL,	0,	GDK_n		},
	{ "command",		0,	0,	GDK_colon	},
	{ "qa",			CTRL,	0,	GDK_q		},
	{ "restart",		MOD1,	0,	GDK_q		},
	{ "js toggle",		CTRL,	0,	GDK_j		},
	{ "cookie toggle",	MOD1,	0,	GDK_c		},
	{ "togglesrc",		CTRL,	0,	GDK_s		},
	{ "yankuri",		0,	0,	GDK_y		},
	{ "pasteuricur",	0,	0,	GDK_p		},
	{ "pasteurinew",	0,	0,	GDK_P		},
	{ "toplevel toggle",	0,	0,	GDK_F4		},
	{ "help",		0,	0,	GDK_F1		},
	{ "run_script",		MOD1,	0,	GDK_r		},

	/* search */
	{ "searchnext",		0,	0,	GDK_n		},
	{ "searchprevious",	0,	0,	GDK_N		},

	/* focus */
	{ "focusaddress",	0,	0,	GDK_F6		},
	{ "focussearch",	0,	0,	GDK_F7		},

	/* hinting */
	{ "hinting",		0,	0,	GDK_f		},
	{ "hinting",		0,	0,	GDK_period	},
	{ "hinting_newtab",	SHFT,	0,	GDK_F		},
	{ "hinting_newtab",	0,	0,	GDK_comma	},

	/* custom stylesheet */
	{ "userstyle",		0,	0,	GDK_s		},

	/* navigation */
	{ "goback",		0,	0,	GDK_BackSpace	},
	{ "goback",		MOD1,	0,	GDK_Left	},
	{ "goforward",		SHFT,	0,	GDK_BackSpace	},
	{ "goforward",		MOD1,	0,	GDK_Right	},
	{ "reload",		0,	0,	GDK_F5		},
	{ "reload",		CTRL,	0,	GDK_r		},
	{ "reload",		CTRL,	0,	GDK_l		},
	{ "favorites",		MOD1,	1,	GDK_f		},

	/* vertical movement */
	{ "scrolldown",		0,	0,	GDK_j		},
	{ "scrolldown",		0,	0,	GDK_Down	},
	{ "scrollup",		0,	0,	GDK_Up		},
	{ "scrollup",		0,	0,	GDK_k		},
	{ "scrollbottom",	0,	0,	GDK_G		},
	{ "scrollbottom",	0,	0,	GDK_End		},
	{ "scrolltop",		0,	0,	GDK_Home	},
	{ "scrollpagedown",	0,	0,	GDK_space	},
	{ "scrollpagedown",	CTRL,	0,	GDK_f		},
	{ "scrollhalfdown",	CTRL,	0,	GDK_d		},
	{ "scrollpagedown",	0,	0,	GDK_Page_Down	},
	{ "scrollpageup",	0,	0,	GDK_Page_Up	},
	{ "scrollpageup",	CTRL,	0,	GDK_b		},
	{ "scrollhalfup",	CTRL,	0,	GDK_u		},
	/* horizontal movement */
	{ "scrollright",	0,	0,	GDK_l		},
	{ "scrollright",	0,	0,	GDK_Right	},
	{ "scrollleft",		0,	0,	GDK_Left	},
	{ "scrollleft",		0,	0,	GDK_h		},
	{ "scrollfarright",	0,	0,	GDK_dollar	},
	{ "scrollfarleft",	0,	0,	GDK_0		},

	/* tabs */
	{ "tabnew",		CTRL,	0,	GDK_t		},
	{ "999tabnew",		CTRL,	0,	GDK_T		},
	{ "tabclose",		CTRL,	1,	GDK_w		},
	{ "tabundoclose",	0,	0,	GDK_U		},
	{ "tabnext 1",		CTRL,	0,	GDK_1		},
	{ "tabnext 2",		CTRL,	0,	GDK_2		},
	{ "tabnext 3",		CTRL,	0,	GDK_3		},
	{ "tabnext 4",		CTRL,	0,	GDK_4		},
	{ "tabnext 5",		CTRL,	0,	GDK_5		},
	{ "tabnext 6",		CTRL,	0,	GDK_6		},
	{ "tabnext 7",		CTRL,	0,	GDK_7		},
	{ "tabnext 8",		CTRL,	0,	GDK_8		},
	{ "tabnext 9",		CTRL,	0,	GDK_9		},
	{ "tabfirst",		CTRL,	0,	GDK_less	},
	{ "tablast",		CTRL,	0,	GDK_greater	},
	{ "tabprevious",	CTRL,	0,	GDK_Left	},
	{ "tabnext",		CTRL,	0,	GDK_Right	},
	{ "focusout",		CTRL,	0,	GDK_minus	},
	{ "focusin",		CTRL,	0,	GDK_plus	},
	{ "focusin",		CTRL,	0,	GDK_equal	},
	{ "focusreset",		CTRL,	0,	GDK_0		},

	/* command aliases (handy when -S flag is used) */
	{ "promptopen",		0,	0,	GDK_F9		},
	{ "promptopencurrent",	0,	0,	GDK_F10		},
	{ "prompttabnew",	0,	0,	GDK_F11		},
	{ "prompttabnewcurrent",0,	0,	GDK_F12		},
};
TAILQ_HEAD(keybinding_list, key_binding);

void
walk_kb(struct settings *s,
    void (*cb)(struct settings *, char *, void *), void *cb_args)
{
	struct key_binding	*k;
	char			str[1024];

	if (s == NULL || cb == NULL) {
		show_oops(NULL, "walk_kb invalid parameters");
		return;
	}

	TAILQ_FOREACH(k, &kbl, entry) {
		if (k->cmd == NULL)
			continue;
		str[0] = '\0';

		/* sanity */
		if (gdk_keyval_name(k->key) == NULL)
			continue;

		strlcat(str, k->cmd, sizeof str);
		strlcat(str, ",", sizeof str);

		if (k->mask & GDK_SHIFT_MASK)
			strlcat(str, "S-", sizeof str);
		if (k->mask & GDK_CONTROL_MASK)
			strlcat(str, "C-", sizeof str);
		if (k->mask & GDK_MOD1_MASK)
			strlcat(str, "M1-", sizeof str);
		if (k->mask & GDK_MOD2_MASK)
			strlcat(str, "M2-", sizeof str);
		if (k->mask & GDK_MOD3_MASK)
			strlcat(str, "M3-", sizeof str);
		if (k->mask & GDK_MOD4_MASK)
			strlcat(str, "M4-", sizeof str);
		if (k->mask & GDK_MOD5_MASK)
			strlcat(str, "M5-", sizeof str);

		strlcat(str, gdk_keyval_name(k->key), sizeof str);
		cb(s, str, cb_args);
	}
}

void
init_keybindings(void)
{
	int			i;
	struct key_binding	*k;

	for (i = 0; i < LENGTH(keys); i++) {
		k = g_malloc0(sizeof *k);
		k->cmd = keys[i].cmd;
		k->mask = keys[i].mask;
		k->use_in_entry = keys[i].use_in_entry;
		k->key = keys[i].key;
		TAILQ_INSERT_HEAD(&kbl, k, entry);

		DNPRINTF(XT_D_KEYBINDING, "init_keybindings: added: %s\n",
		    k->cmd ? k->cmd : "unnamed key");
	}
}

void
keybinding_clearall(void)
{
	struct key_binding	*k, *next;

	for (k = TAILQ_FIRST(&kbl); k; k = next) {
		next = TAILQ_NEXT(k, entry);
		if (k->cmd == NULL)
			continue;

		DNPRINTF(XT_D_KEYBINDING, "keybinding_clearall: %s\n",
		    k->cmd ? k->cmd : "unnamed key");
		TAILQ_REMOVE(&kbl, k, entry);
		g_free(k);
	}
}

int
keybinding_add(char *cmd, char *key, int use_in_entry)
{
	struct key_binding	*k;
	guint			keyval, mask = 0;
	int			i;

	DNPRINTF(XT_D_KEYBINDING, "keybinding_add: %s %s\n", cmd, key);

	/* Keys which are to be used in entry have been prefixed with an
	 * exclamation mark. */
	if (use_in_entry)
		key++;

	/* find modifier keys */
	if (strstr(key, "S-"))
		mask |= GDK_SHIFT_MASK;
	if (strstr(key, "C-"))
		mask |= GDK_CONTROL_MASK;
	if (strstr(key, "M1-"))
		mask |= GDK_MOD1_MASK;
	if (strstr(key, "M2-"))
		mask |= GDK_MOD2_MASK;
	if (strstr(key, "M3-"))
		mask |= GDK_MOD3_MASK;
	if (strstr(key, "M4-"))
		mask |= GDK_MOD4_MASK;
	if (strstr(key, "M5-"))
		mask |= GDK_MOD5_MASK;

	/* find keyname */
	for (i = strlen(key) - 1; i > 0; i--)
		if (key[i] == '-')
			key = &key[i + 1];

	/* validate keyname */
	keyval = gdk_keyval_from_name(key);
	if (keyval == GDK_VoidSymbol) {
		warnx("invalid keybinding name %s", key);
		return (1);
	}
	/* must run this test too, gtk+ doesn't handle 10 for example */
	if (gdk_keyval_name(keyval) == NULL) {
		warnx("invalid keybinding name %s", key);
		return (1);
	}

	/* Remove eventual dupes. */
	TAILQ_FOREACH(k, &kbl, entry)
		if (k->key == keyval && k->mask == mask) {
			TAILQ_REMOVE(&kbl, k, entry);
			g_free(k);
			break;
		}

	/* add keyname */
	k = g_malloc0(sizeof *k);
	k->cmd = g_strdup(cmd);
	k->mask = mask;
	k->use_in_entry = use_in_entry;
	k->key = keyval;

	DNPRINTF(XT_D_KEYBINDING, "keybinding_add: %s 0x%x %d 0x%x\n",
	    k->cmd,
	    k->mask,
	    k->use_in_entry,
	    k->key);
	DNPRINTF(XT_D_KEYBINDING, "keybinding_add: adding: %s %s\n",
	    k->cmd, gdk_keyval_name(keyval));

	TAILQ_INSERT_HEAD(&kbl, k, entry);

	return (0);
}

int
add_kb(struct settings *s, char *entry)
{
	char			*kb, *key;

	DNPRINTF(XT_D_KEYBINDING, "add_kb: %s\n", entry);

	/* clearall is special */
	if (!strcmp(entry, "clearall")) {
		keybinding_clearall();
		return (0);
	}

	kb = strstr(entry, ",");
	if (kb == NULL)
		return (1);
	*kb = '\0';
	key = kb + 1;

	return (keybinding_add(entry, key, key[0] == '!'));
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

	/* yanking and pasting */
	{ "yankuri",		0,	yank_uri,		0,			0 },
	/* XXX: pasteuri{cur,new} do not work from the cmd_entry? */
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
	{ "userstyle",		0,	userstyle,		0,			0 },

	/* navigation */
	{ "goback",		0,	navaction,		XT_NAV_BACK,		0 },
	{ "goforward",		0,	navaction,		XT_NAV_FORWARD,		0 },
	{ "reload",		0,	navaction,		XT_NAV_RELOAD,		0 },

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

	{ "favorites",		0,	xtp_page_fl,		0,			0 },
	{ "fav",		0,	xtp_page_fl,		0,			0 },
	{ "favadd",		0,	add_favorite,		0,			0 },

	{ "qall",		0,	quit,			0,			0 },
	{ "quitall",		0,	quit,			0,			0 },
	{ "w",			0,	save_tabs,		0,			0 },
	{ "wq",			0,	save_tabs_and_quit,	0,			0 },
	{ "help",		0,	help,			0,			0 },
	{ "about",		0,	about,			0,			0 },
	{ "stats",		0,	stats,			0,			0 },
	{ "version",		0,	about,			0,			0 },

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
	{ "tabprevious",	0,	movetab,		XT_TAB_PREV,		XT_PREFIX | XT_INTARG},
	{ "tabrewind",		0,	movetab,		XT_TAB_FIRST,		0 },
	{ "tabshow",		0,	tabaction,		XT_TAB_SHOW,		0 },
	{ "tabs",		0,	buffers,		0,			0 },
	{ "tabundoclose",	0,	tabaction,		XT_TAB_UNDO_CLOSE,	0 },
	{ "buffers",		0,	buffers,		0,			0 },
	{ "ls",			0,	buffers,		0,			0 },
	{ "encoding",		0,	set_encoding,		0,			XT_USERARG },

	/* command aliases (handy when -S flag is used) */
	{ "promptopen",		0,	command,		XT_CMD_OPEN,		0 },
	{ "promptopencurrent",	0,	command,		XT_CMD_OPEN_CURRENT,	0 },
	{ "prompttabnew",	0,	command,		XT_CMD_TABNEW,		0 },
	{ "prompttabnewcurrent",0,	command,		XT_CMD_TABNEW_CURRENT,	0 },

	/* settings */
	{ "set",		0,	set,			0,			XT_SETARG },

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

	hide_oops(t);
	hide_buffers(t);

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

	/* if xxxt:// treat specially */
	if (parse_xtp_url(t, uri))
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

	DNPRINTF(XT_D_URL, "activate_search_entry_cb: %s\n", search);

	if (t == NULL) {
		show_oops(NULL, "activate_search_entry_cb invalid parameters");
		return;
	}

	if (search_string == NULL) {
		show_oops(t, "no search_string");
		return;
	}

	t->xtp_meaning = XT_XTP_TAB_MEANING_NORMAL;

	enc_search = soup_uri_encode(search, XT_RESERVED_CHARS);
	newuri = g_strdup_printf(search_string, enc_search);
	g_free(enc_search);

	marks_clear(t);
	webkit_web_view_load_uri(t->wv, newuri);
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

void
color_address_bar(gpointer p)
{
	GdkColor		color;
	struct tab		*tt, *t = p;
	gchar			*col_str = XT_COLOR_WHITE;
	const gchar		*uri, *u = NULL, *error_str = NULL;

#ifdef USE_THREADS
	gdk_threads_enter();
#endif
	DNPRINTF(XT_D_URL, "%s:\n", __func__);

	/* make sure t still exists */
	if (t == NULL)
		return;
	TAILQ_FOREACH(tt, &tabs, entry)
		if (t == tt)
			break;
	if (t != tt)
		goto done;

	if ((uri = get_uri(t)) == NULL)
		goto white;
	u = g_strdup(uri);

#ifdef USE_THREADS
	gdk_threads_leave();
#endif

	col_str = XT_COLOR_YELLOW;
	switch (load_compare_cert(u, &error_str)) {
	case CERT_LOCAL:
		col_str = XT_COLOR_BLUE;
		break;
	case CERT_TRUSTED:
		col_str = XT_COLOR_GREEN;
		break;
	case CERT_UNTRUSTED:
		col_str = XT_COLOR_YELLOW;
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
	gdk_color_parse(col_str, &color);
	gtk_widget_modify_base(t->uri_entry, GTK_STATE_NORMAL, &color);

	if (!strcmp(col_str, XT_COLOR_WHITE))
		statusbar_modify_attr(t, col_str, XT_COLOR_BLACK);
	else
		statusbar_modify_attr(t, XT_COLOR_BLACK, col_str);

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
	gdk_threads_leave();
#endif
}

void
show_ca_status(struct tab *t, const char *uri)
{
	GdkColor		color;
	gchar			*col_str = XT_COLOR_WHITE;

	DNPRINTF(XT_D_URL, "show_ca_status: %d %s %s\n",
	    ssl_strict_certs, ssl_ca_file, uri);

	if (t == NULL)
		return;

	if (uri == NULL)
		goto done;
	if (ssl_ca_file == NULL) {
		if (g_str_has_prefix(uri, "http://"))
			goto done;
		if (g_str_has_prefix(uri, "https://")) {
			col_str = XT_COLOR_RED;
			goto done;
		}
		return;
	}
	if (g_str_has_prefix(uri, "http://") ||
	    !g_str_has_prefix(uri, "https://"))
		goto done;
#ifdef USE_THREADS
	/*
	 * It is not necessary to see if the thread is already running.
	 * If the thread is in progress setting it to something else aborts it
	 * on the way out.
	 */

	/* thread the coloring of the address bar */
	t->thread = g_thread_create((GThreadFunc)color_address_bar, t, TRUE, NULL);
#else
	color_address_bar(t);
#endif
	return;

done:
	if (col_str) {
		gdk_color_parse(col_str, &color);
		gtk_widget_modify_base(t->uri_entry, GTK_STATE_NORMAL, &color);

		if (!strcmp(col_str, XT_COLOR_WHITE))
			statusbar_modify_attr(t, col_str, XT_COLOR_BLACK);
		else
			statusbar_modify_attr(t, XT_COLOR_BLACK, col_str);
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
	gtk_entry_set_icon_from_icon_name(GTK_ENTRY(t->uri_entry),
	    GTK_ENTRY_ICON_PRIMARY, "text-html");
	if (show_url == 0)
		gtk_entry_set_icon_from_icon_name(GTK_ENTRY(t->sbe.statusbar),
		    GTK_ENTRY_ICON_PRIMARY, "text-html");
	else
		gtk_entry_set_icon_from_icon_name(GTK_ENTRY(t->sbe.statusbar),
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

	gtk_entry_set_icon_from_pixbuf(GTK_ENTRY(t->uri_entry),
	    GTK_ENTRY_ICON_PRIMARY, pb_scaled);
	if (show_url == 0)
		gtk_entry_set_icon_from_pixbuf(GTK_ENTRY(t->sbe.statusbar),
		    GTK_ENTRY_ICON_PRIMARY, pb_scaled);
	else
		gtk_entry_set_icon_from_icon_name(GTK_ENTRY(t->sbe.statusbar),
		    GTK_ENTRY_ICON_PRIMARY, NULL);

	if (pb_scaled != pb)
		g_object_unref(pb_scaled);
}

void
xt_icon_from_file(struct tab *t, char *file)
{
	GdkPixbuf		*pb;

	if (g_str_has_prefix(file, "file://"))
		file += strlen("file://");

	pb = gdk_pixbuf_new_from_file(file, NULL);
	if (pb) {
		xt_icon_from_pixbuf(t, pb);
		g_object_unref(pb);
	} else
		xt_icon_from_name(t, "text-html");
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
set_favicon_from_file(struct tab *t, char *file)
{
	struct stat		sb;

	if (t == NULL || file == NULL)
		return;

	if (g_str_has_prefix(file, "file://"))
		file += strlen("file://");
	DNPRINTF(XT_D_DOWNLOAD, "%s: loading %s\n", __func__, file);

	if (!stat(file, &sb)) {
		if (sb.st_size == 0 || !is_valid_icon(file)) {
			/* corrupt icon so trash it */
			DNPRINTF(XT_D_DOWNLOAD, "%s: corrupt icon %s\n",
			    __func__, file);
			unlink(file);
			/* no need to set icon to default here */
			return;
		}
	}
	xt_icon_from_file(t, file);
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
	GdkPixbuf		*pb;

	pb = webkit_web_view_get_icon_pixbuf(wv);
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
	snprintf(file, sizeof file, "%s/%s.ico", cache_dir, name_hash);
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
	t->icon_dest_uri = g_strdup_printf("file://%s", file);
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
	const gchar		*uri = NULL, *title = NULL;
	struct history		*h, find;
	struct karg		a;
	GdkColor		color;

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
		gtk_label_set_text(GTK_LABEL(t->label), "Loading");

		gtk_widget_set_sensitive(GTK_WIDGET(t->stop), TRUE);

		/* assume we are a new address */
		gdk_color_parse("white", &color);
		gtk_widget_modify_base(t->uri_entry, GTK_STATE_NORMAL, &color);
		statusbar_modify_attr(t, "white", XT_COLOR_BLACK);

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
		set_status(t, (char *)uri, XT_STATUS_LOADING);

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
		break;

	case WEBKIT_LOAD_FIRST_VISUALLY_NON_EMPTY_LAYOUT:
		/* 3 */
		break;

	case WEBKIT_LOAD_FINISHED:
		/* 2 */
		uri = get_uri(t);
		if (uri == NULL)
			return;

		if (!strncmp(uri, "http://", strlen("http://")) ||
		    !strncmp(uri, "https://", strlen("https://")) ||
		    !strncmp(uri, "file://", strlen("file://"))) {
			find.uri = uri;
			h = RB_FIND(history_list, &hl, &find);
			if (!h) {
				title = get_title(t, FALSE);
				h = g_malloc(sizeof *h);
				h->uri = g_strdup(uri);
				h->title = g_strdup(title);
				RB_INSERT(history_list, &hl, h);
				completion_add_uri(h->uri);
				update_history_tabs(NULL);
			}
		}

		set_status(t, (char *)uri, XT_STATUS_URI);
#if WEBKIT_CHECK_VERSION(1, 1, 18)
	case WEBKIT_LOAD_FAILED:
		/* 4 */
#endif
#if GTK_CHECK_VERSION(2, 20, 0)
		gtk_spinner_stop(GTK_SPINNER(t->spinner));
		gtk_widget_hide(t->spinner);
#endif
	default:
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
	gtk_label_set_text(GTK_LABEL(t->label), title);
	gtk_label_set_text(GTK_LABEL(t->tab_elems.label), title);
	if (t->tab_id == gtk_notebook_get_current_page(notebook))
		gtk_window_set_title(GTK_WINDOW(main_window), win_title);
}

void
webview_load_finished_cb(WebKitWebView *wv, WebKitWebFrame *wf, struct tab *t)
{
	run_script(t, JS_HINTING);
	if (autofocus_onload &&
	    t->tab_id == gtk_notebook_get_current_page(notebook))
		run_script(t, "hints.focusInput();");
	else
		run_script(t, "hints.clearFocus();");
}

void
webview_progress_changed_cb(WebKitWebView *wv, int progress, struct tab *t)
{
	gtk_entry_set_progress_fraction(GTK_ENTRY(t->uri_entry),
	    progress == 100 ? 0 : (double)progress / 100);
	if (show_url == 0) {
		gtk_entry_set_progress_fraction(GTK_ENTRY(t->sbe.statusbar),
		    progress == 100 ? 0 : (double)progress / 100);
	}

	update_statusbar_position(NULL, NULL);
}

int
webview_npd_cb(WebKitWebView *wv, WebKitWebFrame *wf,
    WebKitNetworkRequest *request, WebKitWebNavigationAction *na,
    WebKitWebPolicyDecision *pd, struct tab *t)
{
	char				*uri;
	WebKitWebNavigationReason	reason;
	struct domain			*d = NULL;

	if (t == NULL) {
		show_oops(NULL, "webview_npd_cb invalid parameters");
		return (FALSE);
	}

	DNPRINTF(XT_D_NAV, "webview_npd_cb: ctrl_click %d %s\n",
	    t->ctrl_click,
	    webkit_network_request_get_uri(request));

	uri = (char *)webkit_network_request_get_uri(request);

	/* if this is an xtp url, we don't load anything else */
	if (parse_xtp_url(t, uri))
		    return (TRUE);

	if ((t->hints_on && t->new_tab) || t->ctrl_click) {
		t->ctrl_click = 0;
		create_new_tab(uri, NULL, ctrl_click_focus, -1);
		webkit_web_policy_decision_ignore(pd);
		return (TRUE); /* we made the decission */
	}

	/*
	 * This is a little hairy but it comes down to this:
	 * when we run in whitelist mode we have to assist the browser in
	 * opening the URL that it would have opened in a new tab.
	 */
	reason = webkit_web_navigation_action_get_reason(na);
	if (reason == WEBKIT_WEB_NAVIGATION_REASON_LINK_CLICKED) {
		t->xtp_meaning = XT_XTP_TAB_MEANING_NORMAL;
		if (enable_scripts == 0 && enable_cookie_whitelist == 1)
			if (uri && (d = wl_find_uri(uri, &js_wl)) == NULL)
				load_uri(t, uri);
		webkit_web_policy_decision_use(pd);
		return (TRUE); /* we made the decision */
	}

	return (FALSE);
}

WebKitWebView *
webview_cwv_cb(WebKitWebView *wv, WebKitWebFrame *wf, struct tab *t)
{
	struct tab		*tt;
	struct domain		*d = NULL;
	const gchar		*uri;
	WebKitWebView		*webview = NULL;

	DNPRINTF(XT_D_NAV, "webview_cwv_cb: %s\n",
	    webkit_web_view_get_uri(wv));

	if (tabless) {
		/* open in current tab */
		webview = t->wv;
	} else if (enable_scripts == 0 && enable_cookie_whitelist == 1) {
		uri = webkit_web_view_get_uri(wv);
		if (uri && (d = wl_find_uri(uri, &js_wl)) == NULL)
			return (NULL);

		tt = create_new_tab(NULL, NULL, 1, -1);
		webview = tt->wv;
	} else if (enable_scripts == 1) {
		tt = create_new_tab(NULL, NULL, 1, -1);
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

	m = find_mime_type(mime_type);
	if (m == NULL)
		return (1);
	if (m->mt_download)
		return (1);

	switch (fork()) {
	case -1:
		show_oops(t, "can't fork mime handler");
		return (1);
		/* NOTREACHED */
	case 0:
		break;
	default:
		return (0);
	}

	/* child */
	execlp(m->mt_action, m->mt_action,
	    webkit_network_request_get_uri(request), (void *)NULL);

	_exit(0);

	/* NOTREACHED */
	return (0);
}

char *
get_mime_type(const char *file)
{
	const gchar		*m;
	char			*mime_type = NULL;
	GFileInfo		*fi;
	GFile			*gf;

	if (g_str_has_prefix(file, "file://"))
		file += strlen("file://");

	gf = g_file_new_for_path(file);
	fi = g_file_query_info(gf, G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE, 0,
	    NULL, NULL);
	if ((m = g_file_info_get_content_type(fi)) != NULL)
		mime_type = g_strdup(m);
	g_object_unref(fi);
	g_object_unref(gf);

	return (mime_type);
}

int
run_download_mimehandler(char *mime_type, char *file)
{
	struct mime_type	*m;

	m = find_mime_type(mime_type);
	if (m == NULL)
		return (1);

	switch (fork()) {
	case -1:
		show_oops(NULL, "can't fork download mime handler");
		return (1);
		/* NOTREACHED */
	case 0:
		break;
	default:
		return (0);
	}

	/* child */
	if (g_str_has_prefix(file, "file://"))
		file += strlen("file://");
	execlp(m->mt_action, m->mt_action, file, (void *)NULL);

	_exit(0);

	/* NOTREACHED */
	return (0);
}

void
download_status_changed_cb(WebKitDownload *download, GParamSpec *spec,
    WebKitWebView *wv)
{
	WebKitDownloadStatus	status;
	const char		*file = NULL;
	char			*mime = NULL;

	if (download == NULL)
		return;
	status = webkit_download_get_status(download);
	if (status != WEBKIT_DOWNLOAD_STATUS_FINISHED)
		return;

	file = webkit_download_get_destination_uri(download);
	if (file == NULL)
		return;
	mime = get_mime_type(file);
	if (mime == NULL)
		return;

	run_download_mimehandler((char *)mime, (char *)file);
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
webview_download_cb(WebKitWebView *wv, WebKitDownload *wk_download,
    struct tab *t)
{
	struct stat		sb;
	const gchar		*suggested_name;
	gchar			*filename = NULL;
	char			*uri = NULL;
	struct download		*download_entry;
	int			i, ret = TRUE;

	if (wk_download == NULL || t == NULL) {
		show_oops(NULL, "%s invalid parameters", __func__);
		return (FALSE);
	}

	suggested_name = webkit_download_get_suggested_filename(wk_download);
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
		uri = g_strdup_printf("file://%s/%s", download_dir, i ?
		    filename : suggested_name);
		i++;
	} while (!stat(uri + strlen("file://"), &sb));

	DNPRINTF(XT_D_DOWNLOAD, "%s: tab %d filename %s "
	    "local %s\n", __func__, t->tab_id, filename, uri);

	webkit_download_set_destination_uri(wk_download, uri);

	if (webkit_download_get_status(wk_download) ==
	    WEBKIT_DOWNLOAD_STATUS_ERROR) {
		show_oops(t, "%s: download failed to start", __func__);
		ret = FALSE;
		gtk_label_set_text(GTK_LABEL(t->label), "Download Failed");
	} else {
		/* connect "download first" mime handler */
		g_signal_connect(G_OBJECT(wk_download), "notify::status",
		    G_CALLBACK(download_status_changed_cb), NULL);

		download_entry = g_malloc(sizeof(struct download));
		download_entry->download = wk_download;
		download_entry->tab = t;
		download_entry->id = next_download_id++;
		RB_INSERT(download_list, &downloads, download_entry);
		/* get from history */
		g_object_ref(wk_download);
		gtk_label_set_text(GTK_LABEL(t->label), "Downloading");
		show_oops(t, "Download of '%s' started...",
		    basename((char *)webkit_download_get_destination_uri(wk_download)));
	}

	if (uri)
		g_free(uri);

	if (filename)
		g_free(filename);

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
		set_status(t, uri, XT_STATUS_LINK);
	else {
		if (t->status)
			set_status(t, t->status, XT_STATUS_NOTHING);
	}
}

int
mark(struct tab *t, struct karg *arg)
{
	char                    mark;
	int                     index;

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
		gtk_adjustment_set_value(t->adjust_v, t->mark[index]);
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

	snprintf(file, sizeof file, "%s/%s", work_dir, XT_QMARKS_FILE);
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
		    (index = marktoindex(*p)) == -1) {
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

	snprintf(file, sizeof file, "%s/%s", work_dir, XT_QMARKS_FILE);
	if ((f = fopen(file, "r+")) == NULL) {
		show_oops(NULL, "Can't open quickmarks file: %s", strerror(errno));
		return (1);
	}

	for (i = 0; i < XT_NOMARKS; i++)
		if (qmarks[i] != NULL)
			fprintf(f, "%c %s\n", indextomark(i), qmarks[i]);

	fclose(f);

	return (0);
}

int
qmark(struct tab *t, struct karg *arg)
{
	char		mark;
	int		index;

	mark = arg->s[strlen(arg->s)-1];
	index = marktoindex(mark);
	if (index == -1)
		return (-1);

	switch (arg->i) {
	case XT_QMARK_SET:
		if (qmarks[index] != NULL)
			g_free(qmarks[index]);

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
	int		 levels;
	char		*uri;
	char		*tmp;

	levels = atoi(args->s);
	if (levels == 0)
		levels = 1;

	uri = g_strdup(webkit_web_view_get_uri(t->wv));
	if ((tmp = strstr(uri, XT_PROTO_DELIM)) == NULL)
		return (1);
	tmp += strlen(XT_PROTO_DELIM);

	/* if an uri starts with a slash, leave it alone (for file:///) */
	if (tmp[0] == '/')
		tmp++;

	while (levels--) {
		char	*p;

		p = strrchr(tmp, '/');
		if (p != NULL)
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

	arg.i = XT_TAB_NEXT;
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
	{ "^gg$",		XT_PRE_NO,	"gg",	move,		XT_MOVE_TOP },
	{ "^gG$",		XT_PRE_NO,	"gG",	move,		XT_MOVE_BOTTOM },
	{ "^[0-9]+%$",		XT_PRE_YES,	"%",	move,		XT_MOVE_PERCENT },
	{ "^gh$",		XT_PRE_NO,	"gh",	go_home,	0 },
	{ "^m[a-zA-Z0-9]$",	XT_PRE_NO,	"m",	mark,		XT_MARK_SET },
	{ "^['][a-zA-Z0-9]$",	XT_PRE_NO,	"'",	mark,		XT_MARK_GOTO },
	{ "^[0-9]+t$",		XT_PRE_YES,	"t",	gototab,	0 },
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

	DNPRINTF(XT_D_BUFFERCMD, "buffercmd_abort: clearing buffer\n");
	for (i = 0; i < LENGTH(bcmd); i++)
		bcmd[i] = '\0';

	cmd_prefix = 0; /* clear prefix for non-buffer commands */
	gtk_entry_set_text(GTK_ENTRY(t->sbe.buffercmd), bcmd);
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

	if (keyval == GDK_Escape) {
		buffercmd_abort(t);
		return (XT_CB_HANDLED);
	}

	/* key with modifier or non-ascii character */
	if (!isascii(keyval))
		return (XT_CB_PASSTHROUGH);

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

	gtk_entry_set_text(GTK_ENTRY(t->sbe.buffercmd), bcmd);

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

gboolean
handle_keypress(struct tab *t, GdkEventKey *e, int entry)
{
	struct key_binding	*k;

	/* handle keybindings if buffercmd is empty.
	   if not empty, allow commands like C-n */
	if (bcmd[0] == '\0' || ((e->state & (CTRL | MOD1)) != 0))
		TAILQ_FOREACH(k, &kbl, entry)
			if (e->keyval == k->key
			    && (entry ? k->use_in_entry : 1)) {
				if (k->mask == 0) {
					if ((e->state & (CTRL | MOD1)) == 0)
						return (cmd_execute(t, k->cmd));
				} else if ((e->state & k->mask) == k->mask) {
					return (cmd_execute(t, k->cmd));
				}
			}

	if (!entry && ((e->state & (CTRL | MOD1)) == 0))
		return buffercmd_addkey(t, e->keyval);

	return (XT_CB_PASSTHROUGH);
}

int
wv_keypress_after_cb(GtkWidget *w, GdkEventKey *e, struct tab *t)
{
	char			s[2];

	/* don't use w directly; use t->whatever instead */

	if (t == NULL) {
		show_oops(NULL, "wv_keypress_after_cb");
		return (XT_CB_PASSTHROUGH);
	}

	DNPRINTF(XT_D_KEY, "wv_keypress_after_cb: keyval 0x%x mask 0x%x t %p\n",
	    e->keyval, e->state, t);

	if (t->hints_on) {
		/* XXX make sure cmd entry is enabled */
		return (XT_CB_HANDLED);
	} else {
		/* prefix input*/
		snprintf(s, sizeof s, "%c", e->keyval);
		if (CLEAN(e->state) == 0 && isdigit(s[0]))
			cmd_prefix = 10 * cmd_prefix + atoi(s);
	}

	return (handle_keypress(t, e, 0));
}

int
wv_keypress_cb(GtkEntry *w, GdkEventKey *e, struct tab *t)
{
	hide_oops(t);

	/* Hide buffers, if they are visible, with escape. */
	if (gtk_widget_get_visible(GTK_WIDGET(t->buffers)) &&
	    CLEAN(e->state) == 0 && e->keyval == GDK_Escape) {
		gtk_widget_grab_focus(GTK_WIDGET(t->wv));
		hide_buffers(t);
		return (XT_CB_HANDLED);
	}

	return (XT_CB_PASSTHROUGH);
}

gboolean
hint_continue(struct tab *t)
{
	const gchar		*c = gtk_entry_get_text(GTK_ENTRY(t->cmd));
	char			*s;
	const gchar		*errstr = NULL;
	gboolean		rv = TRUE;
	long long		i;

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
		s = g_strdup_printf("hints.updateHints(%lld);", i);
		run_script(t, s);
		g_free(s);
	} else {
		/* alphanumeric input */
		s = g_strdup_printf("hints.createHints('%s', '%c');",
		    &c[1], c[0] == '.' ? 'f' : 'F');
		run_script(t, s);
		g_free(s);
	}

	rv = TRUE;
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
	GdkColor		color;

	if (search_continue(t) == FALSE)
		goto done;

	/* search */
	if (webkit_web_view_search_text(t->wv, &c[1], FALSE, t->search_forward,
	    TRUE) == FALSE) {
		/* not found, mark red */
		gdk_color_parse(XT_COLOR_RED, &color);
		gtk_widget_modify_base(t->cmd, GTK_STATE_NORMAL, &color);
		/* unmark and remove selection */
		webkit_web_view_unmark_text_matches(t->wv);
		/* my kingdom for a way to unselect text in webview */
	} else {
		/* found, highlight all */
		webkit_web_view_unmark_text_matches(t->wv);
		webkit_web_view_mark_text_matches(t->wv, &c[1], FALSE, 0);
		webkit_web_view_set_highlight_text_matches(t->wv, TRUE);
		gdk_color_parse(XT_COLOR_WHITE, &color);
		gtk_widget_modify_base(t->cmd, GTK_STATE_NORMAL, &color);
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

	DNPRINTF(XT_D_CMD, "cmd_keyrelease_cb: keyval 0x%x mask 0x%x t %p\n",
	    e->keyval, e->state, t);

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
			for (i = 0; i < LENGTH(rs); i++)
				if(!strncmp(key, rs[i].name, strlen(key)))
					cmd_status.list[c++] = rs[i].name;
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

gboolean
cmd_execute(struct tab *t, char *str)
{
	struct cmd		*cmd = NULL;
	char			*tok, *last = NULL, *s = g_strdup(str), *sc;
	char			prefixstr[4];
	int			j, len, c = 0, dep = 0, matchcount = 0;
	int			prefix = -1, rv = XT_CB_PASSTHROUGH;
	struct karg		arg = {0, NULL, -1};

	sc = s;

	/* copy prefix*/
	for (j = 0; j<3 && isdigit(s[j]); j++)
		prefixstr[j]=s[j];

	prefixstr[j]='\0';

	s += j;
	while (isspace(s[0]))
		s++;

	if (strlen(s) > 0 && strlen(prefixstr) > 0)
		prefix = atoi(prefixstr);
	else
		s = sc;

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
	g_free(sc);
	if (arg.s)
		g_free(arg.s);

	return (rv);
}

int
entry_key_cb(GtkEntry *w, GdkEventKey *e, struct tab *t)
{
	if (t == NULL) {
		show_oops(NULL, "entry_key_cb invalid parameters");
		return (XT_CB_PASSTHROUGH);
	}

	DNPRINTF(XT_D_CMD, "entry_key_cb: keyval 0x%x mask 0x%x t %p\n",
	    e->keyval, e->state, t);

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

	DNPRINTF(XT_D_CMD, "cmd_keypress_cb: keyval 0x%x mask 0x%x t %p\n",
	    e->keyval, e->state, t);

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
			if ((search_at = history_prev(&chl, search_at))) {
				search_at->line[0] = c[0];
				gtk_entry_set_text(w, search_at->line);
				gtk_editable_set_position(GTK_EDITABLE(w), -1);
			}
		} if (c[0] == ':') {
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
			if ((search_at = history_prev(&chl, search_at))) {
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
		focus_webview(t);

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
wv_popup_cb(GtkEntry *entry, GtkMenu *menu, struct tab *t)
{
	DNPRINTF(XT_D_CMD, "wv_popup_cb: tab %d\n", t->tab_id);
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

	if (show_url == 0 || t->focus_wv)
		focus_webview(t);
	else
		gtk_widget_grab_focus(GTK_WIDGET(t->uri_entry));

	return (XT_CB_PASSTHROUGH);
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
	hide_cmd(t);

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
		goto done;
	} else if (c[0] == '.' || c[0] == ',') {
		run_script(t, "hints.fire();");
		/* XXX history for link following? */
		goto done;
	}

	history_add(&chl, command_file, s, &cmd_history_count);
	cmd_execute(t, s);
done:
	return;
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

	gtk_entry_set_text(GTK_ENTRY(t->sbe.position), position);
	g_free(position);

	return (TRUE);
}

GtkWidget *
create_window(const gchar *name)
{
	GtkWidget		*w;

	w = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_default_size(GTK_WINDOW(w), window_width, window_height);
	gtk_widget_set_name(w, name);
	gtk_window_set_wmclass(GTK_WINDOW(w), name, "XXXTerm");

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

	t->sb_h = GTK_SCROLLBAR(gtk_hscrollbar_new(NULL));
	t->sb_v = GTK_SCROLLBAR(gtk_vscrollbar_new(NULL));
	t->adjust_h = gtk_range_get_adjustment(GTK_RANGE(t->sb_h));
	t->adjust_v = gtk_range_get_adjustment(GTK_RANGE(t->sb_v));

	w = gtk_scrolled_window_new(t->adjust_h, t->adjust_v);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(w),
	    GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

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
		t->user_agent = g_strdup(user_agent);

	t->stylesheet = g_strdup_printf("file://%s/style.css", resource_dir);

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

	b = gtk_hbox_new(FALSE, 0);
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
	t->stop = create_button("Stop", GTK_STOCK_STOP, 0);
	t->js_toggle = create_button("JS-Toggle", enable_scripts ?
	    GTK_STOCK_MEDIA_PLAY : GTK_STOCK_MEDIA_PAUSE, 0);

	return (toolbar);
}

GtkWidget *
create_toolbar(struct tab *t)
{
	GtkWidget		*toolbar = NULL, *b, *eb1;

	b = gtk_hbox_new(FALSE, 0);
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
	gtk_box_pack_start(GTK_BOX(b), t->forward, FALSE,
	    FALSE, 0);

	/* stop button */
	t->stop = create_button("Stop", GTK_STOCK_STOP, 0);
	gtk_widget_set_sensitive(t->stop, FALSE);
	g_signal_connect(G_OBJECT(t->stop), "clicked",
	    G_CALLBACK(stop_cb), t);
	gtk_box_pack_start(GTK_BOX(b), t->stop, FALSE,
	    FALSE, 0);

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
	completion_add(t);
	eb1 = gtk_hbox_new(FALSE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(eb1), 1);
	gtk_box_pack_start(GTK_BOX(eb1), t->uri_entry, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(b), eb1, TRUE, TRUE, 0);

	/* search entry */
	if (search_string) {
		GtkWidget *eb2;
		t->search_entry = gtk_entry_new();
		gtk_entry_set_width_chars(GTK_ENTRY(t->search_entry), 30);
		g_signal_connect(G_OBJECT(t->search_entry), "activate",
		    G_CALLBACK(activate_search_entry_cb), t);
		g_signal_connect(G_OBJECT(t->search_entry), "key-press-event",
		    G_CALLBACK(entry_key_cb), t);
		gtk_widget_set_size_request(t->search_entry, -1, -1);
		eb2 = gtk_hbox_new(FALSE, 0);
		gtk_container_set_border_width(GTK_CONTAINER(eb2), 1);
		gtk_box_pack_start(GTK_BOX(eb2), t->search_entry, TRUE, TRUE,
		    0);
		gtk_box_pack_start(GTK_BOX(b), eb2, FALSE, FALSE, 0);
	}

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

	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_insert_column_with_attributes
	    (GTK_TREE_VIEW(view), -1, "Title", renderer, "text", COL_TITLE,
	    (char *)NULL);

	gtk_tree_view_set_model
	    (GTK_TREE_VIEW(view), GTK_TREE_MODEL(buffers_store));

	return view;
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
	int			 maxid = 0;

	TAILQ_FOREACH(t, &tabs, entry) {
		t->tab_id = gtk_notebook_page_num(notebook, t->vbox);
		if (t->tab_id > maxid)
			maxid = t->tab_id;

		gtk_widget_show(t->tab_elems.sep);
	}

	TAILQ_FOREACH(t, &tabs, entry) {
		if (t->tab_id == maxid) {
			gtk_widget_hide(t->tab_elems.sep);
			break;
		}
	}
}

/* after active tab change */
void
recolor_compact_tabs(void)
{
	struct tab		*t;
	int			 curid = 0;
	GdkColor		 color;

	gdk_color_parse(XT_COLOR_CT_INACTIVE, &color);
	TAILQ_FOREACH(t, &tabs, entry)
		gtk_widget_modify_fg(t->tab_elems.label, GTK_STATE_NORMAL,
		    &color);

	curid = gtk_notebook_get_current_page(notebook);
	TAILQ_FOREACH(t, &tabs, entry)
		if (t->tab_id == curid) {
			gdk_color_parse(XT_COLOR_CT_ACTIVE, &color);
			gtk_widget_modify_fg(t->tab_elems.label,
			    GTK_STATE_NORMAL, &color);
			break;
		}
}

void
set_current_tab(int page_num)
{
	buffercmd_abort(get_current_tab());
	gtk_notebook_set_current_page(notebook, page_num);
	recolor_compact_tabs();
}

int
undo_close_tab_save(struct tab *t)
{
	int				m, n;
	const gchar			*uri;
	struct undo			*u1, *u2;
	GList				*items;
	WebKitWebHistoryItem		*item;

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

	/* inspector */
	bzero(&a, sizeof a);
	a.i = XT_INS_CLOSE;
	inspector_cmd(t, &a);

	if (browser_mode == XT_BM_KIOSK) {
		gtk_widget_destroy(t->uri_entry);
		gtk_widget_destroy(t->stop);
		gtk_widget_destroy(t->js_toggle);
	}

	gtk_widget_destroy(t->tab_elems.eventbox);
	gtk_widget_destroy(t->vbox);

	g_free(t->user_agent);
	g_free(t->stylesheet);
	g_free(t->tmp_uri);
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
	gtk_entry_set_text(GTK_ENTRY(t->sbe.zoom), s);
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
create_sbe(int width)
{
	GtkWidget		*sbe;

	sbe = gtk_entry_new();
	gtk_entry_set_inner_border(GTK_ENTRY(sbe), NULL);
	gtk_entry_set_has_frame(GTK_ENTRY(sbe), FALSE);
	gtk_widget_set_can_focus(GTK_WIDGET(sbe), FALSE);
	gtk_widget_modify_font(GTK_WIDGET(sbe), statusbar_font);
	gtk_entry_set_alignment(GTK_ENTRY(sbe), 1.0);
	gtk_widget_set_size_request(sbe, width, -1);

	return sbe;
}

struct tab *
create_new_tab(char *title, struct undo *u, int focus, int position)
{
	struct tab			*t;
	int				load = 1, id;
	GtkWidget			*b, *bb;
	WebKitWebHistoryItem		*item;
	GList				*items;
	GdkColor			color;
	char				*p;
	int				sbe_p = 0, sbe_b = 0,
					sbe_z = 0;

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

	t->vbox = gtk_vbox_new(FALSE, 0);

	/* label + button for tab */
	b = gtk_hbox_new(FALSE, 0);
	t->tab_content = b;

#if GTK_CHECK_VERSION(2, 20, 0)
	t->spinner = gtk_spinner_new();
#endif
	t->label = gtk_label_new(title);
	bb = create_button("Close", GTK_STOCK_CLOSE, 1);
	gtk_widget_set_size_request(t->label, 100, 0);
	gtk_label_set_max_width_chars(GTK_LABEL(t->label), 20);
	gtk_label_set_ellipsize(GTK_LABEL(t->label), PANGO_ELLIPSIZE_END);
	gtk_widget_set_size_request(b, 130, 0);

	gtk_box_pack_start(GTK_BOX(b), bb, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(b), t->label, FALSE, FALSE, 0);
#if GTK_CHECK_VERSION(2, 20, 0)
	gtk_box_pack_start(GTK_BOX(b), t->spinner, FALSE, FALSE, 0);
#endif

	/* toolbar */
	if (browser_mode == XT_BM_KIOSK) {
		t->toolbar = create_kiosk_toolbar(t);
		gtk_box_pack_start(GTK_BOX(t->vbox), t->toolbar, FALSE, FALSE,
		    0);
	} else {
		t->toolbar = create_toolbar(t);
		if (fancy_bar)
			gtk_box_pack_start(GTK_BOX(t->vbox), t->toolbar, FALSE,
			    FALSE, 0);
	}

	/* marks */
	marks_clear(t);

	/* browser */
	t->browser_win = create_browser(t);
	gtk_box_pack_start(GTK_BOX(t->vbox), t->browser_win, TRUE, TRUE, 0);

	/* oops message for user feedback */
	t->oops = gtk_entry_new();
	gtk_entry_set_inner_border(GTK_ENTRY(t->oops), NULL);
	gtk_entry_set_has_frame(GTK_ENTRY(t->oops), FALSE);
	gtk_widget_set_can_focus(GTK_WIDGET(t->oops), FALSE);
	gdk_color_parse(XT_COLOR_RED, &color);
	gtk_widget_modify_base(t->oops, GTK_STATE_NORMAL, &color);
	gtk_box_pack_end(GTK_BOX(t->vbox), t->oops, FALSE, FALSE, 0);
	gtk_widget_modify_font(GTK_WIDGET(t->oops), oops_font);

	/* command entry */
	t->cmd = gtk_entry_new();
	gtk_entry_set_inner_border(GTK_ENTRY(t->cmd), NULL);
	gtk_entry_set_has_frame(GTK_ENTRY(t->cmd), FALSE);
	gtk_box_pack_end(GTK_BOX(t->vbox), t->cmd, FALSE, FALSE, 0);
	gtk_widget_modify_font(GTK_WIDGET(t->cmd), cmd_font);

	/* status bar */
	t->statusbar_box = gtk_hbox_new(FALSE, 0);

	t->sbe.statusbar = gtk_entry_new();
	gtk_entry_set_inner_border(GTK_ENTRY(t->sbe.statusbar), NULL);
	gtk_entry_set_has_frame(GTK_ENTRY(t->sbe.statusbar), FALSE);
	gtk_widget_set_can_focus(GTK_WIDGET(t->sbe.statusbar), FALSE);
	gtk_widget_modify_font(GTK_WIDGET(t->sbe.statusbar), statusbar_font);

	/* create these widgets only if specified in statusbar_elems */

	t->sbe.position = create_sbe(40);
	t->sbe.zoom = create_sbe(40);
	t->sbe.buffercmd = create_sbe(60);

	statusbar_modify_attr(t, XT_COLOR_WHITE, XT_COLOR_BLACK);

	gtk_box_pack_start(GTK_BOX(t->statusbar_box), t->sbe.statusbar, TRUE,
	    TRUE, FALSE);

	/* gtk widgets cannot be added to a box twice. sbe_* variables
	   make sure of this */
	for (p = statusbar_elems; *p != '\0'; p++) {
		switch (*p) {
		case '|':
		{
			GtkWidget *sep = gtk_vseparator_new();

			gdk_color_parse(XT_COLOR_SB_SEPARATOR, &color);
			gtk_widget_modify_bg(sep, GTK_STATE_NORMAL, &color);
			gtk_box_pack_start(GTK_BOX(t->statusbar_box), sep,
			    FALSE, FALSE, FALSE);
			break;
		}
		case 'P':
			if (sbe_p) {
				warnx("flag \"%c\" specified more than "
				    "once in statusbar_elems\n", *p);
				break;
			}
			sbe_p = 1;
			gtk_box_pack_start(GTK_BOX(t->statusbar_box),
			    t->sbe.position, FALSE, FALSE, FALSE);
			break;
		case 'B':
			if (sbe_b) {
				warnx("flag \"%c\" specified more than "
				    "once in statusbar_elems\n", *p);
				break;
			}
			sbe_b = 1;
			gtk_box_pack_start(GTK_BOX(t->statusbar_box),
			    t->sbe.buffercmd, FALSE, FALSE, FALSE);
			break;
		case 'Z':
			if (sbe_z) {
				warnx("flag \"%c\" specified more than "
				    "once in statusbar_elems\n", *p);
				break;
			}
			sbe_z = 1;
			gtk_box_pack_start(GTK_BOX(t->statusbar_box),
			    t->sbe.zoom, FALSE, FALSE, FALSE);
			break;
		default:
			warnx("illegal flag \"%c\" in statusbar_elems\n", *p);
			break;
		}
	}

	gtk_box_pack_end(GTK_BOX(t->vbox), t->statusbar_box, FALSE, FALSE, 0);

	/* buffer list */
	t->buffers = create_buffers(t);
	gtk_box_pack_end(GTK_BOX(t->vbox), t->buffers, FALSE, FALSE, 0);

	/* xtp meaning is normal by default */
	t->xtp_meaning = XT_XTP_TAB_MEANING_NORMAL;

	/* set empty favicon */
	xt_icon_from_name(t, "text-html");

	/* and show it all */
	gtk_widget_show_all(b);
	gtk_widget_show_all(t->vbox);

	/* compact tab bar */
	t->tab_elems.label = gtk_label_new(title);
	gtk_label_set_width_chars(GTK_LABEL(t->tab_elems.label), 1.0);
	gtk_misc_set_alignment(GTK_MISC(t->tab_elems.label), 0.0, 0.0);
	gtk_misc_set_padding(GTK_MISC(t->tab_elems.label), 4.0, 4.0);
	gtk_widget_modify_font(GTK_WIDGET(t->tab_elems.label), tabbar_font);

	t->tab_elems.eventbox = gtk_event_box_new();
	t->tab_elems.box = gtk_hbox_new(FALSE, 0);
	t->tab_elems.sep = gtk_vseparator_new();

	gdk_color_parse(XT_COLOR_CT_BACKGROUND, &color);
	gtk_widget_modify_bg(t->tab_elems.eventbox, GTK_STATE_NORMAL, &color);
	gdk_color_parse(XT_COLOR_CT_INACTIVE, &color);
	gtk_widget_modify_fg(t->tab_elems.label, GTK_STATE_NORMAL, &color);
	gdk_color_parse(XT_COLOR_CT_SEPARATOR, &color);
	gtk_widget_modify_bg(t->tab_elems.sep, GTK_STATE_NORMAL, &color);

	gtk_box_pack_start(GTK_BOX(t->tab_elems.box), t->tab_elems.label, TRUE,
	    TRUE, 0);
	gtk_box_pack_start(GTK_BOX(t->tab_elems.box), t->tab_elems.sep, FALSE,
	    FALSE, 0);
	gtk_container_add(GTK_CONTAINER(t->tab_elems.eventbox),
	    t->tab_elems.box);

	gtk_box_pack_start(GTK_BOX(tab_bar), t->tab_elems.eventbox, TRUE,
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
			gtk_box_reorder_child(GTK_BOX(tab_bar),
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
	    "signal::key-press-event", G_CALLBACK(cmd_keypress_cb), t,
	    "signal::key-release-event", G_CALLBACK(cmd_keyrelease_cb), t,
	    "signal::focus-out-event", G_CALLBACK(cmd_focusout_cb), t,
	    "signal::activate", G_CALLBACK(cmd_activate_cb), t,
	    "signal::populate-popup", G_CALLBACK(cmd_popup_cb), t,
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
	    "signal-after::key-press-event", G_CALLBACK(wv_keypress_after_cb), t,
	    "signal::hovering-over-link", G_CALLBACK(webview_hover_cb), t,
	    "signal::download-requested", G_CALLBACK(webview_download_cb), t,
	    "signal::mime-type-policy-decision-requested", G_CALLBACK(webview_mimetype_cb), t,
	    "signal::navigation-policy-decision-requested", G_CALLBACK(webview_npd_cb), t,
	    "signal::new-window-policy-decision-requested", G_CALLBACK(webview_npd_cb), t,
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
	g_object_connect(G_OBJECT(t->toolbar),
	    "signal-after::key-press-event", G_CALLBACK(wv_keypress_after_cb), t,
	    (char *)NULL);

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

	/* hide stuff */
	hide_cmd(t);
	hide_oops(t);
	hide_buffers(t);
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

			if (t->focus_wv) {
				/* can't use focus_webview here */
				gtk_widget_grab_focus(GTK_WIDGET(t->wv));
			}
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

	gtk_box_reorder_child(GTK_BOX(tab_bar), t->tab_elems.eventbox,
	    t->tab_id);
}

void
menuitem_response(struct tab *t)
{
	gtk_notebook_set_current_page(notebook, t->tab_id);
}

gboolean
arrow_cb(GtkWidget *w, GdkEventButton *event, gpointer user_data)
{
	GtkWidget		*menu, *menu_items;
	GdkEventButton		*bevent;
	const gchar		*uri;
	struct tab		*ti;

	if (event->type == GDK_BUTTON_PRESS) {
		bevent = (GdkEventButton *) event;
		menu = gtk_menu_new();

		TAILQ_FOREACH(ti, &tabs, entry) {
			if ((uri = get_uri(ti)) == NULL)
				/* XXX make sure there is something to print */
				/* XXX add gui pages in here to look purdy */
				uri = "(untitled)";
			menu_items = gtk_menu_item_new_with_label(uri);
			gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_items);
			gtk_widget_show(menu_items);

			g_signal_connect_swapped((menu_items),
			    "activate", G_CALLBACK(menuitem_response),
			    (gpointer)ti);
		}

		gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL,
		    bevent->button, bevent->time);

		/* unref object so it'll free itself when popped down */
#if !GTK_CHECK_VERSION(3, 0, 0)
		/* XXX does not need unref with gtk+3? */
		g_object_ref_sink(menu);
		g_object_unref(menu);
#endif

		return (TRUE /* eat event */);
	}

	return (FALSE /* propagate */);
}

int
icon_size_map(int icon_size)
{
	if (icon_size <= GTK_ICON_SIZE_INVALID ||
	    icon_size > GTK_ICON_SIZE_DIALOG)
		return (GTK_ICON_SIZE_SMALL_TOOLBAR);

	return (icon_size);
}

GtkWidget *
create_button(char *name, char *stockid, int size)
{
	GtkWidget		*button, *image;
	gchar			*rcstring;
	int			gtk_icon_size;

	rcstring = g_strdup_printf(
	    "style \"%s-style\"\n"
	    "{\n"
	    "  GtkWidget::focus-padding = 0\n"
	    "  GtkWidget::focus-line-width = 0\n"
	    "  xthickness = 0\n"
	    "  ythickness = 0\n"
	    "}\n"
	    "widget \"*.%s\" style \"%s-style\"", name, name, name);
	gtk_rc_parse_string(rcstring);
	g_free(rcstring);
	button = gtk_button_new();
	gtk_button_set_focus_on_click(GTK_BUTTON(button), FALSE);
	gtk_icon_size = icon_size_map(size ? size : icon_size);

	image = gtk_image_new_from_stock(stockid, gtk_icon_size);
	gtk_widget_set_size_request(GTK_WIDGET(image), -1, -1);
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
	gtk_widget_set_size_request(GTK_WIDGET(image), -1, -1);
	gtk_button_set_image(GTK_BUTTON(button), image);
}

void
clipb_primary_cb(GtkClipboard *primary, GdkEvent *event, gpointer notused)
{
	gchar			*p = NULL;
	GdkAtom			atom = gdk_atom_intern("CUT_BUFFER0", FALSE);
	gint			len;

	if (xterm_workaround == 0)
		return;

	/*
	 * xterm doesn't play nice with clipboards because it clears the
	 * primary when clicked.  We rely on primary being set to properly
	 * handle middle mouse button clicks (paste).  So when someone clears
	 * primary copy whatever is in CUT_BUFFER0 into primary to simualte
	 * other application behavior (as in DON'T clear primary).
	 */

	p = gtk_clipboard_wait_for_text(primary);
	if (p == NULL) {
		if (gdk_property_get(gdk_get_default_root_window(),
		    atom,
		    gdk_atom_intern("STRING", FALSE),
		    0,
		    1024 * 1024 /* picked out of my butt */,
		    FALSE,
		    NULL,
		    NULL,
		    &len,
		    (guchar **)&p)) {
			/* yes sir, we need to NUL the string */
			p[len] = '\0';
			gtk_clipboard_set_text(primary, p, -1);
		}
	}

	if (p)
		g_free(p);
}

void
create_canvas(void)
{
	GtkWidget		*vbox;
	GList			*l = NULL;
	GdkPixbuf		*pb;
	char			file[PATH_MAX];
	int			i;

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_box_set_spacing(GTK_BOX(vbox), 0);
	notebook = GTK_NOTEBOOK(gtk_notebook_new());
#if !GTK_CHECK_VERSION(3, 0, 0)
	/* XXX seems to be needed with gtk+2 */
	gtk_notebook_set_tab_hborder(notebook, 0);
	gtk_notebook_set_tab_vborder(notebook, 0);
#endif
	gtk_notebook_set_scrollable(notebook, TRUE);
	gtk_notebook_set_show_border(notebook, FALSE);
	gtk_widget_set_can_focus(GTK_WIDGET(notebook), FALSE);

	abtn = gtk_button_new();
	arrow = gtk_arrow_new(GTK_ARROW_DOWN, GTK_SHADOW_NONE);
	gtk_widget_set_size_request(arrow, -1, -1);
	gtk_container_add(GTK_CONTAINER(abtn), arrow);
	gtk_widget_set_size_request(abtn, -1, 20);

#if GTK_CHECK_VERSION(2, 20, 0)
	gtk_notebook_set_action_widget(notebook, abtn, GTK_PACK_END);
#endif
	gtk_widget_set_size_request(GTK_WIDGET(notebook), -1, -1);

	/* compact tab bar */
	tab_bar = gtk_hbox_new(TRUE, 0);

	gtk_box_pack_start(GTK_BOX(vbox), tab_bar, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), GTK_WIDGET(notebook), TRUE, TRUE, 0);
	gtk_widget_set_size_request(vbox, -1, -1);

	g_object_connect(G_OBJECT(notebook),
	    "signal::switch-page", G_CALLBACK(notebook_switchpage_cb), NULL,
	    (char *)NULL);
	g_object_connect(G_OBJECT(notebook),
	    "signal::page-reordered", G_CALLBACK(notebook_pagereordered_cb),
	    NULL, (char *)NULL);
	g_signal_connect(G_OBJECT(abtn), "button_press_event",
	    G_CALLBACK(arrow_cb), NULL);

	main_window = create_window("xxxterm");
	gtk_container_add(GTK_CONTAINER(main_window), vbox);
	g_signal_connect(G_OBJECT(main_window), "delete_event",
	    G_CALLBACK(gtk_main_quit), NULL);

	/* icons */
	for (i = 0; i < LENGTH(icons); i++) {
		snprintf(file, sizeof file, "%s/%s", resource_dir, icons[i]);
		pb = gdk_pixbuf_new_from_file(file, NULL);
		l = g_list_append(l, pb);
	}
	gtk_window_set_default_icon_list(l);

	/* clipboard work around */
	if (xterm_workaround)
		g_signal_connect(
		    G_OBJECT(gtk_clipboard_get(GDK_SELECTION_PRIMARY)),
		    "owner-change", G_CALLBACK(clipb_primary_cb), NULL);

	gtk_widget_show_all(abtn);
	gtk_widget_show_all(main_window);
	notebook_tab_set_visibility();
}

void
set_hook(void **hook, char *name)
{
	if (hook == NULL)
		errx(1, "set_hook");

	if (*hook == NULL) {
		*hook = dlsym(RTLD_NEXT, name);
		if (*hook == NULL)
			errx(1, "can't hook %s", name);
	}
}

/* override libsoup soup_cookie_equal because it doesn't look at domain */
gboolean
soup_cookie_equal(SoupCookie *cookie1, SoupCookie *cookie2)
{
	g_return_val_if_fail(cookie1, FALSE);
	g_return_val_if_fail(cookie2, FALSE);

	return (!strcmp (cookie1->name, cookie2->name) &&
	    !strcmp (cookie1->value, cookie2->value) &&
	    !strcmp (cookie1->path, cookie2->path) &&
	    !strcmp (cookie1->domain, cookie2->domain));
}

void
transfer_cookies(void)
{
	GSList			*cf;
	SoupCookie		*sc, *pc;

	cf = soup_cookie_jar_all_cookies(p_cookiejar);

	for (;cf; cf = cf->next) {
		pc = cf->data;
		sc = soup_cookie_copy(pc);
		_soup_cookie_jar_add_cookie(s_cookiejar, sc);
	}

	soup_cookies_free(cf);
}

void
soup_cookie_jar_delete_cookie(SoupCookieJar *jar, SoupCookie *c)
{
	GSList			*cf;
	SoupCookie		*ci;

	print_cookie("soup_cookie_jar_delete_cookie", c);

	if (cookies_enabled == 0)
		return;

	if (jar == NULL || c == NULL)
		return;

	/* find and remove from persistent jar */
	cf = soup_cookie_jar_all_cookies(p_cookiejar);

	for (;cf; cf = cf->next) {
		ci = cf->data;
		if (soup_cookie_equal(ci, c)) {
			_soup_cookie_jar_delete_cookie(p_cookiejar, ci);
			break;
		}
	}

	soup_cookies_free(cf);

	/* delete from session jar */
	_soup_cookie_jar_delete_cookie(s_cookiejar, c);
}

void
soup_cookie_jar_add_cookie(SoupCookieJar *jar, SoupCookie *cookie)
{
	struct domain		*d = NULL;
	SoupCookie		*c;
	FILE			*r_cookie_f;

	DNPRINTF(XT_D_COOKIE, "soup_cookie_jar_add_cookie: %p %p %p\n",
	    jar, p_cookiejar, s_cookiejar);

	if (cookies_enabled == 0)
		return;

	/* see if we are up and running */
	if (p_cookiejar == NULL) {
		_soup_cookie_jar_add_cookie(jar, cookie);
		return;
	}
	/* disallow p_cookiejar adds, shouldn't happen */
	if (jar == p_cookiejar)
		return;

	/* sanity */
	if (jar == NULL || cookie == NULL)
		return;

	if (enable_cookie_whitelist &&
	    (d = wl_find(cookie->domain, &c_wl)) == NULL) {
		blocked_cookies++;
		DNPRINTF(XT_D_COOKIE,
		    "soup_cookie_jar_add_cookie: reject %s\n",
		    cookie->domain);
		if (save_rejected_cookies) {
			if ((r_cookie_f = fopen(rc_fname, "a+")) == NULL) {
				show_oops(NULL, "can't open reject cookie file");
				return;
			}
			fseek(r_cookie_f, 0, SEEK_END);
			fprintf(r_cookie_f, "%s%s\t%s\t%s\t%s\t%lu\t%s\t%s\n",
			    cookie->http_only ? "#HttpOnly_" : "",
			    cookie->domain,
			    *cookie->domain == '.' ? "TRUE" : "FALSE",
			    cookie->path,
			    cookie->secure ? "TRUE" : "FALSE",
			    cookie->expires ?
			        (gulong)soup_date_to_time_t(cookie->expires) :
			        0,
			    cookie->name,
			    cookie->value);
			fflush(r_cookie_f);
			fclose(r_cookie_f);
		}
		if (!allow_volatile_cookies)
			return;
	}

	if (cookie->expires == NULL && session_timeout) {
		soup_cookie_set_expires(cookie,
		    soup_date_new_from_now(session_timeout));
		print_cookie("modified add cookie", cookie);
	}

	/* see if we are white listed for persistence */
	if ((d && d->handy) || (enable_cookie_whitelist == 0)) {
		/* add to persistent jar */
		c = soup_cookie_copy(cookie);
		print_cookie("soup_cookie_jar_add_cookie p_cookiejar", c);
		_soup_cookie_jar_add_cookie(p_cookiejar, c);
	}

	/* add to session jar */
	print_cookie("soup_cookie_jar_add_cookie s_cookiejar", cookie);
	_soup_cookie_jar_add_cookie(s_cookiejar, cookie);
}

void
setup_cookies(void)
{
	char			file[PATH_MAX];

	set_hook((void *)&_soup_cookie_jar_add_cookie,
	    "soup_cookie_jar_add_cookie");
	set_hook((void *)&_soup_cookie_jar_delete_cookie,
	    "soup_cookie_jar_delete_cookie");

	if (cookies_enabled == 0)
		return;

	/*
	 * the following code is intricate due to overriding several libsoup
	 * functions.
	 * do not alter order of these operations.
	 */

	/* rejected cookies */
	if (save_rejected_cookies)
		snprintf(rc_fname, sizeof file, "%s/%s", work_dir,
		    XT_REJECT_FILE);

	/* persistent cookies */
	snprintf(file, sizeof file, "%s/%s", work_dir, XT_COOKIE_FILE);
	p_cookiejar = soup_cookie_jar_text_new(file, read_only_cookies);

	/* session cookies */
	s_cookiejar = soup_cookie_jar_new();
	g_object_set(G_OBJECT(s_cookiejar), SOUP_COOKIE_JAR_ACCEPT_POLICY,
	    cookie_policy, (void *)NULL);
	transfer_cookies();

	soup_session_add_feature(session, (SoupSessionFeature*)s_cookiejar);
}

void
setup_proxy(char *uri)
{
	if (proxy_uri) {
		g_object_set(session, "proxy_uri", NULL, (char *)NULL);
		soup_uri_free(proxy_uri);
		proxy_uri = NULL;
	}
	if (http_proxy) {
		if (http_proxy != uri) {
			g_free(http_proxy);
			http_proxy = NULL;
		}
	}

	if (uri) {
		http_proxy = g_strdup(uri);
		DNPRINTF(XT_D_CONFIG, "setup_proxy: %s\n", uri);
		proxy_uri = soup_uri_new(http_proxy);
		if (!(proxy_uri == NULL || !SOUP_URI_VALID_FOR_HTTP(proxy_uri)))
			g_object_set(session, "proxy-uri", proxy_uri,
			    (char *)NULL);
	}
}

int
set_http_proxy(char *proxy)
{
	SoupURI			*uri;

	if (proxy == NULL)
		return (1);

	/* see if we need to clear it instead */
	if (strlen(proxy) == 0) {
		setup_proxy(NULL);
		return (0);
	}

	uri = soup_uri_new(proxy);
	if (uri == NULL || !SOUP_URI_VALID_FOR_HTTP(uri))
		return (1);

	setup_proxy(proxy);

	soup_uri_free(uri);

	return (0);
}

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
	snprintf(sa.sun_path, sizeof(sa.sun_path), "%s/%s",
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

	tt = TAILQ_LAST(&tabs, tab_list);
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
	snprintf(sa.sun_path, sizeof(sa.sun_path), "%s/%s",
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
	snprintf(sa.sun_path, sizeof(sa.sun_path), "%s/%s",
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

void
xxx_dir(char *dir)
{
	struct stat		sb;

	if (stat(dir, &sb)) {
		if (mkdir(dir, S_IRWXU) == -1)
			err(1, "mkdir %s", dir);
		if (stat(dir, &sb))
			err(1, "stat %s", dir);
	}
	if (S_ISDIR(sb.st_mode) == 0)
		errx(1, "%s not a dir", dir);
	if (((sb.st_mode & (S_IRWXU | S_IRWXG | S_IRWXO))) != S_IRWXU) {
		warnx("fixing invalid permissions on %s", dir);
		if (chmod(dir, S_IRWXU) == -1)
			err(1, "chmod %s", dir);
	}
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
	g_static_rec_mutex_lock(&my_gdk_mtx);
	mtx_depth++;

	if (mtx_depth <= 0) {
		/* should not happen */
		show_oops(NULL, "negative mutex locking bug, trying to "
		    "correct");
		fprintf(stderr, "negative mutex locking bug, trying to "
		    "correct\n");
		g_static_rec_mutex_unlock_full(&my_gdk_mtx);
		g_static_rec_mutex_lock(&my_gdk_mtx);
		mtx_depth = 1;
		return;
	}

	if (mtx_depth != 1) {
		/* decrease mutext depth to 1 */
		do {
			g_static_rec_mutex_unlock(&my_gdk_mtx);
			mtx_depth--;
		} while (mtx_depth > 1);
	}
}

void
mtx_unlock(void)
{
	guint x;

	/* if mutex depth isn't 1 then something went bad */
	if (mtx_depth != 1) {
		x = g_static_rec_mutex_unlock_full(&my_gdk_mtx);
		if (x != 1) {
			/* should not happen */
			show_oops(NULL, "mutex unlocking bug, trying to "
			    "correct");
			fprintf(stderr, "mutex unlocking bug, trying to "
			    "correct\n");
		}
		mtx_depth = 0;
		if (mtx_complain == 0) {
			show_oops(NULL, "buggy mutex implementation detected, "
			    "work around implemented");
			fprintf(stderr, "buggy mutex implementation detected, "
			    "work around implemented");
			mtx_complain = 1;
		}
		return;
	}

	mtx_depth--;
	g_static_rec_mutex_unlock(&my_gdk_mtx);
}

int
main(int argc, char *argv[])
{
	struct stat		sb;
	int			c, s, optn = 0, opte = 0, focus = 1;
	char			conf[PATH_MAX] = { '\0' };
	char			file[PATH_MAX];
	char			*env_proxy = NULL;
	char			*cmd = NULL;
	FILE			*f = NULL;
	struct karg		a;
	struct sigaction	sact;
	GIOChannel		*channel;
	struct rlimit		rlp;

	start_argv = argv;

	/* prepare gtk */
#ifdef USE_THREADS
	g_thread_init(NULL);
	gdk_threads_set_lock_functions(mtx_lock, mtx_unlock);
	gdk_threads_init();
	gdk_threads_enter();

	gcry_control (GCRYCTL_SET_THREAD_CBS, &gcry_threads_pthread);
#endif
	gtk_init(&argc, &argv);

	gnutls_global_init();

	strlcpy(named_session, XT_SAVED_TABS_FILE, sizeof named_session);

	RB_INIT(&hl);
	RB_INIT(&js_wl);
	RB_INIT(&pl_wl);
	RB_INIT(&downloads);

	TAILQ_INIT(&sessions);
	TAILQ_INIT(&tabs);
	TAILQ_INIT(&mtl);
	TAILQ_INIT(&aliases);
	TAILQ_INIT(&undos);
	TAILQ_INIT(&kbl);
	TAILQ_INIT(&spl);
	TAILQ_INIT(&chl);
	TAILQ_INIT(&shl);

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
		else if (rlp.rlim_cur <= 256)
			startpage_add("%s requires at least 256 file "
			    "descriptors, currently it has up to %d available",
			   __progname, rlp.rlim_cur);
	}

	while ((c = getopt(argc, argv, "STVf:s:tne")) != -1) {
		switch (c) {
		case 'S':
			show_url = 0;
			break;
		case 'T':
			show_tabs = 0;
			break;
		case 'V':
			errx(0 , "Version: %s", version);
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

	xtp_generate_keys();

	/* signals */
	bzero(&sact, sizeof(sact));
	sigemptyset(&sact.sa_mask);
	sact.sa_handler = sigchild;
	sact.sa_flags = SA_NOCLDSTOP;
	sigaction(SIGCHLD, &sact, NULL);

	/* set download dir */
	pwd = getpwuid(getuid());
	if (pwd == NULL)
		errx(1, "invalid user %d", getuid());
	strlcpy(download_dir, pwd->pw_dir, sizeof download_dir);

	/* compile buffer command regexes */
	buffercmd_init();

	/* set default string settings */
	home = g_strdup("https://www.cyphertite.com");
	search_string = g_strdup("https://ssl.scroogle.org/cgi-bin/nbbwssl.cgi?Gw=%s");
	resource_dir = g_strdup("/usr/local/share/xxxterm/");
	strlcpy(runtime_settings, "runtime", sizeof runtime_settings);
	cmd_font_name = g_strdup("monospace normal 9");
	oops_font_name = g_strdup("monospace normal 9");
	statusbar_font_name = g_strdup("monospace normal 9");
	tabbar_font_name = g_strdup("monospace normal 9");
	statusbar_elems = g_strdup("BP");
	encoding = g_strdup("ISO-8859-1");

	/* read config file */
	if (strlen(conf) == 0)
		snprintf(conf, sizeof conf, "%s/.%s",
		    pwd->pw_dir, XT_CONF_FILE);
	config_parse(conf, 0);

	/* init fonts */
	cmd_font = pango_font_description_from_string(cmd_font_name);
	oops_font = pango_font_description_from_string(oops_font_name);
	statusbar_font = pango_font_description_from_string(statusbar_font_name);
	tabbar_font = pango_font_description_from_string(tabbar_font_name);

	/* working directory */
	if (strlen(work_dir) == 0)
		snprintf(work_dir, sizeof work_dir, "%s/%s",
		    pwd->pw_dir, XT_DIR);
	xxx_dir(work_dir);

	/* icon cache dir */
	snprintf(cache_dir, sizeof cache_dir, "%s/%s", work_dir, XT_CACHE_DIR);
	xxx_dir(cache_dir);

	/* certs dir */
	snprintf(certs_dir, sizeof certs_dir, "%s/%s", work_dir, XT_CERT_DIR);
	xxx_dir(certs_dir);

	/* sessions dir */
	snprintf(sessions_dir, sizeof sessions_dir, "%s/%s",
	    work_dir, XT_SESSIONS_DIR);
	xxx_dir(sessions_dir);

	/* runtime settings that can override config file */
	if (runtime_settings[0] != '\0')
		config_parse(runtime_settings, 1);

	/* download dir */
	if (!strcmp(download_dir, pwd->pw_dir))
		strlcat(download_dir, "/downloads", sizeof download_dir);
	xxx_dir(download_dir);

	/* favorites file */
	snprintf(file, sizeof file, "%s/%s", work_dir, XT_FAVS_FILE);
	if (stat(file, &sb)) {
		warnx("favorites file doesn't exist, creating it");
		if ((f = fopen(file, "w")) == NULL)
			err(1, "favorites");
		fclose(f);
	}

	/* quickmarks file */
	snprintf(file, sizeof file, "%s/%s", work_dir, XT_QMARKS_FILE);
	if (stat(file, &sb)) {
		warnx("quickmarks file doesn't exist, creating it");
		if ((f = fopen(file, "w")) == NULL)
			err(1, "quickmarks");
		fclose(f);
	}

	/* search history */
	if (history_autosave) {
		snprintf(search_file, sizeof search_file, "%s/%s",
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
		snprintf(command_file, sizeof command_file, "%s/%s",
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
	session = webkit_get_default_session();
	setup_cookies();

	/* certs */
	if (ssl_ca_file) {
		if (stat(ssl_ca_file, &sb)) {
			warnx("no CA file: %s", ssl_ca_file);
			g_free(ssl_ca_file);
			ssl_ca_file = NULL;
		} else
			g_object_set(session,
			    SOUP_SESSION_SSL_CA_FILE, ssl_ca_file,
			    SOUP_SESSION_SSL_STRICT, ssl_strict_certs,
			    (void *)NULL);
	}

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
	else
		setup_proxy(http_proxy);

	if (opte) {
		send_cmd_to_socket(argv[0]);
		exit(0);
	}

	/* set some connection parameters */
	g_object_set(session, "max-conns", max_connections, (char *)NULL);
	g_object_set(session, "max-conns-per-host", max_host_connections,
	    (char *)NULL);

	/* see if there is already an xxxterm running */
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

	/* uri completion */
	completion_model = gtk_list_store_new(1, G_TYPE_STRING);

	/* buffers */
	buffers_store = gtk_list_store_new
	    (NUM_COLS, G_TYPE_UINT, G_TYPE_STRING);

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

	if (enable_socket)
		if ((s = build_socket()) != -1) {
			channel = g_io_channel_unix_new(s);
			g_io_add_watch(channel, G_IO_IN, socket_watcher, NULL);
		}

	gtk_main();

#ifdef USE_THREADS
	gdk_threads_leave();
	g_static_rec_mutex_unlock_full(&my_gdk_mtx); /* just in case */
#endif

	gnutls_global_deinit();

	if (url_regex)
		regfree(&url_re);

	return (0);
}
