# giffany_dds modular — C89 static/fixed-point edition

A small **multi-file ISO C89/C90** library for **encoding and decoding DDS textures** under **CC0-1.0**.

This edition keeps the original API surface and support examples/tests while enforcing fixed storage, 32-bit-or-smaller integer arithmetic, and integer/fixed-point normal processing. The codec does not require a runtime heap or a math library.

This phase keeps the strict/permissive parser split, mipchain APIs, automatic mip generation, sRGB-aware color mipmaps, BC4 / BC5 support, and the previous BC5 normal-map helpers, then adds two big things:

- **BC4_SNORM / BC5_SNORM** read + write support through the generic DDS API
- a more explicit **normal-map import/export convention layer** for BC5 workflows:
  - choose **DirectX**, **OpenGL**, or **glTF** style conventions
  - convert conventions in-place on `RGBA8` normal maps
  - encode BC5 normal maps as either **BC5_UNORM** or **BC5_SNORM**
  - decode BC5 normal maps while converting from the stored convention to the requested output convention

The normal-map pipeline still supports:

- optional **Z reconstruction**
- optional **renormalization**
- automatic **normal-aware mip generation**
- standalone in-place utilities for reconstructing Z, normalizing, and flipping Y

## Supported formats

### Read
- Legacy DDS 2D textures:
  - `RGBA8`
  - `BGRA8`
  - `DXT1` / `BC1`
  - `DXT3` / `BC2`
  - `DXT5` / `BC3`
  - `ATI1` / `BC4_UNORM`
  - `ATI2` / `BC5_UNORM`
- DX10 header subset:
  - `R8G8B8A8_UNORM`
  - `R8G8B8A8_UNORM_SRGB`
  - `B8G8R8A8_UNORM`
  - `B8G8R8A8_UNORM_SRGB`
  - `BC1_UNORM(_SRGB)`
  - `BC2_UNORM(_SRGB)`
  - `BC3_UNORM(_SRGB)`
  - `BC4_UNORM`
  - `BC4_SNORM`
  - `BC5_UNORM`
  - `BC5_SNORM`

All generic decodes produce **RGBA8** pixels.

### Write
- Legacy-header output:
  - `GDDS_FORMAT_RGBA8`
  - `GDDS_FORMAT_BGRA8`
  - `GDDS_FORMAT_DXT1`
  - `GDDS_FORMAT_DXT3`
  - `GDDS_FORMAT_DXT5`
- DX10-header output:
  - `GDDS_FORMAT_RGBA8_SRGB`
  - `GDDS_FORMAT_BGRA8_SRGB`
  - `GDDS_FORMAT_DXT1_SRGB`
  - `GDDS_FORMAT_DXT3_SRGB`
  - `GDDS_FORMAT_DXT5_SRGB`
  - `GDDS_FORMAT_BC4_UNORM`
  - `GDDS_FORMAT_BC4_SNORM`
  - `GDDS_FORMAT_BC5_UNORM`
  - `GDDS_FORMAT_BC5_SNORM`

## Intentional non-goals

- cubemaps
- texture arrays
- volume textures
- BC6H / BC7
- floating-point DDS formats

## BC5 normal-map API in this phase

### Option types

```c
typedef enum gdds_normal_map_layout {
    GDDS_NORMAL_MAP_LAYOUT_XY_RG = 0,
    GDDS_NORMAL_MAP_LAYOUT_XYZ_RGB = 1
} gdds_normal_map_layout;

typedef enum gdds_normal_map_convention {
    GDDS_NORMAL_MAP_CONVENTION_DIRECTX = 0,
    GDDS_NORMAL_MAP_CONVENTION_OPENGL = 1,
    GDDS_NORMAL_MAP_CONVENTION_GLTF = 2
} gdds_normal_map_convention;

typedef struct gdds_normal_map_decode_options {
    int reconstruct_z;
    int normalize;
    int invert_y;
    gdds_normal_map_convention stored_convention;
    gdds_normal_map_convention output_convention;
} gdds_normal_map_decode_options;

typedef struct gdds_normal_map_encode_options {
    gdds_normal_map_layout input_layout;
    int normalize;
    int invert_y;
    gdds_normal_map_convention input_convention;
    gdds_normal_map_convention output_convention;
    gdds_format output_format; /* BC5_UNORM or BC5_SNORM */
} gdds_normal_map_encode_options;
```

### Utility functions

```c
gdds_normal_map_decode_options gdds_normal_map_decode_options_default(void);
gdds_normal_map_encode_options gdds_normal_map_encode_options_default(void);
const char* gdds_normal_map_convention_string(gdds_normal_map_convention convention);
int gdds_normal_map_convention_needs_y_flip(gdds_normal_map_convention source,
                                            gdds_normal_map_convention target);

gdds_result gdds_normal_map_reconstruct_z_rgba8(gdds_u8* rgba8,
                                               gdds_u32 width,
                                               gdds_u32 height);

gdds_result gdds_normal_map_normalize_rgba8(gdds_u8* rgba8,
                                             gdds_u32 width,
                                             gdds_u32 height);

gdds_result gdds_normal_map_invert_y_rgba8(gdds_u8* rgba8,
                                            gdds_u32 width,
                                            gdds_u32 height);

gdds_result gdds_normal_map_convert_convention_rgba8(gdds_u8* rgba8,
                                                      gdds_u32 width,
                                                      gdds_u32 height,
                                                      gdds_normal_map_layout layout,
                                                      gdds_normal_map_convention source_convention,
                                                      gdds_normal_map_convention target_convention);
```

