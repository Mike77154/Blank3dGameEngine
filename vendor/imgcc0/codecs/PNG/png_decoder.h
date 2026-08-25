/*
 * png_decoder.h
 *
 * Small embeddable PNG library with a compatibility layer for the original
 * tiny decoder/encoder API plus a richer platform-style API.
 */
#ifndef PNG_DECODER_H
#define PNG_DECODER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "png_fixed89.h"

/* ---------------- Types ---------------- */

typedef unsigned char  png_u8;
typedef unsigned short png_u16;
typedef unsigned int   png_u32;
typedef signed int     png_i32;

/* ---------------- Version ---------------- */

#define PNG_DEC_VERSION_MAJOR 2
#define PNG_DEC_VERSION_MINOR 13
#define PNG_DEC_VERSION_PATCH 1

/* C89/fixed-point sanitation profile. */
#define PNG_DEC_SANITIZED_C89_FIXED89 1
#define PNG_DEC_FIXED_FRAC_BITS 16
/* All public png_fixed89 metadata values use signed Q16.16. */

/* ---------------- Error codes ---------------- */

#define PNG_DEC_OK               0
#define PNG_DEC_DONE             1  /* progressive/incremental API */
#define PNG_DEC_PAUSED           2  /* progressive decoder paused by callback */
#define PNG_DEC_YIELDED          3  /* progressive decoder yielded for backpressure/work budget */

#define PNG_DEC_ERR_FORMAT      -1
#define PNG_DEC_ERR_OOM         -2
#define PNG_DEC_ERR_ZLIB        -3
#define PNG_DEC_ERR_CRC         -4
#define PNG_DEC_ERR_SIG         -5
#define PNG_DEC_ERR_UNSUPPORTED -6
#define PNG_DEC_ERR_IO          -7
#define PNG_DEC_ERR_DIMENSIONS_TOO_LARGE -8
#define PNG_DEC_ERR_TOO_MANY_PIXELS      -9
#define PNG_DEC_ERR_INFLATED_TOO_LARGE   -10
#define PNG_DEC_ERR_CHUNK_TOO_LARGE      -11
#define PNG_DEC_ERR_TOO_MANY_CHUNKS      -12
#define PNG_DEC_ERR_TEXT_TOO_LARGE       -13
#define PNG_DEC_ERR_FRAME_LIMIT          -14
#define PNG_DEC_ERR_TEMP_MEMORY_LIMIT    -15
#define PNG_DEC_ERR_WORK_BUDGET          -16
#define PNG_DEC_ERR_CONVERSION_LIMIT     -17

/* ---------------- PNG color types ---------------- */

#define PNG_COLOR_GRAYSCALE        0
#define PNG_COLOR_TRUECOLOR        2
#define PNG_COLOR_INDEXED          3
#define PNG_COLOR_GRAYSCALE_ALPHA  4
#define PNG_COLOR_TRUECOLOR_ALPHA  6

/* ---------------- SIMD hints ---------------- */

#if defined(__SSE2__) || (defined(_MSC_VER) && (defined(_M_X64) || (defined(_M_IX86_FP) && _M_IX86_FP >= 2)))
#define PNG_DEC_HAVE_SSE2 1
#else
#define PNG_DEC_HAVE_SSE2 0
#endif

#if defined(__ARM_NEON) || defined(__ARM_NEON__)
#define PNG_DEC_HAVE_NEON 1
#else
#define PNG_DEC_HAVE_NEON 0
#endif

/* Backward-compatible feature aliases used internally by older code. */
#define PNG_DEC_ENABLE_SSE2 PNG_DEC_HAVE_SSE2
#define PNG_DEC_ENABLE_NEON PNG_DEC_HAVE_NEON

/* ---------------- Feature toggles ---------------- */

#ifndef PNG_DEC_DISABLE_GAMMA
#define PNG_DEC_DISABLE_GAMMA 0
#endif

/* ---------------- Safety limits ---------------- */

#ifndef PNG_DEC_MAX_WIDTH
#define PNG_DEC_MAX_WIDTH  16384u
#endif

#ifndef PNG_DEC_MAX_HEIGHT
#define PNG_DEC_MAX_HEIGHT 16384u
#endif

#ifndef PNG_DEC_MAX_IMAGE_BYTES
#define PNG_DEC_MAX_IMAGE_BYTES (256u * 1024u * 1024u)
#endif

#ifndef PNG_DEC_MAX_FILE_BYTES
#define PNG_DEC_MAX_FILE_BYTES  (512u * 1024u * 1024u)
#endif

#ifndef PNG_DEC_MAX_TEXT_ENTRIES
#define PNG_DEC_MAX_TEXT_ENTRIES 256u
#endif

