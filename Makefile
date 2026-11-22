CC       ?= cc
CFLAGS   ?= -std=c11 -Wall -Wextra
INCLUDES := -Isrc

BIN := build/tm
SRC := src/tm_lexer.c
HDR := src/tm_lexer.h

.PHONY: all run example clean

all: $(BIN)

$(BIN): $(SRC) $(HDR)
	mkdir -p build
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $(SRC)

run: $(BIN)
	./$(BIN)

example: $(BIN)
	./$(BIN) examples/divisivel_por_3.tmt

clean:
	rm -rf build
