@echo off
set CC=gcc
set CFLAGS=-std=c89 -Wall -Wextra -pedantic -Iinclude
%CC% %CFLAGS% src\grecoil89.c src\grecoil89_profiles.c src\grecoil89_bridge.c src\grecoil89_provider.c demo\demo_grecoil89.c -o demo_grecoil89.exe
%CC% %CFLAGS% src\grecoil89.c src\grecoil89_profiles.c src\grecoil89_bridge.c src\grecoil89_provider.c demo\demo_bridge.c -o demo_bridge.exe
%CC% %CFLAGS% src\grecoil89.c src\grecoil89_profiles.c src\grecoil89_bridge.c src\grecoil89_provider.c demo\demo_provider.c -o demo_provider.exe
%CC% %CFLAGS% src\grecoil89.c src\grecoil89_profiles.c src\grecoil89_bridge.c src\grecoil89_provider.c tools\grecoil89_curve_dump.c -o grecoil89_curve_dump.exe
echo Done.
