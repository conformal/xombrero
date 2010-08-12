/*
    (c) 2009 by Leon Winter
    (c) 2009 by Hannes Schueller
    see LICENSE file
*/

function vimprobable_clearfocus() {
    if(document.activeElement && document.activeElement.blur)
        document.activeElement.blur();
}

function vimprobable_show_hints(inputText) {
    if (document.getElementsByTagName("body")[0] !== null && typeof(document.getElementsByTagName("body")[0]) == "object") {
        var height = window.innerHeight;
        var width = window.innerWidth;
        var scrollX = document.defaultView.scrollX;
        var scrollY = document.defaultView.scrollY;
        /* prefixing html: will result in namespace error */
        var hinttags;
        if (typeof(inputText) == "undefined" || inputText == "") {
            hinttags = "//*[@onclick or @onmouseover or @onmousedown or @onmouseup or @oncommand or @class='lk' or @role='link' or @href] | //input[not(@type='hidden')] | //a | //area | //iframe | //textarea | //button | //select";
        } else {
            /* only elements which match the text entered so far */
            hinttags = "//*[(@onclick or @onmouseover or @onmousedown or @onmouseup or @oncommand or @class='lk' or @role='link' or @href) and contains(., '" + inputText + "')] | //input[not(@type='hidden') and contains(., '" + inputText + "')] | //a[contains(., '" + inputText + "')] | //area[contains(., '" + inputText + "')] | //iframe[contains(@name, '" + inputText + "')] | //textarea[contains(., '" + inputText + "')] | //button[contains(@value, '" + inputText + "')] | //select[contains(., '" + inputText + "')]";
        }

        /* iterator type isn't suitable here, because: "DOMException NVALID_STATE_ERR: The document has been mutated since the result was returned." */
        var r = document.evaluate(hinttags, document,
            function(p) {
                return 'http://www.w3.org/1999/xhtml';
            }, XPathResult.ORDERED_NODE_SNAPSHOT_TYPE, null);
        div = document.createElement("div");
        /* due to the different XPath result type, we will need two counter variables */
        vimprobable_j = 0;
        var i;
        vimprobable_a = [];
        vimprobable_colors = [];
        vimprobable_backgrounds = [];
        for (i = 0; i < r.snapshotLength; i++)
        {
            var elem = r.snapshotItem(i);
            rect = elem.getBoundingClientRect();
            if (!rect || rect.top > height || rect.bottom < 0 || rect.left > width || rect.right < 0 || !(elem.getClientRects()[0]))
                continue;
            var computedStyle = document.defaultView.getComputedStyle(elem, null);
            if (computedStyle.getPropertyValue("visibility") != "visible" || computedStyle.getPropertyValue("display") == "none")
                continue;
            var leftpos = Math.max((rect.left + scrollX), scrollX);
            var toppos = Math.max((rect.top + scrollY), scrollY);
            vimprobable_a.push(elem);
            /* making this block DOM compliant */
            var hint = document.createElement("span");
            hint.setAttribute("class", "hinting_mode_hint");
            hint.setAttribute("id", "vimprobablehint" + vimprobable_j);
            hint.style.position = "absolute";
            hint.style.left = leftpos + "px";
            hint.style.top =  toppos + "px";
            hint.style.background = "red";
            hint.style.color = "#fff";
            hint.style.font = "bold 10px monospace";
            hint.style.zIndex = "99";
            var text = document.createTextNode(vimprobable_j + 1);
            hint.appendChild(text);
            div.appendChild(hint);
            /* remember site-defined colour of this element */
            vimprobable_colors[vimprobable_j] = elem.style.color;
            vimprobable_backgrounds[vimprobable_j] = elem.style.background;
            /* make the link black to ensure it's readable */
            elem.style.color = "#000";
            elem.style.background = "#ff0";
            vimprobable_j++;
        }
        i = 0;
        while (typeof(vimprobable_a[i]) != "undefined") {
            vimprobable_a[i].className += " hinting_mode_hint";
            i++;
        }
        document.getElementsByTagName("body")[0].appendChild(div);
        vimprobable_clearfocus();
        vimprobable_h = null;
        if (i == 1) {
            /* just one hinted element - might as well follow it */
            return vimprobable_fire(1);
        }
	return "found;" + i;
    }
}
function vimprobable_fire(n)
{
    if (typeof(vimprobable_a[n - 1]) != "undefined") {
        el = vimprobable_a[n - 1];
        tag = el.nodeName.toLowerCase();
        vimprobable_clear();
        if(tag == "iframe" || tag == "frame" || tag == "textarea" || tag == "input" && (el.type == "text" || el.type == "password" || el.type == "checkbox" || el.type == "radio") || tag == "select") {
            el.focus();
            if (tag == "textarea" || tag == "input")
                console.log('insertmode_on');
        } else {
            if (el.onclick) {
                var evObj = document.createEvent('MouseEvents');
                evObj.initMouseEvent('click', true, true, window, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, null);
                el.dispatchEvent(evObj);
            } else if (el.href) {
                if (el.href.match(/^javascript:/)) {
                    var evObj = document.createEvent('MouseEvents');
                    evObj.initMouseEvent('click', true, true, window, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, null);
                    el.dispatchEvent(evObj);
                } else {
                    /* send signal to open link */
                    return "open;" + el.href;
                }
            } else {
                var evObj = document.createEvent('MouseEvents');
                evObj.initMouseEvent('click', true, true, window, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, null);
                el.dispatchEvent(evObj);
            }
        }
    }
}
function vimprobable_cleanup()
{
    for(e in vimprobable_a) {
        if (typeof(vimprobable_a[e].className) != "undefined") {
            vimprobable_a[e].className = vimprobable_a[e].className.replace(/hinting_mode_hint/,'');
            /* reset to site-defined colour */
            vimprobable_a[e].style.color = vimprobable_colors[e];
            vimprobable_a[e].style.background = vimprobable_backgrounds[e];
        }
    }
    div.parentNode.removeChild(div);
    window.onkeyup = null;
}
function vimprobable_clear()
{
    vimprobable_cleanup();
    console.log("hintmode_off")
}

