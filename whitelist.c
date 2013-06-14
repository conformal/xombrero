/*
 * Copyright (c) 2010, 2011 Marco Peereboom <marco@peereboom.us>
 * Copyright (c) 2011 Stevan Andjelkovic <stevan@student.chalmers.se>
 * Copyright (c) 2010, 2011 Edd Barrett <vext01@gmail.com>
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

gchar *
find_domain(const gchar *s, int flags)
{
	SoupURI			*uri;
	gchar			*ret, *p;

	if (s == NULL)
		return (NULL);

	uri = soup_uri_new(s);

	if (uri == NULL)
		return (NULL);
	if (!SOUP_URI_VALID_FOR_HTTP(uri)) {
		soup_uri_free(uri);
		return (NULL);
	}

	if (flags & XT_WL_TOPLEVEL &&
	    !isdigit((unsigned char)uri->host[strlen(uri->host) - 1]))
		p = tld_get_suffix(uri->host);
	else
		p = uri->host;

	if (flags & XT_WL_TOPLEVEL)
		ret = g_strdup_printf(".%s", p);
	else	/* assume FQDN */
		ret = g_strdup(p);

	soup_uri_free(uri);

	return (ret);
}

struct wl_entry *
wl_find(const gchar *s, struct wl_list *wl)
{
	struct wl_entry		*w;

	if (s == NULL || strlen(s) == 0 || wl == NULL)
		return (NULL);

	TAILQ_FOREACH(w, wl, entry) {
		if (w->re == NULL)
			continue;
		if (!regexec(w->re, s, 0, 0, 0))
			return (w);
	}

	return (NULL);
}

