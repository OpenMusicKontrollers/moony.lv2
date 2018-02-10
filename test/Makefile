CC ?= gcc
#C_FLAGS ?= -I../ -Wall -Wextra -Wpedantic $(shell pkg-config --cflags lv2) \
#					 -fprofile-arcs -ftest-coverage
C_FLAGS ?= -I../ $(shell pkg-config --cflags lv2) \
					 -fprofile-arcs -ftest-coverage

all: osc_test

osc_test: osc_test.c ../osc.lv2/*.h
	$(CC) -std=gnu11 -g -o $@ $< $(C_FLAGS) -lpthread
	./$@
	gcov $<

clean:
	rm -f osc_test *.gcov *.gc* vgcore.*
