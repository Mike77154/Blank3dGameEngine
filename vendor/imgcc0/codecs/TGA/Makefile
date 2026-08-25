CC ?= cc
CFLAGS ?= -std=c89 -pedantic -Wall -Wextra -O2

all: selftest example_palette16 example_forensics example_extension example_truecolor16 example_canonicalize

selftest: gafnyf_tga.c gafnyf_tga.h selftest.c
	$(CC) $(CFLAGS) gafnyf_tga.c selftest.c -o $@

example_palette16: gafnyf_tga.c gafnyf_tga.h example_palette16.c
	$(CC) $(CFLAGS) gafnyf_tga.c example_palette16.c -o $@

example_forensics: gafnyf_tga.c gafnyf_tga.h example_forensics.c
	$(CC) $(CFLAGS) gafnyf_tga.c example_forensics.c -o $@

example_extension: gafnyf_tga.c gafnyf_tga.h example_extension.c
	$(CC) $(CFLAGS) gafnyf_tga.c example_extension.c -o $@

example_truecolor16: gafnyf_tga.c gafnyf_tga.h example_truecolor16.c
	$(CC) $(CFLAGS) gafnyf_tga.c example_truecolor16.c -o $@

example_canonicalize: gafnyf_tga.c gafnyf_tga.h example_canonicalize.c
	$(CC) $(CFLAGS) gafnyf_tga.c example_canonicalize.c -o $@

clean:
	rm -f selftest example_palette16 example_forensics example_extension example_truecolor16 example_canonicalize *.o