#ifndef PNG_DEC_MAX_UNKNOWN_CHUNKS
#define PNG_DEC_MAX_UNKNOWN_CHUNKS 512u
#endif

#ifndef PNG_DEC_MAX_PIXELS
#define PNG_DEC_MAX_PIXELS (64u * 1024u * 1024u)
#endif

#ifndef PNG_DEC_MAX_INFLATED_BYTES
#define PNG_DEC_MAX_INFLATED_BYTES PNG_DEC_MAX_IMAGE_BYTES
#endif

#ifndef PNG_DEC_MAX_CHUNKS
#define PNG_DEC_MAX_CHUNKS 4096u
#endif

#ifndef PNG_DEC_MAX_TEXT_BYTES
#define PNG_DEC_MAX_TEXT_BYTES (16u * 1024u * 1024u)
#endif

#ifndef PNG_DEC_MAX_APNG_FRAMES
#define PNG_DEC_MAX_APNG_FRAMES 4096u
#endif

#ifndef PNG_DEC_MAX_TEMP_BYTES
#define PNG_DEC_MAX_TEMP_BYTES (512u * 1024u * 1024u)
#endif

#ifndef PNG_DEC_MAX_CONVERSION_EXPANSION
#define PNG_DEC_MAX_CONVERSION_EXPANSION 8u
#endif

/* ---------------- Decode transform flags ---------------- */

#define PNG_DEC_TRANSFORM_NONE              0u
#define PNG_DEC_TRANSFORM_APPLY_GAMMA       0x0001u
#define PNG_DEC_TRANSFORM_PREMULTIPLY_ALPHA 0x0002u
#define PNG_DEC_TRANSFORM_SWAP_RB           0x0004u
#define PNG_DEC_TRANSFORM_INVERT_ALPHA      0x0008u
#define PNG_DEC_TRANSFORM_STRIP_ALPHA       0x0010u
#define PNG_DEC_TRANSFORM_SWAP_16_ENDIAN    0x0020u

/* ---------------- Public output formats ---------------- */
/* 8-bit formats store one byte per channel.
 * 16-bit formats store each channel in PNG/network byte order (MSB first).
 */

#define PNG_OUTPUT_RGBA8  0u
#define PNG_OUTPUT_BGRA8  1u
#define PNG_OUTPUT_ARGB8  2u
#define PNG_OUTPUT_RGB8   3u
#define PNG_OUTPUT_BGR8   4u
#define PNG_OUTPUT_GA8    5u
#define PNG_OUTPUT_AG8    6u
#define PNG_OUTPUT_G8     7u
#define PNG_OUTPUT_RGBA16 8u
#define PNG_OUTPUT_BGRA16 9u
#define PNG_OUTPUT_ARGB16 10u
#define PNG_OUTPUT_RGB16  11u
#define PNG_OUTPUT_BGR16  12u
#define PNG_OUTPUT_GA16   13u
#define PNG_OUTPUT_AG16   14u
#define PNG_OUTPUT_G16    15u

/* ---------------- Unknown chunk policy ---------------- */

#define PNG_DEC_KEEP_UNKNOWN_NEVER 0
#define PNG_DEC_KEEP_UNKNOWN_SAFE  1
#define PNG_DEC_KEEP_UNKNOWN_ALL   2

/* ---------------- Chunk storage locations ---------------- */

#define PNG_CHUNK_POS_AFTER_IHDR  1u
#define PNG_CHUNK_POS_AFTER_PLTE  2u
#define PNG_CHUNK_POS_AFTER_IDAT  3u
#define PNG_CHUNK_POS_BEFORE_IEND 4u

/* ---------------- APNG dispose / blend operations ---------------- */

#define PNG_APNG_DISPOSE_OP_NONE       0u
#define PNG_APNG_DISPOSE_OP_BACKGROUND 1u
#define PNG_APNG_DISPOSE_OP_PREVIOUS   2u

#define PNG_APNG_BLEND_OP_SOURCE 0u
#define PNG_APNG_BLEND_OP_OVER   1u

/* ---------------- Encoder input formats / transforms ---------------- */

#define PNG_ENC_INPUT_FORMAT_NATIVE 255u

#define PNG_ENC_TRANSFORM_NONE               0u
#define PNG_ENC_TRANSFORM_UNPREMULTIPLY_ALPHA 0x0001u
#define PNG_ENC_TRANSFORM_INVERT_ALPHA        0x0002u

/* ---------------- Encoder filter strategies ---------------- */

#define PNG_ENC_FILTER_NONE_FIXED  0u
#define PNG_ENC_FILTER_SUB_FIXED   1u
#define PNG_ENC_FILTER_UP_FIXED    2u
#define PNG_ENC_FILTER_AVG_FIXED   3u
#define PNG_ENC_FILTER_PAETH_FIXED 4u
#define PNG_ENC_FILTER_ADAPTIVE    5u

