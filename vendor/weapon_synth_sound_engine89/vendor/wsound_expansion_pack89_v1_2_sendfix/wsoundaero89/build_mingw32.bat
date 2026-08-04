@echo off
gcc -std=c89 -pedantic -Wall -Wextra -Werror -O2 -Iinclude -c src\wsoundaero89.c -o wsoundaero89.o
ar rcs libwsoundaero89.a wsoundaero89.o
gcc -std=c89 -pedantic -Wall -Wextra -Werror -O2 -Iinclude tests\smoke.c libwsoundaero89.a -o smoke.exe
