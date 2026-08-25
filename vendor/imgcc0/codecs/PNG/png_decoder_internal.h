/* png_decoder_internal.h */
#ifndef PNG_DECODER_INTERNAL_H
#define PNG_DECODER_INTERNAL_H

#include "png_decoder.h"

typedef struct png_state_s
{
    png_u32 width;
    png_u32 height;
    png_u8  bit_depth;
    png_u8  color_type;
    png_u8  interlace_method;
    png_u8  compression_method;
    png_u8  filter_method;

    /* Decode/config policy */
    png_zlib_decompress_func zfunc;
    png_u32 transform_flags;
    int keep_text;
    int keep_unknown_chunks;
    int strict_trailing_data;

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

    png_u32 chunk_count;
    png_u32 text_bytes;

    /* Palette (PLTE) */
    png_u8* palette;
    png_u32 palette_entries;

    /* Transparency (tRNS) */
    int     has_trns;
    png_u8* trns_data;
    png_u32 trns_size;

    /* IDAT (concatenated) */
    png_u8* idat_data;
    png_u32 idat_size;
    png_u32 idat_capacity;
    int     idat_finished;

    /* Gamma / colorimetry */
    int     have_gama;
    int     have_srgb;
    png_fixed89  image_gamma;
    png_u8  gamma_lut[256];
    int     use_gamma_lut;

    /* cHRM */
    int     have_chrm;
    png_fixed89  white_x, white_y;
    png_fixed89  red_x,   red_y;
    png_fixed89  green_x, green_y;
    png_fixed89  blue_x,  blue_y;

    /* pHYs */
    int     have_phys;
    png_u32 phys_ppu_x, phys_ppu_y;
    png_u8  phys_unit;

    /* cICP / HDR static metadata */
    int     have_cicp;
    png_cicp_info cicp;

    int     have_mdcv;
    png_mdcv_info mdcv;

    int     have_clli;
    png_clli_info clli;

    /* oFFs */
    int     have_offs;
    png_i32 offs_x, offs_y;
    png_u8  offs_unit;

    /* sCAL */
    int     have_scal;
    png_u8  scal_unit;
    char*   scal_pixel_width;
    char*   scal_pixel_height;
    png_fixed89  scal_width;
    png_fixed89  scal_height;

    /* sTER */
    int     have_ster;
    png_u8  ster_mode;

    /* pCAL */
    int     have_pcal;
    png_pcal_info pcal;

    /* bKGD */
    int     have_bkgd;
    png_u16 bkgd_r, bkgd_g, bkgd_b;

    /* tIME */
    int     have_time;
    png_u16 time_year;
    png_u8  time_month, time_day;
    png_u8  time_hour, time_minute, time_second;

    /* sBIT */
    int     have_sbit;
    png_u8  sbit[4];

    /* iCCP */
    int     have_iccp;
    char*   iccp_name;
    png_u8  iccp_compression_method;
    png_u8* iccp_profile;
    png_u32 iccp_profile_size;

    /* eXIf */
    int     have_exif;
    png_u8* exif_profile;
    png_u32 exif_profile_size;

    /* dSIG */
    png_dsig_entry* dsig_chunks;
    png_u32 dsig_chunk_count;
    png_u32 dsig_chunk_capacity;

    /* GIF compatibility / legacy extension chunks */
    png_gifg_entry* gifg_chunks;
    png_u32 gifg_chunk_count;
    png_u32 gifg_chunk_capacity;

    png_gifx_entry* gifx_chunks;
    png_u32 gifx_chunk_count;
    png_u32 gifx_chunk_capacity;

    png_gift_entry* gift_chunks;
    png_u32 gift_chunk_count;
    png_u32 gift_chunk_capacity;

    png_frac_entry* frac_chunks;
    png_u32 frac_chunk_count;
    png_u32 frac_chunk_capacity;

    /* Stored ancillarys */
    png_u16* hist_entries;
    png_u32 hist_count;

    png_splt_palette* splt_palettes;
    png_u32 splt_palette_count;
    png_u32 splt_palette_capacity;

    png_text_entry* text_entries;
    png_u32 text_count;
    png_u32 text_capacity;

    png_unknown_chunk* unknown_chunks;
    png_u32 unknown_chunk_count;
    png_u32 unknown_chunk_capacity;

    /* Robustness flags */
    int seen_IHDR;
    int seen_PLTE;
    int seen_IDAT;
    int seen_IEND;
    int have_palette;
    int post_ihdr_non_dsig_seen;
    int trailing_dsig_started;

    png_u32 idat_bytes_seen;

} png_state;

