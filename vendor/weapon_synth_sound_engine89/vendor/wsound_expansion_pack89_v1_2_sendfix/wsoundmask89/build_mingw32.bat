@echo off
gcc -std=c89 -pedantic -Wall -Wextra -Werror -O2 -Iinclude -c src\wsoundmask89.c -o wsoundmask89.o
ar rcs libwsoundmask89.a wsoundmask89.o
gcc -std=c89 -pedantic -Wall -Wextra -Werror -O2 -Iinclude tests\smoke.c libwsoundmask89.a -o smoke.exe
