# $xxxterm

PROG=xxxterm
MAN=xxxterm.1

SRCS= xxxterm.c
COPT+= -O2
DEBUG+= -ggdb3 
LDADD= -l util
LIBS+= gtk+-2.0
LIBS+= webkit-1.0
LIBS+= libsoup-2.4
GTK!=pkg-config --cflags --libs $(LIBS)
CFLAGS+=$(GTK) -Wall -pthread
LDFLAGS+=$(GTK) -pthread

.include <bsd.prog.mk>
