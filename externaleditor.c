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

#if WEBKIT_CHECK_VERSION(1, 5, 0)
	/* we got the DOM API we need */

struct edit_src_cb_args {
		WebKitWebFrame		*frame;
		WebKitWebDataSource	*data_src;
};

struct open_external_editor_cb_args {
		int		child_pid;
		char		*path;
		time_t		mtime;
		struct tab	*tab;
		int		(*callback)(const char *,gpointer);
		gpointer	cb_data;
};

int
open_external_editor_cb(gpointer data)
{
	struct open_external_editor_cb_args	*args;
	struct stat				st;
	struct tab				*t;

	int					fd = -1;
	int					rv, nb;
	int					status;
	int					found_tab = 0;
	GString					*contents = NULL;
	char					buf[XT_EE_BUFSZ];

	args = (struct open_external_editor_cb_args*)data;

	/* Check if tab is still open */
	TAILQ_FOREACH(t, &tabs, entry)
		if (t == args->tab) {
			found_tab = 1;
			break;
		}

	/* Tab was deleted */
	if (!found_tab){
		g_free(args->path);
		g_free(args->cb_data);
		g_free(args);
		return (0);
	}

	rv = stat(args->path, &st);
	if (rv == -1 && errno != ENOENT) {
		DPRINTF("open_external_editor_cb: stat error, file %s, "
		    "error: %s\n", args->path, strerror(errno));
		show_oops(args->tab, "stat error : %s", strerror(errno));
		goto done;
	} else if ( rv == 0 && st.st_mtime > args->mtime) {
		DPRINTF("File %s has been modified\n", args->path);
		args->mtime = st.st_mtime;

		contents = g_string_sized_new(XT_EE_BUFSZ);
		fd = open(args->path, O_RDONLY);
		if (fd == -1) {
			DPRINTF("open_external_editor_cb, open error, %s\n",
			    strerror(errno));
			goto done;
		}

		nb = 0;
		for (;;) {
			nb = read(fd, buf, XT_EE_BUFSZ);
			if (nb < 0) {
				g_string_free(contents, TRUE);
				show_oops(args->tab, strerror(errno));
				goto done;
			}

			buf[nb] = '\0';
			contents = g_string_append(contents, buf);

			if (nb < XT_EE_BUFSZ)
				break;
		}
		close(fd);
		fd = -1;

		DPRINTF("external_editor_cb: contents updated\n");
		if (args->callback)
			args->callback(contents->str, args->cb_data);

		g_string_free(contents, TRUE);
	}

	/* Check if child has terminated */
	rv = waitpid(args->child_pid, &status, WNOHANG);
	if (rv == -1 /* || WIFEXITED(status) */) {

		/* Delete the file */
		unlink(args->path);
		goto done;
	}

	return (1);
done:
	g_free(args->path);
	g_free(args->cb_data);
	g_free(args);

	if (fd != -1)
		close(fd);

	return (0);
}

int
open_external_editor(struct tab *t, const char *contents, const char *suffix,
    int (*callback)(const char *, gpointer), gpointer cb_data)
{
	struct open_external_editor_cb_args	*a;
	char					command[PATH_MAX];
	char					*filename;
	char					*ptr;
	int					fd;
	int					nb, rv;
	int					cnt;
	struct stat				st;
	pid_t					pid;

	if (external_editor == NULL)
		return (0);

	if (suffix == NULL)
		suffix = "";

	filename = g_malloc(strlen(temp_dir) + strlen("/xxxtermXXXXXX") +
	    strlen(suffix) + 1);
	sprintf(filename, "%s/xxxtermXXXXXX%s", temp_dir, suffix);

	/* Create a temporary file */
	fd = mkstemps(filename, strlen(suffix));
	if (fd == -1) {
		show_oops(t, "Cannot create temporary file");
		return (1);
	}

	nb = 0;
	while (nb < strlen(contents)) {
		if (strlen(contents) - nb > XT_EE_BUFSZ)
			cnt = XT_EE_BUFSZ;
		else
			cnt = strlen(contents) - nb;

		rv = write(fd, contents+nb, cnt);
		if (rv < 0) {
			g_free(filename);
			close(fd);
			show_oops(t,strerror(errno));
			return (1);
		}

		nb += rv;
	}

	rv = fstat(fd, &st);
	if (rv == -1){
		g_free(filename);
		close(fd);
		show_oops(t,"Cannot stat file: %s\n", strerror(errno));
		return (1);
	}
	close(fd);

	DPRINTF("edit_src: external_editor: %s\n", external_editor);

	nb = 0;
	for (ptr = external_editor; nb < sizeof(command) - 1 && *ptr; ptr++) {

		if (*ptr == '<') {
			if (strncasecmp(ptr, "<file>", 6) == 0) {
				strlcpy(command+nb, filename,
				    sizeof(command) - nb);
				ptr += 5;
				nb += strlen(filename);
			}
		} else
			command[nb++] = *ptr;
	}
	command[nb] = '\0';

	DPRINTF("edit_src: Launching program %s\n", command);

	/* Launch editor */
	pid = fork();
	switch (pid) {
	case -1:
		show_oops(t, "can't fork to execute editor");
		return (1);
	case 0:
		break;
	default:

		a = g_malloc(sizeof(struct open_external_editor_cb_args));
		a->child_pid = pid;
		a->path = filename;
		a->tab = t;
		a->mtime = st.st_mtime;
		a->callback = callback;
		a->cb_data = cb_data;

		/* Check every 100 ms if file has changed */
		g_timeout_add(100, (GSourceFunc)open_external_editor_cb,
		    (gpointer)a);
		return (0);
	}

	/* child */
	rv = system(command);

	_exit(0);
	/* NOTREACHED */
	return (0);
}

