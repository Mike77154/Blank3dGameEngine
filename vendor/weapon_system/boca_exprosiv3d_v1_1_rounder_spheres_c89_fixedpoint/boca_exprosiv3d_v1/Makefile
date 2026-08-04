CC = gcc
AR = ar
CFLAGS = -std=c89 -pedantic -Wall -Wextra -Werror -O2

all: libboca_exprosiv3d.a test_boca_exprosiv3d demo_export mesh_budget

boca_exprosiv3d.o: boca_exprosiv3d.c boca_exprosiv3d.h
	$(CC) $(CFLAGS) -c boca_exprosiv3d.c -o boca_exprosiv3d.o

libboca_exprosiv3d.a: boca_exprosiv3d.o
	$(AR) rcs libboca_exprosiv3d.a boca_exprosiv3d.o

test_boca_exprosiv3d: boca_exprosiv3d.c boca_exprosiv3d.h test_boca_exprosiv3d.c
	$(CC) $(CFLAGS) boca_exprosiv3d.c test_boca_exprosiv3d.c -o test_boca_exprosiv3d

demo_export: boca_exprosiv3d.c boca_exprosiv3d.h demo_export.c
	$(CC) $(CFLAGS) boca_exprosiv3d.c demo_export.c -o demo_export

mesh_budget: boca_exprosiv3d.c boca_exprosiv3d.h mesh_budget.c
	$(CC) $(CFLAGS) boca_exprosiv3d.c mesh_budget.c -o mesh_budget

test: test_boca_exprosiv3d
	./test_boca_exprosiv3d

samples: demo_export
	mkdir -p samples
	./demo_export

budget: mesh_budget
	./mesh_budget

clean:
	rm -f boca_exprosiv3d.o libboca_exprosiv3d.a
	rm -f test_boca_exprosiv3d demo_export mesh_budget
	rm -f samples/*.obj
