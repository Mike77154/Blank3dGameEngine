@echo off
if not exist build mkdir build
gcc -std=c89 -pedantic -Wall -Wextra -Werror -O2 -Iinclude -c src\wsoundreceiver89.c -o build\wsoundreceiver89.o || exit /b 1
ar rcs libwsoundreceiver89.a build\wsoundreceiver89.o || exit /b 1
gcc -std=c89 -pedantic -Wall -Wextra -Werror -O2 -Iinclude tests\smoke.c libwsoundreceiver89.a -o smoke.exe || exit /b 1
smoke.exe