#define PNG_ENC_FILTER_MASK_NONE   0x01u
#define PNG_ENC_FILTER_MASK_SUB    0x02u
#define PNG_ENC_FILTER_MASK_UP     0x04u
#define PNG_ENC_FILTER_MASK_AVG    0x08u
#define PNG_ENC_FILTER_MASK_PAETH  0x10u
#define PNG_ENC_FILTER_MASK_ALL    0x1Fu

/* ---------------- Metadata containers ---------------- */

typedef struct png_text_entry_s {
    char* keyword;
    char* text;
    char* language_tag;
    char* translated_keyword;
    png_u8 compression; /* 0=tEXt, 1=zTXt, 2=iTXt */
} png_text_entry;

typedef struct png_unknown_chunk_s {
    png_u8  type[5];
    png_u8* data;
    png_u32 size;
    png_u8  location;     /* PNG_CHUNK_POS_* */
    png_u8  safe_to_copy; /* derived from chunk type */
} png_unknown_chunk;

typedef struct png_splt_entry_s {
    png_u16 red;
    png_u16 green;
    png_u16 blue;
    png_u16 alpha;
    png_u16 frequency;
} png_splt_entry;

typedef struct png_splt_palette_s {
    char* name;
    png_u8 sample_depth; /* 8 or 16 */
    png_splt_entry* entries;
    png_u32 entry_count;
} png_splt_palette;

typedef struct png_pcal_info_s {
    char* name;
    png_i32 x0;
    png_i32 x1;
    png_u8 equation_type;
    char* unit_name;
    char** params;
    png_u32 param_count;
} png_pcal_info;

typedef struct png_cicp_info_s {
    png_u8 colour_primaries;
    png_u8 transfer_function;
    png_u8 matrix_coefficients;
    png_u8 full_range_flag;
} png_cicp_info;

typedef struct png_mdcv_info_s {
    png_fixed89 display_primaries_x[3];
    png_fixed89 display_primaries_y[3];
    png_fixed89 white_point_x;
    png_fixed89 white_point_y;
    png_fixed89 max_luminance;
    png_fixed89 min_luminance;
} png_mdcv_info;

typedef struct png_clli_info_s {
    png_fixed89 max_content_light_level;
    png_fixed89 max_frame_average_light_level;
} png_clli_info;

typedef struct png_dsig_entry_s {
    png_u8* cms_data;
    png_u32 size;
    png_u8 location; /* PNG_CHUNK_POS_AFTER_IHDR or PNG_CHUNK_POS_BEFORE_IEND */
} png_dsig_entry;

typedef struct png_gifg_entry_s {
    png_u8 disposal_method;
    png_u8 user_input_flag;
    png_u16 delay_time_cs;
    png_u8 location;
} png_gifg_entry;

typedef struct png_gifx_entry_s {
    char application_identifier[9];
    png_u8 authentication_code[3];
    png_u8* application_data;
    png_u32 application_data_size;
    png_u8 location;
} png_gifx_entry;

typedef struct png_gift_entry_s {
    png_i32 grid_left;
    png_i32 grid_top;
    png_u32 grid_width;
    png_u32 grid_height;
    png_u8 cell_width;
    png_u8 cell_height;
    png_u8 foreground_rgb[3];
    png_u8 background_rgb[3];
    png_u8* text_data;
    png_u32 text_data_size;
    png_u8 location;
} png_gift_entry;

typedef struct png_frac_entry_s {
    png_u8* data;
    png_u32 size;
    png_u8 location;
} png_frac_entry;

typedef struct png_apng_frame_control_s {
    png_u32 width;
    png_u32 height;
    png_u32 x_offset;
    png_u32 y_offset;
    png_u16 delay_num;
    png_u16 delay_den;
    png_u8  dispose_op;
    png_u8  blend_op;
} png_apng_frame_control;

typedef struct png_apng_frame_s {
    png_apng_frame_control control;
    png_u8* pixels;
    png_u32 pixel_rowbytes;
    png_u8 output_format;
    png_u8 output_channels;
    png_u8 output_sample_depth;
    png_u8 output_bytes_per_channel;
} png_apng_frame;

typedef struct png_apng_encode_frame_s {
    const png_u8* pixels;
    png_u32 width;
    png_u32 height;
    png_u32 x_offset;
    png_u32 y_offset;
    png_u32 stride_bytes;
    png_u16 delay_num;
    png_u16 delay_den;
    png_u8  dispose_op;
    png_u8  blend_op;
} png_apng_encode_frame;

/* ---------------- Public decoded image ---------------- */

