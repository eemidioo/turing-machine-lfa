CC := gcc

CFLAGS := -std=c11 -Wall -Wextra -Wpedantic
DEBUG_FLAGS := -g3 -O0
RELEASE_FLAGS := -O2 -DNDEBUG

BUILD_DIR := build
BIN := $(BUILD_DIR)/tm

SRC := main.c src/tm_lexer.c src/tm_parser.c
HDR := src/tm_lexer.h src/tm_parser.h

.PHONY: all debug release run clean

all: $(BIN)

$(BIN): $(SRC) $(HDR)
	mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $(EXTRA_FLAGS) -Isrc $(SRC) -o $(BIN)

debug:
	$(MAKE) EXTRA_FLAGS="$(DEBUG_FLAGS)" all

release:
	$(MAKE) EXTRA_FLAGS="$(RELEASE_FLAGS)" all

run: all
	./$(BIN) $(ARGS) examples/divisivel_por_3.tmt

clean:
	rm -rf $(BUILD_DIR)

