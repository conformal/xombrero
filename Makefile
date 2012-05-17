PREFIX?=/usr/local
BINDIR=${PREFIX}/bin

PROG=xombrero
MAN=xombrero.1

DEBUG= -g

SRCS= cookie.c inspector.c marco.c about.c whitelist.c settings.c inputfocus.c
SRCS+= history.c completion.c tldlist.c externaleditor.c unix.c xombrero.c
CFLAGS+= -O2 -Wall -Wno-format-extra-args -Wunused
CFLAGS+= -Wextra -Wno-unused-parameter -Wno-missing-field-initializers -Wno-sign-compare ${DEBUG}
CFLAGS+= -I. -I${.CURDIR}
LDADD= -lutil -lgcrypt
LIBS+= gtk+-2.0
LIBS+= webkit-1.0
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

CLEANFILES += ${.CURDIR}/javascript.h javascript.h xombrero.cat1 xombrero.core

JSFILES += hinting.js
JSFILES += input-focus.js
JSFILES += autoscroll.js

.for _js in ${JSFILES}
JSCURDIR += ${.CURDIR}/${_js}
.endfor

javascript.h: ${JSFILES} js-merge-helper.pl
	perl ${.CURDIR}/js-merge-helper.pl \
		${JSCURDIR} > javascript.h

beforeinstall:
	install -m 755 -d ${PREFIX}/share/xombrero
	install -m 644 ${.CURDIR}/xombreroicon.png ${PREFIX}/share/xombrero
	install -m 644 ${.CURDIR}/xombreroicon16.png ${PREFIX}/share/xombrero
	install -m 644 ${.CURDIR}/xombreroicon32.png ${PREFIX}/share/xombrero
	install -m 644 ${.CURDIR}/xombreroicon48.png ${PREFIX}/share/xombrero
	install -m 644 ${.CURDIR}/xombreroicon64.png ${PREFIX}/share/xombrero
	install -m 644 ${.CURDIR}/xombreroicon128.png ${PREFIX}/share/xombrero
	install -m 644 ${.CURDIR}/tld-rules ${PREFIX}/share/xombrero
	install -m 644 ${.CURDIR}/style.css ${PREFIX}/share/xombrero

${PROG} ${OBJS} beforedepend: javascript.h

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
