@echo off
setlocal

gcc -std=c89 -pedantic -Wall -Wextra -Werror -O2 -c grocketwhistle89.c -o grocketwhistle89.o
if errorlevel 1 exit /b 1

ar rcs libgrocketwhistle89.a grocketwhistle89.o
if errorlevel 1 exit /b 1

gcc -std=c89 -pedantic -Wall -Wextra -Werror -O2 grocketwhistle89.c demo_wav.c -o demo_wav.exe
if errorlevel 1 exit /b 1

gcc -std=c89 -pedantic -Wall -Wextra -Werror -O2 grocketwhistle89.c demo_ab.c -o demo_ab.exe
if errorlevel 1 exit /b 1

gcc -std=c89 -pedantic -Wall -Wextra -Werror -O2 grocketwhistle89.c test_api.c -o test_api.exe
if errorlevel 1 exit /b 1

test_api.exe
if errorlevel 1 exit /b 1

demo_wav.exe
if errorlevel 1 exit /b 1

demo_ab.exe
if errorlevel 1 exit /b 1

echo Build, tests and WAV previews completed.
endlocal
