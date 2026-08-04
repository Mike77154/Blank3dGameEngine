@echo off
gcc -std=c89 -pedantic -Wall -Wextra -Werror -O2 -Iinclude -c src\wsoundbeltfeed89.c -o wsoundbeltfeed89.o
ar rcs libwsoundbeltfeed89.a wsoundbeltfeed89.o
gcc -std=c89 -pedantic -Wall -Wextra -Werror -O2 -Iinclude tests\smoke.c libwsoundbeltfeed89.a -o smoke.exe