extern const int PNG_ADAM7_X_START[7];
extern const int PNG_ADAM7_Y_START[7];
extern const int PNG_ADAM7_X_STEP[7];
extern const int PNG_ADAM7_Y_STEP[7];

png_u32 png_crc32(const png_u8* buf, png_u32 len);
png_u32 png_crc32_start(void);
png_u32 png_crc32_update(png_u32 crc, const png_u8* buf, png_u32 len);
png_u32 png_crc32_finish(png_u32 crc);

void png_unfilter_scanline(png_u8* row,
                           const png_u8* prev_row,
                           png_u32 rowbytes,
                           int filter_type,
                           png_u32 bpp);

void png_state_init(png_state* s);
void png_state_set_decode_options(png_state* s, png_zlib_decompress_func zfunc,
                                  const png_decode_options* opt,
                                  int compatibility_defaults);
void png_state_free(png_state* s);

int png_state_add_text_entry(png_state* s,
                             const char* keyword,
                             const char* text,
                             const char* language_tag,
                             const char* translated_keyword,
                             png_u8 compression);

int png_state_add_unknown_chunk(png_state* s,
                                png_u32 type,
                                const png_u8* data,
                                png_u32 length,
                                png_u8 location);
int png_state_add_dsig_chunk(png_state* s,
                             const png_u8* data,
                             png_u32 length,
                             png_u8 location);
int png_state_add_gifg_chunk(png_state* s, png_u8 disposal_method, png_u8 user_input_flag, png_u16 delay_time_cs, png_u8 location);
int png_state_add_gifx_chunk(png_state* s, const char application_identifier[8], const png_u8 authentication_code[3], const png_u8* data, png_u32 length, png_u8 location);
int png_state_add_gift_chunk(png_state* s, const png_gift_entry* entry);
int png_state_add_frac_chunk(png_state* s, const png_u8* data, png_u32 length, png_u8 location);

int png_parse_png(const png_u8* data,
                  png_u32 size,
                  png_zlib_decompress_func zfunc,
                  png_state* st,
                  png_u8** out_img_data,
                  png_u32* out_img_size);

void png_build_gamma_lut(png_state* s);

png_u32 png_rowbytes_for_width_internal(const png_state* st, png_u32 width);
png_u32 png_filter_bpp_bytes_internal(const png_state* st);
int png_render_scanline_rgba8(const png_state* st,
                              const png_u8* scan,
                              png_u8* out_rgba,
                              png_u32 width);
int png_render_scanline_rgba16(const png_state* st,
                               const png_u8* scan,
                               png_u8* out_rgba16,
                               png_u32 width);

int png_decode_image_data(const png_state* st,
                          const png_u8* img_data,
                          png_u32 img_size,
                          png_u8* out_rgba);
int png_decode_image_data16(const png_state* st,
                            const png_u8* img_data,
                            png_u32 img_size,
                            png_u8* out_rgba16);

int png_decode_image_data_rows(const png_state* st,
                               const png_u8* img_data,
                               png_u32 img_size,
                               png_u8* out_rgba,
                               png_row_callback_fn row_fn,
                               void* row_user);
int png_decode_image_data_rows16(const png_state* st,
                                 const png_u8* img_data,
                                 png_u32 img_size,
                                 png_u8* out_rgba16,
                                 png_row_callback_fn row_fn,
                                 void* row_user);

#endif /* PNG_DECODER_INTERNAL_H */
