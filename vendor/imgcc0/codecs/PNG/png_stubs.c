/* png_stubs.c
 *
 * Despite the historical name, this file now contains the real chunk handlers
 * and state management needed for decoding.
 */

#include "png_mem89.h"
#include <string.h>

#include "png_decoder_internal.h"
#include "png_chunks.h"

#ifndef PNG_FOURCC
#define PNG_FOURCC(a,b,c,d) \
    (((png_u32)(a) << 24) | ((png_u32)(b) << 16) | \
     ((png_u32)(c) <<  8) | ((png_u32)(d)))
#endif

static png_u32 read_be32(const png_u8* p)
{
    return ((png_u32)p[0] << 24) |
           ((png_u32)p[1] << 16) |
           ((png_u32)p[2] <<  8) |
           (png_u32)p[3];
}

static png_u16 read_be16(const png_u8* p)
{
    return (png_u16)(((png_u16)p[0] << 8) | (png_u16)p[1]);
}

static png_u32 sample_mask(png_u8 bit_depth)
{
    if (bit_depth >= 16u) return 0xFFFFu;
    return (1u << bit_depth) - 1u;
}

static png_u16 scale_to_16(png_u32 sample, png_u8 bit_depth)
{
    png_u32 max;

    if (bit_depth >= 16u)
        return (png_u16)sample;

    if (bit_depth == 0u)
        return 0;

    max = (1u << bit_depth) - 1u;

    /* Round to nearest: (x*65535 + max/2)/max */
    return (png_u16)((sample * 65535u + (max >> 1)) / max);
}

/* ---------------- State ---------------- */

static char* png_dup_string_n(const png_u8* src, png_u32 len)
{
    char* out;

    out = (char*)png_mem89_alloc(len + 1u);
    if (!out)
        return 0;

    if (len != 0u)
        memcpy(out, src, len);
    out[len] = '\0';
    return out;
}

static png_u8* png_dup_bytes(const png_u8* src, png_u32 len)
{
    png_u8* out;

    if (len == 0u)
        return 0;

    out = (png_u8*)png_mem89_alloc(len);
    if (!out)
        return 0;

    memcpy(out, src, len);
    return out;
}

static void png_free_text_entries(png_text_entry* entries, png_u32 count)
{
    png_u32 i;

    if (!entries)
        return;

    for (i = 0; i < count; ++i)
    {
        if (entries[i].keyword) png_mem89_release(entries[i].keyword);
        if (entries[i].text) png_mem89_release(entries[i].text);
        if (entries[i].language_tag) png_mem89_release(entries[i].language_tag);
        if (entries[i].translated_keyword) png_mem89_release(entries[i].translated_keyword);
    }

    png_mem89_release(entries);
}

static void png_free_unknown_chunks(png_unknown_chunk* chunks, png_u32 count)
{
    png_u32 i;

    if (!chunks)
        return;

    for (i = 0; i < count; ++i)
    {
        if (chunks[i].data)
            png_mem89_release(chunks[i].data);
    }

    png_mem89_release(chunks);
}

static void png_free_dsig_entries(png_dsig_entry* entries, png_u32 count)
{
    png_u32 i;

    if (!entries)
        return;

    for (i = 0; i < count; ++i)
    {
        if (entries[i].cms_data)
            png_mem89_release(entries[i].cms_data);
    }

    png_mem89_release(entries);
}

static void png_free_gifg_entries(png_gifg_entry* entries)
{
    if (entries)
        png_mem89_release(entries);
}

static void png_free_gifx_entries(png_gifx_entry* entries, png_u32 count)
{
    png_u32 i;

    if (!entries)
        return;

    for (i = 0; i < count; ++i)
    {
        if (entries[i].application_data)
            png_mem89_release(entries[i].application_data);
    }

    png_mem89_release(entries);
}

static void png_free_gift_entries(png_gift_entry* entries, png_u32 count)
{
    png_u32 i;

    if (!entries)
        return;

    for (i = 0; i < count; ++i)
    {
        if (entries[i].text_data)
            png_mem89_release(entries[i].text_data);
    }

    png_mem89_release(entries);
}

static void png_free_frac_entries(png_frac_entry* entries, png_u32 count)
{
    png_u32 i;

    if (!entries)
        return;

    for (i = 0; i < count; ++i)
    {
        if (entries[i].data)
            png_mem89_release(entries[i].data);
    }

    png_mem89_release(entries);
}

int png_exif_has_valid_tiff_header(const png_u8* data, png_u32 size)
{
    if (!data || size < 4u)
        return 0;

    if (data[0] == 0x49u && data[1] == 0x49u && data[2] == 0x2Au && data[3] == 0x00u)
        return 1;
    if (data[0] == 0x4Du && data[1] == 0x4Du && data[2] == 0x00u && data[3] == 0x2Au)
        return 1;

    return 0;
}

void png_swap_16_buffer(png_u8* data, png_u32 size)
{
    png_u32 i;

    if (!data)
        return;

    for (i = 0u; i + 1u < size; i += 2u)
    {
        png_u8 t = data[i];
        data[i] = data[i + 1u];
        data[i + 1u] = t;
    }
}

static void png_free_splt_palettes(png_splt_palette* palettes, png_u32 count)
{
    png_u32 i;

    if (!palettes)
        return;

    for (i = 0; i < count; ++i)
    {
        if (palettes[i].name)
            png_mem89_release(palettes[i].name);
        if (palettes[i].entries)
            png_mem89_release(palettes[i].entries);
    }

    png_mem89_release(palettes);
}

static void png_free_string_array(char** strings, png_u32 count)
{
    png_u32 i;

    if (!strings)
        return;

    for (i = 0; i < count; ++i)
    {
        if (strings[i])
            png_mem89_release(strings[i]);
    }

    png_mem89_release(strings);
}

static void png_free_pcal_info(png_pcal_info* pcal)
{
    if (!pcal)
        return;

    if (pcal->name) {
        png_mem89_release(pcal->name);
        pcal->name = 0;
    }
    if (pcal->unit_name) {
        png_mem89_release(pcal->unit_name);
        pcal->unit_name = 0;
    }
    png_free_string_array(pcal->params, pcal->param_count);
    pcal->params = 0;
    pcal->param_count = 0u;
    pcal->x0 = 0;
    pcal->x1 = 0;
    pcal->equation_type = 0u;
}

static int png_validate_latin1_name(const png_u8* s, png_u32 len)
{
    png_u32 i;
    png_u8 prev_space;

    if (!s || len == 0u || len > 79u)
        return 0;

    prev_space = 1u;
    for (i = 0u; i < len; ++i)
    {
        png_u8 c = s[i];
        if (!((c >= 0x20u && c <= 0x7Eu) || c >= 0xA1u))
            return 0;
        if (c == 0x20u)
        {
            if (prev_space)
                return 0;
            prev_space = 1u;
        }
        else
        {
            prev_space = 0u;
        }
    }

    return prev_space ? 0 : 1;
}

static int png_parse_ascii_fixed(const png_u8* s, png_u32 len, png_fixed89* out_value)
{
    char* tmp;
    int ok;
    if (!s || len == 0u)
        return 0;
    tmp = png_dup_string_n(s, len);
    if (!tmp)
        return 0;
    ok = png_fixed89_parse_decimal(tmp, out_value);
    png_mem89_release(tmp);
    return ok;
}

static int png_parse_positive_ascii_fixed(const png_u8* s, png_u32 len, png_fixed89* out_value)
{
    png_fixed89 v;
    if (!png_parse_ascii_fixed(s, len, &v) || v <= 0)
        return 0;
    if (out_value)
        *out_value = v;
    return 1;
}

static png_fixed89 png_fixed_from_u32_ratio(png_u32 num, png_u32 den)
{
    png_u32 whole;
    png_u32 rem;
    png_fixed89 frac;
    if (den == 0u)
        return 0;
    whole = num / den;
    rem = num % den;
    if (whole > 32767u)
        return 2147483647;
    frac = png_fixed89_from_ratio((signed int)rem, (signed int)den);
    return (png_fixed89)(whole << 16) + frac;
}

static int png_validate_latin1_text(const png_u8* s, png_u32 len, int allow_empty)
{
    png_u32 i;

    if (!s)
        return 0;
    if (!allow_empty && len == 0u)
        return 0;

    for (i = 0u; i < len; ++i)
    {
        png_u8 c = s[i];
        if (!((c >= 0x20u && c <= 0x7Eu) || c >= 0xA1u))
            return 0;
    }

    return 1;
}

static int png_validate_printable_ascii_bytes(const png_u8* s, png_u32 len)
{
    png_u32 i;

    if (!s)
        return 0;

    for (i = 0u; i < len; ++i)
    {
        if (!(s[i] >= 0x20u && s[i] <= 0x7Eu))
            return 0;
    }

    return 1;
}

static png_i32 read_be32s(const png_u8* p)
{
    png_u32 u = read_be32(p);
    if (u & 0x80000000u)
        return ((png_i32)(u & 0x7FFFFFFFu)) + (-2147483647 - 1);
    return (png_i32)u;
}

static int png_special_chunk_location(const png_state* st, png_u8* out_location)
{
    if (!st || !out_location)
        return 0;

    if (st->seen_IDAT)
        *out_location = PNG_CHUNK_POS_AFTER_IDAT;
    else if (st->seen_PLTE)
        *out_location = PNG_CHUNK_POS_AFTER_PLTE;
    else
        *out_location = PNG_CHUNK_POS_AFTER_IHDR;

    return 1;
}

void png_state_init(png_state* s)
{
    if (!s) return;
    memset(s, 0, sizeof(*s));
}

