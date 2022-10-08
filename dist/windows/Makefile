TOPDIR = ../..

VERSION := $(shell $(TOPDIR)/src/build/print_version_str)


.PHONY: all
all: hoichess-$(VERSION)-win32.zip hoichess-$(VERSION)-win64.zip \
	hoichess-$(VERSION)-win32-pthread.zip hoichess-$(VERSION)-win64-pthread.zip

.PHONY: clean
clean:
	rm -rf build-*
	rm -f hoichess-*.zip


BUILDFLAGS := prefix= bindir=/bin docdir=/doc mandir=/man datadir="%USERPROFILE%/.hoichess" install_datadir=/share

hoichess-$(VERSION)-win32.zip: CXX=i686-w64-mingw32-g++
hoichess-$(VERSION)-win32.zip: LDFLAGS=-static
hoichess-$(VERSION)-win32.zip: BUILDFLAGS+=HAVE_PTHREAD=0

hoichess-$(VERSION)-win64.zip: CXX=x86_64-w64-mingw32-g++
hoichess-$(VERSION)-win64.zip: LDFLAGS=-static
hoichess-$(VERSION)-win64.zip: BUILDFLAGS+=HAVE_PTHREAD=0

hoichess-$(VERSION)-win32-pthread.zip: CXX=i686-w64-mingw32-g++
hoichess-$(VERSION)-win32-pthread.zip: LDFLAGS=-static

hoichess-$(VERSION)-win64-pthread.zip: CXX=x86_64-w64-mingw32-g++
hoichess-$(VERSION)-win64-pthread.zip: LDFLAGS=-static


hoichess-$(VERSION)-%.zip:
	mkdir build-$(VERSION)-$*
	$(MAKE) -s -C $(TOPDIR) BUILDDIR=$(CURDIR)/build-$(VERSION)-$* CXX=$(CXX) LDFLAGS="$(LDFLAGS)" $(BUILDFLAGS)
	$(MAKE) -s -C $(TOPDIR) BUILDDIR=$(CURDIR)/build-$(VERSION)-$* CXX=$(CXX) install $(BUILDFLAGS) \
		DESTDIR=$(CURDIR)/build-$(VERSION)-$*/hoichess-$(VERSION)-$*
	cp install.cmd build-$(VERSION)-$*/hoichess-$(VERSION)-$*/install.cmd
	cd build-$(VERSION)-$* && zip -q -9 -r hoichess-$(VERSION)-$*.zip hoichess-$(VERSION)-$*
	mv build-$(VERSION)-$*/hoichess-$(VERSION)-$*.zip $@