typedef struct png_image_s {
    png_u32 width;
    png_u32 height;

    /* Decoded pixel buffer in the requested public output format. */
    png_u8* pixels;
    png_u32 pixel_rowbytes;
    png_u8  output_format;
    png_u8  output_channels;
    png_u8  output_sample_depth;
    png_u8  output_bytes_per_channel;

    /* Compatibility alias: only non-NULL when output_format == PNG_OUTPUT_RGBA8. */
    png_u8* rgba;

    /* Original source format metadata. */
    png_u8 source_color_type;
    png_u8 source_bit_depth;
    png_u8 interlace_method;

    /* Which transforms were applied before public-format conversion. */
    png_u32 transform_flags_applied;

    /* Basic color/physical metadata. */
    png_fixed89 image_gamma; /* 0 if unknown */
    int    is_srgb;

    int    has_cHRM;
    png_fixed89 white_x, white_y;
    png_fixed89 red_x, red_y;
    png_fixed89 green_x, green_y;
    png_fixed89 blue_x, blue_y;

    int    has_pHYs;
    png_u32 pHYs_ppu_x;
    png_u32 pHYs_ppu_y;
    png_u8  pHYs_unit;

    int    has_cICP;
    png_cicp_info cicp;

    int    has_mDCV;
    png_mdcv_info mdcv;

    int    has_cLLI;
    png_clli_info clli;

    int    has_oFFs;
    png_i32 offset_x;
    png_i32 offset_y;
    png_u8  offset_unit;

    int    has_sCAL;
    png_u8  scal_unit;
    char*   scal_pixel_width;
    char*   scal_pixel_height;
    png_fixed89  scal_width;
    png_fixed89  scal_height;

    int    has_sTER;
    png_u8  ster_mode;

    int    has_pCAL;
    png_pcal_info pcal;

    int    has_bKGD;
    png_u16 bkgd_r, bkgd_g, bkgd_b;

    int    has_tIME;
    png_u16 time_year;
    png_u8  time_month, time_day;
    png_u8  time_hour, time_minute, time_second;

    int    has_sBIT;
    png_u8  sbit_r, sbit_g, sbit_b, sbit_a;

    int    has_iCCP;
    char*  iccp_name;
    png_u8 iccp_compression_method;
    png_u8* iccp_profile;
    png_u32 iccp_profile_size;

    int    has_eXIf;
    png_u8* exif_profile;
    png_u32 exif_profile_size;

    png_dsig_entry* dsig_chunks;
    png_u32 dsig_chunk_count;

    png_gifg_entry* gifg_chunks;
    png_u32 gifg_chunk_count;

    png_gifx_entry* gifx_chunks;
    png_u32 gifx_chunk_count;

    png_gift_entry* gift_chunks;
    png_u32 gift_chunk_count;

    png_frac_entry* frac_chunks;
    png_u32 frac_chunk_count;

    png_u16* hist_entries;
    png_u32 hist_count;

    png_splt_palette* splt_palettes;
    png_u32 splt_palette_count;

    png_text_entry* text_entries;
    png_u32 text_count;

    png_unknown_chunk* unknown_chunks;
    png_u32 unknown_chunk_count;

} png_image;

typedef struct png_apng_s {
    png_u32 width;
    png_u32 height;
    png_u32 num_plays;
    png_u32 frame_count;
    int     has_default_image;
    png_u8  output_format;
    png_u8  output_channels;
    png_u8  output_sample_depth;
    png_u8  output_bytes_per_channel;
    png_apng_frame* frames;
    png_image default_image;
} png_apng;

typedef struct png_apng_info_s {
    png_u32 width;
    png_u32 height;
    png_u32 num_frames_declared;
    png_u32 num_plays;
    png_u8  output_format;
    png_u8  output_channels;
    png_u8  output_sample_depth;
    png_u8  output_bytes_per_channel;
    png_u8  source_color_type;
    png_u8  source_bit_depth;
    png_u8  interlace_method;
} png_apng_info;

/* ---------------- Zlib hooks ---------------- */

typedef int (*png_zlib_decompress_func)(png_u8* dest, png_u32* dest_len,
                                       const png_u8* src, png_u32 src_len);

typedef int (*png_zlib_compress_func)(png_u8* dest, png_u32* dest_len,
                                     const png_u8* src, png_u32 src_len,
                                     int level);

/* ---------------- Decode / progressive options ---------------- */

typedef struct png_decode_options_s {
    png_u32 transform_flags;
    png_u8  output_format;
    int keep_text;
    int keep_unknown_chunks; /* PNG_DEC_KEEP_UNKNOWN_* */
    int strict_trailing_data;

    /* Optional runtime safety limits. 0 => use compile-time defaults. */
    png_u32 max_width;
    png_u32 max_height;
    png_u32 max_image_bytes;
    png_u32 max_file_bytes;
    png_u32 max_text_entries;
    png_u32 max_unknown_chunks;
    png_u32 max_chunk_bytes;
    png_u32 max_pixels;
    png_u32 max_inflated_bytes;
    png_u32 max_chunks;
    png_u32 max_text_bytes;
    png_u32 max_apng_frames;
    png_u32 max_temp_bytes;
    png_u32 max_conversion_expansion;
} png_decode_options;

