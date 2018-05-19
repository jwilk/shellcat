.PHONY: all
all: ../shellcat
	./run-tests $(<)

../shellcat:
	$(MAKE) -C ..

.PHONY: clean
clean:
	rm -rf *.err *.out *.report *.tmp

# vim:ts=4 sts=4 sw=4 noet
