LD ?= ld
CC ?= gcc

CFLAGS = -ffreestanding -m32
AFLAGS = -ffreestanding -m32

all: boot.bin

OBJS = boot.o

boot.bin: $(OBJS) linker.ld
	$(LD) -T linker.ld $(OBJS) -o boot.bin

%.o: %.S
	$(CC) $(AFLAGS) -c $< -o $@

clean:
	rm *.o boot.bin

.PHONY: clean
