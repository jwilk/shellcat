VERSION   = $(shell cat VERSION)
DISTFILES = Makefile README VERSION scat.c

CC     = gcc
CFLAGS = -s -Os -DVERSION=\"$(VERSION)\"

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

.PHONY: all scat clean distclean dist
