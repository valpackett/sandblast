PROG = sandblast
CFLAGS ?= -Ofast
CFLAGS += -std=c99
#CFLAGS += -g -O0
CFLAGS += -Wall -Wextra -Wconversion -Wno-unused-parameter -Wno-incompatible-pointer-types-discards-qualifiers -Wstrict-overflow -Wformat-security -Wformat=2 -Wno-format-nonliteral
CFLAGS += -fstack-protector-all -fsanitize=bounds -ftrapv
CFLAGS += -fPIC -fPIE
CFLAGS += `pkgconf --cflags --libs jansson`
CFLAGS += -I/usr/include -L/usr/lib -ljail
LDFLAGS += -pie
CLEANFILES += sandblast.core
DESTDIR ?= /usr/local
BINDIR ?= /bin
SHAREDIR ?= /share

beforeinstall:
	mkdir -p ${DESTDIR}/share/sandblast
	cp -Rf plugins ${DESTDIR}/share/sandblast/

.include <bsd.prog.mk>
