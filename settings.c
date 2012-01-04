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

/* globals */
SoupURI			*proxy_uri = NULL;
PangoFontDescription	*cmd_font;
PangoFontDescription	*oops_font;
PangoFontDescription	*statusbar_font;
PangoFontDescription	*tabbar_font;

/* settings that require restart */
int		tabless = 0;	/* allow only 1 tab */
int		enable_socket = 0;
int		single_instance = 0; /* only allow one xxxterm to run */
int		fancy_bar = 1;	/* fancy toolbar */
int		browser_mode = XT_BM_NORMAL;
int		gui_mode = XT_GM_CLASSIC;
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
int		window_maximize = 0;
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
int		color_visited_uris = 1;
int		save_global_history = 0; /* save global history to disk */
struct user_agent	*user_agent = NULL;
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
int		js_autorun_enabled = 1;
int		edit_mode = XT_EM_HYBRID;
int		userstyle_global = 0;
int		auto_load_images = 1;
int		enable_autoscroll = 0;
int		enable_favicon_entry = 1;
int		enable_favicon_tabs = 0;
char		*external_editor = NULL;

char		*cmd_font_name = NULL;
char		*oops_font_name = NULL;
char		*statusbar_font_name = NULL;
char		*tabbar_font_name = NULL;

char		*get_download_dir(struct settings *);
char		*get_default_script(struct settings *);
char		*get_runtime_dir(struct settings *);
char		*get_tab_style(struct settings *);
char		*get_edit_mode(struct settings *);
char		*get_work_dir(struct settings *);

int		add_cookie_wl(struct settings *, char *);
int		add_js_wl(struct settings *, char *);
int		add_pl_wl(struct settings *, char *);
int		add_mime_type(struct settings *, char *);
int		add_alias(struct settings *, char *);
int		add_kb(struct settings *, char *);
int		add_ua(struct settings *, char *);

int		set_download_dir(struct settings *, char *);
int		set_default_script(struct settings *, char *);
int		set_runtime_dir(struct settings *, char *);
int		set_tab_style(struct settings *, char *);
int		set_edit_mode(struct settings *, char *);
int		set_work_dir(struct settings *, char *);
int		set_auto_load_images(char *value);
int		set_enable_autoscroll(char *value);
int		set_enable_favicon_entry(char *value);
int		set_enable_favicon_tabs(char *value);
int		set_external_editor(char *);

void		walk_mime_type(struct settings *, void (*)(struct settings *,
		    char *, void *), void *);
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
void		walk_ua(struct settings *, void (*)(struct settings *, char *,
		    void *), void *);

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

