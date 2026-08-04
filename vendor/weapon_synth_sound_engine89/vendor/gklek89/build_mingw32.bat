@echo off
if not exist build mkdir build
if not exist audio mkdir audio
gcc -std=c89 -pedantic -Wall -Wextra -O2 -Iinclude -c src\gklek89.c -o build\gklek89.o
if errorlevel 1 exit /b 1
ar rcs build\libgklek89.a build\gklek89.o
gcc -std=c89 -pedantic -Wall -Wextra -O2 -Iinclude -c demo\render_gklek89.c -o build\render_gklek89.o
gcc -std=c89 -pedantic -Wall -Wextra -O2 build\render_gklek89.o build\libgklek89.a -o build\render_gklek89.exe
gcc -std=c89 -pedantic -Wall -Wextra -O2 -Iinclude -c tests\smoke.c -o build\test_smoke.o
gcc -std=c89 -pedantic -Wall -Wextra -O2 build\test_smoke.o build\libgklek89.a -o build\test_smoke.exe
build\test_smoke.exe
if errorlevel 1 exit /b 1
build\render_gklek89.exe