void png_state_set_decode_options(png_state* s, png_zlib_decompress_func zfunc,
                                  const png_decode_options* opt,
                                  int compatibility_defaults)
{
    if (!s)
        return;

    s->zfunc = zfunc;

    if (compatibility_defaults)
    {
        s->transform_flags = PNG_DEC_TRANSFORM_APPLY_GAMMA;
        s->keep_text = 0;
        s->keep_unknown_chunks = PNG_DEC_KEEP_UNKNOWN_NEVER;
        s->strict_trailing_data = 1;
    }
    else
    {
        s->transform_flags = PNG_DEC_TRANSFORM_NONE;
        s->keep_text = 1;
        s->keep_unknown_chunks = PNG_DEC_KEEP_UNKNOWN_SAFE;
        s->strict_trailing_data = 1;
    }

    s->max_width = PNG_DEC_MAX_WIDTH;
    s->max_height = PNG_DEC_MAX_HEIGHT;
    s->max_image_bytes = PNG_DEC_MAX_IMAGE_BYTES;
    s->max_file_bytes = PNG_DEC_MAX_FILE_BYTES;
    s->max_text_entries = PNG_DEC_MAX_TEXT_ENTRIES;
    s->max_unknown_chunks = PNG_DEC_MAX_UNKNOWN_CHUNKS;
    s->max_chunk_bytes = PNG_DEC_MAX_FILE_BYTES;
    s->max_pixels = PNG_DEC_MAX_PIXELS;
    s->max_inflated_bytes = PNG_DEC_MAX_INFLATED_BYTES;
    s->max_chunks = PNG_DEC_MAX_CHUNKS;
    s->max_text_bytes = PNG_DEC_MAX_TEXT_BYTES;
    s->max_apng_frames = PNG_DEC_MAX_APNG_FRAMES;
    s->max_temp_bytes = PNG_DEC_MAX_TEMP_BYTES;
    s->max_conversion_expansion = PNG_DEC_MAX_CONVERSION_EXPANSION;

    if (opt)
    {
        s->transform_flags = opt->transform_flags;
        s->keep_text = opt->keep_text;
        s->keep_unknown_chunks = opt->keep_unknown_chunks;
        s->strict_trailing_data = opt->strict_trailing_data;
        if (opt->max_width != 0u) s->max_width = opt->max_width;
        if (opt->max_height != 0u) s->max_height = opt->max_height;
        if (opt->max_image_bytes != 0u) s->max_image_bytes = opt->max_image_bytes;
        if (opt->max_file_bytes != 0u) s->max_file_bytes = opt->max_file_bytes;
        if (opt->max_text_entries != 0u) s->max_text_entries = opt->max_text_entries;
        if (opt->max_unknown_chunks != 0u) s->max_unknown_chunks = opt->max_unknown_chunks;
        if (opt->max_chunk_bytes != 0u) s->max_chunk_bytes = opt->max_chunk_bytes;
        if (opt->max_pixels != 0u) s->max_pixels = opt->max_pixels;
        if (opt->max_inflated_bytes != 0u) s->max_inflated_bytes = opt->max_inflated_bytes;
        if (opt->max_chunks != 0u) s->max_chunks = opt->max_chunks;
        if (opt->max_text_bytes != 0u) s->max_text_bytes = opt->max_text_bytes;
        if (opt->max_apng_frames != 0u) s->max_apng_frames = opt->max_apng_frames;
        if (opt->max_temp_bytes != 0u) s->max_temp_bytes = opt->max_temp_bytes;
        if (opt->max_conversion_expansion != 0u) s->max_conversion_expansion = opt->max_conversion_expansion;
    }
}

void png_state_free(png_state* s)
{
    if (!s) return;

    if (s->palette) {
        png_mem89_release(s->palette);
        s->palette = 0;
    }
    if (s->trns_data) {
        png_mem89_release(s->trns_data);
        s->trns_data = 0;
    }
    if (s->idat_data) {
        png_mem89_release(s->idat_data);
        s->idat_data = 0;
    }
    if (s->iccp_name) {
        png_mem89_release(s->iccp_name);
        s->iccp_name = 0;
    }
    if (s->iccp_profile) {
        png_mem89_release(s->iccp_profile);
        s->iccp_profile = 0;
    }
    if (s->exif_profile) {
        png_mem89_release(s->exif_profile);
        s->exif_profile = 0;
    }
    if (s->scal_pixel_width) {
        png_mem89_release(s->scal_pixel_width);
        s->scal_pixel_width = 0;
    }
    if (s->scal_pixel_height) {
        png_mem89_release(s->scal_pixel_height);
        s->scal_pixel_height = 0;
    }
    if (s->hist_entries) {
        png_mem89_release(s->hist_entries);
        s->hist_entries = 0;
    }
    png_free_pcal_info(&s->pcal);
    png_free_dsig_entries(s->dsig_chunks, s->dsig_chunk_count);
    png_free_gifg_entries(s->gifg_chunks);
    png_free_gifx_entries(s->gifx_chunks, s->gifx_chunk_count);
    png_free_gift_entries(s->gift_chunks, s->gift_chunk_count);
    png_free_frac_entries(s->frac_chunks, s->frac_chunk_count);
    png_free_splt_palettes(s->splt_palettes, s->splt_palette_count);
    png_free_text_entries(s->text_entries, s->text_count);
    png_free_unknown_chunks(s->unknown_chunks, s->unknown_chunk_count);

    memset(s, 0, sizeof(*s));
}

int png_state_add_text_entry(png_state* s,
                             const char* keyword,
                             const char* text,
                             const char* language_tag,
                             const char* translated_keyword,
                             png_u8 compression)
{
    png_text_entry* new_entries;
    png_u32 newcap;
    png_text_entry* e;
    char* dup_keyword;
    char* dup_text;
    char* dup_lang;
    char* dup_translated;

    if (!s || !keyword || !text)
        return PNG_DEC_ERR_FORMAT;

    if (!s->keep_text)
        return PNG_DEC_OK;

    if (s->text_count >= s->max_text_entries)
        return PNG_DEC_ERR_TEXT_TOO_LARGE;

    dup_keyword = png_dup_string_n((const png_u8*)keyword, (png_u32)strlen(keyword));
    if (!dup_keyword)
        return PNG_DEC_ERR_OOM;

    dup_text = png_dup_string_n((const png_u8*)text, (png_u32)strlen(text));
    if (!dup_text) {
        png_mem89_release(dup_keyword);
        return PNG_DEC_ERR_OOM;
    }

    dup_lang = 0;
    dup_translated = 0;

    if (language_tag && language_tag[0])
    {
        dup_lang = png_dup_string_n((const png_u8*)language_tag,
                                    (png_u32)strlen(language_tag));
        if (!dup_lang) {
            png_mem89_release(dup_keyword);
            png_mem89_release(dup_text);
            return PNG_DEC_ERR_OOM;
        }
    }

    if (translated_keyword && translated_keyword[0])
    {
        dup_translated = png_dup_string_n((const png_u8*)translated_keyword,
                                          (png_u32)strlen(translated_keyword));
        if (!dup_translated) {
            png_mem89_release(dup_keyword);
            png_mem89_release(dup_text);
            if (dup_lang) png_mem89_release(dup_lang);
            return PNG_DEC_ERR_OOM;
        }
    }

    {
        png_u32 added_bytes = (png_u32)strlen(dup_keyword) + (png_u32)strlen(dup_text);
        if (dup_lang)
            added_bytes += (png_u32)strlen(dup_lang);
        if (dup_translated)
            added_bytes += (png_u32)strlen(dup_translated);
        if (added_bytes > s->max_text_bytes || s->text_bytes > s->max_text_bytes - added_bytes) {
            png_mem89_release(dup_keyword);
            png_mem89_release(dup_text);
            if (dup_lang) png_mem89_release(dup_lang);
            if (dup_translated) png_mem89_release(dup_translated);
            return PNG_DEC_ERR_TEXT_TOO_LARGE;
        }
    }

    if (s->text_count == s->text_capacity)
    {
        newcap = (s->text_capacity == 0u) ? 8u : s->text_capacity * 2u;
        if (newcap > s->max_text_entries)
            newcap = s->max_text_entries;

        new_entries = (png_text_entry*)png_mem89_resize(s->text_entries,
                                               newcap * sizeof(png_text_entry));
        if (!new_entries) {
            png_mem89_release(dup_keyword);
            png_mem89_release(dup_text);
            if (dup_lang) png_mem89_release(dup_lang);
            if (dup_translated) png_mem89_release(dup_translated);
            return PNG_DEC_ERR_OOM;
        }

        memset(new_entries + s->text_capacity, 0,
               (newcap - s->text_capacity) * sizeof(png_text_entry));
        s->text_entries = new_entries;
        s->text_capacity = newcap;
    }

    e = &s->text_entries[s->text_count];
    memset(e, 0, sizeof(*e));
    e->keyword = dup_keyword;
    e->text = dup_text;
    e->language_tag = dup_lang;
    e->translated_keyword = dup_translated;
    e->compression = compression;
    s->text_count += 1u;
    s->text_bytes += (png_u32)strlen(dup_keyword) + (png_u32)strlen(dup_text);
    if (dup_lang)
        s->text_bytes += (png_u32)strlen(dup_lang);
    if (dup_translated)
        s->text_bytes += (png_u32)strlen(dup_translated);
    return PNG_DEC_OK;
}

