@echo off
setlocal
if "%CC%"=="" set CC=gcc
set CFLAGS=-std=c89 -pedantic -Wall -Wextra -Werror -Iinclude
%CC% %CFLAGS% src\mount89.c tests\test_mount89.c -o test_mount89.exe
if errorlevel 1 exit /b 1
%CC% %CFLAGS% src\mount89.c demo\demo_mount89.c -o demo_mount89.exe
if errorlevel 1 exit /b 1
test_mount89.exe
if errorlevel 1 exit /b 1
echo Build OK
endlocal
