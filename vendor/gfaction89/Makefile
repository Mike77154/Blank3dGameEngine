CC ?= gcc
CFLAGS ?= -std=c89 -Wall -Wextra -pedantic -Iinclude
SRC = src/gfaction89_core.c src/gfaction89_ini.c src/gfaction89_matrix.c src/gfaction89_tags.c src/gfaction89_query.c src/gfaction89_bridge.c src/gfaction89_debug.c

all: demo_zombie_rampage demo_faction_war

demo_zombie_rampage: $(SRC) demo/demo_zombie_rampage.c
	$(CC) $(CFLAGS) $(SRC) demo/demo_zombie_rampage.c -o demo_zombie_rampage

demo_faction_war: $(SRC) demo/demo_faction_war.c
	$(CC) $(CFLAGS) $(SRC) demo/demo_faction_war.c -o demo_faction_war

clean:
	rm -f demo_zombie_rampage demo_faction_war *.exe
