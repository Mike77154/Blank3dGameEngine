@echo off
setlocal
set CC=gcc
set AR=ar
set CFLAGS=-O2 -std=c89 -pedantic -Wall -Wextra -Werror
set INC=-Iinclude -I..\gcrosshair_core89\include

%CC% %INC% %CFLAGS% -c src\gcrosshair_provider89.c -o gcrosshair_provider89.o || exit /b 1
%AR% rcs libgcrosshair_provider89.a gcrosshair_provider89.o || exit /b 1
%CC% %INC% %CFLAGS% tests\test_provider.c libgcrosshair_provider89.a ..\gcrosshair_core89\libgcrosshair_core89.a -o test_provider.exe || exit /b 1
test_provider.exe || exit /b 1
%CC% %INC% %CFLAGS% examples\example_external_provider.c libgcrosshair_provider89.a ..\gcrosshair_core89\libgcrosshair_core89.a -o example_external_provider.exe || exit /b 1
%CC% %INC% %CFLAGS% examples\example_standalone_ppm.c libgcrosshair_provider89.a ..\gcrosshair_core89\libgcrosshair_core89.a -o example_standalone_ppm.exe || exit /b 1

echo provider89 build/test OK
endlocal
