@echo off
if not exist build mkdir build
gcc -std=c89 -pedantic -Wall -Wextra -Werror -O2 -Iinclude -c src\wsoundprop89.c -o build\wsoundprop89.o || exit /b 1
ar rcs libwsoundprop89.a build\wsoundprop89.o || exit /b 1
gcc -std=c89 -pedantic -Wall -Wextra -Werror -O2 -Iinclude tests\smoke.c libwsoundprop89.a -o smoke.exe || exit /b 1
smoke.exe