void png_decode_options_init(png_decode_options* opt);

/* ---------------- Encoder options ---------------- */

typedef struct png_encode_options_s {
    png_u8 color_type;
    png_u8 bit_depth;
    png_u8 interlace_method; /* currently 0 supported */

    png_u8 filter_strategy;  /* PNG_ENC_FILTER_* */
    png_u8 filter_mask;      /* PNG_ENC_FILTER_MASK_* for adaptive */
    png_u8 input_is_packed;  /* for indexed/gray <8-bit */
    png_u8 input_16bit_little_endian;
    png_u8 input_format; /* PNG_ENC_INPUT_FORMAT_NATIVE or PNG_OUTPUT_* */
    png_u32 input_transform_flags;

    int zlevel;
    png_u32 stride_bytes; /* 0 => computed minimal row stride */

    /* Palette / transparency for indexed, grayscale, truecolor. */
    const png_u8* palette;      /* 3 * palette_entries bytes */
    png_u32 palette_entries;
    const png_u8* trns_data;
    png_u32 trns_size;

    /* Common writable metadata. */
    int write_gAMA;
    png_fixed89 image_gamma;

    int write_cHRM;
    png_fixed89 white_x, white_y;
    png_fixed89 red_x, red_y;
    png_fixed89 green_x, green_y;
    png_fixed89 blue_x, blue_y;

    int write_cICP;
    png_cicp_info cicp;

    int write_mDCV;
    png_mdcv_info mdcv;

    int write_cLLI;
    png_clli_info clli;

    int write_sRGB;
    png_u8 srgb_intent;

    int write_pHYs;
    png_u32 pHYs_ppu_x;
    png_u32 pHYs_ppu_y;
    png_u8  pHYs_unit;

    int write_oFFs;
    png_i32 offset_x;
    png_i32 offset_y;
    png_u8  offset_unit;

    int write_sCAL;
    png_u8  scal_unit;
    const char* scal_pixel_width;
    const char* scal_pixel_height;

    int write_sTER;
    png_u8 ster_mode;

    int write_pCAL;
    png_pcal_info pcal;

    int write_tIME;
    png_u16 time_year;
    png_u8  time_month, time_day;
    png_u8  time_hour, time_minute, time_second;

    int write_bKGD;
    png_u16 bkgd_r, bkgd_g, bkgd_b;
    png_u8  bkgd_palette_index;

    int write_sBIT;
    png_u8  sbit_r, sbit_g, sbit_b, sbit_a;

    int write_iCCP;
    const char* iccp_name;
    const png_u8* iccp_profile;
    png_u32 iccp_profile_size;

    int write_eXIf;
    const png_u8* exif_profile;
    png_u32 exif_profile_size;

    const png_dsig_entry* dsig_chunks;
    png_u32 dsig_chunk_count;

    const png_gifg_entry* gifg_chunks;
    png_u32 gifg_chunk_count;

    const png_gifx_entry* gifx_chunks;
    png_u32 gifx_chunk_count;

    const png_gift_entry* gift_chunks;
    png_u32 gift_chunk_count;

    const png_frac_entry* frac_chunks;
    png_u32 frac_chunk_count;

    const png_u16* hist_entries;
    png_u32 hist_count;

    const png_splt_palette* splt_palettes;
    png_u32 splt_palette_count;

    const png_text_entry* text_entries;
    png_u32 text_count;

    const png_unknown_chunk* unknown_chunks;
    png_u32 unknown_chunk_count;

    png_u32 max_temp_bytes;
    png_u32 max_conversion_expansion;
    png_u32 max_apng_frames;
} png_encode_options;

void png_encode_options_init(png_encode_options* opt);

int png_pcal_expected_param_count(png_u8 equation_type);
int png_pcal_map_stored_to_original(png_u8 bit_depth,
                                    png_i32 x0,
                                    png_i32 x1,
                                    png_u32 stored_sample,
                                    png_i32* out_original);
int png_pcal_map_original_to_physical(const png_pcal_info* pcal,
                                      png_i32 original_sample,
                                      png_fixed89* out_value);
int png_pcal_map_stored_to_physical(png_u8 bit_depth,
                                    const png_pcal_info* pcal,
                                    png_u32 stored_sample,
                                    png_fixed89* out_value);
int png_exif_has_valid_tiff_header(const png_u8* data, png_u32 size);
void png_swap_16_buffer(png_u8* data, png_u32 size);

