@echo off
if not exist build mkdir build
gcc -std=c89 -pedantic -Wall -Wextra -Werror -O2 -Iinclude -c src\wsoundroom89.c -o build\wsoundroom89.o || exit /b 1
ar rcs libwsoundroom89.a build\wsoundroom89.o || exit /b 1
gcc -std=c89 -pedantic -Wall -Wextra -Werror -O2 -Iinclude tests\smoke.c libwsoundroom89.a -o smoke.exe || exit /b 1
smoke.exe
