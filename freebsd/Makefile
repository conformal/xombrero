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
MANDIR?= $(PREFIX)/man
RESDIR?= $(PREFIX)/share/xombrero

CC?= cc

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

xombrero: xombrero.o freebsd.o marco.o about.o inspector.o whitelist.o settings.o \
	cookie.o history.o completion.o inputfocus.o tldlist.o externaleditor.o \
	unix.o
	$(CC) $(LDFLAGS) -o $@ *.o $+ $(LDADD)

install: all
	install -m 755 -d $(DESTDIR)$(BINDIR)
	install -m 755 -d $(DESTDIR)$(MANDIR)/man1
	install -m 755 xombrero $(DESTDIR)$(BINDIR)
	install -m 644 ../xombrero.1 $(DESTDIR)$(MANDIR)/man1/xombrero.1
	install -m 644 ../xombrero.css $(DESTDIR)$(RESDIR)
	install -m 644 ../xombrero.desktop $(DESTDIR)$(PREFIX)/share/applications
	install -m 644 ../xombreroicon16.png $(DESTDIR)$(RESDIR)
	install -m 644 ../xombreroicon32.png $(DESTDIR)$(RESDIR)
	install -m 644 ../xombreroicon48.png $(DESTDIR)$(RESDIR)
	install -m 644 ../xombreroicon64.png $(DESTDIR)$(RESDIR)
	install -m 644 ../xombreroicon128.png $(DESTDIR)$(RESDIR)
	install -m 644 ../xombreroicon256.png $(DESTDIR)$(RESDIR)
	install -m 644 ../favicon.ico $(DESTDIR)$(RESDIR)
	install -m 644 ../tld-rules $(DESTDIR)$(RESDIR)
	install -m 644 ../style.css $(DESTDIR)$(RESDIR)
	install -m 644 ../hsts-preload $(DESTDIR)$(RESDIR)
	install -m 644 ../torenabled.ico $(DESTDIR)$(RESDIR)
	install -m 644 ../tordisabled.ico $(DESTDIR)$(RESDIR)

clean:
	rm -f xombrero *.o
	rm -f javascript.h
	rm -f tooltip.h

.PHONY: all install clean
