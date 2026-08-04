@echo off
if not exist build mkdir build
gcc -std=c89 -pedantic -Wall -Wextra -Iinclude -c src\reactiveloader.c -o build\reactiveloader.o
ar rcs build\libreactiveloader.a build\reactiveloader.o
gcc -std=c89 -pedantic -Wall -Wextra -Iinclude demo\demo_reactiveloader.c build\libreactiveloader.a -o build\demo_reactiveloader.exe
build\demo_reactiveloader.exe
