LIBS= gtk+-2.0 webkit-1.0 libsoup-2.4 gnutls gthread-2.0

CFLAGS+= -O2 -Wall -ggdb3 -D_GNU_SOURCE -I. $(shell pkg-config --cflags $(LIBS))
LDADD+= $(shell pkg-config --libs $(LIBS)) -lbsd -ldl -lgcrypt

PREFIX?= /usr/local
BINDIR?= $(PREFIX)/bin
LIBDIR?= $(PREFIX)/lib
MANDIR?= $(PREFIX)/share/man

CC?= gcc

all: javascript.h xxxterm

javascript.h: ../js-merge-helper.pl ../hinting.js
	perl ../js-merge-helper.pl ../hinting.js ../input-focus.js > \
	javascript.h

xxxterm.o: javascript.h

xxxterm: ../whitelist.c ../settings.c ../marco.o ../about.o ../inspector.o ../xxxterm.o linux.o
	$(CC) $(LDFLAGS) -o $@ $+ $(LDADD)

install: all
	install -m 755 -d $(DESTDIR)$(BINDIR)
	install -m 755 -d $(DESTDIR)$(LIBDIR)
	install -m 755 -d $(DESTDIR)$(MANDIR)/man1
	install -m 755 xxxterm $(DESTDIR)$(BINDIR)
	install -m 644 ../xxxterm.1 $(DESTDIR)$(MANDIR)/man1/xxxterm.1

clean:
	rm -f xxxterm *.o ../*.o
	rm -f javascript.h

.PHONY: all install clean
