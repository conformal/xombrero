/*
 * Copyright (c) 2011 Conformal Systems LLC <info@conformal.com>
 * Copyright (c) 2011 Marco Peereboom <marco@peereboom.us>
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

#include <ctype.h>
#include <dlfcn.h>
#include <err.h>
#include <errno.h>
#include <libgen.h>
#include <pwd.h>
#include <regex.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>

#include <sys/types.h>
#include <sys/wait.h>
#if defined(__linux__)
#include "linux/util.h"
#include "linux/tree.h"
#include <bsd/stdlib.h>
# if !defined(sane_libbsd_headers)
void arc4random_buf(void *, size_t);
# endif
#elif defined(__FreeBSD__)
#include <libutil.h>
#include "freebsd/util.h"
#include <sys/tree.h>
#else /* OpenBSD */
#include <util.h>
#include <sys/tree.h>
#endif
#include <sys/queue.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/un.h>

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#if GTK_CHECK_VERSION(3,0,0)
/* we still use GDK_* instead of GDK_KEY_* */
#include <gdk/gdkkeysyms-compat.h>
#endif

#include <webkit/webkit.h>
#include <libsoup/soup.h>
#include <JavaScriptCore/JavaScript.h>
#include <gnutls/gnutls.h>
#include <gnutls/x509.h>

/* comment if you don't want to use threads */
#define USE_THREADS

#ifdef USE_THREADS
#include <gcrypt.h>
#include <pthread.h>
#endif

#include "version.h"
#include "javascript.h"
/*
javascript.h borrowed from vimprobable2 under the following license:

Copyright (c) 2009 Leon Winter
Copyright (c) 2009-2011 Hannes Schueller
Copyright (c) 2009-2010 Matto Fransen
Copyright (c) 2010-2011 Hans-Peter Deifel
Copyright (c) 2010-2011 Thomas Adam
Copyright (c) 2011 Albert Kim
Copyright (c) 2011 Daniel Carl

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

/*#define XT_DEBUG*/
#ifdef XT_DEBUG
#define DPRINTF(x...)		do { if (swm_debug) fprintf(stderr, x); } while (0)
#define DNPRINTF(n,x...)	do { if (swm_debug & n) fprintf(stderr, x); } while (0)
#define XT_D_MOVE		0x0001
#define XT_D_KEY		0x0002
#define XT_D_TAB		0x0004
#define XT_D_URL		0x0008
#define XT_D_CMD		0x0010
#define XT_D_NAV		0x0020
#define XT_D_DOWNLOAD		0x0040
#define XT_D_CONFIG		0x0080
#define XT_D_JS			0x0100
#define XT_D_FAVORITE		0x0200
#define XT_D_PRINTING		0x0400
#define XT_D_COOKIE		0x0800
#define XT_D_KEYBINDING		0x1000
#define XT_D_CLIP		0x2000
#define XT_D_BUFFERCMD		0x4000
#define XT_D_INSPECTOR		0x8000
extern u_int32_t	swm_debug;
#else
#define DPRINTF(x...)
#define DNPRINTF(n,x...)
#endif

/* defines */
#define XT_NAME			("XXXTerm")
#define XT_DIR			(".xxxterm")
#define XT_CACHE_DIR		("cache")
#define XT_CERT_DIR		("certs/")
#define XT_SESSIONS_DIR		("sessions/")
#define XT_CONF_FILE		("xxxterm.conf")
#define XT_FAVS_FILE		("favorites")
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
#define XT_CB_HANDLED		(TRUE)
#define XT_CB_PASSTHROUGH	(FALSE)
#define XT_DOCTYPE		"<!DOCTYPE html PUBLIC '-//W3C//DTD XHTML 1.0 Transitional//EN' 'http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd'>\n"
#define XT_HTML_TAG		"<html xmlns='http://www.w3.org/1999/xhtml'>\n"
#define XT_DLMAN_REFRESH	"10"
#define XT_PAGE_STYLE		"<style type='text/css'>\n"		\
				"td{overflow: hidden;"			\
				" padding: 2px 2px 2px 2px;"		\
				" border: 1px solid black;"		\
				" vertical-align:top;"			\
				" word-wrap: break-word}\n"		\
				"tr:hover{background: #ffff99}\n"	\
				"th{background-color: #cccccc;"		\
				" border: 1px solid black}\n"		\
				"table{width: 100%%;"			\
				" border: 1px black solid;"		\
				" border-collapse:collapse}\n"		\
				".progress-outer{"			\
				"border: 1px solid black;"		\
				" height: 8px;"				\
				" width: 90%%}\n"			\
				".progress-inner{float: left;"		\
				" height: 8px;"				\
				" background: green}\n"			\
				".dlstatus{font-size: small;"		\
				" text-align: center}\n"		\
				"</style>\n"
