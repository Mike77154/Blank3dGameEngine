@echo off
setlocal
set CC=gcc
set CFLAGS=-O2 -std=gnu89 -pedantic -Wall -Wextra -Werror -Iinclude

%CC% %CFLAGS% -c src\wsound_gtypes89.c -o wsound_gtypes89.o || exit /b 1
%CC% %CFLAGS% -c src\wsound_geq6_89.c -o wsound_geq6_89.o || exit /b 1
%CC% %CFLAGS% -c src\wsound_gdist89.c -o wsound_gdist89.o || exit /b 1
%CC% %CFLAGS% -c src\wsound_gchorus89.c -o wsound_gchorus89.o || exit /b 1
%CC% %CFLAGS% -c src\wsound_greverb89.c -o wsound_greverb89.o || exit /b 1
%CC% %CFLAGS% -c src\wsound_ggrenadeblast89.c -o wsound_ggrenadeblast89.o || exit /b 1

ar rcs libwsound_ggrenadeblast89.a wsound_gtypes89.o wsound_geq6_89.o wsound_gdist89.o wsound_gchorus89.o wsound_greverb89.o wsound_ggrenadeblast89.o || exit /b 1
%CC% %CFLAGS% tests\test_c89.c libwsound_ggrenadeblast89.a -o test_c89.exe || exit /b 1
%CC% %CFLAGS% tests\test_transients.c libwsound_ggrenadeblast89.a -o test_transients.exe || exit /b 1
%CC% %CFLAGS% tests\test_speaker_safe.c libwsound_ggrenadeblast89.a -o test_speaker_safe.exe || exit /b 1
%CC% %CFLAGS% demo\render_previews.c libwsound_ggrenadeblast89.a -o render_previews.exe || exit /b 1

test_c89.exe || exit /b 1
test_transients.exe || exit /b 1
test_speaker_safe.exe || exit /b 1
render_previews.exe || exit /b 1
echo Build OK
endlocal