function vimprobable_update_hints(n)
{
    if(vimprobable_h != null) {
        vimprobable_h.className = vimprobable_h.className.replace("_focus","");
        vimprobable_h.style.background = "#ff0";
    }
    if (vimprobable_j - 1 < n * 10 && typeof(vimprobable_a[n - 1]) != "undefined") {
        /* return signal to follow the link */
        return "fire;" + n;
    } else {
        if (typeof(vimprobable_a[n - 1]) != "undefined") {
            (vimprobable_h = vimprobable_a[n - 1]).className = vimprobable_a[n - 1].className.replace("hinting_mode_hint", "hinting_mode_hint_focus");
            vimprobable_h.style.background = "#8f0";
        }
    }
}

function vimprobable_focus_input()
{
    if (document.getElementsByTagName("body")[0] !== null && typeof(document.getElementsByTagName("body")[0]) == "object") {
        /* prefixing html: will result in namespace error */
        var hinttags = "//input[@type='text'] | //input[@type='password'] | //textarea";
        var r = document.evaluate(hinttags, document,
            function(p) {
                return 'http://www.w3.org/1999/xhtml';
            }, XPathResult.ORDERED_NODE_SNAPSHOT_TYPE, null);
        var i;
        var j = 0;
        var first = null;
        for (i = 0; i < r.snapshotLength; i++) {
            var elem = r.snapshotItem(i);
            if (i == 0) {
                first = elem;
            }
            if (j == 1) {
                elem.focus();
                var tag = elem.nodeName.toLowerCase();
                if (tag == "textarea" || tag == "input")
                    console.log('insertmode_on');
                break;
            } else {
                if (elem == document.activeElement)
                    j = 1;
            }
        }
        if (j == 0) {
            /* no appropriate field found focused - focus the first one */
            if (first !== null) {
                first.focus();
                var tag = elem.nodeName.toLowerCase();
                if (tag == "textarea" || tag == "input")
                    console.log('insertmode_on');
            }
        }
    }
}
