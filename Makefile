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
COMMON := src/common/errors.c src/common/netio.c src/common/protocol.c src/common/image.c src/common/fileio.c src/common/timing.c 
HEADERS := $(wildcard src/common/*.h)

# Each side's own files
ZYNQ   := src/zynq/client.c 
JETSON := src/jetson/server.c src/jetson/filters_cpu.c

.PHONY: all zynq jetson clean

all: zynq jetson

zynq: $(BUILD)/zynq_client
jetson: $(BUILD)/jetson_server

$(BUILD)/zynq_client: $(ZYNQ) $(COMMON) $(HEADERS) | $(BUILD)
	$(CC) $(CFLAGS) -o $@ $(ZYNQ) $(COMMON) $(HEADERS)

$(BUILD)/jetson_server: $(JETSON) $(COMMON) $(HEADERS) | $(BUILD)
	$(CC) $(CFLAGS) -o $@ $(JETSON) $(COMMON) $(HEADERS)

$(BUILD):
	mkdir -p $(BUILD)

clean:
	rm -rf $(BUILD)