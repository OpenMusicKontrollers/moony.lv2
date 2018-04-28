CC ?= gcc
C_FLAGS ?= -I../ -Wall -Wextra -Wpedantic $(shell pkg-config --cflags lv2) \
					 -fprofile-arcs -ftest-coverage

all: osc_test

osc_test: osc_test.c ../osc.lv2/*.h
	$(CC) -std=gnu11 -g -o $@ $< $(C_FLAGS) -lpthread

check: osc_test
	./$<
	gcov $<.c

clean:
	rm -f osc_test *.gcov *.gc* vgcore.*
