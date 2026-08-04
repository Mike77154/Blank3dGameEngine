CC ?= gcc
AR ?= ar
CFLAGS ?= -std=c89 -pedantic -Wall -Wextra -O2 -Iinclude

CORE_SRC = src/gmove89_types.c src/gmove89_math.c
AUT_SRC = src/gautmove89.c
PATTERN_SRC = src/gmovepattern89.c
SEQUENCE_SRC = src/gmovesequence89.c

CORE_OBJ = $(CORE_SRC:.c=.o)
AUT_OBJ = $(AUT_SRC:.c=.o)
PATTERN_OBJ = $(PATTERN_SRC:.c=.o)
SEQUENCE_OBJ = $(SEQUENCE_SRC:.c=.o)
ALL_OBJ = $(CORE_OBJ) $(AUT_OBJ) $(PATTERN_OBJ) $(SEQUENCE_OBJ)

LIB_CORE = libgmove89_core.a
LIB_AUT = libgautmove89.a
LIB_PATTERN = libgmovepattern89.a
LIB_SEQUENCE = libgmovesequence89.a
LIB_BUNDLE = libgautomotion89.a

.PHONY: all clean test examples

all: $(LIB_CORE) $(LIB_AUT) $(LIB_PATTERN) $(LIB_SEQUENCE) $(LIB_BUNDLE)

$(LIB_CORE): $(CORE_OBJ)
	$(AR) rcs $(LIB_CORE) $(CORE_OBJ)

$(LIB_AUT): $(AUT_OBJ)
	$(AR) rcs $(LIB_AUT) $(AUT_OBJ)

$(LIB_PATTERN): $(PATTERN_OBJ)
	$(AR) rcs $(LIB_PATTERN) $(PATTERN_OBJ)

$(LIB_SEQUENCE): $(SEQUENCE_OBJ)
	$(AR) rcs $(LIB_SEQUENCE) $(SEQUENCE_OBJ)

$(LIB_BUNDLE): $(ALL_OBJ)
	$(AR) rcs $(LIB_BUNDLE) $(ALL_OBJ)

src/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

test: $(LIB_BUNDLE)
	$(CC) $(CFLAGS) tests/test_all.c $(LIB_BUNDLE) -o test_all
	./test_all

examples: $(LIB_BUNDLE)
	$(CC) $(CFLAGS) examples/demo_basic.c $(LIB_BUNDLE) -o demo_basic
	$(CC) $(CFLAGS) examples/demo_sequence.c $(LIB_BUNDLE) -o demo_sequence
	$(CC) $(CFLAGS) examples/demo_provider.c $(LIB_BUNDLE) -o demo_provider

clean:
	rm -f $(ALL_OBJ) $(LIB_CORE) $(LIB_AUT) $(LIB_PATTERN) \
		$(LIB_SEQUENCE) $(LIB_BUNDLE) test_all demo_basic demo_sequence demo_provider
