@echo off
gcc -std=c89 -pedantic -Wall -Wextra -Werror -O2 -Iinclude -c src\wsoundportal89.c -o wsoundportal89.o
ar rcs libwsoundportal89.a wsoundportal89.o
gcc -std=c89 -pedantic -Wall -Wextra -Werror -O2 -Iinclude tests\smoke.c libwsoundportal89.a -o smoke.exe
