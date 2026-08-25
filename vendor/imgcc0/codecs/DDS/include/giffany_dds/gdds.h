#ifndef GDDS_H
#define GDDS_H

/*
    gdds.h - modular DDS encoder/decoder for RGBA8/BGRA8/DXT1/DXT3/DXT5/BC4/BC5 (+ SNORM normal-map utilities)
    Public domain / CC0-1.0.

    Scope
    -----
    - Decode DDS 2D textures to RGBA8.
    - Encode RGBA8 pixels to DDS as RGBA8, BGRA8, DXT1, DXT3, DXT5, BC4, or BC5, with optional _SRGB / _SNORM variants where applicable.
    - Read classic DDS headers and a small, useful DX10 subset.
    - Inspect mip layouts and decode a specific mip level.
    - Encode caller-provided mipchains.
    - Generate mipchains from a single RGBA8 image.
    - Generate sRGB-aware mipmaps when requested.
    - Import/export BC5 normal maps with explicit tangent-space convention transforms.
    - No external dependencies.

    Non-goals
    ---------
    - Cubemaps, arrays, volume textures.
    - BC6H/BC7, cubemap normal-map workflows, or floating-point DDS variants.
*/

#include <limits.h>

#if UCHAR_MAX != 255
#error "gdds requires 8-bit unsigned char"
#endif
#if USHRT_MAX != 65535U
#error "gdds requires 16-bit unsigned short"
#endif
#if UINT_MAX != 4294967295U
#error "gdds requires 32-bit unsigned int"
#endif

typedef unsigned char gdds_u8;
typedef signed char gdds_s8;
typedef unsigned short gdds_u16;
typedef signed short gdds_s16;
typedef unsigned int gdds_u32;
typedef signed int gdds_s32;
typedef gdds_u32 gdds_size;

#ifndef GDDS_STATIC_RGBA_CAPACITY
#define GDDS_STATIC_RGBA_CAPACITY 8388608u
#endif
#ifndef GDDS_STATIC_DDS_CAPACITY
#define GDDS_STATIC_DDS_CAPACITY 8388608u
#endif
#ifndef GDDS_STATIC_MIP_CAPACITY
#define GDDS_STATIC_MIP_CAPACITY 12582912u
#endif
#ifndef GDDS_MAX_MIP_LEVELS
#define GDDS_MAX_MIP_LEVELS 32u
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define GDDS_VERSION_MAJOR 0
#define GDDS_VERSION_MINOR 11
#define GDDS_VERSION_PATCH 0

typedef enum gdds_format {
    GDDS_FORMAT_UNKNOWN = 0,
    GDDS_FORMAT_RGBA8 = 1,
    GDDS_FORMAT_BGRA8 = 2,
    GDDS_FORMAT_DXT1 = 3,
    GDDS_FORMAT_DXT3 = 4,
    GDDS_FORMAT_DXT5 = 5,
    GDDS_FORMAT_RGBA8_SRGB = 6,
    GDDS_FORMAT_BGRA8_SRGB = 7,
    GDDS_FORMAT_DXT1_SRGB = 8,
    GDDS_FORMAT_DXT3_SRGB = 9,
    GDDS_FORMAT_DXT5_SRGB = 10,
    GDDS_FORMAT_BC4_UNORM = 11,
    GDDS_FORMAT_BC5_UNORM = 12,
    GDDS_FORMAT_BC4_SNORM = 13,
    GDDS_FORMAT_BC5_SNORM = 14
} gdds_format;

typedef enum gdds_result {
    GDDS_RESULT_OK = 0,
    GDDS_RESULT_INVALID_ARGUMENT = -1,
    GDDS_RESULT_NOT_DDS = -2,
    GDDS_RESULT_UNSUPPORTED = -3,
    GDDS_RESULT_TRUNCATED = -4,
    GDDS_RESULT_ALLOC = -5
} gdds_result;

typedef enum gdds_parse_mode {
    GDDS_PARSE_MODE_PERMISSIVE = 0,
    GDDS_PARSE_MODE_STRICT = 1
} gdds_parse_mode;

typedef enum gdds_alpha_mode {
    GDDS_ALPHA_MODE_UNKNOWN = 0,
    GDDS_ALPHA_MODE_STRAIGHT = 1,
    GDDS_ALPHA_MODE_PREMULTIPLIED = 2,
    GDDS_ALPHA_MODE_OPAQUE = 3,
    GDDS_ALPHA_MODE_CUSTOM = 4
} gdds_alpha_mode;

