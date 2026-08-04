@echo off
if not exist build mkdir build
gcc -std=gnu89 -pedantic -Wall -Wextra -Werror -O2 -Iinclude -c src\gweaponbody89.c -o build\gweaponbody89.o
ar rcs build\libgweaponbody89.a build\gweaponbody89.o
gcc -std=gnu89 -pedantic -Wall -Wextra -Werror -O2 -Iinclude tests\smoke.c build\gweaponbody89.o -o build\smoke.exe
build\smoke.exe