int png_state_add_unknown_chunk(png_state* s,
                                png_u32 type,
                                const png_u8* data,
                                png_u32 length,
                                png_u8 location)
{
    png_unknown_chunk* new_chunks;
    png_u32 newcap;
    png_unknown_chunk* c;

    if (!s)
        return PNG_DEC_ERR_FORMAT;

    if (s->unknown_chunk_count >= s->max_unknown_chunks)
        return PNG_DEC_ERR_TOO_MANY_CHUNKS;

    if (length > s->max_chunk_bytes)
        return PNG_DEC_ERR_CHUNK_TOO_LARGE;

    if (s->unknown_chunk_count == s->unknown_chunk_capacity)
    {
        newcap = (s->unknown_chunk_capacity == 0u) ? 8u : s->unknown_chunk_capacity * 2u;
        if (newcap > s->max_unknown_chunks)
            newcap = s->max_unknown_chunks;

        new_chunks = (png_unknown_chunk*)png_mem89_resize(s->unknown_chunks,
                                                 newcap * sizeof(png_unknown_chunk));
        if (!new_chunks)
            return PNG_DEC_ERR_OOM;

        memset(new_chunks + s->unknown_chunk_capacity, 0,
               (newcap - s->unknown_chunk_capacity) * sizeof(png_unknown_chunk));
        s->unknown_chunks = new_chunks;
        s->unknown_chunk_capacity = newcap;
    }

    c = &s->unknown_chunks[s->unknown_chunk_count];
    memset(c, 0, sizeof(*c));

    c->type[0] = (png_u8)((type >> 24) & 0xFFu);
    c->type[1] = (png_u8)((type >> 16) & 0xFFu);
    c->type[2] = (png_u8)((type >>  8) & 0xFFu);
    c->type[3] = (png_u8)( type        & 0xFFu);
    c->type[4] = '\0';
    c->location = location;
    c->safe_to_copy = (png_u8)((type & 0x20u) ? 1u : 0u);
    c->size = length;
    c->data = png_dup_bytes(data, length);
    if (length != 0u && !c->data)
        return PNG_DEC_ERR_OOM;

    s->unknown_chunk_count += 1u;
    return PNG_DEC_OK;
}


int png_state_add_dsig_chunk(png_state* s,
                             const png_u8* data,
                             png_u32 length,
                             png_u8 location)
{
    png_dsig_entry* new_chunks;
    png_u32 newcap;
    png_dsig_entry* c;

    if (!s || !(location == PNG_CHUNK_POS_AFTER_IHDR || location == PNG_CHUNK_POS_BEFORE_IEND))
        return PNG_DEC_ERR_FORMAT;

    if (s->dsig_chunk_count >= s->max_unknown_chunks)
        return PNG_DEC_ERR_TOO_MANY_CHUNKS;

    if (length > s->max_chunk_bytes)
        return PNG_DEC_ERR_CHUNK_TOO_LARGE;

    if (s->dsig_chunk_count == s->dsig_chunk_capacity)
    {
        newcap = (s->dsig_chunk_capacity == 0u) ? 4u : s->dsig_chunk_capacity * 2u;
        if (newcap > s->max_unknown_chunks)
            newcap = s->max_unknown_chunks;

        new_chunks = (png_dsig_entry*)png_mem89_resize(s->dsig_chunks,
                                              newcap * sizeof(png_dsig_entry));
        if (!new_chunks)
            return PNG_DEC_ERR_OOM;

        memset(new_chunks + s->dsig_chunk_capacity, 0,
               (newcap - s->dsig_chunk_capacity) * sizeof(png_dsig_entry));
        s->dsig_chunks = new_chunks;
        s->dsig_chunk_capacity = newcap;
    }

    c = &s->dsig_chunks[s->dsig_chunk_count];
    memset(c, 0, sizeof(*c));
    c->location = location;
    c->size = length;
    c->cms_data = png_dup_bytes(data, length);
    if (length != 0u && !c->cms_data)
        return PNG_DEC_ERR_OOM;

    s->dsig_chunk_count += 1u;
    return PNG_DEC_OK;
}



int png_state_add_gifg_chunk(png_state* s, png_u8 disposal_method, png_u8 user_input_flag, png_u16 delay_time_cs, png_u8 location)
{
    png_gifg_entry* new_chunks;
    png_u32 newcap;
    png_gifg_entry* c;

    if (!s || !(location == PNG_CHUNK_POS_AFTER_IHDR || location == PNG_CHUNK_POS_AFTER_PLTE || location == PNG_CHUNK_POS_AFTER_IDAT))
        return PNG_DEC_ERR_FORMAT;
    if (s->gifg_chunk_count >= s->max_unknown_chunks)
        return PNG_DEC_ERR_TOO_MANY_CHUNKS;

    if (s->gifg_chunk_count == s->gifg_chunk_capacity)
    {
        newcap = (s->gifg_chunk_capacity == 0u) ? 4u : s->gifg_chunk_capacity * 2u;
        if (newcap > s->max_unknown_chunks)
            newcap = s->max_unknown_chunks;
        new_chunks = (png_gifg_entry*)png_mem89_resize(s->gifg_chunks, newcap * sizeof(png_gifg_entry));
        if (!new_chunks)
            return PNG_DEC_ERR_OOM;
        memset(new_chunks + s->gifg_chunk_capacity, 0,
               (newcap - s->gifg_chunk_capacity) * sizeof(png_gifg_entry));
        s->gifg_chunks = new_chunks;
        s->gifg_chunk_capacity = newcap;
    }

    c = &s->gifg_chunks[s->gifg_chunk_count];
    memset(c, 0, sizeof(*c));
    c->disposal_method = disposal_method;
    c->user_input_flag = user_input_flag;
    c->delay_time_cs = delay_time_cs;
    c->location = location;
    s->gifg_chunk_count += 1u;
    return PNG_DEC_OK;
}

int png_state_add_gifx_chunk(png_state* s,
                             const char application_identifier[8],
                             const png_u8 authentication_code[3],
                             const png_u8* data,
                             png_u32 length,
                             png_u8 location)
{
    png_gifx_entry* new_chunks;
    png_u32 newcap;
    png_gifx_entry* c;

    if (!s || !application_identifier || !authentication_code ||
        !(location == PNG_CHUNK_POS_AFTER_IHDR || location == PNG_CHUNK_POS_AFTER_PLTE || location == PNG_CHUNK_POS_AFTER_IDAT))
        return PNG_DEC_ERR_FORMAT;
    if (s->gifx_chunk_count >= s->max_unknown_chunks)
        return PNG_DEC_ERR_TOO_MANY_CHUNKS;
    if (length > s->max_chunk_bytes)
        return PNG_DEC_ERR_CHUNK_TOO_LARGE;

    if (s->gifx_chunk_count == s->gifx_chunk_capacity)
    {
        newcap = (s->gifx_chunk_capacity == 0u) ? 4u : s->gifx_chunk_capacity * 2u;
        if (newcap > s->max_unknown_chunks)
            newcap = s->max_unknown_chunks;
        new_chunks = (png_gifx_entry*)png_mem89_resize(s->gifx_chunks, newcap * sizeof(png_gifx_entry));
        if (!new_chunks)
            return PNG_DEC_ERR_OOM;
        memset(new_chunks + s->gifx_chunk_capacity, 0,
               (newcap - s->gifx_chunk_capacity) * sizeof(png_gifx_entry));
        s->gifx_chunks = new_chunks;
        s->gifx_chunk_capacity = newcap;
    }

    c = &s->gifx_chunks[s->gifx_chunk_count];
    memset(c, 0, sizeof(*c));
    memcpy(c->application_identifier, application_identifier, 8u);
    c->application_identifier[8] = '\0';
    memcpy(c->authentication_code, authentication_code, 3u);
    c->application_data = png_dup_bytes(data, length);
    if (length != 0u && !c->application_data)
        return PNG_DEC_ERR_OOM;
    c->application_data_size = length;
    c->location = location;
    s->gifx_chunk_count += 1u;
    return PNG_DEC_OK;
}

int png_state_add_gift_chunk(png_state* s, const png_gift_entry* entry)
{
    png_gift_entry* new_chunks;
    png_u32 newcap;
    png_gift_entry* c;

    if (!s || !entry ||
        !(entry->location == PNG_CHUNK_POS_AFTER_IHDR || entry->location == PNG_CHUNK_POS_AFTER_PLTE || entry->location == PNG_CHUNK_POS_AFTER_IDAT))
        return PNG_DEC_ERR_FORMAT;
    if (s->gift_chunk_count >= s->max_unknown_chunks)
        return PNG_DEC_ERR_TOO_MANY_CHUNKS;
    if (entry->text_data_size > s->max_chunk_bytes)
        return PNG_DEC_ERR_CHUNK_TOO_LARGE;

    if (s->gift_chunk_count == s->gift_chunk_capacity)
    {
        newcap = (s->gift_chunk_capacity == 0u) ? 4u : s->gift_chunk_capacity * 2u;
        if (newcap > s->max_unknown_chunks)
            newcap = s->max_unknown_chunks;
        new_chunks = (png_gift_entry*)png_mem89_resize(s->gift_chunks, newcap * sizeof(png_gift_entry));
        if (!new_chunks)
            return PNG_DEC_ERR_OOM;
        memset(new_chunks + s->gift_chunk_capacity, 0,
               (newcap - s->gift_chunk_capacity) * sizeof(png_gift_entry));
        s->gift_chunks = new_chunks;
        s->gift_chunk_capacity = newcap;
    }

    c = &s->gift_chunks[s->gift_chunk_count];
    memset(c, 0, sizeof(*c));
    *c = *entry;
    c->text_data = png_dup_bytes(entry->text_data, entry->text_data_size);
    if (entry->text_data_size != 0u && !c->text_data)
        return PNG_DEC_ERR_OOM;
    s->gift_chunk_count += 1u;
    return PNG_DEC_OK;
}