typedef enum gdds_warning_flags {
    GDDS_WARNING_NONE = 0,
    GDDS_WARNING_MISSING_HEADER_FLAGS = 1u << 0,
    GDDS_WARNING_MISSING_TEXTURE_CAPS = 1u << 1,
    GDDS_WARNING_MISSING_MIPMAP_FLAGS = 1u << 2,
    GDDS_WARNING_RESERVED_HEADER_FIELDS = 1u << 3,
    GDDS_WARNING_DX10_RESERVED_BITS = 1u << 4,
    GDDS_WARNING_INCONSISTENT_PITCH = 1u << 5
} gdds_warning_flags;

typedef enum gdds_mip_filter {
    GDDS_MIP_FILTER_BOX = 0
} gdds_mip_filter;

typedef enum gdds_mip_alpha_filter {
    GDDS_MIP_ALPHA_FILTER_STRAIGHT = 0,
    GDDS_MIP_ALPHA_FILTER_PREMULTIPLIED = 1
} gdds_mip_alpha_filter;

typedef enum gdds_mip_color_space {
    GDDS_MIP_COLOR_SPACE_AUTO = 0,
    GDDS_MIP_COLOR_SPACE_LINEAR = 1,
    GDDS_MIP_COLOR_SPACE_SRGB = 2
} gdds_mip_color_space;

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
    int invert_y; /* legacy extra Y flip, applied after convention transform */
    gdds_normal_map_convention stored_convention;
    gdds_normal_map_convention output_convention;
} gdds_normal_map_decode_options;

typedef struct gdds_normal_map_encode_options {
    gdds_normal_map_layout input_layout;
    int normalize;
    int invert_y; /* legacy extra Y flip, applied after convention transform */
    gdds_normal_map_convention input_convention;
    gdds_normal_map_convention output_convention;
    gdds_format output_format; /* GDDS_FORMAT_BC5_UNORM or GDDS_FORMAT_BC5_SNORM */
} gdds_normal_map_encode_options;

typedef struct gdds_parse_options {
    gdds_parse_mode mode;
} gdds_parse_options;

typedef struct gdds_mipmap_options {
    gdds_u32 mip_count; /* 0 => full chain down to 1x1 */
    gdds_mip_filter filter;
    gdds_mip_alpha_filter alpha_filter;
    gdds_mip_color_space color_space; /* AUTO => linear for standalone generation, inferred in auto-encode */
} gdds_mipmap_options;

typedef struct gdds_info {
    gdds_u32 width;
    gdds_u32 height;
    gdds_u32 mip_count;
    gdds_format source_format;
    gdds_alpha_mode alpha_mode;
    gdds_u32 warning_flags;
    gdds_size data_offset;
    gdds_size top_level_size;
    gdds_size full_chain_size;
    int is_srgb;
    int has_dx10_header;
} gdds_info;

typedef struct gdds_mip_info {
    gdds_u32 level_index;
    gdds_u32 width;
    gdds_u32 height;
    gdds_size data_offset;
    gdds_size data_size;
} gdds_mip_info;

typedef struct gdds_image {
    gdds_u32 width;
    gdds_u32 height;
    gdds_format pixel_format;   /* Decoder output is always GDDS_FORMAT_RGBA8. */
    gdds_format source_format;  /* Original DDS storage format. */
    gdds_alpha_mode alpha_mode;
    int is_srgb;
    gdds_u8* pixels;
    gdds_size size;
} gdds_image;

typedef struct gdds_buffer {
    void* data;
    gdds_size size;
} gdds_buffer;

typedef struct gdds_encode_options {
    gdds_format format; /* RGBA8/BGRA8/DXT1/DXT3/DXT5/BC4/BC5, with optional _SRGB variants where applicable */
} gdds_encode_options;

typedef struct gdds_mip_level_rgba8 {
    const gdds_u8* pixels;
    gdds_size size;
} gdds_mip_level_rgba8;

typedef struct gdds_generated_mip_level {
    gdds_u32 width;
    gdds_u32 height;
    gdds_u8* pixels;
    gdds_size size;
} gdds_generated_mip_level;

