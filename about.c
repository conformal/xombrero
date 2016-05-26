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

/*
 * xombrero "protocol" (xtp)
 * We use this for managing stuff like downloads and favorites. They
 * make magical HTML pages in memory which have xxxt:// links in order
 * to communicate with xombrero's internals. These links take the format:
 * xxxt://class/session_key/action/arg
 *
 * Don't begin xtp class/actions as 0. atoi returns that on error.
 *
 * Typically we have not put addition of items in this framework, as
 * adding items is either done via an ex-command or via a keybinding instead.
 */

#define XT_HTML_TAG		"<html xmlns='http://www.w3.org/1999/xhtml'>\n"
#define XT_DOCTYPE		"<!DOCTYPE html PUBLIC '-//W3C//DTD XHTML 1.0 Transitional//EN' 'http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd'>\n"
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
				"table#settings{background-color: #eee;"\
				" border: 0px;"				\
				" margin: 15px;}\n"			\
				"table#settings td{border: 0px;}\n"	\
				"table#settings th{border: 0px;}\n"	\
				"table#settings tr{"			\
				" background: #f6f6f6;}\n"		\
				"table#settings tr:nth-child(even){"	\
				" background: #eeeeee;}\n"		\
				"table#settings tr#modified{"		\
				" background: #FFFFBA;}\n"		\
				"table#settings tr#modified:nth-child(even){"	\
				" background: #ffffA0;}\n"		\
				"</style>\n"

int			js_show_wl(struct tab *, struct karg *);
int			pl_show_wl(struct tab *, struct karg *);
int			https_show_wl(struct tab *, struct karg *);
int			xtp_page_set(struct tab *, struct karg *);
int			xtp_page_rt(struct tab *, struct karg *);
int			marco(struct tab *, struct karg *);
int			startpage(struct tab *, struct karg *);
const char *		marco_message(int *);
void			update_cookie_tabs(struct tab *apart_from);
int			about_webkit(struct tab *, struct karg *);
int			allthethings(struct tab *, struct karg *);

/*
 * If you change the index of any of these, correct the
 * XT_XTP_TAB_MEANING_* macros in xombrero.h!
 */
struct about_type about_list[] = {
	{ XT_URI_ABOUT_ABOUT,		xtp_page_ab },
	{ XT_URI_ABOUT_ALLTHETHINGS,	allthethings },
	{ XT_URI_ABOUT_BLANK,		blank },
	{ XT_URI_ABOUT_CERTS,		ca_cmd },
	{ XT_URI_ABOUT_COOKIEWL,	cookie_show_wl },
	{ XT_URI_ABOUT_COOKIEJAR,	xtp_page_cl },
	{ XT_URI_ABOUT_DOWNLOADS,	xtp_page_dl },
	{ XT_URI_ABOUT_FAVORITES,	xtp_page_fl },
	{ XT_URI_ABOUT_HELP,		help },
	{ XT_URI_ABOUT_HISTORY,		xtp_page_hl },
	{ XT_URI_ABOUT_JSWL,		js_show_wl },
	{ XT_URI_ABOUT_SET,		xtp_page_set },
	{ XT_URI_ABOUT_STATS,		stats },
	{ XT_URI_ABOUT_MARCO,		marco },
	{ XT_URI_ABOUT_STARTPAGE,	startpage },
	{ XT_URI_ABOUT_PLUGINWL,	pl_show_wl },
	{ XT_URI_ABOUT_HTTPS,		https_show_wl },
	{ XT_URI_ABOUT_WEBKIT,		about_webkit },
	{ XT_URI_ABOUT_SEARCH,		xtp_page_sl },
	{ XT_URI_ABOUT_RUNTIME,		xtp_page_rt },
	{ XT_URI_ABOUT_SECVIOLATION,	NULL },
};

struct search_type {
	const char		*name;
	const char		*url;
} search_list[] = {
	{ "Google (SSL)",	"https://encrypted.google.com/search?q=%s" },
	{ "Bing",		"http://www.bing.com/search?q=%s" },
	{ "Yahoo",		"http://search.yahoo.com/search?p=%s" },
	{ "DuckDuckGo",		"https://duckduckgo.com/?q=%s" },
	{ "DuckDuckGo (HTML)",	"https://duckduckgo.com/html?q=%s" },
	{ "DuckDuckGo (Lite)",	"https://duckduckgo.com/lite?q=%s" },
	{ "Ixquick",		"https://ixquick.com/do/search?q=%s" },
	{ "Startpage",		"https://startpage.com/do/search?q=%s" },
};

/*
 * Session IDs.
 * We use these to prevent people putting xxxt:// URLs on
 * websites in the wild. We generate 8 bytes and represent in hex (16 chars)
 */
#define XT_XTP_SES_KEY_SZ	8
#define XT_XTP_SES_KEY_HEX_FMT  \
	"%02" PRIx8 "%02" PRIx8 "%02" PRIx8 "%02" PRIx8 "%02" PRIx8 "%02" PRIx8 "%02" PRIx8 "%02" PRIx8

int			updating_fl_tabs = 0;
int			updating_dl_tabs = 0;
int			updating_hl_tabs = 0;
int			updating_cl_tabs = 0;
int			updating_sl_tabs = 0;
int			updating_sv_tabs = 0;
int			updating_set_tabs = 0;
struct download_list	downloads;

size_t
about_list_size(void)
{
	return (LENGTH(about_list));
}

gchar *
get_html_page(gchar *title, gchar *body, gchar *head, bool addstyles)
{
	gchar			*r;

	r = g_strdup_printf(XT_DOCTYPE XT_HTML_TAG
	    "<head>\n"
	    "<title>%s</title>\n"
	    "%s"
	    "%s"
	    "</head>\n"
	    "<body>\n"
	    "<h1>%s</h1>\n"
	    "%s\n</body>\n"
	    "</html>",
	    title,
	    addstyles ? XT_PAGE_STYLE : "",
	    head,
	    title,
	    body);

	return (r);
}

/*
 * Display a web page from a HTML string in memory, rather than from a URL
 */
void
load_webkit_string(struct tab *t, const char *str, gchar *title, int nohist)
{
	char			file[PATH_MAX];
	int			i, xtp_meaning;

	if (g_signal_handler_is_connected(t->wv, t->progress_handle))
		g_signal_handler_disconnect(t->wv, t->progress_handle);

	/* we set this to indicate we want to manually do navaction */
	if (t->bfl && !nohist) {
		t->item = webkit_web_back_forward_list_get_current_item(t->bfl);
		if (t->item)
			g_object_ref(t->item);
	}

	xtp_meaning = XT_XTP_TAB_MEANING_NORMAL;
	if (title) {
		/* set xtp_meaning */
		for (i = 0; i < LENGTH(about_list); i++)
			if (!strcmp(title, about_list[i].name)) {
				xtp_meaning = i;
				break;
			}

		webkit_web_view_load_string(t->wv, str, NULL, encoding,
		    XT_XTP_STR);
#if GTK_CHECK_VERSION(2, 20, 0)
		gtk_spinner_stop(GTK_SPINNER(t->spinner));
		gtk_widget_hide(t->spinner);
#endif
		snprintf(file, sizeof file, "%s" PS "%s", resource_dir, icons[0]);
		xt_icon_from_file(t, file);
	}

	set_xtp_meaning(t, xtp_meaning);

	t->progress_handle = g_signal_connect(t->wv,
	    "notify::progress", G_CALLBACK(webview_progress_changed_cb), t);
}

