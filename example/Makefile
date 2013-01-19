sources = $(wildcard [a-z]*.sc)
pages = $(sources:.sc=.html)

.PHONY: all
all: $(pages)

%.html: %.sc
	shellcat _template.sc $(<) > $(@).tmp
	mv $(@).tmp $(@)

.PHONY: clean
clean:
	rm -f *.html *.tmp

# vim:ts=4 sw=4 noet
