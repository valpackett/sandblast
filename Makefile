PROG = sandblast
CFLAGS ?= -O2
CFLAGS += -std=c11
#CFLAGS += -g -O0
CFLAGS += -Wall -Wextra -Wconversion -Wno-unused-parameter -Wno-incompatible-pointer-types-discards-qualifiers -Wstrict-overflow -Wformat=2 -Wformat-security -Werror=format-security -Wno-format-nonliteral
CFLAGS += -fstack-protector-all -ftrapv -D_FORTIFY_SOURCE=2 -fPIE -fsanitize=address
CFLAGS += -fblocks -lBlocksRuntime
CFLAGS += `pkgconf --cflags --libs libucl`
CFLAGS += -I/usr/include -L/usr/lib -ljail
LDFLAGS += -pie -Wl,-z,relro -Wl,-z,now
CLEANFILES += sandblast.core
DESTDIR ?= /usr/local
BINDIR ?= /bin
MANDIR ?= /man/man
SRCS = sandblast.c config.c admin.c logging.c memory.c

.include <bsd.prog.mk>
