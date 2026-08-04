@echo off
if not exist build mkdir build
gcc -std=gnu89 -pedantic -Wall -Wextra -Werror -O2 -Iinclude -c src\gcinemathump89.c -o build\gcinemathump89.o
ar rcs build\libgcinemathump89.a build\gcinemathump89.o
gcc -std=gnu89 -pedantic -Wall -Wextra -Werror -O2 -Iinclude tests\smoke.c build\gcinemathump89.o -o build\smoke.exe
build\smoke.exe
