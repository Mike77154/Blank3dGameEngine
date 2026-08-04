CC=gcc
CFLAGS=-std=c89 -pedantic -Wall -Wextra -Isrc

all: demo_bulletmesh89

demo_bulletmesh89: src/gbulletmesh89.c src/gbulletmesh89.h demo/demo_bulletmesh89.c
	$(CC) $(CFLAGS) src/gbulletmesh89.c demo/demo_bulletmesh89.c -o demo_bulletmesh89

run: demo_bulletmesh89
	./demo_bulletmesh89

clean:
	rm -f demo_bulletmesh89
	rm -f obj/*.obj
