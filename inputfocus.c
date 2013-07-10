/*
 * Copyright (c) 2011 Marco Peereboom <marco@peereboom.us>
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

#if WEBKIT_CHECK_VERSION(1, 5, 0)
	/* we got the DOM API we need */

void
focus_body(WebKitDOMDocument *doc)
{
	WebKitDOMNodeList       *body = NULL;
	WebKitDOMNode           *n;
	int                     i;

	body = webkit_dom_document_get_elements_by_tag_name(doc, "body");
	for (i = 0; i < webkit_dom_node_list_get_length(body); ++i) {
		n = webkit_dom_node_list_item(body, i);
		webkit_dom_element_focus((WebKitDOMElement *)n);
#if WEBKIT_CHECK_VERSION(1, 8, 0)
		webkit_dom_html_element_click((WebKitDOMHTMLElement *)n);
#endif
		break;
	}
}

int
node_is_valid_entry(WebKitDOMNode *n)
{
	if (n == NULL)
		return (FALSE);

	if (WEBKIT_DOM_IS_HTML_INPUT_ELEMENT(n) &&
	    webkit_dom_html_input_element_check_validity(
	    (WebKitDOMHTMLInputElement *)n))
		return (TRUE);
	if (WEBKIT_DOM_IS_HTML_TEXT_AREA_ELEMENT(n) &&
	    webkit_dom_html_text_area_element_check_validity(
	    (WebKitDOMHTMLTextAreaElement *)n))
		return (TRUE);

	return (FALSE);
}

int
focus_input_document(struct tab *t, WebKitDOMDocument *doc)
{
	WebKitDOMNodeList	*input = NULL, *textarea = NULL;
	WebKitDOMNode		*n;
	char			*es;
	int			i, rv = 0 /* not found */;

	WebKitDOMHTMLTextAreaElement	*ta;
	WebKitDOMHTMLInputElement	*in;

	/* we are deliberately ignoring tab index! */

	/* try input first */
	input = webkit_dom_document_get_elements_by_tag_name(doc, "input");
	for (i = 0; i < webkit_dom_node_list_get_length(input); i++) {
		n = webkit_dom_node_list_item(input, i);
		in = (WebKitDOMHTMLInputElement*)n;
		g_object_get(G_OBJECT(in), "type", &es, (char *)NULL);
		if ((g_strcmp0("text", es) && g_strcmp0("password",es)) ||
		    webkit_dom_html_input_element_get_disabled(in)) {
			/* skip not text */
			g_free(es);
			continue;
		}
		webkit_dom_element_focus((WebKitDOMElement*)in);
#if WEBKIT_CHECK_VERSION(1, 8, 0)
		webkit_dom_html_element_click((WebKitDOMHTMLElement*)in);
#endif
		g_free(es);
		rv = 1; /* found */
		goto done;
	}

	/* now try textarea */
	textarea = webkit_dom_document_get_elements_by_tag_name(doc, "textarea");
	for (i = 0; i < webkit_dom_node_list_get_length(textarea); i++) {
		n = webkit_dom_node_list_item(textarea, i);
		ta = (WebKitDOMHTMLTextAreaElement*)n;
		if (webkit_dom_html_text_area_element_get_disabled(ta)) {
			/* it is hidden so skip */
			continue;
		}
		webkit_dom_element_focus((WebKitDOMElement*)ta);
#if WEBKIT_CHECK_VERSION(1, 8, 0)
		webkit_dom_html_element_click((WebKitDOMHTMLElement*)ta);
#endif
		rv = 1; /* found */
		goto done;
	}
done:
	if (input)
		g_object_unref(input);
	if (textarea)
		g_object_unref(textarea);

	return (rv);
}

char *
get_element_text(WebKitDOMNode *n)
{
	if (WEBKIT_DOM_IS_HTML_INPUT_ELEMENT(n))
		return (g_strdup(webkit_dom_html_input_element_get_value(
		    (WebKitDOMHTMLInputElement *)n)));
	else if (WEBKIT_DOM_IS_HTML_TEXT_AREA_ELEMENT(n))
		return (g_strdup(webkit_dom_html_text_area_element_get_value(
		    (WebKitDOMHTMLTextAreaElement *)n)));
	return (NULL);
}

