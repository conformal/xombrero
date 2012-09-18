/*
 * Copyright (c) 2010, 2011 Marco Peereboom <marco@peereboom.us>
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

#define XT_REJECT_FILE		("rejected.txt")
#define XT_COOKIE_FILE		("cookies.txt")

/* hooked functions */
void		(*_soup_cookie_jar_add_cookie)(SoupCookieJar *, SoupCookie *);
void		(*_soup_cookie_jar_delete_cookie)(SoupCookieJar *,
		    SoupCookie *);

void
print_cookie(char *msg, SoupCookie *c)
{
	if (c == NULL)
		return;

	if (msg) {
		DNPRINTF(XT_D_COOKIE, "%s\n", msg);
	}

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
	struct wl_entry		*w = NULL;
	SoupCookie		*c;
	FILE			*r_cookie_f;
	char			*public_suffix;

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

	/* sanity */
	if (jar == NULL || cookie == NULL)
		return;

	/* check if domain is valid */
	public_suffix = tld_get_suffix(cookie->domain[0] == '.' ?
			cookie->domain + 1 : cookie->domain);

	if (public_suffix == NULL ||
	    (enable_cookie_whitelist &&
	    (w = wl_find(cookie->domain, &c_wl)) == NULL)) {
		blocked_cookies++;
		DNPRINTF(XT_D_COOKIE,
		    "soup_cookie_jar_add_cookie: reject %s\n",
		    cookie->domain);
		if (save_rejected_cookies) {
			if ((r_cookie_f = fopen(rc_fname, "a+")) == NULL) {
				show_oops(NULL, "can't open reject cookie file");
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
	if ((w && w->handy) || (enable_cookie_whitelist == 0)) {
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
		snprintf(rc_fname, sizeof file, "%s" PS "%s", work_dir,
		    XT_REJECT_FILE);

	/* persistent cookies */
	snprintf(file, sizeof file, "%s" PS "%s", work_dir, XT_COOKIE_FILE);
	p_cookiejar = soup_cookie_jar_text_new(file, read_only_cookies);

	/* session cookies */
	s_cookiejar = soup_cookie_jar_new();
	g_object_set(G_OBJECT(s_cookiejar), SOUP_COOKIE_JAR_ACCEPT_POLICY,
	    cookie_policy, (void *)NULL);
	transfer_cookies();

	soup_session_add_feature(session, (SoupSessionFeature*)s_cookiejar);
}

