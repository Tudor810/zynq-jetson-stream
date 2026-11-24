# One Makefile for both sides of the project.
#
#   make          -> builds both programs (laptop)
#   make zynq     -> build/zynq_client   (run this on the Zybo)
#   make jetson   -> build/jetson_server (run this on the Jetson / laptop)
#   make clean    -> deletes build/

CC     ?= gcc
CFLAGS ?= -std=c11 -O2 -Wall -Wextra -D_DEFAULT_SOURCE
# -Isrc lets you write #include "common/netio.h" from any file
CFLAGS += -Isrc

BUILD  := build

# Code used by both programs
COMMON := src/common/errors.c src/common/netio.c

# Each side's own files
ZYNQ   := src/zynq/main.c src/zynq/fileio.c src/zynq/image.c
JETSON := src/jetson/server.c

.PHONY: all zynq jetson clean

all: zynq jetson

zynq: $(BUILD)/zynq_client
jetson: $(BUILD)/jetson_server

$(BUILD)/zynq_client: $(ZYNQ) $(COMMON) | $(BUILD)
	$(CC) $(CFLAGS) -o $@ $(ZYNQ) $(COMMON)

$(BUILD)/jetson_server: $(JETSON) $(COMMON) | $(BUILD)
	$(CC) $(CFLAGS) -o $@ $(JETSON) $(COMMON)

$(BUILD):
	mkdir -p $(BUILD)

clean:
	rm -rf $(BUILD)