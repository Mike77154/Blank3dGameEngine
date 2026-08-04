CC ?= gcc
AR ?= ar
CFLAGS ?= -std=c89 -pedantic -Wall -Wextra -O2 -Iinclude

all: libgweaponfoley89.a example.o

src/gweaponfoley89.o: src/gweaponfoley89.c include/gweaponfoley89.h
	$(CC) $(CFLAGS) -c src/gweaponfoley89.c -o src/gweaponfoley89.o

libgweaponfoley89.a: src/gweaponfoley89.o
	$(AR) rcs libgweaponfoley89.a src/gweaponfoley89.o

example.o: examples/engine_pull_example.c include/gweaponfoley89.h
	$(CC) $(CFLAGS) -c examples/engine_pull_example.c -o example.o

render_previews: libgweaponfoley89.a tools/render_previews.c
	mkdir -p previews_real_reference variants_real_reference speeds_real_reference stems_real_reference
	$(CC) $(CFLAGS) tools/render_previews.c libgweaponfoley89.a -o render_previews
	./render_previews

clean:
	rm -f src/gweaponfoley89.o example.o libgweaponfoley89.a render_previews