### Decode helpers

```c
gdds_result gdds_decode_bc5_normal_map_memory(const void* dds_data,
                                               gdds_size dds_size,
                                               const gdds_normal_map_decode_options* options,
                                               gdds_image* out_image);

gdds_result gdds_decode_bc5_normal_map_mip_memory(const void* dds_data,
                                                   gdds_size dds_size,
                                                   gdds_u32 level_index,
                                                   const gdds_normal_map_decode_options* options,
                                                   gdds_image* out_image);
```

These helpers accept `BC5_UNORM` and `BC5_SNORM`. They decode BC5, optionally reconstruct Z, optionally renormalize the normal, and can convert between explicit stored/output conventions.

### Encode helpers

```c
gdds_result gdds_generate_normal_map_mipchain_rgba8(const gdds_u8* rgba8,
                                                     gdds_u32 width,
                                                     gdds_u32 height,
                                                     const gdds_normal_map_encode_options* normal_options,
                                                     const gdds_mipmap_options* mip_options,
                                                     gdds_generated_mipchain* out_mipchain);

gdds_result gdds_encode_bc5_normal_map_rgba8(const gdds_u8* rgba8,
                                              gdds_u32 width,
                                              gdds_u32 height,
                                              const gdds_normal_map_encode_options* normal_options,
                                              gdds_buffer* out_buffer);

gdds_result gdds_encode_bc5_normal_map_rgba8_auto_mips(const gdds_u8* rgba8,
                                                        gdds_u32 width,
                                                        gdds_u32 height,
                                                        const gdds_normal_map_encode_options* normal_options,
                                                        const gdds_mipmap_options* mip_options,
                                                        gdds_buffer* out_buffer);
```

`gdds_encode_bc5_normal_map_rgba8(...)` is the single-level path.

`gdds_encode_bc5_normal_map_rgba8_auto_mips(...)` generates a **normal-aware mipchain** first and then writes it as either `BC5_UNORM` or `BC5_SNORM`, depending on `normal_options.output_format`.

## Generic BC5 behavior still exists

The generic decode API still maps BC5-family textures to simple `RGBA8` staging pixels:

```text
BC5_UNORM  -> R = first block, G = second block, B = 0,   A = 255
BC5_SNORM  -> R = signed-X,    G = signed-Y,    B = 128, A = 255
```

That behavior is preserved for compatibility with the existing API.

The new normal-map helpers sit **on top of** that generic behavior instead of changing it.

## Typical usage

### Encode an OpenGL / glTF-style XY normal map into BC5_SNORM for a DirectX consumer

```c
gdds_normal_map_encode_options normal = gdds_normal_map_encode_options_default();
gdds_buffer dds = {0};

normal.input_layout = GDDS_NORMAL_MAP_LAYOUT_XY_RG;
normal.input_convention = GDDS_NORMAL_MAP_CONVENTION_GLTF;
normal.output_convention = GDDS_NORMAL_MAP_CONVENTION_DIRECTX;
normal.output_format = GDDS_FORMAT_BC5_SNORM;

gdds_encode_bc5_normal_map_rgba8(rgba_normals,
                                 width,
                                 height,
                                 &normal,
                                 &dds);
```

### Encode an XY-in-RG normal map into BC5 with full mipchain generation

```c
gdds_normal_map_encode_options normal = gdds_normal_map_encode_options_default();
gdds_mipmap_options mip = gdds_mipmap_options_default();
gdds_buffer dds = {0};

normal.input_layout = GDDS_NORMAL_MAP_LAYOUT_XY_RG;
mip.mip_count = 0u; /* full chain */

gdds_encode_bc5_normal_map_rgba8_auto_mips(xy_rgba,
                                           width,
                                           height,
                                           &normal,
                                           &mip,
                                           &dds);
```

### Decode BC5 back as a real RGB normal map in glTF convention

```c
gdds_normal_map_decode_options dec = gdds_normal_map_decode_options_default();
gdds_image image = {0};

dec.stored_convention = GDDS_NORMAL_MAP_CONVENTION_DIRECTX;
dec.output_convention = GDDS_NORMAL_MAP_CONVENTION_GLTF;

gdds_decode_bc5_normal_map_memory(dds_bytes,
                                   dds_size,
                                   &dec,
                                   &image);
```

### Convert normal-map conventions in-place

```c
gdds_normal_map_convert_convention_rgba8(rgba_normals,
                                      width,
                                      height,
                                      GDDS_NORMAL_MAP_LAYOUT_XY_RG,
                                      GDDS_NORMAL_MAP_CONVENTION_OPENGL,
                                      GDDS_NORMAL_MAP_CONVENTION_DIRECTX);
```

