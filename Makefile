version = $(shell head -n1 doc/changelog | cut -d ' ' -f2)

CFLAGS = -g -O2 -Wall
CPPFLAGS += -DVERSION='"$(version)"'

.PHONY: all
all: shellcat

shellcat: shellcat.c
	$(LINK.c) $(<) -o $(@)
            
.PHONY: clean
clean:
	rm -f shellcat

# vim:ts=4
