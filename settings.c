/*
 * Copyright (c) 2010, 2011 Marco Peereboom <marco@peereboom.us>
 * Copyright (c) 2011 Stevan Andjelkovic <stevan@student.chalmers.se>
 * Copyright (c) 2010, 2011 Edd Barrett <vext01@gmail.com>
 * Copyright (c) 2011 Todd T. Fries <todd@fries.net>
 * Copyright (c) 2011 Raphael Graf <r@undefined.ch>
 * Copyright (c) 2011 Michal Mazurek <akfaew@jasminek.net>
 * Copyright (c) 2012 Josh Rickmar <jrick@devio.us>
 * Copyright (c) 2013 David Hill <dhill@mindcry.org>
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
#include "tooltip.h"

/* globals */
#if SOUP_CHECK_VERSION(2, 42, 2)
GProxyResolver	*proxy_uri = NULL;
gchar		*proxy_exclude[] = { "fc00::/7", "::1", "127.0.0.1", NULL };
#else
SoupURI		*proxy_uri = NULL;
#endif

PangoFontDescription	*cmd_font;
PangoFontDescription	*oops_font;
PangoFontDescription	*statusbar_font;
PangoFontDescription	*tabbar_font;

/* non-settings */
char		search_file[PATH_MAX];
char		command_file[PATH_MAX];
char		runtime_settings[PATH_MAX]; /* override of settings */

/* settings that require restart */
int		enable_socket = 0;
int		single_instance = 0; /* only allow one xombrero to run */
int		fancy_bar = 1;	/* fancy toolbar */
int		browser_mode = XT_BM_NORMAL;
int		gui_mode = XT_GM_CLASSIC;
char		*statusbar_elems = NULL;
char		*tab_elems = NULL;
int		window_height = 768;
int		window_width = 1024;
int		window_maximize = 0;
int		icon_size = 2; /* 1 = smallest, 2+ = bigger */
char		*resource_dir = NULL;
char		download_dir[PATH_MAX];
int		allow_volatile_cookies = 0;
int		save_global_history = 0; /* save global history to disk */
int		save_rejected_cookies = 0;
gint		max_connections = 25;
gint		max_host_connections = 5;
int		history_autosave = 0;
int		edit_mode = XT_EM_HYBRID;
char		*include_config = NULL;
int		anonymize_headers = 0;
int		tabless = 0;	/* allow only 1 tab */

/* runtime settings */
int		show_tabs = XT_DS_SHOW_TABS;	/* show tabs on notebook */
int		tab_style = XT_DS_TAB_STYLE; /* tab bar style */
int		statusbar_style = XT_DS_STATUSBAR_STYLE; /* status bar style */
int		show_url = XT_DS_SHOW_URL;	/* show url toolbar on notebook */
int		show_scrollbars = XT_DS_SHOW_SCROLLBARS;
int		show_statusbar = XT_DS_SHOW_STATUSBAR; /* vimperator style status bar */
int		ctrl_click_focus = XT_DS_CTRL_CLICK_FOCUS; /* ctrl click gets focus */
int		cookies_enabled = XT_DS_COOKIES_ENABLED; /* enable cookies */
int		read_only_cookies = XT_DS_READ_ONLY_COOKIES; /* enable to not write cookies */
int		enable_scripts = XT_DS_ENABLE_SCRIPTS;
int		enable_plugins = XT_DS_ENABLE_PLUGINS;
double		default_zoom_level = XT_DS_DEFAULT_ZOOM_LEVEL;
char		default_script[PATH_MAX];	/* special setting - is never g_free'd */
int		refresh_interval = XT_DS_REFRESH_INTERVAL; /* download refresh interval */
int		enable_plugin_whitelist = XT_DS_ENABLE_PLUGIN_WHITELIST;
int		enable_cookie_whitelist = XT_DS_ENABLE_COOKIE_WHITELIST;
int		enable_js_whitelist = XT_DS_ENABLE_JS_WHITELIST;
int		enable_localstorage = XT_DS_ENABLE_LOCALSTORAGE;
int		session_timeout = XT_DS_SESSION_TIMEOUT; /* cookie session timeout */
int		cookie_policy = XT_DS_COOKIE_POLICY;
char		ssl_ca_file[PATH_MAX];		/* special setting - is never g_free'd */
gboolean	ssl_strict_certs = XT_DS_SSL_STRICT_CERTS;
gboolean	enable_strict_transport = XT_DS_ENABLE_STRICT_TRANSPORT;
int		append_next = XT_DS_APPEND_NEXT; /* append tab after current tab */
char		*home = NULL;			/* allocated/set at startup */
char		*search_string = NULL;		/* allocated/set at startup */
char		*http_proxy = NULL;
int		http_proxy_starts_enabled = 1;
int		download_mode = XT_DM_START;
int		color_visited_uris = XT_DS_COLOR_VISITED_URIS;
int		session_autosave = XT_DS_SESSION_AUTOSAVE;
int		guess_search = XT_DS_GUESS_SEARCH;
gint		enable_spell_checking = XT_DS_ENABLE_SPELL_CHECKING;
char		*spell_check_languages = NULL;	/* allocated/set at startup */
char		*url_regex = NULL;		/* allocated/set at startup */
char		*encoding = NULL;		/* allocated/set at startup */
int		autofocus_onload = XT_DS_AUTOFOCUS_ONLOAD;
int		enable_js_autorun = XT_DS_ENABLE_JS_AUTORUN;
char		*userstyle = NULL;		/* allocated/set at startup */
int		userstyle_global = XT_DS_USERSTYLE_GLOBAL;
int		auto_load_images = XT_DS_AUTO_LOAD_IMAGES;
int		enable_autoscroll = XT_DS_ENABLE_AUTOSCROLL;
int		enable_cache = 0;
int		enable_favicon_entry = XT_DS_ENABLE_FAVICON_ENTRY;
int		enable_favicon_tabs = XT_DS_ENABLE_FAVICON_TABS;
char		*external_editor = NULL;
int		referer_mode = XT_DS_REFERER_MODE;
char		*referer_custom = NULL;
int		download_notifications = XT_DS_DOWNLOAD_NOTIFICATIONS;
int		warn_cert_changes = 0;
int		allow_insecure_content = XT_DS_ALLOW_INSECURE_CONTENT;
int		allow_insecure_scripts = XT_DS_ALLOW_INSECURE_SCRIPTS;
int		do_not_track = XT_DS_DO_NOT_TRACK;
int		preload_strict_transport = XT_DS_PRELOAD_STRICT_TRANSPORT;
char		*gnutls_priority_string = XT_DS_GNUTLS_PRIORITY_STRING;
int		js_auto_open_windows = XT_DS_JS_AUTO_OPEN_WINDOWS;
char		*cmd_font_name = NULL;	/* these are all set at startup */
char		*oops_font_name = NULL;
char		*statusbar_font_name = NULL;
char		*tabbar_font_name = NULL;

char		*get_download_dir(struct settings *);
char		*get_default_script(struct settings *);
char		*get_runtime_dir(struct settings *);
char		*get_tab_style(struct settings *);
char		*get_statusbar_style(struct settings *);
char		*get_edit_mode(struct settings *);
char		*get_download_mode(struct settings *);
char		*get_work_dir(struct settings *);
char		*get_referer(struct settings *);
char		*get_ssl_ca_file(struct settings *);
char		*get_userstyle(struct settings *);
char		*get_gnutls_priority_string(struct settings *);

int		add_cookie_wl(struct settings *, char *);
int		add_js_wl(struct settings *, char *);
int		add_pl_wl(struct settings *, char *);
int		add_mime_type(struct settings *, char *);
int		add_alias(struct settings *, char *);
int		add_kb(struct settings *, char *);
int		add_ua(struct settings *, char *);
int		add_http_accept(struct settings *, char *);
int		add_cmd_alias(struct settings *, char *);
int		add_custom_uri(struct settings *, char *);
int		add_force_https(struct settings *, char *);

int		set_append_next(char *);
int		set_autofocus_onload(char *);
int		set_cmd_font(char *);
int		set_color_visited_uris(char *);
int		set_cookie_policy_rt(char *);
int		set_cookies_enabled(char *);
int		set_ctrl_click_focus(char *);
int		set_fancy_bar(char *);
int		set_home(char *);
int		set_download_dir(struct settings *, char *);
int		set_download_notifications(char *);
int		set_default_script(struct settings *, char *);
int		set_default_script_rt(char *);
int		set_default_zoom_level(char *);
int		set_enable_cookie_whitelist(char *);
int		set_enable_js_autorun(char *);
int		set_enable_js_whitelist(char *);
int		set_enable_localstorage(char *);
int		set_enable_plugins(char *);
int		set_enable_plugin_whitelist(char *);
int		set_enable_scripts(char *);
int		set_enable_spell_checking(char *);
int		set_enable_strict_transport(char *);
int		set_encoding_rt(char *);
int		set_runtime_dir(struct settings *, char *);
int		set_tabbar_font(char *value);
int		set_tab_style(struct settings *, char *);
int		set_tab_style_rt(char *);
int		set_statusbar_style(struct settings *, char *);
int		set_statusbar_style_rt(char *);
int		set_edit_mode(struct settings *, char *);
int		set_work_dir(struct settings *, char *);
int		set_auto_load_images(char *);
int		set_enable_autoscroll(char *);
int		set_enable_cache(char *);
int		set_enable_favicon_entry(char *);
int		set_enable_favicon_tabs(char *);
int		set_guess_search(char *);
int		set_download_mode(struct settings *, char *);
int		set_download_mode_rt(char *);
int		set_js_auto_open_windows(char *);
int		set_oops_font(char *);
int		set_read_only_cookies(char *);
int		set_referer(struct settings *, char *);
int		set_referer_rt(char *);
int		set_refresh_interval(char *);
int		set_search_string(char *s);
int		set_session_autosave(char *);
int		set_session_timeout(char *);
int		set_show_scrollbars(char *);
int		set_show_statusbar(char *);
int		set_show_tabs(char *);
int		set_show_url(char *);
int		set_spell_check_languages(char *);
int		set_ssl_ca_file_rt(char *);
int		set_ssl_strict_certs(char *);
int		set_statusbar_font(char *);
int		set_url_regex(char *);
int		set_userstyle(struct settings *, char *);
int		set_userstyle_rt(char *);
int		set_userstyle_global(char *);
int		set_external_editor(char *);
int		set_warn_cert_changes(char *);
int		set_allow_insecure_content(char *);
int		set_allow_insecure_scripts(char *);
int		set_http_proxy(char *);
int		set_do_not_track(char *);
int		set_gnutls_priority_string(struct settings *, char *);

int		check_allow_insecure_content(char **);
int		check_allow_insecure_scripts(char **);
int		check_allow_volatile_cookies(char **);
int		check_anonymize_headers(char **);
int		check_append_next(char **);
int		check_auto_load_images(char **);
int		check_autofocus_onload(char **);
int		check_browser_mode(char **);
int		check_cmd_font(char **);
int		check_color_visited_uris(char **);
int		check_cookie_policy(char **);
int		check_cookies_enabled(char **);
int		check_ctrl_click_focus(char **);
int		check_default_script(char **);
int		check_default_zoom_level(char **);
int		check_download_dir(char **);
int		check_download_mode(char **);
int		check_download_notifications(char **);
int		check_edit_mode(char **);
int		check_enable_autoscroll(char **);
int		check_enable_cache(char **);
int		check_enable_cookie_whitelist(char **);
int		check_enable_favicon_entry(char **);
int		check_enable_favicon_tabs(char **);
int		check_enable_js_autorun(char **);
int		check_enable_js_whitelist(char **);
int		check_enable_localstorage(char **);
int		check_enable_plugin_whitelist(char **);
int		check_enable_plugins(char **);
int		check_enable_scripts(char **);
int		check_enable_socket(char **);
int		check_enable_spell_checking(char **);
int		check_enable_strict_transport(char **);
int		check_encoding(char **);
int		check_external_editor(char **);
int		check_fancy_bar(char **);
int		check_gnutls_search_string(char **);
int		check_guess_search(char **);
int		check_gui_mode(char **);
int		check_history_autosave(char **);
int		check_home(char **);
int		check_http_proxy(char **);
int		check_http_proxy_scheme(const char *);
int		check_http_proxy_starts_enabled(char **);
int		check_icon_size(char **);
int		check_js_auto_open_windows(char **);
int		check_max_connections(char **);
int		check_max_host_connections(char **);
int		check_oops_font(char **);
int		check_read_only_cookies(char **);
int		check_referer(char **);
int		check_refresh_interval(char **);
int		check_resource_dir(char **);
int		check_save_global_history(char **);
int		check_save_rejected_cookies(char **);
int		check_search_string(char **);
int		check_session_autosave(char **);
int		check_session_timeout(char **);
int		check_show_scrollbars(char **);
int		check_show_statusbar(char **);
int		check_show_tabs(char **);
int		check_show_url(char **);
int		check_single_instance(char **);
int		check_spell_check_languages(char **);
int		check_ssl_ca_file(char **);
int		check_ssl_strict_certs(char **);
int		check_statusbar_elems(char **);
int		check_tab_elems(char **);
int		check_statusbar_font(char **);
int		check_statusbar_style(char **);
int		check_tab_style(char **);
int		check_tabless(char **);
int		check_tabbar_font(char **);
int		check_url_regex(char **);
int		check_userstyle(char **);
int		check_userstyle_global(char **);
int		check_warn_cert_changes(char **);
int		check_window_height(char **);
int		check_window_maximize(char **);
int		check_window_width(char **);
int		check_work_dir(char **);
int		check_do_not_track(char **);

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
void		walk_http_accept(struct settings *, void (*)(struct settings *,
		    char *, void *), void *);
void		walk_cmd_alias(struct settings *, void (*)(struct settings *,
		    char *, void *), void *);
