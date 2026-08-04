CC=gcc
CFLAGS=-std=c89 -pedantic -Wall -Wextra -Iinclude

all: rocketmeshes_example rocketmeshes_dump_obj

rocketmeshes_example: examples/rocketmeshes_example.c src/rocketmeshes.c include/rocketmeshes.h
	$(CC) $(CFLAGS) examples/rocketmeshes_example.c src/rocketmeshes.c -o rocketmeshes_example

rocketmeshes_dump_obj: tools/rocketmeshes_dump_obj.c src/rocketmeshes.c include/rocketmeshes.h
	$(CC) $(CFLAGS) tools/rocketmeshes_dump_obj.c src/rocketmeshes.c -o rocketmeshes_dump_obj

obj: rocketmeshes_dump_obj
	./rocketmeshes_dump_obj 0 obj/mesh_0.obj
	./rocketmeshes_dump_obj 1 obj/mesh_1.obj
	./rocketmeshes_dump_obj 2 obj/mesh_2.obj
	./rocketmeshes_dump_obj 3 obj/mesh_3.obj
	./rocketmeshes_dump_obj 4 obj/mesh_4.obj
	./rocketmeshes_dump_obj 5 obj/mesh_5.obj
	./rocketmeshes_dump_obj 6 obj/mesh_6.obj
	./rocketmeshes_dump_obj 7 obj/mesh_7.obj
	./rocketmeshes_dump_obj 8 obj/mesh_8.obj
	./rocketmeshes_dump_obj 9 obj/mesh_9.obj
	./rocketmeshes_dump_obj 10 obj/mesh_10.obj
	./rocketmeshes_dump_obj 11 obj/mesh_11.obj
	./rocketmeshes_dump_obj 12 obj/mesh_12.obj
	./rocketmeshes_dump_obj 13 obj/mesh_13.obj
	./rocketmeshes_dump_obj 14 obj/mesh_14.obj
	./rocketmeshes_dump_obj 15 obj/mesh_15.obj
	./rocketmeshes_dump_obj 16 obj/mesh_16.obj
	./rocketmeshes_dump_obj 17 obj/mesh_17.obj

clean:
	rm -f rocketmeshes_example rocketmeshes_dump_obj obj/*.obj
