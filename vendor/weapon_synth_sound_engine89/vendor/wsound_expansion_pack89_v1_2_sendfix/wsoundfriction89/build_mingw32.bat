@echo off
gcc -std=c89 -pedantic -Wall -Wextra -Werror -O2 -Iinclude -c src\wsoundfriction89.c -o wsoundfriction89.o
ar rcs libwsoundfriction89.a wsoundfriction89.o
gcc -std=c89 -pedantic -Wall -Wextra -Werror -O2 -Iinclude tests\smoke.c libwsoundfriction89.a -o smoke.exe
