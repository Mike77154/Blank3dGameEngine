@echo off
gcc -std=c89 -pedantic -Wall -Wextra -Werror -O2 -Iinclude -c src\wsoundthermal89.c -o wsoundthermal89.o
ar rcs libwsoundthermal89.a wsoundthermal89.o
gcc -std=c89 -pedantic -Wall -Wextra -Werror -O2 -Iinclude tests\smoke.c libwsoundthermal89.a -o smoke.exe