void		walk_custom_uri(struct settings *, void (*)(struct settings *,
		    char *, void *), void *);
void		walk_force_https(struct settings *, void (*)(struct settings *,
		    char *, void *), void *);

struct special		s_browser_mode = {
	set_browser_mode,
	get_browser_mode,
	NULL,
	{ "kiosk", "normal", "whitelist", NULL }
};

struct special		s_gui_mode = {
	set_gui_mode,
	get_gui_mode,
	NULL,
	{ "classic", "minimal", NULL }
};

struct special		s_cookie = {
	set_cookie_policy,
	get_cookie_policy,
	NULL,
	{ "accept", "no3rdparty", "reject", NULL }
};

struct special		s_alias = {
	add_alias,
	NULL,
	walk_alias,
	{ NULL }
};

struct special		s_cmd_alias = {
	add_cmd_alias,
	NULL,
	walk_cmd_alias,
	{ NULL }
};

struct special		s_mime = {
	add_mime_type,
	NULL,
	walk_mime_type,
	{ NULL }
};

struct special		s_js = {
	add_js_wl,
	NULL,
	walk_js_wl,
	{ NULL }
};

struct special		s_pl = {
	add_pl_wl,
	NULL,
	walk_pl_wl,
	{ NULL }
};

struct special		s_kb = {
	add_kb,
	NULL,
	walk_kb,
	{ NULL }
};

struct special		s_cookie_wl = {
	add_cookie_wl,
	NULL,
	walk_cookie_wl,
	{ NULL }
};

struct special		s_uri = {
	add_custom_uri,
	NULL,
	walk_custom_uri,
	{ NULL }
};

struct special		s_default_script = {
	set_default_script,
	get_default_script,
	NULL,
	{ NULL }
};

struct special		s_ssl_ca_file = {
	set_ssl_ca_file,
	get_ssl_ca_file,
	NULL,
	{ NULL }
};

struct special		s_download_dir = {
	set_download_dir,
	get_download_dir,
	NULL,
	{ NULL }
};

struct special		s_work_dir = {
	set_work_dir,
	get_work_dir,
	NULL,
	{ NULL }
};

struct special		s_tab_style = {
	set_tab_style,
	get_tab_style,
	NULL,
	{ "compact", "normal", NULL }
};

struct special		s_statusbar_style = {
	set_statusbar_style,
	get_statusbar_style,
	NULL,
	{ "title", "url", NULL }
};

struct special		s_edit_mode = {
	set_edit_mode,
	get_edit_mode,
	NULL,
	{ "hybrid", "vi", NULL }
};

struct special		s_download_mode = {
	set_download_mode,
	get_download_mode,
	NULL,
	{ "add", "ask", "start", NULL }
};

struct special		s_ua = {
	add_ua,
	NULL,
	walk_ua,
	{ NULL }
};

struct special		s_http_accept = {
	add_http_accept,
	NULL,
	walk_http_accept,
	{ NULL }
};

struct special		s_referer = {
	set_referer,
	get_referer,
	NULL,
	{ "always", "never", "same-domain", "same-fqdn", NULL }
};

struct special		s_userstyle = {
	set_userstyle,
	get_userstyle,
	NULL,
	{ NULL }
};

struct special		s_force_https = {
	add_force_https,
	NULL,
	walk_force_https,
	{ NULL }
};

struct special		s_gnutls_priority_string = {
	set_gnutls_priority_string,
	get_gnutls_priority_string,
	NULL,
	{ NULL }
};

struct settings		rs[] = {
	{ "allow_insecure_content",	XT_S_BOOL, 0,		&allow_insecure_content, NULL, NULL, NULL, set_allow_insecure_content, check_allow_insecure_content, TT_ALLOW_INSECURE_CONTENT },
	{ "allow_insecure_scripts",	XT_S_BOOL, 0,		&allow_insecure_scripts, NULL, NULL, NULL, set_allow_insecure_scripts, check_allow_insecure_scripts, TT_ALLOW_INSECURE_SCRIPTS},
	{ "allow_volatile_cookies",	XT_S_BOOL, 0,		&allow_volatile_cookies, NULL, NULL, NULL, NULL, check_allow_volatile_cookies, TT_ALLOW_VOLATILE_COOKIES },
	{ "anonymize_headers",		XT_S_BOOL, 0,		&anonymize_headers, NULL, NULL, NULL, NULL, check_anonymize_headers, TT_ANONYMIZE_HEADERS },
	{ "append_next",		XT_S_BOOL, 0,		&append_next, NULL, NULL, NULL, set_append_next, check_append_next, TT_APPEND_NEXT },
	{ "auto_load_images",		XT_S_BOOL, 0,		&auto_load_images, NULL, NULL, NULL, set_auto_load_images, check_auto_load_images, TT_AUTO_LOAD_IMAGES },
	{ "autofocus_onload",		XT_S_BOOL, 0,		&autofocus_onload, NULL, NULL, NULL, set_autofocus_onload, check_autofocus_onload, TT_AUTOFOCUS_ONLOAD },
	{ "browser_mode",		XT_S_STR, 0, NULL, NULL,&s_browser_mode, NULL, NULL, check_browser_mode, TT_BROWSER_MODE },
	{ "cmd_font",			XT_S_STR, 0, NULL, &cmd_font_name, NULL, NULL, set_cmd_font, check_cmd_font, TT_CMD_FONT },
	{ "color_visited_uris",		XT_S_BOOL, 0,		&color_visited_uris , NULL, NULL, NULL, set_color_visited_uris, check_color_visited_uris, TT_COLOR_VISITED_URIS },
	{ "cookie_policy",		XT_S_STR, 0, NULL, NULL,&s_cookie, NULL, set_cookie_policy_rt, check_cookie_policy, TT_COOKIE_POLICY },
	{ "cookies_enabled",		XT_S_BOOL, 0,		&cookies_enabled, NULL, NULL, NULL, set_cookies_enabled, check_cookies_enabled, TT_COOKIES_ENABLED },
	{ "ctrl_click_focus",		XT_S_BOOL, 0,		&ctrl_click_focus, NULL, NULL, NULL, set_ctrl_click_focus, check_ctrl_click_focus, TT_CTRL_CLICK_FOCUS },
	{ "default_script",		XT_S_STR, 1, NULL, NULL,&s_default_script, NULL, set_default_script_rt, check_default_script, TT_DEFAULT_SCRIPT },
	{ "default_zoom_level",		XT_S_DOUBLE, 0,		NULL, NULL, NULL, &default_zoom_level, set_default_zoom_level, check_default_zoom_level, TT_DEFAULT_ZOOM_LEVEL },
	{ "do_not_track",		XT_S_BOOL, 0,		&do_not_track, NULL, NULL, NULL, set_do_not_track, check_do_not_track, TT_DO_NOT_TRACK },
	{ "download_dir",		XT_S_STR, 0, NULL, NULL,&s_download_dir, NULL, NULL, check_download_dir, TT_DOWNLOAD_DIR },
	{ "download_mode",		XT_S_STR, 0, NULL, NULL,&s_download_mode, NULL, set_download_mode_rt, check_download_mode, TT_DOWNLOAD_MODE },
	{ "download_notifications",	XT_S_BOOL, 0,		&download_notifications, NULL, NULL, NULL, set_download_notifications, check_download_notifications, TT_DOWNLOAD_NOTIFICATIONS },
	{ "edit_mode",			XT_S_STR, 0, NULL, NULL,&s_edit_mode, NULL, NULL, check_edit_mode, NULL },
	{ "enable_autoscroll",		XT_S_BOOL, 0,		&enable_autoscroll, NULL, NULL, NULL, set_enable_autoscroll, check_enable_autoscroll, TT_ENABLE_AUTOSCROLL },
	{ "enable_cache",		XT_S_BOOL, 0,		&enable_cache, NULL, NULL, NULL, set_enable_cache, check_enable_cache, TT_ENABLE_CACHE },
	{ "enable_cookie_whitelist",	XT_S_BOOL, 0,		&enable_cookie_whitelist, NULL, NULL, NULL, set_enable_cookie_whitelist, check_enable_cookie_whitelist, TT_ENABLE_COOKIE_WHITELIST },
	{ "enable_favicon_entry",	XT_S_BOOL, 0,		&enable_favicon_entry, NULL, NULL, NULL, set_enable_favicon_entry, check_enable_favicon_entry, TT_ENABLE_FAVICON_ENTRY },
	{ "enable_favicon_tabs",	XT_S_BOOL, 0,		&enable_favicon_tabs, NULL, NULL, NULL, set_enable_favicon_tabs, check_enable_favicon_tabs, TT_ENABLE_FAVICON_TABS },
	{ "enable_js_autorun",		XT_S_BOOL, 0,		&enable_js_autorun, NULL, NULL, NULL, set_enable_js_autorun, check_enable_js_autorun, TT_ENABLE_JS_AUTORUN },
	{ "enable_js_whitelist",	XT_S_BOOL, 0,		&enable_js_whitelist, NULL, NULL, NULL, set_enable_js_whitelist, check_enable_js_whitelist, TT_ENABLE_JS_WHITELIST },
	{ "enable_localstorage",	XT_S_BOOL, 0,		&enable_localstorage, NULL, NULL, NULL, set_enable_localstorage, check_enable_localstorage, TT_ENABLE_LOCALSTORAGE },
	{ "enable_plugin_whitelist",	XT_S_BOOL, 0,		&enable_plugin_whitelist, NULL, NULL, NULL, set_enable_plugin_whitelist, check_enable_plugin_whitelist, TT_ENABLE_PLUGIN_WHITELIST },
	{ "enable_plugins",		XT_S_BOOL, 0,		&enable_plugins, NULL, NULL, NULL, set_enable_plugins, check_enable_plugins, TT_ENABLE_PLUGINS },
	{ "enable_scripts",		XT_S_BOOL, 0,		&enable_scripts, NULL, NULL, NULL, set_enable_scripts, check_enable_scripts, TT_ENABLE_SCRIPTS },
	{ "enable_socket",		XT_S_BOOL,XT_SF_RESTART,&enable_socket, NULL, NULL, NULL, NULL, check_enable_socket, TT_ENABLE_SOCKET },
	{ "enable_spell_checking",	XT_S_BOOL, 0,		&enable_spell_checking, NULL, NULL, NULL, set_enable_spell_checking, check_enable_spell_checking, TT_ENABLE_SPELL_CHECKING },
	{ "enable_strict_transport",	XT_S_BOOL, 0,		&enable_strict_transport, NULL, NULL, NULL, set_enable_strict_transport, check_enable_strict_transport, TT_ENABLE_STRICT_TRANSPORT },
	{ "encoding",			XT_S_STR, 0, NULL,	&encoding, NULL, NULL, NULL, check_encoding, TT_ENCODING },
	{ "external_editor",		XT_S_STR,0, NULL,	&external_editor, NULL, NULL, set_external_editor, check_external_editor, TT_EXTERNAL_EDITOR },
	{ "fancy_bar",			XT_S_BOOL,XT_SF_RESTART,&fancy_bar, NULL, NULL, NULL, set_fancy_bar, check_fancy_bar, TT_FANCY_BAR },
	{ "gnutls_priority_string",	XT_S_STR, 0, NULL, NULL,&s_gnutls_priority_string, NULL, NULL, check_gnutls_search_string, TT_GNUTLS_PRIORITY_STRING },
	{ "guess_search",		XT_S_BOOL, 0,		&guess_search, NULL, NULL, NULL, set_guess_search, check_guess_search, TT_GUESS_SEARCH },
	{ "gui_mode",			XT_S_STR, 0, NULL, NULL,&s_gui_mode, NULL, NULL, check_gui_mode, TT_GUI_MODE },
	{ "history_autosave",		XT_S_BOOL, 0,		&history_autosave, NULL, NULL, NULL, NULL, check_history_autosave, TT_HISTORY_AUTOSAVE },
	{ "home",			XT_S_STR, 0, NULL,	&home, NULL, NULL, set_home, check_home, TT_HOME },
	{ "http_proxy",			XT_S_STR, 0, NULL,	&http_proxy, NULL, NULL, set_http_proxy, check_http_proxy, TT_HTTP_PROXY },
	{ "http_proxy_starts_enabled",	XT_S_BOOL, 0,		&http_proxy_starts_enabled, NULL, NULL, NULL, NULL, check_http_proxy_starts_enabled, TT_HTTP_PROXY_STARTS_ENABLED },
	{ "icon_size",			XT_S_INT, 0,		&icon_size, NULL, NULL, NULL, NULL, check_icon_size, TT_ICON_SIZE },
	{ "include_config",		XT_S_STR, XT_SF_INVISIBLE, NULL, &include_config, NULL, NULL, NULL, NULL },
	{ "js_auto_open_windows",	XT_S_BOOL, 1,		&js_auto_open_windows, NULL, NULL, NULL, set_js_auto_open_windows, check_js_auto_open_windows, TT_JS_AUTO_OPEN_WINDOWS },
	{ "max_connections",		XT_S_INT, XT_SF_RESTART,&max_connections, NULL, NULL, NULL, NULL, check_max_connections, TT_MAX_CONNECTIONS },
	{ "max_host_connections",	XT_S_INT, XT_SF_RESTART,&max_host_connections, NULL, NULL, NULL, NULL, check_max_host_connections, TT_MAX_HOST_CONNECTIONS },
	{ "oops_font",			XT_S_STR, 0, NULL, &oops_font_name, NULL, NULL, set_oops_font, check_oops_font, TT_OOPS_FONT },
	{ "preload_strict_transport",	XT_S_BOOL, 0,		&preload_strict_transport, NULL, NULL, NULL, NULL, NULL, TT_PRELOAD_STRICT_TRANSPORT },
	{ "read_only_cookies",		XT_S_BOOL, 0,		&read_only_cookies, NULL, NULL, NULL, NULL, check_read_only_cookies, TT_READ_ONLY_COOKIES },
	{ "referer",			XT_S_STR, 0, NULL, NULL,&s_referer, NULL, set_referer_rt, check_referer, TT_REFERER },
	{ "refresh_interval",		XT_S_INT, 0,		&refresh_interval, NULL, NULL, NULL, set_refresh_interval, check_refresh_interval, TT_REFRESH_INTERVAL },
	{ "resource_dir",		XT_S_STR, 0, NULL,	&resource_dir, NULL, NULL, NULL, check_resource_dir, TT_RESOURCE_DIR },
	{ "save_global_history",	XT_S_BOOL,XT_SF_RESTART,&save_global_history, NULL, NULL, NULL, NULL, check_save_global_history, TT_SAVE_GLOBAL_HISTORY },
	{ "save_rejected_cookies",	XT_S_BOOL,XT_SF_RESTART,&save_rejected_cookies, NULL, NULL, NULL, NULL, check_save_rejected_cookies, TT_SAVE_REJECTED_COOKIES },
	{ "search_string",		XT_S_STR, 0, NULL,	&search_string, NULL, NULL, set_search_string, check_search_string, TT_SEARCH_STRING },
	{ "session_autosave",		XT_S_BOOL, 0,		&session_autosave, NULL, NULL, NULL, set_session_autosave, check_session_autosave, TT_SESSION_AUTOSAVE },
	{ "session_timeout",		XT_S_INT, 0,		&session_timeout, NULL, NULL, NULL, set_session_timeout, check_session_timeout, TT_SESSION_TIMEOUT },
	{ "show_scrollbars",		XT_S_BOOL, 0,		&show_scrollbars, NULL, NULL, NULL, set_show_scrollbars, check_show_scrollbars, TT_SHOW_SCROLLBARS },
	{ "show_statusbar",		XT_S_BOOL, 0,		&show_statusbar, NULL, NULL, NULL, set_show_statusbar, check_show_statusbar, TT_SHOW_STATUSBAR },
	{ "show_tabs",			XT_S_BOOL, 0,		&show_tabs, NULL, NULL, NULL, set_show_tabs, check_show_tabs, TT_SHOW_TABS },
	{ "show_url",			XT_S_BOOL, 0,		&show_url, NULL, NULL, NULL, set_show_url, check_show_url, TT_SHOW_URL },
	{ "single_instance",		XT_S_BOOL,XT_SF_RESTART,&single_instance, NULL, NULL, NULL, NULL, check_single_instance, TT_SINGLE_INSTANCE },
	{ "spell_check_languages",	XT_S_STR, 0, NULL,	&spell_check_languages, NULL, NULL, set_spell_check_languages, check_spell_check_languages, TT_SPELL_CHECK_LANGUAGES },
	{ "ssl_ca_file",		XT_S_STR, 0, NULL, NULL,&s_ssl_ca_file, NULL, set_ssl_ca_file_rt, check_ssl_ca_file, TT_SSL_CA_FILE },
	{ "ssl_strict_certs",		XT_S_BOOL, 0,		&ssl_strict_certs, NULL, NULL, NULL, set_ssl_strict_certs, check_ssl_strict_certs, TT_SSL_STRICT_CERTS },
	{ "statusbar_elems",		XT_S_STR, 0, NULL,	&statusbar_elems, NULL, NULL, NULL, check_statusbar_elems, TT_STATUSBAR_ELEMS },
	{ "tab_elems",			XT_S_STR, 0, NULL,	&tab_elems, NULL, NULL, NULL, check_tab_elems, TT_TAB_ELEMS },
	{ "statusbar_font",		XT_S_STR, 0, NULL, &statusbar_font_name, NULL, NULL, set_statusbar_font, check_statusbar_font, TT_STATUSBAR_FONT },
	{ "statusbar_style",		XT_S_STR, 0, NULL, NULL,&s_statusbar_style, NULL, set_statusbar_style_rt, check_statusbar_style, TT_STATUSBAR_STYLE },
	{ "tab_style",			XT_S_STR, 0, NULL, NULL,&s_tab_style, NULL, set_tab_style_rt, check_tab_style, TT_TAB_STYLE },
	{ "tabbar_font",		XT_S_STR, 0, NULL, &tabbar_font_name, NULL, NULL, set_tabbar_font, check_tabbar_font, TT_TABBAR_FONT },
	{ "tabless",			XT_S_BOOL, 0,		&tabless, NULL, NULL, NULL, NULL, check_tabless, TT_TABLESS },
	{ "url_regex",			XT_S_STR, 0, NULL,	&url_regex, NULL, NULL, set_url_regex, check_url_regex, TT_URL_REGEX },
	{ "userstyle",			XT_S_STR, 0, NULL, NULL,&s_userstyle, NULL, set_userstyle_rt, check_userstyle, TT_USERSTYLE },
	{ "userstyle_global",		XT_S_BOOL, 0,		&userstyle_global, NULL, NULL, NULL, set_userstyle_global, check_userstyle_global, TT_USERSTYLE_GLOBAL },
	{ "warn_cert_changes",		XT_S_BOOL, 0,		&warn_cert_changes, NULL, NULL, NULL, set_warn_cert_changes, check_warn_cert_changes, TT_WARN_CERT_CHANGES },
	{ "window_height",		XT_S_INT, 0,		&window_height, NULL, NULL, NULL, NULL, check_window_height, TT_WINDOW_HEIGHT },
	{ "window_maximize",		XT_S_BOOL, 0,		&window_maximize, NULL, NULL, NULL, NULL, check_window_maximize, TT_WINDOW_MAXIMIZE },
	{ "window_width",		XT_S_INT, 0,		&window_width, NULL, NULL, NULL, NULL, check_window_width, TT_WINDOW_WIDTH },
	{ "work_dir",			XT_S_STR, 0, NULL, NULL,&s_work_dir, NULL, NULL, check_work_dir, TT_WORK_DIR },

