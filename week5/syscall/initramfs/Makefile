CC ?= cc
CFLAGS := -ffreestanding -m64 -g
LFLAGS := -nostdlib -z max-page-size=0x1000 -static

default: test

test: test.S
	$(CC) $< $(CFLAGS) $(LFLAGS) -o $@

.PHONY: clean default
clean:
	rm test
