prefix = /usr
# Make these variables also available to sub-makes.
export bindir = $(prefix)/games
export docdir = $(prefix)/share/doc/hoichess
export mandir = $(prefix)/share/man
export datadir = $(prefix)/share/games/hoichess
export install_bindir = $(bindir)
export install_docdir = $(docdir)
export install_mandir = $(mandir)
export install_datadir = $(datadir)


VERSION := $(shell grep '^Version' ChangeLog | head -n 1 | awk '{ print $$2; }')
TARGET := $(shell $(CXX) -dumpmachine)
ifeq ($(TARGET),)
$(error Unable to determine compiler target)
endif


#
# Build
#

.PHONY: all
all: compile book doc cfg

.PHONY: compile
compile:
	$(MAKE) -C src all

.PHONY: book
book:
	$(MAKE) -C book

.PHONY: doc
doc: hoichess.6 hoichess.6.html

hoichess.6: hoichess.6.pod
	pod2man -n hoichess -s 6 -r "hoichess-$(VERSION)" -c Games $< $@

hoichess.6.html: hoichess.6.pod
	pod2html --title "HoiChess" $< > $@

.PHONY: cfg
cfg: hoichess.rc hoixiangqi.rc

hoichess.rc hoixiangqi.rc: %: %.m4
	m4 -D DATADIR=$(datadir) $< > $@


#
# Install
#

.PHONY: install
install: install-bin

INSTALL_BIN_DOCS = AUTHORS BUGS ChangeLog LICENSE README hoichess.6.html
INSTALL_BIN_DATA = hoichess.rc hoixiangqi.rc

.PHONY: install-bin 
install-bin: all
	$(MAKE) -C src install DESTDIR="$(DESTDIR)"
	install -m 644 -D hoichess.6 $(DESTDIR)$(install_mandir)/man6/hoichess.6
	install -m 755 -d $(DESTDIR)$(install_docdir)
	install -m 644 $(INSTALL_BIN_DOCS) $(DESTDIR)$(install_docdir)
	install -m 755 -d $(DESTDIR)$(install_datadir)
	install -m 644 $(INSTALL_BIN_DATA) $(DESTDIR)$(install_datadir)
	$(MAKE) -C book install DESTDIR="$(DESTDIR)"


#
# Clean
#

.PHONY: clean
clean:
	$(MAKE) -C src clean
	rm -f hoichess.6 hoichess.6.html pod2htmd.tmp pod2htmi.tmp
	rm -f hoichess.rc hoixiangqi.rc
	$(MAKE) -C book clean
	rm -rf $(DIST_OUTDIR)

.PHONY: maintainer-clean
maintainer-clean: clean
	$(MAKE) -C src maintainer-clean