int
focus_input(struct tab *t)
{
	WebKitDOMDocument	*doc;
	WebKitDOMNode		*n;
	WebKitDOMNodeList	*fl = NULL, *ifl = NULL;
	WebKitDOMElement	*a;
	int			i, fl_count, ifl_count, rv = 0; /* not found */

	WebKitDOMHTMLFrameElement	*frame;
	WebKitDOMHTMLIFrameElement	*iframe;

	/*
	 * Here is what we are doing:
	 *
	 * If a textbox is already focused, leave it alone.
	 *
	 * Try the tab's previous active entry, for example if it was set by
	 * some javascript when the page loaded.
	 *
	 * See if we got frames or iframes
	 *
	 * if we do focus on input or textarea in frame or in iframe
	 *
	 * if we find nothing or there are no frames focus on first input or
	 * text area
	 */

	doc = webkit_web_view_get_dom_document(t->wv);

	/* try current active element */
	a = webkit_dom_html_document_get_active_element(
	    WEBKIT_DOM_HTML_DOCUMENT(doc);
	if (node_is_valid_entry((WebKitDOMNode *)a)) {
		rv = 1; /* found */
		goto done;
	}

	/* try previous active element */
	if (node_is_valid_entry((WebKitDOMNode *)t->active)) {
		webkit_dom_element_focus((WebKitDOMElement*)t->active);
#if WEBKIT_CHECK_VERSION(1, 8, 0)
		webkit_dom_html_element_click((WebKitDOMHTMLElement*)t->active);
#endif
		rv = 1; /* found */
		goto done;
	} else {
		if (t->active)
			g_object_unref(t->active);
		t->active = NULL;
		if (t->active_text) {
			g_free(t->active_text);
			t->active_text = NULL;
		}
	}

	/* get frames */
	fl = webkit_dom_document_get_elements_by_tag_name(doc, "frame");
	fl_count = webkit_dom_node_list_get_length(fl);

	/* get iframes */
	ifl = webkit_dom_document_get_elements_by_tag_name(doc, "iframe");
	ifl_count = webkit_dom_node_list_get_length(ifl);

	/* walk frames and look for a text input */
	for (i = 0; i < fl_count; i++) {
		n = webkit_dom_node_list_item(fl, i);
		frame = (WebKitDOMHTMLFrameElement*)n;
		doc = webkit_dom_html_frame_element_get_content_document(frame);

		if (focus_input_document(t, doc)) {
			rv = 1; /* focus */
			goto done;
		}
	}

	/* walk iframes and look for a text input */
	for (i = 0; i < ifl_count; i++) {
		n = webkit_dom_node_list_item(ifl, i);
		iframe = (WebKitDOMHTMLIFrameElement*)n;
		doc = webkit_dom_html_iframe_element_get_content_document(iframe);

		if (focus_input_document(t, doc)) {
			rv = 1; /* found */
			goto done;
		}
	}

	/* if we made it here nothing got focused so use normal heuristic */
	if (focus_input_document(t, webkit_web_view_get_dom_document(t->wv)))
		rv = 1; /* found */

done:
	if (fl)
		g_object_unref(fl);
	if (ifl)
		g_object_unref(ifl);

	return (rv);
}

int
dom_is_input(struct tab *t, char **text)
{
	WebKitDOMDocument	*doc;
	WebKitDOMElement	*a;
	WebKitDOMHTMLElement	*aa;
	WebKitDOMHTMLObjectElement *object;

	WebKitDOMHTMLFrameElement *frame;
	WebKitDOMHTMLIFrameElement *iframe;

	/* proof positive that OO is stupid */

	doc = webkit_web_view_get_dom_document(t->wv);

	/* unwind frames and iframes until the cows come home */
	for (;;) {
		if (!WEBKIT_DOM_IS_HTML_DOCUMENT(doc))
			return (0);
		a = webkit_dom_html_document_get_active_element(
		    (WebKitDOMHTMLDocument*)doc);
		if (a == NULL)
			return (0);

		frame = (WebKitDOMHTMLFrameElement *)a;
		if (WEBKIT_DOM_IS_HTML_FRAME_ELEMENT(frame)) {
			doc = webkit_dom_html_frame_element_get_content_document(
			    frame);
			continue;
		}

		iframe = (WebKitDOMHTMLIFrameElement *)a;
		if (WEBKIT_DOM_IS_HTML_IFRAME_ELEMENT(iframe)) {
			doc = webkit_dom_html_iframe_element_get_content_document(
			    iframe);
			continue;
		}

		object = (WebKitDOMHTMLObjectElement *)a;
		if (WEBKIT_DOM_IS_HTML_OBJECT_ELEMENT(object)) {
			doc = webkit_dom_html_object_element_get_content_document(
			    object);
			continue;
		}

		/*
		 * I think this is a total hack because this property isn't
		 * set for textareas or input however, it is set for jquery
		 * textareas that do rich text.  Since this works around issues
		 * in RT we'll simply keep it!
		 *
		 * This might break some other stuff but for now it helps.
		 */
		aa = (WebKitDOMHTMLElement*)a;
		if (WEBKIT_DOM_IS_HTML_ELEMENT(aa) &&
		    webkit_dom_html_element_get_is_content_editable(aa)) {
			if (t->active == NULL) {
				t->active = a;
				g_object_ref(t->active);
			}
			*text = get_element_text((WebKitDOMNode *)a);
			if (t->active_text == NULL)
				t->active_text = g_strdup(*text);
			return (1);
		}
		break;
	}

	if (a == NULL)
		return (0);

	if (node_is_valid_entry((WebKitDOMNode *)a)) {
		if (!node_is_valid_entry((WebKitDOMNode *)t->active)) {
			if (t->active)
				g_object_unref(t->active);
			t->active = NULL;
			if (t->active_text) {
				g_free(t->active_text);
				t->active_text = NULL;
			}
		}
		if (t->active == NULL) {
			t->active = a;
			g_object_ref(t->active);
		}
		*text = get_element_text((WebKitDOMNode *)a);
		if (t->active_text == NULL)
			t->active_text = g_strdup(*text);
		return (1);
	}

	return (0);
}

void *
input_check_mode(struct tab *t)
{
	char			*text = NULL;

	if (dom_is_input(t, &text)) {
		t->mode = XT_MODE_INSERT;
		return (t->active);
	} else
		return (NULL);
}

int
command_mode(struct tab *t, struct karg *args)
{
	WebKitDOMDocument	*doc;
	WebKitDOMElement	*a;

	if (args->i == XT_MODE_COMMAND) {
		doc = webkit_web_view_get_dom_document(t->wv);
		a = webkit_dom_html_document_get_active_element(
		    (WebKitDOMHTMLDocument *)doc);
		if (a) {
			webkit_dom_element_blur(a);
			focus_body(doc);
		}
		t->mode = XT_MODE_COMMAND;
	} else if (args->i == XT_MODE_INSERT && focus_input(t))
		t->mode = XT_MODE_INSERT;
	else if (args->i == XT_MODE_HINT || args->i == XT_MODE_PASSTHROUGH)
		t->mode = args->i;

	if (!node_is_valid_entry((WebKitDOMNode *)t->active)) {
		if (t->active)
			g_object_unref(t->active);
		t->active = NULL;
		if (t->active_text) {
			g_free(t->active_text);
			t->active_text = NULL;
		}
	}

	return (XT_CB_HANDLED);
}

void
input_autofocus(struct tab *t)
{
	struct karg		args = {0};
	char			*text = NULL;

	if (autofocus_onload &&
	    t->tab_id == gtk_notebook_get_current_page(notebook)) {
		if (focus_input(t))
			t->mode = XT_MODE_INSERT;
		else
			t->mode = XT_MODE_COMMAND;
	} else {
		if (dom_is_input(t, &text)) {
			if (text != NULL && g_strcmp0(text, t->active_text))
				args.i = XT_MODE_INSERT;
			else
				args.i = XT_MODE_COMMAND;
		} else
			args.i = XT_MODE_COMMAND;
		command_mode(t, &args);
	}

	if (text)
		g_free(text);
}
#else /* WEBKIT_CHECK_VERSION */
	/* incomplete DOM API */

	/*
	 * XXX
	 * note that we can't check the return value of run_script so we
	 * have to assume that the command worked; this may leave you in
	 * insertmode when in fact you shouldn't be
	*/
void
input_autofocus(struct tab *t)
{
	if (autofocus_onload &&
	    t->tab_id == gtk_notebook_get_current_page(notebook)) {
		run_script(t, "hints.focusInput();");
		t->mode = XT_MODE_INSERT;
	} else {
		run_script(t, "hints.clearFocus();");
		t->mode = XT_MODE_COMMAND;
	}
}

void *
input_check_mode(struct tab *t)
{
	return (NULL);
}

int
command_mode(struct tab *t, struct karg *args)
{
	if (args->i == XT_MODE_COMMAND) {
		run_script(t, "hints.clearFocus();");
		t->mode = XT_MODE_COMMAND;
	} else {
		run_script(t, "hints.focusInput();");
		t->mode = XT_MODE_INSERT;
	}

	return (XT_CB_HANDLED);
}
#endif
