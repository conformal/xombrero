GTK_VERSION?= gtk3
ifeq ("${GTK_VERSION}", "gtk2")
JS_CF=$(shell pkg-config --silence-errors --cflags javascriptcoregtk-1.0)
JS_LD=$(shell pkg-config --silence-errors --libs javascriptcoregtk-1.0)
else
JS_CF=$(shell pkg-config --silence-errors --cflags javascriptcoregtk-3.0)
JS_LD=$(shell pkg-config --silence-errors --libs javascriptcoregtk-3.0)
endif

LIBS= glib-2.0
ifeq ("${GTK_VERSION}", "gtk2")
LIBS+= gtk+-2.0 webkit-1.0
CFLAGS+=-I/usr/include/gtk-2.0
else
LIBS+= gtk+-3.0 webkitgtk-3.0
CFLAGS+=-I/usr/include/gtk-3.0
endif
LIBS+= libsoup-2.4 gthread-2.0 gnutls libbsd

CFLAGS+= -O2 -Wall -ggdb3 -D_GNU_SOURCE -I. -I.. $(shell pkg-config --cflags $(LIBS)) $(JS_CF)
LDADD+= $(shell pkg-config --libs $(LIBS)) $(JS_LD) -ldl -lgcrypt -lX11

PREFIX?= /usr/local
BINDIR?= $(PREFIX)/bin
LIBDIR?= $(PREFIX)/lib
APPDIR?= $(PREFIX)/share/applications
MANDIR?= $(PREFIX)/share/man
RESDIR?= $(PREFIX)/share/xombrero

SRCS= $(shell ls ../*.c)
SRCS+= linux.c
OBJS= $(SRCS:.c=.o)
DEPS= $(addsuffix .depend, $(OBJS))

CC?= gcc

BUILDVERSION= $(shell sh $(CURDIR)/../buildver.sh)
ifneq ("${BUILDVERSION}", "")
CFLAGS+= -DXOMBRERO_BUILDSTR=\"$(BUILDVERSION)\"
endif

all: javascript.h tooltip.h xombrero

javascript.h: ../js-merge-helper.pl ../hinting.js ../autoscroll.js
	perl ../js-merge-helper.pl ../hinting.js ../input-focus.js \
	../autoscroll.js > javascript.h

tooltip.h: ../ascii2txt.pl ../txt2tooltip.pl ../xombrero.1
	nroff -c -Tascii -mandoc ../xombrero.1 | \
	perl ../ascii2txt.pl | \
	perl ../txt2tooltip.pl > tooltip.h

xombrero: $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $+ $(LDADD)

%.o: %.c
	@echo "Generating $@.depend"
	@$(CC) -MM $(CPPFLAGS) $(CFLAGS) $< | \
	sed 's,^.*\.o[ :]*,$@ $@.depend : ,g' > $@.depend
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ -c $<

install: all
	install -m 755 -d $(DESTDIR)$(BINDIR)
	install -m 755 -d $(DESTDIR)$(LIBDIR)
	install -m 755 -d $(DESTDIR)$(MANDIR)/man1
	install -m 755 -d $(DESTDIR)$(RESDIR)
	install -m 755 -d $(DESTDIR)$(APPDIR)
	install -m 755 xombrero $(DESTDIR)$(BINDIR)
	install -m 644 ../xombrero.1 $(DESTDIR)$(MANDIR)/man1/xombrero.1
	install -m 644 ../xombrero.css $(DESTDIR)$(RESDIR)
	install -m 644 ../xombrero.desktop $(DESTDIR)$(APPDIR)
	install -m 644 ../xombreroicon16.png $(DESTDIR)$(RESDIR)
	install -m 644 ../xombreroicon32.png $(DESTDIR)$(RESDIR)
	install -m 644 ../xombreroicon48.png $(DESTDIR)$(RESDIR)
	install -m 644 ../xombreroicon64.png $(DESTDIR)$(RESDIR)
	install -m 644 ../xombreroicon128.png $(DESTDIR)$(RESDIR)
	install -m 644 ../tld-rules $(DESTDIR)$(RESDIR)
	install -m 644 ../style.css $(DESTDIR)$(RESDIR)
	install -m 644 ../hsts-preload $(DESTDIR)$(RESDIR)
	install -m 644 ../torenabled.ico $(DESTDIR)$(RESDIR)
	install -m 644 ../tordisabled.ico $(DESTDIR)$(RESDIR)

uninstall:
	rm -f $(DESTDIR)$(BINDIR)/xombrero
	rm -f $(DESTDIR)$(MANDIR)/man1/xombrero.1
	rm -f $(DESTDIR)$(PREFIX)/share/applications/xombrero.desktop
	rm -f $(DESTDIR)$(RESDIR)/xombreroicon.png
	rm -f $(DESTDIR)$(RESDIR)/xombreroicon16.png
	rm -f $(DESTDIR)$(RESDIR)/xombreroicon32.png
	rm -f $(DESTDIR)$(RESDIR)/xombreroicon48.png
	rm -f $(DESTDIR)$(RESDIR)/xombreroicon64.png
	rm -f $(DESTDIR)$(RESDIR)/xombreroicon128.png
	rm -f $(DESTDIR)$(RESDIR)/xombrero.css
	rm -f $(DESTDIR)$(RESDIR)/tordisabled.ico
	rm -f $(DESTDIR)$(RESDIR)/torenabled.ico
	rm -f $(DESTDIR)$(RESDIR)/tld-rules
	rm -f $(DESTDIR)$(RESDIR)/style.css
	rm -f $(DESTDIR)$(RESDIR)/hsts-preload
	if [ -d $(DESTDIR)$(RESDIR) ]; then rmdir $(DESTDIR)$(RESDIR); fi
clean:
	rm -f xombrero $(OBJS) $(DEPS)
	rm -f javascript.h
	rm -f tooltip.h


-include $(DEPS)

.PHONY: all install clean
