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
				"</style>\n"

/* XTP classes (xxxt://<class>) */
#define XT_XTP_INVALID		(0)	/* invalid */
#define XT_XTP_DL		(1)	/* downloads */
#define XT_XTP_HL		(2)	/* history */
#define XT_XTP_CL		(3)	/* cookies */
#define XT_XTP_FL		(4)	/* favorites */

/* XTP download actions */
#define XT_XTP_DL_LIST		(1)
#define XT_XTP_DL_CANCEL	(2)
#define XT_XTP_DL_REMOVE	(3)
#define XT_XTP_DL_UNLINK	(4)

/* XTP history actions */
#define XT_XTP_HL_LIST		(1)
#define XT_XTP_HL_REMOVE	(2)

/* XTP cookie actions */
#define XT_XTP_CL_LIST		(1)
#define XT_XTP_CL_REMOVE	(2)
#define XT_XTP_CL_REMOVE_DOMAIN	(3)

/* XTP cookie actions */
#define XT_XTP_FL_LIST		(1)
#define XT_XTP_FL_REMOVE	(2)

int			js_show_wl(struct tab *, struct karg *);
int			pl_show_wl(struct tab *, struct karg *);
int			set(struct tab *, struct karg *);
int			marco(struct tab *, struct karg *);
int			startpage(struct tab *, struct karg *);
const char *		marco_message(int *);

struct about_type about_list[] = {
	{ XT_URI_ABOUT_ABOUT,		about },
	{ XT_URI_ABOUT_BLANK,		blank },
	{ XT_URI_ABOUT_CERTS,		ca_cmd },
	{ XT_URI_ABOUT_COOKIEWL,	cookie_show_wl },
	{ XT_URI_ABOUT_COOKIEJAR,	xtp_page_cl },
	{ XT_URI_ABOUT_DOWNLOADS,	xtp_page_dl },
	{ XT_URI_ABOUT_FAVORITES,	xtp_page_fl },
	{ XT_URI_ABOUT_HELP,		help },
	{ XT_URI_ABOUT_HISTORY,		xtp_page_hl },
	{ XT_URI_ABOUT_JSWL,		js_show_wl },
	{ XT_URI_ABOUT_SET,		set },
	{ XT_URI_ABOUT_STATS,		stats },
	{ XT_URI_ABOUT_MARCO,		marco },
	{ XT_URI_ABOUT_STARTPAGE,	startpage },
	{ XT_URI_ABOUT_PLUGINWL,	pl_show_wl },
};

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

int			updating_fl_tabs = 0;
int			updating_dl_tabs = 0;
int			updating_hl_tabs = 0;
int			updating_cl_tabs = 0;
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
load_webkit_string(struct tab *t, const char *str, gchar *title)
{
	char			file[PATH_MAX];
	int			i;

	/* we set this to indicate we want to manually do navaction */
	if (t->bfl)
		t->item = webkit_web_back_forward_list_get_current_item(t->bfl);

	t->xtp_meaning = XT_XTP_TAB_MEANING_NORMAL;
	if (title) {
		/* set t->xtp_meaning */
		for (i = 0; i < LENGTH(about_list); i++)
			if (!strcmp(title, about_list[i].name)) {
				t->xtp_meaning = i;
				break;
			}

		webkit_web_view_load_string(t->wv, str, NULL, encoding,
		    "file://");
#if GTK_CHECK_VERSION(2, 20, 0)
		gtk_spinner_stop(GTK_SPINNER(t->spinner));
		gtk_widget_hide(t->spinner);
#endif
		snprintf(file, sizeof file, "%s/%s", resource_dir, icons[0]);
		xt_icon_from_file(t, file);
	}
}

int
blank(struct tab *t, struct karg *args)
{
	if (t == NULL)
		show_oops(NULL, "blank invalid parameters");

	load_webkit_string(t, "", XT_URI_ABOUT_BLANK);

	return (0);
}

