@echo off
setlocal
if not exist build mkdir build

gcc -std=c89 -pedantic -Wall -Wextra -Werror -O2 -Iinclude -Isrc -c src\nationalmecanicanimal89.c -o build\nationalmecanicanimal89.o
if errorlevel 1 exit /b 1

ar rcs build\libnationalmecanicanimal89.a build\nationalmecanicanimal89.o
if errorlevel 1 exit /b 1

gcc -std=c89 -pedantic -Wall -Wextra -Werror -O2 -Iinclude -Isrc build\nationalmecanicanimal89.o tests\test_nm89.c -o build\test_nm89.exe
if errorlevel 1 exit /b 1

gcc -std=c89 -pedantic -Wall -Wextra -Werror -O2 -Iinclude -Isrc build\nationalmecanicanimal89.o examples\pistol_provider_demo.c -o build\pistol_provider_demo.exe
if errorlevel 1 exit /b 1

build\test_nm89.exe
endlocal
