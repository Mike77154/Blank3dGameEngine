@echo off
if not exist build mkdir build
gcc -std=c89 -pedantic -Wall -Wextra -Werror -O2 -Iinclude -c src\wsoundaction89.c -o build\wsoundaction89.o || exit /b 1
ar rcs libwsoundaction89.a build\wsoundaction89.o || exit /b 1
gcc -std=c89 -pedantic -Wall -Wextra -Werror -O2 -Iinclude tests\smoke.c libwsoundaction89.a -o smoke.exe || exit /b 1
smoke.exe
