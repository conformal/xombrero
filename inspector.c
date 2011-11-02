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

gboolean
inspector_attach_window(WebKitWebInspector *inspector, struct tab *t)
{
	DNPRINTF(XT_D_INSPECTOR, "%s: tab %d\n", __func__, t->tab_id);

	return (FALSE); /* NOT handled */
}

gboolean
inspector_close_window(WebKitWebInspector *inspector, struct tab *t)
{
	DNPRINTF(XT_D_INSPECTOR, "%s: tab %d\n", __func__, t->tab_id);

	if (t->inspector_window) {
		gtk_widget_hide(t->inspector_window);
		return (TRUE); /* handled */
	}

	return (FALSE); /* NOT handled */
}

gboolean
inspector_detach_window(WebKitWebInspector *inspector, struct tab *t)
{
	DNPRINTF(XT_D_INSPECTOR, "%s: tab %d\n", __func__, t->tab_id);

	return (FALSE); /* NOT handled */
}

gboolean
inspector_delete(GtkWidget *inspector_window, GdkEvent *e, struct tab *t)
{
	inspector_close_window(t->inspector, t);

	return (TRUE); /* handled */
}

WebKitWebView*
inspector_inspect_web_view_cb(WebKitWebInspector *inspector, WebKitWebView* wv,
    struct tab *t)
{
	DNPRINTF(XT_D_INSPECTOR, "%s: tab %d\n", __func__, t->tab_id);

	if (t->inspector_window)
		goto done;

	t->inspector_window = create_window("inspector");
	t->inspector_view = webkit_web_view_new();
	gtk_container_add(GTK_CONTAINER(t->inspector_window), t->inspector_view);

	g_signal_connect(t->inspector_window,
	    "delete-event", G_CALLBACK(inspector_delete), t);
done:
	return WEBKIT_WEB_VIEW(t->inspector_view);
}

gboolean
inspector_show_window(WebKitWebInspector *inspector, struct tab *t)
{
	DNPRINTF(XT_D_INSPECTOR, "%s: tab %d\n", __func__, t->tab_id);

	if (t->inspector_window) {
		g_signal_emit_by_name(inspector, "attach-window", t);
		gtk_widget_show_all(t->inspector_window);
		gtk_window_present(GTK_WINDOW(t->inspector_window));
		return (TRUE); /* handled */
	}

	return (FALSE); /* NOT handled */
}

void
inspector_finished(WebKitWebInspector *inspector, struct tab *t)
{
	DNPRINTF(XT_D_INSPECTOR, "%s: tab %d\n", __func__, t->tab_id);

	if (t->inspector_window) {
		g_signal_handlers_disconnect_by_func(
		    t->inspector, inspector_attach_window, t);
		g_signal_handlers_disconnect_by_func(
		    t->inspector, inspector_close_window, t);
		g_signal_handlers_disconnect_by_func(
		    t->inspector, inspector_detach_window, t);
		g_signal_handlers_disconnect_by_func(
		    t->inspector, inspector_finished, t);
		g_signal_handlers_disconnect_by_func(
		    t->inspector, inspector_inspect_web_view_cb, t);
		g_signal_handlers_disconnect_by_func(
		    t->inspector, inspector_show_window, t);

		gtk_widget_hide(t->inspector_window);

		/* XXX it seems that the window is disposed automatically */
		t->inspector_window = NULL;
		t->inspector_view = NULL;
	}
}

void
setup_inspector(struct tab *t)
{

	DNPRINTF(XT_D_INSPECTOR, "%s: tab %d\n", __func__, t->tab_id);

	t->inspector = webkit_web_view_get_inspector(WEBKIT_WEB_VIEW(t->wv));
	g_object_connect(G_OBJECT(t->inspector),
	    "signal::attach-window", G_CALLBACK(inspector_attach_window), t,
	    "signal::close-window", G_CALLBACK(inspector_close_window), t,
	    "signal::detach-window", G_CALLBACK(inspector_detach_window), t,
	    "signal::finished", G_CALLBACK(inspector_finished), t,
	    "signal::inspect-web-view", G_CALLBACK(inspector_inspect_web_view_cb), t,
	    "signal::show-window", G_CALLBACK(inspector_show_window), t,
	    (char *)NULL);

}

int
inspector_cmd(struct tab *t, struct karg *args)
{
	DNPRINTF(XT_D_INSPECTOR, "%s: tab %d\n", __func__, t->tab_id);

	if (t == NULL)
		return (1);

	if (args->i & XT_INS_SHOW)
		webkit_web_inspector_show(t->inspector);
	else if (args->i & XT_INS_HIDE)
		webkit_web_inspector_close(t->inspector);
	else if (args->i & XT_INS_CLOSE)
		inspector_finished(t->inspector, t);

	return (XT_CB_PASSTHROUGH);
}