	/* special settings */
	{ "alias",			XT_S_STR, XT_SF_RUNTIME, NULL, NULL, &s_alias, NULL, NULL },
	{ "cmd_alias",			XT_S_STR, XT_SF_RUNTIME, NULL, NULL, &s_cmd_alias, NULL, NULL },
	{ "cookie_wl",			XT_S_STR, XT_SF_RUNTIME, NULL, NULL, &s_cookie_wl, NULL, NULL },
	{ "custom_uri",			XT_S_STR, XT_SF_RUNTIME, NULL, NULL, &s_uri, NULL, NULL },
	{ "force_https",		XT_S_STR, XT_SF_RUNTIME, NULL, NULL, &s_force_https, NULL, NULL },
	{ "http_accept",		XT_S_STR, XT_SF_RUNTIME, NULL, NULL, &s_http_accept, NULL, NULL },
	{ "js_wl",			XT_S_STR, XT_SF_RUNTIME, NULL, NULL, &s_js, NULL, NULL },
	{ "keybinding",			XT_S_STR, XT_SF_RUNTIME, NULL, NULL, &s_kb, NULL, NULL },
	{ "mime_type",			XT_S_STR, XT_SF_RUNTIME, NULL, NULL, &s_mime, NULL, NULL },
	{ "pl_wl",			XT_S_STR, XT_SF_RUNTIME, NULL, NULL, &s_pl, NULL, NULL },
	{ "user_agent",			XT_S_STR, XT_SF_RUNTIME, NULL, NULL, &s_ua, NULL, NULL },
};

int
set_http_proxy(char *proxy)
{
	char *tmpproxy = proxy;

	/* see if we need to clear it */
	if (proxy == NULL || strlen(proxy) == 0)
		tmpproxy = NULL;

	if (check_http_proxy_scheme(proxy) == 0)
		tmpproxy = NULL;

	return (setup_proxy(tmpproxy));
}

int
check_http_proxy(char **tt)
{
	*tt = g_strdup("Default: (empty)");
	return (g_strcmp0(http_proxy, NULL));
}

int
check_http_proxy_scheme(const char *uri)
{
	int rv = 0;
	char *scheme;

	if (!uri)
		return (0);

	scheme = g_uri_parse_scheme(uri);
	if (!scheme)
		return (0);

#if SOUP_CHECK_VERSION(2, 42, 2)
	if (strcmp(scheme, "socks5") == 0 || strcmp(scheme, "socks4a") == 0 ||
	    strcmp(scheme, "socks4") == 0 || strcmp(scheme, "socks") == 0 ||
	    strcmp(scheme, "http") == 0) {
		rv = 1;
	}
#else
	if (strcmp(scheme, "http") == 0) {
		rv = 1;
	}
#endif
	free(scheme);
	return (rv);
}

int
check_http_proxy_starts_enabled(char **tt)
{
	*tt = g_strdup("Default: Enabled");
	return (http_proxy_starts_enabled != 1);
}

int
check_icon_size(char **tt)
{
	*tt = g_strdup_printf("Default: %d", 2);
	return (icon_size != 2);
}

int
check_max_connections(char **tt)
{
	*tt = g_strdup_printf("Default: %d", 25);
	return (max_connections != 25);
}

int
check_max_host_connections(char **tt)
{
	*tt = g_strdup_printf("Default: %d", 5);
	return (max_host_connections != 5);
}

int
set_default_zoom_level(char *value)
{
	struct karg		args = {0};
	struct tab		*t;

	if (value == NULL || strlen(value) == 0)
		default_zoom_level = XT_DS_DEFAULT_ZOOM_LEVEL;
	else
		default_zoom_level = g_strtod(value, NULL);
	args.i = 100;	/* adjust = 100 percent for no additional adjustment */
	TAILQ_FOREACH(t, &tabs, entry)
		resizetab(t, &args);
	return (0);
}

int
check_default_zoom_level(char **tt)
{
	*tt = g_strdup_printf("Default: %f", XT_DS_DEFAULT_ZOOM_LEVEL);
	return (default_zoom_level < (XT_DS_DEFAULT_ZOOM_LEVEL - 0.0001) ||
		default_zoom_level > (XT_DS_DEFAULT_ZOOM_LEVEL + 0.0001));
}

int
set_cookies_enabled(char *value)
{
	int			tmp;
	const char		*errstr;

	if (value == NULL || strlen(value) == 0)
		cookies_enabled = XT_DS_COOKIES_ENABLED;
	else {
		tmp = strtonum(value, 0, 1, &errstr);
		if (errstr)
			return (-1);
		cookies_enabled = tmp;
	}
	return (0);
}

int
check_cookies_enabled(char **tt)
{
	*tt = g_strdup_printf("Default: %s",
	    XT_DS_COOKIES_ENABLED ? "Enabled" : "Disabled");
	return (cookies_enabled != XT_DS_COOKIES_ENABLED);
}

int
set_append_next(char *value)
{
	int			tmp;
	const char		*errstr;

	if (value == NULL || strlen(value) == 0)
		append_next = XT_DS_APPEND_NEXT;
	else {
		tmp = strtonum(value, 0, 1, &errstr);
		if (errstr)
			return (-1);
		append_next = tmp;
	}
	return (0);
}

int
check_append_next(char **tt)
{
	*tt = g_strdup_printf("Default: %s",
	    XT_DS_APPEND_NEXT ? "Enabled" : "Disabled");
	return (append_next != XT_DS_APPEND_NEXT);
}

int
set_cmd_font(char *value)
{
	struct tab		*t;

	if (cmd_font_name)
		g_free(cmd_font_name);
	if (cmd_font)
		pango_font_description_free(cmd_font);
	if (value == NULL || strlen(value) == 0)
		cmd_font_name = g_strdup(XT_DS_CMD_FONT_NAME);
	else
		cmd_font_name = g_strdup(value);
	cmd_font = pango_font_description_from_string(cmd_font_name);
	TAILQ_FOREACH(t, &tabs, entry)
		modify_font(GTK_WIDGET(t->cmd), cmd_font);
	return (0);
}

int
check_cmd_font(char **tt)
{
	*tt = g_strdup_printf("Default: %s", XT_DS_CMD_FONT_NAME);
	return (g_strcmp0(cmd_font_name, XT_DS_CMD_FONT_NAME));
}

int
set_oops_font(char *value)
{
	struct tab		*t;

	if (oops_font_name)
		g_free(oops_font_name);
	if (oops_font)
		pango_font_description_free(oops_font);
	if (value == NULL || strlen(value) == 0)
		cmd_font_name = g_strdup(XT_DS_OOPS_FONT_NAME);
	else
		oops_font_name = g_strdup(value);
	oops_font = pango_font_description_from_string(oops_font_name);
	TAILQ_FOREACH(t, &tabs, entry)
		modify_font(GTK_WIDGET(t->oops), oops_font);
	return (0);
}

int
check_oops_font(char **tt)
{
	*tt = g_strdup_printf("Default: %s", XT_DS_OOPS_FONT_NAME);
	return (g_strcmp0(oops_font_name, XT_DS_OOPS_FONT_NAME));
}

int
check_read_only_cookies(char **tt)
{
	*tt = g_strdup_printf("Default: %s",
	    XT_DS_READ_ONLY_COOKIES ? "Enabled" : "Disabled");
	return (read_only_cookies != XT_DS_READ_ONLY_COOKIES);
}

int
set_statusbar_font(char *value)
{
	struct tab		*t;

	if (statusbar_font_name)
		g_free(statusbar_font_name);
	if (statusbar_font)
		pango_font_description_free(statusbar_font);
	if (value == NULL || strlen(value) == 0)
		statusbar_font_name = g_strdup(XT_DS_STATUSBAR_FONT_NAME);
	else
		statusbar_font_name = g_strdup(value);
	statusbar_font = pango_font_description_from_string(
	    statusbar_font_name);
	TAILQ_FOREACH(t, &tabs, entry) {
		modify_font(GTK_WIDGET(t->sbe.uri),
		    statusbar_font);
		if (t->sbe.buffercmd != NULL)
			modify_font(GTK_WIDGET(t->sbe.buffercmd),
			    statusbar_font);
		if (t->sbe.zoom != NULL)
			modify_font(GTK_WIDGET(t->sbe.zoom),
			    statusbar_font);
		if (t->sbe.position != NULL)
			modify_font(GTK_WIDGET(t->sbe.position),
			    statusbar_font);
		if (t->sbe.proxy != NULL)
			modify_font(GTK_WIDGET(t->sbe.proxy),
			    statusbar_font);
	}
	return (0);
}

