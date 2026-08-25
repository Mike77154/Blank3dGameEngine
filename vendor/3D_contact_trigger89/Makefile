CC ?= cc
AR ?= ar
CFLAGS ?= -std=c89 -Wall -Wextra -Werror -pedantic-errors
CPPFLAGS ?= -Iinclude

LIB = lib3D_contact_trigger89.a
OBJ = src/3d_contact_trigger89.o
TEST = tests/test_3d_contact_trigger89

.PHONY: all test clean audit

all: $(LIB)

$(LIB): $(OBJ)
	$(AR) rcs $@ $(OBJ)

src/3d_contact_trigger89.o: src/3d_contact_trigger89.c include/3d_contact_trigger89.h
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

test: $(TEST)
	./$(TEST)

$(TEST): tests/test_3d_contact_trigger89.c src/3d_contact_trigger89.c include/3d_contact_trigger89.h
	$(CC) $(CFLAGS) $(CPPFLAGS) tests/test_3d_contact_trigger89.c src/3d_contact_trigger89.c -o $(TEST)

audit:
	@! grep -n -E '(^|[^[:alnum:]_])(malloc|realloc|free)[[:space:]]*\(' include/*.h src/*.c tests/*.c
	@! grep -n -E '\b(float|double)\b' include/*.h src/*.c tests/*.c
	@echo "3D_contact_trigger89 audit: no allocator API and no float/double tokens"

clean:
	rm -f $(OBJ) $(LIB) $(TEST)