int
blank(struct tab *t, struct karg *args)
{
	if (t == NULL)
		show_oops(NULL, "blank invalid parameters");

	load_webkit_string(t, "", XT_URI_ABOUT_BLANK, 0);

	return (0);
}

int
help(struct tab *t, struct karg *args)
{
	char			*page, *head, *body;

	if (t == NULL)
		show_oops(NULL, "help invalid parameters");

	head = "<meta http-equiv=\"REFRESH\" content=\"0;"
	    "url=https://opensource.conformal.com/cgi-bin/man-cgi?xombrero\">"
	    "</head>\n";
	body = "xombrero man page <a href=\"https://opensource.conformal.com/"
	    "cgi-bin/man-cgi?xombrero\">https://opensource.conformal.com/"
	    "cgi-bin/man-cgi?xombrero</a>";

	page = get_html_page(XT_NAME, body, head, FALSE);

	load_webkit_string(t, page, XT_URI_ABOUT_HELP, 0);
	g_free(page);

	return (0);
}

int
stats(struct tab *t, struct karg *args)
{
	char			*page, *body, *s, line[64 * 1024];
	uint64_t		line_count = 0;
	FILE			*r_cookie_f;

	if (t == NULL)
		show_oops(NULL, "stats invalid parameters");

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
			    "<br/>Cookies blocked(*) total: %" PRIu64,
			    line_count);
		} else
			show_oops(t, "Can't open blocked cookies file: %s",
			    strerror(errno));
	}

	body = g_strdup_printf(
	    "Cookies blocked(*) this session: %" PRIu64
	    "%s"
	    "<p><small><b>*</b> results vary based on settings</small></p>",
	    blocked_cookies,
	    line);

	page = get_html_page("Statistics", body, "", 0);
	g_free(body);

	load_webkit_string(t, page, XT_URI_ABOUT_STATS, 0);
	g_free(page);

	return (0);
}

void
show_certs(struct tab *t, gnutls_x509_crt_t *certs,
    size_t cert_count, char *title)
{
	gnutls_datum_t		*cinfo;
	char			*tmp, *body;
	int			i;

	body = g_strdup("");

	for (i = 0; i < cert_count; i++) {
		cinfo = gnutls_malloc(sizeof *cinfo);
		if (gnutls_x509_crt_print(certs[i], GNUTLS_CRT_PRINT_FULL,
		    cinfo)) {
			gnutls_free(cinfo);
			g_free(body);
			return;
		}

		tmp = body;
		body = g_strdup_printf("%s<h2>Cert #%d</h2><pre>%s</pre>",
		    body, i, cinfo->data);
		gnutls_free(cinfo);
		g_free(tmp);
	}

	tmp = get_html_page(title, body, "", 0);
	g_free(body);

	load_webkit_string(t, tmp, XT_URI_ABOUT_CERTS, 0);
	g_free(tmp);
}