typedef struct gdds_generated_mipchain {
    gdds_generated_mip_level* levels;
    gdds_u32 level_count;
} gdds_generated_mipchain;

const char* gdds_result_string(gdds_result result);
int gdds_format_is_srgb(gdds_format format);
gdds_encode_options gdds_encode_options_default(gdds_format format);
int gdds_format_is_snorm(gdds_format format);
gdds_parse_options gdds_parse_options_default(void);
gdds_mipmap_options gdds_mipmap_options_default(void);
gdds_u32 gdds_calc_full_mip_count_2d(gdds_u32 width, gdds_u32 height);

gdds_normal_map_decode_options gdds_normal_map_decode_options_default(void);
gdds_normal_map_encode_options gdds_normal_map_encode_options_default(void);
const char* gdds_normal_map_convention_string(gdds_normal_map_convention convention);
int gdds_normal_map_convention_needs_y_flip(gdds_normal_map_convention source,
                                            gdds_normal_map_convention target);

void gdds_image_release(gdds_image* image);
void gdds_buffer_release(gdds_buffer* buffer);
void gdds_generated_mipchain_release(gdds_generated_mipchain* mipchain);

gdds_result gdds_inspect_memory(const void* dds_data,
                                gdds_size dds_size,
                                gdds_info* out_info);

gdds_result gdds_inspect_memory_ex(const void* dds_data,
                                   gdds_size dds_size,
                                   const gdds_parse_options* options,
                                   gdds_info* out_info);

gdds_result gdds_get_mip_info(const void* dds_data,
                              gdds_size dds_size,
                              gdds_u32 level_index,
                              gdds_mip_info* out_mip_info);

gdds_result gdds_get_mip_info_ex(const void* dds_data,
                                 gdds_size dds_size,
                                 gdds_u32 level_index,
                                 const gdds_parse_options* options,
                                 gdds_mip_info* out_mip_info);

gdds_result gdds_decode_memory(const void* dds_data,
                               gdds_size dds_size,
                               gdds_image* out_image);

gdds_result gdds_decode_memory_ex(const void* dds_data,
                                  gdds_size dds_size,
                                  const gdds_parse_options* options,
                                  gdds_image* out_image);

gdds_result gdds_decode_mip_memory(const void* dds_data,
                                   gdds_size dds_size,
                                   gdds_u32 level_index,
                                   gdds_image* out_image);

gdds_result gdds_decode_mip_memory_ex(const void* dds_data,
                                      gdds_size dds_size,
                                      gdds_u32 level_index,
                                      const gdds_parse_options* options,
                                      gdds_image* out_image);

gdds_result gdds_generate_mipchain_rgba8(const gdds_u8* rgba8,
                                         gdds_u32 width,
                                         gdds_u32 height,
                                         const gdds_mipmap_options* options,
                                         gdds_generated_mipchain* out_mipchain);

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

gdds_result gdds_decode_bc5_normal_map_memory(const void* dds_data,
                                               gdds_size dds_size,
                                               const gdds_normal_map_decode_options* options,
                                               gdds_image* out_image);

gdds_result gdds_decode_bc5_normal_map_mip_memory(const void* dds_data,
                                                   gdds_size dds_size,
                                                   gdds_u32 level_index,
                                                   const gdds_normal_map_decode_options* options,
                                                   gdds_image* out_image);

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

gdds_result gdds_encode_memory_rgba8(const gdds_u8* rgba8,
                                     gdds_u32 width,
                                     gdds_u32 height,
                                     const gdds_encode_options* options,
                                     gdds_buffer* out_buffer);

gdds_result gdds_encode_memory_rgba8_auto_mips(const gdds_u8* rgba8,
                                               gdds_u32 width,
                                               gdds_u32 height,
                                               const gdds_encode_options* encode_options,
                                               const gdds_mipmap_options* mip_options,
                                               gdds_buffer* out_buffer);

gdds_result gdds_encode_mipchain_memory_rgba8(const gdds_mip_level_rgba8* levels,
                                              gdds_u32 level_count,
                                              gdds_u32 base_width,
                                              gdds_u32 base_height,
                                              const gdds_encode_options* options,
                                              gdds_buffer* out_buffer);

#ifdef __cplusplus
}
#endif

#endif /* GDDS_H */
