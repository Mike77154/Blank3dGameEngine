@echo off
if not exist build mkdir build
gcc -std=gnu89 -pedantic -Wall -Wextra -Werror -O2 -Iinclude -c src\gmuzzlegas89.c -o build\gmuzzlegas89.o
ar rcs build\libgmuzzlegas89.a build\gmuzzlegas89.o
gcc -std=gnu89 -pedantic -Wall -Wextra -Werror -O2 -Iinclude tests\smoke.c build\gmuzzlegas89.o -o build\smoke.exe
build\smoke.exe
