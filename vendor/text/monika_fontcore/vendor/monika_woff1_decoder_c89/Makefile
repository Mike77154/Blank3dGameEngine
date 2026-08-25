CC ?= cc
CFLAGS ?= -std=c89 -pedantic -Wall -Wextra -O2
LDFLAGS ?=

SRC = src/woff1_decoder.c src/woff1_inflate.c src/woff1_cli.c
OBJ = $(SRC:.c=.o)
BIN = woff1dec

all: $(BIN)

$(BIN): $(OBJ)
	$(CC) $(CFLAGS) -o $(BIN) $(OBJ) $(LDFLAGS)

clean:
	rm -f $(OBJ) $(BIN) tests/test_inflate tests/test_woff out.ttf out.otf

test_inflate: src/woff1_inflate.c tests/test_inflate.c
	$(CC) $(CFLAGS) -Isrc -o tests/test_inflate src/woff1_inflate.c tests/test_inflate.c
	./tests/test_inflate

.PHONY: all clean test_inflate
