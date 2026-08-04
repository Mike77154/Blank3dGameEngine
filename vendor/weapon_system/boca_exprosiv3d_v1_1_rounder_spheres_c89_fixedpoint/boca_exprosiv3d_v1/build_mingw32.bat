@echo off
setlocal

set CFLAGS=-std=c89 -pedantic -Wall -Wextra -Werror -O2

gcc %CFLAGS% -c boca_exprosiv3d.c -o boca_exprosiv3d.o
if errorlevel 1 exit /b 1

ar rcs libboca_exprosiv3d.a boca_exprosiv3d.o
if errorlevel 1 exit /b 1

gcc %CFLAGS% boca_exprosiv3d.c test_boca_exprosiv3d.c -o test_boca_exprosiv3d.exe
if errorlevel 1 exit /b 1

gcc %CFLAGS% boca_exprosiv3d.c demo_export.c -o demo_export.exe
if errorlevel 1 exit /b 1

gcc %CFLAGS% boca_exprosiv3d.c mesh_budget.c -o mesh_budget.exe
if errorlevel 1 exit /b 1

if not exist samples mkdir samples

test_boca_exprosiv3d.exe
if errorlevel 1 exit /b 1

demo_export.exe
if errorlevel 1 exit /b 1

mesh_budget.exe
if errorlevel 1 exit /b 1

echo.
echo boca_exprosiv3d build completed successfully.
endlocal
