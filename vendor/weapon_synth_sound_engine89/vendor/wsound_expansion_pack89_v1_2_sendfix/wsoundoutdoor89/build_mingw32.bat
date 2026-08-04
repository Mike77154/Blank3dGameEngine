@echo off
gcc -std=c89 -pedantic -Wall -Wextra -Werror -O2 -Iinclude -c src\wsoundoutdoor89.c -o wsoundoutdoor89.o
ar rcs libwsoundoutdoor89.a wsoundoutdoor89.o
gcc -std=c89 -pedantic -Wall -Wextra -Werror -O2 -Iinclude tests\smoke.c libwsoundoutdoor89.a -o smoke.exe
