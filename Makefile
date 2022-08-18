VERCMD  ?= git describe --tags 2> /dev/null
VERSION := $(shell $(VERCMD) || cat VERSION)

CPPFLAGS += -D_POSIX_C_SOURCE=200809L -DVERSION=\"$(VERSION)\"
CFLAGS   += -std=c99 -pedantic -Wall -Wextra -DJSMN_STRICT
LDFLAGS  ?=
LDLIBS    = $(LDFLAGS) -lm -lxcb -lxcb-util -lxcb-keysyms -lxcb-icccm -lxcb-ewmh -lxcb-randr -lxcb-xinerama -lxcb-shape

PREFIX    ?= /usr
BINPREFIX ?= $(PREFIX)/bin
MANPREFIX ?= $(PREFIX)/share/man
DOCPREFIX ?= $(PREFIX)/share/doc/strawberry
BASHCPL   ?= $(PREFIX)/share/bash-completion/completions
XSESSIONS ?= $(PREFIX)/share/xsessions

WM_SRC   = strawberry.c helpers.c geometry.c jsmn.c settings.c monitor.c desktop.c tree.c stack.c history.c \
	 events.c pointer.c window.c messages.c parse.c query.c restore.c rule.c ewmh.c subscribe.c
WM_OBJ  := $(WM_SRC:.c=.o)
CLI_SRC  = strawberryc.c helpers.c
CLI_OBJ := $(CLI_SRC:.c=.o)

all: strawberry strawberryc

debug: CFLAGS += -O0 -g
debug: strawberry strawberryc

VPATH=src

include Sourcedeps

$(WM_OBJ) $(CLI_OBJ): Makefile

strawberry: $(WM_OBJ)

strawberryc: $(CLI_OBJ)

install:
	mkdir -p "$(DESTDIR)$(BINPREFIX)"
	cp -pf strawberry "$(DESTDIR)$(BINPREFIX)"
	cp -pf strawberryc "$(DESTDIR)$(BINPREFIX)"
	mkdir -p "$(DESTDIR)$(MANPREFIX)"/man1
	cp -p doc/strawberry.1 "$(DESTDIR)$(MANPREFIX)"/man1
	cp -Pp doc/strawberryc.1 "$(DESTDIR)$(MANPREFIX)"/man1
	mkdir -p "$(DESTDIR)$(BASHCPL)"
	cp -p examples/bash_completion "$(DESTDIR)$(BASHCPL)"/strawberryc
	mkdir -p "$(DESTDIR)$(DOCPREFIX)"/examples
	cp -pr examples/* "$(DESTDIR)$(DOCPREFIX)"/examples
	mkdir -p "$(DESTDIR)$(XSESSIONS)"
	cp -p examples/strawberry.desktop "$(DESTDIR)$(XSESSIONS)"

uninstall:
	rm -f "$(DESTDIR)$(BINPREFIX)"/strawberry
	rm -f "$(DESTDIR)$(BINPREFIX)"/strawberryc
	rm -f "$(DESTDIR)$(MANPREFIX)"/man1/strawberry.1
	rm -f "$(DESTDIR)$(MANPREFIX)"/man1/strawberryc.1
	rm -f "$(DESTDIR)$(BASHCPL)"/strawberryc
	rm -rf "$(DESTDIR)$(DOCPREFIX)"
	rm -f "$(DESTDIR)$(XSESSIONS)"/strawberry.desktop

doc:
	a2x -v -d manpage -f manpage -a revnumber=$(VERSION) doc/strawberry.1.asciidoc

clean:
	rm -f $(WM_OBJ) $(CLI_OBJ) strawberry strawberryc

.PHONY: all debug install uninstall doc clean
