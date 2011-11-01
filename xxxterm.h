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

#define LENGTH(x)		(sizeof x / sizeof x[0])
#define CLEAN(mask)		(mask & ~(GDK_MOD2_MASK) &	\
				    ~(GDK_BUTTON1_MASK) &	\
				    ~(GDK_BUTTON2_MASK) &	\
				    ~(GDK_BUTTON3_MASK) &	\
				    ~(GDK_BUTTON4_MASK) &	\
				    ~(GDK_BUTTON5_MASK))

#define XT_NOMARKS		(('z' - 'a' + 1) * 2 + 10)

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

GtkWidget		*create_window(const gchar *);

/* inspector */
WebKitWebView*		inspector_inspect_web_view_cb(WebKitWebInspector *,
			    WebKitWebView*, struct tab *);
void			setup_inspector(struct tab *);
