CC ?= cc

default: libfoo.so call

libfoo.so: foo.c foo.h
	$(CC) -fpic -shared  $< -o $@

call: call.c foo.h
	$(CC) $< -L. -lfoo -o $@

.PHONY: clean, default
clean:
	rm libfoo.so call
