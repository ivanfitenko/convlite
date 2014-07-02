all: subdirs headers imthumb.o gifthumb.o brokenjpeg.o fallbacks.o copy.o funnel_gif.o exif_data.o convlite

#installation options
VERSION = 0.9.3
DESTDIR =
PREFIX = /usr
BINDIR = $(PREFIX)/bin
DOCDIR = $(PREFIX)/share/doc

SUBDIRS = libnsgif
CFLAGS = -O2 -Wall -Ilibnsgif/include/ `imlib2-config --cflags`
#CFLAGS = -g -O2 -Wall -Ilibnsgif/include/ `imlib2-config --cflags`

# Target-per-file appears here for historical reasons
# This needs a cleanup

headers:
	echo '#define GEN_IMLIB_ERRNO_LENGTH '`awk /^enum\ _imlib_load_error/,/\}\;/ /usr/include/Imlib2.h | grep IMLIB | sed  -r 's/([A-Z_])+/\"&\"/g' | wc -l` > imlib_errno_generated.h
	echo >> imlib_errno_generated.h
	echo 'char *imlib_errno_generated[GEN_IMLIB_ERRNO_LENGTH] = {' >> imlib_errno_generated.h
	awk /^enum\ _imlib_load_error/,/\}\;/ /usr/include/Imlib2.h | grep IMLIB | sed  -r 's/([A-Z_])+/\"&\"/g' >> imlib_errno_generated.h
	echo '};' >> imlib_errno_generated.h
	
	echo '#define CONVLITE_VERSION "$(VERSION)"' > version.h

subdirs:
	for dir in $(SUBDIRS); do \
	$(MAKE) -C $$dir BUILDDIR=../; \
	done

imthumb: imthumb.c
	gcc $(CFLAGS) -c imthumb.c -o imthumb.o 

gifthumb: gifthumb.c
	gcc $(CFLAGS) -c gifthumb.c -o gifthumb.o

brokenjpeg: brokenjpeg.c
	gcc $(CFLAGS) -c brokenjpeg.c -o brokenjpeg.o

fallbacks: fallbacks.c
	gcc $(CFLAGS) -c fallbacks.c -o fallbacks.o

copy: copy.c
	gcc $(CFLAGS) -c copy.c -o copy.o

funnel_gif: funnel_gif.c
	gcc $(CFLAGS) -c funnel_gif.c funnel_gif.o

exif_data: exif_data.c
	gcc $(CFLAGS) -c exif_data.c exif_data.o

convlite: main.c main.o imthumb.o gifthumb.o brokenjpeg.o copy.o funnel_gif.o exif_data.o libnsgif.a
		gcc $(CFLAGS) -c main.c -o main.o
		gcc -o convlite main.o imthumb.o gifthumb.o brokenjpeg.o fallbacks.o copy.o funnel_gif.o exif_data.o libnsgif.a `imlib2-config --libs` `pkg-config --libs libexif` -lgd -lz -lm -lgif

install:
	install -d $(DESTDIR)$(BINDIR)
	install -d $(DESTDIR)$(DOCDIR)
	install -d $(DESTDIR)$(DOCDIR)/convlite-$(VERSION)/libnsgif
	install -m 0755 convlite $(DESTDIR)$(BINDIR)
	install -m 0644 README $(DESTDIR)$(DOCDIR)/convlite-$(VERSION)/
	install -m 0644 COPYING $(DESTDIR)$(DOCDIR)/convlite-$(VERSION)/
	install -m 0644 AUTHORS $(DESTDIR)$(DOCDIR)/convlite-$(VERSION)/
	install -m 0644 Changelog $(DESTDIR)$(DOCDIR)/convlite-$(VERSION)/
	install -m 0644 libnsgif/COPYING $(DESTDIR)$(DOCDIR)/convlite-$(VERSION)/libnsgif/

test: performancetest sanitycheck

performancetest:
	/bin/sh test/test.sh performance

sanitycheck:
	/bin/sh test/test.sh sanity

clean: 
	rm -f gifthumb
	rm -f imthumb
	rm -f convlite
	rm -f imlib_errno_generated.h
	rm -f version.h
	rm -f *.o
	rm -f *.a
	rm -f *.d
	rm -f stamp
	rmdir coverage
	rmdir docs
	rm -Rf test/tmp

.PHONY: subdirs test install
