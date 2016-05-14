/*
Copyright (c) 2009 Leon Winter
Copyright (c) 2009-2011 Hannes Schueller
Copyright (c) 2009-2010 Matto Fransen
Copyright (c) 2010-2011 Hans-Peter Deifel
Copyright (c) 2010-2011 Thomas Adam
Copyright (c) 2011 Albert Kim
Copyright (c) 2011 Daniel Carl

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/
function Hints() {
    var config = {
        maxAllowedHints: 500,
        hintCss: "z-index:100000;font-family:monospace;font-size:%%%HINTING_FONT_SIZE_PLACEHOLDER%%%px;"
               + "font-weight:bold;color:white;background-color:red;"
               + "padding:0px 1px;position:absolute;",
        hintClass: "hinting_mode_hint",
        hintClassFocus: "hinting_mode_hint_focus",
        elemBackground: "#ff0",
        elemBackgroundFocus: "#8f0",
        elemColor: "#000"
    };

    var hintContainer;
    var currentFocusNum = 1;
    var hints = [];
    var mode;

    this.createHints = function(inputText, hintMode)
    {
        if (hintMode) {
            mode = hintMode;
        }

        var topwin = window;
        var top_height = topwin.innerHeight;
        var top_width = topwin.innerWidth;
        var xpath_expr;

        var hintCount = 0;
        this.clearHints();

        function helper (win, offsetX, offsetY) {
            var doc = win.document;

            var win_height = win.height;
            var win_width = win.width;

            /* Bounds */
            var minX = offsetX < 0 ? -offsetX : 0;
            var minY = offsetY < 0 ? -offsetY : 0;
            var maxX = offsetX + win_width > top_width ? top_width - offsetX : top_width;
            var maxY = offsetY + win_height > top_height ? top_height - offsetY : top_height;

            var scrollX = win.scrollX;
            var scrollY = win.scrollY;

            hintContainer = doc.createElement("div");
            hintContainer.id = "hint_container";

            if (typeof(inputText) == "undefined" || inputText == "") {
                xpath_expr = "//*[@onclick or @onmouseover or @onmousedown or @onmouseup or @oncommand or @class='lk' or @role='link' or @href] | //input[not(@type='hidden')] | //a[href] | //area | //textarea | //button | //select";
            } else {
                xpath_expr = _caseInsensitiveExpr("//*[(@onclick or @onmouseover or @onmousedown or @onmouseup or @oncommand or @class='lk' or @role='link' or @href) and contains(translate(., '$u', '$l'), '$l')] | //input[not(@type='hidden') and contains(translate(., '$u', '$l'), '$l')] | //a[@href and contains(translate(., '$u', '$l'), '$l')] | //area[contains(translate(., '$u', '$l'), '$l')] | //textarea[contains(translate(., '$u', '$l'), '$l')] | //button[contains(translate(@value, '$u', '$l'), '$l')] | //select[contains(translate(., '$u', '$l'), '$l')]", inputText);
            }

            var res = doc.evaluate(xpath_expr, doc,
                function (p) {
                    return "http://www.w3.org/1999/xhtml";
                }, XPathResult.ORDERED_NODE_SNAPSHOT_TYPE, null);

            /* generate basic hint element which will be cloned and updated later */
            var hintSpan = doc.createElement("span");
            hintSpan.setAttribute("class", config.hintClass);
            hintSpan.style.cssText = config.hintCss;

            /* due to the different XPath result type, we will need two counter variables */
            var rect, elem, text, node, show_text;
            for (var i = 0; i < res.snapshotLength; i++)
            {
                if (hintCount >= config.maxAllowedHints)
                    break;

                elem = res.snapshotItem(i);
                rect = elem.getBoundingClientRect();
                if (!rect || rect.left > maxX || rect.right < minX || rect.top > maxY || rect.bottom < minY)
                    continue;

                var style = topwin.getComputedStyle(elem, "");
                if (style.display == "none" || style.visibility != "visible")
                    continue;

                var leftpos = Math.max((rect.left + scrollX), scrollX);
                var toppos = Math.max((rect.top + scrollY), scrollY);

                /* check if we already created a hint for this URL */
                var hintNumber = hintCount;
                if (elem.nodeName.toLowerCase() == "a") {
                        for (var j = 0; j < hints.length; j++) {
                                var h = hints[j];
                                if (h.elem.nodeName.toLowerCase() != "a")
                                        continue;
                                if (h.elem.href.toLowerCase() == elem.href.toLowerCase()){
                                        hintNumber = h.number - 1;
                                        break;
                                }
                        }
                }

                /* making this block DOM compliant */
                var hint = hintSpan.cloneNode(false);
                hint.setAttribute("id", "vimprobablehint" + hintNumber);
                hint.style.left = leftpos + "px";
                hint.style.top =  toppos + "px";
                text = doc.createTextNode(hintNumber + 1);
                hint.appendChild(text);

                hintContainer.appendChild(hint);
                if (hintNumber == hintCount)
                        hintCount++;
                else
                        hintNumber = -2; /* do not follow dupes */

                hints.push({
                    elem:       elem,
                    number:     hintNumber+1,
                    span:       hint,
                    background: elem.style.background,
                    foreground: elem.style.color}
                );

                /* make the link black to ensure it's readable */
                elem.style.color = config.elemColor;
                elem.style.background = config.elemBackground;
            }

            doc.documentElement.appendChild(hintContainer);

            /* recurse into any iframe or frame element */
            var frameTags = ["frame","iframe"];
            for (var f = 0; f < frameTags.length; ++f) {
                var frames = doc.getElementsByTagName(frameTags[f]);
                for (var i = 0, nframes = frames.length; i < nframes; ++i) {
                    elem = frames[i];
                    rect = elem.getBoundingClientRect();
                    if (!elem.contentWindow || !rect || rect.left > maxX || rect.right < minX || rect.top > maxY || rect.bottom < minY)
                        continue;
                    helper(elem.contentWindow, offsetX + rect.left, offsetY + rect.top);
                }
            }
        }

        helper(topwin, 0, 0);

        this.clearFocus();
        this.focusHint(1);
        if (hintCount == 1) {
            /* just one hinted element - might as well follow it */
            return this.fire(1);
        }
    };

    /* set focus on hint with given number */
    this.focusHint = function(n)
    {
        /* reset previous focused hint */
        var hint = _getHintByNumber(currentFocusNum);
        if (hint !== null) {
            hint.elem.className = hint.elem.className.replace(config.hintClassFocus, config.hintClass);
            hint.elem.style.background = config.elemBackground;
        }

        currentFocusNum = n;

        /* mark new hint as focused */
        var hint = _getHintByNumber(currentFocusNum);
        if (hint !== null) {
            hint.elem.className = hint.elem.className.replace(config.hintClass, config.hintClassFocus);
            hint.elem.style.background = config.elemBackgroundFocus;
        }
    };

    /* set focus to next avaiable hint */
    this.focusNextHint = function()
    {
        var index = _getHintIdByNumber(currentFocusNum);

        if (typeof(hints[index + 1]) != "undefined") {
            this.focusHint(hints[index + 1].number);
        } else {
            this.focusHint(hints[0].number);
        }
    };

    /* set focus to previous avaiable hint */
    this.focusPreviousHint = function()
    {
        var index = _getHintIdByNumber(currentFocusNum);
        if (index != 0 && typeof(hints[index - 1].number) != "undefined") {
            this.focusHint(hints[index - 1].number);
        } else {
            this.focusHint(hints[hints.length - 1].number);
        }
    };

    /* filters hints matching given number */
    this.updateHints = function(n)
    {
        if (n == 0) {
            return this.createHints();
        }
        /* remove none matching hints */
        var remove = [];
        for (var i = 0; i < hints.length; ++i) {
            var hint = hints[i];
            if (0 != hint.number.toString().indexOf(n.toString())) {
                remove.push(hint.number);
            }
        }

        for (var i = 0; i < remove.length; ++i) {
            _removeHint(remove[i]);
        }

        if (hints.length === 1) {
            return this.fire(hints[0].number);
        } else {
            return this.focusHint(n);
        }
    };

    this.clearFocus = function()
    {
        if (document.activeElement && document.activeElement.blur) {
            document.activeElement.blur();
        }
    };

    /* remove all hints and set previous style to them */
    this.clearHints = function()
    {
        if (hints.length == 0) {
            return;
        }
        for (var i = 0; i < hints.length; ++i) {
            var hint = hints[i];
            if (typeof(hint.elem) != "undefined") {
                hint.elem.style.background = hint.background;
                hint.elem.style.color = hint.foreground;
                hint.span.parentNode.removeChild(hint.span);
            }
        }
        hints = [];
        hintContainer.parentNode.removeChild(hintContainer);
        window.onkeyup = null;
    };

    /* fires the modeevent on hint with given number */
    this.fire = function(n)
    {
        var doc, result;
        if (!n) {
            var n = currentFocusNum;
        }
        var hint = _getHintByNumber(n);
        if (typeof(hint.elem) == "undefined")
            return "done;";

        var el = hint.elem;
        var tag = el.nodeName.toLowerCase();

        this.clearHints();

        if (tag == "iframe" || tag == "frame" || tag == "textarea" || tag == "input" && (el.type == "text" || el.type == "password" || el.type == "checkbox" || el.type == "radio") || tag == "select") {
            el.focus();
            if (tag == "input" || tag == "textarea") {
                return "insert;"
            }
            return "done;";
        }

        switch (mode)
        {
            case "f": result = _open(el); break;
            case "F": result = _openNewWindow(el); break;
            default:  result = _getElemtSource(el);
        }

        return result;
    };

    this.focusInput = function()
    {
        if (document.getElementsByTagName("body")[0] === null || typeof(document.getElementsByTagName("body")[0]) != "object")
            return;

        /* prefixing html: will result in namespace error */
        var hinttags = "//input[@type='text'] | //input[@type='password'] | //textarea";
        var r = document.evaluate(hinttags, document,
            function(p) {
                return "http://www.w3.org/1999/xhtml";
            }, XPathResult.ORDERED_NODE_SNAPSHOT_TYPE, null);
        var i;
        var j = 0;
        var k = 0;
        var first = null;
        for (i = 0; i < r.snapshotLength; i++) {
            var elem = r.snapshotItem(i);
            if (k == 0) {
                if (elem.style.display != "none" && elem.style.visibility != "hidden") {
                    first = elem;
                } else {
                    k--;
                }
            }
            if (j == 1 && elem.style.display != "none" && elem.style.visibility != "hidden") {
                elem.focus();
                var tag = elem.nodeName.toLowerCase();
                if (tag == "textarea" || tag == "input") {
                    return "insert;";
                }
                break;
            }
            if (elem == document.activeElement) {
                j = 1;
            }
            k++;
        }
        /* no appropriate field found focused - focus the first one */
        if (j == 0 && first !== null) {
            first.focus();
            var tag = elem.nodeName.toLowerCase();
            if (tag == "textarea" || tag == "input") {
                return "insert;";
            }
        }
    };

    /* retrieves text content fro given element */
    function _getTextFromElement(el)
    {
        if (el instanceof HTMLInputElement || el instanceof HTMLTextAreaElement) {
            text = el.value;
        } else if (el instanceof HTMLSelectElement) {
            if (el.selectedIndex >= 0) {
                text = el.item(el.selectedIndex).text;
            } else{
                text = "";
            }
        } else {
            text = el.textContent;
        }
        return text.toLowerCase();;
    }

    /* retrieves the hint for given hint number */
    function _getHintByNumber(n)
    {
        var index = _getHintIdByNumber(n);
        if (index !== null) {
            return hints[index];
        }
        return null;
    }

    /* retrieves the id of hint with given number */
    function _getHintIdByNumber(n)
    {
        for (var i = 0; i < hints.length; ++i) {
            var hint = hints[i];
            if (hint.number === n) {
                return i;
            }
        }
        return null;
    }

    /* removes hint with given number from hints array */
    function _removeHint(n)
    {
        var index = _getHintIdByNumber(n);
        if (index === null) {
            return;
        }
        var hint = hints[index];
        if (hint.number === n) {
            hint.elem.style.background = hint.background;
            hint.elem.style.color = hint.foreground;
            hint.span.parentNode.removeChild(hint.span);

            /* remove hints from all hints */
            hints.splice(index, 1);
        }
    }

    /* opens given element */
    function _open(elem)
    {
        if (elem.target == "_blank") {
            elem.removeAttribute("target");
        }
        _clickElement(elem);
        return "done;";
    }

    /* opens given element into new window */
    function _openNewWindow(elem)
    {
        var oldTarget = elem.target;

        /* set target to open in new window */
        elem.target = "_blank";
        _clickElement(elem);
        elem.target = oldTarget;

        return "done;";
    }

    /* fire moudedown and click event on given element */
    function _clickElement(elem)
    {
        doc = elem.ownerDocument;
        view = elem.contentWindow;

        var evObj = doc.createEvent("MouseEvents");
        evObj.initMouseEvent("mousedown", true, true, view, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, null);
        elem.dispatchEvent(evObj);

        var evObj = doc.createEvent("MouseEvents");
        evObj.initMouseEvent("click", true, true, view, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, null);
        elem.dispatchEvent(evObj);
    }

    /* retrieves the url of given element */
    function _getElemtSource(elem)
    {
        var url = elem.href || elem.src;
        return url;
    }

    /* returns a case-insensitive version of the XPath expression */
    function _caseInsensitiveExpr(xpath, searchString)
    {
        return xpath.split("$u").join(searchString.toUpperCase())
                    .split("$l").join(searchString.toLowerCase());
    }
}
hints = new Hints();
