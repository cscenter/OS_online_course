CFLAGS := -Wall -Werror -g

.PHONY: all clean

all: example

example: switch.o main.o
	$(CC) -g $^ -o $@

switch.o: switch.S
	$(CC) $(CFLAGS) -c $< -o $@

main.o: main.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf *.o example
