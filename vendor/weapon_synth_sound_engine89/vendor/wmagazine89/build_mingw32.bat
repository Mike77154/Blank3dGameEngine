@echo off
setlocal
if not exist previews mkdir previews

gcc -O2 -std=c89 -pedantic -Wall -Wextra -Werror ^
  -Iinclude src\wmagazine89.c demo\wmagazine89_demo.c ^
  -o wmagazine89_demo.exe
if errorlevel 1 exit /b 1

gcc -O2 -std=c89 -pedantic -Wall -Wextra -Werror ^
  -Iinclude src\wmagazine89.c tests\smoke_test.c ^
  -o smoke_test.exe
if errorlevel 1 exit /b 1

smoke_test.exe
if errorlevel 1 exit /b 1

wmagazine89_demo.exe
if errorlevel 1 exit /b 1

echo wmagazine89 build and previews completed.
endlocal
