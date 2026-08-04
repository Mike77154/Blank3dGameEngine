@echo off
set CFLAGS=-std=c89 -pedantic -Wall -Wextra -O2
set INCLUDES=-Iinclude -Ivendor\gpaah89\include -Ivendor\gpump89\include -Ivendor\chuecka89\include -Ivendor\gklek89\include -Ivendor\gweaponfoley89\include -Ivendor\gweaponbody89\include -Ivendor\gmuzzlegas89\include -Ivendor\gballisticcrack89\include -Ivendor\glatetail89\include -Ivendor\gcinemathump89\include
if not exist build mkdir build
if not exist audio mkdir audio

gcc %CFLAGS% %INCLUDES% -c src\gshotgunsequence89.c -o build\gshotgunsequence89.o || exit /b 1
gcc %CFLAGS% %INCLUDES% -c vendor\gpaah89\src\gpaah89.c -o build\gpaah89.o || exit /b 1
gcc %CFLAGS% %INCLUDES% -c vendor\gpump89\src\gpump89.c -o build\gpump89.o || exit /b 1
gcc %CFLAGS% %INCLUDES% -c vendor\chuecka89\src\chuecka89.c -o build\chuecka89.o || exit /b 1
gcc %CFLAGS% %INCLUDES% -c vendor\gklek89\src\gklek89.c -o build\gklek89.o || exit /b 1
gcc %CFLAGS% %INCLUDES% -c vendor\gweaponfoley89\src\gweaponfoley89.c -o build\gweaponfoley89.o || exit /b 1
gcc %CFLAGS% %INCLUDES% -c vendor\gweaponbody89\src\gweaponbody89.c -o build\gweaponbody89.o || exit /b 1
gcc %CFLAGS% %INCLUDES% -c vendor\gmuzzlegas89\src\gmuzzlegas89.c -o build\gmuzzlegas89.o || exit /b 1
gcc %CFLAGS% %INCLUDES% -c vendor\gballisticcrack89\src\gballisticcrack89.c -o build\gballisticcrack89.o || exit /b 1
gcc %CFLAGS% %INCLUDES% -c vendor\glatetail89\src\glatetail89.c -o build\glatetail89.o || exit /b 1
gcc %CFLAGS% %INCLUDES% -c vendor\gcinemathump89\src\gcinemathump89.c -o build\gcinemathump89.o || exit /b 1

ar rcs build\libgshotgunsequence89.a build\gshotgunsequence89.o build\gpaah89.o build\gpump89.o build\chuecka89.o build\gklek89.o build\gweaponfoley89.o build\gweaponbody89.o build\gmuzzlegas89.o build\gballisticcrack89.o build\glatetail89.o build\gcinemathump89.o || exit /b 1

gcc %CFLAGS% %INCLUDES% -c demo\render_shotgun_sequence.c -o build\render_shotgun_sequence.o || exit /b 1
gcc %CFLAGS% build\render_shotgun_sequence.o build\libgshotgunsequence89.a -o build\render_shotgun_sequence.exe || exit /b 1
gcc %CFLAGS% %INCLUDES% -c tests\smoke.c -o build\test_smoke.o || exit /b 1
gcc %CFLAGS% build\test_smoke.o build\libgshotgunsequence89.a -o build\test_smoke.exe || exit /b 1

build\test_smoke.exe || exit /b 1
build\render_shotgun_sequence.exe || exit /b 1
