@echo off
gcc -std=c89 -pedantic -Wall -Wextra -Werror -O2 -Iinclude -c src\wsoundspatial89.c -o wsoundspatial89.o
ar rcs libwsoundspatial89.a wsoundspatial89.o
gcc -std=c89 -pedantic -Wall -Wextra -Werror -O2 -Iinclude tests\smoke.c libwsoundspatial89.a -o smoke.exe
