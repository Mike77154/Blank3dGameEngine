@echo off
gcc -std=c89 -pedantic -Wall -Wextra -Werror -O2 -Iinclude -c src\wsoundlistener89.c -o wsoundlistener89.o
ar rcs libwsoundlistener89.a wsoundlistener89.o
gcc -std=c89 -pedantic -Wall -Wextra -Werror -O2 -Iinclude tests\smoke.c libwsoundlistener89.a -o smoke.exe
