/* $xxxterm$ */
/*
 * Copyright (c) 2010, 2011 Marco Peereboom <marco@peereboom.us>
 * Copyright (c) 2011 Stevan Andjelkovic <stevan@student.chalmers.se>
 * Copyright (c) 2010 Edd Barrett <vext01@gmail.com>
 * Copyright (c) 2011 Todd T. Fries <todd@fries.net>
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
 *	favs
 *		- store in sqlite
 *	multi letter commands
 *	pre and post counts for commands
 *	autocompletion on various inputs
 *	create privacy browsing
 *		- encrypted local data
 */

#include <stdio.h>
#include <stdlib.h>
#include <err.h>
#include <pwd.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <dlfcn.h>
#include <errno.h>
#include <signal.h>
#include <libgen.h>
#include <ctype.h>

#include <sys/types.h>
#include <sys/wait.h>
#if defined(__linux__)
#include "linux/util.h"
#include "linux/tree.h"
#elif defined(__FreeBSD__)
#include <libutil.h>
#include "freebsd/util.h"
#include <sys/tree.h>
#else /* OpenBSD */
#include <util.h>
#include <sys/tree.h>
#endif
#include <sys/queue.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <webkit/webkit.h>
#include <libsoup/soup.h>
#include <gnutls/gnutls.h>
#include <JavaScriptCore/JavaScript.h>
#include <gnutls/x509.h>

#include "javascript.h"
/*

javascript.h borrowed from vimprobable2 under the following license:

Copyright (c) 2009 Leon Winter
Copyright (c) 2009 Hannes Schueller
Copyright (c) 2009 Matto Fransen

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

static char		*version = "$xxxterm$";

/* hooked functions */
void		(*_soup_cookie_jar_add_cookie)(SoupCookieJar *, SoupCookie *);
void		(*_soup_cookie_jar_delete_cookie)(SoupCookieJar *,
		    SoupCookie *);

/*#define XT_DEBUG*/
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
#define	XT_D_JS			0x0100
#define	XT_D_FAVORITE		0x0200
#define	XT_D_PRINTING		0x0400
#define	XT_D_COOKIE		0x0800
#define	XT_D_KEYBINDING		0x1000
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

char		*icons[] = {
	"xxxtermicon16.png",
	"xxxtermicon32.png",
	"xxxtermicon48.png",
	"xxxtermicon64.png",
	"xxxtermicon128.png"
};

struct tab {
	TAILQ_ENTRY(tab)	entry;
	GtkWidget		*vbox;
	GtkWidget		*tab_content;
	GtkWidget		*label;
	GtkWidget		*spinner;
	GtkWidget		*uri_entry;
	GtkWidget		*search_entry;
	GtkWidget		*toolbar;
	GtkWidget		*browser_win;
	GtkWidget		*statusbar;
	GtkWidget		*cmd;
	GtkWidget		*oops;
	GtkWidget		*backward;
	GtkWidget		*forward;
	GtkWidget		*stop;
	GtkWidget		*js_toggle;
	GtkEntryCompletion	*completion;
	guint			tab_id;
	WebKitWebView		*wv;

	WebKitWebHistoryItem		*item;
	WebKitWebBackForwardList	*bfl;

	/* favicon */
	WebKitNetworkRequest	*icon_request;
	WebKitDownload		*icon_download;
	GdkPixbuf		*icon_pixbuf;
	gchar			*icon_dest_uri;

	/* adjustments for browser */
	GtkScrollbar		*sb_h;
	GtkScrollbar		*sb_v;
	GtkAdjustment		*adjust_h;
	GtkAdjustment		*adjust_v;

	/* flags */
	int			focus_wv;
	int			ctrl_click;
	gchar			*status;
	int			xtp_meaning; /* identifies dls/favorites */

	/* hints */
	int			hints_on;
	int			hint_mode;
#define XT_HINT_NONE		(0)
#define XT_HINT_NUMERICAL	(1)
#define XT_HINT_ALPHANUM	(2)
	char			hint_buf[128];
	char			hint_num[128];

	/* search */
	char			*search_text;
	int			search_forward;

	/* settings */
	WebKitWebSettings	*settings;
	int			font_size;
	gchar			*user_agent;
};
TAILQ_HEAD(tab_list, tab);

struct history {
	RB_ENTRY(history)	entry;
	const gchar		*uri;
	const gchar		*title;
};
RB_HEAD(history_list, history);

struct download {
	RB_ENTRY(download)	entry;
	int			id;
	WebKitDownload		*download;
	struct tab		*tab;
};
RB_HEAD(download_list, download);

struct domain {
	RB_ENTRY(domain)	entry;
	gchar			*d;
	int			handy; /* app use */
};
RB_HEAD(domain_list, domain);

struct undo {
        TAILQ_ENTRY(undo)	entry;
        gchar			*uri;
	GList			*history;
	int			back; /* Keeps track of how many back
				       * history items there are. */
};
TAILQ_HEAD(undo_tailq, undo);

/* starts from 1 to catch atoi() failures when calling xtp_handle_dl() */
int				next_download_id = 1;

struct karg {
	int		i;
	char		*s;
};

/* defines */
#define XT_NAME			("XXXTerm")
#define XT_DIR			(".xxxterm")
#define XT_CACHE_DIR		("cache")
#define XT_CERT_DIR		("certs/")
#define XT_SESSIONS_DIR		("sessions/")
#define XT_CONF_FILE		("xxxterm.conf")
#define XT_FAVS_FILE		("favorites")
#define XT_SAVED_TABS_FILE	("main_session")
#define XT_RESTART_TABS_FILE	("restart_tabs")
#define XT_SOCKET_FILE		("socket")
#define XT_HISTORY_FILE		("history")
#define XT_SAVE_SESSION_ID	("SESSION_NAME=")
#define XT_CB_HANDLED		(TRUE)
#define XT_CB_PASSTHROUGH	(FALSE)
#define XT_DOCTYPE		"<!DOCTYPE html PUBLIC '-//W3C//DTD XHTML 1.0 Transitional//EN' 'http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd'>"
#define XT_HTML_TAG		"<html xmlns='http://www.w3.org/1999/xhtml'>"
#define XT_DLMAN_REFRESH	"10"
#define XT_PAGE_STYLE		"<style type='text/css'>\n"		\
				"td {overflow: hidden;"			\
				"  padding: 2px 2px 2px 2px;"		\
				"  border: 1px solid black}\n"		\
				"tr:hover {background: #ffff99 ;}\n"	\
				"th {background-color: #cccccc;"	\
				"  border: 1px solid black}"		\
				"table {border-spacing: 0; "		\
				"  width: 90%%;"			\
				"  border: 1px black solid;}\n"		\
				".progress-outer{"			\
				"  border: 1px solid black;"		\
				"  height: 8px;"			\
				"  width: 90%%;}"			\
				".progress-inner{"			\
				"  float: left;"			\
				"  height: 8px;"			\
				"  background: green;}"			\
				".dlstatus{"				\
				"  font-size: small;"			\
				"  text-align: center;}"		\
				"</style>\n\n"
#define XT_MAX_URL_LENGTH	(4096) /* 1 page is atomic, don't make bigger */
#define XT_MAX_UNDO_CLOSE_TAB	(32)
#define XT_RESERVED_CHARS	"$&+,/:;=?@ \"<>#%%{}|^~[]`"
#define XT_PRINT_EXTRA_MARGIN	10

/* file sizes */
#define SZ_KB		((uint64_t) 1024)
#define SZ_MB		(SZ_KB * SZ_KB)
#define SZ_GB		(SZ_KB * SZ_KB * SZ_KB)
#define SZ_TB		(SZ_KB * SZ_KB * SZ_KB * SZ_KB)

/*
 * xxxterm "protocol" (xtp)
 * We use this for managing stuff like downloads and favorites. They
 * make magical HTML pages in memory which have xxxt:// links in order
 * to communicate with xxxterm's internals. These links take the format:
 * xxxt://class/session_key/action/arg
 *
 * Don't begin xtp class/actions as 0. atoi returns that on error.
 *
 * Typically we have not put addition of items in this framework, as
 * adding items is either done via an ex-command or via a keybinding instead.
 */

#define XT_XTP_STR		"xxxt://"

/* XTP classes (xxxt://<class>) */
#define XT_XTP_DL		1	/* downloads */
#define XT_XTP_HL		2	/* history */
#define XT_XTP_CL		3	/* cookies */
#define XT_XTP_FL		4	/* favorites */

/* XTP download actions */
#define XT_XTP_DL_LIST		1
#define XT_XTP_DL_CANCEL	2
#define XT_XTP_DL_REMOVE	3

/* XTP history actions */
#define XT_XTP_HL_LIST		1
#define XT_XTP_HL_REMOVE	2

/* XTP cookie actions */
#define XT_XTP_CL_LIST		1
#define XT_XTP_CL_REMOVE	2

/* XTP cookie actions */
#define XT_XTP_FL_LIST		1
#define XT_XTP_FL_REMOVE	2

/* xtp tab meanings  - identifies which tabs have xtp pages in */
#define XT_XTP_TAB_MEANING_NORMAL	0 /* normal url */
#define XT_XTP_TAB_MEANING_DL		1 /* download manager in this tab */
#define XT_XTP_TAB_MEANING_FL		2 /* favorite manager in this tab */
#define XT_XTP_TAB_MEANING_HL		3 /* history manager in this tab */
#define XT_XTP_TAB_MEANING_CL		4 /* cookie manager in this tab */

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

#define XT_NAV_INVALID		(0)
#define XT_NAV_BACK		(1)
#define XT_NAV_FORWARD		(2)
#define XT_NAV_RELOAD		(3)
#define XT_NAV_RELOAD_CACHE	(4)

#define XT_FOCUS_INVALID	(0)
#define XT_FOCUS_URI		(1)
#define XT_FOCUS_SEARCH		(2)

#define XT_SEARCH_INVALID	(0)
#define XT_SEARCH_NEXT		(1)
#define XT_SEARCH_PREV		(2)

#define XT_PASTE_CURRENT_TAB	(0)
#define XT_PASTE_NEW_TAB	(1)

#define XT_FONT_SET		(0)

#define XT_URL_SHOW		(1)
#define XT_URL_HIDE		(2)

#define XT_STATUSBAR_SHOW	(1)
#define XT_STATUSBAR_HIDE	(2)

#define XT_WL_TOGGLE		(1<<0)
#define XT_WL_ENABLE		(1<<1)
#define XT_WL_DISABLE		(1<<2)
#define XT_WL_FQDN		(1<<3) /* default */
#define XT_WL_TOPLEVEL		(1<<4)

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

#define XT_COOKIE_NORMAL	(0)
#define XT_COOKIE_WHITELIST	(1)

/* mime types */
struct mime_type {
	char			*mt_type;
	char			*mt_action;
	int			mt_default;
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
int		browser_mode = XT_COOKIE_NORMAL;

/* runtime settings */
int		show_tabs = 1;	/* show tabs on notebook */
int		show_url = 1;	/* show url toolbar on notebook */
int		show_statusbar = 0; /* vimperator style status bar */
int		ctrl_click_focus = 0; /* ctrl click gets focus */
int		cookies_enabled = 1; /* enable cookies */
int		read_only_cookies = 0; /* enable to not write cookies */
int		enable_scripts = 1;
int		enable_plugins = 0;
int		default_font_size = 12;
int		window_height = 768;
int		window_width = 1024;
int		icon_size = 2; /* 1 = smallest, 2+ = bigger */
unsigned	refresh_interval = 10; /* download refresh interval */
int		enable_cookie_whitelist = 0;
int		enable_js_whitelist = 0;
time_t		session_timeout = 3600; /* cookie session timeout */
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
time_t		session_autosave = 0;
int		guess_search = 0;

struct settings;
struct key_binding;
int		set_download_dir(struct settings *, char *);
int		set_work_dir(struct settings *, char *);
int		set_runtime_dir(struct settings *, char *);
int		set_browser_mode(struct settings *, char *);
int		set_cookie_policy(struct settings *, char *);
int		add_alias(struct settings *, char *);
int		add_mime_type(struct settings *, char *);
int		add_cookie_wl(struct settings *, char *);
int		add_js_wl(struct settings *, char *);
int		add_kb(struct settings *, char *);
void		button_set_stockid(GtkWidget *, char *);
GtkWidget *	create_button(char *, char *, int);

char		*get_browser_mode(struct settings *);
char		*get_cookie_policy(struct settings *);

char		*get_download_dir(struct settings *);
char		*get_work_dir(struct settings *);
char		*get_runtime_dir(struct settings *);

void		walk_alias(struct settings *, void (*)(struct settings *, char *, void *), void *);
void		walk_cookie_wl(struct settings *, void (*)(struct settings *, char *, void *), void *);
void		walk_js_wl(struct settings *, void (*)(struct settings *, char *, void *), void *);
void		walk_kb(struct settings *, void (*)(struct settings *, char *, void *), void *);
void		walk_mime_type(struct settings *, void (*)(struct settings *, char *, void *), void *);

struct special {
	int		(*set)(struct settings *, char *);
	char		*(*get)(struct settings *);
	void		(*walk)(struct settings *, void (*cb)(struct settings *, char *, void *), void *);
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

struct settings {
	char		*name;
	int		type;
#define XT_S_INVALID	(0)
#define XT_S_INT	(1)
#define XT_S_STR	(2)
	uint32_t	flags;
#define XT_SF_RESTART	(1<<0)
#define XT_SF_RUNTIME	(1<<1)
	int		*ival;
	char		**sval;
	struct special	*s;
} rs[] = {
	{ "append_next",		XT_S_INT, 0,		&append_next, NULL, NULL },
	{ "allow_volatile_cookies",	XT_S_INT, 0,		&allow_volatile_cookies, NULL, NULL },
	{ "browser_mode",		XT_S_INT, 0, NULL, NULL,&s_browser_mode },
	{ "cookie_policy",		XT_S_INT, 0, NULL, NULL,&s_cookie },
	{ "cookies_enabled",		XT_S_INT, 0,		&cookies_enabled, NULL, NULL },
	{ "ctrl_click_focus",		XT_S_INT, 0,		&ctrl_click_focus, NULL, NULL },
	{ "default_font_size",		XT_S_INT, 0,		&default_font_size, NULL, NULL },
	{ "download_dir",		XT_S_STR, 0, NULL, NULL,&s_download_dir },
	{ "enable_cookie_whitelist",	XT_S_INT, 0,		&enable_cookie_whitelist, NULL, NULL },
	{ "enable_js_whitelist",	XT_S_INT, 0,		&enable_js_whitelist, NULL, NULL },
	{ "enable_plugins",		XT_S_INT, 0,		&enable_plugins, NULL, NULL },
	{ "enable_scripts",		XT_S_INT, 0,		&enable_scripts, NULL, NULL },
	{ "enable_socket",		XT_S_INT, XT_SF_RESTART,&enable_socket, NULL, NULL },
	{ "fancy_bar",			XT_S_INT, XT_SF_RESTART,&fancy_bar, NULL, NULL },
	{ "home",			XT_S_STR, 0, NULL,	&home, NULL },
	{ "http_proxy",			XT_S_STR, 0, NULL,	&http_proxy, NULL },
	{ "icon_size",			XT_S_INT, 0,		&icon_size, NULL, NULL },
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
	{ "guess_search",		XT_S_INT, 0,		&guess_search, NULL, NULL },
	{ "show_statusbar",		XT_S_INT, 0,		&show_statusbar, NULL, NULL },
	{ "ssl_ca_file",		XT_S_STR, 0, NULL,	&ssl_ca_file, NULL },
	{ "ssl_strict_certs",		XT_S_INT, 0,		&ssl_strict_certs, NULL, NULL },
	{ "user_agent",			XT_S_STR, 0, NULL,	&user_agent, NULL },
	{ "window_height",		XT_S_INT, 0,		&window_height, NULL, NULL },
	{ "window_width",		XT_S_INT, 0,		&window_width, NULL, NULL },
	{ "work_dir",			XT_S_STR, 0, NULL, NULL,&s_work_dir },

	/* runtime settings */
	{ "alias",			XT_S_STR, XT_SF_RUNTIME, NULL, NULL, &s_alias },
	{ "cookie_wl",			XT_S_STR, XT_SF_RUNTIME, NULL, NULL, &s_cookie_wl },
	{ "js_wl",			XT_S_STR, XT_SF_RUNTIME, NULL, NULL, &s_js },
	{ "keybinding",			XT_S_STR, XT_SF_RUNTIME, NULL, NULL, &s_kb },
	{ "mime_type",			XT_S_STR, XT_SF_RUNTIME, NULL, NULL, &s_mime },
};

int			about(struct tab *, struct karg *);
int			blank(struct tab *, struct karg *);
int			cookie_show_wl(struct tab *, struct karg *);
int			js_show_wl(struct tab *, struct karg *);
int			help(struct tab *, struct karg *);
int			set(struct tab *, struct karg *);
int			stats(struct tab *, struct karg *);
int			xtp_page_cl(struct tab *, struct karg *);
int			xtp_page_dl(struct tab *, struct karg *);
int			xtp_page_fl(struct tab *, struct karg *);
int			xtp_page_hl(struct tab *, struct karg *);

#define XT_URI_ABOUT		("about:")
#define XT_URI_ABOUT_LEN	(strlen(XT_URI_ABOUT))
#define XT_URI_ABOUT_ABOUT	("about")
#define XT_URI_ABOUT_BLANK	("blank")
#define XT_URI_ABOUT_CERTS	("certs")	/* XXX NOT YET */
#define XT_URI_ABOUT_COOKIEWL	("cookiewl")
#define XT_URI_ABOUT_COOKIEJAR	("cookiejar")
#define XT_URI_ABOUT_DOWNLOADS	("downloads")
#define XT_URI_ABOUT_FAVORITES	("favorites")
#define XT_URI_ABOUT_HELP	("help")
#define XT_URI_ABOUT_HISTORY	("history")
#define XT_URI_ABOUT_JSWL	("jswl")
#define XT_URI_ABOUT_SET	("set")
#define XT_URI_ABOUT_STATS	("stats")

struct about_type {
	char		*name;
	int		(*func)(struct tab *, struct karg *);
} about_list[] = {
	{ XT_URI_ABOUT_ABOUT, about },
	{ XT_URI_ABOUT_BLANK, blank },
	{ XT_URI_ABOUT_COOKIEWL, cookie_show_wl },
	{ XT_URI_ABOUT_COOKIEJAR, xtp_page_cl },
	{ XT_URI_ABOUT_DOWNLOADS, xtp_page_dl },
	{ XT_URI_ABOUT_FAVORITES, xtp_page_fl },
	{ XT_URI_ABOUT_HELP, help },
	{ XT_URI_ABOUT_HISTORY, xtp_page_hl },
	{ XT_URI_ABOUT_JSWL, js_show_wl },
	{ XT_URI_ABOUT_SET, set },
	{ XT_URI_ABOUT_STATS, stats },
};

/* globals */
extern char		*__progname;
char			**start_argv;
struct passwd		*pwd;
GtkWidget		*main_window;
GtkNotebook		*notebook;
GtkWidget		*arrow, *abtn;
struct tab_list		tabs;
struct history_list	hl;
struct download_list	downloads;
struct domain_list	c_wl;
struct domain_list	js_wl;
struct undo_tailq	undos;
struct keybinding_list	kbl;
int			undo_count;
int			updating_dl_tabs = 0;
int			updating_hl_tabs = 0;
int			updating_cl_tabs = 0;
int			updating_fl_tabs = 0;
char			*global_search;
uint64_t		blocked_cookies = 0;
char			named_session[PATH_MAX];
void			update_favicon(struct tab *);
int			icon_size_map(int);

GtkListStore		*completion_model;
void			completion_add(struct tab *);
void			completion_add_uri(const gchar *);

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

void
load_webkit_string(struct tab *t, const char *str, gchar *title)
{
	gchar			*uri;

	/* we set this to indicate we want to manually do navaction */
	if (t->bfl)
		t->item = webkit_web_back_forward_list_get_current_item(t->bfl);
	webkit_web_view_load_string(t->wv, str, NULL, NULL, NULL);

	if (title) {
		uri = g_strdup_printf("%s%s", XT_URI_ABOUT, title);
		gtk_entry_set_text(GTK_ENTRY(t->uri_entry), uri);
		g_free(uri);
	}
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
			t->status = g_strdup(gtk_entry_get_text(GTK_ENTRY(t->statusbar)));
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
	gtk_entry_set_text(GTK_ENTRY(t->statusbar), s);
	if (type)
		g_free(type);
}

void
hide_oops(struct tab *t)
{
	gtk_widget_hide(t->oops);
}

void
hide_cmd(struct tab *t)
{
	gtk_widget_hide(t->cmd);
}

void
show_cmd(struct tab *t)
{
	gtk_widget_hide(t->oops);
	gtk_widget_show(t->cmd);
}

void
show_oops(struct tab *t, const char *fmt, ...)
{
	va_list			ap;
	char			*msg;

	if (fmt == NULL)
		return;

	va_start(ap, fmt);
	if (vasprintf(&msg, fmt, ap) == -1)
		errx(1, "show_oops failed");
	va_end(ap);

	gtk_entry_set_text(GTK_ENTRY(t->oops), msg);
	gtk_widget_hide(t->cmd);
	gtk_widget_show(t->oops);
}

/* XXX collapse with show_oops */
void
show_oops_s(const char *fmt, ...)
{
	va_list			ap;
	char			*msg;
	struct tab		*ti, *t = NULL;

	if (fmt == NULL)
		return;

	TAILQ_FOREACH(ti, &tabs, entry)
		if (ti->tab_id == gtk_notebook_current_page(notebook)) {
			t = ti;
			break;
		}
	if (t == NULL)
		return;

	va_start(ap, fmt);
	if (vasprintf(&msg, fmt, ap) == -1)
		errx(1, "show_oops_s failed");
	va_end(ap);

	gtk_entry_set_text(GTK_ENTRY(t->oops), msg);
	gtk_widget_hide(t->cmd);
	gtk_widget_show(t->oops);
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
		browser_mode = XT_COOKIE_WHITELIST;
		allow_volatile_cookies	= 0;
		cookie_policy = SOUP_COOKIE_JAR_ACCEPT_NO_THIRD_PARTY;
		cookies_enabled = 1;
		enable_cookie_whitelist = 1;
		read_only_cookies = 0;
		save_rejected_cookies = 0;
		session_timeout = 3600;
		enable_scripts = 0;
		enable_js_whitelist = 1;
	} else if (!strcmp(val, "normal")) {
		browser_mode = XT_COOKIE_NORMAL;
		allow_volatile_cookies = 0;
		cookie_policy = SOUP_COOKIE_JAR_ACCEPT_ALWAYS;
		cookies_enabled = 1;
		enable_cookie_whitelist = 0;
		read_only_cookies = 0;
		save_rejected_cookies = 0;
		session_timeout = 3600;
		enable_scripts = 1;
		enable_js_whitelist = 0;
	} else
		return (1);