## Fixed-storage runtime contract

The sanitized build uses fixed compile-time storage instead of dynamically owned output blocks. The public sizing type is `gdds_size`, which is an unsigned 32-bit type on supported targets.

Default capacities are:

- `GDDS_STATIC_RGBA_CAPACITY`: 8 MiB
- `GDDS_STATIC_DDS_CAPACITY`: 8 MiB
- `GDDS_STATIC_MIP_CAPACITY`: 12 MiB
- `GDDS_MAX_MIP_LEVELS`: 32

These macros may be overridden at build time. Returned `gdds_image`, `gdds_buffer`, and generated mip descriptors point into codec-owned static storage. A later operation that uses the same storage category may overwrite an earlier result, so callers that need several results alive simultaneously should copy them into caller-owned fixed buffers.

`gdds_image_release(...)`, `gdds_buffer_release(...)`, and `gdds_generated_mipchain_release(...)` clear descriptors; they do not release heap allocations.

The BC2/BC3/BC4/BC5 48-bit and 64-bit packed fields are handled directly as byte arrays rather than wide scalar integers. Normal-map reconstruction and normalization use integer/fixed-point operations.

## Compatibility model

### `gdds_inspect_memory(...)`, `gdds_get_mip_info(...)`, `gdds_decode_memory(...)`, `gdds_decode_mip_memory(...)`
These use the default **permissive** parser.

That means the library accepts some DDS files that are common in the wild even when their headers are not fully populated, while still rejecting unsupported resource types such as cubemaps, arrays, and volume textures.

### `*_ex(...)`
These accept `gdds_parse_options`.

Set:

```c
gdds_parse_options opt = gdds_parse_options_default();
opt.mode = GDDS_PARSE_MODE_STRICT;
```

Strict mode rejects a subset of files that permissive mode accepts, including cases like:

- missing required DDS header flags
- missing `DDSCAPS_TEXTURE`
- mipmapped files without the expected mip flags/caps
- non-zero reserved header fields
- DX10 reserved bits set in `miscFlags2` or unsupported extra bits in `miscFlag`

Warnings remain useful even in permissive mode, especially for logging or import pipelines.

## Build

### Plain C compiler

```bash
cc -std=c89 -pedantic-errors -Wall -Wextra -Wpedantic -Iinclude \
  src/gdds_common.c src/gdds_parse.c src/gdds_mips.c src/gdds_mipgen.c src/gdds_srgb.c \
  src/gdds_bc.c src/gdds_decode.c src/gdds_encode.c src/gdds_normal.c \
  examples/bc5_snorm_conventions.c -o gdds_bc5_snorm_conventions_example
./gdds_bc5_snorm_conventions_example
```

### CMake

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

The tests cover legacy roundtrips, parser-specific corruption / compatibility cases, explicit mipchains, automatic mip generation, sRGB-aware mip behavior, BC4 / BC5 + ATI1 / ATI2 compatibility, and the BC5 normal-map helpers added in this phase.

## Layout

```text
DDS/
├── include/
│   ├── giffany_dds.h
│   └── giffany_dds/
│       └── gdds.h
├── src/
│   ├── gdds_internal.h
│   ├── gdds_common.c
│   ├── gdds_parse.c
│   ├── gdds_mips.c
│   ├── gdds_mipgen.c
│   ├── gdds_srgb.c
│   ├── gdds_bc.c
│   ├── gdds_decode.c
│   ├── gdds_encode.c
│   └── gdds_normal.c
├── examples/
│   ├── example_support89.h
│   ├── minimal.c
│   ├── mipchain.c
│   ├── auto_mips.c
│   ├── srgb_auto_mips.c
│   ├── bc45.c
│   ├── bc5_normal_map.c
│   └── bc5_snorm_conventions.c
├── tests/
│   ├── test_support89.h
│   ├── golden_vectors.h
│   ├── compile_smoke.c
│   ├── test_roundtrip.c
│   ├── test_mipchain.c
│   ├── test_auto_mips.c
│   ├── test_srgb_mips.c
│   ├── test_bc45.c
│   ├── test_bc5_normal.c
│   ├── test_bc45_snorm.c
│   ├── test_normal_conventions.c
│   ├── test_byte_vectors.c
│   └── test_features.c
├── corpus/
│   ├── README.md
│   ├── PARITY_EXPECTED_SHA256.txt
│   └── parity_expected/        # 36 untouched-reference artifacts
├── tools/
│   ├── audit_c89.sh
│   ├── parity_harness.c
│   └── verify_parity.sh
├── CMakeLists.txt
├── LICENSE.txt
├── README.md
├── SANITIZATION_REPORT.md
├── UPSTREAM_INVENTORY.md
└── PARITY_SHA256.txt
```

The restored test set keeps the original specialized coverage and adds the byte-vector/fixed-storage regression gates from the sanitized edition. The packaged parity corpus can be rechecked at any time with `./tools/verify_parity.sh`.
