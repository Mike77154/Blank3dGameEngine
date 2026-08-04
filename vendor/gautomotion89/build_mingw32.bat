@echo off
setlocal

set CC=gcc
set CFLAGS=-std=c89 -pedantic -Wall -Wextra -O2 -Iinclude

%CC% %CFLAGS% -c src\gmove89_types.c -o gmove89_types.o || goto :error
%CC% %CFLAGS% -c src\gmove89_math.c -o gmove89_math.o || goto :error
%CC% %CFLAGS% -c src\gautmove89.c -o gautmove89.o || goto :error
%CC% %CFLAGS% -c src\gmovepattern89.c -o gmovepattern89.o || goto :error
%CC% %CFLAGS% -c src\gmovesequence89.c -o gmovesequence89.o || goto :error

ar rcs libgmove89_core.a gmove89_types.o gmove89_math.o || goto :error
ar rcs libgautmove89.a gautmove89.o || goto :error
ar rcs libgmovepattern89.a gmovepattern89.o || goto :error
ar rcs libgmovesequence89.a gmovesequence89.o || goto :error
ar rcs libgautomotion89.a ^
  gmove89_types.o ^
  gmove89_math.o ^
  gautmove89.o ^
  gmovepattern89.o ^
  gmovesequence89.o || goto :error

%CC% %CFLAGS% tests\test_all.c libgautomotion89.a -o test_all.exe || goto :error
test_all.exe || goto :error

echo.
echo Build OK.
exit /b 0

:error
echo.
echo Build failed.
exit /b 1
