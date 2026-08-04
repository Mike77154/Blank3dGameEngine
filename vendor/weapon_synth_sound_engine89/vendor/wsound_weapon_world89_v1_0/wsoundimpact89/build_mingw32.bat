@echo off
if not exist build mkdir build
gcc -std=c89 -pedantic -Wall -Wextra -Werror -O2 -Iinclude -c src\wsoundimpact89.c -o build\wsoundimpact89.o || exit /b 1
ar rcs libwsoundimpact89.a build\wsoundimpact89.o || exit /b 1
gcc -std=c89 -pedantic -Wall -Wextra -Werror -O2 -Iinclude tests\smoke.c libwsoundimpact89.a -o smoke.exe || exit /b 1
smoke.exe
