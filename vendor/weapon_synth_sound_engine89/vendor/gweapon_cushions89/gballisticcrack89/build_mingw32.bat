@echo off
if not exist build mkdir build
gcc -std=gnu89 -pedantic -Wall -Wextra -Werror -O2 -Iinclude -c src\gballisticcrack89.c -o build\gballisticcrack89.o
ar rcs build\libgballisticcrack89.a build\gballisticcrack89.o
gcc -std=gnu89 -pedantic -Wall -Wextra -Werror -O2 -Iinclude tests\smoke.c build\gballisticcrack89.o -o build\smoke.exe
build\smoke.exe
