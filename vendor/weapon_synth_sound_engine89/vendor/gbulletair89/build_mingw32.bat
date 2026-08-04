@echo off
set CFLAGS=-std=c89 -pedantic -Wall -Wextra -Werror -O2 -Iinclude
gcc %CFLAGS% src\gbulletair89.c demo\demo_wav.c -o demo_wav.exe
if errorlevel 1 exit /b 1
gcc %CFLAGS% src\gbulletair89.c tests\test_c89.c -o test_c89.exe
if errorlevel 1 exit /b 1
cd preview
..\demo_wav.exe
cd ..
echo Build and previews complete.