#define XT_MAX_URL_LENGTH	(4096) /* 1 page is atomic, don't make bigger */
#define XT_MAX_UNDO_CLOSE_TAB	(32)
#define XT_RESERVED_CHARS	"$&+,/:;=?@ \"<>#%%{}|^~[]`"
#define XT_PRINT_EXTRA_MARGIN	10
#define XT_URL_REGEX		("^[[:blank:]]*[^[:blank:]]*([[:alnum:]-]+\\.)+[[:alnum:]-][^[:blank:]]*[[:blank:]]*$")
#define XT_INVALID_MARK		(-1) /* XXX this is a double, maybe use something else, like a nan */


#define LENGTH(x)		(sizeof x / sizeof x[0])
#define CLEAN(mask)		(mask & ~(GDK_MOD2_MASK) &	\
				    ~(GDK_BUTTON1_MASK) &	\
				    ~(GDK_BUTTON2_MASK) &	\
				    ~(GDK_BUTTON3_MASK) &	\
				    ~(GDK_BUTTON4_MASK) &	\
				    ~(GDK_BUTTON5_MASK))

#define XT_NOMARKS		(('z' - 'a' + 1) * 2 + 10)

#define XT_BM_NORMAL		(0)
#define XT_BM_WHITELIST		(1)
#define XT_BM_KIOSK		(2)

#define XT_SHOW			(1<<7)
#define XT_DELETE		(1<<8)
#define XT_SAVE			(1<<9)
#define XT_OPEN			(1<<10)

#define XT_WL_TOGGLE		(1<<0)
#define XT_WL_ENABLE		(1<<1)
#define XT_WL_DISABLE		(1<<2)
#define XT_WL_FQDN		(1<<3) /* default */
#define XT_WL_TOPLEVEL		(1<<4)
#define XT_WL_PERSISTENT	(1<<5)
#define XT_WL_SESSION		(1<<6)
#define XT_WL_RELOAD		(1<<7)

#define XT_URI_ABOUT		("about:")
#define XT_URI_ABOUT_LEN	(strlen(XT_URI_ABOUT))
#define XT_URI_ABOUT_ABOUT	("about")
#define XT_URI_ABOUT_BLANK	("blank")
#define XT_URI_ABOUT_CERTS	("certs")
#define XT_URI_ABOUT_COOKIEWL	("cookiewl")
#define XT_URI_ABOUT_COOKIEJAR	("cookiejar")
#define XT_URI_ABOUT_DOWNLOADS	("downloads")
#define XT_URI_ABOUT_FAVORITES	("favorites")
#define XT_URI_ABOUT_HELP	("help")
#define XT_URI_ABOUT_HISTORY	("history")
#define XT_URI_ABOUT_JSWL	("jswl")
#define XT_URI_ABOUT_PLUGINWL	("plwl")
#define XT_URI_ABOUT_SET	("set")
#define XT_URI_ABOUT_STATS	("stats")
#define XT_URI_ABOUT_MARCO	("marco")
#define XT_URI_ABOUT_STARTPAGE	("startpage")

/*
 * Session IDs.
 * We use these to prevent people putting xxxt:// URLs on
 * websites in the wild. We generate 8 bytes and represent in hex (16 chars)
 */
#define XT_XTP_SES_KEY_SZ	8
#define XT_XTP_SES_KEY_HEX_FMT  \
	"%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx"


extern int		enable_plugin_whitelist;
extern int		enable_cookie_whitelist;
extern int		enable_js_whitelist;
extern int		browser_mode;
extern int		allow_volatile_cookies;
extern int		cookie_policy;
extern int		cookies_enabled;
extern int		enable_plugins;
extern int		read_only_cookies;
extern int		save_rejected_cookies;
extern int		session_timeout;
extern int		enable_scripts;
extern int		enable_localstorage;
extern int		show_tabs;
extern int		tabless;
extern int		updating_cl_tabs;
extern int		updating_dl_tabs;
extern int		refresh_interval;
extern int		updating_hl_tabs;
extern char		default_script[PATH_MAX];
extern struct passwd	*pwd;
extern char		runtime_settings[PATH_MAX];
extern char		work_dir[PATH_MAX];
extern SoupCookieJar	*s_cookiejar;
extern SoupCookieJar	*p_cookiejar;

extern char		*dl_session_key;
extern char		*hl_session_key;
extern char		*cl_session_key;
extern char		*fl_session_key;

