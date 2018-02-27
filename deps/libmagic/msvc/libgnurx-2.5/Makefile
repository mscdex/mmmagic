all: libgnurx-0.dll libgnurx.dll.a libregex.a 

THIS = libgnurx
VERSION = 2.5

CC = x86_64-w64-mingw32-gcc -mthreads
CFLAGS = -I .

SOURCES = $(wildcard *.c *.h)
OBJECTS = regex.o

libgnurx-0.dll libgnurx.dll.a: $(OBJECTS) Makefile
	$(CC) -shared -o libgnurx-0.dll -Wl,--enable-auto-image-base -Wl,--out-implib,libgnurx.dll.a -Wl,--output-def,libgnurx.def $(OBJECTS)

libregex.a: libgnurx.dll.a
	cp -p libgnurx.dll.a $@

dist: $(THIS)-$(VERSION).zip $(THIS)-dev-$(VERSION).zip $(THIS)-src-$(VERSION).zip

$(THIS)-$(VERSION).zip: libgnurx-0.dll
	mkdir -p runtime/bin
	cp -p libgnurx-0.dll runtime/bin
	(cd runtime; zip -r ../$(THIS)-$(VERSION).zip bin)
	rm -rf runtime

$(THIS)-dev-$(VERSION).zip:  regex.h libgnurx.dll.a libregex.a
	mkdir -p dev/include dev/lib
	cp -p regex.h dev/include
	cp -p libgnurx.dll.a libregex.a gnurx.lib dev/lib
	(cd dev; zip -r ../$(THIS)-dev-$(VERSION).zip .)
	rm -rf dev

$(THIS)-src-$(VERSION).zip: Makefile README COPYING.LIB $(SOURCES)
	mkdir $(THIS)-$(VERSION)
	cp -p Makefile README COPYING.LIB $(SOURCES) $(THIS)-$(VERSION)
	zip -r $@ $(THIS)-$(VERSION$)
	rm -rf $(THIS)-$(VERSION$)

clean:
	rm -f *~ *.o *.dll *.def *.exp *.a *.zip
	rm -rf runtime dev $(THIS)-$(VERSION)
