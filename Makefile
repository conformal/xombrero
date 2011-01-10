# $xxxterm$

PREFIX?=/usr/local
BINDIR=${PREFIX}/bin

PROG=xxxterm
MAN=xxxterm.1

SRCS= xxxterm.c
COPT+= -O2
DEBUG= -ggdb3
LDADD= -lutil
LIBS+= gtk+-2.0
LIBS+= webkit-1.0
LIBS+= libsoup-2.4
LIBS+= gnutls
GTK_CFLAGS!= pkg-config --cflags $(LIBS)
GTK_LDFLAGS!= pkg-config --libs $(LIBS)
CFLAGS+= $(GTK_CFLAGS) -Wall -pthread
LDFLAGS+= $(GTK_LDFLAGS) -pthread

MANDIR= ${PREFIX}/man/cat

CLEANFILES += javascript.h

${.CURDIR}/javascript.h: hinting.js input-focus.js
	perl ${.CURDIR}/js-merge-helper.pl ${.CURDIR}/hinting.js \
	    ${.CURDIR}/input-focus.js >  ${.CURDIR}/javascript.h

beforeinstall:
	mkdir -p ${PREFIX}/share/xxxterm
	cp ${.CURDIR}/xxxtermicon16.png ${PREFIX}/share/xxxterm
	cp ${.CURDIR}/xxxtermicon32.png ${PREFIX}/share/xxxterm
	cp ${.CURDIR}/xxxtermicon48.png ${PREFIX}/share/xxxterm
	cp ${.CURDIR}/xxxtermicon64.png ${PREFIX}/share/xxxterm
	cp ${.CURDIR}/xxxtermicon128.png ${PREFIX}/share/xxxterm

${PROG} ${OBJS} beforedepend: ${.CURDIR}/javascript.h

.include <bsd.prog.mk>
