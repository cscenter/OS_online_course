CXX := g++
CFLAGS := -Wextra -Wall -Werror -pedantic -std=c++11
LFLAGS := -pthread


default: example

example: example.o
	$(CXX) $(LFLAGS) $< -o $@

example.o: example.cpp
	$(CXX) $(CFLAGS) $< -c -o $@

.PHONY: clean, exmaple
clean:
	rm *.o example
