LIBS= gtk+-2.0 webkit-1.0 libsoup-2.4 gnutls gthread-2.0

CFLAGS+= -O2 -Wall -ggdb3 -D_GNU_SOURCE -I. $(shell pkg-config --cflags $(LIBS))
LDADD+= $(shell pkg-config --libs $(LIBS)) -lbsd -ldl -lgcrypt

PREFIX?= /usr/local
BINDIR?= $(PREFIX)/bin
LIBDIR?= $(PREFIX)/lib
MANDIR?= $(PREFIX)/share/man
RESDIR?= $(PREFIX)/share/xxxterm

SRCS= $(shell ls ../*.c)
SRCS+= linux.c
OBJS= $(SRCS:.c=.o)
DEPS= $(addsuffix .depend, $(OBJS))

CC?= gcc

all: javascript.h xxxterm

javascript.h: ../js-merge-helper.pl ../hinting.js ../autoscroll.js
	perl ../js-merge-helper.pl ../hinting.js ../input-focus.js \
	../autoscroll.js > javascript.h

xxxterm: $(OBJS)
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
	install -m 755 xxxterm $(DESTDIR)$(BINDIR)
	install -m 644 ../xxxterm.1 $(DESTDIR)$(MANDIR)/man1/xxxterm.1
	install -m 644 ../xxxtermicon16.png $(DESTDIR)$(RESDIR)
	install -m 644 ../xxxtermicon32.png $(DESTDIR)$(RESDIR)
	install -m 644 ../xxxtermicon48.png $(DESTDIR)$(RESDIR)
	install -m 644 ../xxxtermicon64.png $(DESTDIR)$(RESDIR)
	install -m 644 ../xxxtermicon128.png $(DESTDIR)$(RESDIR)
	install -m 644 ../style.css $(DESTDIR)$(RESDIR)

uninstall:
	rm -f $(DESTDIR)$(BINDIR)/xxxterm
	rm -f $(DESTDIR)$(MANDIR)/man1/xxxterm.1
	rm -f $(DESTDIR)$(RESDIR)/xxxtermicon.png
	rm -f $(DESTDIR)$(RESDIR)/xxxtermicon16.png
	rm -f $(DESTDIR)$(RESDIR)/xxxtermicon32.png
	rm -f $(DESTDIR)$(RESDIR)/xxxtermicon48.png
	rm -f $(DESTDIR)$(RESDIR)/xxxtermicon64.png
	rm -f $(DESTDIR)$(RESDIR)/xxxtermicon128.png
	rm -f $(DESTDIR)$(RESDIR)/style.css
	if [ -d $(DESTDIR)$(RESDIR) ]; then rmdir $(DESTDIR)$(RESDIR); fi
clean:
	rm -f xxxterm $(OBJS) $(DEPS)
	rm -f javascript.h


-include $(DEPS)

.PHONY: all install clean
