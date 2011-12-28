LIBS= gtk+-2.0 webkit-1.0 libsoup-2.4 gnutls gthread-2.0

LDADD= -lutil -lgcrypt
GTK_CFLAGS!= pkg-config --cflags $(LIBS)
GTK_LDFLAGS!= pkg-config --libs $(LIBS)
CFLAGS+= $(GTK_CFLAGS) -O2 -Wall -pthread
LDFLAGS+= $(GTK_LDFLAGS) -pthread

PREFIX?= /usr/local
BINDIR?= $(PREFIX)/bin
LIBDIR?= $(PREFIX)/lib
MANDIR?= $(PREFIX)/man

CC?= cc

all: ../javascript.h xxxterm

../javascript.h: ../js-merge-helper.pl ../hinting.js
	perl ../js-merge-helper.pl ../hinting.js ../input-focus.js > \
	../javascript.h

xxxterm.o: ../xxxterm.o

marco.o: ../marco.o

whitelist.o: ../whitelist.o

settings.o: ../settings.o

about.o: ../about.o

inspector.o: ../inspector.o

cookie.o: ../cookie.o

inputfocus.o: ../inputfocus.o

../xxxterm.o: ../javascript.h

xxxterm: xxxterm.o freebsd.o marco.o about.o inspector.o whitelist.o settings.o cookie.o
	$(CC) $(LDFLAGS) -o $@ *.o $+ $(LDADD)

install: all
	install -m 755 -d $(DESTDIR)$(BINDIR)
	install -m 755 -d $(DESTDIR)$(LIBDIR)
	install -m 755 -d $(DESTDIR)$(MANDIR)/man1
	install -m 755 xxxterm $(DESTDIR)$(BINDIR)
	install -m 644 ../xxxterm.1 $(DESTDIR)$(MANDIR)/man1/xxxterm.1

clean:
	rm -f xxxterm *.o
	rm -f javascript.h

.PHONY: all install clean
