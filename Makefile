version = $(shell head -n1 doc/changelog | cut -d ' ' -f2)

CC = gcc
CFLAGS = -g -O2 -Wall -Wformat -Wextra
CPPFLAGS += -DVERSION='"$(version)"'

.PHONY: all
all: shellcat

shellcat: shellcat.c
	$(LINK.c) $(<) -o $(@)
            
.PHONY: clean
clean:
	rm -f shellcat

# vim:ts=4 sw=4 noet
