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

int		enable_plugin_whitelist = 0;
int		enable_cookie_whitelist = 0;
int		enable_js_whitelist = 0;

RB_HEAD(domain_list, domain);

int
domain_rb_cmp(struct domain *d1, struct domain *d2)
{
	return (strcmp(d1->d, d2->d));
}
RB_GENERATE(domain_list, domain, entry, domain_rb_cmp);

struct domain_list	c_wl;
struct domain_list	js_wl;
struct domain_list	pl_wl;

void
wl_init(void)
{
	RB_INIT(&js_wl);
	RB_INIT(&pl_wl);
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
cookie_show_wl(struct tab *t, struct karg *args)
{
	args->i = XT_SHOW | XT_WL_PERSISTENT | XT_WL_SESSION;
	wl_show(t, args, "Cookie White List", &c_wl);

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
