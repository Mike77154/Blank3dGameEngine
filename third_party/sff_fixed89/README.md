# sff_fixed89

A fixed-capacity, no-heap SFF v1/v2 parser and sprite decoder written in C89.

**License:** CC0 1.0 Universal. See `LICENSE` and `NOTICE.md`.

## What changed

This version was intentionally refit around these constraints:

- **no malloc / realloc / free**
- **no float / double**
- **integer-only, fixed-point-friendly core**
- **generic dependency hooks**
  - source I/O is a `read_at` callback (`SffIo`)
  - PNG decoding can use either embedded `png89+zlib89` or an external callback table (`SffImageCodec`)
- **caller-owned decode buffers**
- **fixed compile-time capacities**
  - `SFF_MAX_SPRITES`
  - `SFF_MAX_PALETTES`
  - `SFF_MAP_CAPACITY`

The public API is therefore **not ABI-compatible** with the original heap-based version.

## Core API

Open from memory:

```c
SffFile sff;
SffOpenOptions opt;
opt.tolerant = 1;
opt.codec = 0;
sff_open_memory(&sff, bytes, size, &opt);
```

Open from a generic data source:

```c
SffIo io;
io.read_at = my_read_at;
io.user = my_user_ptr;
io.size = my_total_size;
sff_open_io(&sff, &io, &opt);
```

Decode indexed:

```c
sff_u8 pixels[W * H];
sff_u8 pal[768];
int has_pal;
sff_decode_sprite_indexed_into(&sff, index,
                               pixels, sizeof(pixels),
                               &w, &h,
                               pal, &has_pal,
                               work_buf, work_buf_size);
```

Decode RGBA:

```c
sff_u8 rgba[W * H * 4];
sff_decode_sprite_rgba_into(&sff, index,
                            rgba, sizeof(rgba),
                            &w, &h,
                            work_buf, work_buf_size);
```

## Important behavior changes

### 1) No path-based `sff_open`
The core does not own file I/O anymore. If you want `FILE*`, use the optional adapter in `extras/sff_stdio_adapter.*`.

### 2) No alloc-returning decode functions
The old `*_alloc` API was removed. Decode is now always **into caller buffers**.

### 3) PNG can be embedded or pluggable
`build/libsff_fixed89.a` embeds `png89 + zlib89` for an internal no-heap PNG path.
You can still override that by supplying your own callbacks through `SffImageCodec`.


## V2 interpretation

The rewrite keeps the widely implemented SFF v2 table layout used by the runtime-style oracle:

- sprite table offset/count
- palette table offset/count
- ldata offset/length
- tdata offset/length

For palette entries, the parser preserves the raw middle fields (`raw_a`, `raw_b`) and supports both common cases:

- **linked palette entry**: `data_len == 0`, `raw_b == link`
- **direct palette entry**: `data_len != 0`, `raw_a` may be treated as color count when it looks sane

That keeps the parser close to Elecbyte-style files without hard-wiring it to a single community reinterpretation of those two middle words.

## Main fixes from the old code

- fixed the **SFF v1 version rejection bug**
- fixed **PCX inverted-dimension underflow**
- removed all **heap allocation paths**
- removed libpng-specific ownership/leak hazards from the core
- added overflow-checked size math for pixel counts
- separated the core from concrete dependency choices

## Capacity tuning

Override these in your build if you want a smaller or larger footprint:

```c
#define SFF_MAX_SPRITES   8192u
#define SFF_MAX_PALETTES   512u
#define SFF_MAP_CAPACITY 16384u
```

`SFF_MAP_CAPACITY` must be a power of two.

## Build

```sh
make
make test
make audit
```

## Embedded image/data stack

`build/libsff_fixed89.a` includes:

- `pcx89`
- `png89`
- `png89_zlib89`
- `zlib89`

So the default stack is now capable of decoding the separately tested v1, v2 MUGEN 1.0, and v2 MUGEN 1.1 corpus without any external PNG library, as long as decode calls receive a large enough caller-owned work buffer.


## Public package policy

This repository contains code, tests, and engineering notes only. Third-party
SFF archives and rendered character artwork are intentionally excluded. The
CC0 dedication applies only to material for which the affirmer owns the rights;
see `NOTICE.md` for the exact scope.
