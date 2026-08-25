#!/bin/sh
set -eu
CC=${CC:-cc}
CFLAGS="-std=c89 -pedantic -Wall -Wextra -Werror -Iinclude"
$CC $CFLAGS src/mount89.c tests/test_mount89.c -o test_mount89
$CC $CFLAGS src/mount89.c demo/demo_mount89.c -o demo_mount89
./test_mount89
