# monika_otf_decoder_c89 — OTF/CFF/CFF2 decoder with HVAR/VVAR/MVAR

C89 OpenType decoder focused on **OTF / PostScript outlines** (`OTTO`, `CFF `, `CFF2`).

Hard constraints kept in the library:

- no `malloc`
- no `free`
- no `realloc`
- no internal heap
- no `float`
- no `double`
- fixed point math only, mainly 16.16
- caller-owned buffers and structs

## Current scope

### OpenType container

- sfnt table directory
- `head`
- `maxp`
- `hhea`
- `hmtx`
- `vhea`
- `vmtx`
- `OS/2`
- `name`
- `cmap` format 4 and 12

### CFF 1.0

- `CFF ` table
- CFF INDEX parser
- Top DICT
- Private DICT
- Global and local subroutines
- Type 2 charstrings to fixed-point path commands

### CFF2

- `CFF2` table
- CFF2 Top DICT
- FDArray / FDSelect formats 0, 3, 4
- per-FD Private DICT
- LocalSubr per FD
- CFF2 ItemVariationStore
- `vsindex`
- `blend`
- variable outline interpolation using `fvar` + `avar`

### New in this build: HVAR/VVAR/MVAR

- Generic ItemVariationStore parser for top-level variation tables
- DeltaSetIndexMap format 0 and format 1
- HVAR advance-width variation
- HVAR left-side-bearing variation when mapping is present
- VVAR advance-height variation
- VVAR top-side-bearing variation when mapping is present
- VVAR vertical-origin variation via `otf_get_vorg_var`
- MVAR font-wide metric deltas via `otf_get_mvar_delta`
- Region scalar cache updated when axis values change

## Public metric APIs

```c
int otf_get_hmetric(const otf_font *font, unsigned short gid,
                    unsigned short *advance, short *lsb);

int otf_get_hmetric_var(const otf_font *font, unsigned short gid,
                        unsigned short *advance, short *lsb);

int otf_get_vmetric(const otf_font *font, unsigned short gid,
                    unsigned short *advance_height, short *tsb);

int otf_get_vmetric_var(const otf_font *font, unsigned short gid,
                        unsigned short *advance_height, short *tsb);

int otf_get_vorg_var(const otf_font *font, unsigned short gid,
                     short default_y_origin, short *y_origin);

int otf_get_mvar_delta(const otf_font *font, unsigned long value_tag,
                       long *delta_out);
```

`otf_decode_glyph_path()` now uses `otf_get_hmetric_var()` internally, so glyph paths report interpolated horizontal advance when HVAR is available.

## Build

```sh
make
```

## Demo

```sh
./otf_dump /usr/share/fonts/opentype/cantarell/Cantarell-VF.otf 41 wght=700
```

The dump prints table presence:

```txt
HVAR=1 VVAR=0 MVAR=1 hvarData=2 vvarData=0 mvarRecords=4
```

and reports interpolated glyph metrics after setting the axis.

## Notes

- VVAR is optional and only appears in fonts supporting vertical metrics / vertical layout.
- HVAR is especially important for CFF2 fonts because CFF2 does not have TrueType phantom points.
- MVAR returns the delta; the caller chooses which base table value to apply it to, for example OS/2 `sTypoAscender` for tag `hasc`.
- The static limits are intentionally explicit and tuneable in `otf_c89.h`.
