PROG = sandblast
CFLAGS ?= -Ofast
CFLAGS += -std=c99
#CFLAGS += -g -O0
CFLAGS += -Wall -Wextra -Wconversion -Wno-unused-parameter -Wno-incompatible-pointer-types-discards-qualifiers -Wstrict-overflow -Wformat-security -Wformat=2 -Wno-format-nonliteral
CFLAGS += -fstack-protector-all -ftrapv
CFLAGS += -fPIC -fPIE
CFLAGS += -fblocks -lBlocksRuntime
CFLAGS += `pkgconf --cflags --libs libucl`
CFLAGS += -I/usr/include -L/usr/lib -ljail
LDFLAGS += -pie
CLEANFILES += sandblast.core
DESTDIR ?= /usr/local
BINDIR ?= /bin
SHAREDIR ?= /share
SRCS = sandblast.c config.c admin.c logging.c memory.c

.include <bsd.prog.mk>
