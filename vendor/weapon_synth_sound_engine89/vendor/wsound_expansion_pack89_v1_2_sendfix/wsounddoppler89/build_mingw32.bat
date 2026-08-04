@echo off
gcc -std=c89 -pedantic -Wall -Wextra -Werror -O2 -Iinclude -c src\wsounddoppler89.c -o wsounddoppler89.o
ar rcs libwsounddoppler89.a wsounddoppler89.o
gcc -std=c89 -pedantic -Wall -Wextra -Werror -O2 -Iinclude tests\smoke.c libwsounddoppler89.a -o smoke.exe
