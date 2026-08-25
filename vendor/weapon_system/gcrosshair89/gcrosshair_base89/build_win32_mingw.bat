@echo off
mingw32-make clean
if errorlevel 1 exit /b 1
mingw32-make all
if errorlevel 1 exit /b 1
test_base.exe
if errorlevel 1 exit /b 1
test_recipe_loader.exe
if errorlevel 1 exit /b 1
test_recipe_io_provider.exe
if errorlevel 1 exit /b 1
test_pre_ini_equivalence.exe
if errorlevel 1 exit /b 1
test_animation_recipes.exe
if errorlevel 1 exit /b 1
