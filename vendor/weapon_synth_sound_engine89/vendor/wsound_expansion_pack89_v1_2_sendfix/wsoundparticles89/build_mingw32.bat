@echo off
gcc -std=c89 -pedantic -Wall -Wextra -Werror -O2 -Iinclude -c src\wsoundparticles89.c -o wsoundparticles89.o
ar rcs libwsoundparticles89.a wsoundparticles89.o
gcc -std=c89 -pedantic -Wall -Wextra -Werror -O2 -Iinclude tests\smoke.c libwsoundparticles89.a -o smoke.exe
