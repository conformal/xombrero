# $xxxterm$ 

LIBS= gtk+-2.0 webkit-1.0 libsoup-2.4

CFLAGS+= -Wall -ggdb -D_GNU_SOURCE -I. $(shell pkg-config --cflags $(LIBS))
LDADD+= $(shell pkg-config --libs $(LIBS))

PREFIX?= /usr/local
BINDIR?= $(PREFIX)/bin
LIBDIR?= $(PREFIX)/lib
MANDIR?= $(PREFIX)/share/man

CC?= gcc

all: xxxterm

javascript.h: ../js-merge-helper.pl ../hinting.js
	perl ../js-merge-helper.pl ../hinting.js \
		../input-focus.js > javascript.h

xxxterm.o: javascript.h

xxxterm: ../xxxterm.o linux.o
	$(CC) $(LDFLAGS) -o $@ $+ $(LDADD)

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