png_u8 png_output_format_channels(png_u8 output_format);
png_u8 png_output_format_bytes_per_channel(png_u8 output_format);
png_u8 png_output_format_sample_depth(png_u8 output_format);
int png_output_format_rowbytes(png_u8 output_format, png_u32 width, png_u32* out_rowbytes);
typedef struct png_conversion_limits_s {
    png_u32 max_output_bytes;         /* 0 => use compile-time defaults */
    png_u32 max_expansion;           /* output_size <= input_size * max_expansion; 0 => PNG_DEC_MAX_CONVERSION_EXPANSION */
    png_u8  max_output_sample_depth; /* 0 => no extra cap; otherwise 8 or 16 */
} png_conversion_limits;

void png_conversion_limits_init(png_conversion_limits* limits);
int png_image_convert_format_ex(png_image* img, png_u8 output_format, const png_conversion_limits* limits);
int png_image_convert_format(png_image* img, png_u8 output_format);

/* ---------------- Decoder API ---------------- */

int png_decode_memory(const png_u8* data,
                      png_u32 size,
                      png_zlib_decompress_func zfunc,
                      png_image* out_image);

int png_decode_memory_ex(const png_u8* data,
                         png_u32 size,
                         png_zlib_decompress_func zfunc,
                         const png_decode_options* options,
                         png_image* out_image);

int png_load_file(const char* filename,
                  png_zlib_decompress_func zfunc,
                  png_image* out_image);

int png_load_file_ex(const char* filename,
                     png_zlib_decompress_func zfunc,
                     const png_decode_options* options,
                     png_image* out_image);

void png_free_image(png_image* img);

/* zlib convenience wrappers */
int png_zlib_decompress(png_u8* dest, png_u32* dest_len,
                        const png_u8* src, png_u32 src_len);

int png_zlib_compress(png_u8* dest, png_u32* dest_len,
                      const png_u8* src, png_u32 src_len,
                      int level);

int png_decode_memory_zlib(const png_u8* data, png_u32 size, png_image* out_image);
int png_decode_memory_ex_zlib(const png_u8* data, png_u32 size,
                              const png_decode_options* options,
                              png_image* out_image);
int png_load_file_zlib(const char* filename, png_image* out_image);
int png_load_file_ex_zlib(const char* filename,
                          const png_decode_options* options,
                          png_image* out_image);

/* ---------------- Incremental/progressive decoder ---------------- */

typedef struct png_decoder_s png_decoder;

typedef struct png_chunk_progress_info_s {
    png_u32 type;
    png_u32 length;
    png_u32 file_offset;
    png_u32 data_offset;
    png_u32 total_size;
    png_u8  ancillary;
    png_u8  private_bit;
    png_u8  reserved_bit;
    png_u8  safe_to_copy;
} png_chunk_progress_info;

typedef void (*png_info_callback_fn)(void* user_ptr, const png_image* header_only);
typedef void (*png_row_callback_fn)(void* user_ptr,
                                    png_u32 row_index,
                                    const png_u8* row_data,
                                    png_u32 rowbytes,
                                    int pass);
typedef void (*png_end_callback_fn)(void* user_ptr, const png_image* image);
typedef void (*png_chunk_callback_fn)(void* user_ptr, const png_chunk_progress_info* info);

typedef struct png_progressive_callbacks_s {
    void* user_ptr;
    png_info_callback_fn info_fn;
    png_row_callback_fn  row_fn;
    png_end_callback_fn  end_fn;
    png_chunk_callback_fn chunk_fn;
} png_progressive_callbacks;

typedef struct png_progressive_control_s {
    png_u32 max_feed_bytes;              /* max caller bytes accepted per feed_ex call; 0 = unlimited */
    png_u32 max_buffered_bytes;          /* max unread bytes kept internally; 0 = unlimited */
    png_u32 max_parse_bytes_per_call;    /* max compressed/PNG bytes parsed per feed call; 0 = unlimited */
    png_u32 max_row_callbacks_per_call;  /* max row callbacks emitted per feed call; 0 = unlimited */
    png_u32 max_chunk_callbacks_per_call;/* max chunk callbacks emitted per feed call; 0 = unlimited */
    png_u32 max_chunk_bytes_per_call;    /* max complete chunk bytes accounted per feed call; 0 = unlimited */
    png_u32 max_zlib_work_bytes_per_call;/* max output bytes inflated per feed call; 0 = unlimited */
    png_u32 max_zlib_steps_per_call;     /* max inflate() calls per feed call; 0 = unlimited */
    int hard_fail_on_budget_exhaustion;  /* 0 => PNG_DEC_YIELDED, non-zero => PNG_DEC_ERR_WORK_BUDGET */
} png_progressive_control;

