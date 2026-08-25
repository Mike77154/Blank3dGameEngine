# giffy_hb_c89 v0.20

Tiny HarfBuzz-like shaping runtime for retro/embedded engines.

## Contract

```txt
C89
no malloc/free/realloc in runtime
no runtime heap ownership
no float/double
fixed 26.6
static font packs generated offline
```

## v0.20 focus

This version continues the HUD/debug + HVAR path from v0.19:

```txt
HVAR glyph explanation
  -> validate glyph->item map
  -> validate region axis ranges
  -> compact event/id overlay rows
  -> numeric trace dump usable without names
  -> cluster status enum for dotted-circle overlays
```

## New APIs

```c
int ghb_buffer_copy_trace_overlay_compact(
    const ghb_buffer *b,
    ghb_trace_overlay_compact *out_records,
    int max_records,
    ghb_u32 event_mask
);

int ghb_buffer_dump_trace_events_numeric(
    const ghb_buffer *b,
    ghb_trace_numeric_func fn,
    void *user,
    ghb_u32 event_mask
);

int ghb_font_get_hvar_region_diagnostics(
    const ghb_static_font *font,
    ghb_hvar_region_diag *diag
);

ghb_u8 ghb_buffer_get_cluster_status(const ghb_buffer *b, ghb_u8 syllable);
const char *ghb_cluster_status_name(ghb_u8 status);
```

## Build

```sh
make clean all
./demo
```

No trace / no trace names:

```sh
cc -std=c89 -pedantic -Wall -Wextra \
  -DGHB_NO_TRACE -DGHB_NO_TRACE_NAMES \
  -Iinclude -Iexamples \
  -c src/giffy_hb.c \
  -o /tmp/giffy_hb_no_trace_names_v20.o
```

## Offline packer

```sh
python3 tools/ttf_minipack.py /usr/share/fonts/truetype/dejavu/DejaVuSerif.ttf \
  --var dejavu_serif_v20_smoke \
  --no-gpos \
  --dump-inventory \
  --loss-report \
  -o examples/dejavu_serif_v20_smoke_pack.h
```

The packer remains host-side Python. The runtime remains plain static C89.