int
about(struct tab *t, struct karg *args)
{
	char			*page, *body;

	if (t == NULL)
		show_oops(NULL, "about invalid parameters");

	body = g_strdup_printf("<b>Version: %s</b>"
#ifdef XXXTERM_BUILDSTR
	    "<br><b>Build: %s</b>"
#endif
	    "<p>"
	    "Authors:"
	    "<ul>"
	    "<li>Marco Peereboom &lt;marco@peereboom.us&gt;</li>"
	    "<li>Stevan Andjelkovic &lt;stevan@student.chalmers.se&gt;</li>"
	    "<li>Edd Barrett &lt;vext01@gmail.com&gt;</li>"
	    "<li>Todd T. Fries &lt;todd@fries.net&gt;</li>"
	    "<li>Raphael Graf &lt;r@undefined.ch&gt;</li>"
	    "<li>Michal Mazurek &lt;akfaew@jasminek.net&gt;</li>"
	    "</ul>"
	    "Copyrights and licenses can be found on the XXXTerm "
	    "<a href=\"http://opensource.conformal.com/wiki/XXXTerm\">website</a>"
	    "</p>",
#ifdef XXXTERM_BUILDSTR
	    version, XXXTERM_BUILDSTR
#else
	    version
#endif
	    );

	page = get_html_page("About", body, "", 0);
	g_free(body);

	load_webkit_string(t, page, XT_URI_ABOUT_ABOUT);
	g_free(page);

	return (0);
}

int
help(struct tab *t, struct karg *args)
{
	char			*page, *head, *body;

	if (t == NULL)
		show_oops(NULL, "help invalid parameters");

	head = "<meta http-equiv=\"REFRESH\" content=\"0;"
	    "url=http://opensource.conformal.com/cgi-bin/man-cgi?xxxterm\">"
	    "</head>\n";
	body = "XXXTerm man page <a href=\"http://opensource.conformal.com/"
	    "cgi-bin/man-cgi?xxxterm\">http://opensource.conformal.com/"
	    "cgi-bin/man-cgi?xxxterm</a>";

	page = get_html_page(XT_NAME, body, head, FALSE);

	load_webkit_string(t, page, XT_URI_ABOUT_HELP);
	g_free(page);

	return (0);
}

int
stats(struct tab *t, struct karg *args)
{
	char			*page, *body, *s, line[64 * 1024];
	long long unsigned int	line_count = 0;
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
			    "<br/>Cookies blocked(*) total: %llu", line_count);
		} else
			show_oops(t, "Can't open blocked cookies file: %s",
			    strerror(errno));
	}

	body = g_strdup_printf(
	    "Cookies blocked(*) this session: %llu"
	    "%s"
	    "<p><small><b>*</b> results vary based on settings</small></p>",
	    blocked_cookies,
	    line);

	page = get_html_page("Statistics", body, "", 0);
	g_free(body);

	load_webkit_string(t, page, XT_URI_ABOUT_STATS);
	g_free(page);

	return (0);
}

void
show_certs(struct tab *t, gnutls_x509_crt_t *certs,
    size_t cert_count, char *title)
{
	gnutls_datum_t		cinfo;
	char			*tmp, *body;
	int			i;

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

	tmp = get_html_page(title, body, "", 0);
	g_free(body);

	load_webkit_string(t, tmp, XT_URI_ABOUT_CERTS);
	g_free(tmp);
}