int png_state_add_frac_chunk(png_state* s, const png_u8* data, png_u32 length, png_u8 location)
{
    png_frac_entry* new_chunks;
    png_u32 newcap;
    png_frac_entry* c;

    if (!s || !(location == PNG_CHUNK_POS_AFTER_IHDR || location == PNG_CHUNK_POS_AFTER_PLTE || location == PNG_CHUNK_POS_AFTER_IDAT))
        return PNG_DEC_ERR_FORMAT;
    if (s->frac_chunk_count >= s->max_unknown_chunks)
        return PNG_DEC_ERR_TOO_MANY_CHUNKS;
    if (length > s->max_chunk_bytes)
        return PNG_DEC_ERR_CHUNK_TOO_LARGE;

    if (s->frac_chunk_count == s->frac_chunk_capacity)
    {
        newcap = (s->frac_chunk_capacity == 0u) ? 4u : s->frac_chunk_capacity * 2u;
        if (newcap > s->max_unknown_chunks)
            newcap = s->max_unknown_chunks;
        new_chunks = (png_frac_entry*)png_mem89_resize(s->frac_chunks, newcap * sizeof(png_frac_entry));
        if (!new_chunks)
            return PNG_DEC_ERR_OOM;
        memset(new_chunks + s->frac_chunk_capacity, 0,
               (newcap - s->frac_chunk_capacity) * sizeof(png_frac_entry));
        s->frac_chunks = new_chunks;
        s->frac_chunk_capacity = newcap;
    }

    c = &s->frac_chunks[s->frac_chunk_count];
    memset(c, 0, sizeof(*c));
    c->data = png_dup_bytes(data, length);
    if (length != 0u && !c->data)
        return PNG_DEC_ERR_OOM;
    c->size = length;
    c->location = location;
    s->frac_chunk_count += 1u;
    return PNG_DEC_OK;
}
static int png_try_decompress_unknown_size(png_zlib_decompress_func zfunc,
                                           const png_u8* src,
                                           png_u32 src_len,
                                           png_u8** out_buf,
                                           png_u32* out_len)
{
    png_u32 cap;
    png_u8* buf;

    if (!out_buf || !out_len || !zfunc)
        return PNG_DEC_ERR_FORMAT;

    *out_buf = 0;
    *out_len = 0u;

    cap = (src_len == 0u) ? 64u : (src_len * 4u + 64u);
    if (cap < 64u)
        cap = 64u;

    while (cap <= PNG_DEC_MAX_FILE_BYTES)
    {
        png_u32 dest_len;
        int zerr;

        buf = (png_u8*)png_mem89_alloc(cap);
        if (!buf)
            return PNG_DEC_ERR_OOM;

        dest_len = cap;
        zerr = zfunc(buf, &dest_len, src, src_len);
        if (zerr == 0)
        {
            *out_buf = buf;
            *out_len = dest_len;
            return PNG_DEC_OK;
        }

        png_mem89_release(buf);

        if (cap > PNG_DEC_MAX_FILE_BYTES / 2u)
            break;
        cap *= 2u;
    }

    return PNG_DEC_ERR_ZLIB;
}

/* IDAT append: concatenate IDAT payloads into a single buffer */
int png_idat_append(png_state* st, const png_u8* data, png_u32 length)
{
    png_u32 needed;
    png_u32 newcap;
    png_u8*  newbuf;

    if (!st)
        return PNG_DEC_ERR_FORMAT;

    if (length == 0u)
        return PNG_DEC_OK;

    if (!data)
        return PNG_DEC_ERR_FORMAT;

    /* Global safety */
    if (st->idat_size > st->max_file_bytes || length > st->max_file_bytes)
        return PNG_DEC_ERR_CHUNK_TOO_LARGE;

    if (st->idat_size > (png_u32)(st->max_file_bytes - length))
        return PNG_DEC_ERR_CHUNK_TOO_LARGE;

    needed = st->idat_size + length;

    if (needed > st->idat_capacity) {
        newcap = (st->idat_capacity == 0u) ? 4096u : st->idat_capacity;
        while (newcap < needed) {
            if (newcap > (png_u32)(st->max_temp_bytes / 2u))
                return PNG_DEC_ERR_TEMP_MEMORY_LIMIT;
            newcap *= 2u;
        }

        newbuf = (png_u8*)png_mem89_resize(st->idat_data, newcap);
        if (!newbuf)
            return PNG_DEC_ERR_OOM;

        st->idat_data = newbuf;
        st->idat_capacity = newcap;
    }

    memcpy(st->idat_data + st->idat_size, data, length);
    st->idat_size += length;

    return PNG_DEC_OK;
}

/* ---------------- Chunk handlers ---------------- */

int png_handle_IHDR(png_state* st, const png_u8* d, png_u32 length)
{
    png_u8 bd, ct, cm, fm, im;

    if (!st || !d)
        return PNG_DEC_ERR_FORMAT;

    if (st->seen_IHDR)
        return PNG_DEC_ERR_FORMAT;

    /* IHDR must be first chunk (enforced by png_handle_chunk), length fixed */
    if (length != 13u)
        return PNG_DEC_ERR_FORMAT;

    st->width  = read_be32(d + 0);
    st->height = read_be32(d + 4);

    bd = d[8];
    ct = d[9];
    cm = d[10];
    fm = d[11];
    im = d[12];

    if (st->width == 0u || st->height == 0u)
        return PNG_DEC_ERR_FORMAT;

    if (st->width > st->max_width || st->height > st->max_height)
        return PNG_DEC_ERR_DIMENSIONS_TOO_LARGE;

    if (st->height != 0u && st->width > st->max_pixels / st->height)
        return PNG_DEC_ERR_TOO_MANY_PIXELS;

    /* Validate bit depth and color type combinations (PNG spec Table 12). */
    switch (ct)
    {
        case PNG_COLOR_GRAYSCALE:
            if (!(bd == 1u || bd == 2u || bd == 4u || bd == 8u || bd == 16u))
                return PNG_DEC_ERR_UNSUPPORTED;
            break;

        case PNG_COLOR_TRUECOLOR:
            if (!(bd == 8u || bd == 16u))
                return PNG_DEC_ERR_UNSUPPORTED;
            break;

        case PNG_COLOR_INDEXED:
            if (!(bd == 1u || bd == 2u || bd == 4u || bd == 8u))
                return PNG_DEC_ERR_UNSUPPORTED;
            break;

        case PNG_COLOR_GRAYSCALE_ALPHA:
            if (!(bd == 8u || bd == 16u))
                return PNG_DEC_ERR_UNSUPPORTED;
            break;

        case PNG_COLOR_TRUECOLOR_ALPHA:
            if (!(bd == 8u || bd == 16u))
                return PNG_DEC_ERR_UNSUPPORTED;
            break;

        default:
            return PNG_DEC_ERR_UNSUPPORTED;
    }

    /* Only compression method 0 (deflate), filter method 0, interlace 0/1. */
    if (cm != 0u || fm != 0u)
        return PNG_DEC_ERR_UNSUPPORTED;

    if (!(im == 0u || im == 1u))
        return PNG_DEC_ERR_UNSUPPORTED;

    st->bit_depth = bd;
    st->color_type = ct;
    st->compression_method = cm;
    st->filter_method = fm;
    st->interlace_method = im;

    st->seen_IHDR = 1;
    return PNG_DEC_OK;
}

int png_handle_PLTE(png_state* st, const png_u8* d, png_u32 length)
{
    png_u32 entries;
    png_u8* pal;

    if (!st || !d)
        return PNG_DEC_ERR_FORMAT;

    if (!st->seen_IHDR)
        return PNG_DEC_ERR_FORMAT;

    /* PLTE must be before first IDAT */
    if (st->seen_IDAT)
        return PNG_DEC_ERR_FORMAT;

    if (st->seen_PLTE)
        return PNG_DEC_ERR_FORMAT;

    /* Per spec: PLTE shall not appear for color types 0 and 4. */
    if (st->color_type == PNG_COLOR_GRAYSCALE || st->color_type == PNG_COLOR_GRAYSCALE_ALPHA)
        return PNG_DEC_ERR_FORMAT;

    if (length == 0u || (length % 3u) != 0u)
        return PNG_DEC_ERR_FORMAT;

    entries = length / 3u;
    if (entries == 0u || entries > 256u)
        return PNG_DEC_ERR_FORMAT;

    /* Indexed-color: entries must fit in bit_depth range (<= 2^bit_depth). */
    if (st->color_type == PNG_COLOR_INDEXED) {
        png_u32 max_entries = 1u << st->bit_depth;
        if (entries > max_entries)
            return PNG_DEC_ERR_FORMAT;
    }

    pal = (png_u8*)png_mem89_alloc(length);
    if (!pal)
        return PNG_DEC_ERR_OOM;

    memcpy(pal, d, length);

    if (st->palette)
        png_mem89_release(st->palette);

    st->palette = pal;
    st->palette_entries = entries;
    st->have_palette = 1;
    st->seen_PLTE = 1;

    return PNG_DEC_OK;
}

int png_handle_IDAT(png_state* st, const png_u8* d, png_u32 length)
{
    int err;

    if (!st)
        return PNG_DEC_ERR_FORMAT;

    if (!st->seen_IHDR)
        return PNG_DEC_ERR_FORMAT;

    if (st->seen_IEND)
        return PNG_DEC_ERR_FORMAT;

    /* Enforce: IDAT chunks must be consecutive. */
    if (st->idat_finished)
        return PNG_DEC_ERR_FORMAT;

    err = png_idat_append(st, d, length);
    if (err != PNG_DEC_OK)
        return err;

    st->seen_IDAT = 1;
    return PNG_DEC_OK;
}

int png_handle_IEND(png_state* st, const png_u8* d, png_u32 length)
{
    (void)d;

    if (!st)
        return PNG_DEC_ERR_FORMAT;

    if (!st->seen_IHDR)
        return PNG_DEC_ERR_FORMAT;

    if (!st->seen_IDAT)
        return PNG_DEC_ERR_FORMAT;

    if (length != 0u)
        return PNG_DEC_ERR_FORMAT;

    st->seen_IEND = 1;
    return PNG_DEC_OK;
}

