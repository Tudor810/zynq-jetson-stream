# make        -> build/zynq_client
# make clean
CC     ?= gcc
CFLAGS ?= -std=c11 -O2 -Wall -Wextra -D_DEFAULT_SOURCE

SRC := src/zynq/main.c src/zynq/fileio.c src/common/errors.c

build/zynq_client: $(SRC)
	mkdir -p build
	$(CC) $(CFLAGS) -o $@ $(SRC)

.PHONY: clean
clean:
	rm -rf build
