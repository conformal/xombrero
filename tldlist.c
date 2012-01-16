/*
 * Copyright (c) 2012 Elias Norberg <xyzzy@kudzu.se>
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

#define TLD_TREE_END_NODE 1
#define TLD_TREE_EXCEPTION 2

struct tld_tree_node {
	struct tld_tree_node	*next;
	struct tld_tree_node	*child;
	const char		*lbl;
	char			flags;
};

struct tld_tree_node tld_tree_root = { NULL, NULL, "" };

#define TREE_INSERT_CHILD(n, data) \
		n->child = g_malloc(sizeof *n); \
		n->child->next = NULL; \
		n->child->child = NULL; \
		n->child->flags = 0; \
		n->child->lbl = data;

#define TREE_INSERT_NEXT(n, data) \
		n->next = g_malloc(sizeof *n); \
		n->next->next = NULL; \
		n->next->child = NULL; \
		n->next->flags = 0; \
		n->next->lbl = data;

#define P_BASE		(36)
#define P_TMIN		(1)
#define P_TMAX		(26)
#define P_SKEW		(38)
#define P_DAMP		(700)
#define INITIAL_BIAS	(72)
#define INITIAL_N	(128)

int
adapt(int delta, int numpoints, int firsttime)
{
	int		k;

	if (firsttime)
		delta = delta / P_DAMP;
	else
		delta = delta / 2;

	delta += (delta / numpoints);

	k = 0;
	while (delta > (((P_BASE - P_TMIN) * P_TMAX) / 2)) {
		delta = delta / (P_BASE - P_TMIN);
		k += P_BASE;
	}

	k += (((P_BASE - P_TMIN + 1) * delta) / (delta + P_SKEW));
	return (k);
}

int
get_minimum_char(char *str, int n)
{
	gunichar	ch = 0;
	gunichar	min = 0xffffff;

	for(; *str; str = g_utf8_next_char(str)) {
		ch = g_utf8_get_char(str);
		if (ch >= n && ch < min)
			min = ch;
	}

	return (min);
}

char
encode_digit(int n)
{
	if (n < 26)
		return n + 'a';
	return (n - 26) + '0';
}

char *
punycode_encode(char *str)
{
	char		output[1024];
	char		*s;
	gunichar	c;
	int		need_coding = 0;
	int		l, len, i;

	int		n = INITIAL_N;
	int		delta = 0;
	int		bias = INITIAL_BIAS;
	int		h, b, m, k, t, q;

	l = 0;
	for (s=str; *s; s = g_utf8_next_char(s)) {
		c = g_utf8_get_char(s);
		if (c < 128)
			output[l++] = *s;
		else
			need_coding = 1;
	}

	output[l] = '\0';

	if (!need_coding)
		return g_strdup(output);

	h = b = strlen(output);

	if (l > 0)
		output[l++] = '-';
	output[l] = '\0';

	len = g_utf8_strlen(str, -1);
	while (h < len) {
		m = get_minimum_char(str, n);
		delta += (m - n) * (h + 1);
		n = m;
		for (s=str; *s; s = g_utf8_next_char(s)) {
			c = g_utf8_get_char(s);

			if (c < n) delta ++;
			if (c == n) {
				q = delta;
				for (k=P_BASE;; k+=P_BASE) {
					if (k <= bias)
						t = P_TMIN;
					else if(k >= bias + P_TMAX)
						t = P_TMAX;
					else
						t = k - bias;

					if (q < t)
						break;

					output[l++] = encode_digit(t+((q-t)%(P_BASE-t)));
					q = (q - t) / (P_BASE - t);
				}
				output[l++] = encode_digit(q);
				bias = adapt(delta, h + 1, h == b);
				delta = 0;
				h ++;
			}
		}
		delta ++;
		n ++;
	}

	output[l] = '\0';
	for (i=l+4;i>=4;i--)
		output[i] = output[i-4];
	l += 4;
	output[0] = 'x';
	output[1] = 'n';
	output[2] = '-';
	output[3] = '-';
	output[l] = '\0';
	return g_strdup(output);
}

/*
 * strrchr2(str, saveptr, ch)
 *
 * Walk backwards through str, jumping to next 'ch'
 * On first call, *saveptr should be set to NULL.
 * On following calls, supply the same saveptr.
 *
 * Returns NULL when the whole string 'str' has been
 * looped through. Otherwise returns the position
 * before the next 'ch'.
 */
const char *
strrchr2(const char *str, const char **saveptr, int ch)
{
	const char		*ptr;

	if (str != NULL && *saveptr == NULL) {
		*saveptr = str + strlen(str);
	} else if (str == *saveptr) {
		return (NULL);
	}

	for (ptr= *saveptr - 1; ptr != str && *ptr != ch; ptr--)
		;

	*saveptr = ptr;
	if (ptr != str)
		return (ptr+1);
	return (ptr);
}

/*
 * tld_tree_add(rule)
 *
 * Adds a tld-rule to the tree
 */
