/*
 * CSS from NightShift - Eye Care by vetinari
 * http://userstyles.org/styles/18192
 */

function inverse() {
	var css = "
	    body, html { min-height: 100%!important; }
	    html, body { background-color:#111!important }
	    body>*:not(:empty) { background-color:#222!important }
	    body>*>*:not(:empty) { background-color:#222!important }
	    body>*>*>*:not(:empty) { background-color:#282828!important }
	    body>*>*>*>*:not(:empty) { background-color:#282828!important }
	    body>*>*>*>*>*:not(:empty) {background-color:#383838!important }
	    body>*>*>*>*>* * { background-color:#383838!important }
	    body table[border=\"0\"] td { background-color:#111!important }
	    body table table[border=\"0\"] td {
	    	background-color:#333!important
	    }
	    body table table table[border=\"0\"] td {
	    	background-color:#222!important
	    }
	    body table table table table[border=\"0\"] td {
	    	background-color:#444!important
	    }
	    body *:empty { background-color: #252525!important }

	    body p:not(:empty),
	    body p *,
	    body h1,
	    body h1 *,
	    body h2,
	    body h2 *,
	    body h3,
	    body h3 *,
	    body h4,
	    body h4 *,
	    body h5,
	    body h5 *,
	    body strong>*,
	    body b>*,
	    body em>*,
	    body i>*,
	    body span>*:not(img){ background:transparent none!important }

	    body h1,
	    body h1 *,
	    body h2,
	    body h2 *,
	    p>strong:only-of-type,
	    p>b:only-of-type{ color: #a98!important }

	    body h3,
	    body h3 *,
	    body h4,
	    body h4 *{ color: #aaa!important }
	    *:not([onclick]):not(input):not(a):not(img):not([class^=\"UI\"]),
	    body a:not(:empty),
	    div:not([onclick]) {
	    	background-image:none!important;
	        text-indent:0!important
	    }
	    *[onclick] { color:#79a!important }
	    *[onclick]:hover { color:#99a8aa!important }
	    body hr { 
	    	background: #666 none!important;
	    	color: #666!important;
	        border:1px solid #666!important;
	    	height: 1px!important;
	        overflow:hidden!important;
	    	display: block!important }
	    * { color: #c0c0c0!important; border-color:#666!important; }
	    * body a, body a *{ color: #B6AA7B!important; }
	    body a:hover, body a:hover * {
	    	color: #D9C077!important;
	        text-decoration: underline!important
	    }
	    body img,a[href] img,
	    a[href] button,
	    input[type=\"image\"],*[onclick]:empty,
	    body a:empty{ opacity:.5!important }
	    body img:hover,
	    a[href]:hover img,
	    a[href]:hover button,
	    *[onclick]:empty:hover,
	    body a:empty:hover{ opacity:1!important }

	    body input[type],
	    body textarea[name],
	    body input[name],
	    body input[id],
	    body select[name] {
	    	-webkit-appearance:none!important;
	        color: #bbb!important;
	    	-webkit-border-radius:4px !important;
	        border-width: 1px!important;
	        border-color: #778!important;
	    	border-style:solid!important;
	        background:#555 none!important
	    }
	    body select[name] {
	    	-webkit-appearance:none!important;
	        color: #bbb!important;
	    	-webkit-border-radius:4px !important;
	        border-width: 1px!important;
	        border-color: #778!important;
	    	border-style:solid!important;
	        background-color:#555!important
	    }
	    body input>*, body textarea>* {
	        background:transparent none!important;
	        color: #bbb!important;
	    	border-style:solid!important;
	        border-width: 0px!important;
	    }
	    body select * {
	    	background-color:transparent !important;
	        color: #bbb!important;
	    	border-style:solid!important;
	        border-width: 0px!important;
	    }
	    pre:not(:empty),
	    code:not(:empty),
	    cite:not(:empty),
	    pre:not(:empty) *,
	    code:not(:empty) *,
	    cite:not(:empty) * {
	    	background-image:url(data:image/gif; base64,
	    	R0lGODlhBAAEAIAAABERESIiIiH5BAAAAAAALAAAAAAEAAQAAAIGTACXaHkFADs=
	    	)
	    !important;
	    color: #bcc8dc!important;
	    }";
	var heads = document.getElementsByTagName("head");
	var orig = inverse;

	if (heads.length == 0)
		return function() {};

	var node = document.createElement("style");
	node.type = "text/css";
	node.appendChild(document.createTextNode(css));
	heads[0].appendChild(node); 
	return function() {
		node.parentNode.removeChild(node);
		return orig;
	};
}
/*
 * CSS from NightShift - Eye Care by vetinari
 * http://userstyles.org/styles/18192
 */

function inverse() {
	var css = "
	    body, html { min-height: 100%!important; }
	    html, body { background-color:#111!important }
	    body>*:not(:empty) { background-color:#222!important }
	    body>*>*:not(:empty) { background-color:#222!important }
	    body>*>*>*:not(:empty) { background-color:#282828!important }
	    body>*>*>*>*:not(:empty) { background-color:#282828!important }
	    body>*>*>*>*>*:not(:empty) {background-color:#383838!important }
	    body>*>*>*>*>* * { background-color:#383838!important }
	    body table[border=\"0\"] td { background-color:#111!important }
	    body table table[border=\"0\"] td {
	    	background-color:#333!important
	    }
	    body table table table[border=\"0\"] td {
	    	background-color:#222!important
	    }
	    body table table table table[border=\"0\"] td {
	    	background-color:#444!important
	    }
	    body *:empty { background-color: #252525!important }

	    body p:not(:empty),
	    body p *,
	    body h1,
	    body h1 *,
	    body h2,
	    body h2 *,
	    body h3,
	    body h3 *,
	    body h4,
	    body h4 *,
	    body h5,
	    body h5 *,
	    body strong>*,
	    body b>*,
	    body em>*,
	    body i>*,
	    body span>*:not(img){ background:transparent none!important }

	    body h1,
	    body h1 *,
	    body h2,
	    body h2 *,
	    p>strong:only-of-type,
	    p>b:only-of-type{ color: #a98!important }

	    body h3,
	    body h3 *,
	    body h4,
	    body h4 *{ color: #aaa!important }
	    *:not([onclick]):not(input):not(a):not(img):not([class^=\"UI\"]),
	    body a:not(:empty),
	    div:not([onclick]) {
	    	background-image:none!important;
	        text-indent:0!important
	    }
	    *[onclick] { color:#79a!important }
	    *[onclick]:hover { color:#99a8aa!important }
	    body hr { 
	    	background: #666 none!important;
	    	color: #666!important;
	        border:1px solid #666!important;
	    	height: 1px!important;
	        overflow:hidden!important;
	    	display: block!important }
	    * { color: #c0c0c0!important; border-color:#666!important; }
	    * body a, body a *{ color: #B6AA7B!important; }
	    body a:hover, body a:hover * {
	    	color: #D9C077!important;
	        text-decoration: underline!important
	    }
	    body img,a[href] img,
	    a[href] button,
	    input[type=\"image\"],*[onclick]:empty,
	    body a:empty{ opacity:.5!important }
	    body img:hover,
	    a[href]:hover img,
	    a[href]:hover button,
	    *[onclick]:empty:hover,
	    body a:empty:hover{ opacity:1!important }

	    body input[type],
	    body textarea[name],
	    body input[name],
	    body input[id],
	    body select[name] {
	    	-webkit-appearance:none!important;
	        color: #bbb!important;
	    	-webkit-border-radius:4px !important;
	        border-width: 1px!important;
	        border-color: #778!important;
	    	border-style:solid!important;
	        background:#555 none!important
	    }
	    body select[name] {
	    	-webkit-appearance:none!important;
	        color: #bbb!important;
	    	-webkit-border-radius:4px !important;
	        border-width: 1px!important;
	        border-color: #778!important;
	    	border-style:solid!important;
	        background-color:#555!important
	    }
	    body input>*, body textarea>* {
	        background:transparent none!important;
	        color: #bbb!important;
	    	border-style:solid!important;
	        border-width: 0px!important;
	    }
	    body select * {
	    	background-color:transparent !important;
	        color: #bbb!important;
	    	border-style:solid!important;
	        border-width: 0px!important;
	    }
	    pre:not(:empty),
	    code:not(:empty),
	    cite:not(:empty),
	    pre:not(:empty) *,
	    code:not(:empty) *,
	    cite:not(:empty) * {
	    	background-image:url(data:image/gif; base64,
	    	R0lGODlhBAAEAIAAABERESIiIiH5BAAAAAAALAAAAAAEAAQAAAIGTACXaHkFADs=
	    	)
	    !important;
	    color: #bcc8dc!important;
	    }";
	var heads = document.getElementsByTagName("head");
	var orig = inverse;

	if (heads.length == 0)
		return function() {};

	var node = document.createElement("style");
	node.type = "text/css";
	node.appendChild(document.createTextNode(css));
	heads[0].appendChild(node); 
	return function() {
		node.parentNode.removeChild(node);
		return orig;
	};
}
/*
 * CSS from NightShift - Eye Care by vetinari
 * http://userstyles.org/styles/18192
 */

function inverse() {
	var css = "
	    body, html { min-height: 100%!important; }
	    html, body { background-color:#111!important }
	    body>*:not(:empty) { background-color:#222!important }
	    body>*>*:not(:empty) { background-color:#222!important }
	    body>*>*>*:not(:empty) { background-color:#282828!important }
	    body>*>*>*>*:not(:empty) { background-color:#282828!important }
	    body>*>*>*>*>*:not(:empty) {background-color:#383838!important }
	    body>*>*>*>*>* * { background-color:#383838!important }
	    body table[border=\"0\"] td { background-color:#111!important }
	    body table table[border=\"0\"] td {
	    	background-color:#333!important
	    }
	    body table table table[border=\"0\"] td {
	    	background-color:#222!important
	    }
	    body table table table table[border=\"0\"] td {
	    	background-color:#444!important
	    }
	    body *:empty { background-color: #252525!important }

	    body p:not(:empty),
	    body p *,
	    body h1,
	    body h1 *,
	    body h2,
	    body h2 *,
	    body h3,
	    body h3 *,
	    body h4,
	    body h4 *,
	    body h5,
	    body h5 *,
	    body strong>*,
	    body b>*,
	    body em>*,
	    body i>*,
	    body span>*:not(img){ background:transparent none!important }

	    body h1,
	    body h1 *,
	    body h2,
	    body h2 *,
	    p>strong:only-of-type,
	    p>b:only-of-type{ color: #a98!important }

	    body h3,
	    body h3 *,
	    body h4,
	    body h4 *{ color: #aaa!important }
	    *:not([onclick]):not(input):not(a):not(img):not([class^=\"UI\"]),
	    body a:not(:empty),
	    div:not([onclick]) {
	    	background-image:none!important;
	        text-indent:0!important
	    }
	    *[onclick] { color:#79a!important }
	    *[onclick]:hover { color:#99a8aa!important }
	    body hr { 
	    	background: #666 none!important;
	    	color: #666!important;
	        border:1px solid #666!important;
	    	height: 1px!important;
	        overflow:hidden!important;
	    	display: block!important }
	    * { color: #c0c0c0!important; border-color:#666!important; }
	    * body a, body a *{ color: #B6AA7B!important; }
	    body a:hover, body a:hover * {
	    	color: #D9C077!important;
	        text-decoration: underline!important
	    }
	    body img,a[href] img,
	    a[href] button,
	    input[type=\"image\"],*[onclick]:empty,
	    body a:empty{ opacity:.5!important }
	    body img:hover,
	    a[href]:hover img,
	    a[href]:hover button,
	    *[onclick]:empty:hover,
	    body a:empty:hover{ opacity:1!important }

	    body input[type],
	    body textarea[name],
	    body input[name],
	    body input[id],
	    body select[name] {
	    	-webkit-appearance:none!important;
	        color: #bbb!important;
	    	-webkit-border-radius:4px !important;
	        border-width: 1px!important;
	        border-color: #778!important;
	    	border-style:solid!important;
	        background:#555 none!important
	    }
	    body select[name] {
	    	-webkit-appearance:none!important;
	        color: #bbb!important;
	    	-webkit-border-radius:4px !important;
	        border-width: 1px!important;
	        border-color: #778!important;
	    	border-style:solid!important;
	        background-color:#555!important
	    }
	    body input>*, body textarea>* {
	        background:transparent none!important;
	        color: #bbb!important;
	    	border-style:solid!important;
	        border-width: 0px!important;
	    }
	    body select * {
	    	background-color:transparent !important;
	        color: #bbb!important;
	    	border-style:solid!important;
	        border-width: 0px!important;
	    }
	    pre:not(:empty),
	    code:not(:empty),
	    cite:not(:empty),
	    pre:not(:empty) *,
	    code:not(:empty) *,
	    cite:not(:empty) * {
	    	background-image:url(data:image/gif; base64,
	    	R0lGODlhBAAEAIAAABERESIiIiH5BAAAAAAALAAAAAAEAAQAAAIGTACXaHkFADs=
	    	)
	    !important;
	    color: #bcc8dc!important;
	    }";
	var heads = document.getElementsByTagName("head");
	var orig = inverse;

	if (heads.length == 0)
		return function() {};

	var node = document.createElement("style");
	node.type = "text/css";
	node.appendChild(document.createTextNode(css));
	heads[0].appendChild(node); 
	return function() {
		node.parentNode.removeChild(node);
		return orig;
	};
}
