@echo off
setlocal
set CC=gcc
set AR=ar
set CFLAGS=-O2 -std=c89 -pedantic -Wall -Wextra -Werror
set INC=-Iinclude -I..\gcrosshair_base89\include -I..\gcrosshair_params89\include -I..\gcrosshair_core89\include -I..\gcrosshair_provider89\include -I..\gcrosshair_anim89\include
set LIBS=libgcrosshair_runtime89.a ..\gcrosshair_anim89\libgcrosshair_anim89.a ..\gcrosshair_provider89\libgcrosshair_provider89.a ..\gcrosshair_core89\libgcrosshair_core89.a ..\gcrosshair_params89\libgcrosshair_params89.a ..\gcrosshair_base89\libgcrosshair_base89.a

%CC% %INC% %CFLAGS% -c src\gcrosshair_runtime89.c -o gcrosshair_runtime89.o || exit /b 1
%AR% rcs libgcrosshair_runtime89.a gcrosshair_runtime89.o || exit /b 1
%CC% %INC% %CFLAGS% tests\test_runtime.c %LIBS% -o test_runtime.exe || exit /b 1
%CC% %INC% %CFLAGS% tests\test_all_preset_animation.c %LIBS% -o test_all_preset_animation.exe || exit /b 1
test_all_preset_animation.exe || exit /b 1
test_runtime.exe || exit /b 1
%CC% %INC% %CFLAGS% tests\test_all_preset_animation.c %LIBS% -o test_all_preset_animation.exe || exit /b 1
test_all_preset_animation.exe || exit /b 1
%CC% %INC% %CFLAGS% examples\example_plug_and_play.c %LIBS% -o example_plug_and_play.exe || exit /b 1

echo runtime89 build/test OK
endlocal