#define PNG_PROGRESSIVE_POLL_WANT_INPUT    0x01u
#define PNG_PROGRESSIVE_POLL_CAN_DRAIN     0x02u
#define PNG_PROGRESSIVE_POLL_HAVE_BUFFERED 0x04u
#define PNG_PROGRESSIVE_POLL_PAUSED        0x08u
#define PNG_PROGRESSIVE_POLL_DONE          0x10u

typedef struct png_progressive_poll_state_s {
    png_u32 events;
    png_u32 input_room;
    png_u32 unprocessed_bytes;
    png_u32 pending_bytes;
    png_u32 suggested_read_bytes;
} png_progressive_poll_state;

void png_progressive_control_init(png_progressive_control* ctl);
void png_progressive_poll_state_init(png_progressive_poll_state* st);

typedef struct png_apng_decoder_s png_apng_decoder;

typedef void (*png_apng_info_callback_fn)(void* user_ptr, const png_apng_info* info);
typedef void (*png_apng_frame_info_callback_fn)(void* user_ptr,
                                                 png_u32 frame_index,
                                                 const png_apng_frame_control* control,
                                                 png_u32 rowbytes);
typedef void (*png_apng_frame_row_callback_fn)(void* user_ptr,
                                                png_u32 frame_index,
                                                png_u32 row_index,
                                                const png_u8* row_data,
                                                png_u32 rowbytes);
typedef void (*png_apng_frame_row_pass_callback_fn)(void* user_ptr,
                                                     png_u32 frame_index,
                                                     png_u32 row_index,
                                                     const png_u8* row_data,
                                                     png_u32 rowbytes,
                                                     int pass);
typedef void (*png_apng_frame_end_callback_fn)(void* user_ptr,
                                                png_u32 frame_index,
                                                const png_apng_frame* frame);
typedef void (*png_apng_end_callback_fn)(void* user_ptr, const png_apng* apng);
typedef void (*png_apng_chunk_callback_fn)(void* user_ptr,
                                            const png_chunk_progress_info* info,
                                            png_i32 frame_index);

typedef struct png_apng_progressive_callbacks_s {
    void* user_ptr;
    png_apng_info_callback_fn info_fn;
    png_apng_frame_info_callback_fn frame_info_fn;
    png_apng_frame_row_callback_fn  frame_row_fn;      /* compatibility callback */
    png_apng_frame_row_pass_callback_fn frame_row_pass_fn; /* pass-aware callback */
    png_apng_frame_end_callback_fn  frame_end_fn;
    png_apng_end_callback_fn        end_fn;
    png_apng_chunk_callback_fn      chunk_fn;
} png_apng_progressive_callbacks;

int  png_decoder_init(png_decoder** dec_out, png_zlib_decompress_func zfunc);
int  png_decoder_set_options(png_decoder* dec, const png_decode_options* options);
int  png_decoder_set_callbacks(png_decoder* dec, const png_progressive_callbacks* callbacks);
int  png_decoder_set_progressive_control(png_decoder* dec, const png_progressive_control* ctl);
int  png_decoder_feed(png_decoder* dec, const png_u8* data, png_u32 size);
int  png_decoder_feed_ex(png_decoder* dec, const png_u8* data, png_u32 size, png_u32* consumed_out);
png_u32 png_decoder_process_data_pause(png_decoder* dec, int save);
png_u32 png_decoder_process_data_skip(png_decoder* dec);
png_u32 png_decoder_pending_bytes(const png_decoder* dec);
png_u32 png_decoder_unprocessed_bytes(const png_decoder* dec);
png_u32 png_decoder_input_room(const png_decoder* dec);
int  png_decoder_poll(const png_decoder* dec, png_progressive_poll_state* st);
int  png_decoder_is_paused(const png_decoder* dec);
int  png_decoder_take_image(png_decoder* dec, png_image* out_image);
void png_decoder_free(png_decoder* dec);

/* ---------------- Encoder API ---------------- */

int png_encode_rgba8_memory(const png_u8* rgba,
                            png_u32 width,
                            png_u32 height,
                            png_zlib_compress_func zfunc,
                            int zlevel,
                            png_u8** out_png,
                            png_u32* out_png_size);

int png_encode_memory_ex(const png_u8* pixels,
                         png_u32 width,
                         png_u32 height,
                         png_zlib_compress_func zfunc,
                         const png_encode_options* options,
                         png_u8** out_png,
                         png_u32* out_png_size);

int png_encode_image_ex(const png_image* image,
                        png_zlib_compress_func zfunc,
                        const png_encode_options* options,
                        png_u8** out_png,
                        png_u32* out_png_size);

void png_free_file(png_u8* file_data);

