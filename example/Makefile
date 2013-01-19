sources = $(wildcard [a-z]*.sc)
pages = $(sources:.sc=.html)
shellcat = $(shell which shell || echo ../shellcat)

.PHONY: all
all: $(pages)

%.html: %.sc $(shellcat)
	$(shellcat) _template.sc $(<) > $(@).tmp
	mv $(@).tmp $(@)

.PHONY: clean
clean:
	rm -f *.html *.tmp

# vim:ts=4 sw=4 noet