int
check_statusbar_font(char **tt)
{
	*tt = g_strdup_printf("Default: %s", XT_DS_STATUSBAR_FONT_NAME);
	return (g_strcmp0(statusbar_font_name, XT_DS_STATUSBAR_FONT_NAME));
}

int
set_tabbar_font(char *value)
{
	struct tab		*t;

	if (tabbar_font_name)
		g_free(tabbar_font_name);
	if (tabbar_font)
		pango_font_description_free(tabbar_font);
	if (value == NULL || strlen(value) == 0)
		tabbar_font_name = g_strdup(XT_DS_TABBAR_FONT_NAME);
	else
		tabbar_font_name = g_strdup(value);
	tabbar_font = pango_font_description_from_string(tabbar_font_name);
	TAILQ_FOREACH(t, &tabs, entry)
		modify_font(GTK_WIDGET(t->tab_elems.label),
		    tabbar_font);
	return (0);
}

int
check_tabbar_font(char **tt)
{
	*tt = g_strdup_printf("Default: %s", XT_DS_TABBAR_FONT_NAME);
	return (g_strcmp0(tabbar_font_name, XT_DS_TABBAR_FONT_NAME));
}

int
check_tabless(char **tt)
{
	*tt = g_strdup_printf("Default: Disabled\n");
	return (tabless != 0);
}

int
set_color_visited_uris(char *value)
{
	int			tmp;
	const char		*errstr;

	if (value == NULL || strlen(value) == 0)
		color_visited_uris = XT_DS_COLOR_VISITED_URIS;
	else {
		tmp = strtonum(value, 0, 1, &errstr);
		if (errstr)
			return (-1);
		color_visited_uris = tmp;
	}
	return (0);
}

int
check_color_visited_uris(char **tt)
{
	*tt = g_strdup_printf("Default: %s",
	    XT_DS_COLOR_VISITED_URIS ? "Enabled" : "Disabled");
	return (color_visited_uris != XT_DS_COLOR_VISITED_URIS);
}

int
set_home(char *value)
{
	if (home)
		g_free(home);
	if (value == NULL || strlen(value) == 0)
		home = g_strdup(XT_DS_HOME);
	else
		home = g_strdup(value);
	return (0);
}

int
check_home(char **tt)
{
	*tt = g_strdup_printf("Default: %s", XT_DS_HOME);
	return (g_strcmp0(home, XT_DS_HOME));
}

int
set_search_string(char *value)
{
	struct tab		*t;

	if (search_string)
		g_free(search_string);
	if (value == NULL || strlen(value) == 0) {
		search_string = NULL;
		TAILQ_FOREACH(t, &tabs, entry)
			gtk_widget_hide(t->search_entry);
	} else {
		search_string = g_strdup(value);
		TAILQ_FOREACH(t, &tabs, entry)
			gtk_widget_show(t->search_entry);
	}
	return (0);
}

int
check_search_string(char **tt)
{
	*tt = g_strdup_printf("Default: %s", XT_DS_SEARCH_STRING);
	return (g_strcmp0(search_string, XT_DS_SEARCH_STRING));
}

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

	if (s->s && s->s->get)
		r = s->s->get(s);
	else if (s->type == XT_S_INT)
		r = g_strdup_printf("%d", *s->ival);
	else if (s->type == XT_S_STR)
		r = g_strdup(*s->sval);
	else if (s->type == XT_S_DOUBLE)
		r = g_strdup_printf("%lf", *s->dval);
	else if (s->type == XT_S_BOOL)
		r = g_strdup_printf("%d", *s->ival);
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
check_allow_volatile_cookies(char **tt)
{
	*tt = g_strdup("Default: Disabled");
	return (allow_volatile_cookies != 0);
}

