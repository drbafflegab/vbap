CC=cc
CFLAGS=-std=c17

example: drb-vbap.h drb-vbap.c example.c
	$(CC) $(CFLAGS) drb-vbap.c example.c -o example

tests: drb-vbap.h drb-vbap.c tests.c test-sanity.c
	$(CC) $(CFLAGS) drb-vbap.c tests.c test-sanity.c -o tests

.PHONY: all

all: example tests

.PHONY: clean

clean:
	rm example tests
