@echo off
if not exist build mkdir build
gcc -std=c89 -pedantic -Wall -Wextra -Werror -O2 -Iinclude -c src\wsoundprojectile89.c -o build\wsoundprojectile89.o || exit /b 1
ar rcs libwsoundprojectile89.a build\wsoundprojectile89.o || exit /b 1
gcc -std=c89 -pedantic -Wall -Wextra -Werror -O2 -Iinclude tests\smoke.c libwsoundprojectile89.a -o smoke.exe || exit /b 1
smoke.exe
