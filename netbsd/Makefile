GTK_VERSION?= gtk3
.if ${GTK_VERSION} == "gtk2"
LIBS= gtk+-2.0 webkit-1.0
.else
LIBS= gtk+-3.0 webkitgtk-3.0
.endif
LIBS+= libsoup-2.4 gnutls gthread-2.0

LDADD= -lutil -lgcrypt -lX11
GTK_CFLAGS!= pkg-config --cflags $(LIBS)
GTK_LDFLAGS!= pkg-config --libs $(LIBS)
CFLAGS+= $(GTK_CFLAGS) -O2 -Wall -pthread -I. -I..
LDFLAGS+= $(GTK_LDFLAGS) -pthread

PREFIX?= /usr/local
BINDIR?= $(PREFIX)/bin
LIBDIR?= $(PREFIX)/lib
MANDIR?= $(PREFIX)/man
RESDIR?= $(PREFIX)/share/xombrero

CC= cc

all: ../javascript.h ../tooltip.h xombrero

../javascript.h: ../js-merge-helper.pl ../hinting.js ../autoscroll.js
	perl ../js-merge-helper.pl ../hinting.js ../input-focus.js \
	../autoscroll.js > ../javascript.h

../tooltip.h: ../ascii2txt.pl ../txt2tooltip.pl ../xombrero.1
	nroff -c -Tascii -mandoc ../xombrero.1 | \
	perl ../ascii2txt.pl | \
	perl ../txt2tooltip.pl > ../tooltip.h

xombrero.o: ../xombrero.o

unix.o: ../unix.o

marco.o: ../marco.o

whitelist.o: ../whitelist.o

settings.o: ../settings.o

about.o: ../about.o

inspector.o: ../inspector.o

cookie.o: ../cookie.o

inputfocus.o: ../inputfocus.o

history.o: ../history.o

completion.o: ../completion.o

externaleditor.o: ../externaleditor.o

tldlist.o: ../tldlist.o

../xombrero.o: ../javascript.h ../tooltip.h

xombrero: xombrero.o netbsd.o marco.o about.o inspector.o whitelist.o settings.o \
	cookie.o history.o completion.o inputfocus.o tldlist.o externaleditor.o \
	unix.o
	$(CC) $(LDFLAGS) -o $@ *.o $+ $(LDADD)

install: all
	install -m 755 -d $(DESTDIR)$(BINDIR)
	install -m 755 -d $(DESTDIR)$(LIBDIR)
	install -m 755 -d $(DESTDIR)$(MANDIR)/man1
	install -m 755 -d $(DESTDIR)$(RESDIR)
	install -m 755 xombrero $(DESTDIR)$(BINDIR)
	install -m 644 ../xombrero.1 $(DESTDIR)$(MANDIR)/man1/xombrero.1
	install -m 644 ../xombrero.css $(DESTDIR)$(RESDIR)
	install -m 644 ../xombreroicon16.png $(DESTDIR)$(RESDIR)
	install -m 644 ../xombreroicon32.png $(DESTDIR)$(RESDIR)
	install -m 644 ../xombreroicon48.png $(DESTDIR)$(RESDIR)
	install -m 644 ../xombreroicon64.png $(DESTDIR)$(RESDIR)
	install -m 644 ../xombreroicon128.png $(DESTDIR)$(RESDIR)
	install -m 644 ../tld-rules $(DESTDIR)$(RESDIR)
	install -m 644 ../style.css $(DESTDIR)$(RESDIR)
	install -m 644 ../hsts-preload $(DESTDIR)$(RESDIR)

clean:
	rm -f xombrero *.o
	rm -f javascript.h
	rm -f tooltip.h

.PHONY: all install clean
PREFIX?=/usr/local
BINDIR=${PREFIX}/bin

PROG=xombrero
MAN=xombrero.1

DEBUG= -g

SRCS= cookie.c inspector.c marco.c about.c whitelist.c settings.c inputfocus.c
SRCS+= history.c completion.c tldlist.c externaleditor.c unix.c xombrero.c
CFLAGS+= -O2 -Wall -Wno-format-extra-args -Wunused
CFLAGS+= -Wextra -Wno-unused-parameter -Wno-missing-field-initializers -Wno-sign-compare ${DEBUG}
CFLAGS+= -DGTK_DISABLE_SINGLE_INCLUDES -DGDK_DISABLE_DEPRECATED -DGTK_DISABLE_DEPRECATED -DGSEAL_ENABLE
CFLAGS+= -I. -I${.CURDIR}
LDADD= -lutil -lgcrypt
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
LIBS+= gthread-2.0
GTK_CFLAGS!= pkg-config --cflags $(LIBS)
GTK_LDFLAGS!= pkg-config --libs $(LIBS)
CFLAGS+= $(GTK_CFLAGS)
LDFLAGS+= $(GTK_LDFLAGS)
BUILDVERSION != sh "${.CURDIR}/buildver.sh"
.if !${BUILDVERSION} == ""
CPPFLAGS+= -DXOMBRERO_BUILDSTR=\"$(BUILDVERSION)\"
.endif

MANDIR= ${PREFIX}/man/man

CLEANFILES += ${.CURDIR}/javascript.h javascript.h tooltip.h xombrero.cat1 xombrero.core

JSFILES += hinting.js
JSFILES += input-focus.js
JSFILES += autoscroll.js

.for _js in ${JSFILES}
JSCURDIR += ${.CURDIR}/${_js}
.endfor

javascript.h: ${JSFILES} js-merge-helper.pl
	perl ${.CURDIR}/js-merge-helper.pl \
		${JSCURDIR} > javascript.h

tooltip.h: ${MAN} ascii2txt.pl txt2tooltip.pl
	mandoc -Tascii ${.CURDIR}/${MAN} | \
		perl ${.CURDIR}/ascii2txt.pl | \
		perl ${.CURDIR}/txt2tooltip.pl > tooltip.h

beforeinstall:
	install -m 755 -d ${PREFIX}/bin
	install -m 755 -d ${PREFIX}/man/man1/
	install -m 755 -d ${PREFIX}/share/xombrero
	install -m 644 $(.CURDIR)/xombrero.css ${PREFIX}/share/xombrero
	install -m 644 ${.CURDIR}/xombreroicon.png ${PREFIX}/share/xombrero
	install -m 644 ${.CURDIR}/xombreroicon16.png ${PREFIX}/share/xombrero
	install -m 644 ${.CURDIR}/xombreroicon32.png ${PREFIX}/share/xombrero
	install -m 644 ${.CURDIR}/xombreroicon48.png ${PREFIX}/share/xombrero
	install -m 644 ${.CURDIR}/xombreroicon64.png ${PREFIX}/share/xombrero
	install -m 644 ${.CURDIR}/xombreroicon128.png ${PREFIX}/share/xombrero
	install -m 644 ${.CURDIR}/tld-rules ${PREFIX}/share/xombrero
	install -m 644 ${.CURDIR}/style.css ${PREFIX}/share/xombrero
	install -m 644 ${.CURDIR}/hsts-preload ${PREFIX}/share/xombrero

${PROG} ${OBJS} beforedepend: javascript.h tooltip.h

# clang targets
.if ${.TARGETS:M*analyze*}
CFLAGS+= -Wdeclaration-after-statement -Wshadow
CC=clang
CXX=clang++
CPP=clang -E
CFLAGS+=--analyze
.elif ${.TARGETS:M*clang*}
CFLAGS+= -Wdeclaration-after-statement -Wshadow
CC=clang
CXX=clang++
CPP=clang -E
.endif

analyze: all
clang: all

.include <bsd.prog.mk>