int
edit_src_cb(const char *contents, gpointer data)
{
	struct edit_src_cb_args *args;

	args = (struct edit_src_cb_args *)data;

	webkit_web_frame_load_string(args->frame, contents, NULL,
	    webkit_web_data_source_get_encoding(args->data_src),
	    webkit_web_frame_get_uri(args->frame));
	return (0);
}

int
edit_src(struct tab *t, struct karg *args)
{
	WebKitWebFrame		*frame;
	WebKitWebDataSource	*ds;
	GString			*contents;
	struct edit_src_cb_args	*ext_args;

	if (external_editor == NULL){
		show_oops(t,"Setting external_editor not set");
		return (1);
	}

	frame = webkit_web_view_get_focused_frame(t->wv);
	ds = webkit_web_frame_get_data_source(frame);
	if (webkit_web_data_source_is_loading(ds)) {
		show_oops(t,"Webpage is still loading.");
		return (1);
	}

	contents = webkit_web_data_source_get_data(ds);
	if (!contents)
		show_oops(t,"No contents - opening empty file");

	ext_args = g_malloc(sizeof(struct edit_src_cb_args));
	ext_args->frame = frame;
	ext_args->data_src = ds;

	/* Check every 100 ms if file has changed */
	open_external_editor(t, contents->str, ".html", &edit_src_cb, ext_args);
	return (0);
}

struct edit_element_cb_args {
	WebKitDOMElement	*active;
	struct tab		*tab;
};

int
edit_element_cb(const char *contents, gpointer data)
{
	struct				edit_element_cb_args *args;
	WebKitDOMHTMLTextAreaElement	*ta;
	WebKitDOMHTMLInputElement	*el;

	args = (struct edit_element_cb_args*)data;

	if (!args || !args->active)
		return (0);

	el = (WebKitDOMHTMLInputElement*)args->active;
	ta = (WebKitDOMHTMLTextAreaElement*)args->active;

	if (WEBKIT_DOM_IS_HTML_INPUT_ELEMENT(el))
		webkit_dom_html_input_element_set_value(el, contents);
	else if (WEBKIT_DOM_IS_HTML_TEXT_AREA_ELEMENT(ta))
		webkit_dom_html_text_area_element_set_value(ta, contents);

	return (0);
}

int
edit_element(struct tab *t, struct karg *a)
{
	WebKitDOMHTMLDocument		*doc;
	WebKitDOMElement		*active_element;
	WebKitDOMHTMLTextAreaElement	*ta;
	WebKitDOMHTMLInputElement	*el;
	char				*contents;
	struct edit_element_cb_args	*args;

	if (external_editor == NULL){
		show_oops(t,"Setting external_editor not set");
		return (0);
	}

	doc = (WebKitDOMHTMLDocument*)webkit_web_view_get_dom_document(t->wv);
	active_element = webkit_dom_html_document_get_active_element(doc);
	el = (WebKitDOMHTMLInputElement*)active_element;
	ta = (WebKitDOMHTMLTextAreaElement*)active_element;

	if (doc == NULL || active_element == NULL ||
	    (WEBKIT_DOM_IS_HTML_INPUT_ELEMENT(el) == 0 &&
	    WEBKIT_DOM_IS_HTML_TEXT_AREA_ELEMENT(ta) == 0)) {
		show_oops(t, "No active text element!");
		return (1);
	}

	contents = "";
	if (WEBKIT_DOM_IS_HTML_INPUT_ELEMENT(el))
		contents = webkit_dom_html_input_element_get_value(el);
	else if (WEBKIT_DOM_IS_HTML_TEXT_AREA_ELEMENT(ta))
		contents = webkit_dom_html_text_area_element_get_value(ta);

	if ((args = g_malloc(sizeof(struct edit_element_cb_args))) == NULL)
		return (1);

	args->tab = t;
	args->active = active_element;

	open_external_editor(t, contents, NULL, &edit_element_cb,  args);

	return (0);
}

#else /* Just to make things compile. */

int
edit_element(struct tab *t, struct karg *a)
{
	show_oops(t, "external editor feature requires webkit >= 1.5.0");
	return (1);
}

int
edit_src(struct tab *t, struct karg *args)
{
	show_oops(t, "external editor feature requires webkit >= 1.5.0");
	return (1);
}

#endif
