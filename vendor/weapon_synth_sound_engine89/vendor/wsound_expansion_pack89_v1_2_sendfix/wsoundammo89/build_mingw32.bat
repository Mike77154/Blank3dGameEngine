@echo off
gcc -std=c89 -pedantic -Wall -Wextra -Werror -O2 -Iinclude -c src\wsoundammo89.c -o wsoundammo89.o
ar rcs libwsoundammo89.a wsoundammo89.o
gcc -std=c89 -pedantic -Wall -Wextra -Werror -O2 -Iinclude tests\smoke.c libwsoundammo89.a -o smoke.exe