int png_handle_tRNS(png_state* st, const png_u8* d, png_u32 length)
{
    png_u8* t;

    if (!st || (!d && length != 0u))
        return PNG_DEC_ERR_FORMAT;

    if (!st->seen_IHDR)
        return PNG_DEC_ERR_FORMAT;

    /* Must be before first IDAT (per ordering rules). */
    if (st->seen_IDAT)
        return PNG_DEC_ERR_FORMAT;

    /* Only one. */
    if (st->has_trns)
        return PNG_DEC_ERR_FORMAT;

    /* Not allowed for images that already have an alpha channel. */
    if (st->color_type == PNG_COLOR_GRAYSCALE_ALPHA || st->color_type == PNG_COLOR_TRUECOLOR_ALPHA)
        return PNG_DEC_ERR_FORMAT;

    if (st->color_type == PNG_COLOR_INDEXED)
    {
        if (!st->have_palette || st->palette_entries == 0u)
            return PNG_DEC_ERR_FORMAT;

        if (length > st->palette_entries)
            return PNG_DEC_ERR_FORMAT;

        /* length may be 0..palette_entries (0 means: all opaque) */
    }
    else if (st->color_type == PNG_COLOR_GRAYSCALE)
    {
        if (length != 2u)
            return PNG_DEC_ERR_FORMAT;
    }
    else if (st->color_type == PNG_COLOR_TRUECOLOR)
    {
        if (length != 6u)
            return PNG_DEC_ERR_FORMAT;
    }
    else
    {
        return PNG_DEC_ERR_FORMAT;
    }

    if (length == 0u) {
        st->has_trns = 1;
        st->trns_data = 0;
        st->trns_size = 0u;
        return PNG_DEC_OK;
    }

    t = (png_u8*)png_mem89_alloc(length);
    if (!t)
        return PNG_DEC_ERR_OOM;

    memcpy(t, d, length);

    st->trns_data = t;
    st->trns_size = length;
    st->has_trns = 1;

    return PNG_DEC_OK;
}

int png_handle_gAMA(png_state* st, const png_u8* d, png_u32 length)
{
    png_u32 ugamma;

    if (!st || !d)
        return PNG_DEC_ERR_FORMAT;

    if (!st->seen_IHDR)
        return PNG_DEC_ERR_FORMAT;

    /* Must be before PLTE and IDAT */
    if (st->seen_PLTE || st->seen_IDAT)
        return PNG_DEC_ERR_FORMAT;

    if (st->have_gama)
        return PNG_DEC_ERR_FORMAT;

    if (length != 4u)
        return PNG_DEC_ERR_FORMAT;

    ugamma = read_be32(d);
    if (ugamma == 0u)
        return PNG_DEC_OK; /* ignore invalid */

    st->have_gama = 1;
    st->image_gamma = png_fixed_from_u32_ratio(ugamma, 100000u);

    return PNG_DEC_OK;
}

int png_handle_cHRM(png_state* st, const png_u8* d, png_u32 length)
{
    png_u32 wx, wy, rx, ry, gx, gy, bx, by;

    if (!st || !d)
        return PNG_DEC_ERR_FORMAT;

    if (!st->seen_IHDR)
        return PNG_DEC_ERR_FORMAT;

    /* Must be before PLTE and IDAT */
    if (st->seen_PLTE || st->seen_IDAT)
        return PNG_DEC_ERR_FORMAT;

    if (st->have_chrm)
        return PNG_DEC_ERR_FORMAT;

    if (length != 32u)
        return PNG_DEC_ERR_FORMAT;

    wx = read_be32(d +  0);
    wy = read_be32(d +  4);
    rx = read_be32(d +  8);
    ry = read_be32(d + 12);
    gx = read_be32(d + 16);
    gy = read_be32(d + 20);
    bx = read_be32(d + 24);
    by = read_be32(d + 28);

    /* Values represent x/y * 100000. Be tolerant; ignore obviously insane values. */
    if (wx > 100000u || wy > 100000u || rx > 100000u || ry > 100000u ||
        gx > 100000u || gy > 100000u || bx > 100000u || by > 100000u)
        return PNG_DEC_OK;

    st->have_chrm = 1;
    st->white_x = png_fixed_from_u32_ratio(wx, 100000u);
    st->white_y = png_fixed_from_u32_ratio(wy, 100000u);
    st->red_x   = png_fixed_from_u32_ratio(rx, 100000u);
    st->red_y   = png_fixed_from_u32_ratio(ry, 100000u);
    st->green_x = png_fixed_from_u32_ratio(gx, 100000u);
    st->green_y = png_fixed_from_u32_ratio(gy, 100000u);
    st->blue_x  = png_fixed_from_u32_ratio(bx, 100000u);
    st->blue_y  = png_fixed_from_u32_ratio(by, 100000u);

    return PNG_DEC_OK;
}

int png_handle_cICP(png_state* st, const png_u8* d, png_u32 length)
{
    if (!st || !d)
        return PNG_DEC_ERR_FORMAT;
    if (!st->seen_IHDR)
        return PNG_DEC_ERR_FORMAT;
    if (st->seen_PLTE || st->seen_IDAT)
        return PNG_DEC_ERR_FORMAT;
    if (st->have_cicp)
        return PNG_DEC_ERR_FORMAT;
    if (length != 4u)
        return PNG_DEC_ERR_FORMAT;
    if (!(d[3] == 0u || d[3] == 1u))
        return PNG_DEC_ERR_FORMAT;

    st->have_cicp = 1;
    st->cicp.colour_primaries = d[0];
    st->cicp.transfer_function = d[1];
    st->cicp.matrix_coefficients = d[2];
    st->cicp.full_range_flag = d[3];
    return PNG_DEC_OK;
}

int png_handle_mDCV(png_state* st, const png_u8* d, png_u32 length)
{
    if (!st || !d)
        return PNG_DEC_ERR_FORMAT;
    if (!st->seen_IHDR)
        return PNG_DEC_ERR_FORMAT;
    if (st->seen_PLTE || st->seen_IDAT)
        return PNG_DEC_ERR_FORMAT;
    if (st->have_mdcv)
        return PNG_DEC_ERR_FORMAT;
    if (length != 24u)
        return PNG_DEC_ERR_FORMAT;

    st->have_mdcv = 1;
    st->mdcv.display_primaries_x[0] = png_fixed_from_u32_ratio((png_u32)read_be16(d + 0u), 50000u);
    st->mdcv.display_primaries_y[0] = png_fixed_from_u32_ratio((png_u32)read_be16(d + 2u), 50000u);
    st->mdcv.display_primaries_x[1] = png_fixed_from_u32_ratio((png_u32)read_be16(d + 4u), 50000u);
    st->mdcv.display_primaries_y[1] = png_fixed_from_u32_ratio((png_u32)read_be16(d + 6u), 50000u);
    st->mdcv.display_primaries_x[2] = png_fixed_from_u32_ratio((png_u32)read_be16(d + 8u), 50000u);
    st->mdcv.display_primaries_y[2] = png_fixed_from_u32_ratio((png_u32)read_be16(d + 10u), 50000u);
    st->mdcv.white_point_x = png_fixed_from_u32_ratio((png_u32)read_be16(d + 12u), 50000u);
    st->mdcv.white_point_y = png_fixed_from_u32_ratio((png_u32)read_be16(d + 14u), 50000u);
    st->mdcv.max_luminance = png_fixed_from_u32_ratio(read_be32(d + 16u), 10000u);
    st->mdcv.min_luminance = png_fixed_from_u32_ratio(read_be32(d + 20u), 10000u);
    return PNG_DEC_OK;
}

int png_handle_cLLI(png_state* st, const png_u8* d, png_u32 length)
{
    if (!st || !d)
        return PNG_DEC_ERR_FORMAT;
    if (!st->seen_IHDR)
        return PNG_DEC_ERR_FORMAT;
    if (st->seen_PLTE || st->seen_IDAT)
        return PNG_DEC_ERR_FORMAT;
    if (st->have_clli)
        return PNG_DEC_ERR_FORMAT;
    if (length != 8u)
        return PNG_DEC_ERR_FORMAT;

    st->have_clli = 1;
    st->clli.max_content_light_level = png_fixed_from_u32_ratio(read_be32(d + 0u), 10000u);
    st->clli.max_frame_average_light_level = png_fixed_from_u32_ratio(read_be32(d + 4u), 10000u);
    return PNG_DEC_OK;
}

int png_handle_sRGB(png_state* st, const png_u8* d, png_u32 length)
{
    if (!st || !d)
        return PNG_DEC_ERR_FORMAT;

    if (!st->seen_IHDR)
        return PNG_DEC_ERR_FORMAT;

    /* Must be before PLTE and IDAT */
    if (st->seen_PLTE || st->seen_IDAT)
        return PNG_DEC_ERR_FORMAT;

    if (st->have_srgb)
        return PNG_DEC_ERR_FORMAT;

    if (length != 1u)
        return PNG_DEC_ERR_FORMAT;

    /* Rendering intent 0..3 */
    if (d[0] > 3u)
        return PNG_DEC_OK; /* ignore invalid */

    st->have_srgb = 1;

    /* sRGB implies gAMA = 45455/100000. */
    st->image_gamma = png_fixed89_from_ratio(45455, 100000);

    return PNG_DEC_OK;
}

int png_handle_pHYs(png_state* st, const png_u8* d, png_u32 length)
{
    png_u32 xppu, yppu;
    png_u8 unit;

    if (!st || !d)
        return PNG_DEC_ERR_FORMAT;

    if (!st->seen_IHDR)
        return PNG_DEC_ERR_FORMAT;

    /* Should be before IDAT (ordering). */
    if (st->seen_IDAT)
        return PNG_DEC_ERR_FORMAT;

    if (st->have_phys)
        return PNG_DEC_ERR_FORMAT;

    if (length != 9u)
        return PNG_DEC_ERR_FORMAT;

    xppu = read_be32(d + 0);
    yppu = read_be32(d + 4);
    unit = d[8];

    if (unit > 1u)
        return PNG_DEC_OK; /* ignore invalid */

    st->have_phys = 1;
    st->phys_ppu_x = xppu;
    st->phys_ppu_y = yppu;
    st->phys_unit = unit;

    return PNG_DEC_OK;
}