struct special		s_gui_mode = {
	set_gui_mode,
	get_gui_mode,
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

struct special		s_edit_mode = {
	set_edit_mode,
	get_edit_mode,
	NULL
};

struct special		s_ua = {
	add_ua,
	NULL,
	walk_ua
};

struct settings		rs[] = {
	{ "allow_volatile_cookies",	XT_S_INT, 0,		&allow_volatile_cookies, NULL, NULL },
	{ "append_next",		XT_S_INT, 0,		&append_next, NULL, NULL },
	{ "autofocus_onload",		XT_S_INT, 0,		&autofocus_onload, NULL, NULL },
	{ "browser_mode",		XT_S_INT, 0, NULL, NULL,&s_browser_mode },
	{ "gui_mode",			XT_S_INT, 0, NULL, NULL,&s_gui_mode },
	{ "color_visited_uris",		XT_S_INT, 0,		&color_visited_uris, NULL, NULL },
	{ "cookie_policy",		XT_S_INT, 0, NULL, NULL,&s_cookie },
	{ "cookies_enabled",		XT_S_INT, 0,		&cookies_enabled, NULL, NULL },
	{ "ctrl_click_focus",		XT_S_INT, 0,		&ctrl_click_focus, NULL, NULL },
	{ "default_zoom_level",		XT_S_FLOAT, 0,		NULL, NULL, NULL, &default_zoom_level },
	{ "default_script",		XT_S_STR, 0, NULL, NULL,&s_default_script },
	{ "download_dir",		XT_S_STR, 0, NULL, NULL,&s_download_dir },
	{ "edit_mode",			XT_S_STR, 0, NULL, NULL,&s_edit_mode },
	{ "enable_cookie_whitelist",	XT_S_INT, 0,		&enable_cookie_whitelist, NULL, NULL },
	{ "enable_js_whitelist",	XT_S_INT, 0,		&enable_js_whitelist, NULL, NULL },
	{ "enable_plugin_whitelist",	XT_S_INT, 0,		&enable_plugin_whitelist, NULL, NULL },
	{ "enable_localstorage",	XT_S_INT, 0,		&enable_localstorage, NULL, NULL },
	{ "enable_plugins",		XT_S_INT, 0,		&enable_plugins, NULL, NULL },
	{ "enable_scripts",		XT_S_INT, 0,		&enable_scripts, NULL, NULL },
	{ "enable_socket",		XT_S_INT, XT_SF_RESTART,&enable_socket, NULL, NULL },
	{ "enable_spell_checking",	XT_S_INT, 0,		&enable_spell_checking, NULL, NULL },
	{ "encoding",			XT_S_STR, 0, NULL,	&encoding, NULL },
	{ "external_editor",		XT_S_STR,0, NULL,	&external_editor, NULL, NULL, set_external_editor },
	{ "fancy_bar",			XT_S_INT, XT_SF_RESTART,&fancy_bar, NULL, NULL },
	{ "guess_search",		XT_S_INT, 0,		&guess_search, NULL, NULL },
	{ "history_autosave",		XT_S_INT, 0,		&history_autosave, NULL, NULL },
	{ "home",			XT_S_STR, 0, NULL,	&home, NULL },
	{ "http_proxy",			XT_S_STR, 0, NULL,	&http_proxy, NULL, NULL, set_http_proxy },
	{ "icon_size",			XT_S_INT, 0,		&icon_size, NULL, NULL },
	{ "js_autorun_enabled",		XT_S_INT, 0,		&js_autorun_enabled, NULL, NULL },
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
	{ "window_height",		XT_S_INT, 0,		&window_height, NULL, NULL },
	{ "window_width",		XT_S_INT, 0,		&window_width, NULL, NULL },
	{ "window_maximize",		XT_S_INT, 0,		&window_maximize, NULL, NULL },
	{ "work_dir",			XT_S_STR, 0, NULL, NULL,&s_work_dir },
	{ "xterm_workaround",		XT_S_INT, 0,		&xterm_workaround, NULL, NULL },
	{ "auto_load_images",		XT_S_INT, 0,		&auto_load_images, NULL, NULL, NULL, set_auto_load_images },
	{ "enable_autoscroll",		XT_S_INT, 0,		&enable_autoscroll, NULL, NULL, NULL, set_enable_autoscroll },
	{ "enable_favicon_entry",	XT_S_INT, 0,		&enable_favicon_entry, NULL, NULL, NULL, set_enable_favicon_entry },
	{ "enable_favicon_tabs",	XT_S_INT, 0,		&enable_favicon_tabs, NULL, NULL, NULL, set_enable_favicon_tabs },

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
	{ "user_agent",			XT_S_STR, XT_SF_RUNTIME, NULL,	NULL, &s_ua },
};

size_t
get_settings_size(void)
{
	return (LENGTH(rs));
}

char *
get_setting_name(int i)
{
	if (i > LENGTH(rs))
		return (NULL);
	return (rs[i].name);
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
set_gui_mode(struct settings *s, char *val)
{
	if (!strcmp(val, "classic")) {
		fancy_bar = 1;
		show_tabs = 1;
		tab_style = XT_TABS_NORMAL;
		show_url = 1;
		show_statusbar = 0;
	} else if (!strcmp(val, "minimal")) {
		fancy_bar = 0;
		show_tabs = 1;
		tab_style = XT_TABS_COMPACT;
		show_url = 0;
		show_statusbar = 1;
	} else
		return (1);

	return (0);
}

char *
get_gui_mode(struct settings *s)
{
	char			*r = NULL;

	if (gui_mode == XT_GM_CLASSIC)
		r = g_strdup("classic");
	else if (browser_mode == XT_GM_MINIMAL)
		r = g_strdup("minimal");
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

/* inherent to GTK not all keys will be caught at all times */
/* XXX sort key bindings */
struct key_binding	keys[] = {
	{ "command_mode",	0,	1,	GDK_Escape	},
	{ "insert_mode",	0,	0,	GDK_i		},
	{ "cookiejar",		MOD1,	1,	GDK_j		},
	{ "downloadmgr",	MOD1,	1,	GDK_d		},
	{ "history",		MOD1,	1,	GDK_h		},
	{ "print",		CTRL,	1,	GDK_p		},
	{ "search",		0,	0,	GDK_slash	},
	{ "searchb",		0,	0,	GDK_question	},
	{ "statustoggle",	CTRL,	1,	GDK_n		},
	{ "command",		0,	0,	GDK_colon	},
	{ "qa",			CTRL,	1,	GDK_q		},
	{ "restart",		MOD1,	1,	GDK_q		},
	{ "js toggle",		CTRL,	1,	GDK_j		},
	{ "plugin toggle",	MOD1,	1,	GDK_p		},
	{ "cookie toggle",	MOD1,	1,	GDK_c		},
	{ "togglesrc",		CTRL,	1,	GDK_s		},
	{ "yankuri",		0,	0,	GDK_y		},
	{ "pasteuricur",	0,	0,	GDK_p		},
	{ "pasteurinew",	0,	0,	GDK_P		},
	{ "toplevel toggle",	0,	1,	GDK_F4		},
	{ "help",		0,	1,	GDK_F1		},
	{ "run_script",		MOD1,	1,	GDK_r		},
	{ "proxy toggle",	0,	1,	GDK_F2		},
	{ "editelement",	CTRL,	1,	GDK_i		},

	/* search */
	{ "searchnext",		0,	0,	GDK_n		},
	{ "searchprevious",	0,	0,	GDK_N		},

	/* focus */
	{ "focusaddress",	0,	1,	GDK_F6		},
	{ "focussearch",	0,	1,	GDK_F7		},

	/* hinting */
	{ "hinting",		0,	0,	GDK_f		},
	{ "hinting",		0,	0,	GDK_period	},
	{ "hinting_newtab",	SHFT,	0,	GDK_F		},
	{ "hinting_newtab",	0,	0,	GDK_comma	},

	/* custom stylesheet */
	{ "userstyle",		0,	0,	GDK_s		},
	{ "userstyle_global",	SHFT,	0,	GDK_S		},

	/* navigation */
	{ "goback",		0,	0,	GDK_BackSpace	},
	{ "goback",		MOD1,	1,	GDK_Left	},
	{ "goforward",		SHFT,	1,	GDK_BackSpace	},
	{ "goforward",		MOD1,	1,	GDK_Right	},
	{ "reload",		0,	1,	GDK_F5		},
	{ "reload",		CTRL,	1,	GDK_r		},
	{ "reload",		CTRL,	1,	GDK_l		},
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
	{ "scrollpagedown",	CTRL,	1,	GDK_f		},
	{ "scrollhalfdown",	CTRL,	1,	GDK_d		},
	{ "scrollpagedown",	0,	0,	GDK_Page_Down	},
	{ "scrollpageup",	0,	0,	GDK_Page_Up	},
	{ "scrollpageup",	CTRL,	1,	GDK_b		},
	{ "scrollhalfup",	CTRL,	1,	GDK_u		},
	/* horizontal movement */
	{ "scrollright",	0,	0,	GDK_l		},
	{ "scrollright",	0,	0,	GDK_Right	},
	{ "scrollleft",		0,	0,	GDK_Left	},
	{ "scrollleft",		0,	0,	GDK_h		},
	{ "scrollfarright",	0,	0,	GDK_dollar	},
	{ "scrollfarleft",	0,	0,	GDK_0		},

	/* tabs */
	{ "tabnew",		CTRL,	1,	GDK_t		},
	{ "999tabnew",		CTRL,	1,	GDK_T		},
	{ "tabclose",		CTRL,	1,	GDK_w		},
	{ "tabundoclose",	0,	0,	GDK_U		},
	{ "tabnext 1",		CTRL,	1,	GDK_1		},
	{ "tabnext 2",		CTRL,	1,	GDK_2		},
	{ "tabnext 3",		CTRL,	1,	GDK_3		},
	{ "tabnext 4",		CTRL,	1,	GDK_4		},
	{ "tabnext 5",		CTRL,	1,	GDK_5		},
	{ "tabnext 6",		CTRL,	1,	GDK_6		},
	{ "tabnext 7",		CTRL,	1,	GDK_7		},
	{ "tabnext 8",		CTRL,	1,	GDK_8		},
	{ "tabnext 9",		CTRL,	1,	GDK_9		},
	{ "tabfirst",		CTRL,	1,	GDK_less	},
	{ "tablast",		CTRL,	1,	GDK_greater	},
	{ "tabprevious",	CTRL,	1,	GDK_Left	},
	{ "tabnext",		CTRL,	1,	GDK_Right	},
	{ "focusout",		CTRL,	1,	GDK_minus	},
	{ "focusin",		CTRL,	1,	GDK_plus	},
	{ "focusin",		CTRL,	1,	GDK_equal	},
	{ "focusreset",		CTRL,	1,	GDK_0		},

	/* command aliases (handy when -S flag is used) */
	{ "promptopen",		0,	1,	GDK_F9		},
	{ "promptopencurrent",	0,	1,	GDK_F10		},
	{ "prompttabnew",	0,	1,	GDK_F11		},
	{ "prompttabnewcurrent",0,	1,	GDK_F12		},
};

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

int
add_ua(struct settings *s, char *value)
{
	struct user_agent *ua;

	ua = g_malloc0(sizeof *ua);
	ua->value = g_strdup(value);

	TAILQ_INSERT_HEAD(&ua_list, ua, entry);

	/* use the last added user agent */
	user_agent = TAILQ_FIRST(&ua_list);
	user_agent_count++;

	return (0);
}


void
walk_ua(struct settings *s,
    void (*cb)(struct settings *, char *, void *), void *cb_args)
{
	struct user_agent		*ua;

	if (s == NULL || cb == NULL) {
		show_oops(NULL, "walk_ua invalid parameters");
		return;
	}

	TAILQ_FOREACH(ua, &ua_list, entry) {
		cb(s, ua->value, cb_args);
	}
}

int
set_auto_load_images(char *value)
{
	struct tab *t;

	auto_load_images = atoi(value);
	TAILQ_FOREACH(t, &tabs, entry) {
		g_object_set(G_OBJECT(t->settings),
		    "auto-load-images", auto_load_images, (char *)NULL);
		webkit_web_view_set_settings(t->wv, t->settings);
	}
	return (0);
}

int
set_enable_autoscroll(char *value)
{
	enable_autoscroll = atoi(value);
	return (0);
}

int
set_enable_favicon_entry(char *value)
{
	enable_favicon_entry = atoi(value);
	return (0);
}

int
set_enable_favicon_tabs(char *value)
{
	enable_favicon_tabs = atoi(value);
	return (0);
}

int
set_external_editor(char *editor)
{
	if (external_editor)
		g_free(external_editor);

	external_editor = g_strdup(editor);

	return (0);
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

char *
get_edit_mode(struct settings *s)
{
	if (edit_mode == XT_EM_HYBRID)
		return (g_strdup("hybrid"));
	else
		return (g_strdup("vi"));
}

int
set_edit_mode(struct settings *s, char *val)
{
	if (!strcmp(val, "hybrid"))
		edit_mode = XT_EM_HYBRID;
	else if (!strcmp(val, "vi"))
		edit_mode = XT_EM_VI;
	else
		return (1);

	return (0);
}

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

		for (i = 0; i < get_settings_size(); i++) {
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

		for (i = 0; i < get_settings_size(); i++) {
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

