@echo off
setlocal
if not exist build mkdir build

gcc -O2 -std=c89 -pedantic -Wall -Wextra -Iinclude -c src\gk3d.c -o build\gk3d.o
if errorlevel 1 goto :fail

ar rcs build\libgk3d.a build\gk3d.o
if errorlevel 1 goto :fail

gcc -O2 -std=c89 -pedantic -Wall -Wextra -Iinclude tests\test_gk3d.c build\libgk3d.a -o build\test_gk3d.exe
if errorlevel 1 goto :fail

build\test_gk3d.exe
if errorlevel 1 goto :fail

echo 3DKin-GF build and tests passed.
exit /b 0

:fail
echo 3DKin-GF build failed.
exit /b 1