int
check_anonymize_headers(char **tt)
{
	*tt = g_strdup("Default: Disabled");
	return (anonymize_headers != 0);
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
		referer_mode = XT_REFERER_SAME_DOMAIN;
		allow_insecure_content = 0;
		allow_insecure_scripts = 0;
		do_not_track = 1;
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
		referer_mode = XT_REFERER_ALWAYS;
		allow_insecure_content = 1;
		allow_insecure_scripts = 1;
		do_not_track = 0;
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
		referer_mode = XT_REFERER_ALWAYS;
		allow_insecure_content = 1;
		allow_insecure_scripts = 1;
		do_not_track = 0;
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
check_browser_mode(char **tt)
{
	*tt = g_strdup("Default: normal");
	return (browser_mode != XT_BM_NORMAL);
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
		show_scrollbars = 1;
	} else if (!strcmp(val, "minimal")) {
		fancy_bar = 0;
		show_tabs = 1;
		tab_style = XT_TABS_COMPACT;
		show_url = 0;
		show_statusbar = 1;
		show_scrollbars = 0;
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
check_gui_mode(char **tt)
{
	*tt = g_strdup("Default: classic");
	return (gui_mode != XT_GM_CLASSIC);
}

int
check_history_autosave(char **tt)
{
	*tt = g_strdup("Default: Disabled");
	return (history_autosave != 0);
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

int
set_cookie_policy_rt(char *value)
{
	if (value == NULL || strlen(value) == 0)
		cookie_policy = XT_DS_COOKIE_POLICY;
	else if (set_cookie_policy(NULL, value))
		return (-1);
	g_object_set(G_OBJECT(s_cookiejar), SOUP_COOKIE_JAR_ACCEPT_POLICY,
	    cookie_policy, (void *)NULL);
	return (0);
}

int
check_cookie_policy(char **tt)
{
	*tt = g_strdup("Default (depends on browser_mode):\n"
	    "\tnormal:\talways\n"
	    "\twhitelist:\tno3rdparty\n"
	    "\tkiosk:\talways");
	if (browser_mode == XT_BM_WHITELIST &&
	    cookie_policy != SOUP_COOKIE_JAR_ACCEPT_NO_THIRD_PARTY)
		return (1);
	if (browser_mode == XT_BM_NORMAL &&
	    cookie_policy != SOUP_COOKIE_JAR_ACCEPT_ALWAYS)
		return (1);
	if (browser_mode == XT_BM_KIOSK &&
	    cookie_policy != SOUP_COOKIE_JAR_ACCEPT_ALWAYS)
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
	expand_tilde(default_script, sizeof default_script, val);
	return (0);
}

int
check_default_script(char **tt)
{
	*tt = g_strdup("Default: (empty)");
	return (g_strcmp0(default_script, XT_DS_DEFAULT_SCRIPT));
}

int
set_default_script_rt(char *value)
{
	if (value == NULL || strlen(value) == 0)
		return set_default_script(NULL, "");
	return (set_default_script(NULL, value));
}

int
set_do_not_track(char *value)
{
	int			tmp;
	const char		*errstr;

	if (value == NULL || strlen(value) == 0)
		do_not_track = XT_DS_DO_NOT_TRACK;
	else {
		tmp = strtonum(value, 0, 1, &errstr);
		if (errstr)
			return (-1);
		do_not_track = tmp;
	}
	return (0);
}

int
check_do_not_track(char **tt)
{
	*tt = g_strdup("Default (depends on browser_mode):\n"
	    "\tnormal:\tDisabled\n"
	    "\twhitelist:\tEnabled\n"
	    "\tkiosk:\tDisabled");
	if (browser_mode == XT_BM_WHITELIST && do_not_track != 1)
		return (1);
	if (browser_mode == XT_BM_NORMAL && do_not_track != 0)
		return (1);
	if (browser_mode == XT_BM_KIOSK && do_not_track != 0)
		return (1);
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
	expand_tilde(download_dir, sizeof download_dir, val);
	return (0);
}

int
check_download_dir(char **tt)
{
	struct passwd		*pwd;
	char			buf[PATH_MAX] = {0};

	/* XXX this might need some additonal magic on windows */
	if ((pwd = getpwuid(getuid())) == NULL)
		return (-1);
	snprintf(buf, sizeof buf, "%s" PS "downloads", pwd->pw_dir);
	*tt = g_strdup_printf("Default: %s", buf);
	return (g_strcmp0(download_dir, buf));
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

	/* Remove additional leading whitespace */
	l += (long)strspn(l, " \t");

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
	{ "passthrough",	CTRL,	1,	GDK_z		},
	{ "modurl",		CTRL,	1,	GDK_Return	},
	{ "modsearchentry",	CTRL,	1,	GDK_Return	},
	{ "urlmod plus",	MOD1,	1,	GDK_a		},
	{ "urlmod min",		MOD1,	1,	GDK_A		},

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
	{ ":open ",		0,	1,	GDK_F9		},
	{ ":open <uri>",	0,	1,	GDK_F10		},
	{ ":tabnew ",		0,	1,	GDK_F11		},
	{ ":tabnew <uri>",	0,	1,	GDK_F12		},
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
keybinding_set(char *cmd, char *key, int use_in_entry)
{
	struct key_binding	*k;
	guint			keyval, mask = 0;
	int			i;

	DNPRINTF(XT_D_KEYBINDING, "keybinding_set: %s %s\n", cmd, key);

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

	if (strcmp(cmd, "unbind") == 0) {
		DNPRINTF(XT_D_KEYBINDING, "keybinding_set: just unbinding: %s\n",
		    gdk_keyval_name(keyval));
		printf("keybinding_set: just unbinding: %s\n",
		    gdk_keyval_name(keyval));
		return (0);
	}

	/* add keyname */
	k = g_malloc0(sizeof *k);
	k->cmd = g_strdup(cmd);
	k->mask = mask;
	k->use_in_entry = use_in_entry;
	k->key = keyval;

	DNPRINTF(XT_D_KEYBINDING, "keybinding_set: %s 0x%x %d 0x%x\n",
	    k->cmd,
	    k->mask,
	    k->use_in_entry,
	    k->key);
	DNPRINTF(XT_D_KEYBINDING, "keybinding_set: adding: %s %s\n",
	    k->cmd, gdk_keyval_name(keyval));

	TAILQ_INSERT_HEAD(&kbl, k, entry);

	return (0);
}

int
cmd_alias_add(char *alias, char *cmd)
{
	struct cmd_alias	*c;

	/* XXX */
	TAILQ_FOREACH(c, &cal, entry)
		if (!strcmp((alias), c->alias)) {
			TAILQ_REMOVE(&cal, c, entry);
			g_free(c);
		}

	c = g_malloc(sizeof (struct cmd_alias));
	c->alias = g_strchug(g_strdup(alias));
	c->cmd = g_strchug(g_strdup(cmd));

	DNPRINTF(XT_D_CUSTOM_URI, "cmd_alias_add: %s %s\n", c->alias, c->cmd);

	TAILQ_INSERT_HEAD(&cal, c, entry);
	return (0);
}

int
custom_uri_add(char *uri, char *cmd)
{
	struct custom_uri	*u;

	TAILQ_FOREACH(u, &cul, entry)
		if (!strcmp((uri), u->uri) && !strcmp(cmd, u->cmd)) {
			TAILQ_REMOVE(&cul, u, entry);
			g_free(u);
		}

	u = g_malloc(sizeof (struct custom_uri));
	u->uri = g_strdup(uri);
	expand_tilde(u->cmd, sizeof u->cmd, cmd);

	DNPRINTF(XT_D_CUSTOM_URI, "custom_uri_add: %s %s\n", u->uri, u->cmd);

	/* don't check here if the script is valid, wait until running it */
	TAILQ_INSERT_HEAD(&cul, u, entry);
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

	return (keybinding_set(entry, key, key[0] == '!'));
}

int
add_custom_uri(struct settings *s, char *entry)
{
	char			*uri, *cmd;

	DNPRINTF(XT_D_CUSTOM_URI, "add_custom_uri: %s\n", entry);

	uri = strstr(entry, ",");
	if (uri == NULL)
		return (1);
	*uri = '\0';
	cmd = uri + 1;

	return (custom_uri_add(entry, cmd));
}

void
walk_custom_uri(struct settings *s,
    void (*cb)(struct settings *, char *, void *), void *cb_args)
{
	struct custom_uri	*u;
	char			buf[1024];

	if (s == NULL || cb == NULL) {
		show_oops(NULL, "walk_custom_uri invalid parameters");
		return;
	}

	TAILQ_FOREACH(u, &cul, entry) {
		snprintf(buf, sizeof buf, "%s,%s", u->uri, u->cmd);
		cb(s, buf, cb_args);
	}
}

int
add_ua(struct settings *s, char *value)
{
	struct user_agent	*ua;
	static int		ua_count = 0;

	ua = g_malloc(sizeof *ua);
	ua->id = ua_count++;
	ua->value = g_strdup(value);

	RB_INSERT(user_agent_list, &ua_list, ua);

	return (0);
}


void
walk_ua(struct settings *s,
    void (*cb)(struct settings *, char *, void *), void *cb_args)
{
	struct user_agent	*ua;

	if (s == NULL || cb == NULL) {
		show_oops(NULL, "walk_ua invalid parameters");
		return;
	}

	RB_FOREACH(ua, user_agent_list, &ua_list)
		cb(s, ua->value, cb_args);
}

int
add_force_https(struct settings *s, char *value)
{
	if (g_str_has_prefix(value, "re:")) {
		value = &value[3];
		wl_add(value, &force_https, XT_WL_PERSISTENT | XT_WL_REGEX);
	} else
		wl_add(value, &force_https, XT_WL_PERSISTENT);
	return (0);
}

int
add_http_accept(struct settings *s, char *value)
{
	struct http_accept		*ha;
	static int			ha_count = 0;

	ha = g_malloc(sizeof *ha);
	ha->id = ha_count++;
	ha->value = g_strdup(value);

	RB_INSERT(http_accept_list, &ha_list, ha);

	return (0);
}

void
walk_http_accept(struct settings *s,
    void (*cb)(struct settings *, char *, void *), void *cb_args)
{
	struct http_accept		*ha;

	if (s == NULL || cb == NULL) {
		show_oops(NULL, "%s: invalid parameters", __func__);
		return;
	}

	RB_FOREACH(ha, http_accept_list, &ha_list)
		cb(s, ha->value, cb_args);
}

int
add_cmd_alias(struct settings *s, char *entry)
{
	char			*alias, *cmd;

	DNPRINTF(XT_D_CMD_ALIAS, "add_cmd_alias: %s\n", entry);

	alias = strstr(entry, ",");
	if (alias == NULL)
		return (1);
	*alias = '\0';
	cmd = alias + 1;

	return (cmd_alias_add(entry, cmd));
}

void
walk_cmd_alias(struct settings *s,
    void (*cb)(struct settings *, char *, void *), void *cb_args)
{
	struct cmd_alias	*c;
	char			buf[1024];

	if (s == NULL || cb == NULL) {
		show_oops(NULL, "walk_cmd_alias invalid parameters");
		return;
	}

	TAILQ_FOREACH(c, &cal, entry) {
		snprintf(buf, sizeof buf, "%s --> %s", c->alias, c->cmd);
		cb(s, buf, cb_args);
	}
}

int
set_allow_insecure_content(char *value)
{
	struct tab		*t;
	int			tmp;
	const char		*errstr;

	if (value == NULL || strlen(value) == 0)
		allow_insecure_content = XT_DS_ALLOW_INSECURE_CONTENT;
	else {
		tmp = strtonum(value, 0, 1, &errstr);
		if (errstr)
			return (-1);
		allow_insecure_content = tmp;
	}
	TAILQ_FOREACH(t, &tabs, entry)
		if (is_g_object_setting(G_OBJECT(t->settings),
		    "enable-display-of-insecure-content")) {
			g_object_set(G_OBJECT(t->settings),
			    "enable-display-of-insecure-content",
			    allow_insecure_content, (char *)NULL);
			webkit_web_view_set_settings(t->wv, t->settings);
		}
	return (0);
}

int
check_allow_insecure_content(char **tt)
{
	*tt = g_strdup("Default (depends on browser_mode):\n"
	    "\tnormal:\tEnabled\n"
	    "\twhitelist:\tDisabled\n"
	    "\tkiosk:\tEnabled");
	if (browser_mode == XT_BM_NORMAL && allow_insecure_content != 1)
		return (1);
	if (browser_mode == XT_BM_WHITELIST && allow_insecure_content != 0)
		return (1);
	if (browser_mode == XT_BM_KIOSK && allow_insecure_content != 1)
		return (1);
	return (0);
}

int
set_allow_insecure_scripts(char *value)
{
	struct tab		*t;
	int			tmp;
	const char		*errstr;

	if (value == NULL || strlen(value) == 0)
		allow_insecure_scripts = XT_DS_ALLOW_INSECURE_SCRIPTS;
	else {
		tmp = strtonum(value, 0, 1, &errstr);
		if (errstr)
			return (-1);
		allow_insecure_scripts = tmp;
	}
	TAILQ_FOREACH(t, &tabs, entry)
		if (is_g_object_setting(G_OBJECT(t->settings),
		    "enable-running-of-insecure-content")) {
			g_object_set(G_OBJECT(t->settings),
			    "enable-running-of-insecure-content",
			    allow_insecure_scripts, (char *)NULL);
			webkit_web_view_set_settings(t->wv, t->settings);
		}
	return (0);
}

int
check_allow_insecure_scripts(char **tt)
{
	*tt = g_strdup("Default (depends on browser_mode):\n"
	    "\tnormal:\tEnabled\n"
	    "\twhitelist:\tDisabled\n"
	    "\tkiosk:\tEnabled");
	if (browser_mode == XT_BM_NORMAL && allow_insecure_scripts != 1)
		return (1);
	if (browser_mode == XT_BM_WHITELIST && allow_insecure_scripts != 0)
		return (1);
	if (browser_mode == XT_BM_KIOSK && allow_insecure_scripts != 1)
		return (1);
	return (0);
}

int
set_auto_load_images(char *value)
{
	struct tab		*t;
	int			tmp;
	const char		*errstr;

	if (value == NULL || strlen(value) == 0)
		auto_load_images = XT_DS_AUTO_LOAD_IMAGES;
	else {
		tmp = strtonum(value, 0, 1, &errstr);
		if (errstr)
			return (-1);
		auto_load_images = tmp;
	}
	TAILQ_FOREACH(t, &tabs, entry) {
		g_object_set(G_OBJECT(t->settings),
		    "auto-load-images", auto_load_images, (char *)NULL);
		webkit_web_view_set_settings(t->wv, t->settings);
	}
	return (0);
}

int
check_auto_load_images(char **tt)
{
	*tt = g_strdup_printf("Default: %s",
	    XT_DS_AUTO_LOAD_IMAGES ? "Enabled" : "Disabled");
	return (auto_load_images != XT_DS_AUTO_LOAD_IMAGES);
}

int
set_autofocus_onload(char *value)
{
	int			tmp;
	const char		*errstr;

	if (value == NULL || strlen(value) == 0)
		autofocus_onload = XT_DS_AUTOFOCUS_ONLOAD;
	else {
		tmp = strtonum(value, 0, 1, &errstr);
		if (errstr)
			return (-1);
		autofocus_onload = tmp;
	}
	return (0);
}

int
check_autofocus_onload(char **tt)
{
	*tt = g_strdup_printf("Default: %s",
	    XT_DS_AUTOFOCUS_ONLOAD ? "Enabled" : "Disabled");
	return (autofocus_onload != XT_DS_AUTOFOCUS_ONLOAD);
}

int
set_ctrl_click_focus(char *value)
{
	int			tmp;
	const char		*errstr;

	if (value == NULL || strlen(value) == 0)
		ctrl_click_focus = XT_DS_CTRL_CLICK_FOCUS;
	else {
		tmp = strtonum(value, 0, 1, &errstr);
		if (errstr)
			return (-1);
		ctrl_click_focus = tmp;
	}
	return (0);
}

int
check_ctrl_click_focus(char **tt)
{
	*tt = g_strdup_printf("Default: %s",
	    XT_DS_CTRL_CLICK_FOCUS ? "Enabled" : "Disabled");
	return (ctrl_click_focus != XT_DS_CTRL_CLICK_FOCUS);
}

int
set_download_notifications(char *value)
{
	int			tmp;
	const char		*errstr;

	if (value == NULL || strlen(value) == 0)
		download_notifications = XT_DS_DOWNLOAD_NOTIFICATIONS;
	else {
		tmp = strtonum(value, 0, 1, &errstr);
		if (errstr)
			return (-1);
		download_notifications = tmp;
	}
	return (0);
}

int
check_download_notifications(char **tt)
{
	*tt = g_strdup_printf("Default: %s",
	    XT_DS_DOWNLOAD_NOTIFICATIONS ? "Enabled" : "Disabled");
	return (download_notifications != XT_DS_DOWNLOAD_NOTIFICATIONS);
}

int
set_enable_autoscroll(char *value)
{
	int			tmp;
	const char		*errstr;

	if (value == NULL || strlen(value) == 0)
		enable_autoscroll = XT_DS_ENABLE_AUTOSCROLL;
	else {
		tmp = strtonum(value, 0, 1, &errstr);
		if (errstr)
			return (-1);
		enable_autoscroll = tmp;
	}
	return (0);
}

int
check_enable_autoscroll(char **tt)
{
	*tt = g_strdup_printf("Default: %s",
	    XT_DS_ENABLE_AUTOSCROLL ? "Enabled" : "Disabled");
	return (enable_autoscroll != XT_DS_ENABLE_AUTOSCROLL);
}

int
set_enable_cache(char *value)
{
	int		tmp;
	const char	*errstr;

	if (value == NULL || strlen(value) == 0)
		enable_cache = 1;
	else {
		tmp = (int)strtonum(value, 0, 1, &errstr);
		if (errstr)
			return (-1);
		enable_cache = tmp;
	}
        if (enable_cache)
                webkit_set_cache_model(WEBKIT_CACHE_MODEL_WEB_BROWSER);
        else
                webkit_set_cache_model(WEBKIT_CACHE_MODEL_DOCUMENT_VIEWER);

	return (0);
}

int
check_enable_cache(char **tt)
{
	*tt = g_strdup_printf("Default: Disabled");

	return (enable_cache);
}

int
set_enable_cookie_whitelist(char *value)
{
	int			tmp;
	const char		*errstr;

	if (value == NULL || strlen(value) == 0)
		enable_cookie_whitelist = XT_DS_ENABLE_COOKIE_WHITELIST;
	else {
		tmp = strtonum(value, 0, 1, &errstr);
		if (errstr)
			return (-1);
		enable_cookie_whitelist = tmp;
	}
	return (0);
}

int
check_enable_cookie_whitelist(char **tt)
{
	*tt = g_strdup("Default (depends on browser_mode):\n"
	    "\tnormal:\tDisabled\n"
	    "\twhitelist:\tEnabled\n"
	    "\tkiosk:\tDisabled");
	if (browser_mode == XT_BM_WHITELIST && enable_cookie_whitelist != 1)
		return (1);
	if (browser_mode == XT_BM_NORMAL && enable_cookie_whitelist != 0)
		return (1);
	if (browser_mode == XT_BM_KIOSK && enable_cookie_whitelist != 0)
		return (1);
	return (0);
}

int
set_enable_js_autorun(char *value)
{
	int			tmp;
	const char		*errstr;

	if (value == NULL || strlen(value) == 0)
		enable_js_autorun = XT_DS_ENABLE_JS_AUTORUN;
	else {
		tmp = strtonum(value, 0, 1, &errstr);
		if (errstr)
			return (-1);
		enable_js_autorun = tmp;
	}
	return (0);
}

int
check_enable_js_autorun(char **tt)
{
	*tt = g_strdup_printf("Default: %s",
	    XT_DS_ENABLE_JS_AUTORUN ? "Enabled" : "Disabled");
	return (enable_js_autorun != XT_DS_ENABLE_JS_AUTORUN);
}

int
set_enable_js_whitelist(char *value)
{
	int			tmp;
	const char		*errstr;

	if (value == NULL || strlen(value) == 0)
		enable_js_whitelist = XT_DS_ENABLE_JS_WHITELIST;
	else {
		tmp = strtonum(value, 0, 1, &errstr);
		if (errstr)
			return (-1);
		enable_js_whitelist = tmp;
	}
	return (0);
}

int
check_enable_js_whitelist(char **tt)
{
	*tt = g_strdup("Default (depends on browser_mode):\n"
	    "\tnormal:\tDisabled\n"
	    "\twhitelist:\tEnabled\n"
	    "\tkiosk:\tDisabled");
	if (browser_mode == XT_BM_WHITELIST && enable_js_whitelist != 1)
		return (1);
	if (browser_mode == XT_BM_NORMAL && enable_js_whitelist != 0)
		return (1);
	if (browser_mode == XT_BM_KIOSK && enable_js_whitelist != 0)
		return (1);
	return (0);
}

int
set_enable_favicon_entry(char *value)
{
	int			tmp;
	const char		*errstr;

	if (value == NULL || strlen(value) == 0)
		enable_favicon_entry = XT_DS_ENABLE_FAVICON_ENTRY;
	else {
		tmp = strtonum(value, 0, 1, &errstr);
		if (errstr)
			return (-1);
		enable_favicon_entry = tmp;
	}
	return (0);
}

int
check_enable_favicon_entry(char **tt)
{
	*tt = g_strdup_printf("Default: %s",
	    XT_DS_ENABLE_FAVICON_ENTRY ? "Enabled" : "Disabled");
	return (enable_favicon_entry != XT_DS_ENABLE_FAVICON_ENTRY);
}

int
set_enable_favicon_tabs(char *value)
{
	int			tmp;
	const char		*errstr;

	if (value == NULL || strlen(value) == 0)
		enable_favicon_tabs = XT_DS_ENABLE_FAVICON_TABS;
	else {
		tmp = strtonum(value, 0, 1, &errstr);
		if (errstr)
			return (-1);
		enable_favicon_tabs = tmp;
	}
	return (0);
}

int
check_enable_favicon_tabs(char **tt)
{
	*tt = g_strdup_printf("Default: %s",
	    XT_DS_ENABLE_FAVICON_TABS ? "Enabled" : "Disabled");
	return (enable_favicon_tabs != XT_DS_ENABLE_FAVICON_TABS);
}

int
set_enable_localstorage(char *value)
{
	struct tab		*t;
	int			tmp;
	const char		*errstr;

	if (value == NULL || strlen(value) == 0)
		enable_localstorage = XT_DS_ENABLE_LOCALSTORAGE;
	else {
		tmp = strtonum(value, 0, 1, &errstr);
		if (errstr)
			return (-1);
		enable_localstorage = tmp;
	}
	TAILQ_FOREACH(t, &tabs, entry)
		g_object_set(G_OBJECT(t->settings),
		    "enable-html5-local-storage", enable_localstorage,
		    (char *)NULL);
	return (0);
}

int
check_enable_localstorage(char **tt)
{
	*tt = g_strdup("Default (depends on browser_mode):\n"
	    "\tnormal:\tEnabled\n"
	    "\twhitelist:\tDisabled\n"
	    "\tkiosk:\tEnabled");
	if (browser_mode == XT_BM_WHITELIST && enable_localstorage != 0)
		return (1);
	if (browser_mode == XT_BM_NORMAL && enable_localstorage != 1)
		return (1);
	if (browser_mode == XT_BM_KIOSK && enable_localstorage != 1)
		return (1);
	return (0);
}

int
set_enable_plugins(char *value)
{
	struct tab		*t;
	int			tmp;
	const char		*errstr;

	if (value == NULL || strlen(value) == 0)
		enable_plugins = XT_DS_ENABLE_PLUGINS;
	else {
		tmp = strtonum(value, 0, 1, &errstr);
		if (errstr)
			return (-1);
		enable_plugins = tmp;
	}
	TAILQ_FOREACH(t, &tabs, entry)
		g_object_set(G_OBJECT(t->settings), "enable-plugins",
		    enable_plugins, (char *)NULL);
	return (0);
}

int
check_enable_plugins(char **tt)
{
	*tt = g_strdup("Default (depends on browser_mode):\n"
	    "\tnormal:\tEnabled\n"
	    "\twhitelist:\tDisabled\n"
	    "\tkiosk:\tEnabled");
	if (browser_mode == XT_BM_WHITELIST && enable_plugins != 0)
		return (1);
	if (browser_mode == XT_BM_NORMAL && enable_plugins != 1)
		return (1);
	if (browser_mode == XT_BM_KIOSK && enable_plugins != 1)
		return (1);
	return (0);
}

int
set_enable_plugin_whitelist(char *value)
{
	int			tmp;
	const char		*errstr;

	if (value == NULL || strlen(value) == 0)
		enable_plugin_whitelist = XT_DS_ENABLE_PLUGIN_WHITELIST;
	else {
		tmp = strtonum(value, 0, 1, &errstr);
		if (errstr)
			return (-1);
		enable_plugin_whitelist = tmp;
	}
	return (0);
}

int
check_enable_plugin_whitelist(char **tt)
{
	*tt = g_strdup("Default (depends on browser_mode):\n"
	    "\tnormal:\tDisabled\n"
	    "\twhitelist:\tEnabled\n"
	    "\tkiosk:\tDisabled");
	if (browser_mode == XT_BM_WHITELIST && enable_plugin_whitelist != 1)
		return (1);
	if (browser_mode == XT_BM_NORMAL && enable_plugin_whitelist != 0)
		return (1);
	if (browser_mode == XT_BM_KIOSK && enable_plugin_whitelist != 0)
		return (1);
	return (0);
}

int
set_enable_scripts(char *value)
{
	struct tab		*t;
	int			tmp;
	const char		*errstr;

	if (value == NULL || strlen(value) == 0)
		enable_scripts = XT_DS_ENABLE_SCRIPTS;
	else {
		tmp = strtonum(value, 0, 1, &errstr);
		if (errstr)
			return (-1);
		enable_scripts = tmp;
	}
	TAILQ_FOREACH(t, &tabs, entry)
		g_object_set(G_OBJECT(t->settings), "enable-scripts",
		    enable_scripts, (char *)NULL);
	return (0);
}

int
check_enable_scripts(char **tt)
{
	*tt = g_strdup("Default (depends on browser_mode):\n"
	    "\tnormal:\tEnabled\n"
	    "\twhitelist:\tDisabled\n"
	    "\tkiosk:\tEnabled");
	if (browser_mode == XT_BM_WHITELIST && enable_scripts != 0)
		return (1);
	if (browser_mode == XT_BM_NORMAL && enable_scripts != 1)
		return (1);
	if (browser_mode == XT_BM_KIOSK && enable_scripts != 1)
		return (1);
	return (0);
}

int
check_enable_socket(char **tt)
{
	*tt = g_strdup("Default: Disabled");
	return (enable_socket != 0);
}

int
set_enable_spell_checking(char *value)
{
	struct tab		*t;
	int			tmp;
	const char		*errstr;

	if (value == NULL || strlen(value) == 0)
		enable_spell_checking = XT_DS_ENABLE_SPELL_CHECKING;
	else {
		tmp = strtonum(value, 0, 1, &errstr);
		if (errstr)
		return (-1);
		enable_spell_checking = tmp;
	}
	TAILQ_FOREACH(t, &tabs, entry)
		g_object_set(G_OBJECT(t->settings), "enable_spell_checking",
		    enable_spell_checking, (char *)NULL);
	return (0);
}

int
check_enable_spell_checking(char **tt)
{
	*tt = g_strdup_printf("Default: %s",
	    XT_DS_ENABLE_SPELL_CHECKING ? "Enabled" : "Disabled");
	return (enable_spell_checking != XT_DS_ENABLE_SPELL_CHECKING);
}

int
set_enable_strict_transport(char *value)
{
	int			tmp;
	const char		*errstr;

	if (value == NULL || strlen(value) == 0)
		enable_strict_transport = XT_DS_ENABLE_STRICT_TRANSPORT;
	else {
		tmp = strtonum(value, 0, 1, &errstr);
		if (errstr)
			return (-1);
		enable_strict_transport = tmp;
	}
	return (0);
}

int
check_enable_strict_transport(char **tt)
{
	*tt = g_strdup_printf("Default: %s",
	    XT_DS_ENABLE_STRICT_TRANSPORT ? "Enabled" : "Disabled");
	return (enable_strict_transport != XT_DS_ENABLE_STRICT_TRANSPORT);
}

#if 0
/*
 * XXX: this is currently broken.  Need to figure out what to do with
 * this.  Problemm is set_encoding will refresh the tab it's run on, so
 * we can either put a big fat warning in the manpage and refresh every
 * single open tab with the new encoding or scrap it as a runtime
 * setting.
 */
int
set_encoding_rt(char *value)
{
	struct karg		args = {0};

	if (value == NULL || strlen(value) == 0)
		return (-1);
	if (encoding)
		g_free(encoding);
	encoding = g_strdup(value);
	args.s = encoding;
	set_encoding(get_current_tab(), &args);
	return (0);
}
#endif

int
check_encoding(char **tt)
{
	*tt = g_strdup_printf("Default: %s", XT_DS_ENCODING);
	return (g_strcmp0(encoding, XT_DS_ENCODING));
}

int
check_gnutls_search_string(char **tt)
{
	*tt = g_strdup("Default: (empty)");
	return (g_strcmp0(gnutls_priority_string,
	    XT_DS_GNUTLS_PRIORITY_STRING));
}

int
set_gnutls_priority_string(struct settings *s, char *value)
{
	return (!g_setenv("G_TLS_GNUTLS_PRIORITY", value, FALSE));
}

char *
get_gnutls_priority_string(struct settings *s)
{
	return (g_strdup(g_getenv("G_TLS_GNUTLS_PRIORITY")));
}

int
set_guess_search(char *value)
{
	int			tmp;
	const char		*errstr;

	if (value == NULL || strlen(value) == 0)
		guess_search = XT_DS_GUESS_SEARCH;
	else {
		tmp = strtonum(value, 0, 1, &errstr);
		if (errstr)
			return (-1);
		guess_search = tmp;
	}
	return (0);
}

int
check_guess_search(char **tt)
{
	*tt = g_strdup_printf("Default: %s",
	    XT_DS_GUESS_SEARCH ? "Enabled" : "Disabled");
	return (guess_search != XT_DS_GUESS_SEARCH);
}

int
set_js_auto_open_windows(char *value)
{
	int		tmp;
	const char	*errstr;
	struct tab	*t;

	if (value == NULL || strlen(value) == 0)
		js_auto_open_windows = XT_DS_JS_AUTO_OPEN_WINDOWS;
	else {
		tmp = (int)strtonum(value, 0, 1, &errstr);
		if (errstr)
			return (-1);
		js_auto_open_windows = tmp;
	}
	TAILQ_FOREACH(t, &tabs, entry)
		g_object_set(G_OBJECT(t->settings),
		    "javascript-can-open-windows-automatically",
		    js_auto_open_windows, NULL);
	return (0);
}

int
check_js_auto_open_windows(char **tt)
{
	*tt = g_strdup("Default: Enabled");
	return (js_auto_open_windows != XT_DS_JS_AUTO_OPEN_WINDOWS);
}

char *
get_referer(struct settings *s)
{
	if (referer_mode == XT_REFERER_ALWAYS)
		return (g_strdup("always"));
	if (referer_mode == XT_REFERER_NEVER)
		return (g_strdup("never"));
	if (referer_mode == XT_REFERER_SAME_DOMAIN)
		return (g_strdup("same-domain"));
	if (referer_mode == XT_REFERER_SAME_FQDN)
		return (g_strdup("same-fqdn"));
	if (referer_mode == XT_REFERER_CUSTOM)
		return (g_strdup(referer_custom));
	return (NULL);
}

int
set_referer(struct settings *s, char *value)
{
	if (referer_custom) {
		g_free(referer_custom);
		referer_custom = NULL;
	}

	if (!strcmp(value, "always"))
		referer_mode = XT_REFERER_ALWAYS;
	else if (!strcmp(value, "never"))
		referer_mode = XT_REFERER_NEVER;
	else if (!strcmp(value, "same-domain"))
		referer_mode = XT_REFERER_SAME_DOMAIN;
	else if (!strcmp(value, "same-fqdn"))
		referer_mode = XT_REFERER_SAME_FQDN;
	else if (!valid_url_type(value)) {
		referer_mode = XT_REFERER_CUSTOM;
		referer_custom = g_strdup(value);
	} else {
		/* we've already free'd the custom referer */
		if (referer_mode == XT_REFERER_CUSTOM)
			referer_mode = XT_REFERER_NEVER;
		return (1);
	}

	return (0);
}

int
set_referer_rt(char *value)
{
	if (value == NULL || strlen(value) == 0) {
		if (referer_custom)
			g_free(referer_custom);
		referer_custom = g_strdup(XT_DS_REFERER_CUSTOM);
		referer_mode = XT_DS_REFERER_MODE;
		return (0);
	}
	return (set_referer(NULL, value));
}

int
check_referer(char **tt)
{
	*tt = g_strdup("Default (depends on browser_mode):\n"
	    "\tnormal:\talways\n"
	    "\twhitelist:\tsame-domain\n"
	    "\tkiosk:\talways");
	if (browser_mode == XT_BM_WHITELIST &&
	    referer_mode != XT_REFERER_SAME_DOMAIN)
		return (1);
	if (browser_mode == XT_BM_NORMAL && referer_mode != XT_REFERER_ALWAYS)
		return (1);
	if (browser_mode == XT_BM_KIOSK && referer_mode != XT_REFERER_ALWAYS)
		return (1);
	return (0);
}

char *
get_ssl_ca_file(struct settings *s)
{
	if (strlen(ssl_ca_file) == 0)
		return (NULL);
	return (g_strdup(ssl_ca_file));
}

int
set_refresh_interval(char *value)
{
	int			tmp;
	const char		*errstr;

	if (value == NULL || strlen(value) == 0)
		refresh_interval = XT_DS_REFRESH_INTERVAL;
	else {
		tmp = strtonum(value, 0, INT_MAX, &errstr);
		if (errstr)
			return (-1);
		refresh_interval = tmp;
	}
	return (0);
}

int
check_resource_dir(char **tt)
{
	*tt = g_strdup_printf("Default: %s", XT_DS_RESOURCE_DIR);
	return (g_strcmp0(resource_dir, XT_DS_RESOURCE_DIR));
}

int
check_save_global_history(char **tt)
{
	*tt = g_strdup("Default: Disabled");
	return (save_global_history != 0);
}

int
check_save_rejected_cookies(char **tt)
{
	*tt = g_strdup("Default: Disabled");
	return (save_rejected_cookies != 0);
}

int
check_refresh_interval(char **tt)
{
	*tt = g_strdup_printf("Default: %d", XT_DS_REFRESH_INTERVAL);
	return (refresh_interval != XT_DS_REFRESH_INTERVAL);
}

int
set_session_autosave(char *value)
{
	int			tmp;
	const char		*errstr;

	if (value == NULL || strlen(value) == 0)
		session_autosave = XT_DS_SESSION_AUTOSAVE;
	else {
		tmp = strtonum(value, 0, 1, &errstr);
		if (errstr)
		return (-1);
		session_autosave = tmp;
	}
	return (0);
}

int
check_session_autosave(char **tt)
{
	*tt = g_strdup_printf("Default: %s",
	    XT_DS_SESSION_AUTOSAVE ? "Enabled" : "Disabled");
	return (session_autosave != XT_DS_SESSION_AUTOSAVE);
}

int
set_session_timeout(char *value)
{
	int			tmp;
	const char		*errstr;

	if (value == NULL || strlen(value) == 0)
		session_timeout = XT_DS_SESSION_TIMEOUT;
	else {
		tmp = strtonum(value, 0, INT_MAX, &errstr);
		if (errstr)
			return (-1);
		session_timeout = tmp;
	}
	return (0);
}

int
check_session_timeout(char **tt)
{
	*tt = g_strdup_printf("Default: %d", XT_DS_SESSION_TIMEOUT);
	return (session_timeout != XT_DS_SESSION_TIMEOUT);
}

int
set_show_scrollbars(char *value)
{
	struct tab		*t;
	int			tmp;
	const char		*errstr;

	if (value == NULL || strlen(value) == 0)
		tmp = XT_DS_SHOW_SCROLLBARS;
	else {
		tmp = strtonum(value, 0, 1, &errstr);
		if (errstr)
			return (-1);
	}
	show_scrollbars = tmp;
	TAILQ_FOREACH(t, &tabs, entry)
		if (set_scrollbar_visibility(t, show_scrollbars))
			return (-1);
	return (0);
}

int
check_show_scrollbars(char **tt)
{
	*tt = g_strdup("Default (depends on gui_mode):\n"
	    "\tclassic:\tEnabled\n"
	    "\tminimal:\tDisabled");
	if (gui_mode == XT_GM_CLASSIC && show_scrollbars != 1)
		return (1);
	if (gui_mode == XT_GM_MINIMAL && show_scrollbars != 0)
		return (1);
	return (0);
}

int
set_show_statusbar(char *value)
{
	int			tmp;
	const char		*errstr;

	if (value == NULL || strlen(value) == 0)
		tmp = XT_DS_SHOW_STATUSBAR;
	else {
		tmp = strtonum(value, 0, 1, &errstr);
		if (errstr)
			return (-1);
	}
	show_statusbar = tmp;
	statusbar_set_visibility();
	return (0);
}

int
check_show_statusbar(char **tt)
{
	*tt = g_strdup("Default (depends on gui_mode):\n"
	    "\tclassic:\tDisabled\n"
	    "\tminimal:\tEnabled");
	if (gui_mode == XT_GM_CLASSIC && show_statusbar != 0)
		return (1);
	if (gui_mode == XT_GM_MINIMAL && show_statusbar != 1)
		return (1);
	return (0);
}

int
set_show_tabs(char *value)
{
	struct karg		args = {0};
	int			val;
	const char		*errstr;

	if (value == NULL || strlen(value) == 0)
		val = XT_DS_SHOW_TABS;
	else {
		val = strtonum(value, 0, 1, &errstr);
		if (errstr)
			return (-1);
	}
	args.i = val ? XT_TAB_SHOW : XT_TAB_HIDE;
	tabaction(get_current_tab(), &args);
	return (0);
}

int
check_show_tabs(char **tt)
{
	*tt = g_strdup("Default (depends on gui_mode):\n"
	    "\tclassic:\tEnabled\n"
	    "\tminimal:\tDisabled");
	if (gui_mode == XT_GM_CLASSIC && show_tabs != 1)
		return (1);
	if (gui_mode == XT_GM_MINIMAL && show_tabs != 0)
		return (1);
	return (0);
}

int
set_show_url(char *value)
{
	struct karg		args = {0};
	int			val;
	const char		*errstr;

	if (value == NULL || strlen(value) == 0)
		val = XT_DS_SHOW_URL;
	else {
		val = strtonum(value, 0, 1, &errstr);
		if (errstr)
			return (-1);
	}
	args.i = val ? XT_URL_SHOW : XT_URL_HIDE;
	urlaction(get_current_tab(), &args);
	return (0);
}

int
check_show_url(char **tt)
{
	*tt = g_strdup("Default (depends on gui_mode):\n"
	    "\tclassic:\tEnabled\n"
	    "\tminimal:\tDisabled");
	if (gui_mode == XT_GM_CLASSIC && show_url != 1)
		return (1);
	if (gui_mode == XT_GM_MINIMAL && show_url != 0)
		return (1);
	return (0);
}

int
check_single_instance(char **tt)
{
	*tt = g_strdup("Default: Disabled");
	return (single_instance != 0);
}

int
set_spell_check_languages(char *value)
{
	struct tab		*t;

	if (spell_check_languages)
		g_free(spell_check_languages);
	if (value == NULL || strlen(value) == 0)
		spell_check_languages = g_strdup(XT_DS_SPELL_CHECK_LANGUAGES);
	else
		spell_check_languages = g_strdup(value);
	TAILQ_FOREACH(t, &tabs, entry)
		g_object_set(G_OBJECT(t->settings), "spell_checking_languages",
		    spell_check_languages, (char *)NULL);
	return (0);
}

int
check_spell_check_languages(char **tt)
{
	*tt = g_strdup_printf("Default: %s", XT_DS_SPELL_CHECK_LANGUAGES);
	return (g_strcmp0(spell_check_languages, XT_DS_SPELL_CHECK_LANGUAGES));
}

int
check_valid_file(char *name)
{
	struct stat		sb;

	if (name == NULL || stat(name, &sb))
		return (-1);
	return (0);
}

int
set_ssl_ca_file_rt(char *value)
{
	if (value == NULL || strlen(value) == 0) {
		strlcpy(ssl_ca_file, XT_DS_SSL_CA_FILE, sizeof ssl_ca_file);
		g_object_set(session, SOUP_SESSION_SSL_CA_FILE, "", NULL);
		return (0);
	} else
		return (set_ssl_ca_file(NULL, value));
}

int
check_ssl_ca_file(char **tt)
{
	*tt = g_strdup("Default: (empty)");
	return (g_strcmp0(ssl_ca_file, XT_DS_SSL_CA_FILE));
}

int
set_ssl_strict_certs(char *value)
{
	int			tmp;
	const char		*errstr;

	if (value == NULL || strlen(value) == 0)
		ssl_strict_certs = XT_DS_SSL_STRICT_CERTS;
	else {
		tmp = strtonum(value, 0, 1, &errstr);
		if (errstr)
			return (-1);
		ssl_strict_certs = tmp;
	}
	g_object_set(session, SOUP_SESSION_SSL_STRICT, ssl_strict_certs, NULL);
	return (0);
}

int
check_ssl_strict_certs(char **tt)
{
	*tt = g_strdup_printf("Default: %s",
	    XT_DS_SSL_STRICT_CERTS ? "Enabled" : "Disabled");
	return (ssl_strict_certs != XT_DS_SSL_STRICT_CERTS);
}

int
check_statusbar_elems(char **tt)
{
	*tt = g_strdup("Default: BP");
	return (g_strcmp0(statusbar_elems, "BP"));
}

int
check_tab_elems(char **tt)
{
	*tt = g_strdup("Default: CT");
	return (g_strcmp0(tab_elems, "CT"));
}

int
set_external_editor(char *editor)
{
	if (external_editor)
		g_free(external_editor);
	if (editor == NULL || strlen(editor) == 0)
		external_editor = NULL;
	else
		external_editor = g_strdup(editor);
	return (0);
}

int
check_external_editor(char **tt)
{
	*tt = g_strdup("Default: (empty)");
	return (g_strcmp0(external_editor, NULL));
}

int
set_fancy_bar(char *value)
{
	struct tab		*t;
	int			tmp;
	const char		*errstr;

	if (value == NULL || strlen(value) == 0)
		fancy_bar = 1;	/* XXX */
	else {
		tmp = strtonum(value, 0, 1, &errstr);
		if (errstr)
			return (-1);
		fancy_bar = tmp;
	}
	TAILQ_FOREACH(t, &tabs, entry)
		if (fancy_bar) {
			gtk_widget_show(t->backward);
			gtk_widget_show(t->forward);
			gtk_widget_show(t->stop);
			gtk_widget_show(t->js_toggle);
			if (search_string && strlen(search_string))
				gtk_widget_show(t->search_entry);
		} else {
			gtk_widget_hide(t->backward);
			gtk_widget_hide(t->forward);
			gtk_widget_hide(t->stop);
			gtk_widget_hide(t->js_toggle);
			gtk_widget_hide(t->search_entry);
		}
	return (0);
}

int
check_fancy_bar(char **tt)
{
	*tt = g_strdup("Default (depends on gui_mode):\n"
	    "\tclassic:\tEnabled\n"
	    "\tminimal:\tDisabled");
	if (gui_mode == XT_GM_CLASSIC && fancy_bar != 1)
		return (1);
	if (gui_mode == XT_GM_MINIMAL && fancy_bar != 0)
		return (1);
	return (0);
}

int
setup_proxy(const char *uri)
{
	struct tab		*t;

	if (proxy_uri) {
#if SOUP_CHECK_VERSION(2, 42, 2)
		g_object_set(session, "proxy-resolver", NULL, (char *)NULL);
		g_object_unref(proxy_uri);
#else
		g_object_set(session, "proxy-uri", NULL, (char *)NULL);
		soup_uri_free(proxy_uri);
#endif
		proxy_uri = NULL;
		TAILQ_FOREACH(t, &tabs, entry)
			if (t->sbe.proxy != NULL)
				gtk_label_set_text(GTK_LABEL(t->sbe.proxy), "");
	}

	if (http_proxy) {
		if (http_proxy != uri) {
			g_free(http_proxy);
			http_proxy = NULL;
		}
	}

	if (uri && check_http_proxy_scheme(uri) != 1)
		return (1);

	if (uri) {
		http_proxy = g_strdup(uri);
		DNPRINTF(XT_D_CONFIG, "setup_proxy: %s\n", uri);
#if SOUP_CHECK_VERSION(2, 42, 2)
		proxy_uri = g_simple_proxy_resolver_new(http_proxy, proxy_exclude);
		if (proxy_uri != NULL) {
			g_object_set(session, "proxy-resolver", proxy_uri,
			    (char *)NULL);
#else
		proxy_uri = soup_uri_new(http_proxy);
		if (proxy_uri != NULL && SOUP_URI_VALID_FOR_HTTP(proxy_uri)) {
			g_object_set(session, "proxy-uri", proxy_uri,
			    (char *)NULL);
#endif
			TAILQ_FOREACH(t, &tabs, entry) {
				if (t->sbe.proxy != NULL) {
					gtk_label_set_text(
					    GTK_LABEL(t->sbe.proxy), "proxy");
				}
				gtk_widget_show(t->proxy_toggle);
				button_set_file(t->proxy_toggle,
				    "torenabled.ico");
			}
		}
	} else {
		TAILQ_FOREACH(t, &tabs, entry)
			button_set_file(t->proxy_toggle, "tordisabled.ico");
	}
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
check_tab_style(char **tt)
{
	*tt = g_strdup("Default (depends on gui_mode):\n"
	    "\tclassic:\tnormal\n"
	    "\tminimal:\tcompact");
	if (gui_mode == XT_GM_CLASSIC && tab_style != XT_TABS_NORMAL)
		return (1);
	if (gui_mode == XT_GM_MINIMAL && tab_style != XT_TABS_COMPACT)
		return (1);
	return (0);
}

int
set_tab_style_rt(char *value)
{
	struct karg		args = {0};
	int			old_tab_style;

	if (value == NULL || strlen(value) == 0) {
		if (tab_style == XT_DS_TAB_STYLE)
			return (0);
		tab_style = XT_TABS_COMPACT;
		args.i = XT_TAB_NEXTSTYLE;
	} else {
		old_tab_style = tab_style;
		if (set_tab_style(NULL, value))
			return (-1);
		if (old_tab_style != tab_style) {
			tab_style = old_tab_style;
			args.i = XT_TAB_NEXTSTYLE;
		}
	}
	tabaction(get_current_tab(), &args);
	return (0);
}

char *
get_statusbar_style(struct settings *s)
{
	if (statusbar_style == XT_STATUSBAR_URL)
		return (g_strdup("url"));
	else
		return (g_strdup("title"));
}

int
set_statusbar_style(struct settings *s, char *val)
{
	if (!strcmp(val, "url"))
		statusbar_style = XT_STATUSBAR_URL;
	else if (!strcmp(val, "title"))
		statusbar_style = XT_STATUSBAR_TITLE;
	else
		return (1);

	return (0);
}

int
set_statusbar_style_rt(char *value)
{
	struct tab		*t;
	const gchar		*page_uri;

	if (value == NULL || strlen(value) == 0) {
		if (statusbar_style == XT_DS_STATUSBAR_STYLE)
			return (0);
		statusbar_style = XT_DS_STATUSBAR_STYLE;
	} else {
		if (!strcmp(value, "url"))
			statusbar_style = XT_STATUSBAR_URL;
		else if (!strcmp(value, "title"))
			statusbar_style = XT_STATUSBAR_TITLE;
	}

	/* apply changes */
	TAILQ_FOREACH(t, &tabs, entry) {
		if (statusbar_style == XT_STATUSBAR_TITLE)
			set_status(t, "%s", get_title(t, FALSE));
		else if ((page_uri = get_uri(t)) != NULL)
			set_status(t, "%s", page_uri);
	}
	
	return (0);
}

int
check_statusbar_style(char **tt)
{
	*tt = g_strdup("Default: url");
	return (statusbar_style != XT_DS_STATUSBAR_STYLE);
}

int
set_url_regex(char *value)
{
	if (value == NULL || strlen(value) == 0) {
		if (url_regex)
			g_free(url_regex);
		url_regex = g_strdup(XT_DS_URL_REGEX);
	} else {
		if (regcomp(&url_re, value, REG_EXTENDED | REG_NOSUB))
			return (-1);
		if (url_regex)
			g_free(url_regex);
		url_regex = g_strdup(value);
	}
	return (0);
}

int
check_url_regex(char **tt)
{
	*tt = g_strdup_printf("Default: %s", XT_DS_URL_REGEX);
	return (g_strcmp0(url_regex, XT_DS_URL_REGEX));
}

int
set_userstyle(struct settings *s, char *value)
{
	int		rv = 0;
	char		script[PATH_MAX] = {'\0'};
	char		*tmp;

	if (value == NULL || strlen(value) == 0) {
		snprintf(script, sizeof script, "%s" PS "style.css", resource_dir);
		tmp = g_filename_to_uri(script, NULL, NULL);
		if (tmp == NULL)
			rv = -1;
		else {
			if (userstyle)
				g_free(userstyle);
			userstyle = tmp;
		}
	} else {
		expand_tilde(script, sizeof script, value);
		tmp = g_filename_to_uri(script, NULL, NULL);
		if (tmp == NULL)
			rv = -1;
		else {
			if (userstyle)
				g_free(userstyle);
			userstyle = tmp;
		}
	}
	if (stylesheet)
		g_free(stylesheet);
	stylesheet = g_strdup(userstyle);
	return (rv);
}

char *
get_userstyle(struct settings *s)
{
	if (userstyle)
		return (g_filename_from_uri(userstyle, NULL, NULL));
	return (NULL);
}

int
set_userstyle_rt(char *value)
{
	return (set_userstyle(NULL, value));
}

int
check_userstyle(char **tt)
{
	int			rv = 0;
	char			buf[PATH_MAX];
	char			*file = NULL;

	snprintf(buf, sizeof buf, "%s" PS "%s", resource_dir, "style.css");
	*tt = g_strdup_printf("Default: %s", buf);
	file = g_filename_from_uri(userstyle, NULL, NULL);
	rv = g_strcmp0(file, buf);
	if (file)
		g_free(file);
	return (rv);
}

int
set_userstyle_global(char *value)
{
	struct karg		args = {0};
	int			tmp, old_style;
	const char		*errstr;

	if (value == NULL || strlen(value) == 0) {
		if (userstyle_global == XT_DS_USERSTYLE_GLOBAL)
			return (0);
		userstyle_global = 1;
		args.i = XT_STYLE_GLOBAL;
		userstyle_cmd(get_current_tab(), &args);
	} else {
		old_style = userstyle_global;
		tmp = strtonum(value, 0, 1, &errstr);
		if (errstr)
			return (-1);
		if (tmp != old_style) {
			args.i = XT_STYLE_GLOBAL;
			userstyle_cmd(get_current_tab(), &args);
		}
	}
	return (0);
}

int
check_userstyle_global(char **tt)
{
	*tt = g_strdup_printf("Default: %s",
	    XT_DS_USERSTYLE_GLOBAL ? "Enabled" : "Disabled");
	return (userstyle_global != XT_DS_USERSTYLE_GLOBAL);
}

int
set_warn_cert_changes(char *value)
{
	int			tmp;
	const char		*errstr;

	if (value == NULL || strlen(value) == 0)
		warn_cert_changes = XT_DS_WARN_CERT_CHANGES;
	else {
		tmp = strtonum(value, 0, 1, &errstr);
		if (errstr)
			return (-1);
		warn_cert_changes = tmp;
	}
	return (0);
}

int
check_warn_cert_changes(char **tt)
{
	*tt = g_strdup_printf("Default: %s",
	    XT_DS_WARN_CERT_CHANGES ? "Enabled" : "Disabled");
	return (warn_cert_changes != XT_DS_WARN_CERT_CHANGES);
}

int
check_window_height(char **tt)
{
	*tt = g_strdup_printf("Default: %d", 768);
	return (window_height != 768);
}

int
check_window_maximize(char **tt)
{
	*tt = g_strdup("Default: Disabled");
	return (window_maximize != 0);
}

int
check_window_width(char **tt)
{
	*tt = g_strdup_printf("Default: %d", 768);
	return (window_width != 1024);
}

int
check_work_dir(char **tt)
{
	struct passwd		*pwd;
	char			buf[PATH_MAX];

	if ((pwd = getpwuid(getuid())) == NULL)
		return (-1);
	snprintf(buf, sizeof buf, "%s" PS ".xombrero", pwd->pw_dir);
	*tt = g_strdup_printf("Default: %s", buf);
	return (g_strcmp0(work_dir, buf));
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

int
check_edit_mode(char **tt)
{
	*tt = g_strdup("Default: hybrid");
	return (edit_mode != XT_EM_HYBRID);
}

char *
get_download_mode(struct settings *s)
{
	switch (download_mode) {
	case XT_DM_START:
		return (g_strdup("start"));
		break;
	case XT_DM_ASK:
		return (g_strdup("ask"));
		break;
	case XT_DM_ADD:
		return (g_strdup("add"));
		break;
	}
	return (g_strdup("unknown"));
}

int
set_download_mode(struct settings *s, char *val)
{
	if (val == NULL || strlen(val) == 0 || !strcmp(val, "start"))
		download_mode = XT_DM_START;
	else if (!strcmp(val, "ask"))
		download_mode = XT_DM_ASK;
	else if (!strcmp(val, "add"))
		download_mode = XT_DM_ADD;
	else
		return (1);

	return (0);
}

int
set_download_mode_rt(char *val)
{
	return (set_download_mode(NULL, val));
}

int
check_download_mode(char **tt)
{
	*tt = g_strdup("Default: start");
	return (download_mode != XT_DM_START);
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
	expand_tilde(work_dir, sizeof work_dir, val);
	return (0);
}

void
walk_cookie_wl(struct settings *s,
    void (*cb)(struct settings *, char *, void *), void *cb_args)
{
	struct wl_entry		*w;

	if (s == NULL || cb == NULL) {
		show_oops(NULL, "walk_cookie_wl invalid parameters");
		return;
	}

	TAILQ_FOREACH_REVERSE(w, &c_wl, wl_list, entry)
		cb(s, w->pat, cb_args);
}

void
walk_js_wl(struct settings *s,
    void (*cb)(struct settings *, char *, void *), void *cb_args)
{
	struct wl_entry		*w;

	if (s == NULL || cb == NULL) {
		show_oops(NULL, "walk_js_wl invalid parameters");
		return;
	}

	TAILQ_FOREACH_REVERSE(w, &js_wl, wl_list, entry)
		cb(s, w->pat, cb_args);
}

void
walk_pl_wl(struct settings *s,
    void (*cb)(struct settings *, char *, void *), void *cb_args)
{
	struct wl_entry		*w;

	if (s == NULL || cb == NULL) {
		show_oops(NULL, "walk_pl_wl invalid parameters");
		return;
	}

	TAILQ_FOREACH_REVERSE(w, &pl_wl, wl_list, entry)
		cb(s, w->pat, cb_args);
}

void
walk_force_https(struct settings *s,
    void (*cb)(struct settings *, char *, void *), void *cb_args)
{
	struct wl_entry		*w;

	if (s == NULL || cb == NULL) {
		show_oops(NULL, "walk_force_https invalid parameters");
		return;
	}

	TAILQ_FOREACH_REVERSE(w, &force_https, wl_list, entry)
		cb(s, w->pat, cb_args);
}

int
settings_add(char *var, char *val)
{
	int			i, rv, *p;
	int			tmp;
	double			*d;
	char			c[PATH_MAX], **s;
	const char		*errstr;

	/* get settings */
	for (i = 0, rv = 0; i < LENGTH(rs); i++) {
		errstr = NULL;

		if (strcmp(var, rs[i].name))
			continue;

		if (!strcmp(var, "include_config")) {
			expand_tilde(c, sizeof c, val);
			config_parse(c, 0);
			rv = 1;
			break;
		}

		if (rs[i].s) {
			if (rs[i].s->set(&rs[i], val))
				startpage_add("invalid value for %s: %s", var,
				    val);
			rv = 1;
			break;
		} else
			switch (rs[i].type) {
			case XT_S_INT:	/* FALLTHROUGH */
			case XT_S_BOOL:
				p = rs[i].ival;
				tmp = strtonum(val, INT_MIN, INT_MAX, &errstr);
				if (errstr)
					break;
				*p = tmp;
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
			case XT_S_DOUBLE:
				d = rs[i].dval;
				*d = g_ascii_strtod(val, NULL);
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
		snprintf(file, sizeof file, "%s" PS "%s",
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
			g_strstrip(val);
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
	char			*color;
	gchar			*tmp, *enc_val;
	struct settings_args	*sa = cb_args;

	if (sa == NULL || s->flags & XT_SF_INVISIBLE)
		return;

	if (s->flags & XT_SF_RUNTIME)
		color = "#22cc22";
	else
		color = "#cccccc";

	enc_val = html_escape(val);
	tmp = *sa->body;
	*sa->body = g_strdup_printf(
	    "%s\n<tr>"
	    "<td style='background-color: %s; width: 10%%;word-break:break-all'>%s</td>"
	    "<td style='background-color: %s; width: 20%%;word-break:break-all'>%s</td>",
	    *sa->body,
	    color,
	    s->name,
	    color,
	    enc_val == NULL ? "" : enc_val
	    );
	g_free(tmp);
	if (enc_val)
		g_free(enc_val);
	sa->i++;
}

void
print_runtime_setting(struct settings *s, char *val, void *cb_args)
{
	char			*tmp, *tt = NULL;
	struct settings_args	*sa = cb_args;
	int			modified = 0;
	int			i;

	if (sa == NULL)
		return;

	if (s == NULL || s->flags & XT_SF_INVISIBLE || s->activate == NULL)
		return;

	tmp = *sa->body;
	if (s->ismodified)
		modified = s->ismodified(&tt);
	*sa->body = g_strdup_printf(
	    "%s\n<tr %s>"
	    "<td title=\"%s\" style='width: 240px;'><div style='width: 100%%;'>%s</div></td>"
	    "<td title=\"%s\"><div style='width: 100%%; word-break:break-all'>",
	    *sa->body,
	    modified ? "id='modified'" : "",
	    s->tt ? s->tt : "",
	    s->name,
	    tt);
	g_free(tmp);
	g_free(tt);

	tmp = *sa->body;
	if (s->type == XT_S_STR && s->s && s->s->valid_options[0]) {
		*sa->body = g_strdup_printf("%s<select name='%s'>",
		    *sa->body,
		    s->name);
		for (i = 0; s->s->valid_options[i]; ++i) {
			g_free(tmp);
			tmp = *sa->body;
			*sa->body = g_strdup_printf("%s"
			    "<option value='%s' %s>%s</option>",
			    *sa->body,
			    s->s->valid_options[i],
			    !strcmp(s->s->valid_options[i], val) ? "selected" : "",
			    s->s->valid_options[i]);
		}
	} else if (s->type == XT_S_STR)
		*sa->body = g_strdup_printf(
		    "%s<input style='width: 98%%' type='text' name='%s' value='%s'>"
		    "</div></td></tr>",
		    *sa->body,
		    s->name,
		    val ? val : "");
	else if (s->type == XT_S_INT)
		*sa->body = g_strdup_printf(
		    "%s<input type='number' name='%s' value='%s'>"
		    "</div></td></tr>",
		    *sa->body,
		    s->name,
		    val);
	else if (s->type == XT_S_DOUBLE)
		*sa->body = g_strdup_printf(
		    "%s<input type='number' step='0.000001' name='%s' value='%s'>"
		    "</div></td></tr>",
		    *sa->body,
		    s->name,
		    val);
	else if (s->type == XT_S_BOOL)
		*sa->body = g_strdup_printf(
		    "%s<input type='hidden' name='%s' value='0'>"
		    "<input type='checkbox' name='%s' value='1''%s>"
		    "</div></td></tr>",
		    *sa->body,
		    s->name,
		    s->name,
		    atoi(val) ? " checked='checked'" : "");
	else {
		*sa->body = g_strdup(*sa->body);
		warnx("Did not print setting %s", s->name);
	}
	g_free(tmp);
	sa->i++;
}

char *
print_set_rejects(void)
{
	struct set_reject	*sr, *st;
	char			*body = NULL, *tmp;

	if (TAILQ_EMPTY(&srl))
		return (NULL);

	body = g_strdup_printf("The following settings were invalid and could "
	    "not be set:<br><div align='center'><table id='settings'>"
	    "<th align='left'>Setting</th>"
	    "<th align='left'>Value</th>");
	TAILQ_FOREACH_SAFE(sr, &srl, entry, st) {
		tmp = body;
		body = g_strdup_printf("%s\n<tr>"
		    "<td style='width:240px'><div>%s</div></td>"
		    "<td><div style='word-break:break-all'>%s</div></td></tr>",
		    body,
		    sr->name,
		    sr->value);
		g_free(tmp);
		TAILQ_REMOVE(&srl, sr, entry);
		g_free(sr->name);
		g_free(sr->value);
		g_free(sr);
	}
	tmp = body;
	body = g_strdup_printf("%s</table></div><br>", body);
	g_free(tmp);
	return (body);
}

int
xtp_page_set(struct tab *t, struct karg *args)
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

	load_webkit_string(t, page, XT_URI_ABOUT_SET, 0);

	g_free(page);

	return (XT_CB_PASSTHROUGH);
}

int
xtp_page_rt(struct tab *t, struct karg *args)
{
	char			*body = NULL, *page, *tmp;
	int			i = 1;
	struct settings_args	sa;

	generate_xtp_session_key(&t->session_key);

	bzero(&sa, sizeof sa);
	sa.body = &body;

	/* body */
	body = print_set_rejects();	

	tmp = body;
	body = g_strdup_printf("%s<div align='center'>"
	    "<form method='GET' action='%s%d/%s/%d'>"
	    "<input type='submit' value='Save Settings'>"
	    "<table id='settings'>"
	    "<th align='left'>Setting</th>"
	    "<th align='left'>Value</th>\n",
	    body ? body : "",
	    XT_XTP_STR,
	    XT_XTP_RT,
	    t->session_key,
	    XT_XTP_RT_SAVE);
	g_free(tmp);

	settings_walk(print_runtime_setting, &sa);
	i = sa.i;

	/* small message if there are none */
	if (i == 1) {
		tmp = body;
		body = g_strdup_printf("%s\n<tr><td style='text-align:center'"
		    "colspan='2'>No settings</td></tr>\n", body);
		g_free(tmp);
	}

	tmp = body;
	body = g_strdup_printf("%s</table>"
	    "<input type='submit' value='Save Settings'></form></div>", body);
	g_free(tmp);

	page = get_html_page("Runtime Settings", body, "", 1);

	g_free(body);

	load_webkit_string(t, page, XT_URI_ABOUT_RUNTIME, 0);

	g_free(page);

	return (XT_CB_PASSTHROUGH);
}

int
set(struct tab *t, struct karg *args)
{
	char			*p, *val;
	int			i;

	if (args == NULL || args->s == NULL)
		return (xtp_page_set(t, args));

	/* strip spaces */
	p = g_strstrip(args->s);

	if (strlen(p) == 0)
		return (xtp_page_set(t, args));

	/* we got some sort of string */
	val = g_strstr_len(p, strlen(p), "=");
	if (val) {
		*val++ = '\0';
		val = g_strstrip(val);
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
			case XT_S_INT:	/* FALLTHROUGH */
			case XT_S_BOOL:
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
			case XT_S_DOUBLE:
				if (rs[i].dval)
					show_oops(t, "%s = %f",
					    rs[i].name, *rs[i].dval);
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