int
ca_cmd(struct tab *t, struct karg *args)
{
	FILE			*f = NULL;
	int			rv = 1, certs = 0, certs_read;
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
	c = g_malloc(sizeof(gnutls_x509_crt_t) * certs);
	certs_read = gnutls_x509_crt_list_import(c, (unsigned int *)&certs, &dt,
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

/*
 * cancel, remove, etc. downloads
 */
void
xtp_handle_dl(struct tab *t, uint8_t cmd, int id)
{
	struct download		find, *d = NULL;

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
		g_object_unref(d->download);
		break;
	case XT_XTP_DL_UNLINK:
		unlink(webkit_download_get_destination_uri(d->download) +
		    strlen("file://"));
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
	char			file[PATH_MAX], *title, *uri = NULL;
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
	new_favs = g_strdup("");

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

	if (fwrite(new_favs, strlen(new_favs), 1, f) != 1)
		show_oops(t, "%s: can't fwrite", __func__);
	fclose(f);

clean:
	if (uri)
		free(uri);
	if (title)
		free(title);

	g_free(new_favs);
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

	title = get_title(t, FALSE);
	uri = get_uri(t);

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
	case XT_XTP_CL_REMOVE_DOMAIN:
		remove_cookie_domain(arg);
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
	{ XT_XTP_INVALID, NULL, NULL }
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

void
xtp_generate_keys(void)
{
	/* generate session keys for xtp pages */
	generate_xtp_session_key(&dl_session_key);
	generate_xtp_session_key(&hl_session_key);
	generate_xtp_session_key(&cl_session_key);
	generate_xtp_session_key(&fl_session_key);
}

/*
 * validate a xtp session key.
 * return (1) if OK
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

/*
 * is the url xtp protocol? (xxxt://)
 * if so, parse and despatch correct bahvior
 */
int
parse_xtp_url(struct tab *t, const char *url)
{
	char			*dup = NULL, *p, *last = NULL;
	uint8_t			n_tokens = 0;
	char			*tokens[4] = {NULL, NULL, NULL, ""};
	struct xtp_despatch	*dsp, *dsp_match = NULL;
	uint8_t			req_class;
	int			ret = FALSE;

	/*
	 * tokens array meaning:
	 *   tokens[0] = class
	 *   tokens[1] = session key
	 *   tokens[2] = action
	 *   tokens[3] = optional argument
	 */

	DNPRINTF(XT_D_URL, "%s: url %s\n", __func__, url);

	if (strncmp(url, XT_XTP_STR, strlen(XT_XTP_STR)))
		goto clean;

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
	while (dsp->xtp_class) {
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
		ret = TRUE; /* all is well, this was a valid xtp request */
		dsp_match->handle_func(t, atoi(tokens[2]), atoi(tokens[3]));
	}

clean:
	if (dup)
		g_free(dup);

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

/* show a list of favorites (bookmarks) */
int
xtp_page_fl(struct tab *t, struct karg *args)
{
	char			file[PATH_MAX];
	FILE			*f;
	char			*uri = NULL, *title = NULL;
	size_t			len, lineno = 0;
	int			i, failed = 0;
	char			*body, *tmp, *page = NULL;
	const char		delim[3] = {'\\', '\\', '\0'};

	DNPRINTF(XT_D_FAVORITE, "%s:", __func__);

	if (t == NULL)
		warn("%s: bad param", __func__);

	/* new session key */
	if (!updating_fl_tabs)
		generate_xtp_session_key(&fl_session_key);

	/* open favorites */
	snprintf(file, sizeof file, "%s/%s", work_dir, XT_FAVS_FILE);
	if ((f = fopen(file, "r")) == NULL) {
		show_oops(t, "Can't open favorites file: %s", strerror(errno));
		return (1);
	}

	/* body */
	body = g_strdup_printf("<table style='table-layout:fixed'><tr>"
	    "<th style='width: 40px'>&#35;</th><th>Link</th>"
	    "<th style='width: 40px'>Rm</th></tr>\n");

	for (i = 1;;) {
		if ((title = fparseln(f, &len, &lineno, delim, 0)) == NULL)
			break;
		if (strlen(title) == 0) {
			free(title);
			title = NULL;
			continue;
		}

		if ((uri = fparseln(f, &len, &lineno, delim, 0)) == NULL)
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

	tmp = body;
	body = g_strdup_printf("%s</table>", body);
	g_free(tmp);

	if (uri)
		free(uri);
	if (title)
		free(title);

	/* render */
	if (!failed) {
		page = get_html_page("Favorites", body, "", 1);
		load_webkit_string(t, page, XT_URI_ABOUT_FAVORITES);
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
		    "<a href='%s%d/%d'>Remove</a> / <a href='%s%d/%d'>Unlink</a>",
		    xtp_prefix, XT_XTP_DL_REMOVE, dl->id, xtp_prefix,
		    XT_XTP_DL_UNLINK, dl->id);
		break;
	case WEBKIT_DOWNLOAD_STATUS_ERROR:
		status_html = g_strdup_printf("Error!");
		cmd_html = g_strdup_printf(
		    "<a href='%s%d/%d'>Remove</a> / <a href='%s%d/%d'>Unlink</a>",
		    xtp_prefix, XT_XTP_DL_REMOVE, dl->id, xtp_prefix,
		    XT_XTP_DL_UNLINK, dl->id);
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
	    html, basename((char *)webkit_download_get_destination_uri(dl->download)),
	    status_html, cmd_html);
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

	/* Generate a new session key */
	if (!updating_cl_tabs)
		generate_xtp_session_key(&cl_session_key);

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

	body = NULL;
	last_domain = strdup("");
	for (; sc; sc = sc->next) {
		c = sc->data;

		if (strcmp(last_domain, c->domain) != 0) {
			/* new domain */
			domain_id ++;
			free(last_domain);
			last_domain = strdup(c->domain);

			if (body != NULL) {
				tmp = body;
				body = g_strdup_printf("%s</table>"
				    "<h2>%s</h2>"
				    "<a href='%s%d/%s/%d/%d'>remove all</a>%s\n",
				    body, c->domain,
				    XT_XTP_STR, XT_XTP_CL,
				    cl_session_key, XT_XTP_CL_REMOVE_DOMAIN, domain_id,
				    table_headers);
				g_free(tmp);
			} else {
				/* first domain */
				body = g_strdup_printf("<h2>%s</h2>"
				    "<a href='%s%d/%s/%d/%d'>remove all</a>%s\n",
				    c->domain,
				    XT_XTP_STR, XT_XTP_CL,
				    cl_session_key, XT_XTP_CL_REMOVE_DOMAIN, domain_id,
				    table_headers);
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
	tmp = body;
	body = g_strdup_printf("%s</table>", body);
	g_free(tmp);

	page = get_html_page("Cookie Jar", body, "", TRUE);
	g_free(body);
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
	char			*body, *page, *tmp;
	struct history		*h;
	int			i = 1; /* all ids start 1 */

	DNPRINTF(XT_D_CMD, "%s", __func__);

	if (t == NULL) {
		show_oops(NULL, "%s invalid parameters", __func__);
		return (1);
	}

	/* Generate a new session key */
	if (!updating_hl_tabs)
		generate_xtp_session_key(&hl_session_key);

	/* body */
	body = g_strdup_printf("<table style='table-layout:fixed'><tr>"
	    "<th>URI</th><th>Title</th><th>Last visited</th><th style='width: 40px'>Rm</th></tr>\n");

	RB_FOREACH_REVERSE(h, history_list, &hl) {
		tmp = body;
		body = g_strdup_printf(
		    "%s\n<tr>"
		    "<td><a href='%s'>%s</a></td>"
		    "<td>%s</td>"
		    "<td>%s</td>"
		    "<td style='text-align: center'>"
		    "<a href='%s%d/%s/%d/%d'>X</a></td></tr>\n",
		    body, h->uri, h->uri, h->title, ctime(&h->time),
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
	char			*body, *page, *tmp;
	char			*ref;
	int			n_dl = 1;

	DNPRINTF(XT_D_DOWNLOAD, "%s", __func__);

	if (t == NULL) {
		show_oops(NULL, "%s invalid parameters", __func__);
		return (1);
	}

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

	body = g_strdup_printf("<div align='center'>"
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

	load_webkit_string(t, page, XT_URI_ABOUT_DOWNLOADS);
	g_free(page);

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

