@echo off
mingw32-make clean
if errorlevel 1 exit /b 1
mingw32-make all
if errorlevel 1 exit /b 1
test_core.exe
if errorlevel 1 exit /b 1
test_abi3_shapes.exe
if errorlevel 1 exit /b 1
test_outline.exe
if errorlevel 1 exit /b 1
