VERSION   = $(shell cat VERSION)
DISTFILES = Makefile COPYING README VERSION scat.c

ifndef TCC
  CC = gcc
  CFLAGS_opt = -Os -s
  CFLAGS_std = -std=gnu99 -pedantic -Wall
else
  CC = tcc
endif
CFLAGS = $(CFLAGS_opt) $(CFLAGS_std) -DLARGEBUFFER -DVERSION=\"$(VERSION)\"

all: scat

scat: scat.c
	$(CC) $(CFLAGS) scat.c -o scat
            
clean:
	rm -f scat

distclean:
	rm -f scat-$(VERSION).tar.*

dist: distclean
	fakeroot tar cf scat-$(VERSION).tar $(DISTFILES)
	bzip2 -9 scat-$(VERSION).tar

.PHONY: all clean distclean dist
