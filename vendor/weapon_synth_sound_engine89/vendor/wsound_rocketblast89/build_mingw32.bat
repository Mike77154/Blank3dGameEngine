@echo off
set CC=gcc
%CC% -O2 -std=c89 -pedantic -Wall -Wextra -Werror -Iinclude -Isrc ^
 src\wsrb89_dsp.c src\wsrb89_presets.c src\wsound_rocketblast89.c ^
 demo\wsrb89_demo.c -o wsrb89_demo.exe
if errorlevel 1 exit /b 1
wsrb89_demo.exe