void
tld_tree_add(const char *rule)
{
	struct tld_tree_node	*n;
	const char		*lbl;
	const char		*saveptr;

	saveptr = NULL;
	lbl = strrchr2(rule, &saveptr, '.');

	for (n = &tld_tree_root; lbl != NULL;) {

		if (strcmp(n->lbl, lbl) == 0) {
			lbl = strrchr2(rule, &saveptr, '.');

			if (!n->child)
				break;

			n = n->child;
			continue;
		}

		if (n->next == NULL) {
			TREE_INSERT_NEXT(n, lbl);
			n = n->next;

			lbl = strrchr2(rule, &saveptr, '.');
			break;
		}
		n = n->next;
	}

	while (lbl) {
		TREE_INSERT_CHILD(n, lbl);

		lbl = strrchr2(rule, &saveptr, '.');
		n = n->child;
	}

	n->flags |= TLD_TREE_END_NODE;
	if (n->lbl[0] == '!') {
		n->flags |= TLD_TREE_EXCEPTION;
		n->lbl ++;
	}
}

void
tld_tree_init()
{
	FILE		*fd;
	char		buf[1024];
	char		file[PATH_MAX];
	char		*ptr, *next_lbl;
	char		*enc_lbl;
	char		*rule, *rp;
	char		extra_ch;

	snprintf(file, sizeof file, "%s/tld-rules", resource_dir);
	fd = fopen(file, "r");
	if (fd == NULL) {
		/* a poor replacement for the real list - but it's
		 * better than nothing.
		 */
		tld_tree_add("*");
		startpage_add("Could not open %s ant this file is required"
		    "to handle TLD whitelisting properly", file);
		return;
	}

	for (;;) {
		ptr = fgets(buf, sizeof buf, fd);
		if (ptr == NULL || feof(fd))
			break;

		/* skip comments */
		if ((ptr = strstr(buf, "//")) != NULL)
			*ptr = '\0';
		/* skip anything after space or tab */
		for (ptr = buf; *ptr; ptr++)
			if (*ptr == ' ' || *ptr == '\t' ||
			    *ptr == '\n' || *ptr == '\r')
				break;
		*ptr = '\0';

		if (!strlen(buf))
			continue;

		extra_ch = 0;
		ptr = buf;
		if (buf[0] == '!' && buf[0] == '*') {
			extra_ch = buf[0];
			ptr ++;
		}


		rule = NULL;
		/* split into labels, and convert them one by one */
		for (;;) {
			if ((next_lbl = strchr(ptr, '.')))
				*next_lbl = '\0';

			enc_lbl = punycode_encode(ptr);
			if (rule) {
				rp = rule;
				rule = g_strdup_printf("%s%s%s", rp, enc_lbl,
				    next_lbl ? "." : "");
				g_free(rp);
				g_free(enc_lbl);
			} else {
				rule = g_strdup_printf("%.1s%s%s",
				    extra_ch ? buf:"", enc_lbl,
				    next_lbl ? ".":"");
				g_free(enc_lbl);
			}

			if (!next_lbl)
				break;
			ptr = next_lbl + 1;
		}
		tld_tree_add(rule);
	}

	fclose(fd);
}

/*
 * tld_get_suffix(domain)
 *
 * Find the public suffix for domain.
 *
 * Returns a pointer to the suffix position
 * in domain, or NULL if no public suffix
 * was found.
 */
char *
tld_get_suffix(const char *domain)
{
	struct tld_tree_node	*n;
	const char		*suffix;
	const char		*lbl, *saveptr;
	const char		*tmp_saveptr, *tmp_lbl;

	if (domain == NULL)
		return (NULL);
	if (domain[0] == '.')
		return (NULL);

	saveptr = NULL;
	suffix = NULL;
	lbl = strrchr2(domain, &saveptr, '.');

	for (n = &tld_tree_root; n != NULL && lbl != NULL;) {

		if (!strlen(n->lbl)) {
			n = n->next;
			continue;
		}

		if (n->lbl[0] == '*') {
			if (n->flags & TLD_TREE_END_NODE) {

				tmp_saveptr = saveptr;
				tmp_lbl = lbl;

				lbl = strrchr2(domain, &saveptr, '.');

				/* Save possible public suffix */
				suffix = lbl;
				saveptr = tmp_saveptr;
				lbl = tmp_lbl;
			}

			n = n->next;
			continue;
		}

		if (strcmp(n->lbl, lbl) == 0) {
			if (n->flags & TLD_TREE_EXCEPTION) {
				/* We're done looking */
				suffix = lbl;
				break;
			}

			lbl = strrchr2(domain, &saveptr, '.');

			/* Possible public suffix - other rules might
			 * still apply
			 */
			if (n->flags & TLD_TREE_END_NODE)
				suffix = lbl;

			/* Domain too short */
			if (lbl == NULL) {
				/* Check if we have a child that is '*' */
				for (n = n->child; n; n = n->next)
					if (n->lbl[0] == '*')
						suffix = NULL;
				break;
			}

			if (n->child == NULL)
				break;

			n = n->child;
			continue;
		}

		if (n->next == NULL)
			break;
		n = n->next;
	}

	/* If we can't find a matching suffix, it can mean that either
	 * a) the user is surfing a local prefix
	 * b) the list is not properly updated
	 *
	 * In any case - in order not to break stuff while surfing
	 * new TLD's, we return the public suffix as the top 2 labels
	 *
	 * www.abc.xyz therefore has public suffix 'abc.xyz'
	 */
	if (!suffix) {
		saveptr = NULL;
		lbl = strrchr2(domain, &saveptr, '.');
		lbl = strrchr2(domain, &saveptr, '.');
		suffix = lbl;
	}

	return ((char*)suffix);
}
