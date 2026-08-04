CC ?= gcc
CFLAGS ?= -std=c89 -Wall -Wextra -pedantic -Iinclude
SRC = src/grecoil89.c src/grecoil89_profiles.c src/grecoil89_bridge.c src/grecoil89_provider.c

all: demo_grecoil89 demo_bridge demo_provider grecoil89_curve_dump

demo_grecoil89: $(SRC) demo/demo_grecoil89.c
	$(CC) $(CFLAGS) $(SRC) demo/demo_grecoil89.c -o demo_grecoil89

demo_bridge: $(SRC) demo/demo_bridge.c
	$(CC) $(CFLAGS) $(SRC) demo/demo_bridge.c -o demo_bridge

demo_provider: $(SRC) demo/demo_provider.c
	$(CC) $(CFLAGS) $(SRC) demo/demo_provider.c -o demo_provider

grecoil89_curve_dump: $(SRC) tools/grecoil89_curve_dump.c
	$(CC) $(CFLAGS) $(SRC) tools/grecoil89_curve_dump.c -o grecoil89_curve_dump

clean:
	rm -f demo_grecoil89 demo_bridge demo_provider grecoil89_curve_dump *.exe *.o