int png_handle_bKGD(png_state* st, const png_u8* d, png_u32 length)
{
    png_u32 m;

    if (!st || !d)
        return PNG_DEC_ERR_FORMAT;

    if (!st->seen_IHDR)
        return PNG_DEC_ERR_FORMAT;

    /* Must be before IDAT (ordering). */
    if (st->seen_IDAT)
        return PNG_DEC_ERR_FORMAT;

    if (st->have_bkgd)
        return PNG_DEC_ERR_FORMAT;

    m = sample_mask(st->bit_depth);

    if (st->color_type == PNG_COLOR_INDEXED)
    {
        png_u32 idx;
        png_u32 off;
        png_u8 r, g, b;

        if (length != 1u)
            return PNG_DEC_ERR_FORMAT;

        if (!st->have_palette || st->palette_entries == 0u)
            return PNG_DEC_ERR_FORMAT;

        idx = d[0];
        if (idx >= st->palette_entries)
            return PNG_DEC_OK; /* ignore invalid */

        off = idx * 3u;
        r = st->palette[off + 0u];
        g = st->palette[off + 1u];
        b = st->palette[off + 2u];

        st->bkgd_r = (png_u16)((png_u16)r * 257u);
        st->bkgd_g = (png_u16)((png_u16)g * 257u);
        st->bkgd_b = (png_u16)((png_u16)b * 257u);
    }
    else if (st->color_type == PNG_COLOR_GRAYSCALE || st->color_type == PNG_COLOR_GRAYSCALE_ALPHA)
    {
        png_u32 gray;
        if (length != 2u)
            return PNG_DEC_ERR_FORMAT;

        gray = (png_u32)read_be16(d);
        gray &= m;

        st->bkgd_r = scale_to_16(gray, st->bit_depth);
        st->bkgd_g = st->bkgd_r;
        st->bkgd_b = st->bkgd_r;
    }
    else
    {
        png_u32 r16, g16, b16;

        if (length != 6u)
            return PNG_DEC_ERR_FORMAT;

        r16 = (png_u32)read_be16(d + 0) & m;
        g16 = (png_u32)read_be16(d + 2) & m;
        b16 = (png_u32)read_be16(d + 4) & m;

        st->bkgd_r = scale_to_16(r16, st->bit_depth);
        st->bkgd_g = scale_to_16(g16, st->bit_depth);
        st->bkgd_b = scale_to_16(b16, st->bit_depth);
    }

    st->have_bkgd = 1;
    return PNG_DEC_OK;
}

int png_handle_tIME(png_state* st, const png_u8* d, png_u32 length)
{
    png_u16 year;
    png_u8 month, day, hour, minute, second;

    if (!st || !d)
        return PNG_DEC_ERR_FORMAT;

    if (!st->seen_IHDR)
        return PNG_DEC_ERR_FORMAT;

    if (st->have_time)
        return PNG_DEC_ERR_FORMAT;

    if (length != 7u)
        return PNG_DEC_ERR_FORMAT;

    year   = read_be16(d + 0);
    month  = d[2];
    day    = d[3];
    hour   = d[4];
    minute = d[5];
    second = d[6];

    /* Basic sanity; ignore if invalid */
    if (month < 1u || month > 12u) return PNG_DEC_OK;
    if (day   < 1u || day   > 31u) return PNG_DEC_OK;
    if (hour  > 23u) return PNG_DEC_OK;
    if (minute > 59u) return PNG_DEC_OK;
    if (second > 60u) return PNG_DEC_OK; /* leap second allowed */

    st->have_time = 1;
    st->time_year = year;
    st->time_month = month;
    st->time_day = day;
    st->time_hour = hour;
    st->time_minute = minute;
    st->time_second = second;

    return PNG_DEC_OK;
}

int png_handle_sBIT(png_state* st, const png_u8* d, png_u32 length)
{
    png_u8 bd;

    if (!st || !d)
        return PNG_DEC_ERR_FORMAT;

    if (!st->seen_IHDR)
        return PNG_DEC_ERR_FORMAT;

    /* Must be before PLTE and IDAT */
    if (st->seen_PLTE || st->seen_IDAT)
        return PNG_DEC_ERR_FORMAT;

    if (st->have_sbit)
        return PNG_DEC_ERR_FORMAT;

    bd = st->bit_depth;

    /* Indexed-color sample depth is always 8 bits (palette entries). */
    if (st->color_type == PNG_COLOR_INDEXED)
        bd = 8u;

    /* Parse by color type */
    if (st->color_type == PNG_COLOR_GRAYSCALE)
    {
        if (length != 1u) return PNG_DEC_ERR_FORMAT;
        if (d[0] == 0u || d[0] > bd) return PNG_DEC_OK;
        st->sbit[0] = d[0];
        st->sbit[1] = d[0];
        st->sbit[2] = d[0];
        st->sbit[3] = 0u;
    }
    else if (st->color_type == PNG_COLOR_TRUECOLOR || st->color_type == PNG_COLOR_INDEXED)
    {
        if (length != 3u) return PNG_DEC_ERR_FORMAT;
        if (d[0] == 0u || d[0] > bd) return PNG_DEC_OK;
        if (d[1] == 0u || d[1] > bd) return PNG_DEC_OK;
        if (d[2] == 0u || d[2] > bd) return PNG_DEC_OK;
        st->sbit[0] = d[0];
        st->sbit[1] = d[1];
        st->sbit[2] = d[2];
        st->sbit[3] = 0u;
    }
    else if (st->color_type == PNG_COLOR_GRAYSCALE_ALPHA)
    {
        if (length != 2u) return PNG_DEC_ERR_FORMAT;
        if (d[0] == 0u || d[0] > bd) return PNG_DEC_OK;
        if (d[1] == 0u || d[1] > bd) return PNG_DEC_OK;
        st->sbit[0] = d[0];
        st->sbit[1] = d[0];
        st->sbit[2] = d[0];
        st->sbit[3] = d[1];
    }
    else if (st->color_type == PNG_COLOR_TRUECOLOR_ALPHA)
    {
        if (length != 4u) return PNG_DEC_ERR_FORMAT;
        if (d[0] == 0u || d[0] > bd) return PNG_DEC_OK;
        if (d[1] == 0u || d[1] > bd) return PNG_DEC_OK;
        if (d[2] == 0u || d[2] > bd) return PNG_DEC_OK;
        if (d[3] == 0u || d[3] > bd) return PNG_DEC_OK;
        st->sbit[0] = d[0];
        st->sbit[1] = d[1];
        st->sbit[2] = d[2];
        st->sbit[3] = d[3];
    }
    else
    {
        return PNG_DEC_OK;
    }

    st->have_sbit = 1;
    return PNG_DEC_OK;
}

/* ---------------- Text / profile chunks ---------------- */

static png_u32 png_find_nul_byte(const png_u8* d, png_u32 start, png_u32 length)
{
    png_u32 i;
    for (i = start; i < length; ++i)
    {
        if (d[i] == 0u)
            return i;
    }
    return length;
}

int png_handle_tEXt(png_state* st, const png_u8* d, png_u32 length)
{
    png_u32 key_end;
    char* key;
    char* text;
    int err;

    if (!st || !d)
        return PNG_DEC_ERR_FORMAT;

    key_end = png_find_nul_byte(d, 0u, length);
    if (key_end == 0u || key_end > 79u || key_end >= length)
        return PNG_DEC_ERR_FORMAT;

    key = png_dup_string_n(d, key_end);
    if (!key)
        return PNG_DEC_ERR_OOM;

    text = png_dup_string_n(d + key_end + 1u, length - key_end - 1u);
    if (!text) {
        png_mem89_release(key);
        return PNG_DEC_ERR_OOM;
    }

    err = png_state_add_text_entry(st, key, text, "", "", 0u);
    png_mem89_release(key);
    png_mem89_release(text);
    return err;
}

int png_handle_zTXt(png_state* st, const png_u8* d, png_u32 length)
{
    png_u32 key_end;
    png_u8* text_buf;
    png_u32 text_len;
    char* key;
    char* text;
    int err;

    if (!st || !d)
        return PNG_DEC_ERR_FORMAT;

    key_end = png_find_nul_byte(d, 0u, length);
    if (key_end == 0u || key_end > 79u || key_end + 2u > length)
        return PNG_DEC_ERR_FORMAT;

    if (d[key_end + 1u] != 0u)
        return PNG_DEC_OK;

    key = png_dup_string_n(d, key_end);
    if (!key)
        return PNG_DEC_ERR_OOM;

    text_buf = 0;
    text_len = 0u;
    err = png_try_decompress_unknown_size(st->zfunc,
                                          d + key_end + 2u,
                                          length - key_end - 2u,
                                          &text_buf,
                                          &text_len);
    if (err != PNG_DEC_OK) {
        png_mem89_release(key);
        return err;
    }

    text = png_dup_string_n(text_buf, text_len);
    png_mem89_release(text_buf);
    if (!text) {
        png_mem89_release(key);
        return PNG_DEC_ERR_OOM;
    }

    err = png_state_add_text_entry(st, key, text, "", "", 1u);
    png_mem89_release(key);
    png_mem89_release(text);
    return err;
}

