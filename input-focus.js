/*
    (c) 2009 by Leon Winter
    (c) 2009, 2010 by Hannes Schueller
    (c) 2010 by Hans-Peter Deifel
    see LICENSE file
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
