/*
    (c) 2009 by Leon Winter
    (c) 2009, 2010 by Hannes Schueller
    (c) 2010 by Hans-Peter Deifel

    Copyright (c) 2009 Leon Winter
    Copyright (c) 2009, 2010 Hannes Schueller
    Copyright (c) 2009, 2010 Matto Fransen
    Copyright (c) 2010 Hans-Peter Deifel
    Copyright (c) 2010 Thomas Adam

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

function vimprobable_v(e, y) {
    t = e.nodeName.toLowerCase();
    if((t == 'input' && /^(text|password|checkbox|radio)$/.test(e.type))
    || /^(select|textarea)$/.test(t)
    || e.contentEditable == 'true')
        console.log('insertmode_'+(y=='focus'?'on':'off'));
}

if(document.activeElement)
    vimprobable_v(document.activeElement,'focus');

vimprobable_m=['focus','blur'];

if (document.getElementsByTagName("body")[0] !== null && typeof(document.getElementsByTagName("body")[0]) == "object") {
    for(i in vimprobable_m)
        document.getElementsByTagName('body')[0].addEventListener(vimprobable_m[i], function(x) {
            vimprobable_v(x.target,x.type);
        }, true);
}

self.onunload = function() {
    vimprobable_v(document.activeElement, '');
};
