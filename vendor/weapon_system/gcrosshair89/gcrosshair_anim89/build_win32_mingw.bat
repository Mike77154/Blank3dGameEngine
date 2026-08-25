@echo off
setlocal
set CC=gcc
set AR=ar
set CFLAGS=-O2 -std=c89 -pedantic -Wall -Wextra -Werror
set INC=-Iinclude -I..\gcrosshair_base89\include -I..\gcrosshair_core89\include

%CC% %INC% %CFLAGS% -c src\gcrosshair_anim89.c -o gcrosshair_anim89.o || exit /b 1
%AR% rcs libgcrosshair_anim89.a gcrosshair_anim89.o || exit /b 1
%CC% %INC% %CFLAGS% tests\test_anim.c libgcrosshair_anim89.a ..\gcrosshair_core89\libgcrosshair_core89.a -o test_anim.exe || exit /b 1
test_anim.exe || exit /b 1

echo anim89 build/test OK
endlocal
