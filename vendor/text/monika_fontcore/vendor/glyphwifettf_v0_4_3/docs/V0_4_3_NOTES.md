# GlyphWifeTTF v0.4.3 Notes

This patch focuses on making the library cleaner for engine integration.

## Important integration change

SVG export was moved out of the core translation unit:

```txt
src/glyphwifettf.c      -> core parser/layout/raster/atlas/text bitmap
src/glyphwifettf_svg.c  -> optional SVG debug/export helpers
```

If your runtime does not need SVG export, compile only `src/glyphwifettf.c`.

## Freestanding-ish core

`src/glyphwifettf.c` no longer includes `<stdio.h>` and does not call `sprintf`.
The examples still use normal C file IO because they are host tools.

## Long long switch

By default, the core uses `long long` for accurate fixed-point intermediate math:

```sh
gcc -std=c89 -Wall -Wextra -Iinclude src/glyphwifettf.c examples/ttf_dump.c -o ttf_dump
```

For stricter C89 experiments:

```sh
gcc -std=c89 -pedantic -DGWT_USE_LONG_LONG=0 -Wall -Wextra -Iinclude -c src/glyphwifettf.c
```

The fallback is intended for moderate 16.16 values. Use the default for best raster/layout accuracy on PC/tool builds.

## SVG safety

SVG export no longer uses fixed-size `sprintf` formatting for dynamic attributes. `fill` and `stroke` are emitted through the writer and XML-escaped.