	return (0);
}

char *
get_browser_mode(struct settings *s)
{
	char			*r = NULL;

	if (browser_mode == XT_COOKIE_WHITELIST)
		r = g_strdup("whitelist");
	else if (browser_mode == XT_COOKIE_NORMAL)
		r = g_strdup("normal");
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

/*
 * Session IDs.
 * We use these to prevent people putting xxxt:// URLs on
 * websites in the wild. We generate 8 bytes and represent in hex (16 chars)
 */
#define XT_XTP_SES_KEY_SZ	8
#define XT_XTP_SES_KEY_HEX_FMT  \
	"%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx"
char			*dl_session_key;	/* downloads */
char			*hl_session_key;	/* history list */
char			*cl_session_key;	/* cookie list */
char			*fl_session_key;	/* favorites list */

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
void			create_new_tab(char *, struct undo *, int);
void			delete_tab(struct tab *);
void			adjustfont_webkit(struct tab *, int);
int			run_script(struct tab *, char *);
int			download_rb_cmp(struct download *, struct download *);

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

/*
 * generate a session key to secure xtp commands.
 * pass in a ptr to the key in question and it will
 * be modified in place.
 */
void
generate_xtp_session_key(char **key)
{
	uint8_t			rand_bytes[XT_XTP_SES_KEY_SZ];

	/* free old key */
	if (*key)
		g_free(*key);

	/* make a new one */
	arc4random_buf(rand_bytes, XT_XTP_SES_KEY_SZ);
	*key = g_strdup_printf(XT_XTP_SES_KEY_HEX_FMT,
	    rand_bytes[0], rand_bytes[1], rand_bytes[2], rand_bytes[3],
	    rand_bytes[4], rand_bytes[5], rand_bytes[6], rand_bytes[7]);

	DNPRINTF(XT_D_DOWNLOAD, "%s: new session key '%s'\n", __func__, *key);
}

/*
 * validate a xtp session key.
 * return 1 if OK
 */
int
validate_xtp_session_key(struct tab *t, char *trusted, char *untrusted)
{
	if (strcmp(trusted, untrusted) != 0) {
		show_oops(t, "%s: xtp session key mismatch possible spoof",
		    __func__);
		return (0);
	}

	return (1);
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
		show_oops_s("walk_alias invalid parameters");
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
		show_oops_s("match_alias: NULL URL");
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
			url_out = g_strdup(a->a_uri);
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

	url_out = match_alias(url_in);
	if (url_out != NULL)
		return (url_out);

	if (guess_search) {
		/*
		 * If there is no dot nor slash in the string and it isn't a
		 * path to a local file and doesn't resolves to an IP, assume
		 * that the user wants to search for the string.
		 */

		if (strchr(url_in, '.') == NULL &&
		    strchr(url_in, '/') == NULL &&
		    stat(url_in, &sb) != 0 &&
		    gethostbyname(url_in) == NULL) {

			enc_search = soup_uri_encode(url_in, XT_RESERVED_CHARS);
			url_out = g_strdup_printf(search_string, enc_search);
			g_free(enc_search);
			return (url_out);
		}
	}

	/* XXX not sure about this heuristic */
	if (stat(url_in, &sb) == 0)
		url_out = g_strdup_printf("file://%s", url_in);
	else
		url_out = g_strdup_printf("http://%s", url_in); /* guess http */

	DNPRINTF(XT_D_URL, "guess_url_type: guessed %s\n", url_out);

	return (url_out);
}

void
load_uri(struct tab *t, gchar *uri)
{
	struct karg	args;
	gchar		*newuri = NULL;
	int		i;
	char			file[PATH_MAX];
	GdkPixbuf		*pb;

	if (uri == NULL)
		return;

	/* Strip leading spaces. */
	while(*uri && isspace(*uri))
		uri++;

	if (strlen(uri) == 0) {
		blank(t, NULL);
		return;
	}

	if (!strncmp(uri, XT_URI_ABOUT, XT_URI_ABOUT_LEN)) {
		for (i = 0; i < LENGTH(about_list); i++)
			if (!strcmp(&uri[XT_URI_ABOUT_LEN], about_list[i].name)) {
				bzero(&args, sizeof args);
				about_list[i].func(t, &args);

				snprintf(file, sizeof file, "%s/%s",
				    resource_dir, icons[0]);
				pb = gdk_pixbuf_new_from_file(file, NULL);
				gtk_entry_set_icon_from_pixbuf(
				    GTK_ENTRY(t->uri_entry),
				    GTK_ENTRY_ICON_PRIMARY, pb);
				gdk_pixbuf_unref(pb);
				return;
			}
		show_oops(t, "invalid about page");
		return;
	}

	if (valid_url_type(uri)) {
		newuri = guess_url_type(uri);
		uri = newuri;
	}

	set_status(t, (char *)uri, XT_STATUS_LOADING);
	webkit_web_view_load_uri(t->wv, uri);

	if (newuri)
		g_free(newuri);
}

const gchar *
get_uri(WebKitWebView *wv)
{
	WebKitWebFrame		*frame;
	const gchar		*uri;

	frame = webkit_web_view_get_main_frame(wv);
	uri = webkit_web_frame_get_uri(frame);

	if (uri && strlen(uri) > 0)
		return (uri);
	else
		return (NULL);
}

int
add_alias(struct settings *s, char *line)
{
	char			*l, *alias;
	struct alias		*a = NULL;

	if (s == NULL || line == NULL) {
		show_oops_s("add_alias invalid parameters");
		return (1);
	}

	l = line;
	a = g_malloc(sizeof(*a));

	if ((alias = strsep(&l, " \t,")) == NULL || l == NULL) {
		show_oops_s("add_alias: incomplete alias definition");
		goto bad;
	}
	if (strlen(alias) == 0 || strlen(l) == 0) {
		show_oops_s("add_alias: invalid alias definition");
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

	/* XXX this could be smarter */

	if (line == NULL) {
		show_oops_s("add_mime_type invalid parameters");
		return (1);
	}

	l = line;
	m = g_malloc(sizeof(*m));

	if ((mime_type = strsep(&l, " \t,")) == NULL || l == NULL) {
		show_oops_s("add_mime_type: invalid mime_type");
		goto bad;
	}
	if (mime_type[strlen(mime_type) - 1] == '*') {
		mime_type[strlen(mime_type) - 1] = '\0';
		m->mt_default = 1;
	} else
		m->mt_default = 0;

	if (strlen(mime_type) == 0 || strlen(l) == 0) {
		show_oops_s("add_mime_type: invalid mime_type");
		goto bad;
	}

	m->mt_type = g_strdup(mime_type);
	m->mt_action = g_strdup(l);

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

	if (s == NULL || cb == NULL)
		show_oops_s("walk_mime_type invalid parameters");

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

	if (str == NULL || wl == NULL)
		return;
	if (strlen(str) < 2)
		return;

	DNPRINTF(XT_D_COOKIE, "wl_add in: %s\n", str);

	/* treat *.moo.com the same as .moo.com */
	if (str[0] == '*' && str[1] == '.')
		str = &str[1];
	else if (str[0] == '.')
		str = &str[0];
	else
		add_dot = 1;

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
		show_oops_s("walk_cookie_wl invalid parameters");
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
		show_oops_s("walk_js_wl invalid parameters");
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
		if (s[i] == '/' || s[i] == '\0') {
			ss = g_strdup(s);
			ss[i] = '\0';
			r = wl_find(ss, wl);
			g_free(ss);
			return (r);
		}

	return (NULL);
}

char *
get_toplevel_domain(char *domain)
{
	char			*s;
	int			found = 0;

	if (domain == NULL)
		return (NULL);
	if (strlen(domain) < 2)
		return (NULL);

	s = &domain[strlen(domain) - 1];
	while (s != domain) {
		if (*s == '.') {
			found++;
			if (found == 2)
				return (s);
		}
		s--;
	}

	if (found)
		return (domain);

	return (NULL);
}

int
settings_add(char *var, char *val)
{
	int i, rv, *p;
	char **s;

	/* get settings */
	for (i = 0, rv = 0; i < LENGTH(rs); i++) {
		if (strcmp(var, rs[i].name))
			continue;

		if (rs[i].s) {
			if (rs[i].s->set(&rs[i], val))
				errx(1, "invalid value for %s", var);
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
			case XT_S_INVALID:
			default:
				errx(1, "invalid type for %s", var);
			}
		break;
	}
	return (rv);
}

#define	WS	"\n= \t"
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
			errx(1, "invalid config file entry: %s", line);

		cp += (long)strspn(cp, WS);

		if ((val = strsep(&cp, "\0")) == NULL)
			break;

		DNPRINTF(XT_D_CONFIG, "config_parse: %s=%s\n",var ,val);
		handled = settings_add(var, val);
		if (handled == 0)
			errx(1, "invalid conf file entry: %s=%s", var, val);

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
	bzero(t->hint_buf, sizeof t->hint_buf);
	bzero(t->hint_num, sizeof t->hint_num);
	run_script(t, "vimprobable_clear()");
	t->hints_on = 0;
	t->hint_mode = XT_HINT_NONE;
}

void
enable_hints(struct tab *t)
{
	bzero(t->hint_buf, sizeof t->hint_buf);
	run_script(t, "vimprobable_show_hints()");
	t->hints_on = 1;
	t->hint_mode = XT_HINT_NONE;
}

#define XT_JS_OPEN	("open;")
#define XT_JS_OPEN_LEN	(strlen(XT_JS_OPEN))
#define XT_JS_FIRE	("fire;")
#define XT_JS_FIRE_LEN	(strlen(XT_JS_FIRE))
#define XT_JS_FOUND	("found;")
#define XT_JS_FOUND_LEN	(strlen(XT_JS_FOUND))

int
run_script(struct tab *t, char *s)
{
	JSGlobalContextRef	ctx;
	WebKitWebFrame		*frame;
	JSStringRef		str;
	JSValueRef		val, exception;
	char			*es, buf[128];

	DNPRINTF(XT_D_JS, "run_script: tab %d %s\n",
	    t->tab_id, s == (char *)JS_HINTING ? "JS_HINTING" : s);

	frame = webkit_web_view_get_main_frame(t->wv);
	ctx = webkit_web_frame_get_global_context(frame);

	str = JSStringCreateWithUTF8CString(s);
	val = JSEvaluateScript(ctx, str, JSContextGetGlobalObject(ctx),
	    NULL, 0, &exception);
	JSStringRelease(str);

	DNPRINTF(XT_D_JS, "run_script: val %p\n", val);
	if (val == NULL) {
		es = js_ref_to_string(ctx, exception);
		DNPRINTF(XT_D_JS, "run_script: exception %s\n", es);
		g_free(es);
		return (1);
	} else {
		es = js_ref_to_string(ctx, val);
		DNPRINTF(XT_D_JS, "run_script: val %s\n", es);

		/* handle return value right here */
		if (!strncmp(es, XT_JS_OPEN, XT_JS_OPEN_LEN)) {
			disable_hints(t);
			webkit_web_view_load_uri(t->wv, &es[XT_JS_OPEN_LEN]);
		}

		if (!strncmp(es, XT_JS_FIRE, XT_JS_FIRE_LEN)) {
			snprintf(buf, sizeof buf, "vimprobable_fire(%s)",
			    &es[XT_JS_FIRE_LEN]);
			run_script(t, buf);
			disable_hints(t);
		}

		if (!strncmp(es, XT_JS_FOUND, XT_JS_FOUND_LEN)) {
			if (atoi(&es[XT_JS_FOUND_LEN]) == 0)
				disable_hints(t);
		}

		g_free(es);
	}

	return (0);
}

int
hint(struct tab *t, struct karg *args)
{

	DNPRINTF(XT_D_JS, "hint: tab %d\n", t->tab_id);

	if (t->hints_on == 0)
		enable_hints(t);
	else
		disable_hints(t);

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

	snprintf(file, sizeof file, "%s/%s", work_dir, XT_HISTORY_FILE);

	if ((f = fopen(file, "r")) == NULL) {
		warnx("%s: fopen", __func__);
		return (1);
	}

	for (;;) {
		if ((uri = fparseln(f, NULL, NULL, NULL, 0)) == NULL)
			if (feof(f) || ferror(f))
				break;

		if ((title = fparseln(f, NULL, NULL, NULL, 0)) == NULL)
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

	snprintf(file, sizeof file, "%s/%s", sessions_dir, a->s);
	if ((f = fopen(file, "r")) == NULL)
		goto done;

	ti = TAILQ_LAST(&tabs, tab_list);

	for (;;) {
		if ((uri = fparseln(f, NULL, NULL, NULL, 0)) == NULL)
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
			create_new_tab(uri, NULL, 1);

		free(uri);
		uri = NULL;
	}

	/* close open tabs */
	if (a->i == XT_SES_CLOSETABS && ti != NULL) {
		for (;;) {
			tt = TAILQ_FIRST(&tabs);
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
	struct tab		*ti;
	const gchar		*uri;
	int			len = 0, i;
	const gchar		**arr = NULL;

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

	/* save tabs, in the order they are arranged in the notebook */
	TAILQ_FOREACH(ti, &tabs, entry)
		len++;

	arr = g_malloc0(len * sizeof(gchar *));

	TAILQ_FOREACH(ti, &tabs, entry) {
		if ((uri = get_uri(ti->wv)) != NULL)
			arr[gtk_notebook_page_num(notebook, ti->vbox)] = uri;
	}

	for (i = 0; i < len; i++)
		if (arr[i])
			fprintf(f, "%s\n", arr[i]);

	g_free(arr);
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
yank_uri(struct tab *t, struct karg *args)
{
	const gchar		*uri;
	GtkClipboard		*clipboard;

	if ((uri = get_uri(t->wv)) == NULL)
		return (1);

	clipboard = gtk_clipboard_get(GDK_SELECTION_PRIMARY);
	gtk_clipboard_set_text(clipboard, uri, -1);

	return (0);
}

struct paste_args {
	struct tab	*t;
	int		i;
};

void
paste_uri_cb(GtkClipboard *clipboard, const gchar *text, gpointer data)
{
	struct paste_args	*pap;

	if (data == NULL || text == NULL || !strlen(text))
		return;

	pap = (struct paste_args *)data;

	switch(pap->i) {
	case XT_PASTE_CURRENT_TAB:
		load_uri(pap->t, (gchar *)text);
		break;
	case XT_PASTE_NEW_TAB:
		create_new_tab((gchar *)text, NULL, 1);
		break;
	}

	g_free(pap);
}

int
paste_uri(struct tab *t, struct karg *args)
{
	GtkClipboard		*clipboard;
	struct paste_args	*pap;

	pap = g_malloc(sizeof(struct paste_args));

	pap->t = t;
	pap->i = args->i;

	clipboard = gtk_clipboard_get(GDK_SELECTION_PRIMARY);
	gtk_clipboard_request_text(clipboard, paste_uri_cb, pap);

	return (0);
}

char *
find_domain(const gchar *s, int add_dot)
{
	int			i;
	char			*r = NULL, *ss = NULL;

	if (s == NULL)
		return (NULL);

	if (!strncmp(s, "http://", strlen("http://")))
		s = &s[strlen("http://")];
	else if (!strncmp(s, "https://", strlen("https://")))
		s = &s[strlen("https://")];

	if (strlen(s) < 2)
		return (NULL);

	ss = g_strdup(s);
	for (i = 0; i < strlen(ss) + 1 /* yes er need this */; i++)
		/* chop string at first slash */
		if (ss[i] == '/' || ss[i] == '\0') {
			ss[i] = '\0';
			if (add_dot)
				r = g_strdup_printf(".%s", ss);
			else
				r = g_strdup(ss);
			break;
		}
	g_free(ss);

	return (r);
}

int
toggle_cwl(struct tab *t, struct karg *args)
{
	struct domain		*d;
	const gchar		*uri;
	char			*dom = NULL, *dom_toggle = NULL;
	int			es;

	if (args == NULL)
		return (1);

	uri = get_uri(t->wv);
	dom = find_domain(uri, 1);
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

	if (args->i & XT_WL_TOPLEVEL)
		dom_toggle = get_toplevel_domain(dom);
	else
		dom_toggle = dom;

	if (es)
		/* enable cookies for domain */
		wl_add(dom_toggle, &c_wl, 0);
	else
		/* disable cookies for domain */
		RB_REMOVE(domain_list, &c_wl, d);

	webkit_web_view_reload(t->wv);

	g_free(dom);
	return (0);
}

int
toggle_js(struct tab *t, struct karg *args)
{
	int			es;
	const gchar		*uri;
	struct domain		*d;
	char			*dom = NULL, *dom_toggle = NULL;

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

	uri = get_uri(t->wv);
	dom = find_domain(uri, 1);

	if (uri == NULL || dom == NULL) {
		show_oops(t, "Can't toggle domain in JavaScript white list");
		goto done;
	}

	if (args->i & XT_WL_TOPLEVEL)
		dom_toggle = get_toplevel_domain(dom);
	else
		dom_toggle = dom;

	if (es) {
		button_set_stockid(t->js_toggle, GTK_STOCK_MEDIA_PLAY);
		wl_add(dom_toggle, &js_wl, 0 /* session */);
	} else {
		d = wl_find(dom_toggle, &js_wl);
		if (d)
			RB_REMOVE(domain_list, &js_wl, d);
		button_set_stockid(t->js_toggle, GTK_STOCK_MEDIA_PAUSE);
	}
	g_object_set(G_OBJECT(t->settings),
	    "enable-scripts", es, (char *)NULL);
	webkit_web_view_set_settings(t->wv, t->settings);
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

	a.i = XT_WL_TOGGLE | XT_WL_FQDN;
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
stats(struct tab *t, struct karg *args)
{
	char			*stats, *s, line[64 * 1024];
	uint64_t		line_count = 0;
	FILE			*r_cookie_f;

	if (t == NULL)
		show_oops_s("stats invalid parameters");

	line[0] = '\0';
	if (save_rejected_cookies) {
		if ((r_cookie_f = fopen(rc_fname, "r"))) {
			for (;;) {
				s = fgets(line, sizeof line, r_cookie_f);
				if (s == NULL || feof(r_cookie_f) ||
				    ferror(r_cookie_f))
					break;
				line_count++;
			}
			fclose(r_cookie_f);
			snprintf(line, sizeof line,
			    "<br>Cookies blocked(*) total: %llu", line_count);
		} else
			show_oops(t, "Can't open blocked cookies file: %s",
			    strerror(errno));
	}

	stats = g_strdup_printf(XT_DOCTYPE
	    "<html>"
	    "<head>"
	    "<title>Statistics</title>"
	    "</head>"
	    "<h1>Statistics</h1>"
	    "<body>"
	    "Cookies blocked(*) this session: %llu"
	    "%s"
	    "<p><small><b>*</b> results vary based on settings"
	    "</body>"
	    "</html>",
	   blocked_cookies,
	   line);

	load_webkit_string(t, stats, XT_URI_ABOUT_STATS);
	g_free(stats);

	return (0);
}

int
blank(struct tab *t, struct karg *args)
{
	if (t == NULL)
		show_oops_s("about invalid parameters");

	load_webkit_string(t, "", XT_URI_ABOUT_BLANK);

	return (0);
}
int
about(struct tab *t, struct karg *args)
{
	char			*about;

	if (t == NULL)
		show_oops_s("about invalid parameters");

	about = g_strdup_printf(XT_DOCTYPE
	    "<html>"
	    "<head>"
	    "<title>About</title>"
	    "</head>"
	    "<h1>About</h1>"
	    "<body>"
	    "<b>Version: %s</b><p>"
	    "Authors:"
	    "<ul>"
	    "<li>Marco Peereboom &lt;marco@peereboom.us&gt;</li>"
	    "<li>Stevan Andjelkovic &lt;stevan@student.chalmers.se&gt;</li>"
	    "<li>Edd Barrett &lt;vext01@gmail.com&gt; </li>"
	    "<li>Todd T. Fries &lt;todd@fries.net&gt; </li>"
	    "</ul>"
	    "Copyrights and licenses can be found on the XXXterm "
	    "<a href=\"http://opensource.conformal.com/wiki/XXXTerm\">website</a>"
	    "</body>"
	    "</html>",
	    version
	    );

	load_webkit_string(t, about, XT_URI_ABOUT_ABOUT);
	g_free(about);

	return (0);
}

int
help(struct tab *t, struct karg *args)
{
	char			*help;

	if (t == NULL)
		show_oops_s("help invalid parameters");

	help = XT_DOCTYPE
	    "<html>"
	    "<head>"
	    "<title>XXXterm</title>"
	    "<meta http-equiv=\"REFRESH\" content=\"0;"
	        "url=http://opensource.conformal.com/cgi-bin/man-cgi?xxxterm\">"
	    "</head>"
	    "<body>"
	    "XXXterm man page <a href=\"http://opensource.conformal.com/"
	        "cgi-bin/man-cgi?xxxterm\">http://opensource.conformal.com/"
		"cgi-bin/man-cgi?xxxterm</a>"
	    "</body>"
	    "</html>"
	    ;

	load_webkit_string(t, help, XT_URI_ABOUT_HELP);

	return (0);
}

/*
 * update all favorite tabs apart from one. Pass NULL if
 * you want to update all.
 */
void
update_favorite_tabs(struct tab *apart_from)
{
	struct tab			*t;
	if (!updating_fl_tabs) {
		updating_fl_tabs = 1; /* stop infinite recursion */
		TAILQ_FOREACH(t, &tabs, entry)
			if ((t->xtp_meaning == XT_XTP_TAB_MEANING_FL)
			    && (t != apart_from))
				xtp_page_fl(t, NULL);
		updating_fl_tabs = 0;
	}
}

/* show a list of favorites (bookmarks) */
int
xtp_page_fl(struct tab *t, struct karg *args)
{
	char			file[PATH_MAX];
	FILE			*f;
	char			*uri = NULL, *title = NULL;
	size_t			len, lineno = 0;
	int			i, failed = 0;
	char			*header, *body, *tmp, *html = NULL;

	DNPRINTF(XT_D_FAVORITE, "%s:", __func__);

	if (t == NULL)
		warn("%s: bad param", __func__);

	/* mark tab as favorite list */
	t->xtp_meaning = XT_XTP_TAB_MEANING_FL;

	/* new session key */
	if (!updating_fl_tabs)
		generate_xtp_session_key(&fl_session_key);

	/* open favorites */
	snprintf(file, sizeof file, "%s/%s", work_dir, XT_FAVS_FILE);
	if ((f = fopen(file, "r")) == NULL) {
		show_oops(t, "Can't open favorites file: %s", strerror(errno));
		return (1);
	}

	/* header */
	header = g_strdup_printf(XT_DOCTYPE XT_HTML_TAG "\n<head>"
	    "<title>Favorites</title>\n"
	    "%s"
	    "</head>"
	    "<h1>Favorites</h1>\n",
	    XT_PAGE_STYLE);

	/* body */
	body = g_strdup_printf("<div align='center'><table><tr>"
	    "<th style='width: 4%%'>&#35;</th><th>Link</th>"
	    "<th style='width: 15%%'>Remove</th></tr>\n");

	for (i = 1;;) {
		if ((title = fparseln(f, &len, &lineno, NULL, 0)) == NULL)
			if (feof(f) || ferror(f))
				break;
		if (len == 0) {
			free(title);
			title = NULL;
			continue;
		}

		if ((uri = fparseln(f, &len, &lineno, NULL, 0)) == NULL)
			if (feof(f) || ferror(f)) {
				show_oops(t, "favorites file corrupt");
				failed = 1;
				break;
			}

		tmp = body;
		body = g_strdup_printf("%s<tr>"
		    "<td>%d</td>"
		    "<td><a href='%s'>%s</a></td>"
		    "<td style='text-align: center'>"
		    "<a href='%s%d/%s/%d/%d'>X</a></td>"
		    "</tr>\n",
		    body, i, uri, title,
		    XT_XTP_STR, XT_XTP_FL, fl_session_key, XT_XTP_FL_REMOVE, i);

		g_free(tmp);

		free(uri);
		uri = NULL;
		free(title);
		title = NULL;
		i++;
	}
	fclose(f);

	/* if none, say so */
	if (i == 1) {
		tmp = body;
		body = g_strdup_printf("%s<tr>"
		    "<td colspan='3' style='text-align: center'>"
		    "No favorites - To add one use the 'favadd' command."
		    "</td></tr>", body);
		g_free(tmp);
	}

	if (uri)
		free(uri);
	if (title)
		free(title);

	/* render */
	if (!failed) {
		html = g_strdup_printf("%s%s</table></div></html>",
		    header, body);
		load_webkit_string(t, html, XT_URI_ABOUT_FAVORITES);
	}

	update_favorite_tabs(t);

	if (header)
		g_free(header);
	if (body)
		g_free(body);
	if (html)
		g_free(html);

	return (failed);
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

void
show_certs(struct tab *t, gnutls_x509_crt_t *certs,
    size_t cert_count, char *title)
{
	gnutls_datum_t		cinfo;
	char			*tmp, *header, *body, *footer;
	int			i;

	header = g_strdup_printf("<title>%s</title><html><body>", title);
	footer = g_strdup("</body></html>");
	body = g_strdup("");

	for (i = 0; i < cert_count; i++) {
		if (gnutls_x509_crt_print(certs[i], GNUTLS_CRT_PRINT_FULL,
		    &cinfo))
			return;

		tmp = body;
		body = g_strdup_printf("%s<h2>Cert #%d</h2><pre>%s</pre>",
		    body, i, cinfo.data);
		gnutls_free(cinfo.data);
		g_free(tmp);
	}

	tmp = g_strdup_printf("%s%s%s", header, body, footer);
	g_free(header);
	g_free(body);
	g_free(footer);
	load_webkit_string(t, tmp, XT_URI_ABOUT_CERTS);
	g_free(tmp);
}

int
ca_cmd(struct tab *t, struct karg *args)
{
	FILE			*f = NULL;
	int			rv = 1, certs = 0, certs_read;
	struct stat		sb;
	gnutls_datum		dt;
	gnutls_x509_crt_t	*c = NULL;
	char			*certs_buf = NULL, *s;

	/* yeah yeah stat race */
	if (stat(ssl_ca_file, &sb)) {
		show_oops(t, "no CA file: %s", ssl_ca_file);
		goto done;
	}

	if ((f = fopen(ssl_ca_file, "r")) == NULL) {
		show_oops(t, "Can't open CA file: %s", strerror(errno));
		return (1);
	}

	certs_buf = g_malloc(sb.st_size + 1);
	if (fread(certs_buf, 1, sb.st_size, f) != sb.st_size) {
		show_oops(t, "Can't read CA file: %s", strerror(errno));
		goto done;
	}
	certs_buf[sb.st_size] = '\0';

	s = certs_buf;
	while ((s = strstr(s, "BEGIN CERTIFICATE"))) {
		certs++;
		s += strlen("BEGIN CERTIFICATE");
	}

	bzero(&dt, sizeof dt);
	dt.data = certs_buf;
	dt.size = sb.st_size;
	c = g_malloc(sizeof(gnutls_x509_crt_t) * certs);
	certs_read = gnutls_x509_crt_list_import(c, &certs, &dt,
	    GNUTLS_X509_FMT_PEM, 0);
	if (certs_read <= 0) {
		show_oops(t, "No cert(s) available");
		goto done;
	}
	show_certs(t, c, certs_read, "Certificate Authority Certificates");
done:
	if (c)
		g_free(c);
	if (certs_buf)
		g_free(certs_buf);
	if (f)
		fclose(f);

	return (rv);
}

int
connect_socket_from_uri(const gchar *uri, char *domain, size_t domain_sz)
{
	SoupURI			*su = NULL;
	struct addrinfo		hints, *res = NULL, *ai;
	int			s = -1, on;
	char			port[8];

	if (uri && !g_str_has_prefix(uri, "https://"))
		goto done;

	su = soup_uri_new(uri);
	if (su == NULL)
		goto done;
	if (!SOUP_URI_VALID_FOR_HTTP(su))
		goto done;

	snprintf(port, sizeof port, "%d", su->port);
	bzero(&hints, sizeof(struct addrinfo));
	hints.ai_flags = AI_CANONNAME;
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if (getaddrinfo(su->host, port, &hints, &res))
		goto done;

	for (ai = res; ai; ai = ai->ai_next) {
		if (ai->ai_family != AF_INET && ai->ai_family != AF_INET6)
			continue;

		s = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
		if (s < 0)
			goto done;
		if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &on,
			    sizeof(on)) == -1)
			goto done;

		if (connect(s, ai->ai_addr, ai->ai_addrlen) < 0)
			goto done;
	}

	if (domain)
		strlcpy(domain, su->host, domain_sz);
done:
	if (su)
		soup_uri_free(su);
	if (res)
		freeaddrinfo(res);

	return (s);
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
start_tls(struct tab *t, int s, gnutls_session_t *gs,
    gnutls_certificate_credentials_t *xc)
{
	gnutls_certificate_credentials_t xcred;
	gnutls_session_t	gsession;
	int			rv = 1;

	if (gs == NULL || xc == NULL)
		goto done;

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
		show_oops(t, "gnutls_handshake failed %d fatal %d %s",
		    rv,
		    gnutls_error_is_fatal(rv),
		    gnutls_strerror_name(rv));
		stop_tls(gsession, xcred);
		goto done;
	}

	gnutls_credentials_type_t cred;
	cred = gnutls_auth_get_type(gsession);
	if (cred != GNUTLS_CRD_CERTIFICATE) {
		stop_tls(gsession, xcred);
		goto done;
	}

	*gs = gsession;
	*xc = xcred;
	rv = 0;
done:
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
	gtk_widget_modify_base(t->statusbar, GTK_STATE_NORMAL, &color);
	gdk_color_parse("black", &color);
	gtk_widget_modify_text(t->statusbar, GTK_STATE_NORMAL, &color);
done:
	fclose(f);
}

int
load_compare_cert(struct tab *t, struct karg *args)
{
	const gchar		*uri;
	char			domain[8182], file[PATH_MAX];
	char			cert_buf[64 * 1024], r_cert_buf[64 * 1024];
	int			s = -1, rv = 1, i;
	size_t			cert_count;
	FILE			*f = NULL;
	size_t			cert_buf_sz;
	gnutls_session_t	gsession;
	gnutls_x509_crt_t	*certs;
	gnutls_certificate_credentials_t xcred;

	if (t == NULL)
		return (1);

	if ((uri = get_uri(t->wv)) == NULL)
		return (1);

	if ((s = connect_socket_from_uri(uri, domain, sizeof domain)) == -1)
		return (1);

	/* go ssl/tls */
	if (start_tls(t, s, &gsession, &xcred)) {
		show_oops(t, "Start TLS failed");
		goto done;
	}

	/* get certs */
	if (get_connection_certs(gsession, &certs, &cert_count)) {
		show_oops(t, "Can't get connection certificates");
		goto done;
	}

	snprintf(file, sizeof file, "%s/%s", certs_dir, domain);
	if ((f = fopen(file, "r")) == NULL)
		goto freeit;

	for (i = 0; i < cert_count; i++) {
		cert_buf_sz = sizeof cert_buf;
		if (gnutls_x509_crt_export(certs[i], GNUTLS_X509_FMT_PEM,
		    cert_buf, &cert_buf_sz)) {
			goto freeit;
		}
		if (fread(r_cert_buf, cert_buf_sz, 1, f) != 1) {
			rv = -1; /* critical */
			goto freeit;
		}
		if (bcmp(r_cert_buf, cert_buf, sizeof cert_buf_sz)) {
			rv = -1; /* critical */
			goto freeit;
		}
	}

	rv = 0;
freeit:
	if (f)
		fclose(f);
	free_connection_certs(certs, cert_count);
done:
	/* we close the socket first for speed */
	if (s != -1)
		close(s);
	stop_tls(gsession, xcred);

	return (rv);
}

int
cert_cmd(struct tab *t, struct karg *args)
{
	const gchar		*uri;
	char			*action, domain[8182];
	int			s = -1;
	size_t			cert_count;
	gnutls_session_t	gsession;
	gnutls_x509_crt_t	*certs;
	gnutls_certificate_credentials_t xcred;

	if (t == NULL)
		return (1);

	if ((action = getparams(args->s, "cert")))
		;
	else
		action = "show";

	if ((uri = get_uri(t->wv)) == NULL) {
		show_oops(t, "Invalid URI");
		return (1);
	}

	if ((s = connect_socket_from_uri(uri, domain, sizeof domain)) == -1) {
		show_oops(t, "Invalid certidicate URI: %s", uri);
		return (1);
	}

	/* go ssl/tls */
	if (start_tls(t, s, &gsession, &xcred)) {
		show_oops(t, "Start TLS failed");
		goto done;
	}

	/* get certs */
	if (get_connection_certs(gsession, &certs, &cert_count)) {
		show_oops(t, "get_connection_certs failed");
		goto done;
	}

	if (!strcmp(action, "show"))
		show_certs(t, certs, cert_count, "Certificate Chain");
	else if (!strcmp(action, "save"))
		save_certs(t, certs, cert_count, domain);
	else
		show_oops(t, "Invalid command: %s", action);

	free_connection_certs(certs, cert_count);
done:
	/* we close the socket first for speed */
	if (s != -1)
		close(s);
	stop_tls(gsession, xcred);

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
wl_show(struct tab *t, char *args, char *title, struct domain_list *wl)
{
	struct domain		*d;
	char			*tmp, *header, *body, *footer;
	int			p_js = 0, s_js = 0;

	/* we set this to indicate we want to manually do navaction */
	t->item = webkit_web_back_forward_list_get_current_item(t->bfl);

	if (g_str_has_prefix(args, "show a") ||
	    !strcmp(args, "show")) {
		/* show all */
		p_js = 1;
		s_js = 1;
	} else if (g_str_has_prefix(args, "show p")) {
		/* show persistent */
		p_js = 1;
	} else if (g_str_has_prefix(args, "show s")) {
		/* show session */
		s_js = 1;
	} else
		return (1);

	header = g_strdup_printf("<title>%s</title><html><body><h1>%s</h1>",
	    title, title);
	footer = g_strdup("</body></html>");
	body = g_strdup("");

	/* p list */
	if (p_js) {
		tmp = body;
		body = g_strdup_printf("%s<h2>Persistent</h2>", body);
		g_free(tmp);
		RB_FOREACH(d, domain_list, wl) {
			if (d->handy == 0)
				continue;
			tmp = body;
			body = g_strdup_printf("%s%s<br>", body, d->d);
			g_free(tmp);
		}
	}

	/* s list */
	if (s_js) {
		tmp = body;
		body = g_strdup_printf("%s<h2>Session</h2>", body);
		g_free(tmp);
		RB_FOREACH(d, domain_list, wl) {
			if (d->handy == 1)
				continue;
			tmp = body;
			body = g_strdup_printf("%s%s<br>", body, d->d);
			g_free(tmp);
		}
	}

	tmp = g_strdup_printf("%s%s%s", header, body, footer);
	g_free(header);
	g_free(body);
	g_free(footer);
	if (wl == &js_wl)
		load_webkit_string(t, tmp, XT_URI_ABOUT_JSWL);
	else
		load_webkit_string(t, tmp, XT_URI_ABOUT_COOKIEWL);
	g_free(tmp);
	return (0);
}

int
wl_save(struct tab *t, struct karg *args, int js)
{
	char			file[PATH_MAX];
	FILE			*f;
	char			*line = NULL, *lt = NULL;
	size_t			linelen;
	const gchar		*uri;
	char			*dom = NULL, *dom_save = NULL;
	struct karg		a;
	struct domain		*d;
	GSList			*cf;
	SoupCookie		*ci, *c;
	int			flags;

	if (t == NULL || args == NULL)
		return (1);

	if (runtime_settings[0] == '\0')
		return (1);

	snprintf(file, sizeof file, "%s/%s", work_dir, runtime_settings);
	if ((f = fopen(file, "r+")) == NULL)
		return (1);

	uri = get_uri(t->wv);
	dom = find_domain(uri, 1);
	if (uri == NULL || dom == NULL) {
		show_oops(t, "Can't add domain to %s white list",
		  js ? "JavaScript" : "cookie");
		goto done;
	}

	if (g_str_has_prefix(args->s, "save d")) {
		/* save domain */
		if ((dom_save = get_toplevel_domain(dom)) == NULL) {
			show_oops(t, "invalid domain: %s", dom);
			goto done;
		}
		flags = XT_WL_TOPLEVEL;
	} else if (g_str_has_prefix(args->s, "save f") ||
	    !strcmp(args->s, "save")) {
		/* save fqdn */
		dom_save = dom;
		flags = XT_WL_FQDN;
	} else {
		show_oops(t, "invalid command: %s", args->s);
		goto done;
	}

	lt = g_strdup_printf("%s=%s", js ? "js_wl" : "cookie_wl", dom_save);

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
	a.i |= flags;
	if (js) {
		d = wl_find(dom_save, &js_wl);
		if (!d) {
			settings_add("js_wl", dom_save);
			d = wl_find(dom_save, &js_wl);
		}
		toggle_js(t, &a);
	} else {
		d = wl_find(dom_save, &c_wl);
		if (!d) {
			settings_add("cookie_wl", dom_save);
			d = wl_find(dom_save, &c_wl);
		}
		toggle_cwl(t, &a);

		/* find and add to persistent jar */
		cf = soup_cookie_jar_all_cookies(s_cookiejar);
		for (;cf; cf = cf->next) {
			ci = cf->data;
			if (!strcmp(dom_save, ci->domain) ||
			    !strcmp(&dom_save[1], ci->domain)) /* deal with leading . */ {
				c = soup_cookie_copy(ci);
				_soup_cookie_jar_add_cookie(p_cookiejar, c);
			}
		}
		soup_cookies_free(cf);
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
	wl_show(t, "show all", "JavaScript White List", &js_wl);

	return (0);
}

int
cookie_show_wl(struct tab *t, struct karg *args)
{
	wl_show(t, "show all", "Cookie White List", &c_wl);

	return (0);
}

int
cookie_cmd(struct tab *t, struct karg *args)
{
	char			*cmd;
	struct karg		a;

	if ((cmd = getparams(args->s, "cookie")))
		;
	else
		cmd = "show all";


	if (g_str_has_prefix(cmd, "show")) {
		wl_show(t, cmd, "Cookie White List", &c_wl);
	} else if (g_str_has_prefix(cmd, "save")) {
		a.s = cmd;
		wl_save(t, &a, 0);
	} else if (g_str_has_prefix(cmd, "toggle")) {
		a.i = XT_WL_TOGGLE;
		if (g_str_has_prefix(cmd, "toggle d"))
			a.i |= XT_WL_TOPLEVEL;
		else
			a.i |= XT_WL_FQDN;
		toggle_cwl(t, &a);
	} else if (g_str_has_prefix(cmd, "delete")) {
		show_oops(t, "'cookie delete' currently unimplemented");
	} else
		show_oops(t, "unknown cookie command: %s", cmd);

	return (0);
}

int
js_cmd(struct tab *t, struct karg *args)
{
	char			*cmd;
	struct karg		a;

	if ((cmd = getparams(args->s, "js")))
		;
	else
		cmd = "show all";

	if (g_str_has_prefix(cmd, "show")) {
		wl_show(t, cmd, "JavaScript White List", &js_wl);
	} else if (g_str_has_prefix(cmd, "save")) {
		a.s = cmd;
		wl_save(t, &a, 1);
	} else if (g_str_has_prefix(cmd, "toggle")) {
		a.i = XT_WL_TOGGLE;
		if (g_str_has_prefix(cmd, "toggle d"))
			a.i |= XT_WL_TOPLEVEL;
		else
			a.i |= XT_WL_FQDN;
		toggle_js(t, &a);
	} else if (g_str_has_prefix(cmd, "delete")) {
		show_oops(t, "'js delete' currently unimplemented");
	} else
		show_oops(t, "unknown js command: %s", cmd);

	return (0);
}

int
add_favorite(struct tab *t, struct karg *args)
{
	char			file[PATH_MAX];
	FILE			*f;
	char			*line = NULL;
	size_t			urilen, linelen;
	const gchar		*uri, *title;

	if (t == NULL)
		return (1);

	/* don't allow adding of xtp pages to favorites */
	if (t->xtp_meaning != XT_XTP_TAB_MEANING_NORMAL) {
		show_oops(t, "%s: can't add xtp pages to favorites", __func__);
		return (1);
	}

	snprintf(file, sizeof file, "%s/%s", work_dir, XT_FAVS_FILE);
	if ((f = fopen(file, "r+")) == NULL) {
		show_oops(t, "Can't open favorites file: %s", strerror(errno));
		return (1);
	}

	title = webkit_web_view_get_title(t->wv);
	uri = get_uri(t->wv);

	if (title == NULL)
		title = uri;

	if (title == NULL || uri == NULL) {
		show_oops(t, "can't add page to favorites");
		goto done;
	}

	urilen = strlen(uri);

	for (;;) {
		if ((line = fparseln(f, &linelen, NULL, NULL, 0)) == NULL)
			if (feof(f) || ferror(f))
				break;

		if (linelen == urilen && !strcmp(line, uri))
			goto done;

		free(line);
		line = NULL;
	}

	fprintf(f, "\n%s\n%s", title, uri);
done:
	if (line)
		free(line);
	fclose(f);

	update_favorite_tabs(NULL);

	return (0);
}

int
navaction(struct tab *t, struct karg *args)
{
	WebKitWebHistoryItem	*item;

	DNPRINTF(XT_D_NAV, "navaction: tab %d opcode %d\n",
	    t->tab_id, args->i);

	if (t->item) {
		if (args->i == XT_NAV_BACK)
			item = webkit_web_back_forward_list_get_current_item(t->bfl);
		else
			item = webkit_web_back_forward_list_get_forward_item(t->bfl);
		if (item == NULL)
			return (XT_CB_PASSTHROUGH);
		webkit_web_view_load_uri(t->wv, webkit_web_history_item_get_uri(item));
		t->item = NULL;
		return (XT_CB_PASSTHROUGH);
	}

	switch (args->i) {
	case XT_NAV_BACK:
		webkit_web_view_go_back(t->wv);
		break;
	case XT_NAV_FORWARD:
		webkit_web_view_go_forward(t->wv);
		break;
	case XT_NAV_RELOAD:
		webkit_web_view_reload(t->wv);
		break;
	case XT_NAV_RELOAD_CACHE:
		webkit_web_view_reload_bypass_cache(t->wv);
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
	case XT_MOVE_HALFDOWN:
	case XT_MOVE_HALFUP:
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

	TAILQ_FOREACH(t, &tabs, entry) {
		if (show_url == 0) {
			gtk_widget_hide(t->toolbar);
			focus_webview(t);
		} else
			gtk_widget_show(t->toolbar);
	}
}

void
notebook_tab_set_visibility(GtkNotebook *notebook)
{
	if (show_tabs == 0)
		gtk_notebook_set_show_tabs(notebook, FALSE);
	else
		gtk_notebook_set_show_tabs(notebook, TRUE);
}

void
statusbar_set_visibility(void)
{
	struct tab		*t;

	TAILQ_FOREACH(t, &tabs, entry) {
		if (show_statusbar == 0) {
			gtk_widget_hide(t->statusbar);
			focus_webview(t);
		} else
			gtk_widget_show(t->statusbar);
	}
}

void
url_set(struct tab *t, int enable_url_entry)
{
	GdkPixbuf	*pixbuf;
	int		progress;

	show_url = enable_url_entry;

	if (enable_url_entry) {
		gtk_entry_set_icon_from_icon_name(GTK_ENTRY(t->statusbar),
		    GTK_ENTRY_ICON_PRIMARY, NULL);
		gtk_entry_set_progress_fraction(GTK_ENTRY(t->statusbar), 0);
	} else {
		pixbuf = gtk_entry_get_icon_pixbuf(GTK_ENTRY(t->uri_entry),
		    GTK_ENTRY_ICON_PRIMARY);
		progress =
		    gtk_entry_get_progress_fraction(GTK_ENTRY(t->uri_entry));
		gtk_entry_set_icon_from_pixbuf(GTK_ENTRY(t->statusbar),
		    GTK_ENTRY_ICON_PRIMARY, pixbuf);
		gtk_entry_set_progress_fraction(GTK_ENTRY(t->statusbar),
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
	notebook_tab_set_visibility(notebook);

	return (XT_CB_HANDLED);
}

int
statusaction(struct tab *t, struct karg *args)
{
	int			rv = XT_CB_HANDLED;

	DNPRINTF(XT_D_TAB, "%s: %p %d\n", __func__, t, args->i);

	if (t == NULL)
		return (XT_CB_PASSTHROUGH);

	switch (args->i) {
	case XT_STATUSBAR_SHOW:
		if (show_statusbar == 0) {
			show_statusbar = 1;
			statusbar_set_visibility();
		}
		break;
	case XT_STATUSBAR_HIDE:
		if (show_statusbar == 1) {
			show_statusbar = 0;
			statusbar_set_visibility();
		}
		break;
	}
	return (rv);
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
	char			*url = NULL;
	struct undo		*u;

	DNPRINTF(XT_D_TAB, "tabaction: %p %d\n", t, args->i);

	if (t == NULL)
		return (XT_CB_PASSTHROUGH);

	switch (args->i) {
	case XT_TAB_NEW:
		if ((url = getparams(args->s, "tabnew")))
			create_new_tab(url, NULL, 1);
		else
			create_new_tab(NULL, NULL, 1);
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
		load_uri(t, url);
		break;
	case XT_TAB_SHOW:
		if (show_tabs == 0) {
			show_tabs = 1;
			notebook_tab_set_visibility(notebook);
		}
		break;
	case XT_TAB_HIDE:
		if (show_tabs == 1) {
			show_tabs = 0;
			notebook_tab_set_visibility(notebook);
		}
		break;
	case XT_TAB_UNDO_CLOSE:
		if (undo_count == 0) {
			DNPRINTF(XT_D_TAB, "%s: no tabs to undo close", __func__);
			goto done;
		} else {
			undo_count--;
			u = TAILQ_FIRST(&undos);
			create_new_tab(u->uri, u, 1);

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
		show_oops_s("resizetab invalid parameters");
		return (XT_CB_PASSTHROUGH);
	}

	DNPRINTF(XT_D_TAB, "resizetab: tab %d %d\n",
	    t->tab_id, args->i);

	adjustfont_webkit(t, args->i);

	return (XT_CB_HANDLED);
}

int
movetab(struct tab *t, struct karg *args)
{
	struct tab		*tt;
	int			x;

	if (t == NULL || args == NULL) {
		show_oops_s("movetab invalid parameters");
		return (XT_CB_PASSTHROUGH);
	}

	DNPRINTF(XT_D_TAB, "movetab: tab %d opcode %d\n",
	    t->tab_id, args->i);

	if (args->i == XT_TAB_INVALID)
		return (XT_CB_PASSTHROUGH);

	if (args->i < XT_TAB_INVALID) {
		/* next or previous tab */
		if (TAILQ_EMPTY(&tabs))
			return (XT_CB_PASSTHROUGH);

		switch (args->i) {
		case XT_TAB_NEXT:
			/* if at the last page, loop around to the first */
			if (gtk_notebook_get_current_page(notebook) ==
			    gtk_notebook_get_n_pages(notebook) - 1)
				gtk_notebook_set_current_page(notebook, 0);
			else
				gtk_notebook_next_page(notebook);
			break;
		case XT_TAB_PREV:
			/* if at the first page, loop around to the last */
			if (gtk_notebook_current_page(notebook) == 0)
				gtk_notebook_set_current_page(notebook,
				    gtk_notebook_get_n_pages(notebook) - 1);
			else
				gtk_notebook_prev_page(notebook);
			break;
		case XT_TAB_FIRST:
			gtk_notebook_set_current_page(notebook, 0);
			break;
		case XT_TAB_LAST:
			gtk_notebook_set_current_page(notebook, -1);
			break;
		default:
			return (XT_CB_PASSTHROUGH);
		}

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
				focus_webview(tt);
		}
	}

	return (XT_CB_HANDLED);
}

int
command(struct tab *t, struct karg *args)
{
	char			*s = NULL, *ss = NULL;
	GdkColor		color;
	const gchar		*uri;

	if (t == NULL || args == NULL) {
		show_oops_s("command invalid parameters");
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
		s = ":";
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
		if ((uri = get_uri(t->wv)) != NULL) {
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
	gdk_color_parse("white", &color);
	gtk_widget_modify_base(t->cmd, GTK_STATE_NORMAL, &color);
	show_cmd(t);
	gtk_widget_grab_focus(GTK_WIDGET(t->cmd));
	gtk_editable_set_position(GTK_EDITABLE(t->cmd), -1);

	if (ss)
		g_free(ss);

	return (XT_CB_HANDLED);
}

/*
 * Return a new string with a download row (in html)
 * appended. Old string is freed.
 */
char *
xtp_page_dl_row(struct tab *t, char *html, struct download *dl)
{

	WebKitDownloadStatus	stat;
	char			*status_html = NULL, *cmd_html = NULL, *new_html;
	gdouble			progress;
	char			cur_sz[FMT_SCALED_STRSIZE];
	char			tot_sz[FMT_SCALED_STRSIZE];
	char			*xtp_prefix;

	DNPRINTF(XT_D_DOWNLOAD, "%s: dl->id %d\n", __func__, dl->id);

	/* All actions wil take this form:
	 * xxxt://class/seskey
	 */
	xtp_prefix = g_strdup_printf("%s%d/%s/",
	    XT_XTP_STR, XT_XTP_DL, dl_session_key);

	stat = webkit_download_get_status(dl->download);

	switch (stat) {
	case WEBKIT_DOWNLOAD_STATUS_FINISHED:
		status_html = g_strdup_printf("Finished");
		cmd_html = g_strdup_printf(
		    "<a href='%s%d/%d'>Remove</a>",
		    xtp_prefix, XT_XTP_DL_REMOVE, dl->id);
		break;
	case WEBKIT_DOWNLOAD_STATUS_STARTED:
		/* gather size info */
		progress = 100 * webkit_download_get_progress(dl->download);

		fmt_scaled(
		    webkit_download_get_current_size(dl->download), cur_sz);
		fmt_scaled(
		    webkit_download_get_total_size(dl->download), tot_sz);

		status_html = g_strdup_printf(
		    "<div style='width: 100%%' align='center'>"
		    "<div class='progress-outer'>"
		    "<div class='progress-inner' style='width: %.2f%%'>"
		    "</div></div></div>"
		    "<div class='dlstatus'>%s of %s (%.2f%%)</div>",
		    progress, cur_sz, tot_sz, progress);

		cmd_html = g_strdup_printf("<a href='%s%d/%d'>Cancel</a>",
		    xtp_prefix, XT_XTP_DL_CANCEL, dl->id);

		break;
		/* LLL */
	case WEBKIT_DOWNLOAD_STATUS_CANCELLED:
		status_html = g_strdup_printf("Cancelled");
		cmd_html = g_strdup_printf("<a href='%s%d/%d'>Remove</a>",
		    xtp_prefix, XT_XTP_DL_REMOVE, dl->id);
		break;
	case WEBKIT_DOWNLOAD_STATUS_ERROR:
		status_html = g_strdup_printf("Error!");
		cmd_html = g_strdup_printf("<a href='%s%d/%d'>Remove</a>",
		    xtp_prefix, XT_XTP_DL_REMOVE, dl->id);
		break;
	case WEBKIT_DOWNLOAD_STATUS_CREATED:
		cmd_html = g_strdup_printf("<a href='%s%d/%d'>Cancel</a>",
		    xtp_prefix, XT_XTP_DL_CANCEL, dl->id);
		status_html = g_strdup_printf("Starting");
		break;
	default:
		show_oops(t, "%s: unknown download status", __func__);
	};

	new_html = g_strdup_printf(
	    "%s\n<tr><td>%s</td><td>%s</td>"
	    "<td style='text-align:center'>%s</td></tr>\n",
	    html, basename(webkit_download_get_uri(dl->download)),
	    status_html, cmd_html);
	g_free(html);

	if (status_html)
		g_free(status_html);

	if (cmd_html)
		g_free(cmd_html);

	g_free(xtp_prefix);

	return new_html;
}

/*
 * update all download tabs apart from one. Pass NULL if
 * you want to update all.
 */
void
update_download_tabs(struct tab *apart_from)
{
	struct tab			*t;
	if (!updating_dl_tabs) {
		updating_dl_tabs = 1; /* stop infinite recursion */
		TAILQ_FOREACH(t, &tabs, entry)
			if ((t->xtp_meaning == XT_XTP_TAB_MEANING_DL)
			    && (t != apart_from))
				xtp_page_dl(t, NULL);
		updating_dl_tabs = 0;
	}
}

/*
 * update all cookie tabs apart from one. Pass NULL if
 * you want to update all.
 */
void
update_cookie_tabs(struct tab *apart_from)
{
	struct tab			*t;
	if (!updating_cl_tabs) {
		updating_cl_tabs = 1; /* stop infinite recursion */
		TAILQ_FOREACH(t, &tabs, entry)
			if ((t->xtp_meaning == XT_XTP_TAB_MEANING_CL)
			    && (t != apart_from))
				xtp_page_cl(t, NULL);
		updating_cl_tabs = 0;
	}
}

/*
 * update all history tabs apart from one. Pass NULL if
 * you want to update all.
 */
void
update_history_tabs(struct tab *apart_from)
{
	struct tab			*t;

	if (!updating_hl_tabs) {
		updating_hl_tabs = 1; /* stop infinite recursion */
		TAILQ_FOREACH(t, &tabs, entry)
			if ((t->xtp_meaning == XT_XTP_TAB_MEANING_HL)
			    && (t != apart_from))
				xtp_page_hl(t, NULL);
		updating_hl_tabs = 0;
	}
}

/* cookie management XTP page */
int
xtp_page_cl(struct tab *t, struct karg *args)
{
	char			*header, *body, *footer, *page, *tmp;
	int			i = 1; /* all ids start 1 */
	GSList			*sc, *pc, *pc_start;
	SoupCookie		*c;
	char			*type, *table_headers;
	char			*last_domain = strdup("");

	DNPRINTF(XT_D_CMD, "%s", __func__);

	if (t == NULL) {
		show_oops_s("%s invalid parameters", __func__);
		return (1);
	}
	/* mark this tab as cookie jar */
	t->xtp_meaning = XT_XTP_TAB_MEANING_CL;

	/* Generate a new session key */
	if (!updating_cl_tabs)
		generate_xtp_session_key(&cl_session_key);

	/* header */
	header = g_strdup_printf(XT_DOCTYPE XT_HTML_TAG
	  "\n<head><title>Cookie Jar</title>\n" XT_PAGE_STYLE
	  "</head><body><h1>Cookie Jar</h1>\n");

	/* table headers */
	table_headers = g_strdup_printf("<div align='center'><table><tr>"
	    "<th>Type</th>"
	    "<th>Name</th>"
	    "<th>Value</th>"
	    "<th>Path</th>"
	    "<th>Expires</th>"
	    "<th>Secure</th>"
	    "<th>HTTP<br />only</th>"
	    "<th>Rm</th></tr>\n");

	sc = soup_cookie_jar_all_cookies(s_cookiejar);
	pc = soup_cookie_jar_all_cookies(p_cookiejar);
	pc_start = pc;

	body = NULL;
	for (; sc; sc = sc->next) {
		c = sc->data;

                if (strcmp(last_domain, c->domain) != 0) {
                        /* new domain */
                        free(last_domain);
                        last_domain = strdup(c->domain);

                        if (body != NULL) {
                                tmp = body;
                                body = g_strdup_printf("%s</table></div>"
                                    "<h2>%s</h2>%s\n",
                                    body, c->domain, table_headers);
                                g_free(tmp);
                        } else {
                                /* first domain */
                                body = g_strdup_printf("<h2>%s</h2>%s\n",
                                    c->domain, table_headers);
                        }
                }

		type = "Session";
		for (pc = pc_start; pc; pc = pc->next)
			if (soup_cookie_equal(pc->data, c)) {
				type = "Session + Persistent";
				break;
			}

		tmp = body;
		body = g_strdup_printf(
		    "%s\n<tr>"
		    "<td style='width: text-align: center'>%s</td>"
		    "<td style='width: 1px'>%s</td>"
		    "<td style='width=70%%;overflow: visible'>"
		    "  <textarea rows='4'>%s</textarea>"
		    "</td>"
		    "<td>%s</td>"
		    "<td>%s</td>"
		    "<td style='width: 1px; text-align: center'>%d</td>"
		    "<td style='width: 1px; text-align: center'>%d</td>"
		    "<td style='width: 1px; text-align: center'>"
		    "<a href='%s%d/%s/%d/%d'>X</a></td></tr>\n",
		    body,
		    type,
		    c->name,
		    c->value,
		    c->path,
		    c->expires ?
		        soup_date_to_string(c->expires, SOUP_DATE_COOKIE) : "",
		    c->secure,
		    c->http_only,

		    XT_XTP_STR,
		    XT_XTP_CL,
		    cl_session_key,
		    XT_XTP_CL_REMOVE,
		    i
		    );

		g_free(tmp);
		i++;
	}

	soup_cookies_free(sc);
	soup_cookies_free(pc);

	/* small message if there are none */
	if (i == 1) {
		body = g_strdup_printf("%s\n<tr><td style='text-align:center'"
		    "colspan='8'>No Cookies</td></tr>\n", table_headers);
	}

	/* footer */
	footer = g_strdup_printf("</table></div></body></html>");

	page = g_strdup_printf("%s%s%s", header, body, footer);

	g_free(header);
	g_free(body);
	g_free(footer);
	g_free(table_headers);
	g_free(last_domain);

	load_webkit_string(t, page, XT_URI_ABOUT_COOKIEJAR);
	update_cookie_tabs(t);

	g_free(page);

	return (0);
}

int
xtp_page_hl(struct tab *t, struct karg *args)
{
	char			*header, *body, *footer, *page, *tmp;
	struct history		*h;
	int			i = 1; /* all ids start 1 */

	DNPRINTF(XT_D_CMD, "%s", __func__);

	if (t == NULL) {
		show_oops_s("%s invalid parameters", __func__);
		return (1);
	}

	/* mark this tab as history manager */
	t->xtp_meaning = XT_XTP_TAB_MEANING_HL;

	/* Generate a new session key */
	if (!updating_hl_tabs)
		generate_xtp_session_key(&hl_session_key);

	/* header */
	header = g_strdup_printf(XT_DOCTYPE XT_HTML_TAG "\n<head>"
	    "<title>History</title>\n"
	    "%s"
	    "</head>"
	    "<h1>History</h1>\n",
	    XT_PAGE_STYLE);

	/* body */
	body = g_strdup_printf("<div align='center'><table><tr>"
	    "<th>URI</th><th>Title</th><th style='width: 15%%'>Remove</th></tr>\n");

	RB_FOREACH_REVERSE(h, history_list, &hl) {
		tmp = body;
		body = g_strdup_printf(
		    "%s\n<tr>"
		    "<td><a href='%s'>%s</a></td>"
		    "<td>%s</td>"
		    "<td style='text-align: center'>"
		    "<a href='%s%d/%s/%d/%d'>X</a></td></tr>\n",
		    body, h->uri, h->uri, h->title,
		    XT_XTP_STR, XT_XTP_HL, hl_session_key,
		    XT_XTP_HL_REMOVE, i);

		g_free(tmp);
		i++;
	}

	/* small message if there are none */
	if (i == 1) {
		tmp = body;
		body = g_strdup_printf("%s\n<tr><td style='text-align:center'"
		    "colspan='3'>No History</td></tr>\n", body);
		g_free(tmp);
	}

	/* footer */
	footer = g_strdup_printf("</table></div></body></html>");

	page = g_strdup_printf("%s%s%s", header, body, footer);

	/*
	 * update all history manager tabs as the xtp session
	 * key has now changed. No need to update the current tab.
	 * Already did that above.
	 */
	update_history_tabs(t);

	g_free(header);
	g_free(body);
	g_free(footer);

	load_webkit_string(t, page, XT_URI_ABOUT_HISTORY);
	g_free(page);

	return (0);
}

/*
 * Generate a web page detailing the status of any downloads
 */
int
xtp_page_dl(struct tab *t, struct karg *args)
{
	struct download		*dl;
	char			*header, *body, *footer, *page, *tmp;
	char			*ref;
	int			n_dl = 1;

	DNPRINTF(XT_D_DOWNLOAD, "%s", __func__);

	if (t == NULL) {
		show_oops_s("%s invalid parameters", __func__);
		return (1);
	}
	/* mark as a download manager tab */
	t->xtp_meaning = XT_XTP_TAB_MEANING_DL;

	/*
	 * Generate a new session key for next page instance.
	 * This only happens for the top level call to xtp_page_dl()
	 * in which case updating_dl_tabs is 0.
	 */
	if (!updating_dl_tabs)
		generate_xtp_session_key(&dl_session_key);

	/* header - with refresh so as to update */
	if (refresh_interval >= 1)
		ref = g_strdup_printf(
		    "<meta http-equiv='refresh' content='%u"
		    ";url=%s%d/%s/%d' />\n",
		    refresh_interval,
		    XT_XTP_STR,
		    XT_XTP_DL,
		    dl_session_key,
		    XT_XTP_DL_LIST);
		else
			ref = g_strdup("");


	header = g_strdup_printf(
	    "%s\n<head>"
	    "<title>Downloads</title>\n%s%s</head>\n",
	    XT_DOCTYPE XT_HTML_TAG,
	    ref,
	    XT_PAGE_STYLE);

	body = g_strdup_printf("<body><h1>Downloads</h1><div align='center'>"
	    "<p>\n<a href='%s%d/%s/%d'>\n[ Refresh Downloads ]</a>\n"
	    "</p><table><tr><th style='width: 60%%'>"
	    "File</th>\n<th>Progress</th><th>Command</th></tr>\n",
	    XT_XTP_STR, XT_XTP_DL, dl_session_key, XT_XTP_DL_LIST);

	RB_FOREACH_REVERSE(dl, download_list, &downloads) {
		body = xtp_page_dl_row(t, body, dl);
		n_dl++;
	}

	/* message if no downloads in list */
	if (n_dl == 1) {
		tmp = body;
		body = g_strdup_printf("%s\n<tr><td colspan='3'"
		    " style='text-align: center'>"
		    "No downloads</td></tr>\n", body);
		g_free(tmp);
	}

	/* footer */
	footer = g_strdup_printf("</table></div></body></html>");

	page = g_strdup_printf("%s%s%s", header, body, footer);


	/*
	 * update all download manager tabs as the xtp session
	 * key has now changed. No need to update the current tab.
	 * Already did that above.
	 */
	update_download_tabs(t);

	g_free(ref);
	g_free(header);
	g_free(body);
	g_free(footer);

	load_webkit_string(t, page, XT_URI_ABOUT_DOWNLOADS);
	g_free(page);

	return (0);
}

int
search(struct tab *t, struct karg *args)
{
	gboolean		d;

	if (t == NULL || args == NULL) {
		show_oops_s("search invalid parameters");
		return (1);
	}
	if (t->search_text == NULL) {
		if (global_search == NULL)
			return (XT_CB_PASSTHROUGH);
		else {
			t->search_text = g_strdup(global_search);
			webkit_web_view_mark_text_matches(t->wv, global_search, FALSE, 0);
			webkit_web_view_set_highlight_text_matches(t->wv, TRUE);
		}
	}

	DNPRINTF(XT_D_CMD, "search: tab %d opc %d forw %d text %s\n",
	    t->tab_id, args->i, t->search_forward, t->search_text);

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
	    "<td style='background-color: %s; width: 10%%; word-break: break-all'>%s</td>"
	    "<td style='background-color: %s; width: 20%%; word-break: break-all'>%s</td>",
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
set(struct tab *t, struct karg *args)
{
	char			*header, *body, *footer, *page, *tmp, *pars;
	int			i = 1;
	struct settings_args	sa;

	if ((pars = getparams(args->s, "set")) == NULL) {
		bzero(&sa, sizeof sa);
		sa.body = &body;

		/* header */
		header = g_strdup_printf(XT_DOCTYPE XT_HTML_TAG
		  "\n<head><title>Settings</title>\n"
		  "</head><body><h1>Settings</h1>\n");

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

		/* footer */
		footer = g_strdup_printf("</table></div></body></html>");

		page = g_strdup_printf("%s%s%s", header, body, footer);

		g_free(header);
		g_free(body);
		g_free(footer);

		load_webkit_string(t, page, XT_URI_ABOUT_SET);
	} else
		show_oops(t, "Invalid command: %s", pars);

	return (XT_CB_PASSTHROUGH);
}

int
session_save(struct tab *t, char *filename, char **ret)
{
	struct karg		a;
	char			*f = filename;
	int			rv = 1;

	f += strlen("save");
	while (*f == ' ' && *f != '\0')
		f++;
	if (strlen(f) == 0)
		goto done;

	*ret = f;
	if (f[0] == '.' || f[0] == '/')
		goto done;

	a.s = f;
	if (save_tabs(t, &a))
		goto done;
	strlcpy(named_session, f, sizeof named_session);

	rv = 0;
done:
	return (rv);
}

int
session_open(struct tab *t, char *filename, char **ret)
{
	struct karg		a;
	char			*f = filename;
	int			rv = 1;

	f += strlen("open");
	while (*f == ' ' && *f != '\0')
		f++;
	if (strlen(f) == 0)
		goto done;

	*ret = f;
	if (f[0] == '.' || f[0] == '/')
		goto done;

	a.s = f;
	a.i = XT_SES_CLOSETABS;
	if (open_tabs(t, &a))
		goto done;

	strlcpy(named_session, f, sizeof named_session);

	rv = 0;
done:
	return (rv);
}

int
session_delete(struct tab *t, char *filename, char **ret)
{
	char			file[PATH_MAX];
	char			*f = filename;
	int			rv = 1;

	f += strlen("delete");
	while (*f == ' ' && *f != '\0')
		f++;
	if (strlen(f) == 0)
		goto done;

	*ret = f;
	if (f[0] == '.' || f[0] == '/')
		goto done;

	snprintf(file, sizeof file, "%s/%s", sessions_dir, f);
	if (unlink(file))
		goto done;

	if (!strcmp(f, named_session))
		strlcpy(named_session, XT_SAVED_TABS_FILE,
		    sizeof named_session);

	rv = 0;
done:
	return (rv);
}

int
session_cmd(struct tab *t, struct karg *args)
{
	char			*action = NULL;
	char			*filename = NULL;

	if (t == NULL)
		return (1);

	if ((action = getparams(args->s, "session")))
		;
	else
		action = "show";

	if (!strcmp(action, "show"))
		show_oops(t, "Current session: %s", named_session[0] == '\0' ?
		    XT_SAVED_TABS_FILE : named_session);
	else if (g_str_has_prefix(action, "save ")) {
		if (session_save(t, action, &filename)) {
			show_oops(t, "Can't save session: %s",
			    filename ? filename : "INVALID");
			goto done;
		}
	} else if (g_str_has_prefix(action, "open ")) {
		if (session_open(t, action, &filename)) {
			show_oops(t, "Can't open session: %s",
			    filename ? filename : "INVALID");
			goto done;
		}
	} else if (g_str_has_prefix(action, "delete ")) {
		if (session_delete(t, action, &filename)) {
			show_oops(t, "Can't delete session: %s",
			    filename ? filename : "INVALID");
			goto done;
		}
	} else
		show_oops(t, "Invalid command: %s", action);
done:
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
		show_oops_s("can't print: %s", g_err->message);
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
	char				*name;
	guint				mask;
	guint				use_in_entry;
	guint				key;
	int				(*func)(struct tab *, struct karg *);
	struct karg			arg;
	TAILQ_ENTRY(key_binding)	entry;	/* in bss so no need to init */
} keys[] = {
	{ "cookiejar",		MOD1,	0,	GDK_j,		xtp_page_cl,	{0} },
	{ "downloadmgr",	MOD1,	0,	GDK_d,		xtp_page_dl,	{0} },
	{ "history",		MOD1,	0,	GDK_h,		xtp_page_hl,	{0} },
	{ "print",		CTRL,	0,	GDK_p,		print_page,	{0}},
	{ NULL,			0,	0,	GDK_slash,	command,	{.i = '/'} },
	{ NULL,			0,	0,	GDK_question,	command,	{.i = '?'} },
	{ NULL,			0,	0,	GDK_colon,	command,	{.i = ':'} },
	{ "quit",		CTRL,	0,	GDK_q,		quit,		{0} },
	{ "restart",		MOD1,	0,	GDK_q,		restart,	{0} },
	{ "togglejs",		CTRL,	0,	GDK_j,		toggle_js,	{.i = XT_WL_TOGGLE | XT_WL_FQDN} },
	{ "togglecookie",	MOD1,	0,	GDK_c,		toggle_cwl,	{.i = XT_WL_TOGGLE | XT_WL_FQDN} },
	{ "togglesrc",		CTRL,	0,	GDK_s,		toggle_src,	{0} },
	{ "yankuri",		0,	0,	GDK_y,		yank_uri,	{0} },
	{ "pasteuricur",	0,	0,	GDK_p,		paste_uri,	{.i = XT_PASTE_CURRENT_TAB} },
	{ "pasteurinew",	0,	0,	GDK_P,		paste_uri,	{.i = XT_PASTE_NEW_TAB} },

	/* search */
	{ "searchnext",		0,	0,	GDK_n,		search,		{.i = XT_SEARCH_NEXT} },
	{ "searchprev",		0,	0,	GDK_N,		search,		{.i = XT_SEARCH_PREV} },

	/* focus */
	{ "focusaddress",	0,	0,	GDK_F6,		focus,		{.i = XT_FOCUS_URI} },
	{ "focussearch",	0,	0,	GDK_F7,		focus,		{.i = XT_FOCUS_SEARCH} },

	/* command aliases (handy when -S flag is used) */
	{ NULL,			0,	0,	GDK_F9,		command,	{.i = XT_CMD_OPEN} },
	{ NULL,			0,	0,	GDK_F10,	command,	{.i = XT_CMD_OPEN_CURRENT} },
	{ NULL,			0,	0,	GDK_F11,	command,	{.i = XT_CMD_TABNEW} },
	{ NULL,			0,	0,	GDK_F12,	command,	{.i = XT_CMD_TABNEW_CURRENT} },

	/* hinting */
	{ "hinting",		0,	0,	GDK_f,		hint,		{.i = 0} },

	/* navigation */
	{ "goback",		0,	0,	GDK_BackSpace,	navaction,	{.i = XT_NAV_BACK} },
	{ "goback",		MOD1,	0,	GDK_Left,	navaction,	{.i = XT_NAV_BACK} },
	{ "goforward",		SHFT,	0,	GDK_BackSpace,	navaction,	{.i = XT_NAV_FORWARD} },
	{ "goforward",		MOD1,	0,	GDK_Right,	navaction,	{.i = XT_NAV_FORWARD} },
	{ "reload",		0,	0,	GDK_F5,		navaction,	{.i = XT_NAV_RELOAD} },
	{ "reload",		CTRL,	0,	GDK_r,		navaction,	{.i = XT_NAV_RELOAD} },
	{ "reloadforce",	CTRL,	0,	GDK_R,		navaction,	{.i = XT_NAV_RELOAD_CACHE} },
	{ "reload"	,	CTRL,	0,	GDK_l,		navaction,	{.i = XT_NAV_RELOAD} },
	{ "favorites",		MOD1,	1,	GDK_f,		xtp_page_fl,	{0} },

	/* vertical movement */
	{ "scrolldown",		0,	0,	GDK_j,		move,		{.i = XT_MOVE_DOWN} },
	{ "scrolldown",		0,	0,	GDK_Down,	move,		{.i = XT_MOVE_DOWN} },
	{ "scrollup",		0,	0,	GDK_Up,		move,		{.i = XT_MOVE_UP} },
	{ "scrollup",		0,	0,	GDK_k,		move,		{.i = XT_MOVE_UP} },
	{ "scrollbottom",	0,	0,	GDK_G,		move,		{.i = XT_MOVE_BOTTOM} },
	{ "scrollbottom",	0,	0,	GDK_End,	move,		{.i = XT_MOVE_BOTTOM} },
	{ "scrolltop",		0,	0,	GDK_Home,	move,		{.i = XT_MOVE_TOP} },
	{ "scrolltop",		0,	0,	GDK_g,		move,		{.i = XT_MOVE_TOP} },
	{ "scrollpagedown",	0,	0,	GDK_space,	move,		{.i = XT_MOVE_PAGEDOWN} },
	{ "scrollpagedown",	CTRL,	0,	GDK_f,		move,		{.i = XT_MOVE_PAGEDOWN} },
	{ "scrollhalfdown",	CTRL,	0,	GDK_d,		move,		{.i = XT_MOVE_HALFDOWN} },
	{ "scrollpagedown",	0,	0,	GDK_Page_Down,	move,		{.i = XT_MOVE_PAGEDOWN} },
	{ "scrollpageup",	0,	0,	GDK_Page_Up,	move,		{.i = XT_MOVE_PAGEUP} },
	{ "scrollpageup",	CTRL,	0,	GDK_b,		move,		{.i = XT_MOVE_PAGEUP} },
	{ "scrollhalfup",	CTRL,	0,	GDK_u,		move,		{.i = XT_MOVE_HALFUP} },
	/* horizontal movement */
	{ "scrollright",	0,	0,	GDK_l,		move,		{.i = XT_MOVE_RIGHT} },
	{ "scrollright",	0,	0,	GDK_Right,	move,		{.i = XT_MOVE_RIGHT} },
	{ "scrollleft",		0,	0,	GDK_Left,	move,		{.i = XT_MOVE_LEFT} },
	{ "scrollleft",		0,	0,	GDK_h,		move,		{.i = XT_MOVE_LEFT} },
	{ "scrollfarright",	0,	0,	GDK_dollar,	move,		{.i = XT_MOVE_FARRIGHT} },
	{ "scrollfarleft",	0,	0,	GDK_0,		move,		{.i = XT_MOVE_FARLEFT} },

	/* tabs */
	{ "tabnew",		CTRL,	0,	GDK_t,		tabaction,	{.i = XT_TAB_NEW} },
	{ "tabclose",		CTRL,	1,	GDK_w,		tabaction,	{.i = XT_TAB_DELETE} },
	{ "tabundoclose",	0,	0,	GDK_U,		tabaction,	{.i = XT_TAB_UNDO_CLOSE} },
	{ "tabgoto1",		CTRL,	0,	GDK_1,		movetab,	{.i = 1} },
	{ "tabgoto2",		CTRL,	0,	GDK_2,		movetab,	{.i = 2} },
	{ "tabgoto3",		CTRL,	0,	GDK_3,		movetab,	{.i = 3} },
	{ "tabgoto4",		CTRL,	0,	GDK_4,		movetab,	{.i = 4} },
	{ "tabgoto5",		CTRL,	0,	GDK_5,		movetab,	{.i = 5} },
	{ "tabgoto6",		CTRL,	0,	GDK_6,		movetab,	{.i = 6} },
	{ "tabgoto7",		CTRL,	0,	GDK_7,		movetab,	{.i = 7} },
	{ "tabgoto8",		CTRL,	0,	GDK_8,		movetab,	{.i = 8} },
	{ "tabgoto9",		CTRL,	0,	GDK_9,		movetab,	{.i = 9} },
	{ "tabgoto10",		CTRL,	0,	GDK_0,		movetab,	{.i = 10} },
	{ "tabgotofirst",	CTRL,	0,	GDK_less,	movetab,	{.i = XT_TAB_FIRST} },
	{ "tabgotolast",	CTRL,	0,	GDK_greater,	movetab,	{.i = XT_TAB_LAST} },
	{ "tabgotoprev",	CTRL,	0,	GDK_Left,	movetab,	{.i = XT_TAB_PREV} },
	{ "tabgotonext",	CTRL,	0,	GDK_Right,	movetab,	{.i = XT_TAB_NEXT} },
	{ "focusout",		CTRL,	0,	GDK_minus,	resizetab,	{.i = -1} },
	{ "focusin",		CTRL,	0,	GDK_plus,	resizetab,	{.i = 1} },
	{ "focusin",		CTRL,	0,	GDK_equal,	resizetab,	{.i = 1} },
};
TAILQ_HEAD(keybinding_list, key_binding);

void
walk_kb(struct settings *s,
    void (*cb)(struct settings *, char *, void *), void *cb_args)
{
	struct key_binding	*k;
	char			str[1024];

	if (s == NULL || cb == NULL) {
		show_oops_s("walk_kb invalid parameters");
		return;
	}

	TAILQ_FOREACH(k, &kbl, entry) {
		if (k->name == NULL)
			continue;
		str[0] = '\0';

		/* sanity */
		if (gdk_keyval_name(k->key) == NULL)
			continue;

		strlcat(str, k->name, sizeof str);
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
		k->name = keys[i].name;
		k->mask = keys[i].mask;
		k->use_in_entry = keys[i].use_in_entry;
		k->key = keys[i].key;
		k->func = keys[i].func;
		bcopy(&keys[i].arg, &k->arg, sizeof k->arg);
		TAILQ_INSERT_HEAD(&kbl, k, entry);

		DNPRINTF(XT_D_KEYBINDING, "init_keybindings: added: %s\n",
		    k->name ? k->name : "unnamed key");
	}
}

void
keybinding_clearall(void)
{
	struct key_binding	*k, *next;

	for (k = TAILQ_FIRST(&kbl); k; k = next) {
		next = TAILQ_NEXT(k, entry);
		if (k->name == NULL)
			continue;

		DNPRINTF(XT_D_KEYBINDING, "keybinding_clearall: %s\n",
		    k->name ? k->name : "unnamed key");
		TAILQ_REMOVE(&kbl, k, entry);
		g_free(k);
	}
}

int
keybinding_add(char *kb, char *value, struct key_binding *orig)
{
	struct key_binding	*k;
	guint			keyval, mask = 0;
	int			i;

	DNPRINTF(XT_D_KEYBINDING, "keybinding_add: %s %s %s\n", kb, value, orig->name);

	if (orig == NULL)
		return (1);
	if (strcmp(kb, orig->name))
		return (1);

	/* find modifier keys */
	if (strstr(value, "S-"))
		mask |= GDK_SHIFT_MASK;
	if (strstr(value, "C-"))
		mask |= GDK_CONTROL_MASK;
	if (strstr(value, "M1-"))
		mask |= GDK_MOD1_MASK;
	if (strstr(value, "M2-"))
		mask |= GDK_MOD2_MASK;
	if (strstr(value, "M3-"))
		mask |= GDK_MOD3_MASK;
	if (strstr(value, "M4-"))
		mask |= GDK_MOD4_MASK;
	if (strstr(value, "M5-"))
		mask |= GDK_MOD5_MASK;

	/* find keyname */
	for (i = strlen(value) - 1; i > 0; i--)
		if (value[i] == '-')
			value = &value[i + 1];

	/* validate keyname */
	keyval = gdk_keyval_from_name(value);
	if (keyval == GDK_VoidSymbol) {
		warnx("invalid keybinding name %s", value);
		return (1);
	}
	/* must run this test too, gtk+ doesn't handle 10 for example */
	if (gdk_keyval_name(keyval) == NULL) {
		warnx("invalid keybinding name %s", value);
		return (1);
	}

	/* make sure it isn't a dupe */
	TAILQ_FOREACH(k, &kbl, entry)
		if (k->key == keyval && k->mask == mask) {
			warnx("duplicate keybinding for %s", value);
			return (1);
		}

	/* add keyname */
	k = g_malloc0(sizeof *k);
	k->name = orig->name;
	k->mask = mask;
	k->use_in_entry = orig->use_in_entry;
	k->key = keyval;
	k->func = orig->func;
	bcopy(&orig->arg, &k->arg, sizeof k->arg);

	DNPRINTF(XT_D_KEYBINDING, "keybinding_add: %s 0x%x %d 0x%x\n",
	    k->name,
	    k->mask,
	    k->use_in_entry,
	    k->key);
	DNPRINTF(XT_D_KEYBINDING, "keybinding_add: adding: %s %s\n",
	    k->name, gdk_keyval_name(keyval));

	TAILQ_INSERT_HEAD(&kbl, k, entry);

	return (0);
}

int
add_kb(struct settings *s, char *entry)
{
	int			i;
	char			*kb, *value;

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
	value = kb + 1;

	/* make sure it is a valid keybinding */
	for (i = 0; i < LENGTH(keys); i++)
		if (keys[i].name && !strcmp(entry, keys[i].name)) {
			DNPRINTF(XT_D_KEYBINDING, "add_kb: %s 0x%x %d 0x%x\n",
			    keys[i].name,
			    keys[i].mask,
			    keys[i].use_in_entry,
			    keys[i].key);

			return (keybinding_add(entry, value, &keys[i]));
		}

	return (1);
}

struct cmd {
	char		*cmd;
	int		params;
	int		(*func)(struct tab *, struct karg *);
	struct karg	arg;
} cmds[] = {
	{ "q!",			0,	quit,			{0} },
	{ "qa",			0,	quit,			{0} },
	{ "qa!",		0,	quit,			{0} },
	{ "w",			0,	save_tabs,		{0} },
	{ "wq",			0,	save_tabs_and_quit,	{0} },
	{ "wq!",		0,	save_tabs_and_quit,	{0} },
	{ "help",		0,	help,			{0} },
	{ "about",		0,	about,			{0} },
	{ "stats",		0,	stats,			{0} },
	{ "version",		0,	about,			{0} },
	{ "cookies",		0,	xtp_page_cl,		{0} },
	{ "fav",		0,	xtp_page_fl,		{0} },
	{ "favadd",		0,	add_favorite,		{0} },
	{ "js",			2,	js_cmd,			{0} },
	{ "cookie",		2,	cookie_cmd,		{0} },
	{ "cert",		1,	cert_cmd,		{0} },
	{ "ca",			0,	ca_cmd,			{0} },
	{ "dl",			0,	xtp_page_dl,		{0} },
	{ "h",			0,	xtp_page_hl,		{0} },
	{ "hist",		0,	xtp_page_hl,		{0} },
	{ "history",		0,	xtp_page_hl,		{0} },
	{ "home",		0,	go_home,		{0} },
	{ "restart",		0,	restart,		{0} },
	{ "urlhide",		0,	urlaction,		{.i = XT_URL_HIDE} },
	{ "urlh",		0,	urlaction,		{.i = XT_URL_HIDE} },
	{ "urlshow",		0,	urlaction,		{.i = XT_URL_SHOW} },
	{ "urls",		0,	urlaction,		{.i = XT_URL_SHOW} },
	{ "statushide",		0,	statusaction,		{.i = XT_STATUSBAR_HIDE} },
	{ "statush",		0,	statusaction,		{.i = XT_STATUSBAR_HIDE} },
	{ "statusshow",		0,	statusaction,		{.i = XT_STATUSBAR_SHOW} },
	{ "statuss",		0,	statusaction,		{.i = XT_STATUSBAR_SHOW} },

	{ "1",			0,	move,			{.i = XT_MOVE_TOP} },
	{ "print",		0,	print_page,		{0} },

	/* tabs */
	{ "o",			1,	tabaction,		{.i = XT_TAB_OPEN} },
	{ "op",			1,	tabaction,		{.i = XT_TAB_OPEN} },
	{ "open",		1,	tabaction,		{.i = XT_TAB_OPEN} },
	{ "tabnew",		1,	tabaction,		{.i = XT_TAB_NEW} },
	{ "tabedit",		1,	tabaction,		{.i = XT_TAB_NEW} },
	{ "tabe",		1,	tabaction,		{.i = XT_TAB_NEW} },
	{ "tabclose",		0,	tabaction,		{.i = XT_TAB_DELETE} },
	{ "tabc",		0,	tabaction,		{.i = XT_TAB_DELETE} },
	{ "tabshow",		1,	tabaction,		{.i = XT_TAB_SHOW} },
	{ "tabs",		1,	tabaction,		{.i = XT_TAB_SHOW} },
	{ "tabhide",		1,	tabaction,		{.i = XT_TAB_HIDE} },
	{ "tabh",		1,	tabaction,		{.i = XT_TAB_HIDE} },
	{ "quit",		0,	tabaction,		{.i = XT_TAB_DELQUIT} },
	{ "q",			0,	tabaction,		{.i = XT_TAB_DELQUIT} },
	/* XXX add count to these commands */
	{ "tabfirst",		0,	movetab,		{.i = XT_TAB_FIRST} },
	{ "tabfir",		0,	movetab,		{.i = XT_TAB_FIRST} },
	{ "tabrewind",		0,	movetab,		{.i = XT_TAB_FIRST} },
	{ "tabr",		0,	movetab,		{.i = XT_TAB_FIRST} },
	{ "tablast",		0,	movetab,		{.i = XT_TAB_LAST} },
	{ "tabl",		0,	movetab,		{.i = XT_TAB_LAST} },
	{ "tabprevious",	0,	movetab,		{.i = XT_TAB_PREV} },
	{ "tabp",		0,	movetab,		{.i = XT_TAB_PREV} },
	{ "tabnext",		0,	movetab,		{.i = XT_TAB_NEXT} },
	{ "tabn",		0,	movetab,		{.i = XT_TAB_NEXT} },

	/* settings */
	{ "set",		1,	set,			{0} },
	{ "fullscreen",		0,	fullscreen,		{0} },
	{ "f",			0,	fullscreen,		{0} },

	/* sessions */
	{ "session",		1,	session_cmd,		{0} },
};

gboolean
wv_button_cb(GtkWidget *btn, GdkEventButton *e, struct tab *t)
{
	hide_oops(t);

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

/*
 * cancel, remove, etc. downloads
 */
void
xtp_handle_dl(struct tab *t, uint8_t cmd, int id)
{
	struct download		find, *d;

	DNPRINTF(XT_D_DOWNLOAD, "download control: cmd %d, id %d\n", cmd, id);

	/* some commands require a valid download id */
	if (cmd != XT_XTP_DL_LIST) {
		/* lookup download in question */
		find.id = id;
		d = RB_FIND(download_list, &downloads, &find);

		if (d == NULL) {
			show_oops(t, "%s: no such download", __func__);
			return;
		}
	}

	/* decide what to do */
	switch (cmd) {
	case XT_XTP_DL_CANCEL:
		webkit_download_cancel(d->download);
		break;
	case XT_XTP_DL_REMOVE:
		webkit_download_cancel(d->download); /* just incase */
		g_object_unref(d->download);
		RB_REMOVE(download_list, &downloads, d);
		break;
	case XT_XTP_DL_LIST:
		/* Nothing */
		break;
	default:
		show_oops(t, "%s: unknown command", __func__);
		break;
	};
	xtp_page_dl(t, NULL);
}

/*
 * Actions on history, only does one thing for now, but
 * we provide the function for future actions
 */
void
xtp_handle_hl(struct tab *t, uint8_t cmd, int id)
{
	struct history		*h, *next;
	int			i = 1;

	switch (cmd) {
	case XT_XTP_HL_REMOVE:
		/* walk backwards, as listed in reverse */
		for (h = RB_MAX(history_list, &hl); h != NULL; h = next) {
			next = RB_PREV(history_list, &hl, h);
			if (id == i) {
				RB_REMOVE(history_list, &hl, h);
				g_free((gpointer) h->title);
				g_free((gpointer) h->uri);
				g_free(h);
				break;
			}
			i++;
		}
		break;
	case XT_XTP_HL_LIST:
		/* Nothing - just xtp_page_hl() below */
		break;
	default:
		show_oops(t, "%s: unknown command", __func__);
		break;
	};

	xtp_page_hl(t, NULL);
}

/* remove a favorite */
void
remove_favorite(struct tab *t, int index)
{
	char			file[PATH_MAX], *title, *uri;
	char			*new_favs, *tmp;
	FILE			*f;
	int			i;
	size_t			len, lineno;

	/* open favorites */
	snprintf(file, sizeof file, "%s/%s", work_dir, XT_FAVS_FILE);

	if ((f = fopen(file, "r")) == NULL) {
		show_oops(t, "%s: can't open favorites: %s",
		    __func__, strerror(errno));
		return;
	}

	/* build a string which will become the new favroites file */
	new_favs = g_strdup_printf("%s", "");

	for (i = 1;;) {
		if ((title = fparseln(f, &len, &lineno, NULL, 0)) == NULL)
			if (feof(f) || ferror(f))
				break;
		/* XXX THIS IS NOT THE RIGHT HEURISTIC */
		if (len == 0) {
			free(title);
			title = NULL;
			continue;
		}

		if ((uri = fparseln(f, &len, &lineno, NULL, 0)) == NULL) {
			if (feof(f) || ferror(f)) {
				show_oops(t, "%s: can't parse favorites %s",
				    __func__, strerror(errno));
				goto clean;
			}
		}

		/* as long as this isn't the one we are deleting add to file */
		if (i != index) {
			tmp = new_favs;
			new_favs = g_strdup_printf("%s%s\n%s\n",
			    new_favs, title, uri);
			g_free(tmp);
		}

		free(uri);
		uri = NULL;
		free(title);
		title = NULL;
		i++;
	}
	fclose(f);

	/* write back new favorites file */
	if ((f = fopen(file, "w")) == NULL) {
		show_oops(t, "%s: can't open favorites: %s",
		    __func__, strerror(errno));
		goto clean;
	}

	fwrite(new_favs, strlen(new_favs), 1, f);
	fclose(f);

clean:
	if (uri)
		free(uri);
	if (title)
		free(title);

	g_free(new_favs);
}

void
xtp_handle_fl(struct tab *t, uint8_t cmd, int arg)
{
	switch (cmd) {
	case XT_XTP_FL_LIST:
		/* nothing, just the below call to xtp_page_fl() */
		break;
	case XT_XTP_FL_REMOVE:
		remove_favorite(t, arg);
		break;
	default:
		show_oops(t, "%s: invalid favorites command", __func__);
		break;
	};

	xtp_page_fl(t, NULL);
}

void
xtp_handle_cl(struct tab *t, uint8_t cmd, int arg)
{
	switch (cmd) {
	case XT_XTP_CL_LIST:
		/* nothing, just xtp_page_cl() */
		break;
	case XT_XTP_CL_REMOVE:
		remove_cookie(arg);
		break;
	default:
		show_oops(t, "%s: unknown cookie xtp command", __func__);
		break;
	};

	xtp_page_cl(t, NULL);
}

/* link an XTP class to it's session key and handler function */
struct xtp_despatch {
	uint8_t			xtp_class;
	char			**session_key;
	void			(*handle_func)(struct tab *, uint8_t, int);
};

struct xtp_despatch		xtp_despatches[] = {
	{ XT_XTP_DL, &dl_session_key, xtp_handle_dl },
	{ XT_XTP_HL, &hl_session_key, xtp_handle_hl },
	{ XT_XTP_FL, &fl_session_key, xtp_handle_fl },
	{ XT_XTP_CL, &cl_session_key, xtp_handle_cl },
	{ NULL, NULL, NULL }
};

/*
 * is the url xtp protocol? (xxxt://)
 * if so, parse and despatch correct bahvior
 */
int
parse_xtp_url(struct tab *t, const char *url)
{
	char			*dup = NULL, *p, *last;
	uint8_t			n_tokens = 0;
	char			*tokens[4] = {NULL, NULL, NULL, ""};
	struct xtp_despatch	*dsp, *dsp_match = NULL;
	uint8_t			req_class;

	/*
	 * tokens array meaning:
	 *   tokens[0] = class
	 *   tokens[1] = session key
	 *   tokens[2] = action
	 *   tokens[3] = optional argument
	 */

	DNPRINTF(XT_D_URL, "%s: url %s\n", __func__, url);

	/*xtp tab meaning is normal unless proven special */
	t->xtp_meaning = XT_XTP_TAB_MEANING_NORMAL;

	if (strncmp(url, XT_XTP_STR, strlen(XT_XTP_STR)))
		return 0;

	dup = g_strdup(url + strlen(XT_XTP_STR));

	/* split out the url */
	for ((p = strtok_r(dup, "/", &last)); p;
	    (p = strtok_r(NULL, "/", &last))) {
		if (n_tokens < 4)
			tokens[n_tokens++] = p;
	}

	/* should be atleast three fields 'class/seskey/command/arg' */
	if (n_tokens < 3)
		goto clean;

	dsp = xtp_despatches;
	req_class = atoi(tokens[0]);
	while (dsp->xtp_class != NULL) {
		if (dsp->xtp_class == req_class) {
			dsp_match = dsp;
			break;
		}
		dsp++;
	}

	/* did we find one atall? */
	if (dsp_match == NULL) {
		show_oops(t, "%s: no matching xtp despatch found", __func__);
		goto clean;
	}

	/* check session key and call despatch function */
	if (validate_xtp_session_key(t, *(dsp_match->session_key), tokens[1])) {
		dsp_match->handle_func(t, atoi(tokens[2]), atoi(tokens[3]));
	}

clean:
	if (dup)
		g_free(dup);

	return 1;
}



void
activate_uri_entry_cb(GtkWidget* entry, struct tab *t)
{
	const gchar		*uri = gtk_entry_get_text(GTK_ENTRY(entry));

	DNPRINTF(XT_D_URL, "activate_uri_entry_cb: %s\n", uri);

	if (t == NULL) {
		show_oops_s("activate_uri_entry_cb invalid parameters");
		return;
	}

	if (uri == NULL) {
		show_oops(t, "activate_uri_entry_cb no uri");
		return;
	}

	uri += strspn(uri, "\t ");

	/* if xxxt:// treat specially */
	if (!parse_xtp_url(t, uri)) {
		load_uri(t, (gchar *)uri);
		focus_webview(t);
	}
}

void
activate_search_entry_cb(GtkWidget* entry, struct tab *t)
{
	const gchar		*search = gtk_entry_get_text(GTK_ENTRY(entry));
	char			*newuri = NULL;
	gchar			*enc_search;

	DNPRINTF(XT_D_URL, "activate_search_entry_cb: %s\n", search);

	if (t == NULL) {
		show_oops_s("activate_search_entry_cb invalid parameters");
		return;
	}

	if (search_string == NULL) {
		show_oops(t, "no search_string");
		return;
	}

	enc_search = soup_uri_encode(search, XT_RESERVED_CHARS);
	newuri = g_strdup_printf(search_string, enc_search);
	g_free(enc_search);

	webkit_web_view_load_uri(t->wv, newuri);
	focus_webview(t);

	if (newuri)
		g_free(newuri);
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
	webkit_web_view_set_settings(t->wv, t->settings);

	button_set_stockid(t->js_toggle,
	    es ? GTK_STOCK_MEDIA_PLAY : GTK_STOCK_MEDIA_PAUSE);
}

void
show_ca_status(struct tab *t, const char *uri)
{
	WebKitWebFrame		*frame;
	WebKitWebDataSource	*source;
	WebKitNetworkRequest	*request;
	SoupMessage		*message;
	GdkColor		color;
	gchar			*col_str = "white";
	int			r;

	DNPRINTF(XT_D_URL, "show_ca_status: %d %s %s\n",
	    ssl_strict_certs, ssl_ca_file, uri);

	if (uri == NULL)
		goto done;
	if (ssl_ca_file == NULL) {
		if (g_str_has_prefix(uri, "http://"))
			goto done;
		if (g_str_has_prefix(uri, "https://")) {
			col_str = "red";
			goto done;
		}
		return;
	}
	if (g_str_has_prefix(uri, "http://") ||
	    !g_str_has_prefix(uri, "https://"))
		goto done;

	frame = webkit_web_view_get_main_frame(t->wv);
	source = webkit_web_frame_get_data_source(frame);
	request = webkit_web_data_source_get_request(source);
	message = webkit_network_request_get_message(request);

	if (message && (soup_message_get_flags(message) &
	    SOUP_MESSAGE_CERTIFICATE_TRUSTED)) {
		col_str = "green";
		goto done;
	} else {
		r = load_compare_cert(t, NULL);
		if (r == 0)
			col_str = "lightblue";
		else if (r == 1)
			col_str = "yellow";
		else
			col_str = "red";
		goto done;
	}
done:
	if (col_str) {
		gdk_color_parse(col_str, &color);
		gtk_widget_modify_base(t->uri_entry, GTK_STATE_NORMAL, &color);

		if (!strcmp(col_str, "white")) {
			gtk_widget_modify_text(t->statusbar, GTK_STATE_NORMAL,
			    &color);
			gdk_color_parse("black", &color);
			gtk_widget_modify_base(t->statusbar, GTK_STATE_NORMAL,
			    &color);
		} else {
			gtk_widget_modify_base(t->statusbar, GTK_STATE_NORMAL,
			    &color);
			gdk_color_parse("black", &color);
			gtk_widget_modify_text(t->statusbar, GTK_STATE_NORMAL,
			    &color);
		}
	}
}

void
free_favicon(struct tab *t)
{
	DNPRINTF(XT_D_DOWNLOAD, "%s: down %p req %p pix %p\n",
	    __func__, t->icon_download, t->icon_request, t->icon_pixbuf);

	if (t->icon_request)
		g_object_unref(t->icon_request);
	if (t->icon_pixbuf)
		g_object_unref(t->icon_pixbuf);
	if (t->icon_dest_uri)
		g_free(t->icon_dest_uri);

	t->icon_pixbuf = NULL;
	t->icon_request = NULL;
	t->icon_dest_uri = NULL;
}

void
xt_icon_from_name(struct tab *t, gchar *name)
{
	gtk_entry_set_icon_from_icon_name(GTK_ENTRY(t->uri_entry),
	    GTK_ENTRY_ICON_PRIMARY, "text-html");
	if (show_url == 0) {
		gtk_entry_set_icon_from_icon_name(GTK_ENTRY(t->statusbar),
		    GTK_ENTRY_ICON_PRIMARY, "text-html");
	} else {
		gtk_entry_set_icon_from_icon_name(GTK_ENTRY(t->statusbar),
		    GTK_ENTRY_ICON_PRIMARY, NULL);
	}
	return;
}

void
xt_icon_from_pixbuf(struct tab *t, GdkPixbuf *pixbuf)
{
	gtk_entry_set_icon_from_pixbuf(GTK_ENTRY(t->uri_entry),
	    GTK_ENTRY_ICON_PRIMARY, pixbuf);
	if (show_url == 0) {
		gtk_entry_set_icon_from_pixbuf(GTK_ENTRY(t->statusbar),
		    GTK_ENTRY_ICON_PRIMARY, pixbuf);
	} else {
		gtk_entry_set_icon_from_icon_name(GTK_ENTRY(t->statusbar),
		    GTK_ENTRY_ICON_PRIMARY, NULL);
	}
}

void
abort_favicon_download(struct tab *t)
{
	DNPRINTF(XT_D_DOWNLOAD, "%s: down %p\n", __func__, t->icon_download);

	if (t->icon_download)
		webkit_download_cancel(t->icon_download);
	else
		free_favicon(t);

	xt_icon_from_name(t, "text-html");
}

void
set_favicon_from_file(struct tab *t, char *file)
{
	gint			width, height;
	GdkPixbuf		*pixbuf, *scaled;
	struct stat		sb;

	if (t == NULL || file == NULL)
		return;
	if (t->icon_pixbuf) {
		DNPRINTF(XT_D_DOWNLOAD, "%s: icon already set\n", __func__);
		return;
	}

	if (g_str_has_prefix(file, "file://"))
		file += strlen("file://");
	DNPRINTF(XT_D_DOWNLOAD, "%s: loading %s\n", __func__, file);

	if (!stat(file, &sb)) {
		if (sb.st_size == 0) {
			/* corrupt icon so trash it */
			DNPRINTF(XT_D_DOWNLOAD, "%s: corrupt icon %s\n",
			    __func__, file);
			unlink(file);
			/* no need to set icon to default here */
			return;
		}
	}

	pixbuf = gdk_pixbuf_new_from_file(file, NULL);
	if (pixbuf == NULL) {
		xt_icon_from_name(t, "text-html");
		return;
	}

	g_object_get(pixbuf, "width", &width, "height", &height,
	    (char *)NULL);
	DNPRINTF(XT_D_DOWNLOAD, "%s: tab %d icon size %dx%d\n",
	    __func__, t->tab_id, width, height);

	if (width > 16 || height > 16) {
		scaled = gdk_pixbuf_scale_simple(pixbuf, 16, 16,
		    GDK_INTERP_BILINEAR);
		g_object_unref(pixbuf);
	} else
		scaled = pixbuf;

	if (scaled == NULL) {
		scaled = gdk_pixbuf_scale_simple(pixbuf, 16, 16,
		    GDK_INTERP_BILINEAR);
		return;
	}

	t->icon_pixbuf = scaled;
	xt_icon_from_pixbuf(t, t->icon_pixbuf);
}

void
favicon_download_status_changed_cb(WebKitDownload *download, GParamSpec *spec,
    struct tab *t)
{
	WebKitDownloadStatus	status = webkit_download_get_status(download);

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
notify_icon_loaded_cb(WebKitWebView *wv, gchar *uri, struct tab *t)
{
	gchar			*name_hash, file[PATH_MAX];
	struct stat		sb;

	DNPRINTF(XT_D_DOWNLOAD, "notify_icon_loaded_cb %s\n", uri);

	if (uri == NULL || t == NULL)
		return;

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

	/* we have to free icon_dest_uri later */
	t->icon_dest_uri = g_strdup_printf("file://%s", file);
	webkit_download_set_destination_uri(t->icon_download,
	    t->icon_dest_uri);

	g_signal_connect(G_OBJECT(t->icon_download), "notify::status",
	    G_CALLBACK(favicon_download_status_changed_cb), t);

	webkit_download_start(t->icon_download);
}

void
notify_load_status_cb(WebKitWebView* wview, GParamSpec* pspec, struct tab *t)
{
	const gchar		*set = NULL, *uri = NULL, *title = NULL;
	struct history		*h, find;
	int			add = 0;
	const gchar		*s_loading;
	struct karg		a;

	DNPRINTF(XT_D_URL, "notify_load_status_cb: %d\n",
	    webkit_web_view_get_load_status(wview));

	if (t == NULL) {
		show_oops_s("notify_load_status_cb invalid paramters");
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

		break;

	case WEBKIT_LOAD_COMMITTED:
		/* 1 */
		if ((uri = get_uri(wview)) != NULL) {
			if (strncmp(uri, XT_URI_ABOUT, XT_URI_ABOUT_LEN))
				gtk_entry_set_text(GTK_ENTRY(t->uri_entry), uri);

			if (t->status) {
				g_free(t->status);
				t->status = NULL;
			}
			set_status(t, (char *)uri, XT_STATUS_LOADING);
		}

		/* check if js white listing is enabled */
		if (enable_js_whitelist) {
			uri = get_uri(wview);
			check_and_set_js(uri, t);
		}

		show_ca_status(t, uri);

		/* we know enough to autosave the session */
		if (session_autosave) {
			a.s = NULL;
			save_tabs(t, &a);
		}
		break;

	case WEBKIT_LOAD_FIRST_VISUALLY_NON_EMPTY_LAYOUT:
		/* 3 */
		title = webkit_web_view_get_title(wview);
		uri = get_uri(wview);
		if (title)
			set = title;
		else if (uri)
			set = uri;
		else
			break;

		gtk_label_set_text(GTK_LABEL(t->label), set);
		gtk_window_set_title(GTK_WINDOW(main_window), set);

		if (uri) {
			if (!strncmp(uri, "http://", strlen("http://")) ||
			    !strncmp(uri, "https://", strlen("https://")) ||
			    !strncmp(uri, "file://", strlen("file://")))
				add = 1;
			if (add == 0)
				break;
			find.uri = uri;
			h = RB_FIND(history_list, &hl, &find);
			if (h)
				break;

			h = g_malloc(sizeof *h);
			h->uri = g_strdup(uri);
			if (title)
				h->title = g_strdup(title);
			else
				h->title = g_strdup(uri);
			RB_INSERT(history_list, &hl, h);
			completion_add_uri(h->uri);
			update_history_tabs(NULL);
		}

		break;

	case WEBKIT_LOAD_FINISHED:
		/* 2 */
		uri = get_uri(wview);
		set_status(t, (char *)uri, XT_STATUS_URI);
#if WEBKIT_CHECK_VERSION(1, 1, 18)
	case WEBKIT_LOAD_FAILED:
		/* 4 */
#endif
#if GTK_CHECK_VERSION(2, 20, 0)
		gtk_spinner_stop(GTK_SPINNER(t->spinner));
		gtk_widget_hide(t->spinner);
#endif
		s_loading = gtk_label_get_text(GTK_LABEL(t->label));
		if (s_loading && !strcmp(s_loading, "Loading"))
			gtk_label_set_text(GTK_LABEL(t->label), "(untitled)");
	default:
		gtk_widget_set_sensitive(GTK_WIDGET(t->stop), FALSE);
		break;
	}

	if (t->item)
		gtk_widget_set_sensitive(GTK_WIDGET(t->backward), TRUE);
	else
		gtk_widget_set_sensitive(GTK_WIDGET(t->backward),
		    webkit_web_view_can_go_back(wview));

	gtk_widget_set_sensitive(GTK_WIDGET(t->forward),
	    webkit_web_view_can_go_forward(wview));

	/* take focus if we are visible */
	t->focus_wv = 1;
	focus_webview(t);
}

void
webview_load_finished_cb(WebKitWebView *wv, WebKitWebFrame *wf, struct tab *t)
{
	run_script(t, JS_HINTING);
}

void
webview_progress_changed_cb(WebKitWebView *wv, int progress, struct tab *t)
{
	gtk_entry_set_progress_fraction(GTK_ENTRY(t->uri_entry),
	    progress == 100 ? 0 : (double)progress / 100);
	if (show_url == 0) {
		gtk_entry_set_progress_fraction(GTK_ENTRY(t->statusbar),
		    progress == 100 ? 0 : (double)progress / 100);
	}
}

int
webview_nw_cb(WebKitWebView *wv, WebKitWebFrame *wf,
    WebKitNetworkRequest *request, WebKitWebNavigationAction *na,
    WebKitWebPolicyDecision *pd, struct tab *t)
{
	char			*uri;

	if (t == NULL) {
		show_oops_s("webview_nw_cb invalid paramters");
		return (FALSE);
	}

	DNPRINTF(XT_D_NAV, "webview_nw_cb: %s\n",
	    webkit_network_request_get_uri(request));

	/* open in current tab */
	uri = (char *)webkit_network_request_get_uri(request);
	webkit_web_view_load_uri(t->wv, uri);
	webkit_web_policy_decision_ignore(pd);

	return (TRUE); /* we made the decission */
}

int
webview_npd_cb(WebKitWebView *wv, WebKitWebFrame *wf,
    WebKitNetworkRequest *request, WebKitWebNavigationAction *na,
    WebKitWebPolicyDecision *pd, struct tab *t)
{
	char			*uri;

	if (t == NULL) {
		show_oops_s("webview_npd_cb invalid parameters");
		return (FALSE);
	}

	DNPRINTF(XT_D_NAV, "webview_npd_cb: ctrl_click %d %s\n",
	    t->ctrl_click,
	    webkit_network_request_get_uri(request));

	uri = (char *)webkit_network_request_get_uri(request);

	if ((!parse_xtp_url(t, uri) && (t->ctrl_click))) {
		t->ctrl_click = 0;
		create_new_tab(uri, NULL, ctrl_click_focus);
		webkit_web_policy_decision_ignore(pd);
		return (TRUE); /* we made the decission */
	}

	webkit_web_policy_decision_use(pd);
	return (TRUE); /* we made the decission */
}

WebKitWebView *
webview_cwv_cb(WebKitWebView *wv, WebKitWebFrame *wf, struct tab *t)
{
	DNPRINTF(XT_D_NAV, "webview_cwv_cb: %s\n",
	    webkit_web_view_get_uri(wv));

	return (wv);
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

	switch (fork()) {
	case -1:
		show_oops(t, "can't fork mime handler");
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

int
webview_mimetype_cb(WebKitWebView *wv, WebKitWebFrame *frame,
    WebKitNetworkRequest *request, char *mime_type,
    WebKitWebPolicyDecision *decision, struct tab *t)
{
	if (t == NULL) {
		show_oops_s("webview_mimetype_cb invalid parameters");
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
	const gchar		*filename;
	char			*uri = NULL;
	struct download		*download_entry;
	int			ret = TRUE;

	if (wk_download == NULL || t == NULL) {
		show_oops_s("%s invalid parameters", __func__);
		return (FALSE);
	}

	filename = webkit_download_get_suggested_filename(wk_download);
	if (filename == NULL)
		return (FALSE); /* abort download */

	uri = g_strdup_printf("file://%s/%s", download_dir, filename);

	DNPRINTF(XT_D_DOWNLOAD, "%s: tab %d filename %s "
	    "local %s\n", __func__, t->tab_id, filename, uri);

	webkit_download_set_destination_uri(wk_download, uri);

	if (webkit_download_get_status(wk_download) ==
	    WEBKIT_DOWNLOAD_STATUS_ERROR) {
		show_oops(t, "%s: download failed to start", __func__);
		ret = FALSE;
		gtk_label_set_text(GTK_LABEL(t->label), "Download Failed");
	} else {
		download_entry = g_malloc(sizeof(struct download));
		download_entry->download = wk_download;
		download_entry->tab = t;
		download_entry->id = next_download_id++;
		RB_INSERT(download_list, &downloads, download_entry);
		/* get from history */
		g_object_ref(wk_download);
		gtk_label_set_text(GTK_LABEL(t->label), "Downloading");
	}

	if (uri)
		g_free(uri);

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
		show_oops_s("webview_hover_cb");
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
wv_keypress_after_cb(GtkWidget *w, GdkEventKey *e, struct tab *t)
{
	char			s[2], buf[128];
	const char		*errstr = NULL;
	long long		link;

	/* don't use w directly; use t->whatever instead */

	if (t == NULL) {
		show_oops_s("wv_keypress_after_cb");
		return (XT_CB_PASSTHROUGH);
	}

	DNPRINTF(XT_D_KEY, "wv_keypress_after_cb: keyval 0x%x mask 0x%x t %p\n",
	    e->keyval, e->state, t);

	if (t->hints_on) {
		/* ESC */
		if (CLEAN(e->state) == 0 && e->keyval == GDK_Escape) {
			disable_hints(t);
			return (XT_CB_HANDLED);
		}

		/* RETURN */
		if (CLEAN(e->state) == 0 && e->keyval == GDK_Return) {
			link = strtonum(t->hint_num, 1, 1000, &errstr);
			if (errstr) {
				/* we have a string */
			} else {
				/* we have a number */
				snprintf(buf, sizeof buf, "vimprobable_fire(%s)",
				    t->hint_num);
				run_script(t, buf);
			}
			disable_hints(t);
		}

		/* BACKSPACE */
		/* XXX unfuck this */
		if (CLEAN(e->state) == 0 && e->keyval == GDK_BackSpace) {
			if (t->hint_mode == XT_HINT_NUMERICAL) {
				/* last input was numerical */
				int		l;
				l = strlen(t->hint_num);
				if (l > 0) {
					l--;
					if (l == 0) {
						disable_hints(t);
						enable_hints(t);
					} else {
						t->hint_num[l] = '\0';
						goto num;
					}
				}
			} else if (t->hint_mode == XT_HINT_ALPHANUM) {
				/* last input was alphanumerical */
				int		l;
				l = strlen(t->hint_buf);
				if (l > 0) {
					l--;
					if (l == 0) {
						disable_hints(t);
						enable_hints(t);
					} else {
						t->hint_buf[l] = '\0';
						goto anum;
					}
				}
			} else {
				/* bogus */
				disable_hints(t);
			}
		}

		/* numerical input */
		if (CLEAN(e->state) == 0 &&
		    ((e->keyval >= GDK_0 && e->keyval <= GDK_9) || (e->keyval >= GDK_KP_0 && e->keyval <= GDK_KP_9))) {
			snprintf(s, sizeof s, "%c", e->keyval);
			strlcat(t->hint_num, s, sizeof t->hint_num);
			DNPRINTF(XT_D_JS, "wv_keypress_after_cb: numerical %s\n",
			    t->hint_num);
num:
			link = strtonum(t->hint_num, 1, 1000, &errstr);
			if (errstr) {
				DNPRINTF(XT_D_JS, "wv_keypress_after_cb: invalid link number\n");
				disable_hints(t);
			} else {
				snprintf(buf, sizeof buf, "vimprobable_update_hints(%s)",
				    t->hint_num);
				t->hint_mode = XT_HINT_NUMERICAL;
				run_script(t, buf);
			}

			/* empty the counter buffer */
			bzero(t->hint_buf, sizeof t->hint_buf);
			return (XT_CB_HANDLED);
		}

		/* alphanumerical input */
		if (
		    (CLEAN(e->state) == 0 && e->keyval >= GDK_a && e->keyval <= GDK_z) ||
		    (CLEAN(e->state) == GDK_SHIFT_MASK && e->keyval >= GDK_A && e->keyval <= GDK_Z) ||
		    (CLEAN(e->state) == 0 && ((e->keyval >= GDK_0 && e->keyval <= GDK_9) ||
		    ((e->keyval >= GDK_KP_0 && e->keyval <= GDK_KP_9) && (t->hint_mode != XT_HINT_NUMERICAL))))) {
			snprintf(s, sizeof s, "%c", e->keyval);
			strlcat(t->hint_buf, s, sizeof t->hint_buf);
			DNPRINTF(XT_D_JS, "wv_keypress_after_cb: alphanumerical %s\n",
			    t->hint_buf);
anum:
			snprintf(buf, sizeof buf, "vimprobable_cleanup()");
			run_script(t, buf);

			snprintf(buf, sizeof buf, "vimprobable_show_hints('%s')",
			    t->hint_buf);
			t->hint_mode = XT_HINT_ALPHANUM;
			run_script(t, buf);

			/* empty the counter buffer */
			bzero(t->hint_num, sizeof t->hint_num);
			return (XT_CB_HANDLED);
		}

		return (XT_CB_HANDLED);
	}

	struct key_binding	*k;
	TAILQ_FOREACH(k, &kbl, entry)
		if (e->keyval == k->key) {
			if (k->mask == 0) {
				if ((e->state & (CTRL | MOD1)) == 0) {
					k->func(t, &k->arg);
					return (XT_CB_HANDLED);
				}
			}
			else if ((e->state & k->mask) == k->mask) {
				k->func(t, &k->arg);
				return (XT_CB_HANDLED);
			}
		}

	return (XT_CB_PASSTHROUGH);
}

int
wv_keypress_cb(GtkEntry *w, GdkEventKey *e, struct tab *t)
{
	hide_oops(t);

	return (XT_CB_PASSTHROUGH);
}

int
cmd_keyrelease_cb(GtkEntry *w, GdkEventKey *e, struct tab *t)
{
	const gchar		*c = gtk_entry_get_text(w);
	GdkColor		color;
	int			forward = TRUE;

	DNPRINTF(XT_D_CMD, "cmd_keyrelease_cb: keyval 0x%x mask 0x%x t %p\n",
	    e->keyval, e->state, t);

	if (t == NULL) {
		show_oops_s("cmd_keyrelease_cb invalid parameters");
		return (XT_CB_PASSTHROUGH);
	}

	DNPRINTF(XT_D_CMD, "cmd_keyrelease_cb: keyval 0x%x mask 0x%x t %p\n",
	    e->keyval, e->state, t);

	if (c[0] == ':')
		goto done;
	if (strlen(c) == 1) {
		webkit_web_view_unmark_text_matches(t->wv);
		goto done;
	}

	if (c[0] == '/')
		forward = TRUE;
	else if (c[0] == '?')
		forward = FALSE;
	else
		goto done;

	/* search */
	if (webkit_web_view_search_text(t->wv, &c[1], FALSE, forward, TRUE) ==
	    FALSE) {
		/* not found, mark red */
		gdk_color_parse("red", &color);
		gtk_widget_modify_base(t->cmd, GTK_STATE_NORMAL, &color);
		/* unmark and remove selection */
		webkit_web_view_unmark_text_matches(t->wv);
		/* my kingdom for a way to unselect text in webview */
	} else {
		/* found, highlight all */
		webkit_web_view_unmark_text_matches(t->wv);
		webkit_web_view_mark_text_matches(t->wv, &c[1], FALSE, 0);
		webkit_web_view_set_highlight_text_matches(t->wv, TRUE);
		gdk_color_parse("white", &color);
		gtk_widget_modify_base(t->cmd, GTK_STATE_NORMAL, &color);
	}
done:
	return (XT_CB_PASSTHROUGH);
}

#if 0
int
cmd_complete(struct tab *t, char *s)
{
	int			i;
	GtkEntry		*w = GTK_ENTRY(t->cmd);

	DNPRINTF(XT_D_CMD, "cmd_keypress_cb: complete %s\n", s);

	for (i = 0; i < LENGTH(cmds); i++) {
		if (!strncasecmp(cmds[i].cmd, s, strlen(s))) {
			fprintf(stderr, "match %s %d\n", cmds[i].cmd, strcasecmp(cmds[i].cmd, s));
#if 0
			gtk_entry_set_text(w, ":");
			gtk_entry_append_text(w, cmds[i].cmd);
			gtk_editable_set_position(GTK_EDITABLE(w), -1);
#endif
		}
	}

	return (0);
}
#endif

int
entry_key_cb(GtkEntry *w, GdkEventKey *e, struct tab *t)
{
	if (t == NULL) {
		show_oops_s("entry_key_cb invalid parameters");
		return (XT_CB_PASSTHROUGH);
	}

	DNPRINTF(XT_D_CMD, "entry_key_cb: keyval 0x%x mask 0x%x t %p\n",
	    e->keyval, e->state, t);

	hide_oops(t);

	if (e->keyval == GDK_Escape) {
		/* don't use focus_webview(t) because we want to type :cmds */
		gtk_widget_grab_focus(GTK_WIDGET(t->wv));
	}

	struct key_binding	*k;
	TAILQ_FOREACH(k, &kbl, entry)
		if (e->keyval == k->key && k->use_in_entry) {
			if (k->mask == 0) {
				if ((e->state & (CTRL | MOD1)) == 0) {
					k->func(t, &k->arg);
					return (XT_CB_HANDLED);
				}
			}
			else if ((e->state & k->mask) == k->mask) {
				k->func(t, &k->arg);
				return (XT_CB_HANDLED);
			}
		}

	return (XT_CB_PASSTHROUGH);
}

int
cmd_keypress_cb(GtkEntry *w, GdkEventKey *e, struct tab *t)
{
	int			rv = XT_CB_HANDLED;
	const gchar		*c = gtk_entry_get_text(w);

	if (t == NULL) {
		show_oops_s("cmd_keypress_cb parameters");
		return (XT_CB_PASSTHROUGH);
	}

	DNPRINTF(XT_D_CMD, "cmd_keypress_cb: keyval 0x%x mask 0x%x t %p\n",
	    e->keyval, e->state, t);

	/* sanity */
	if (c == NULL)
		e->keyval = GDK_Escape;
	else if (!(c[0] == ':' || c[0] == '/' || c[0] == '?'))
		e->keyval = GDK_Escape;

	switch (e->keyval) {
#if 0
	case GDK_Tab:
		if (c[0] != ':')
			goto done;

		if (strchr (c, ' ')) {
			/* par completion */
			fprintf(stderr, "completeme par\n");
			goto done;
		}

		cmd_complete(t, (char *)&c[1]);

		goto done;
#endif
	case GDK_BackSpace:
		if (!(!strcmp(c, ":") || !strcmp(c, "/") || !strcmp(c, "?")))
			break;
		/* FALLTHROUGH */
	case GDK_Escape:
		hide_cmd(t);
		focus_webview(t);

		/* cancel search */
		if (c[0] == '/' || c[0] == '?')
			webkit_web_view_unmark_text_matches(t->wv);
		goto done;
	}

	rv = XT_CB_PASSTHROUGH;
done:
	return (rv);
}

int
cmd_focusout_cb(GtkWidget *w, GdkEventFocus *e, struct tab *t)
{
	if (t == NULL) {
		show_oops_s("cmd_focusout_cb invalid parameters");
		return (XT_CB_PASSTHROUGH);
	}
	DNPRINTF(XT_D_CMD, "cmd_focusout_cb: tab %d\n", t->tab_id);

	hide_cmd(t);
	hide_oops(t);

	if (show_url == 0 || t->focus_wv)
		focus_webview(t);
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

	if (t == NULL) {
		show_oops_s("cmd_activate_cb invalid parameters");
		return;
	}

	DNPRINTF(XT_D_CMD, "cmd_activate_cb: tab %d %s\n", t->tab_id, c);

	/* sanity */
	if (c == NULL)
		goto done;
	else if (!(c[0] == ':' || c[0] == '/' || c[0] == '?'))
		goto done;
	if (strlen(c) < 2)
		goto done;
	s = (char *)&c[1];

	if (c[0] == '/' || c[0] == '?') {
		if (t->search_text) {
			g_free(t->search_text);
			t->search_text = NULL;
		}

		t->search_text = g_strdup(s);
		if (global_search)
			g_free(global_search);
		global_search = g_strdup(s);
		t->search_forward = c[0] == '/';

		goto done;
	}

	for (i = 0; i < LENGTH(cmds); i++)
		if (cmds[i].params) {
			if (!strncmp(s, cmds[i].cmd, strlen(cmds[i].cmd))) {
				cmds[i].arg.s = g_strdup(s);
				goto execute_command;
			}
		} else {
			if (!strcmp(s, cmds[i].cmd))
				goto execute_command;
		}
	show_oops(t, "Invalid command: %s", s);
done:
	hide_cmd(t);
	return;

execute_command:
	hide_cmd(t);
	cmds[i].func(t, &cmds[i].arg);
}
void
backward_cb(GtkWidget *w, struct tab *t)
{
	struct karg		a;

	if (t == NULL) {
		show_oops_s("backward_cb invalid parameters");
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
		show_oops_s("forward_cb invalid parameters");
		return;
	}

	DNPRINTF(XT_D_NAV, "forward_cb: tab %d\n", t->tab_id);

	a.i = XT_NAV_FORWARD;
	navaction(t, &a);
}

void
stop_cb(GtkWidget *w, struct tab *t)
{
	WebKitWebFrame		*frame;

	if (t == NULL) {
		show_oops_s("stop_cb invalid parameters");
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
	g_object_set(G_OBJECT(t->settings),
	    "user-agent", t->user_agent, (char *)NULL);
	g_object_set(G_OBJECT(t->settings),
	    "enable-scripts", enable_scripts, (char *)NULL);
	g_object_set(G_OBJECT(t->settings),
	    "enable-plugins", enable_plugins, (char *)NULL);
	adjustfont_webkit(t, XT_FONT_SET);

	webkit_web_view_set_settings(t->wv, t->settings);
}

GtkWidget *
create_browser(struct tab *t)
{
	GtkWidget		*w;
	gchar			*strval;

	if (t == NULL) {
		show_oops_s("create_browser invalid parameters");
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

	if (user_agent == NULL) {
		g_object_get(G_OBJECT(t->settings), "user-agent", &strval,
		    (char *)NULL);
		t->user_agent = g_strdup_printf("%s %s+", strval, version);
		g_free(strval);
	} else {
		t->user_agent = g_strdup(user_agent);
	}

	setup_webkit(t);

	return (w);
}

GtkWidget *
create_window(void)
{
	GtkWidget		*w;

	w = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_default_size(GTK_WINDOW(w), window_width, window_height);
	gtk_widget_set_name(w, "xxxterm");
	gtk_window_set_wmclass(GTK_WINDOW(w), "xxxterm", "XXXTerm");
	g_signal_connect(G_OBJECT(w), "delete_event",
	    G_CALLBACK (gtk_main_quit), NULL);

	return (w);
}

GtkWidget *
create_toolbar(struct tab *t)
{
	GtkWidget		*toolbar = NULL, *b, *eb1;

	b = gtk_hbox_new(FALSE, 0);
	toolbar = b;
	gtk_container_set_border_width(GTK_CONTAINER(toolbar), 0);

	if (fancy_bar) {
		/* backward button */
		t->backward = create_button("GoBack", GTK_STOCK_GO_BACK, 0);
		gtk_widget_set_sensitive(t->backward, FALSE);
		g_signal_connect(G_OBJECT(t->backward), "clicked",
		    G_CALLBACK(backward_cb), t);
		gtk_box_pack_start(GTK_BOX(b), t->backward, FALSE, FALSE, 0);

		/* forward button */
		t->forward = create_button("GoForward",GTK_STOCK_GO_FORWARD, 0);
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
	}

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
	if (fancy_bar && search_string) {
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

void
recalc_tabs(void)
{
	struct tab		*t;

	TAILQ_FOREACH(t, &tabs, entry)
		t->tab_id = gtk_notebook_page_num(notebook, t->vbox);
}

int
undo_close_tab_save(struct tab *t)
{
	int				m, n;
	const gchar			*uri;
	struct undo			*u1, *u2;
	GList				*items;
	WebKitWebHistoryItem		*item;

	if ((uri = get_uri(t->wv)) == NULL)
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

	TAILQ_REMOVE(&tabs, t, entry);

	/* halt all webkit activity */
	abort_favicon_download(t);
	webkit_web_view_stop_loading(t->wv);
	undo_close_tab_save(t);

	gtk_widget_destroy(t->vbox);
	g_free(t->user_agent);
	g_free(t);

	recalc_tabs();
	if (TAILQ_EMPTY(&tabs))
		create_new_tab(NULL, NULL, 1);


	/* recreate session */
	if (session_autosave) {
		a.s = NULL;
		save_tabs(t, &a);
	}
}

void
adjustfont_webkit(struct tab *t, int adjust)
{
	if (t == NULL) {
		show_oops_s("adjustfont_webkit invalid parameters");
		return;
	}

	if (adjust == XT_FONT_SET)
		t->font_size = default_font_size;

	t->font_size += adjust;
	g_object_set(G_OBJECT(t->settings), "default-font-size",
	    t->font_size, (char *)NULL);
	g_object_get(G_OBJECT(t->settings), "default-font-size",
	    &t->font_size, (char *)NULL);
}

void
append_tab(struct tab *t)
{
	if (t == NULL)
		return;

	TAILQ_INSERT_TAIL(&tabs, t, entry);
	t->tab_id = gtk_notebook_append_page(notebook, t->vbox, t->tab_content);
}

void
create_new_tab(char *title, struct undo *u, int focus)
{
	struct tab			*t, *tt;
	int				load = 1, id, notfound;
	GtkWidget			*b, *bb;
	WebKitWebHistoryItem		*item;
	GList				*items;
	GdkColor			color;
	PangoFontDescription		*fd = NULL;

	DNPRINTF(XT_D_TAB, "create_new_tab: title %s focus %d\n", title, focus);

	if (tabless && !TAILQ_EMPTY(&tabs)) {
		DNPRINTF(XT_D_TAB, "create_new_tab: new tab rejected\n");
		return;
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
	t->spinner = gtk_spinner_new ();
#endif
	t->label = gtk_label_new(title);
	bb = create_button("Close", GTK_STOCK_CLOSE, 1);
	gtk_widget_set_size_request(t->label, 100, 0);
	gtk_widget_set_size_request(b, 130, 0);

	gtk_box_pack_start(GTK_BOX(b), bb, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(b), t->label, FALSE, FALSE, 0);
#if GTK_CHECK_VERSION(2, 20, 0)
	gtk_box_pack_start(GTK_BOX(b), t->spinner, FALSE, FALSE, 0);
#endif

	/* toolbar */
	t->toolbar = create_toolbar(t);
	gtk_box_pack_start(GTK_BOX(t->vbox), t->toolbar, FALSE, FALSE, 0);

	/* browser */
	t->browser_win = create_browser(t);
	gtk_box_pack_start(GTK_BOX(t->vbox), t->browser_win, TRUE, TRUE, 0);

	/* oops message for user feedback */
	t->oops = gtk_entry_new();
	gtk_entry_set_inner_border(GTK_ENTRY(t->oops), NULL);
	gtk_entry_set_has_frame(GTK_ENTRY(t->oops), FALSE);
	gtk_widget_set_can_focus(GTK_WIDGET(t->oops), FALSE);
	gdk_color_parse("red", &color);
	gtk_widget_modify_base(t->oops, GTK_STATE_NORMAL, &color);
	gtk_box_pack_end(GTK_BOX(t->vbox), t->oops, FALSE, FALSE, 0);

	/* command entry */
	t->cmd = gtk_entry_new();
	gtk_entry_set_inner_border(GTK_ENTRY(t->cmd), NULL);
	gtk_entry_set_has_frame(GTK_ENTRY(t->cmd), FALSE);
	gtk_box_pack_end(GTK_BOX(t->vbox), t->cmd, FALSE, FALSE, 0);

	/* status bar */
	t->statusbar = gtk_entry_new();
	gtk_entry_set_inner_border(GTK_ENTRY(t->statusbar), NULL);
	gtk_entry_set_has_frame(GTK_ENTRY(t->statusbar), FALSE);
	gtk_widget_set_can_focus(GTK_WIDGET(t->statusbar), FALSE);
	gdk_color_parse("black", &color);
	gtk_widget_modify_base(t->statusbar, GTK_STATE_NORMAL, &color);
	gdk_color_parse("white", &color);
	gtk_widget_modify_text(t->statusbar, GTK_STATE_NORMAL, &color);
	fd = GTK_WIDGET(t->statusbar)->style->font_desc;
	pango_font_description_set_weight(fd, PANGO_WEIGHT_SEMIBOLD);
	pango_font_description_set_absolute_size(fd, 10 * PANGO_SCALE); /* 10 px font */
	gtk_widget_modify_font(t->statusbar, fd);
	gtk_box_pack_end(GTK_BOX(t->vbox), t->statusbar, FALSE, FALSE, 0);

	/* xtp meaning is normal by default */
	t->xtp_meaning = XT_XTP_TAB_MEANING_NORMAL;

	/* set empty favicon */
	xt_icon_from_name(t, "text-html");

	/* and show it all */
	gtk_widget_show_all(b);
	gtk_widget_show_all(t->vbox);

	if (append_next == 0 || gtk_notebook_get_n_pages(notebook) == 0)
		append_tab(t);
	else {
		notfound = 1;
		id = gtk_notebook_get_current_page(notebook);
		TAILQ_FOREACH(tt, &tabs, entry) {
			if (tt->tab_id == id) {
				notfound = 0;
				TAILQ_INSERT_AFTER(&tabs, tt, t, entry);
				gtk_notebook_insert_page(notebook, t->vbox, b,
				    id + 1);
				recalc_tabs();
				break;
			}
		}
		if (notfound)
			append_tab(t);
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

	g_object_connect(G_OBJECT(t->cmd),
	    "signal::key-press-event", G_CALLBACK(cmd_keypress_cb), t,
	    "signal::key-release-event", G_CALLBACK(cmd_keyrelease_cb), t,
	    "signal::focus-out-event", G_CALLBACK(cmd_focusout_cb), t,
	    "signal::activate", G_CALLBACK(cmd_activate_cb), t,
	    (char *)NULL);

	/* reuse wv_button_cb to hide oops */
	g_object_connect(G_OBJECT(t->oops),
	    "signal::button_press_event", G_CALLBACK(wv_button_cb), t,
	    (char *)NULL);

	g_object_connect(G_OBJECT(t->wv),
	    "signal::key-press-event", G_CALLBACK(wv_keypress_cb), t,
	    "signal-after::key-press-event", G_CALLBACK(wv_keypress_after_cb), t,
	    "signal::hovering-over-link", G_CALLBACK(webview_hover_cb), t,
	    "signal::download-requested", G_CALLBACK(webview_download_cb), t,
	    "signal::mime-type-policy-decision-requested", G_CALLBACK(webview_mimetype_cb), t,
	    "signal::navigation-policy-decision-requested", G_CALLBACK(webview_npd_cb), t,
	    "signal::new-window-policy-decision-requested", G_CALLBACK(webview_nw_cb), t,
	    "signal::create-web-view", G_CALLBACK(webview_cwv_cb), t,
	    "signal::event", G_CALLBACK(webview_event_cb), t,
	    "signal::load-finished", G_CALLBACK(webview_load_finished_cb), t,
	    "signal::load-progress-changed", G_CALLBACK(webview_progress_changed_cb), t,
#if WEBKIT_CHECK_VERSION(1, 1, 18)
	    "signal::icon-loaded", G_CALLBACK(notify_icon_loaded_cb), t,
#endif
	    "signal::button_press_event", G_CALLBACK(wv_button_cb), t,
	    (char *)NULL);
	g_signal_connect(t->wv,
	    "notify::load-status", G_CALLBACK(notify_load_status_cb), t);

	/* hijack the unused keys as if we were the browser */
	g_object_connect(G_OBJECT(t->toolbar),
	    "signal-after::key-press-event", G_CALLBACK(wv_keypress_after_cb), t,
	    (char *)NULL);

	g_signal_connect(G_OBJECT(bb), "button_press_event",
	    G_CALLBACK(tab_close_cb), t);

	/* hide stuff */
	hide_cmd(t);
	hide_oops(t);
	url_set_visibility();
	statusbar_set_visibility();

	if (focus) {
		gtk_notebook_set_current_page(notebook, t->tab_id);
		DNPRINTF(XT_D_TAB, "create_new_tab: going to tab: %d\n",
		    t->tab_id);
	}

	if (load) {
		gtk_entry_set_text(GTK_ENTRY(t->uri_entry), title);
		load_uri(t, title);
	} else {
		if (show_url == 1)
			gtk_widget_grab_focus(GTK_WIDGET(t->uri_entry));
		else
			focus_webview(t);
	}

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
}

void
notebook_switchpage_cb(GtkNotebook *nb, GtkNotebookPage *nbp, guint pn,
    gpointer *udata)
{
	struct tab		*t;
	const gchar		*uri;

	DNPRINTF(XT_D_TAB, "notebook_switchpage_cb: tab: %d\n", pn);

	TAILQ_FOREACH(t, &tabs, entry) {
		if (t->tab_id == pn) {
			DNPRINTF(XT_D_TAB, "notebook_switchpage_cb: going to "
			    "%d\n", pn);

			uri = webkit_web_view_get_title(t->wv);
			if (uri == NULL)
				uri = XT_NAME;
			gtk_window_set_title(GTK_WINDOW(main_window), uri);

			hide_cmd(t);
			hide_oops(t);

			if (t->focus_wv)
				focus_webview(t);
		}
	}
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
			if ((uri = get_uri(ti->wv)) == NULL)
				/* XXX make sure there is something to print */
				/* XXX add gui pages in here to look purdy */
				uri = "(untitled)";
			menu_items = gtk_menu_item_new_with_label(uri);
			gtk_menu_append(GTK_MENU (menu), menu_items);
			gtk_widget_show(menu_items);

			gtk_signal_connect_object(GTK_OBJECT(menu_items),
			    "activate", GTK_SIGNAL_FUNC(menuitem_response),
			    (gpointer)ti);
		}

		gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL,
		    bevent->button, bevent->time);

		/* unref object so it'll free itself when popped down */
		g_object_ref_sink(menu);
		g_object_unref(menu);

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
	GtkWidget *button, *image;
	gchar *rcstring;
	int gtk_icon_size;
	rcstring = g_strdup_printf(
	    "style \"%s-style\"\n"
	    "{\n"
	    "  GtkWidget::focus-padding = 0\n"
	    "  GtkWidget::focus-line-width = 0\n"
	    "  xthickness = 0\n"
	    "  ythickness = 0\n"
	    "}\n"
	    "widget \"*.%s\" style \"%s-style\"",name,name,name);
	gtk_rc_parse_string(rcstring);
	g_free(rcstring);
	button = gtk_button_new();
	gtk_button_set_focus_on_click(GTK_BUTTON(button), FALSE);
	gtk_icon_size = icon_size_map(size?size:icon_size);

	image = gtk_image_new_from_stock(stockid, gtk_icon_size);
	gtk_widget_set_size_request(GTK_WIDGET(image), -1, -1);
	gtk_container_set_border_width(GTK_CONTAINER(button), 1);
	gtk_container_add(GTK_CONTAINER(button), GTK_WIDGET(image));
	gtk_widget_set_name(button, name);
	gtk_button_set_relief(GTK_BUTTON(button), GTK_RELIEF_NONE);
	gtk_widget_set_tooltip_text(button, name);

	return button;
}

void
button_set_stockid(GtkWidget *button, char *stockid)
{
	GtkWidget *image;
	image = gtk_image_new_from_stock(stockid, icon_size_map(icon_size));
	gtk_widget_set_size_request(GTK_WIDGET(image), -1, -1);
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

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_box_set_spacing(GTK_BOX(vbox), 0);
	notebook = GTK_NOTEBOOK(gtk_notebook_new());
	gtk_notebook_set_tab_hborder(notebook, 0);
	gtk_notebook_set_tab_vborder(notebook, 0);
	gtk_notebook_set_scrollable(notebook, TRUE);
	notebook_tab_set_visibility(notebook);
	gtk_notebook_set_show_border(notebook, FALSE);
	gtk_widget_set_can_focus(GTK_WIDGET(notebook), FALSE);

	abtn = gtk_button_new();
	arrow = gtk_arrow_new(GTK_ARROW_DOWN, GTK_SHADOW_NONE);
	gtk_widget_set_size_request(arrow, -1, -1);
	gtk_container_add(GTK_CONTAINER(abtn), arrow);
	gtk_widget_set_size_request(abtn, -1, 20);
	gtk_notebook_set_action_widget(notebook, abtn, GTK_PACK_END);

	gtk_widget_set_size_request(GTK_WIDGET(notebook), -1, -1);
	gtk_box_pack_start(GTK_BOX(vbox), GTK_WIDGET(notebook), TRUE, TRUE, 0);
	gtk_widget_set_size_request(vbox, -1, -1);

	g_object_connect(G_OBJECT(notebook),
	    "signal::switch-page", G_CALLBACK(notebook_switchpage_cb), NULL,
	    (char *)NULL);
	g_signal_connect(G_OBJECT(abtn), "button_press_event",
	    G_CALLBACK(arrow_cb), NULL);

	main_window = create_window();
	gtk_container_add(GTK_CONTAINER(main_window), vbox);
	gtk_window_set_title(GTK_WINDOW(main_window), XT_NAME);

	/* icons */
	for (i = 0; i < LENGTH(icons); i++) {
		snprintf(file, sizeof file, "%s/%s", resource_dir, icons[i]);
		pb = gdk_pixbuf_new_from_file(file, NULL);
		l = g_list_append(l, pb);
	}
	gtk_window_set_default_icon_list(l);

	gtk_widget_show_all(abtn);
	gtk_widget_show_all(main_window);
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

	if (enable_cookie_whitelist &&
	    (d = wl_find(cookie->domain, &c_wl)) == NULL) {
		blocked_cookies++;
		DNPRINTF(XT_D_COOKIE,
		    "soup_cookie_jar_add_cookie: reject %s\n",
		    cookie->domain);
		if (save_rejected_cookies) {
			if ((r_cookie_f = fopen(rc_fname, "a+")) == NULL) {
				show_oops_s("can't open reject cookie file");
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
		snprintf(rc_fname, sizeof file, "%s/rejected.txt", work_dir);

	/* persistent cookies */
	snprintf(file, sizeof file, "%s/cookies.txt", work_dir);
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
		g_object_set(session, "proxy-uri", proxy_uri, (char *)NULL);
	}
}

int
send_url_to_socket(char *url)
{
	int			s, len, rv = -1;
	struct sockaddr_un	sa;

	if ((s = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
		warnx("send_url_to_socket: socket");
		return (-1);
	}

	sa.sun_family = AF_UNIX;
	snprintf(sa.sun_path, sizeof(sa.sun_path), "%s/%s",
	    work_dir, XT_SOCKET_FILE);
	len = SUN_LEN(&sa);

	if (connect(s, (struct sockaddr *)&sa, len) == -1) {
		warnx("send_url_to_socket: connect");
		goto done;
	}

	if (send(s, url, strlen(url) + 1, 0) == -1) {
		warnx("send_url_to_socket: send");
		goto done;
	}
done:
	close(s);
	return (rv);
}

void
socket_watcher(gpointer data, gint fd, GdkInputCondition cond)
{
	int			s, n;
	char			str[XT_MAX_URL_LENGTH];
	socklen_t		t = sizeof(struct sockaddr_un);
	struct sockaddr_un	sa;
	struct passwd		*p;
	uid_t			uid;
	gid_t			gid;

	if ((s = accept(fd, (struct sockaddr *)&sa, &t)) == -1) {
		warn("socket_watcher: accept");
		return;
	}

	if (getpeereid(s, &uid, &gid) == -1) {
		warn("socket_watcher: getpeereid");
		return;
	}
	if (uid != getuid() || gid != getgid()) {
		warnx("socket_watcher: unauthorized user");
		return;
	}

	p = getpwuid(uid);
	if (p == NULL) {
		warnx("socket_watcher: not a valid user");
		return;
	}

	n = recv(s, str, sizeof(str), 0);
	if (n <= 0)
		return;

	create_new_tab(str, NULL, 1);
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

static gboolean
completion_select_cb(GtkEntryCompletion *widget, GtkTreeModel *model,
    GtkTreeIter *iter, struct tab *t)
{
	gchar			*value;

	gtk_tree_model_get(model, iter, 0, &value, -1);
	load_uri(t, value);

	return (FALSE);
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
	gchar			*value, *voffset;
	size_t			len;
	gboolean		match = FALSE;

	len = strlen(key);

	gtk_tree_model_get(GTK_TREE_MODEL(completion_model), iter, 0, &value,
	    -1);

	if (value == NULL)
		return FALSE;

	if (!strncmp(key, value, len))
		match = TRUE;
	else {
		voffset = strstr(value, "/") + 2;
		if (!strncmp(key, voffset, len))
			match = TRUE;
		else if (g_str_has_prefix(voffset, "www.")) {
		    voffset = voffset + strlen("www.");
		    if (!strncmp(key, voffset, len))
				match = TRUE;
		}
	}

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
	g_signal_connect(G_OBJECT (t->completion), "match-selected",
	    G_CALLBACK(completion_select_cb), t);
}

void
usage(void)
{
	fprintf(stderr,
	    "%s [-nSTVt][-f file][-s session] url ...\n", __progname);
	exit(0);
}

int
main(int argc, char *argv[])
{
	struct stat		sb;
	int			c, s, optn = 0, focus = 1;
	char			conf[PATH_MAX] = { '\0' };
	char			file[PATH_MAX];
	char			*env_proxy = NULL;
	FILE			*f = NULL;
	struct karg		a;
	struct sigaction	sact;

	start_argv = argv;

	strlcpy(named_session, XT_SAVED_TABS_FILE, sizeof named_session);

	while ((c = getopt(argc, argv, "STVf:s:tn")) != -1) {
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
		default:
			usage();
			/* NOTREACHED */
		}
	}
	argc -= optind;
	argv += optind;

	RB_INIT(&hl);
	RB_INIT(&js_wl);
	RB_INIT(&downloads);

	TAILQ_INIT(&tabs);
	TAILQ_INIT(&mtl);
	TAILQ_INIT(&aliases);
	TAILQ_INIT(&undos);
	TAILQ_INIT(&kbl);

	init_keybindings();

	gnutls_global_init();

	/* generate session keys for xtp pages */
	generate_xtp_session_key(&dl_session_key);
	generate_xtp_session_key(&hl_session_key);
	generate_xtp_session_key(&cl_session_key);
	generate_xtp_session_key(&fl_session_key);

	/* prepare gtk */
	gtk_init(&argc, &argv);
	if (!g_thread_supported())
		g_thread_init(NULL);

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

	/* set default string settings */
	home = g_strdup("http://www.peereboom.us");
	search_string = g_strdup("https://ssl.scroogle.org/cgi-bin/nbbwssl.cgi?Gw=%s");
	resource_dir = g_strdup("/usr/local/share/xxxterm/");
	strlcpy(runtime_settings,"runtime", sizeof runtime_settings);

	/* read config file */
	if (strlen(conf) == 0)
		snprintf(conf, sizeof conf, "%s/.%s",
		    pwd->pw_dir, XT_CONF_FILE);
	config_parse(conf, 0);

	/* working directory */
	if (strlen(work_dir) == 0)
		snprintf(work_dir, sizeof work_dir, "%s/%s",
		    pwd->pw_dir, XT_DIR);
	if (stat(work_dir, &sb)) {
		if (mkdir(work_dir, S_IRWXU) == -1)
			err(1, "mkdir work_dir");
		if (stat(work_dir, &sb))
			err(1, "stat work_dir");
	}
	if (S_ISDIR(sb.st_mode) == 0)
		errx(1, "%s not a dir", work_dir);
	if (((sb.st_mode & (S_IRWXU | S_IRWXG | S_IRWXO))) != S_IRWXU) {
		warnx("fixing invalid permissions on %s", work_dir);
		if (chmod(work_dir, S_IRWXU) == -1)
			err(1, "chmod");
	}

	/* icon cache dir */
	snprintf(cache_dir, sizeof cache_dir, "%s/%s", work_dir, XT_CACHE_DIR);
	if (stat(cache_dir, &sb)) {
		if (mkdir(cache_dir, S_IRWXU) == -1)
			err(1, "mkdir cache_dir");
		if (stat(cache_dir, &sb))
			err(1, "stat cache_dir");
	}
	if (S_ISDIR(sb.st_mode) == 0)
		errx(1, "%s not a dir", cache_dir);
	if (((sb.st_mode & (S_IRWXU | S_IRWXG | S_IRWXO))) != S_IRWXU) {
		warnx("fixing invalid permissions on %s", cache_dir);
		if (chmod(cache_dir, S_IRWXU) == -1)
			err(1, "chmod");
	}

	/* certs dir */
	snprintf(certs_dir, sizeof certs_dir, "%s/%s", work_dir, XT_CERT_DIR);
	if (stat(certs_dir, &sb)) {
		if (mkdir(certs_dir, S_IRWXU) == -1)
			err(1, "mkdir certs_dir");
		if (stat(certs_dir, &sb))
			err(1, "stat certs_dir");
	}
	if (S_ISDIR(sb.st_mode) == 0)
		errx(1, "%s not a dir", certs_dir);
	if (((sb.st_mode & (S_IRWXU | S_IRWXG | S_IRWXO))) != S_IRWXU) {
		warnx("fixing invalid permissions on %s", certs_dir);
		if (chmod(certs_dir, S_IRWXU) == -1)
			err(1, "chmod");
	}

	/* sessions dir */
	snprintf(sessions_dir, sizeof sessions_dir, "%s/%s",
	    work_dir, XT_SESSIONS_DIR);
	if (stat(sessions_dir, &sb)) {
		if (mkdir(sessions_dir, S_IRWXU) == -1)
			err(1, "mkdir sessions_dir");
		if (stat(sessions_dir, &sb))
			err(1, "stat sessions_dir");
	}
	if (S_ISDIR(sb.st_mode) == 0)
		errx(1, "%s not a dir", sessions_dir);
	if (((sb.st_mode & (S_IRWXU | S_IRWXG | S_IRWXO))) != S_IRWXU) {
		warnx("fixing invalid permissions on %s", sessions_dir);
		if (chmod(sessions_dir, S_IRWXU) == -1)
			err(1, "chmod");
	}
	/* runtime settings that can override config file */
	if (runtime_settings[0] != '\0')
		config_parse(runtime_settings, 1);

	/* download dir */
	if (!strcmp(download_dir, pwd->pw_dir))
		strlcat(download_dir, "/downloads", sizeof download_dir);
	if (stat(download_dir, &sb)) {
		if (mkdir(download_dir, S_IRWXU) == -1)
			err(1, "mkdir download_dir");
		if (stat(download_dir, &sb))
			err(1, "stat download_dir");
	}
	if (S_ISDIR(sb.st_mode) == 0)
		errx(1, "%s not a dir", download_dir);
	if (((sb.st_mode & (S_IRWXU | S_IRWXG | S_IRWXO))) != S_IRWXU) {
		warnx("fixing invalid permissions on %s", download_dir);
		if (chmod(download_dir, S_IRWXU) == -1)
			err(1, "chmod");
	}

	/* favorites file */
	snprintf(file, sizeof file, "%s/%s", work_dir, XT_FAVS_FILE);
	if (stat(file, &sb)) {
		warnx("favorites file doesn't exist, creating it");
		if ((f = fopen(file, "w")) == NULL)
			err(1, "favorites");
		fclose(f);
	}

	/* cookies */
	session = webkit_get_default_session();
	setup_cookies();

	/* certs */
	if (ssl_ca_file) {
		if (stat(ssl_ca_file, &sb)) {
			warn("no CA file: %s", ssl_ca_file);
			g_free(ssl_ca_file);
			ssl_ca_file = NULL;
		} else
			g_object_set(session,
			    SOUP_SESSION_SSL_CA_FILE, ssl_ca_file,
			    SOUP_SESSION_SSL_STRICT, ssl_strict_certs,
			    (void *)NULL);
	}

	/* proxy */
	env_proxy = getenv("http_proxy");
	if (env_proxy)
		setup_proxy(env_proxy);
	else
		setup_proxy(http_proxy);

	/* see if there is already an xxxterm running */
	if (single_instance && is_running()) {
		optn = 1;
		warnx("already running");
	}

	if (optn) {
		while (argc) {
			send_url_to_socket(argv[0]);

			argc--;
			argv++;
		}
		exit(0);
	}

	/* uri completion */
	completion_model = gtk_list_store_new(1, G_TYPE_STRING);

	/* go graphical */
	create_canvas();

	if (save_global_history)
		restore_global_history();

	if (!strcmp(named_session, XT_SAVED_TABS_FILE))
		restore_saved_tabs();
	else {
		a.s = named_session;
		a.i = XT_SES_DONOTHING;
		open_tabs(NULL, &a);
	}

	while (argc) {
		create_new_tab(argv[0], NULL, focus);
		focus = 0;

		argc--;
		argv++;
	}

	if (TAILQ_EMPTY(&tabs))
		create_new_tab(home, NULL, 1);

	if (enable_socket)
		if ((s = build_socket()) != -1)
			gdk_input_add(s, GDK_INPUT_READ, socket_watcher, NULL);

	gtk_main();

	gnutls_global_deinit();

	return (0);
}
