@echo off
if not exist build mkdir build
gcc -std=gnu89 -pedantic -Wall -Wextra -Werror -O2 -Iinclude -c src\glatetail89.c -o build\glatetail89.o
ar rcs build\libglatetail89.a build\glatetail89.o
gcc -std=gnu89 -pedantic -Wall -Wextra -Werror -O2 -Iinclude tests\smoke.c build\glatetail89.o -o build\smoke.exe
build\smoke.exe
