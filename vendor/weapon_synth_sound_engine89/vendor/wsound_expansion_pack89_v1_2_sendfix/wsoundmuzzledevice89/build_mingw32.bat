@echo off
gcc -std=c89 -pedantic -Wall -Wextra -Werror -O2 -Iinclude -c src\wsoundmuzzledevice89.c -o wsoundmuzzledevice89.o
ar rcs libwsoundmuzzledevice89.a wsoundmuzzledevice89.o
gcc -std=c89 -pedantic -Wall -Wextra -Werror -O2 -Iinclude tests\smoke.c libwsoundmuzzledevice89.a -o smoke.exe
