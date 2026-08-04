@echo off
setlocal
if not exist build mkdir build
if not exist previews_real_reference mkdir previews_real_reference
if not exist variants_real_reference mkdir variants_real_reference
if not exist speeds_real_reference mkdir speeds_real_reference
if not exist stems_real_reference mkdir stems_real_reference

gcc -std=c89 -pedantic -Wall -Wextra -Werror -O2 -Iinclude -c src\gweaponfoley89.c -o build\gweaponfoley89.o
if errorlevel 1 exit /b 1

ar rcs build\libgweaponfoley89.a build\gweaponfoley89.o
if errorlevel 1 exit /b 1

gcc -std=c89 -pedantic -Wall -Wextra -Werror -O2 -Iinclude tools\render_previews.c build\libgweaponfoley89.a -o build\render_previews.exe
if errorlevel 1 exit /b 1

build\render_previews.exe
if errorlevel 1 exit /b 1

echo Build and previews completed.
endlocal
