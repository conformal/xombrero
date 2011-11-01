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

int			updating_fl_tabs = 0;
int			updating_hl_tabs = 0;

char			*dl_session_key;	/* downloads */
char			*hl_session_key;	/* history list */
char			*cl_session_key;	/* cookie list */
char			*fl_session_key;	/* favorites list */

#define XT_XTP_STR		"xxxt://"

/* XTP classes (xxxt://<class>) */
#define XT_XTP_INVALID		0	/* invalid */
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

/* xtp tab meanings  - identifies which tabs have xtp pages in (corresponding to about_list indices) */
#define XT_XTP_TAB_MEANING_NORMAL	-1 /* normal url */
#define XT_XTP_TAB_MEANING_BL		1 /* about:blank in this tab */
#define XT_XTP_TAB_MEANING_CL		4 /* cookie manager in this tab */
#define XT_XTP_TAB_MEANING_DL		5 /* download manager in this tab */
#define XT_XTP_TAB_MEANING_FL		6 /* favorite manager in this tab */
#define XT_XTP_TAB_MEANING_HL		8 /* history manager in this tab */

int	xtp_page_fl(struct tab *, struct karg *);
void	update_cookie_tabs(struct tab *);

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
		if (strlen(title) == 0 || title[0] == '#') {
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
			free(last_domain);
			last_domain = strdup(c->domain);

			if (body != NULL) {
				tmp = body;
				body = g_strdup_printf("%s</table>"
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
		show_oops(t, "%s: can't fwrite"); /* shut gcc up */
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
	{ XT_XTP_INVALID, NULL, NULL }
};

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

