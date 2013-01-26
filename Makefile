version = $(shell head -n1 doc/changelog | cut -d ' ' -f2)
pod2man = $(shell which pod2man)

CC = gcc
CFLAGS = -g -O2 -Wall -Wformat -Wextra -pedantic
CPPFLAGS += -DVERSION='"$(version)"'

.PHONY: all
all: shellcat

shellcat: shellcat.c
	$(LINK.c) $(<) -o $(@)
            
.PHONY: clean
clean:
	rm -f shellcat

ifneq "$(pod2man)" ""

all: doc/shellcat.1

doc/shellcat.1: doc/manpage.pod
	sed -e 's/L<\([a-z_-]\+\)(\([0-9]\+\))>/B<\1>(\2)/' $(<) \
	| pod2man --utf8 -c '' -n shellcat -r 'shellcat $(version)' \
	> $(@).tmp
	mv $(@).tmp $(@)

clean: clean-doc

.PHONY: clean-doc
clean-doc:
	rm -f doc/*.1 doc/*.tmp

endif

# vim:ts=4 sw=4 noet