struct domain_list;
extern struct domain_list	c_wl;
extern struct domain_list	js_wl;
extern struct domain_list	pl_wl;
extern struct tab_list		tabs;

struct tab {
	TAILQ_ENTRY(tab)	entry;
	GtkWidget		*vbox;
	GtkWidget		*tab_content;
	struct {
		GtkWidget	*label;
		GtkWidget	*eventbox;
		GtkWidget	*box;
		GtkWidget	*sep;
	} tab_elems;
	GtkWidget		*label;
	GtkWidget		*spinner;
	GtkWidget		*uri_entry;
	GtkWidget		*search_entry;
	GtkWidget		*toolbar;
	GtkWidget		*browser_win;
	GtkWidget		*statusbar_box;
	struct {
		GtkWidget	*statusbar;
		GtkWidget	*buffercmd;
		GtkWidget	*zoom;
		GtkWidget	*position;
	} sbe;
	GtkWidget		*cmd;
	GtkWidget		*buffers;
	GtkWidget		*oops;
	GtkWidget		*backward;
	GtkWidget		*forward;
	GtkWidget		*stop;
	GtkWidget		*gohome;
	GtkWidget		*js_toggle;
	GtkEntryCompletion	*completion;
	guint			tab_id;
	WebKitWebView		*wv;

	WebKitWebHistoryItem		*item;
	WebKitWebBackForwardList	*bfl;

	/* favicon */
	WebKitNetworkRequest	*icon_request;
	WebKitDownload		*icon_download;
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
	gchar			*tmp_uri;
	int			popup; /* 1 if cmd_entry has popup visible */
#ifdef USE_THREADS
	/* https thread stuff */
	GThread			*thread;
#endif
	/* hints */
	int			script_init;
	int			hints_on;
	int			new_tab;

	/* custom stylesheet */
	int			styled;
	char			*stylesheet;

	/* search */
	char			*search_text;
	int			search_forward;
	guint			search_id;

	/* settings */
	WebKitWebSettings	*settings;
	gchar			*user_agent;

	/* marks */
	double			mark[XT_NOMARKS];
};
TAILQ_HEAD(tab_list, tab);

struct domain {
	RB_ENTRY(domain)	entry;
	gchar			*d;
	int			handy; /* app use */
};

struct history {
	RB_ENTRY(history)	entry;
	const gchar		*uri;
	const gchar		*title;
};
struct history_list;
extern struct history_list	hl;

struct karg {
	int		i;
	char		*s;
	int		precount;
};

struct download {
	RB_ENTRY(download)	entry;
	int			id;
	WebKitDownload		*download;
	struct tab		*tab;
};

GtkWidget		*create_window(const gchar *);

/* inspector */
WebKitWebView*		inspector_inspect_web_view_cb(WebKitWebInspector *,
			    WebKitWebView*, struct tab *);
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

struct domain	*wl_find(const gchar *, struct domain_list *);
void		wl_add(char *, struct domain_list *, int);
int		toggle_pl(struct tab *, struct karg *);
int		js_cmd(struct tab *, struct karg *);
int		cookie_cmd(struct tab *, struct karg *);
int		pl_cmd(struct tab *, struct karg *);
int		toplevel_cmd(struct tab *, struct karg *);
struct domain	*wl_find_uri(const gchar *, struct domain_list *);
void		js_toggle_cb(GtkWidget *, struct tab *);
void		show_oops(struct tab *, const char *, ...);
const gchar	*get_uri(struct tab *);
gchar		*find_domain(const gchar *, int);
gchar		*get_html_page(gchar *title, gchar *, gchar *, bool);
void		load_webkit_string(struct tab *, const char *, gchar *);
int		settings_add(char *, char *);
void		wl_init(void);
int		history_rb_cmp(struct history *, struct history *);
int		remove_cookie(int);
void		xtp_handle_dl(struct tab *, uint8_t, int);
void		xtp_handle_hl(struct tab *, uint8_t, int);
int		xtp_page_hl(struct tab *, struct karg *);
char		*xtp_page_dl_row(struct tab *, char *, struct download *);
void		update_favorite_tabs(struct tab *);
void		update_history_tabs(struct tab *);
void		remove_favorite(struct tab *, int);
void		setup_inspector(struct tab *);
int		parse_xtp_url(struct tab *, const char *);

/* hooked functions */
void		(*_soup_cookie_jar_add_cookie)(SoupCookieJar *, SoupCookie *);
void		(*_soup_cookie_jar_delete_cookie)(SoupCookieJar *,
		    SoupCookie *);

