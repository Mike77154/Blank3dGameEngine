@echo off
setlocal
if not exist preview_frames mkdir preview_frames

gcc -std=c89 -pedantic -Wall -Wextra -Werror -O2 ^
    fmuzzle89.c demo_render.c -o demo_render.exe
if errorlevel 1 exit /b 1

gcc -std=c89 -pedantic -Wall -Wextra -Werror -O2 ^
    fmuzzle89.c test_fmuzzle89.c -o test_fmuzzle89.exe
if errorlevel 1 exit /b 1

test_fmuzzle89.exe
if errorlevel 1 exit /b 1

demo_render.exe
if errorlevel 1 exit /b 1

echo Build, tests and preview completed.
endlocal
