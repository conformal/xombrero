GTK_VERSION?= gtk3
.if ${GTK_VERSION} == "gtk2"
LIBS= gtk+-2.0 webkit-1.0
.else
LIBS= gtk+-3.0 webkitgtk-3.0
.endif
LIBS+= libsoup-2.4 gnutls

LDADD= -lutil
GTK_CFLAGS!= pkgconf --cflags $(LIBS)
GTK_LDFLAGS!= pkgconf --libs $(LIBS)
CFLAGS+= $(GTK_CFLAGS) -O2 -Wall -I. -I..
LDFLAGS+= $(GTK_LDFLAGS)

PREFIX?= /usr/local
BINDIR?= $(PREFIX)/bin
MANDIR?= $(PREFIX)/man/man
RESDIR?= $(PREFIX)/share/xombrero
CFLAGS+= -DXT_DS_RESOURCE_DIR=\"$(RESDIR)\"

PROG=xombrero
MAN=xombrero.1

DEBUG= -g

SRCS= cookie.c inspector.c marco.c about.c whitelist.c settings.c inputfocus.c
SRCS+= history.c completion.c tldlist.c externaleditor.c unix.c xombrero.c
CFLAGS+= -O2 -Wall -Wno-format-extra-args -Wunused -Wextra -Wno-unused-parameter
CFLAGS+= -Wno-missing-field-initializers -Wno-sign-compare
CFLAGS+= -Wno-deprecated-declarations -Wfloat-equal ${DEBUG}
CFLAGS+= -DGTK_DISABLE_SINGLE_INCLUDES -DGDK_DISABLE_DEPRECATED -DGTK_DISABLE_DEPRECATED -DGSEAL_ENABLE
CFLAGS+= -I. -I${.CURDIR}
LDADD= -lutil
GTK_VERSION ?= gtk3
.if ${GTK_VERSION} == "gtk2"
LIBS+= gtk+-2.0
LIBS+= webkit-1.0
.else
LIBS+= gtk+-3.0
LIBS+= webkitgtk-3.0
.endif
LIBS+= libsoup-2.4
LIBS+= gnutls
GTK_CFLAGS!= pkg-config --cflags $(LIBS)
GTK_LDFLAGS!= pkg-config --libs $(LIBS)
CFLAGS+= $(GTK_CFLAGS)
LDFLAGS+= $(GTK_LDFLAGS)
BUILDVERSION != sh "${.CURDIR}/scripts/buildver.sh"
.if !${BUILDVERSION} == ""
CPPFLAGS+= -DXOMBRERO_BUILDSTR=\"$(BUILDVERSION)\"
.endif

CLEANFILES += ${.CURDIR}/javascript.h javascript.h tooltip.h xombrero.cat1 xombrero.core

JSFILES += hinting.js
JSFILES += input-focus.js
JSFILES += autoscroll.js

.for _js in ${JSFILES}
JSCURDIR += ${.CURDIR}/${_js}
.endfor

javascript.h: ${JSFILES} scripts/js-merge-helper.pl
	perl ${.CURDIR}/scripts/js-merge-helper.pl \
		${JSCURDIR} > javascript.h

tooltip.h: ${MAN} scripts/ascii2txt.pl scripts/txt2tooltip.pl
	mandoc -Tascii ${.CURDIR}/${MAN} | \
		perl ${.CURDIR}/scripts/ascii2txt.pl | \
		perl ${.CURDIR}/scripts/txt2tooltip.pl > tooltip.h


CC?= cc

all: javascript.h tooltip.h xombrero

xombrero: xombrero.o marco.o about.o inspector.o whitelist.o settings.o \
	cookie.o history.o completion.o inputfocus.o tldlist.o externaleditor.o \
	unix.o platform.o
	$(CC) $(LDFLAGS) -o $@ *.o $+ $(LDADD)

install: all
	install -m 755 -d $(DESTDIR)$(BINDIR)
	install -m 755 -d $(DESTDIR)$(MANDIR)/man1
	install -m 755 -d $(DESTDIR)$(PREFIX)/share/applications
	install -m 755 xombrero $(DESTDIR)$(BINDIR)
	install -m 644 xombrero.1 $(DESTDIR)$(MANDIR)/man1/xombrero.1
	install -m 644 xombrero.css $(DESTDIR)$(RESDIR)
	install -m 644 style.css $(DESTDIR)$(RESDIR)
	install -m 644 xombrero.desktop $(DESTDIR)$(PREFIX)/share/applications
	install -m 644 icons/xombreroicon16.png $(DESTDIR)$(RESDIR)
	install -m 644 icons/xombreroicon32.png $(DESTDIR)$(RESDIR)
	install -m 644 icons/xombreroicon48.png $(DESTDIR)$(RESDIR)
	install -m 644 icons/xombreroicon64.png $(DESTDIR)$(RESDIR)
	install -m 644 icons/xombreroicon128.png $(DESTDIR)$(RESDIR)
	install -m 644 icons/xombreroicon256.png $(DESTDIR)$(RESDIR)
	install -m 644 icons/favicon.ico $(DESTDIR)$(RESDIR)
	install -m 644 icons/torenabled.ico $(DESTDIR)$(RESDIR)
	install -m 644 icons/tordisabled.ico $(DESTDIR)$(RESDIR)
	install -m 644 data/tld-rules $(DESTDIR)$(RESDIR)
	install -m 644 data/hsts-preload $(DESTDIR)$(RESDIR)

clean:
	rm -f xombrero *.o
	rm -f javascript.h
	rm -f tooltip.h

.PHONY: all install clean

${PROG} ${OBJS} beforedepend: javascript.h tooltip.h

.include <bsd.prog.mk>
