@echo off
setlocal
if not exist build mkdir build
set CFLAGS=-O2 -std=c89 -pedantic -Wall -Wextra -Werror -Iinclude -Ivendor\gvoice89\include
gcc %CFLAGS% -c vendor\gvoice89\src\gvoice89.c -o build\gvoice89.o || exit /b 1
gcc %CFLAGS% -c src\gweaponvoice89.c -o build\gweaponvoice89.o || exit /b 1
ar rcs build\libgweaponvoice89.a build\gvoice89.o build\gweaponvoice89.o || exit /b 1
gcc %CFLAGS% tests\smoke.c build\libgweaponvoice89.a -o build\test_smoke.exe || exit /b 1
gcc %CFLAGS% demo\render_256_voice_weapon_scene.c build\libgweaponvoice89.a -o build\stress_demo.exe || exit /b 1
build\test_smoke.exe || exit /b 1
build\stress_demo.exe audio\weapon_voice_256logical_64physical_stress.wav || exit /b 1
endlocal