int
wl_save(struct tab *t, struct karg *args, int list)
{
	char			file[PATH_MAX], *lst_str = NULL;
	FILE			*f = NULL;
	char			*line = NULL, *lt = NULL, *dom;
	size_t			linelen;
	const gchar		*uri;
	struct karg		a;
	struct wl_entry		*w;
	GSList			*cf;
	SoupCookie		*ci, *c;

	if (t == NULL || args == NULL)
		return (1);

	if (runtime_settings[0] == '\0')
		return (1);

	switch (list) {
	case XT_WL_JAVASCRIPT:
		lst_str = "JavaScript";
		break;
	case XT_WL_COOKIE:
		lst_str = "Cookie";
		break;
	case XT_WL_PLUGIN:
		lst_str = "Plugin";
		break;
	case XT_WL_HTTPS:
		lst_str = "HTTPS";
		break;
	default:
		show_oops(t, "Invalid list id: %d", list);
		return (1);
	}

	uri = get_uri(t);
	dom = find_domain(uri, args->i);
	if (uri == NULL || dom == NULL ||
	    webkit_web_view_get_load_status(t->wv) == WEBKIT_LOAD_FAILED) {
		show_oops(t, "Can't add domain to %s white list", lst_str);
		goto done;
	}

	switch (list) {
	case XT_WL_JAVASCRIPT:
		lt = g_strdup_printf("js_wl=%s", dom);
		break;
	case XT_WL_COOKIE:
		lt = g_strdup_printf("cookie_wl=%s", dom);
		break;
	case XT_WL_PLUGIN:
		lt = g_strdup_printf("pl_wl=%s", dom);
		break;
	case XT_WL_HTTPS:
		lt = g_strdup_printf("force_https=%s", dom);
		break;
	default:
		/* can't happen */
		show_oops(t, "Invalid list id: %d", list);
		goto done;
	}

	snprintf(file, sizeof file, "%s" PS "%s", work_dir, runtime_settings);
	if ((f = fopen(file, "r+")) == NULL) {
		show_oops(t, "can't open file %s");
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
		w = wl_find(dom, &js_wl);
		if (w == NULL) {
			settings_add("js_wl", dom);
			w = wl_find(dom, &js_wl);
		}
		toggle_js(t, &a);
		break;

	case XT_WL_COOKIE:
		w = wl_find(dom, &c_wl);
		if (w == NULL) {
			settings_add("cookie_wl", dom);
			w = wl_find(dom, &c_wl);
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
		w = wl_find(dom, &pl_wl);
		if (w == NULL) {
			settings_add("pl_wl", dom);
			w = wl_find(dom, &pl_wl);
		}
		toggle_pl(t, &a);
		break;
	case XT_WL_HTTPS:
		w = wl_find(dom, &force_https);
		if (w == NULL) {
			settings_add("force_https", dom);
			w = wl_find(dom, &force_https);
		}
		toggle_force_https(t, &a);
		break;
	default:
		abort(); /* can't happen */
	}
	if (w != NULL)
		w->handy = 1;

done:
	if (line)
		free(line);
	if (dom)
		g_free(dom);
	if (lt)
		g_free(lt);
	if (f)
		fclose(f);

	return (0);
}

int
wl_show(struct tab *t, struct karg *args, char *title, struct wl_list *wl)
{
	struct wl_entry		*w;
	char			*tmp, *body;

	body = g_strdup("");

	/* p list */
	if (args->i & XT_WL_PERSISTENT) {
		tmp = body;
		body = g_strdup_printf("%s<h2>Persistent</h2>", body);
		g_free(tmp);
		TAILQ_FOREACH(w, wl, entry) {
			if (w->handy == 0)
				continue;
			tmp = body;
			body = g_strdup_printf("%s%s<br/>", body, w->pat);
			g_free(tmp);
		}
	}

	/* s list */
	if (args->i & XT_WL_SESSION) {
		tmp = body;
		body = g_strdup_printf("%s<h2>Session</h2>", body);
		g_free(tmp);
		TAILQ_FOREACH(w, wl, entry) {
			if (w->handy == 1)
				continue;
			tmp = body;
			body = g_strdup_printf("%s%s<br/>", body, w->pat);
			g_free(tmp);
		}
	}

	tmp = get_html_page(title, body, "", 0);
	g_free(body);
	if (wl == &js_wl)
		load_webkit_string(t, tmp, XT_URI_ABOUT_JSWL, 0);
	else if (wl == &c_wl)
		load_webkit_string(t, tmp, XT_URI_ABOUT_COOKIEWL, 0);
	else if (wl == &pl_wl)
		load_webkit_string(t, tmp, XT_URI_ABOUT_PLUGINWL, 0);
	else if (wl == &force_https)
		load_webkit_string(t, tmp, XT_URI_ABOUT_HTTPS, 0);
	g_free(tmp);
	return (0);
}

void
wl_add(const char *str, struct wl_list *wl, int flags)
{
	struct wl_entry		*w;
	int			add_dot = 0, chopped = 0;
	const char		*s = str;
	char			*escstr, *p, *pat;
	char			**sv;

	if (str == NULL || wl == NULL || strlen(str) < 2)
		return;

	DNPRINTF(XT_D_COOKIE, "wl_add in: %s\n", str);

	/* slice off port number */
	p = g_strrstr(str, ":");
	if (p)
		*p = '\0';

	w = g_malloc(sizeof *w);
	w->re = g_malloc(sizeof *w->re);
	if (flags & XT_WL_REGEX) {
		w->pat = g_strdup_printf("re:%s", str);
		regcomp(w->re, str, REG_EXTENDED | REG_NOSUB);
		DNPRINTF(XT_D_COOKIE, "wl_add: %s\n", str);
	} else {
		/* treat *.moo.com the same as .moo.com */
		if (s[0] == '*' && s[1] == '.')
			s = &s[1];
		else if (s[0] != '.' && (flags & XT_WL_TOPLEVEL))
			add_dot = 1;

		if (s[0] == '.') {
			s = &s[1];
			chopped = 1;
		}
		sv = g_strsplit(s, ".", 0);
		escstr = g_strjoinv("\\.", sv);
		g_strfreev(sv);

		if (add_dot) {
			w->pat = g_strdup_printf(".%s", str);
			pat = g_strdup_printf("^(.*\\.)*%s$", escstr);
			regcomp(w->re, pat, REG_EXTENDED | REG_NOSUB);
		} else {
			w->pat = g_strdup(str);
			if (chopped)
				pat = g_strdup_printf("^(.*\\.)*%s$", escstr);
			else
				pat = g_strdup_printf("^%s$", escstr);
			regcomp(w->re, pat, REG_EXTENDED | REG_NOSUB);
		}
		DNPRINTF(XT_D_COOKIE, "wl_add: %s\n", pat);
		g_free(escstr);
		g_free(pat);
	}

	w->handy = (flags & XT_WL_PERSISTENT) ? 1 : 0;

	TAILQ_INSERT_HEAD(wl, w, entry);

	return;
}

int
add_cookie_wl(struct settings *s, char *entry)
{
	if (g_str_has_prefix(entry, "re:")) {
		entry = &entry[3];
		wl_add(entry, &c_wl, XT_WL_PERSISTENT | XT_WL_REGEX);
	} else
		wl_add(entry, &c_wl, XT_WL_PERSISTENT);
	return (0);
}

int
add_js_wl(struct settings *s, char *entry)
{
	if (g_str_has_prefix(entry, "re:")) {
		entry = &entry[3];
		wl_add(entry, &js_wl, XT_WL_PERSISTENT | XT_WL_REGEX);
	} else
		wl_add(entry, &js_wl, XT_WL_PERSISTENT);
	return (0);
}

int
add_pl_wl(struct settings *s, char *entry)
{
	if (g_str_has_prefix(entry, "re:")) {
		entry = &entry[3];
		wl_add(entry, &pl_wl, XT_WL_PERSISTENT | XT_WL_REGEX);
	} else
		wl_add(entry, &pl_wl, XT_WL_PERSISTENT);
	return (0);
}

int
toggle_cwl(struct tab *t, struct karg *args)
{
	struct wl_entry		*w;
	const gchar		*uri;
	char			*dom = NULL;
	int			es;

	if (args == NULL)
		return (1);

	uri = get_uri(t);
	dom = find_domain(uri, args->i);

	if (uri == NULL || dom == NULL ||
	    webkit_web_view_get_load_status(t->wv) == WEBKIT_LOAD_FAILED) {
		show_oops(t, "Can't toggle domain in cookie white list");
		goto done;
	}
	w = wl_find(dom, &c_wl);

	if (w == NULL)
		es = 0;
	else
		es = 1;

	if (args->i & XT_WL_TOGGLE)
		es = !es;
	else if ((args->i & XT_WL_ENABLE) && es != 1)
		es = 1;
	else if ((args->i & XT_WL_DISABLE) && es != 0)
		es = 0;

	if (es) {
		/* enable cookies for domain */
		args->i |= !XT_WL_PERSISTENT;
		wl_add(dom, &c_wl, args->i);
	} else {
		/* disable cookies for domain */
		if (w != NULL) {
			TAILQ_REMOVE(&c_wl, w, entry);
			g_free(w->re);
			g_free(w->pat);
		}
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
	struct wl_entry		*w;
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
	dom = find_domain(uri, args->i);

	if (uri == NULL || dom == NULL ||
	    webkit_web_view_get_load_status(t->wv) == WEBKIT_LOAD_FAILED) {
		show_oops(t, "Can't toggle domain in JavaScript white list");
		goto done;
	}

	if (es) {
		button_set_stockid(t->js_toggle, GTK_STOCK_MEDIA_PLAY);
		args->i |= !XT_WL_PERSISTENT;
		wl_add(dom, &js_wl, args->i);
	} else {
		w = wl_find(dom, &js_wl);
		if (w != NULL) {
			TAILQ_REMOVE(&js_wl, w, entry);
			g_free(w->re);
			g_free(w->pat);
		}
		button_set_stockid(t->js_toggle, GTK_STOCK_MEDIA_PAUSE);
	}
	g_object_set(G_OBJECT(t->settings),
	    "enable-scripts", es, (char *)NULL);
	g_object_set(G_OBJECT(t->settings),
	    "javascript-can-open-windows-automatically", js_auto_open_windows ? es : 0, (char *)NULL);
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
	struct wl_entry		*w;
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
	dom = find_domain(uri, args->i);

	if (uri == NULL || dom == NULL ||
	    webkit_web_view_get_load_status(t->wv) == WEBKIT_LOAD_FAILED) {
		show_oops(t, "Can't toggle domain in plugins white list");
		goto done;
	}

	if (es) {
		args->i |= !XT_WL_PERSISTENT;
		wl_add(dom, &pl_wl, args->i);
	} else {
		w = wl_find(dom, &pl_wl);
		if (w != NULL) {
			TAILQ_REMOVE(&pl_wl, w, entry);
			g_free(w->re);
			g_free(w->pat);
		}
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

int
toggle_force_https(struct tab *t, struct karg *args)
{
	int			es;
	const gchar		*uri;
	struct wl_entry		*w;
	char			*dom = NULL;

	if (args == NULL)
		return (1);

	uri = get_uri(t);
	dom = find_domain(uri, args->i);

	if (uri == NULL || dom == NULL ||
	    webkit_web_view_get_load_status(t->wv) == WEBKIT_LOAD_FAILED) {
		show_oops(t, "Can't toggle domain in https force list");
		goto done;
	}
	w = wl_find(dom, &force_https);

	if (w == NULL)
		es = 0;
	else
		es = 1;

	if (args->i & XT_WL_TOGGLE)
		es = !es;
	else if ((args->i & XT_WL_ENABLE) && es != 1)
		es = 1;
	else if ((args->i & XT_WL_DISABLE) && es != 0)
		es = 0;

	uri = get_uri(t);
	dom = find_domain(uri, args->i);

	if (es) {
		args->i |= !XT_WL_PERSISTENT;
		wl_add(dom, &force_https, args->i);
	} else {
		w = wl_find(dom, &force_https);
		if (w != NULL) {
			TAILQ_REMOVE(&force_https, w, entry);
			g_free(w->re);
			g_free(w->pat);
		}
	}

	if (args->i & XT_WL_RELOAD)
		webkit_web_view_reload(t->wv);
done:
	if (dom)
		g_free(dom);
	return (0);
}
