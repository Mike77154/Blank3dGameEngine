@echo off
if not exist build mkdir build
gcc -std=c89 -pedantic -Wall -Wextra -Werror -O2 -Iinclude -c src\wsoundricochet89.c -o build\wsoundricochet89.o || exit /b 1
ar rcs libwsoundricochet89.a build\wsoundricochet89.o || exit /b 1
gcc -std=c89 -pedantic -Wall -Wextra -Werror -O2 -Iinclude tests\smoke.c libwsoundricochet89.a -o smoke.exe || exit /b 1
smoke.exe
