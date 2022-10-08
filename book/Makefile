# To build the book, we need a binary for the host architecture.
# Even when not cross-compiling the main binary (i.e. CXX=HOST_CXX),
# we build this "host binary" in a separate directory to avoid any
# conflicts.
# (Or call 'make HOST_HOICHESS=/usr/games/hoichess' if already installed.)
HOST_CXX = g++
HOST_CXXFLAGS = -W -Wall -O2
HOST_HOICHESS = ../build/host/hoichess

INSTALL = /usr/bin/install


# TODO xiangqi

BOOKS = $(patsubst %.pgn,%.dat,$(wildcard *.pgn))
BOOK_DEPTH = 0
BOOK_MINMOVECOUNT = 5


.PHONY: all
all: $(BOOKS)

.PHONY: clean
clean:
	rm -f $(BOOKS)

.PHONY: install
install: all
	$(INSTALL) -d $(DESTDIR)$(install_datadir)
ifneq ($(BOOKS),)
	$(INSTALL) -m 644 $(BOOKS) $(DESTDIR)$(install_datadir)
endif


#
# build book
#

$(BOOKS): %.dat: %.pgn $(HOST_HOICHESS)
	echo "book create $@ $< $(BOOK_DEPTH) $(BOOK_MINMOVECOUNT)" | $(HOST_HOICHESS) --norc

#
# build host binary required to create opening book
#

$(HOST_HOICHESS):
	@# BUILDDIR is relative to src/!
	$(MAKE) -C ../src CXX="$(HOST_CXX)" CXXFLAGS="$(HOST_CXXFLAGS)" BUILDDIR="$(dir $@)" all-hoichess

