PREFIX?=/usr/local
BINDIR=${PREFIX}/bin

PROG=xxxterm
MAN=xxxterm.1

SRCS= xxxterm.c inspector.c marco.c
CFLAGS+= -O2
DEBUG= -ggdb3
LDADD= -lutil -lgcrypt
LIBS+= gtk+-2.0
LIBS+= webkit-1.0
LIBS+= libsoup-2.4
LIBS+= gnutls
LIBS+= gthread-2.0
GTK_CFLAGS!= pkg-config --cflags $(LIBS)
GTK_LDFLAGS!= pkg-config --libs $(LIBS)
CFLAGS+= $(GTK_CFLAGS) -Wall
LDFLAGS+= $(GTK_LDFLAGS)
BUILDVERSION != sh "${.CURDIR}/buildver.sh"
.if !${BUILDVERSION} == ""
CPPFLAGS+= -DXXXTERM_BUILDSTR=\"$(BUILDVERSION)\"
.endif

MANDIR= ${PREFIX}/man/man

CLEANFILES += ${.CURDIR}/javascript.h xxxterm.cat1 xxxterm.core

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
	cp ${.CURDIR}/style.css ${PREFIX}/share/xxxterm

${PROG} ${OBJS} beforedepend: ${.CURDIR}/javascript.h

# clang targets
.if ${.TARGETS:M*analyze*}
CC=clang
CXX=clang++
CPP=clang -E
CFLAGS+=--analyze
.elif ${.TARGETS:M*clang*}
CC=clang
CXX=clang++
CPP=clang -E
.endif

analyze: all
clang: all

.include <bsd.prog.mk>
