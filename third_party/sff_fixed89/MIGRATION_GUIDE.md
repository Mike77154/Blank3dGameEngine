# Migration guide: original `sff.zip` API -> fixed-capacity no-heap API

This guide maps the original heap-backed API to the migrated fixed-capacity API in `sff_fixed/sff`.

## Goals of the migration

- remove heap usage from the library core
- remove hidden file I/O from the parser
- keep the public surface familiar where possible
- make PNG support pluggable instead of hard-wired
- keep C89 compatibility

## Big behavioral changes

### 1) `sff_open(path, ...)` is gone
The library no longer opens files by pathname internally.

Use one of these instead:

- `sff_open_memory(...)` when you already own the bytes
- `sff_open_io(...)` with an `SffIo` callback table
- `extras/sff_stdio_adapter.*` if you still want stdio-backed reads

### 2) decode is now caller-owned
The old API allocated decode buffers for you.

The new API writes into buffers you provide:

- `sff_decode_sprite_indexed_into(...)`
- `sff_decode_sprite_rgba_into(...)`

So:

- no `sff_free(...)`
- no hidden `malloc`
- capacity errors are explicit

### 3) fixed compile-time capacities
The parser no longer grows tables dynamically.

Capacities are controlled in `sff_config.h`:

- `SFF_MAX_SPRITES`
- `SFF_MAX_PALETTES`
- `SFF_MAP_CAPACITY`

## API mapping

### Open / close

Old:

```c
SffFile *s = 0;
SffOpenOptions opt;
opt.tolerant = 1;
rc = sff_open(&s, path, &opt);
...
sff_close(s);
```

New, memory-backed:

```c
SffFile s;
SffOpenOptions opt;
opt.tolerant = 1;
opt.codec = 0;
rc = sff_open_memory(&s, bytes, size, &opt);
...
sff_close(&s);
```

New, stdio-backed:

```c
FILE *fp;
SffFile s;
SffOpenOptions opt;
SffStdioSource src;
SffIo io;

fp = fopen(path, "rb");
if (!fp) return 1;
if (!sff_stdio_source_init(&src, fp)) return 1;
sff_stdio_make_io(&src, &io);

opt.tolerant = 1;
opt.codec = 0;
rc = sff_open_io(&s, &io, &opt);
```

### Decode indexed

Old:

```c
sff_u8 *pixels = 0;
sff_u16 w, h;
sff_u8 pal[768];
int has_pal;
rc = sff_decode_sprite_indexed_alloc(s, idx, &pixels, &w, &h, pal, &has_pal);
...
sff_free(s, pixels);
```

New:

```c
SffSpriteInfo info;
sff_u8 *pixels;
sff_u32 pixels_size;
sff_u16 w, h;
sff_u8 pal[768];
int has_pal;

sff_get_sprite_info(&s, idx, &info);
pixels_size = (sff_u32)info.w * (sff_u32)info.h;
pixels = caller_buffer;

rc = sff_decode_sprite_indexed_into(&s, idx,
                                    pixels, pixels_size,
                                    &w, &h,
                                    pal, &has_pal,
                                    0, 0u);
```

### Decode RGBA

Old:

```c
sff_u8 *rgba = 0;
sff_u16 w, h;
rc = sff_decode_sprite_rgba_alloc(s, idx, &rgba, &w, &h);
...
sff_free(s, rgba);
```

New:

```c
SffSpriteInfo info;
sff_u8 *rgba;
sff_u32 rgba_size;
sff_u16 w, h;

sff_get_sprite_info(&s, idx, &info);
rgba_size = (sff_u32)info.w * (sff_u32)info.h * 4u;
rgba = caller_buffer;

rc = sff_decode_sprite_rgba_into(&s, idx,
                                 rgba, rgba_size,
                                 &w, &h,
                                 0, 0u);
```

Notes:

- for memory-backed inputs, RGBA decode now works without an external scratch buffer for indexed sprites
- for I/O-backed inputs, supplying a work buffer is still useful to avoid rereads and temporary copies

## PNG dependency injection

The original library had an internal PNG implementation coupling.
The migrated version exposes hooks in `sff_codec.h`:

```c
typedef struct SffImageCodec {
    SffDecodePngIndexedFn decode_png_indexed;
    SffDecodePngRgbaFn    decode_png_rgba;
    void *user;
} SffImageCodec;
```

So the app can plug in:

- libpng
- stb_image + custom palette handling
- lodepng
- platform image APIs
- no PNG support at all

If no codec is attached, PNG sprites return `SFF_ERR_CODEC_REQUIRED`.

## Struct changes worth noticing

### `SffOpenOptions`
Now includes:

```c
const SffImageCodec *codec;
```

### `SffSpriteInfo`
Now includes:

- `load_mode`
- effective linked-sprite metadata fixes for v2 link-only entries

## Error-handling changes

The migrated code reports capacity and ownership issues more explicitly:

- `SFF_ERR_CAPACITY`
- `SFF_ERR_BUFFER_TOO_SMALL`
- `SFF_ERR_CODEC_REQUIRED`
- `SFF_ERR_NOT_INDEXED`

## Recommended migration strategy

### Minimal-disruption path

1. keep your current call sites around `sff_find_sprite`, `sff_get_sprite_info`, palette reads, etc.
2. replace `sff_open(...)` with `sff_open_memory(...)` or `sff_open_io(...)`
3. replace alloc-style decode calls with caller-owned `_into(...)` calls
4. delete all `sff_free(...)` usage
5. plug a PNG backend only if your corpus actually needs it

### Runtime-friendly path

If you want a mostly drop-in transition while staying no-heap in the core:

- use `extras/sff_stdio_adapter.*`
- allocate decode buffers in the application layer
- keep the core library ignorant of filesystem and allocator policy
