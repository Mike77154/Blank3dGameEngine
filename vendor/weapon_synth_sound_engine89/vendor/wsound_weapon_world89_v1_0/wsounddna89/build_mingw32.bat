@echo off
if not exist build mkdir build
gcc -std=c89 -pedantic -Wall -Wextra -Werror -O2 -Iinclude -c src\wsounddna89.c -o build\wsounddna89.o || exit /b 1
ar rcs libwsounddna89.a build\wsounddna89.o || exit /b 1
gcc -std=c89 -pedantic -Wall -Wextra -Werror -O2 -Iinclude tests\smoke.c libwsounddna89.a -o smoke.exe || exit /b 1
smoke.exe