int png_handle_iTXt(png_state* st, const png_u8* d, png_u32 length)
{
    png_u32 key_end;
    png_u32 lang_end;
    png_u32 trans_end;
    png_u8 compression_flag;
    png_u8 compression_method;
    char* key;
    char* lang;
    char* translated;
    char* text;
    png_u8* text_buf;
    png_u32 text_len;
    int err;

    if (!st || !d)
        return PNG_DEC_ERR_FORMAT;

    key_end = png_find_nul_byte(d, 0u, length);
    if (key_end == 0u || key_end > 79u || key_end + 3u > length)
        return PNG_DEC_ERR_FORMAT;

    compression_flag = d[key_end + 1u];
    compression_method = d[key_end + 2u];
    if (!(compression_flag == 0u || compression_flag == 1u))
        return PNG_DEC_OK;
    if (compression_flag != 0u && compression_method != 0u)
        return PNG_DEC_OK;

    lang_end = png_find_nul_byte(d, key_end + 3u, length);
    if (lang_end >= length)
        return PNG_DEC_ERR_FORMAT;

    trans_end = png_find_nul_byte(d, lang_end + 1u, length);
    if (trans_end >= length)
        return PNG_DEC_ERR_FORMAT;

    key = png_dup_string_n(d, key_end);
    if (!key)
        return PNG_DEC_ERR_OOM;

    lang = png_dup_string_n(d + key_end + 3u, lang_end - (key_end + 3u));
    if (!lang) {
        png_mem89_release(key);
        return PNG_DEC_ERR_OOM;
    }

    translated = png_dup_string_n(d + lang_end + 1u, trans_end - (lang_end + 1u));
    if (!translated) {
        png_mem89_release(key);
        png_mem89_release(lang);
        return PNG_DEC_ERR_OOM;
    }

    if (compression_flag == 0u)
    {
        text = png_dup_string_n(d + trans_end + 1u, length - trans_end - 1u);
        if (!text) {
            png_mem89_release(key); png_mem89_release(lang); png_mem89_release(translated);
            return PNG_DEC_ERR_OOM;
        }
    }
    else
    {
        text_buf = 0;
        text_len = 0u;
        err = png_try_decompress_unknown_size(st->zfunc,
                                              d + trans_end + 1u,
                                              length - trans_end - 1u,
                                              &text_buf,
                                              &text_len);
        if (err != PNG_DEC_OK) {
            png_mem89_release(key); png_mem89_release(lang); png_mem89_release(translated);
            return err;
        }
        text = png_dup_string_n(text_buf, text_len);
        png_mem89_release(text_buf);
        if (!text) {
            png_mem89_release(key); png_mem89_release(lang); png_mem89_release(translated);
            return PNG_DEC_ERR_OOM;
        }
    }

    err = png_state_add_text_entry(st, key, text, lang, translated, 2u);
    png_mem89_release(key);
    png_mem89_release(lang);
    png_mem89_release(translated);
    png_mem89_release(text);
    return err;
}

int png_handle_iCCP(png_state* st, const png_u8* d, png_u32 length)
{
    png_u32 key_end;
    png_u8* prof;
    png_u32 prof_len;
    char* name;
    int err;

    if (!st || !d)
        return PNG_DEC_ERR_FORMAT;
    if (!st->seen_IHDR)
        return PNG_DEC_ERR_FORMAT;
    if (st->seen_PLTE || st->seen_IDAT)
        return PNG_DEC_ERR_FORMAT;
    if (st->have_iccp)
        return PNG_DEC_ERR_FORMAT;

    key_end = png_find_nul_byte(d, 0u, length);
    if (key_end == 0u || key_end > 79u || key_end + 2u > length)
        return PNG_DEC_ERR_FORMAT;

    if (d[key_end + 1u] != 0u)
        return PNG_DEC_OK;

    name = png_dup_string_n(d, key_end);
    if (!name)
        return PNG_DEC_ERR_OOM;

    prof = 0;
    prof_len = 0u;
    err = png_try_decompress_unknown_size(st->zfunc,
                                          d + key_end + 2u,
                                          length - key_end - 2u,
                                          &prof,
                                          &prof_len);
    if (err != PNG_DEC_OK) {
        png_mem89_release(name);
        return err;
    }

    st->have_iccp = 1;
    st->iccp_name = name;
    st->iccp_compression_method = 0u;
    st->iccp_profile = prof;
    st->iccp_profile_size = prof_len;
    return PNG_DEC_OK;
}

int png_handle_eXIf(png_state* st, const png_u8* d, png_u32 length)
{
    png_u8* prof;

    if (!st || !d)
        return PNG_DEC_ERR_FORMAT;
    if (!st->seen_IHDR)
        return PNG_DEC_ERR_FORMAT;
    if (st->seen_IDAT)
        return PNG_DEC_ERR_FORMAT;
    if (st->have_exif)
        return PNG_DEC_ERR_FORMAT;
    if (!png_exif_has_valid_tiff_header(d, length))
        return PNG_DEC_ERR_FORMAT;

    prof = png_dup_bytes(d, length);
    if (length != 0u && !prof)
        return PNG_DEC_ERR_OOM;

    st->have_exif = 1;
    st->exif_profile = prof;
    st->exif_profile_size = length;
    return PNG_DEC_OK;
}

/* ---------------- Advanced ancillary / extension chunks ---------------- */

int png_handle_sPLT(png_state* st, const png_u8* d, png_u32 length)
{
    png_u32 key_end;
    png_u8 sample_depth;
    png_u32 entry_size;
    png_u32 remain;
    png_u32 count;
    png_u32 i;
    char* name;
    png_splt_entry* entries;
    png_splt_palette* new_palettes;
    png_u32 newcap;

    if (!st || !d)
        return PNG_DEC_ERR_FORMAT;
    if (!st->seen_IHDR)
        return PNG_DEC_ERR_FORMAT;
    if (st->seen_IDAT)
        return PNG_DEC_ERR_FORMAT;

    key_end = png_find_nul_byte(d, 0u, length);
    if (key_end == 0u || key_end >= length || key_end + 2u > length)
        return PNG_DEC_ERR_FORMAT;
    if (!png_validate_latin1_name(d, key_end))
        return PNG_DEC_ERR_FORMAT;

    sample_depth = d[key_end + 1u];
    if (!(sample_depth == 8u || sample_depth == 16u))
        return PNG_DEC_ERR_FORMAT;

    entry_size = (sample_depth == 8u) ? 6u : 10u;
    remain = length - key_end - 2u;
    if (remain == 0u || (remain % entry_size) != 0u)
        return PNG_DEC_ERR_FORMAT;
    count = remain / entry_size;

    name = png_dup_string_n(d, key_end);
    if (!name)
        return PNG_DEC_ERR_OOM;

    for (i = 0u; i < st->splt_palette_count; ++i)
    {
        if (st->splt_palettes[i].name && strcmp(st->splt_palettes[i].name, name) == 0)
        {
            png_mem89_release(name);
            return PNG_DEC_ERR_FORMAT;
        }
    }

    entries = (png_splt_entry*)png_mem89_alloc(count * sizeof(png_splt_entry));
    if (!entries)
    {
        png_mem89_release(name);
        return PNG_DEC_ERR_OOM;
    }

    for (i = 0u; i < count; ++i)
    {
        png_u32 off = key_end + 2u + i * entry_size;
        if (sample_depth == 8u)
        {
            entries[i].red       = (png_u16)((png_u16)d[off + 0u] * 257u);
            entries[i].green     = (png_u16)((png_u16)d[off + 1u] * 257u);
            entries[i].blue      = (png_u16)((png_u16)d[off + 2u] * 257u);
            entries[i].alpha     = (png_u16)((png_u16)d[off + 3u] * 257u);
            entries[i].frequency = read_be16(d + off + 4u);
        }
        else
        {
            entries[i].red       = read_be16(d + off + 0u);
            entries[i].green     = read_be16(d + off + 2u);
            entries[i].blue      = read_be16(d + off + 4u);
            entries[i].alpha     = read_be16(d + off + 6u);
            entries[i].frequency = read_be16(d + off + 8u);
        }
    }

    if (st->splt_palette_count == st->splt_palette_capacity)
    {
        newcap = (st->splt_palette_capacity == 0u) ? 4u : st->splt_palette_capacity * 2u;
        new_palettes = (png_splt_palette*)png_mem89_resize(st->splt_palettes,
                                                  newcap * sizeof(png_splt_palette));
        if (!new_palettes)
        {
            png_mem89_release(name);
            png_mem89_release(entries);
            return PNG_DEC_ERR_OOM;
        }
        memset(new_palettes + st->splt_palette_capacity, 0,
               (newcap - st->splt_palette_capacity) * sizeof(png_splt_palette));
        st->splt_palettes = new_palettes;
        st->splt_palette_capacity = newcap;
    }

    st->splt_palettes[st->splt_palette_count].name = name;
    st->splt_palettes[st->splt_palette_count].sample_depth = sample_depth;
    st->splt_palettes[st->splt_palette_count].entries = entries;
    st->splt_palettes[st->splt_palette_count].entry_count = count;
    st->splt_palette_count += 1u;
    return PNG_DEC_OK;
}

int png_handle_hIST(png_state* st, const png_u8* d, png_u32 length)
{
    png_u32 i;
    png_u16* hist;

    if (!st || !d)
        return PNG_DEC_ERR_FORMAT;
    if (!st->seen_IHDR || !st->seen_PLTE)
        return PNG_DEC_ERR_FORMAT;
    if (st->seen_IDAT)
        return PNG_DEC_ERR_FORMAT;
    if (st->hist_entries)
        return PNG_DEC_ERR_FORMAT;
    if (!st->palette_entries || length != st->palette_entries * 2u)
        return PNG_DEC_ERR_FORMAT;

    hist = (png_u16*)png_mem89_alloc(st->palette_entries * sizeof(png_u16));
    if (!hist)
        return PNG_DEC_ERR_OOM;

    for (i = 0u; i < st->palette_entries; ++i)
        hist[i] = read_be16(d + i * 2u);

    st->hist_entries = hist;
    st->hist_count = st->palette_entries;
    return PNG_DEC_OK;
}

int png_handle_oFFs(png_state* st, const png_u8* d, png_u32 length)
{
    if (!st || !d)
        return PNG_DEC_ERR_FORMAT;
    if (!st->seen_IHDR)
        return PNG_DEC_ERR_FORMAT;
    if (st->seen_IDAT)
        return PNG_DEC_ERR_FORMAT;
    if (st->have_offs)
        return PNG_DEC_ERR_FORMAT;
    if (length != 9u)
        return PNG_DEC_ERR_FORMAT;
    if (!(d[8] == 0u || d[8] == 1u))
        return PNG_DEC_ERR_FORMAT;

    st->have_offs = 1;
    st->offs_x = read_be32s(d + 0u);
    st->offs_y = read_be32s(d + 4u);
    st->offs_unit = d[8];
    return PNG_DEC_OK;
}

