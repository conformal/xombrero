JS_CF=$(shell pkg-config --silence-errors --cflags javascriptcoregtk-1.0)
JS_LD=$(shell pkg-config --silence-errors --libs javascriptcoregtk-1.0)

GTK_VERSION?= gtk3
LIBS= glib-2.0
ifeq ("${GTK_VERSION}", "gtk2")
LIBS+= gtk+-2.0 webkit-1.0
else
LIBS+= gtk+-3.0 webkitgtk-3.0
endif
LIBS+= libsoup-2.4 gthread-2.0 gnutls libbsd

CFLAGS+= -O2 -Wall -ggdb3 -D_GNU_SOURCE -I. -I.. $(shell pkg-config --cflags $(LIBS)) $(JS_CF)
LDADD+= $(shell pkg-config --libs $(LIBS)) $(JS_LD) -ldl -lgcrypt -lX11

PREFIX?= /usr/local
BINDIR?= $(PREFIX)/bin
LIBDIR?= $(PREFIX)/lib
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

all: javascript.h xombrero

javascript.h: ../js-merge-helper.pl ../hinting.js ../autoscroll.js
	perl ../js-merge-helper.pl ../hinting.js ../input-focus.js \
	../autoscroll.js > javascript.h

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

uninstall:
	rm -f $(DESTDIR)$(BINDIR)/xombrero
	rm -f $(DESTDIR)$(MANDIR)/man1/xombrero.1
	rm -f $(DESTDIR)$(RESDIR)/xombreroicon.png
	rm -f $(DESTDIR)$(RESDIR)/xombreroicon16.png
	rm -f $(DESTDIR)$(RESDIR)/xombreroicon32.png
	rm -f $(DESTDIR)$(RESDIR)/xombreroicon48.png
	rm -f $(DESTDIR)$(RESDIR)/xombreroicon64.png
	rm -f $(DESTDIR)$(RESDIR)/xombreroicon128.png
	rm -f $(DESTDIR)$(RESDIR)/style.css
	if [ -d $(DESTDIR)$(RESDIR) ]; then rmdir $(DESTDIR)$(RESDIR); fi
clean:
	rm -f xombrero $(OBJS) $(DEPS)
	rm -f javascript.h


-include $(DEPS)

.PHONY: all install clean
