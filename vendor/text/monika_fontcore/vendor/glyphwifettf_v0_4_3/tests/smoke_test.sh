#!/bin/sh
set -e
cc=${CC:-gcc}
$cc -std=c89 -O2 -Wall -Wextra -Iinclude src/glyphwifettf.c examples/ttf_dump.c -o /tmp/gwt_dump
$cc -std=c89 -O2 -Wall -Wextra -Iinclude src/glyphwifettf.c examples/ttf_svg_dump.c -o /tmp/gwt_svg
$cc -std=c89 -O2 -Wall -Wextra -Iinclude src/glyphwifettf.c examples/ttf_pgm_raster.c -o /tmp/gwt_pgm
$cc -std=c89 -O2 -Wall -Wextra -Iinclude src/glyphwifettf.c examples/ttf_layout_demo.c -o /tmp/gwt_layout
echo "build ok"
