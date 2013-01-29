version = $(shell head -n1 doc/changelog | cut -d ' ' -f2 | tr -d '()')
pod2man = $(shell which pod2man)

CC = gcc
CFLAGS = -g -O2 -Wall -Wformat -Wextra -pedantic
CPPFLAGS += -DVERSION='"$(version)"'

PREFIX = /usr/local
DESTDIR =

.PHONY: all
all: shellcat

shellcat: shellcat.c
	$(LINK.c) $(<) -o $(@)

.PHONY: install
install: shellcat
	install -D -m755 $(<) $(DESTDIR)$(PREFIX)/bin/$(<)
            
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

install: install-doc

.PHONY: install-doc
install-doc: doc/shellcat.1
	install -D -m644 $(<) $(DESTDIR)$(PREFIX)/share/man/man1/$(notdir $(<))

clean: clean-doc

.PHONY: clean-doc
clean-doc:
	rm -f doc/*.1 doc/*.tmp

endif

# vim:ts=4 sw=4 noet
