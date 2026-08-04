@echo off
if not exist build mkdir build
gcc -std=c89 -pedantic -Wall -Wextra -Werror -O2 -Iinclude -c src\wsoundcombatbus89.c -o build\wsoundcombatbus89.o || exit /b 1
ar rcs libwsoundcombatbus89.a build\wsoundcombatbus89.o || exit /b 1
gcc -std=c89 -pedantic -Wall -Wextra -Werror -O2 -Iinclude tests\smoke.c libwsoundcombatbus89.a -o smoke.exe || exit /b 1
smoke.exe
