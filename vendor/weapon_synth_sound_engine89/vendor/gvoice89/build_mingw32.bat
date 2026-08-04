@echo off
setlocal
if not exist build mkdir build
set CFLAGS=-O2 -std=c89 -pedantic -Wall -Wextra -Werror -Iinclude
gcc %CFLAGS% -c src\gvoice89.c -o build\gvoice89.o || exit /b 1
ar rcs build\libgvoice89.a build\gvoice89.o || exit /b 1
gcc %CFLAGS% tests\smoke.c build\libgvoice89.a -o build\test_smoke.exe || exit /b 1
gcc %CFLAGS% demo\virtual_256_demo.c build\libgvoice89.a -o build\virtual_256_demo.exe || exit /b 1
build\test_smoke.exe || exit /b 1
endlocal