int png_encode_rgba8_memory_zlib(const png_u8* rgba,
                                 png_u32 width,
                                 png_u32 height,
                                 int zlevel,
                                 png_u8** out_png,
                                 png_u32* out_png_size);

int png_encode_memory_ex_zlib(const png_u8* pixels,
                              png_u32 width,
                              png_u32 height,
                              const png_encode_options* options,
                              png_u8** out_png,
                              png_u32* out_png_size);

int png_encode_image_ex_zlib(const png_image* image,
                             const png_encode_options* options,
                             png_u8** out_png,
                             png_u32* out_png_size);

/* ---------------- APNG API ---------------- */

int png_decode_apng_memory_ex(const png_u8* data,
                              png_u32 size,
                              png_zlib_decompress_func zfunc,
                              const png_decode_options* options,
                              png_apng* out_apng);
int png_decode_apng_memory(const png_u8* data,
                           png_u32 size,
                           png_zlib_decompress_func zfunc,
                           png_apng* out_apng);
int png_load_apng_file_ex(const char* filename,
                          png_zlib_decompress_func zfunc,
                          const png_decode_options* options,
                          png_apng* out_apng);
int png_load_apng_file(const char* filename,
                       png_zlib_decompress_func zfunc,
                       png_apng* out_apng);
void png_free_apng(png_apng* apng);

int png_encode_apng_memory_ex(const png_apng_encode_frame* frames,
                              png_u32 frame_count,
                              png_u32 canvas_width,
                              png_u32 canvas_height,
                              png_u32 num_plays,
                              png_zlib_compress_func zfunc,
                              const png_encode_options* options,
                              png_u8** out_png,
                              png_u32* out_png_size);
int png_encode_apng_auto_memory_ex(const png_apng_encode_frame* full_canvas_frames,
                                   png_u32 frame_count,
                                   png_u32 canvas_width,
                                   png_u32 canvas_height,
                                   png_u32 num_plays,
                                   png_zlib_compress_func zfunc,
                                   const png_encode_options* options,
                                   png_u8** out_png,
                                   png_u32* out_png_size);
int png_encode_apng_memory_ex_zlib(const png_apng_encode_frame* frames,
                                   png_u32 frame_count,
                                   png_u32 canvas_width,
                                   png_u32 canvas_height,
                                   png_u32 num_plays,
                                   const png_encode_options* options,
                                   png_u8** out_png,
                                   png_u32* out_png_size);
int png_encode_apng_auto_memory_ex_zlib(const png_apng_encode_frame* full_canvas_frames,
                                        png_u32 frame_count,
                                        png_u32 canvas_width,
                                        png_u32 canvas_height,
                                        png_u32 num_plays,
                                        const png_encode_options* options,
                                        png_u8** out_png,
                                        png_u32* out_png_size);
int png_load_apng_file_ex_zlib(const char* filename,
                               const png_decode_options* options,
                               png_apng* out_apng);
int png_load_apng_file_zlib(const char* filename, png_apng* out_apng);
int png_decode_apng_memory_ex_zlib(const png_u8* data,
                                   png_u32 size,
                                   const png_decode_options* options,
                                   png_apng* out_apng);
int png_decode_apng_memory_zlib(const png_u8* data,
                                png_u32 size,
                                png_apng* out_apng);

int  png_apng_decoder_init(png_apng_decoder** dec_out, png_zlib_decompress_func zfunc);
int  png_apng_decoder_set_options(png_apng_decoder* dec, const png_decode_options* options);
int  png_apng_decoder_set_callbacks(png_apng_decoder* dec, const png_apng_progressive_callbacks* callbacks);
int  png_apng_decoder_set_progressive_control(png_apng_decoder* dec, const png_progressive_control* ctl);
int  png_apng_decoder_feed(png_apng_decoder* dec, const png_u8* data, png_u32 size);
int  png_apng_decoder_feed_ex(png_apng_decoder* dec, const png_u8* data, png_u32 size, png_u32* consumed_out);
png_u32 png_apng_decoder_process_data_pause(png_apng_decoder* dec, int save);
png_u32 png_apng_decoder_pending_bytes(const png_apng_decoder* dec);
png_u32 png_apng_decoder_unprocessed_bytes(const png_apng_decoder* dec);
png_u32 png_apng_decoder_input_room(const png_apng_decoder* dec);
int  png_apng_decoder_poll(const png_apng_decoder* dec, png_progressive_poll_state* st);
int  png_apng_decoder_is_paused(const png_apng_decoder* dec);
int  png_apng_decoder_take_animation(png_apng_decoder* dec, png_apng* out_apng);
void png_apng_decoder_free(png_apng_decoder* dec);

/* ---------------- Diagnostics ---------------- */

const char* png_strerror(int err);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* PNG_DECODER_H */
