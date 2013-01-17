VERSION = $(shell sed -ne "1 s/^.* \([0-9.]\+\)$$/\\1/gp" < README)
FAKEROOT = $(shell command -v fakeroot 2>/dev/null)
DISTFILES = Makefile COPYING README shellcat.c

CFLAGS_def := -DLARGEBUFFER
CFLAGS_def += -DVERSION='"$(VERSION)"'
ifndef TCC
	CC = gcc
	CFLAGS_opt = -Os -s
	CFLAGS_std = -std=gnu99 -pedantic -Wall
else
	CC = tcc
endif
CFLAGS = $(CFLAGS_opt) $(CFLAGS_std) $(CFLAGS_def)

all: shellcat

shellcat: shellcat.c
	$(CC) $(CFLAGS) shellcat.c -o shellcat
            
clean:
	rm -f shellcat shellcat-*.tar.*

dist:
	$(FAKEROOT) tar -cjf shellcat-$(VERSION).tar.bz2 $(DISTFILES)

.PHONY: all clean dist

# vim:ts=4