int
ca_cmd(struct tab *t, struct karg *args)
{
	FILE			*f = NULL;
	int			rv = 1, certs_read;
	unsigned int		certs = 0;
	struct stat		sb;
	gnutls_datum_t		dt;
	gnutls_x509_crt_t	*c = NULL;
	char			*certs_buf = NULL, *s;

	if ((f = fopen(ssl_ca_file, "r")) == NULL) {
		show_oops(t, "Can't open CA file: %s", ssl_ca_file);
		return (1);
	}

	if (fstat(fileno(f), &sb) == -1) {
		show_oops(t, "Can't stat CA file: %s", ssl_ca_file);
		goto done;
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
	dt.data = (unsigned char *)certs_buf;
	dt.size = sb.st_size;
	c = gnutls_malloc(sizeof(*c) * certs);
	certs_read = gnutls_x509_crt_list_import(c, &certs, &dt,
	    GNUTLS_X509_FMT_PEM, 0);
	if (certs_read <= 0) {
		show_oops(t, "No cert(s) available");
		goto done;
	}
	show_certs(t, c, certs_read, "Certificate Authority Certificates");
done:
	if (c)
		gnutls_free(c);
	if (certs_buf)
		g_free(certs_buf);
	if (f)
		fclose(f);

	return (rv);
}

int
cookie_show_wl(struct tab *t, struct karg *args)
{
	args->i = XT_SHOW | XT_WL_PERSISTENT | XT_WL_SESSION;
	wl_show(t, args, "Cookie White List", &c_wl);

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
	} else if (args->i & XT_DELETE) {
		remove_cookie_all();
		update_cookie_tabs(NULL);
	}

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
https_show_wl(struct tab *t, struct karg *args)
{
	args->i = XT_SHOW | XT_WL_PERSISTENT | XT_WL_SESSION;
	wl_show(t, args, "HTTPS Force List", &force_https);

	return (0);
}

int
https_cmd(struct tab *t, struct karg *args)
{
	if (args->i & XT_SHOW)
		wl_show(t, args, "HTTPS Force List", &force_https);
	else if (args->i & XT_SAVE) {
		args->i |= XT_WL_RELOAD;
		wl_save(t, args, XT_WL_HTTPS);
	} else if (args->i & XT_WL_TOGGLE) {
		args->i |= XT_WL_RELOAD;
		toggle_force_https(t, args);
	} else if (args->i & XT_DELETE)
		show_oops(t, "https delete' currently unimplemented");

	return (0);
}

/*
 * cancel, remove, etc. downloads
 */
void
xtp_handle_dl(struct tab *t, uint8_t cmd, int id, const char *query)
{
	struct download		find, *d = NULL;
#ifndef	__MINGW32__
	char			*file = NULL;
	const char		*uri = NULL;
#endif

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
	case XT_XTP_DL_START:
		/* our downloads always needs to be
		 * restarted if called from here
		 */
		download_start(t, d, XT_DL_RESTART);
		break;
	case XT_XTP_DL_CANCEL:
		webkit_download_cancel(d->download);
		g_object_unref(d->download);
		RB_REMOVE(download_list, &downloads, d);
		break;
	case XT_XTP_DL_UNLINK:
#ifdef __MINGW32__
		/* XXX uri's aren't handled properly on windows? */
		unlink(webkit_download_get_destination_uri(d->download));
#else
		uri = webkit_download_get_destination_uri(d->download);
		if ((file = g_filename_from_uri(uri, NULL, NULL)) != NULL) {
			unlink(file);
			g_free(file);
		}
#endif
		/* FALLTHROUGH */
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

void
xtp_handle_hl(struct tab *t, uint8_t cmd, int id, const char *query)
{
	struct pagelist_entry	*h, *ht;

	switch (cmd) {
	case XT_XTP_HL_REMOVE:
		remove_pagelist_entry_by_count(&hl, 1);
		break;
	case XT_XTP_HL_REMOVE_ALL:
		RB_FOREACH_SAFE(h, pagelist, &hl, ht)
			remove_pagelist_entry(&hl, h);
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

void
remove_favorite(struct tab *t, int index)
{
	/* Load before manipulation in case other processes have changed
	the file beneath us */
	if (restore_favorites()) {
		show_oops(t, "Error reading favorites file: %s",
		    strerror(errno));
		return;
	}
	
	remove_pagelist_entry_by_count(&favs, index);

	if (save_pagelist_to_disk(&favs, XT_FAVS_FILE)) {
		update_favorite_tabs(NULL);
	}
	/* TODO: We should remove the URI from the list of completions here,
	too */
}

int
add_favorite(struct tab *t, struct karg *args)
{
	const gchar		*uri, *title;
	gchar			*argtitle = NULL;
	int 			retval;

	/* don't allow adding of xtp pages to favorites */
	if (t->xtp_meaning != XT_XTP_TAB_MEANING_NORMAL) {
		show_oops(t, "%s: can't add xtp pages to favorites", __func__);
		return (1);
	}

	/* Load before manipulation in case other processes have changed
	the file beneath us */
	if (restore_favorites()) {
		show_oops(t, "Error reading favorites file: %s",
		    strerror(errno));
		return 1;
	}

	if (args->s && strlen(g_strstrip(args->s)))
		argtitle = html_escape(g_strstrip(args->s));
	
	title = argtitle ? argtitle : get_title(t, FALSE);
	uri = get_uri(t);
	insert_pagelist_entry(&favs, uri, title, 0);
	completion_add_uri(uri);

	if ((retval = save_pagelist_to_disk(&favs, XT_FAVS_FILE))) {

		update_favorite_tabs(NULL);
	}
	if (argtitle) 
		g_free(argtitle);

	return (retval);
}

/* read legacy (up to 1.6.4) favorites 

We don't write these any more, so this function should be removed once
we're satisfied all old files have been updated.
*/
static int
load_legacy_favorites(char *file_name)
{
	char                    *uri = NULL, *title = NULL;
	size_t                  len, lineno;
	int                     failed = 0;
	const char              delim[3] = {'\\', '\\', '\0'};
	char			file[PATH_MAX];
	FILE 			*f;
	
	snprintf(file, sizeof file, "%s" PS "%s", work_dir, file_name);
	f = fopen(file, "r");
	if (!f) {
		return 1;
	}

	for (lineno = 1;;) {
		if ((title = fparseln(f, &len, &lineno, delim, 0)) == NULL)
			break;
		if (strlen(title) == 0) {
			free(title);
			title = NULL;
			continue;
		}

		if ((uri = fparseln(f, &len, &lineno, delim, 0)) == NULL) {
			if (feof(f) || ferror(f)) {
				failed = 1;
				break;
			}
		}

		insert_pagelist_entry(&favs, uri, title, 0);
		free(title);
		free(uri);
		lineno += 1;
	}

	fclose(f);
	return failed;
}

/* Pull the list of favorites into memory.  

This supports loading old-style favorite lists (title/url) in addition
to the new standard pagelist format.
*/
int
restore_favorites(void)
{
	int			retval;

	empty_pagelist(&favs);

	if (2 == (retval = load_pagelist_from_disk(&favs, XT_FAVS_FILE))) {
		retval = load_legacy_favorites(XT_FAVS_FILE);
	}
	
	return retval;
}


char *
search_engine_add(char *body, const char *name, const char *url,
    const char *key, int select)
{
	char			*b = body;

	body = g_strdup_printf("%s<tr>"
	    "<td>%s</td>"
	    "<td>%s</td>"
	    "<td style='text-align: center'>"
	    "<a href='%s%d/%s/%d/%d'>[ Select ]</a></td>"
	    "</tr>\n",
	    body,
	    name,
	    url,
	    XT_XTP_STR, XT_XTP_SL, key, XT_XTP_SL_SET, select);
	g_free(b);
	return (body);
}

void
xtp_handle_ab(struct tab *t, uint8_t cmd, int arg, const char *query)
{
	char			config[PATH_MAX];
	char			*cmdstr;
	char			**sv;

	switch (cmd) {
	case XT_XTP_AB_EDIT_CONF:
		if (external_editor == NULL || strlen(external_editor) == 0) {
			show_oops(t, "external_editor is unset");
			break;
		}

		snprintf(config, sizeof config, "%s" PS ".%s", pwd->pw_dir,
		    XT_CONF_FILE);
		sv = g_strsplit(external_editor, "<file>", -1);
		cmdstr = g_strjoinv(config, sv);
		g_strfreev(sv);
		sv = g_strsplit_set(cmdstr, " \t", -1);

		if (!g_spawn_async(NULL, sv, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL,
		    NULL, NULL))
			show_oops(t, "%s: could not spawn process", __func__);

		g_strfreev(sv);
		g_free(cmdstr);
		break;
	default:
		show_oops(t, "%s, invalid about command", __func__);
		break;
	};
	xtp_page_ab(t, NULL);
}
void
xtp_handle_fl(struct tab *t, uint8_t cmd, int arg, const char *query)
{
	struct karg		args = {0};

	switch (cmd) {
	case XT_XTP_FL_LIST:
		/* nothing, just the below call to xtp_page_fl() */
		break;
	case XT_XTP_FL_REMOVE:
		remove_favorite(t, arg);
		args.i = XT_DELETE;
		break;
	default:
		show_oops(t, "%s: invalid favorites command", __func__);
		break;
	};

	xtp_page_fl(t, &args);
}

void
xtp_handle_cl(struct tab *t, uint8_t cmd, int arg, const char *query)
{
	switch (cmd) {
	case XT_XTP_CL_LIST:
		/* nothing, just xtp_page_cl() */
		break;
	case XT_XTP_CL_REMOVE:
		remove_cookie(arg);
		break;
	case XT_XTP_CL_REMOVE_DOMAIN:
		remove_cookie_domain(arg);
		break;
	case XT_XTP_CL_REMOVE_ALL:
		remove_cookie_all();
		break;
	default:
		show_oops(t, "%s: unknown cookie xtp command", __func__);
		break;
	};

	xtp_page_cl(t, NULL);
}

void
xtp_handle_sl(struct tab *t, uint8_t cmd, int arg, const char *query)
{
	const char		*search;
	char			*enc_search, *uri;
	char			**sv;

	switch (cmd) {
	case XT_XTP_SL_SET:
		set_search_string((char *)search_list[arg].url);
		if (save_runtime_setting("search_string", search_list[arg].url))
			show_oops(t, "could not set search_string in runtime");
		break;
	default:
		show_oops(t, "%s: unknown search xtp command", __func__);
		break;
	};

	search = gtk_entry_get_text(GTK_ENTRY(t->search_entry)); /* static */
	enc_search = soup_uri_encode(search, XT_RESERVED_CHARS);
	sv = g_strsplit(search_string, "%s", 2);
	uri = g_strjoinv(enc_search, sv);
	load_uri(t, uri);
	g_free(enc_search);
	g_strfreev(sv);
	g_free(uri);
}

void
xtp_handle_sv(struct tab *t, uint8_t cmd, int id, const char *query)
{
	SoupURI			*soupuri = NULL;
	struct karg		args = {0};
	struct secviolation	find, *sv;

	find.xtp_arg = id;
	if ((sv = RB_FIND(secviolation_list, &svl, &find)) == NULL)
		return;

	args.ptr = (void *)sv->t;
	args.s = sv->uri;

	switch (cmd) {
	case XT_XTP_SV_SHOW_NEW_CERT:
		args.i = XT_SHOW;
		if (cert_cmd(t, &args)) {
			xtp_page_sv(t, &args);
			return;
		}
		break;
	case XT_XTP_SV_SHOW_CACHED_CERT:
		args.i = XT_CACHE | XT_SHOW;
		if (cert_cmd(t, &args)) {
			xtp_page_sv(t, &args);
			return;
		}
		break;
	case XT_XTP_SV_ALLOW_SESSION:
		soupuri = soup_uri_new(sv->uri);
		wl_add(soupuri->host, &svil, 0);
		load_uri(t, sv->uri);
		focus_webview(t);
		break;
	case XT_XTP_SV_CACHE:
		args.i = XT_CACHE;
		if (cert_cmd(t, &args)) {
			xtp_page_sv(t, &args);
			return;
		}
		load_uri(t, sv->uri);
		focus_webview(t);
		break;
	default:
		show_oops(t, "%s: invalid secviolation command", __func__);
		break;
	};

	g_free(sv->uri);
	if (soupuri)
		soup_uri_free(soupuri);
	RB_REMOVE(secviolation_list, &svl, sv);
}

void
xtp_handle_rt(struct tab *t, uint8_t cmd, int id, const char *query)
{
	struct set_reject	*sr;
	GHashTable		*new_settings = NULL;
	int			modify;
	char			*val, *curval, *s;
	int			i = 0;

	switch (cmd) {
	case XT_XTP_RT_SAVE:
		if (query == NULL)
			break;
		new_settings = soup_form_decode(query);
		for (i = 0; i < get_settings_size(); ++i) {
			if (!rs[i].activate)
				continue;
			val = (char *)g_hash_table_lookup(new_settings,
			    rs[i].name);
			modify = 0;
			switch (rs[i].type) {
			case XT_S_INT: /* FALLTHROUGH */
			case XT_S_BOOL:
				if (atoi(val) != *rs[i].ival)
					modify = 1;
				break;
			case XT_S_DOUBLE:
				if (atof(val) < (*rs[i].dval - 0.0001) ||
				    atof(val) > (*rs[i].dval + 0.0001))
					modify = 1;
				break;
			case XT_S_STR:
				s = (rs[i].sval == NULL || *rs[i].sval == NULL)
				    ? "" : *rs[i].sval;
				if (rs[i].sval && g_strcmp0(val, s))
					modify = 1;
				else if (rs[i].s && rs[i].s->get) {
					curval = rs[i].s->get(NULL);
					if (g_strcmp0(val, curval))
						modify = 1;
					g_free(curval);
				}
				break;
			case XT_S_INVALID: /* FALLTHROUGH */
			default:
				break;
			}
			if (rs[i].activate(val)) {
				sr = g_malloc(sizeof *sr);
				sr->name = g_strdup(rs[i].name);
				sr->value = g_strdup(val);
				TAILQ_INSERT_TAIL(&srl, sr, entry);
				continue;
			}
			if (modify)
				if (save_runtime_setting(rs[i].name, val))
					show_oops(t, "error");
		}
		break;
	default:
		show_oops(t, "%s: invalid set command", __func__);
		break;
	}

	if (new_settings)
		g_hash_table_destroy(new_settings);

	xtp_page_rt(t, NULL);
}

/* link an XTP class to it's session key and handler function */
struct xtp_despatch {
	uint8_t			xtp_class;
	void			(*handle_func)(struct tab *, uint8_t, int,
				    const char *query);
};

struct xtp_despatch		xtp_despatches[] = {
	{ XT_XTP_DL, xtp_handle_dl },
	{ XT_XTP_HL, xtp_handle_hl },
	{ XT_XTP_FL, xtp_handle_fl },
	{ XT_XTP_CL, xtp_handle_cl },
	{ XT_XTP_SL, xtp_handle_sl },
	{ XT_XTP_AB, xtp_handle_ab },
	{ XT_XTP_SV, xtp_handle_sv },
	{ XT_XTP_RT, xtp_handle_rt },
	{ XT_XTP_INVALID, NULL }
};

/*
 * generate a session key to secure xtp commands.
 * pass in a ptr to the key in question and it will
 * be modified in place.
 */
void
generate_xtp_session_key(char **key)
{
	uint8_t			rand_bytes[XT_XTP_SES_KEY_SZ];

	if (key == NULL)
		return;

	/* free old key */
	if (*key != NULL)
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
 * return (1) if OK
 */
int
validate_xtp_session_key(struct tab *t, char *key)
{
	if (t == NULL || t->session_key == NULL || key == NULL)
		return (0);

	if (strcmp(t->session_key, key) != 0) {
		show_oops(t, "%s: xtp session key mismatch possible spoof",
		    __func__);
		return (0);
	}

	return (1);
}

/*
 * is the url xtp protocol? (xxxt://)
 * if so, parse and despatch correct bahvior
 */
int
parse_xtp_url(struct tab *t, const char *uri_str)
{
	SoupURI			*uri = NULL;
	struct xtp_despatch	*dsp, *dsp_match = NULL;
	int			ret = FALSE;
	int			class = 0;
	char			**sv = NULL;

	/*
	 *   uri->host	= class
	 *   sv[0]	= session key
	 *   sv[1]	= command
	 *   sv[2]	= optional argument
	 */

	DNPRINTF(XT_D_URL, "%s: url %s\n", __func__, uri_str);

	if ((uri = soup_uri_new(uri_str)) == NULL)
		goto clean;
	if (strncmp(uri->scheme, XT_XTP_SCHEME, strlen(XT_XTP_SCHEME)))
		goto clean;
	if (uri->host == NULL || strlen(uri->host) == 0)
		goto clean;
	if ((sv = g_strsplit(uri->path + 1, "/", 3)) == NULL)
		goto clean;

	if (sv[0] == NULL || sv[1] == NULL)
		goto clean;

	dsp = xtp_despatches;
	class = atoi(uri->host);
	while (dsp->xtp_class) {
		if (dsp->xtp_class == class) {
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
	if (validate_xtp_session_key(t, sv[0])) {
		ret = TRUE; /* all is well, this was a valid xtp request */
		if (sv[2])
			dsp_match->handle_func(t, atoi(sv[1]), atoi(sv[2]),
			    uri->query);
		else
			dsp_match->handle_func(t, atoi(sv[1]), 0, uri->query);
	}

clean:
	if (uri)
		soup_uri_free(uri);
	if (sv)
		g_strfreev(sv);

	return (ret);
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

/*
 * update all search tabs apart from one. Pass NULL if
 * you want to update all.
 */
void
update_search_tabs(struct tab *apart_from)
{
	struct tab			*t;

	if (!updating_sl_tabs) {
		updating_sl_tabs = 1; /* stop infinite recursion */
		TAILQ_FOREACH(t, &tabs, entry)
			if ((t->xtp_meaning == XT_XTP_TAB_MEANING_SL)
			    && (t != apart_from))
				xtp_page_sl(t, NULL);
		updating_sl_tabs = 0;
	}
}

int
xtp_page_ab(struct tab *t, struct karg *args)
{
	char			*page, *body;

	if (t == NULL) {
		show_oops(NULL, "about invalid parameters");
		return (-1);
	}

	generate_xtp_session_key(&t->session_key);

	body = g_strdup_printf("<b>Version: %s</b>"
#ifdef XOMBRERO_BUILDSTR
	    "<br><b>Build: %s</b>"
#endif
	    "<br><b>WebKit: %d.%d.%d</b>"
	    "<br><b>User Agent: %d.%d</b>"
#ifdef WEBKITGTK_API_VERSION
	    "<br><b>WebKit API: %.1f</b>"
#endif
	    "<br><b>Configuration: %s" PS "<a href='%s%d/%s/%d'>.%s</a>"
	    " (remember to restart the browser after any changes)</b>"
	    "<p>"
	    "Authors:"
	    "<ul>"
	    "<li>Marco Peereboom &lt;marco@peereboom.us&gt;</li>"
	    "<li>Stevan Andjelkovic &lt;stevan@student.chalmers.se&gt;</li>"
	    "<li>Edd Barrett &lt;vext01@gmail.com&gt;</li>"
	    "<li>Todd T. Fries &lt;todd@fries.net&gt;</li>"
	    "<li>Raphael Graf &lt;r@undefined.ch&gt;</li>"
	    "<li>Michal Mazurek &lt;akfaew@jasminek.net&gt;</li>"
	    "<li>Josh Rickmar &lt;jrick@devio.us&gt;</li>"
	    "<li>David Hill &lt;dhill@mindcry.org&gt;</li>"
	    "</ul>"
	    "Copyrights and licenses can be found on the xombrero "
	    "<a href=\"https://github.com/conformal/xombrero/wiki\">website</a>"
	    "</p>",
#ifdef XOMBRERO_BUILDSTR
	    version, XOMBRERO_BUILDSTR,
#else
	    version,
#endif
	    WEBKIT_MAJOR_VERSION, WEBKIT_MINOR_VERSION, WEBKIT_MICRO_VERSION,
	    WEBKIT_USER_AGENT_MAJOR_VERSION, WEBKIT_USER_AGENT_MINOR_VERSION
#ifdef WEBKITGTK_API_VERSION
	    ,WEBKITGTK_API_VERSION
#endif
	    ,pwd->pw_dir,
	    XT_XTP_STR,
	    XT_XTP_AB,
	    t->session_key ? t->session_key : "",
	    XT_XTP_AB_EDIT_CONF,
	    XT_CONF_FILE
	    );

	page = get_html_page("About", body, "", 0);
	g_free(body);

	load_webkit_string(t, page, XT_URI_ABOUT_ABOUT, 0);

	g_free(page);

	return (0);
}

/* show a list of favorites (bookmarks) */
int
xtp_page_fl(struct tab *t, struct karg *args)
{
	int			row = 0, failed = 0;
	char			*body, *tmp, *page = NULL;
	struct pagelist_entry	*item;

	DNPRINTF(XT_D_FAVORITE, "%s:", __func__);

	if (t == NULL) {
		show_oops(NULL, "%s: bad param", __func__);
		return (-1);
	}

	generate_xtp_session_key(&t->session_key);

	/* body */
	if (args && args->i & XT_DELETE)
		body = g_strdup_printf("<table style='table-layout:fixed'><tr>"
		    "<th style='width: 40px'>&#35;</th><th>Link</th>"
		    "<th style='width: 40px'>Rm</th></tr>\n");
	else
		body = g_strdup_printf("<table style='table-layout:fixed'><tr>"
		    "<th style='width: 40px'>&#35;</th><th>Link</th></tr>\n");

	RB_FOREACH_REVERSE(item, pagelist, &favs) {
		tmp = body;
		if (args && args->i & XT_DELETE)
			body = g_strdup_printf("%s<tr>"
			    "<td>%d</td>"
			    "<td><a href='%s'>%s</a></td>"
			    "<td style='text-align: center'>"
			    "<a href='%s%d/%s/%d/%d'>X</a></td>"
			    "</tr>\n",
			    body, row, item->uri, item->title,
			    XT_XTP_STR, XT_XTP_FL,
			    t->session_key ? t->session_key : "",
			    XT_XTP_FL_REMOVE, row);
		else
			body = g_strdup_printf("%s<tr>"
			    "<td>%d</td>"
			    "<td><a href='%s'>%s</a></td>"
			    "</tr>\n",
			    body, row, item->uri, item->title);
		g_free(tmp);

		row++;
	}

	/* if none, say so */
	if (row == 1) {
		tmp = body;
		body = g_strdup_printf("%s<tr>"
		    "<td colspan='%d' style='text-align: center'>"
		    "No favorites - To add one use the 'favadd' command."
		    "</td></tr>", body, (args && args->i & XT_DELETE) ? 3 : 2);
		g_free(tmp);
	}

	tmp = body;
	body = g_strdup_printf("%s</table>", body);
	g_free(tmp);

	/* render */
	if (!failed) {
		page = get_html_page("Favorites", body, "", 1);
		load_webkit_string(t, page, XT_URI_ABOUT_FAVORITES, 0);
		g_free(page);
	}

	update_favorite_tabs(t);

	if (body)
		g_free(body);

	return (failed);
}

/*
 * Return a new string with a download row (in html)
 * appended. Old string is freed.
 */
char *
xtp_page_dl_row(struct tab *t, char *html, struct download *dl)
{

	WebKitDownloadStatus	stat;
	const gchar		*destination;
	gchar			*d;
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
	    XT_XTP_STR, XT_XTP_DL, t->session_key);

	stat = webkit_download_get_status(dl->download);

	switch (stat) {
	case WEBKIT_DOWNLOAD_STATUS_FINISHED:
		status_html = g_strdup_printf("Finished");
		cmd_html = g_strdup_printf(
		    "<a href='%s%d/%d'>Remove</a> / <a href='%s%d/%d'>Unlink</a>",
		    xtp_prefix, XT_XTP_DL_REMOVE, dl->id, xtp_prefix,
		    XT_XTP_DL_UNLINK, dl->id);
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
		cmd_html = g_strdup_printf(
		    "<a href='%s%d/%d'>Restart</a> / <a href='%s%d/%d'>Remove</a> / <a href='%s%d/%d'>Unlink</a>",
		    xtp_prefix, XT_XTP_DL_START, dl->id,
		    xtp_prefix, XT_XTP_DL_REMOVE, dl->id, xtp_prefix,
		    XT_XTP_DL_UNLINK, dl->id);
		break;
	case WEBKIT_DOWNLOAD_STATUS_ERROR:
		status_html = g_strdup_printf("Error!");
		cmd_html = g_strdup_printf(
		    "<a href='%s%d/%d'>Restart</a> / <a href='%s%d/%d'>Remove</a> / <a href='%s%d/%d'>Unlink</a>",
		    xtp_prefix, XT_XTP_DL_START, dl->id,
		    xtp_prefix, XT_XTP_DL_REMOVE, dl->id, xtp_prefix,
		    XT_XTP_DL_UNLINK, dl->id);
		break;
	case WEBKIT_DOWNLOAD_STATUS_CREATED:
		cmd_html = g_strdup_printf("<a href='%s%d/%d'>Start</a> / <a href='%s%d/%d'>Cancel</a>",
		    xtp_prefix, XT_XTP_DL_START, dl->id, xtp_prefix,
		    XT_XTP_DL_CANCEL, dl->id);
		status_html = g_strdup_printf("Created");
		break;
	default:
		show_oops(t, "%s: unknown download status", __func__);
	};

	destination = webkit_download_get_destination_uri(dl->download);
	/* we might not have a destination set yet */
	if (!destination)
		destination = webkit_download_get_suggested_filename(dl->download);
	d = g_strdup(destination);	/* copy for basename */
	new_html = g_strdup_printf(
	    "%s\n<tr><td>%s</td><td>%s</td>"
	    "<td style='text-align:center'>%s</td></tr>\n",
	    html, basename(d), status_html, cmd_html);
	g_free(d);
	g_free(html);

	if (status_html)
		g_free(status_html);

	if (cmd_html)
		g_free(cmd_html);

	g_free(xtp_prefix);

	return new_html;
}

/* cookie management XTP page */
int
xtp_page_cl(struct tab *t, struct karg *args)
{
	char			*body, *page, *tmp;
	int			i = 1; /* all ids start 1 */
	int			domain_id = 0;
	GSList			*sc, *pc, *pc_start;
	SoupCookie		*c;
	char			*type, *table_headers, *last_domain;

	DNPRINTF(XT_D_CMD, "%s", __func__);

	if (t == NULL) {
		show_oops(NULL, "%s invalid parameters", __func__);
		return (1);
	}

	generate_xtp_session_key(&t->session_key);

	/* table headers */
	table_headers = g_strdup_printf("<table><tr>"
	    "<th>Type</th>"
	    "<th>Name</th>"
	    "<th style='width:200px'>Value</th>"
	    "<th>Path</th>"
	    "<th>Expires</th>"
	    "<th>Secure</th>"
	    "<th>HTTP<br />only</th>"
	    "<th style='width:40px'>Rm</th></tr>\n");

	sc = soup_cookie_jar_all_cookies(s_cookiejar);
	pc = soup_cookie_jar_all_cookies(p_cookiejar);
	pc_start = pc;

	body = g_strdup_printf("<div align=\"center\"><a href=\"%s%d/%s/%d\">"
	    "[ Remove All Cookies From All Domains ]</a></div>\n",
	    XT_XTP_STR, XT_XTP_CL, t->session_key, XT_XTP_CL_REMOVE_ALL);

	last_domain = g_strdup("");
	for (; sc; sc = sc->next) {
		c = sc->data;

		if (strcmp(last_domain, c->domain) != 0) {
			/* new domain */
			domain_id ++;
			g_free(last_domain);
			last_domain = g_strdup(c->domain);

			if (body != NULL) {
				tmp = body;
				body = g_strdup_printf("%s</table>"
				    "<h2>%s</h2><div align=\"center\">"
				    "<a href='%s%d/%s/%d/%d'>"
				    "[ Remove All From This Domain ]"
				    "</a></div>%s\n",
				    body, c->domain,
				    XT_XTP_STR, XT_XTP_CL, t->session_key,
				    XT_XTP_CL_REMOVE_DOMAIN, domain_id,
				    table_headers);
				g_free(tmp);
			} else {
				/* first domain */
				body = g_strdup_printf("<h2>%s</h2>"
				    "<div align=\"center\">"
				    "<a href='%s%d/%s/%d/%d'>"
				    "[ Remove All From This Domain ]</a></div>%s\n",
				    c->domain, XT_XTP_STR, XT_XTP_CL,
				    t->session_key, XT_XTP_CL_REMOVE_DOMAIN,
				    domain_id, table_headers);
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
		    "<td>%s</td>"
		    "<td style='word-wrap:normal'>%s</td>"
		    "<td>"
		    "  <textarea rows='4'>%s</textarea>"
		    "</td>"
		    "<td>%s</td>"
		    "<td>%s</td>"
		    "<td>%d</td>"
		    "<td>%d</td>"
		    "<td style='text-align:center'>"
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
		    t->session_key,
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
	tmp = body;
	body = g_strdup_printf("%s</table>", body);
	g_free(tmp);

	page = get_html_page("Cookie Jar", body, "", TRUE);
	g_free(body);
	g_free(table_headers);
	g_free(last_domain);

	load_webkit_string(t, page, XT_URI_ABOUT_COOKIEJAR, 0);
	update_cookie_tabs(t);

	g_free(page);

	return (0);
}

int
xtp_page_hl(struct tab *t, struct karg *args)
{
	char			*body, *page, *tmp;
	struct pagelist_entry		*h;
	int			i = 1; /* all ids start 1 */

	DNPRINTF(XT_D_CMD, "%s", __func__);

	if (t == NULL) {
		show_oops(NULL, "%s invalid parameters", __func__);
		return (1);
	}

	generate_xtp_session_key(&t->session_key);

	/* body */
	body = g_strdup_printf("<div align=\"center\"><a href=\"%s%d/%s/%d\">"
	    "[ Remove All ]</a></div>"
	    "<table style='table-layout:fixed'><tr>"
	    "<th>URI</th><th>Title</th><th>Last visited</th>"
	    "<th style='width: 40px'>Rm</th></tr>\n",
	    XT_XTP_STR, XT_XTP_HL, t->session_key, XT_XTP_HL_REMOVE_ALL);

	RB_FOREACH_REVERSE(h, pagelist, &hl) {
		tmp = body;
		body = g_strdup_printf(
		    "%s\n<tr>"
		    "<td><a href='%s'>%s</a></td>"
		    "<td>%s</td>"
		    "<td>%s</td>"
		    "<td style='text-align: center'>"
		    "<a href='%s%d/%s/%d/%d'>X</a></td></tr>\n",
		    body, h->uri, h->uri, h->title, ctime(&h->time),
		    XT_XTP_STR, XT_XTP_HL, t->session_key,
		    XT_XTP_HL_REMOVE, i);

		g_free(tmp);
		i++;
	}

	/* small message if there are none */
	if (i == 1) {
		tmp = body;
		body = g_strdup_printf("%s\n<tr><td style='text-align:center'"
		    "colspan='4'>No History</td></tr>\n", body);
		g_free(tmp);
	}

	tmp = body;
	body = g_strdup_printf("%s</table>", body);
	g_free(tmp);

	page = get_html_page("History", body, "", TRUE);
	g_free(body);

	/*
	 * update all history manager tabs as the xtp session
	 * key has now changed. No need to update the current tab.
	 * Already did that above.
	 */
	update_history_tabs(t);

	load_webkit_string(t, page, XT_URI_ABOUT_HISTORY, 0);
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
	char			*body, *page, *tmp;
	char			*ref;
	int			n_dl = 1;

	DNPRINTF(XT_D_DOWNLOAD, "%s", __func__);

	if (t == NULL) {
		show_oops(NULL, "%s invalid parameters", __func__);
		return (1);
	}

	generate_xtp_session_key(&t->session_key);

	/* header - with refresh so as to update */
	if (refresh_interval >= 1)
		ref = g_strdup_printf(
		    "<meta http-equiv='refresh' content='%u"
		    ";url=%s%d/%s/%d' />\n",
		    refresh_interval,
		    XT_XTP_STR,
		    XT_XTP_DL,
		    t->session_key,
		    XT_XTP_DL_LIST);
	else
		ref = g_strdup("");

	body = g_strdup_printf("<div align='center'>"
	    "<p>\n<a href='%s%d/%s/%d'>\n[ Refresh Downloads ]</a>\n"
	    "</p><table><tr><th style='width: 60%%'>"
	    "File</th>\n<th>Progress</th><th>Command</th></tr>\n",
	    XT_XTP_STR, XT_XTP_DL, t->session_key, XT_XTP_DL_LIST);

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

	tmp = body;
	body = g_strdup_printf("%s</table></div>", body);
	g_free(tmp);

	page = get_html_page("Downloads", body, ref, 1);
	g_free(ref);
	g_free(body);

	/*
	 * update all download manager tabs as the xtp session
	 * key has now changed. No need to update the current tab.
	 * Already did that above.
	 */
	update_download_tabs(t);

	load_webkit_string(t, page, XT_URI_ABOUT_DOWNLOADS, 0);
	g_free(page);

	return (0);
}

int
xtp_page_sl(struct tab *t, struct karg *args)
{
	int			i;
	char			*page, *body, *tmp;

	DNPRINTF(XT_D_SEARCH, "%s", __func__);

	generate_xtp_session_key(&t->session_key);

	if (t == NULL) {
		show_oops(NULL, "%s invalid parameters", __func__);
		return (1);
	}

	body = g_strdup_printf("<p>The xombrero authors will not choose a "
	    "default search engine for you.  What follows is a list of search "
	    "engines (in no particular order) you may be interested in.  "
	    "To permanently choose a search engine, click [ Select ] to save "
	    "<tt>search_string</tt> as a runtime setting, or set "
	    "<tt>search_string</tt> to the appropriate URL in your xombrero "
	    "configuration.</p>");

	tmp = body;
	body = g_strdup_printf("%s\n<table style='table-layout:fixed'><tr>"
	    "<th style='width: 200px'>Name</th><th>URL</th>"
	    "<th style='width: 100px'>Select</th></tr>\n", body);
	g_free(tmp);

	for (i = 0; i < (sizeof search_list / sizeof (struct search_type)); ++i)
		body = search_engine_add(body, search_list[i].name,
		    search_list[i].url, t->session_key, i);

	tmp = body;
	body = g_strdup_printf("%s</table>", body);
	g_free(tmp);

	page = get_html_page("Choose a search engine", body, "", 1);
	g_free(body);

	/*
	 * update all search tabs as the xtp session key has now changed. No
	 * need to update the current tab. Already did that above.
	 */
	update_search_tabs(t);

	load_webkit_string(t, page, XT_URI_ABOUT_SEARCH, 0);
	g_free(page);

	return (0);
}

int
xtp_page_sv(struct tab *t, struct karg *args)
{
	SoupURI			*soupuri;
	static int		arg = 0;
	struct secviolation	find, *sv;
	char			*page, *body;

	if (t == NULL) {
		show_oops(NULL, "secviolation invalid parameters");
		return (-1);
	}

	generate_xtp_session_key(&t->session_key);

	if (args == NULL) {
		find.xtp_arg = t->xtp_arg;
		sv = RB_FIND(secviolation_list, &svl, &find);
		if (sv == NULL)
			return (-1);
	} else {
		sv = g_malloc(sizeof(struct secviolation));
		sv->xtp_arg = ++arg;
		t->xtp_arg = arg;
		sv->t = t;
		sv->uri = args->s;
		RB_INSERT(secviolation_list, &svl, sv);
	}

	if (sv->uri == NULL || (soupuri = soup_uri_new(sv->uri)) == NULL)
		return (-1);

	body = g_strdup_printf(
	    "<p><b>You tried to access %s</b>."
	    "<p><b>The site's security certificate has been modified.</b>"
	    "<p>The domain of the page you have tried to access, <b>%s</b>, "
	    "has a different remote certificate then the local cached version "
	    "from a previous visit.  As a security precaution to help prevent "
	    "against man-in-the-middle attacks, please choose one of the "
	    "following actions to continue, or disable the "
	    "<tt>warn_cert_changes</tt> setting in your xombrero "
	    "configuration."
	    "<p><b>Choose an action:"
	    "<br><a href='%s%d/%s/%d/%d'>Allow for this session</a>"
	    "<br><a href='%s%d/%s/%d/%d'>Cache new certificate</a>"
	    "<br><a href='%s%d/%s/%d/%d'>Show cached certificate</a>"
	    "<br><a href='%s%d/%s/%d/%d'>Show new certificate</a>",
	    sv->uri,
	    soupuri->host,
	    XT_XTP_STR, XT_XTP_SV, t->session_key, XT_XTP_SV_ALLOW_SESSION,
		sv->xtp_arg,
	    XT_XTP_STR, XT_XTP_SV, t->session_key, XT_XTP_SV_CACHE,
		sv->xtp_arg,
	    XT_XTP_STR, XT_XTP_SV, t->session_key, XT_XTP_SV_SHOW_CACHED_CERT,
		sv->xtp_arg,
	    XT_XTP_STR, XT_XTP_SV, t->session_key, XT_XTP_SV_SHOW_NEW_CERT,
		sv->xtp_arg);

	page = get_html_page("Security Violation", body, "", 0);
	g_free(body);

	load_webkit_string(t, page, XT_URI_ABOUT_SECVIOLATION, 1);

	g_free(page);
	if (soupuri)
		soup_uri_free(soupuri);

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

	load_webkit_string(t, page, XT_URI_ABOUT_STARTPAGE, 0);
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
	if ((msg = g_strdup_vprintf(fmt, ap)) == NULL)
		errx(1, "startpage_add failed");
	va_end(ap);

	s = g_malloc0(sizeof *s);
	s->line = msg;

	TAILQ_INSERT_TAIL(&spl, s, entry);
}
gchar *show_g_object_settings(GObject *, char *, int);

char *
xt_g_object_serialize(GValue *value, const gchar *tname, char *str, int recurse)
{
	int		typeno = 0;
	char		*valstr, *tmpstr, *tmpsettings;
	GObject		*object;

	typeno = G_TYPE_FUNDAMENTAL( G_VALUE_TYPE(value) );
	switch ( typeno ) {
	case G_TYPE_ENUM:
		valstr = g_strdup_printf("%d",
		    g_value_get_enum(value));
		break;
	case G_TYPE_CHAR:
		valstr = g_strdup_printf("%c",
#if GLIB_CHECK_VERSION(2, 32, 0)
		    g_value_get_schar(value));
#else
		    g_value_get_char(value));
#endif
		break;
	case G_TYPE_UCHAR:
		valstr = g_strdup_printf("%c",
		    g_value_get_uchar(value));
		break;
	case G_TYPE_LONG:
		valstr = g_strdup_printf("%ld",
		    g_value_get_long(value));
		break;
	case G_TYPE_ULONG:
		valstr = g_strdup_printf("%ld",
		    g_value_get_ulong(value));
		break;
	case G_TYPE_INT:
		valstr = g_strdup_printf("%d",
		    g_value_get_int(value));
		break;
	case G_TYPE_INT64:
		valstr = g_strdup_printf("%" PRIo64,
		    (int64_t) g_value_get_int64(value));
		break;
	case G_TYPE_UINT:
		valstr = g_strdup_printf("%d",
		    g_value_get_uint(value));
		break;
	case G_TYPE_UINT64:
		valstr = g_strdup_printf("%" PRIu64,
		    (uint64_t) g_value_get_uint64(value));
		break;
	case G_TYPE_FLAGS:
		valstr = g_strdup_printf("0x%x",
		    g_value_get_flags(value));
		break;
	case G_TYPE_BOOLEAN:
		valstr = g_strdup_printf("%s",
		    g_value_get_boolean(value) ? "TRUE" : "FALSE");
		break;
	case G_TYPE_FLOAT:
		valstr = g_strdup_printf("%f",
		    g_value_get_float(value));
		break;
	case G_TYPE_DOUBLE:
		valstr = g_strdup_printf("%f",
		    g_value_get_double(value));
		break;
	case G_TYPE_STRING:
		valstr = g_strdup_printf("\"%s\"",
		    g_value_get_string(value));
		break;
	case G_TYPE_POINTER:
		valstr = g_strdup_printf("%p",
		    g_value_get_pointer(value));
		break;
	case G_TYPE_OBJECT:
		object = g_value_get_object(value);
		if (object != NULL) {
			if (recurse) {
				tmpstr = g_strdup_printf("%s     ", str);
				tmpsettings = show_g_object_settings( object,
				    tmpstr, recurse);
				g_free(tmpstr);

				if (strrchr(tmpsettings, '\n') != NULL) {
					valstr = g_strdup_printf("%s%s      }",
					    tmpsettings, str);
					g_free(tmpsettings);
				} else {
					valstr = tmpsettings;
				}
			} else {
				valstr = g_strdup_printf("<...>");
			}
		} else {
			valstr = g_strdup_printf("settings[] = NULL");
		}
		break;
	default:
		valstr = g_strdup_printf("type %s unhandled", tname);
	}
	return valstr;
}

gchar *
show_g_object_settings(GObject *o, char *str, int recurse)
{
	char		*b, *p, *body, *valstr, *tmpstr;
	guint		n_props = 0;
	int		i, typeno = 0;
	GParamSpec	*pspec;
	const gchar	*tname;
	GValue		value;
	GParamSpec	**proplist;
	const gchar	*name;

	if (!G_IS_OBJECT(o)) {
		fprintf(stderr, "%s is not a g_object\n", str);
		return g_strdup("");
	}
	proplist = g_object_class_list_properties(
	    G_OBJECT_GET_CLASS(o), &n_props);

	if (GTK_IS_WIDGET(o)) {
		name = gtk_widget_get_name(GTK_WIDGET(o));
	} else {
		name = "settings";
	}
	if (n_props == 0) {
		body = g_strdup_printf("%s[0] = { }", name);
		goto end_show_g_objects;
	}

	body = g_strdup_printf("%s[%d] = {\n", name, n_props);
	for (i=0; i < n_props; i++) {
		pspec = proplist[i];
		tname = G_OBJECT_TYPE_NAME(pspec);
		bzero(&value, sizeof value);
		valstr = NULL;

		if (!(pspec->flags & G_PARAM_READABLE))
			valstr = g_strdup_printf("not a readable property");
		else {
			g_value_init(&value, G_PARAM_SPEC_VALUE_TYPE(pspec));
			g_object_get_property(G_OBJECT(o), pspec->name,
			    &value);
			typeno = G_TYPE_FUNDAMENTAL( G_VALUE_TYPE(&value) );
		}

		/* based on the type, recurse and display values */
		if (valstr == NULL) {
			valstr = xt_g_object_serialize(&value, tname, str,
			    recurse);
		}

		tmpstr = g_strdup_printf("%-13s %s%s%s,", tname, pspec->name,
			(typeno == G_TYPE_OBJECT) ? "." : " = ", valstr);
		b = body;

#define XT_G_OBJECT_SPACING		50
		p = strrchr(tmpstr, '\n');
		if (p == NULL && strlen(tmpstr) > XT_G_OBJECT_SPACING) {
			body = g_strdup_printf(
			    "%s%s    %-50s\n%s    %50s /* %3d flags=0x%08x */\n",
			    body, str, tmpstr, str, "", i, pspec->flags);
		} else {
			char *fmt;
			int strspaces;
			if (p == NULL)
				strspaces = XT_G_OBJECT_SPACING;
			else
				strspaces = strlen(tmpstr) - (strlen(p) - strlen(str)) + XT_G_OBJECT_SPACING + 5;
			fmt = g_strdup_printf("%%s%%s    %%-%ds /* %%3d flags=0x%%08x */\n", strspaces);
			body = g_strdup_printf(fmt, body, str, tmpstr, i, pspec->flags);
			g_free(fmt);
		}
		g_free(tmpstr);
		g_free(b);
		g_free(valstr);
	}
end_show_g_objects:
	g_free(proplist);
	return (body);
}

char *
xt_append_settings(char *str, GObject *object, char *name, int recurse)
{
	char *newstr, *settings;

	settings = show_g_object_settings(object, name, recurse);
	if (str == NULL)
		str = g_strdup("");

	newstr = g_strdup_printf("%s%s %s%s };\n", str, name, settings, name);
	g_free(str);

	return newstr;
}

int
about_webkit(struct tab *t, struct karg *arg)
{
	char			*page, *body, *settingstr;

	settingstr = xt_append_settings(NULL, G_OBJECT(t->settings),
	    "t->settings", 0);
	body = g_strdup_printf("<pre>%s</pre>\n", settingstr);
	g_free(settingstr);

	page = get_html_page("About Webkit", body, "", 0);
	g_free(body);

	load_webkit_string(t, page, XT_URI_ABOUT_WEBKIT, 0);
	g_free(page);

	return (0);
}

static int		toplevelcount = 0;

void
xt_append_toplevel(GtkWindow *w, char **body)
{
	char			*n;

	n = g_strdup_printf("toplevel#%d", toplevelcount++);
	*body = xt_append_settings(*body, G_OBJECT(w), n, 0);
	g_free(n);
}

int
allthethings(struct tab *t, struct karg *arg)
{
	GList			*list;
	char			*page, *body, *b;

	body = xt_append_settings(NULL, G_OBJECT(t->wv), "t->wv", 1);
	body = xt_append_settings(body, G_OBJECT(t->inspector),
	    "t->inspector", 1);
#if 0 /* not until warnings are gone */
	body = xt_append_settings(body, G_OBJECT(session),
	    "session", 1);
#endif
	toplevelcount = 0;
	list = gtk_window_list_toplevels();
	g_list_foreach(list, (GFunc)g_object_ref, NULL);
	g_list_foreach(list, (GFunc)xt_append_toplevel, &body);
	g_list_foreach(list, (GFunc)g_object_unref, NULL);
	g_list_free(list);
		
	b = body;
	body = g_strdup_printf("<pre>%scan paste clipboard = %d\n</pre>", body,
	    webkit_web_view_can_paste_clipboard(t->wv));
	g_free(b);

	page = get_html_page("About All The Things _o/", body, "", 0);
	g_free(body);

	load_webkit_string(t, page, XT_URI_ABOUT_ALLTHETHINGS, 0);
	g_free(page);

	return (0);
}