int png_handle_sCAL(png_state* st, const png_u8* d, png_u32 length)
{
    png_u8 unit;
    png_u32 x_end;
    char* sx;
    char* sy;
    png_fixed89 xv;
    png_fixed89 yv;

    if (!st || !d)
        return PNG_DEC_ERR_FORMAT;
    if (!st->seen_IHDR)
        return PNG_DEC_ERR_FORMAT;
    if (st->seen_IDAT)
        return PNG_DEC_ERR_FORMAT;
    if (st->have_scal)
        return PNG_DEC_ERR_FORMAT;
    if (length < 4u)
        return PNG_DEC_ERR_FORMAT;

    unit = d[0];
    if (!(unit == 1u || unit == 2u))
        return PNG_DEC_ERR_FORMAT;

    x_end = png_find_nul_byte(d, 1u, length);
    if (x_end <= 1u || x_end >= length || x_end + 1u >= length)
        return PNG_DEC_ERR_FORMAT;

    if (!png_parse_positive_ascii_fixed(d + 1u, x_end - 1u, &xv))
        return PNG_DEC_ERR_FORMAT;
    if (!png_parse_positive_ascii_fixed(d + x_end + 1u, length - x_end - 1u, &yv))
        return PNG_DEC_ERR_FORMAT;

    sx = png_dup_string_n(d + 1u, x_end - 1u);
    if (!sx)
        return PNG_DEC_ERR_OOM;
    sy = png_dup_string_n(d + x_end + 1u, length - x_end - 1u);
    if (!sy)
    {
        png_mem89_release(sx);
        return PNG_DEC_ERR_OOM;
    }

    st->have_scal = 1;
    st->scal_unit = unit;
    st->scal_pixel_width = sx;
    st->scal_pixel_height = sy;
    st->scal_width = xv;
    st->scal_height = yv;
    return PNG_DEC_OK;
}

int png_handle_pCAL(png_state* st, const png_u8* d, png_u32 length)
{
    png_u32 name_end;
    png_u32 unit_start;
    png_u32 unit_end;
    png_u8 eq_type;
    png_u8 nparams;
    int expected_params;
    png_pcal_info info;
    png_u32 cursor;
    png_u32 i;

    if (!st || !d)
        return PNG_DEC_ERR_FORMAT;
    if (!st->seen_IHDR)
        return PNG_DEC_ERR_FORMAT;
    if (st->seen_IDAT)
        return PNG_DEC_ERR_FORMAT;
    if (st->have_pcal)
        return PNG_DEC_ERR_FORMAT;
    if (length < 12u)
        return PNG_DEC_ERR_FORMAT;

    memset(&info, 0, sizeof(info));

    name_end = png_find_nul_byte(d, 0u, length);
    if (name_end == 0u || name_end >= length)
        return PNG_DEC_ERR_FORMAT;
    if (!png_validate_latin1_name(d, name_end))
        return PNG_DEC_ERR_FORMAT;
    if (name_end + 10u >= length)
        return PNG_DEC_ERR_FORMAT;

    info.name = png_dup_string_n(d, name_end);
    if (!info.name)
        return PNG_DEC_ERR_OOM;

    info.x0 = read_be32s(d + name_end + 1u);
    info.x1 = read_be32s(d + name_end + 5u);
    eq_type = d[name_end + 9u];
    nparams = d[name_end + 10u];
    expected_params = png_pcal_expected_param_count(eq_type);
    if (expected_params < 0 || nparams != (png_u8)expected_params || info.x0 == info.x1)
    {
        png_free_pcal_info(&info);
        return PNG_DEC_ERR_FORMAT;
    }

    unit_start = name_end + 11u;
    unit_end = png_find_nul_byte(d, unit_start, length);
    if (unit_end >= length)
    {
        png_free_pcal_info(&info);
        return PNG_DEC_ERR_FORMAT;
    }
    if (!png_validate_latin1_text(d + unit_start, unit_end - unit_start, 1))
    {
        png_free_pcal_info(&info);
        return PNG_DEC_ERR_FORMAT;
    }

    info.unit_name = png_dup_string_n(d + unit_start, unit_end - unit_start);
    if (!info.unit_name)
    {
        png_free_pcal_info(&info);
        return PNG_DEC_ERR_OOM;
    }

    info.equation_type = eq_type;
    info.param_count = (png_u32)nparams;
    if (info.param_count != 0u)
    {
        info.params = (char**)png_mem89_alloc_zero(info.param_count, sizeof(char*));
        if (!info.params)
        {
            png_free_pcal_info(&info);
            return PNG_DEC_ERR_OOM;
        }
    }

    cursor = unit_end + 1u;
    for (i = 0u; i < info.param_count; ++i)
    {
        png_u32 end;
        png_u32 len;
        if (cursor >= length)
        {
            png_free_pcal_info(&info);
            return PNG_DEC_ERR_FORMAT;
        }
        if (i + 1u == info.param_count)
            end = length;
        else
            end = png_find_nul_byte(d, cursor, length);

        if (end > length || end == cursor)
        {
            png_free_pcal_info(&info);
            return PNG_DEC_ERR_FORMAT;
        }

        len = end - cursor;
        if (!png_parse_ascii_fixed(d + cursor, len, 0))
        {
            png_free_pcal_info(&info);
            return PNG_DEC_ERR_FORMAT;
        }
        info.params[i] = png_dup_string_n(d + cursor, len);
        if (!info.params[i])
        {
            png_free_pcal_info(&info);
            return PNG_DEC_ERR_OOM;
        }
        cursor = end + 1u;
    }

    if (cursor != length + 1u)
    {
        png_free_pcal_info(&info);
        return PNG_DEC_ERR_FORMAT;
    }

    st->have_pcal = 1;
    st->pcal = info;
    return PNG_DEC_OK;
}

int png_handle_sTER(png_state* st, const png_u8* d, png_u32 length)
{
    if (!st || !d)
        return PNG_DEC_ERR_FORMAT;
    if (!st->seen_IHDR)
        return PNG_DEC_ERR_FORMAT;
    if (st->seen_IDAT)
        return PNG_DEC_ERR_FORMAT;
    if (st->have_ster)
        return PNG_DEC_ERR_FORMAT;
    if (length != 1u)
        return PNG_DEC_ERR_FORMAT;
    if (!(d[0] == 0u || d[0] == 1u))
        return PNG_DEC_ERR_FORMAT;

    st->have_ster = 1;
    st->ster_mode = d[0];
    return PNG_DEC_OK;
}

int png_handle_gIFg(png_state* st, const png_u8* d, png_u32 length)
{
    png_u8 location;

    if (!st || !d || length != 4u)
        return PNG_DEC_ERR_FORMAT;
    if (!st->seen_IHDR || st->seen_IEND)
        return PNG_DEC_ERR_FORMAT;
    if (d[1] > 1u || d[0] > 7u)
        return PNG_DEC_ERR_FORMAT;
    if (!png_special_chunk_location(st, &location))
        return PNG_DEC_ERR_FORMAT;
    return png_state_add_gifg_chunk(st, d[0], d[1], read_be16(d + 2u), location);
}

int png_handle_gIFx(png_state* st, const png_u8* d, png_u32 length)
{
    png_u8 location;
    char app_id[8];

    if (!st || !d || length < 11u)
        return PNG_DEC_ERR_FORMAT;
    if (!st->seen_IHDR || st->seen_IEND)
        return PNG_DEC_ERR_FORMAT;
    if (!png_validate_printable_ascii_bytes(d, 8u))
        return PNG_DEC_ERR_FORMAT;
    memcpy(app_id, d, 8u);
    if (!png_special_chunk_location(st, &location))
        return PNG_DEC_ERR_FORMAT;
    return png_state_add_gifx_chunk(st, app_id, d + 8u, d + 11u, length - 11u, location);
}

int png_handle_gIFt(png_state* st, const png_u8* d, png_u32 length)
{
    png_gift_entry entry;
    png_u8 location;

    if (!st || !d || length < 24u)
        return PNG_DEC_ERR_FORMAT;
    if (!st->seen_IHDR || st->seen_IEND)
        return PNG_DEC_ERR_FORMAT;
    memset(&entry, 0, sizeof(entry));
    entry.grid_left = read_be32s(d + 0u);
    entry.grid_top = read_be32s(d + 4u);
    entry.grid_width = read_be32(d + 8u);
    entry.grid_height = read_be32(d + 12u);
    entry.cell_width = d[16];
    entry.cell_height = d[17];
    memcpy(entry.foreground_rgb, d + 18u, 3u);
    memcpy(entry.background_rgb, d + 21u, 3u);
    entry.text_data = (png_u8*)(d + 24u);
    entry.text_data_size = length - 24u;
    if (!png_special_chunk_location(st, &location))
        return PNG_DEC_ERR_FORMAT;
    entry.location = location;
    return png_state_add_gift_chunk(st, &entry);
}

int png_handle_dSIG(png_state* st, const png_u8* d, png_u32 length)
{
    png_u8 location;

    if (!st || !d || length == 0u)
        return PNG_DEC_ERR_FORMAT;
    if (!st->seen_IHDR || st->seen_IEND)
        return PNG_DEC_ERR_FORMAT;

    if (!st->seen_IDAT)
    {
        if (st->post_ihdr_non_dsig_seen)
            return PNG_DEC_ERR_FORMAT;
        location = PNG_CHUNK_POS_AFTER_IHDR;
    }
    else
    {
        location = PNG_CHUNK_POS_BEFORE_IEND;
    }

    return png_state_add_dsig_chunk(st, d, length, location);
}

int png_handle_fRAc(png_state* st, const png_u8* d, png_u32 length)
{
    png_u8 location;

    if (!st || (!d && length != 0u))
        return PNG_DEC_ERR_FORMAT;
    if (!st->seen_IHDR || st->seen_IEND)
        return PNG_DEC_ERR_FORMAT;
    if (!png_special_chunk_location(st, &location))
        return PNG_DEC_ERR_FORMAT;
    return png_state_add_frac_chunk(st, d, length, location);
}
