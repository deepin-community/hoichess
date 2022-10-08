CROSS_COMPILE =

CXX = $(CROSS_COMPILE)g++
CPP = $(CXX) -E
INSTALL = /usr/bin/install

LIBPATH =
CXXFLAGS = -W -Wall -O3 -fno-exceptions $(ADD_CXXFLAGS)
LDFLAGS  = 

###############################################################################

ifneq (,$(findstring s,$(MAKEFLAGS)))
SILENT = 1
endif

VERSION := $(shell build/print_version_str ..)

TARGET := $(shell $(CXX) -dumpmachine)
ifeq ($(TARGET),)
$(error Unable to determine compiler target)
endif

BUILDDIR = ../build/$(TARGET)

ifeq (,$(findstring clean,$(MAKECMDGOALS)))
-include $(BUILDDIR)/config.mk
endif

ifneq (,$(findstring mingw32,$(TARGET)))
WITH_THREAD = 1
else ifeq ($(HAVE_PTHREAD),1)
WITH_THREAD = 1
endif

#
# define PROGS
#

PROGS = hoichess hoixiangqi

#
# define SOURCES, CXXFLAGS, INCLUDE, etc.
# (ADD_SOURCES can optionally be set at make command line)
#

SOURCES = \
	debug.cc \
	init.cc \
	main.cc \
	uint64_table.cc \
	util.cc \
	version.cc \
	common/book.cc \
	common/clock.cc \
	common/epd.cc \
	common/eval.cc \
	common/evalcache.cc \
	common/game.cc \
	common/hash.cc \
	common/movelist.cc \
	common/node.cc \
	common/pawnhash.cc \
	common/pgn.cc \
	common/search.cc \
	common/search_util.cc \
	common/shell.cc \
	common/shell_cmd.cc \
	common/shell_util.cc

ifeq ($(WITH_THREAD),1)
SOURCES += \
	mutex.cc \
	queue.cc \
	spinlock.cc \
	thread.cc \
	common/parallelsearch.cc
endif

hoichess_SOURCES = $(SOURCES) \
	chess/basic.cc \
	chess/bitboard.cc \
	chess/bitboard_rotated.cc \
	chess/board.cc \
	chess/board_attack.cc \
	chess/board_generate.cc \
	chess/board_init.cc \
	chess/board_util.cc \
	chess/eval.cc \
	chess/move.cc

hoixiangqi_SOURCES = $(SOURCES) \
	xiangqi/basic.cc \
	xiangqi/board.cc \
	xiangqi/board_attack.cc \
	xiangqi/board_generate.cc \
	xiangqi/board_init.cc \
	xiangqi/board_util.cc \
	xiangqi/eval.cc \
	xiangqi/move.cc


INCLUDE = -I. -Icommon

all-hoichess: override CXXFLAGS += -DHOICHESS
all-hoichess: override INCLUDE += -Ichess

all-hoixiangqi: override CXXFLAGS += -DHOIXIANGQI
all-hoixiangqi: override INCLUDE += -Ixiangqi


ifneq (,$(findstring mingw32,$(TARGET)))
override INCLUDE += -Iwin32
override CXXFLAGS += -DWITH_THREAD
override LDFLAGS += -static-libgcc -static-libstdc++
BIN_SUFFIX = .exe
else
BIN_SUFFIX = 
endif

ifneq (,$(shell echo '' | $(CXX) -E -dM - | grep __leonbare__))
SOURCES += \
	   sparc32/leon/stdio.cc \
	   sparc32/leon/usleep.cc
override INCLUDE += -Isparc32/leon
endif


ifeq ($(HAVE_READLINE),1)
override CXXFLAGS += -DHAVE_READLINE
LIBS += -lreadline
endif

ifeq ($(HAVE_PTHREAD),1)
override CXXFLAGS += -DHAVE_PTHREAD -DWITH_THREAD
LIBS += -lpthread
endif

ifeq ($(HAVE_GETOPT),1)
override CXXFLAGS += -DHAVE_GETOPT
else
SOURCES += lib/my_getopt.cc
endif

ifeq ($(HAVE_SNPRINTF),1)
override CXXFLAGS += -DHAVE_SNPRINTF
else
SOURCES += lib/snprintf.cc
endif

ifeq ($(HAVE_STRTOK_R),1)
override CXXFLAGS += -DHAVE_STRTOK_R
else
SOURCES += lib/strtok_r.cc
endif

ifneq ($(datadir),)
override CXXFLAGS += -DDATA_DIR='"$(datadir)"'
endif

ifeq ($(wildcard vhdlchess/),vhdlchess/)
include vhdlchess/Makefile.inc
endif

$(foreach p,$(PROGS),all-$(p)): override CXXFLAGS += -DVERSION='"$(VERSION)"'

###############################################################################

.DEFAULT_GOAL = all

.PHONY: all
all: $(foreach p,$(PROGS),all-$(p))

.PHONY: install
install: $(foreach p,$(PROGS),install-$(p))

.PHONY: clean
clean:
	rm -rf ../build

.PHONY: maintainer-clean
maintainer-clean: clean


# auto configuration
$(BUILDDIR)/config.mk:
	mkdir -p $(dir $@)
	CXX="$(CXX)" CXXFLAGS="$(CXXFLAGS)" build/configure.sh $(BUILDDIR) $(if $(SILENT),>/dev/null)


#
# building targets
#

define gen_prog_rules
# object files for program
$(1)_OBJS = $$(patsubst %.cc,$$(BUILDDIR)/build-$(1)/%.o,$$($(1)_SOURCES))

# all- target
.PHONY: all-$(1)
all-$(1): $$(BUILDDIR)/$(1)$(BIN_SUFFIX)

# compile sources
$$(BUILDDIR)/build-$(1)/%.o: %.cc
	@mkdir -p $$(dir $$@)
	$$(CXX) $$(CXXFLAGS) $$(CPPFLAGS) $$(INCLUDE) -MT $$@ -MD -MF $$(patsubst %.o,%.d,$$@) -c -o $$@ $$<

# link binary
$$(BUILDDIR)/$(1)$(BIN_SUFFIX): $$($(1)_OBJS)
	@mkdir -p $$(dir $$@)
	$$(CXX) $$(CXXFLAGS) $$(CPPFLAGS) -c -o $$(BUILDDIR)/build-$(1)/version.o version.cc
	$$(CXX) $$(CXXFLAGS) $$(LDFLAGS) -o $$@ $$^ $$(LIBPATH) $$(LIBS)

# include dependencies
-include $$(patsubst %.o,%.d,$$($(1)_OBJS))

# install- target
.PHONY: install-$(1)
install-$(1): all-$(1)
	$$(INSTALL) -m 755 -D $$(BUILDDIR)/$(1)$(BIN_SUFFIX) $(DESTDIR)$(install_bindir)/$$(notdir $$(BUILDDIR)/$(1)$(BIN_SUFFIX))
endef

$(foreach p,$(PROGS),$(eval $(call gen_prog_rules,$(p))))


