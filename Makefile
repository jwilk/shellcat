VERSION = $(shell head -n1 doc/changelog | cut -d ' ' -f2)

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

.PHONY: all clean

# vim:ts=4
