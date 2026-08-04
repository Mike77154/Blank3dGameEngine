CC ?= gcc
AR ?= ar
CFLAGS ?= -std=c89 -pedantic -Wall -Wextra -Iinclude

all: libgsniperhud89.a

gsniperhud89.o: gsniperhud89/src/gsniperhud89.c gsniperhud89/include/gsniperhud89.h include/gscope89_common.h
	$(CC) $(CFLAGS) -c gsniperhud89/src/gsniperhud89.c -o $@

libgsniperhud89.a: gsniperhud89.o
	$(AR) rcs $@ $<

clean:
	rm -f gsniperhud89.o libgsniperhud89.a
