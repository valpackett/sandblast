CC ?= /usr/bin/cc
CFLAGS ?= -Ofast
CFLAGS += -std=c99
#CFLAGS += -g -O0
CFLAGS += -Wall -Wextra -Wconversion -Wno-unused-parameter -Wno-incompatible-pointer-types-discards-qualifiers -Wstrict-overflow -Wformat-security -Wformat=2 -Wno-format-nonliteral
CFLAGS += -fstack-protector-all -fsanitize=bounds -ftrapv
CFLAGS += -fPIC -fPIE
CFLAGS += `pkgconf --cflags --libs jansson`
CFLAGS += -I/usr/include -L/usr/lib -ljail
LDFLAGS += -pie
PREFIX ?= /usr/local

all: sandblast

install: all
	install -s sandblast $(PREFIX)/bin
	install sandblast.1 $(PREFIX)/man/man1/
	mkdir -p $(PREFIX)/share/sandblast
	cp -Rf plugins $(PREFIX)/share/sandblast/

clean:
	rm -f sandblast sandblast.core

sandblast: sandblast.c util.c
	$(CC) $(CFLAGS) -o sandblast sandblast.c $(LDFLAGS)

.PHONY: all install clean
