#include "sfnt_decoder.h"

/* Compile-time width checks. C89 friendly: bad widths trigger division by zero. */
enum {
    sfnt_check_u8_size  = 1 / ((sizeof(sfnt_u8)  == 1u) ? 1 : 0),
    sfnt_check_u16_size = 1 / ((sizeof(sfnt_u16) == 2u) ? 1 : 0),
    sfnt_check_u32_size = 1 / ((sizeof(sfnt_u32) == 4u) ? 1 : 0)
};

#define SFNT_U32_MAX_VALUE ((sfnt_u32)0xffffffffu)
#define SFNT_HEAD_CHECKSUM_ADJUSTMENT_OFFSET 8u
#define SFNT_FONT_MAGIC_CHECKSUM ((sfnt_u32)0xb1b0afbau)
#define SFNT_FIXED_ONE ((sfnt_fixed)65536)

#define SFNT_CMAP_PLATFORM_UNICODE 0u
#define SFNT_CMAP_PLATFORM_WINDOWS 3u

static int sfnt_add_overflow_u32(sfnt_u32 a, sfnt_u32 b, sfnt_u32 *out)
{
    if (out == 0) return SFNT_ERR_NULL;
    if (a > (SFNT_U32_MAX_VALUE - b)) return SFNT_ERR_RANGE;
    *out = a + b;
    return SFNT_OK;
}

static int sfnt_range_ok(sfnt_u32 size, sfnt_u32 off, sfnt_u32 len)
{
    sfnt_u32 end;
    if (sfnt_add_overflow_u32(off, len, &end) != SFNT_OK) return 0;
    if (end > size) return 0;
    return 1;
}

sfnt_u16 sfnt_read_u16(const sfnt_u8 *p)
{
    if (p == 0) return 0u;
    return (sfnt_u16)((((sfnt_u16)p[0]) << 8) | ((sfnt_u16)p[1]));
}

sfnt_i16 sfnt_read_i16(const sfnt_u8 *p)
{
    return (sfnt_i16)sfnt_read_u16(p);
}

sfnt_u32 sfnt_read_u32(const sfnt_u8 *p)
{
    if (p == 0) return 0u;
    return (((sfnt_u32)p[0]) << 24) | (((sfnt_u32)p[1]) << 16) |
           (((sfnt_u32)p[2]) << 8)  |  ((sfnt_u32)p[3]);
}

sfnt_i32 sfnt_read_i32(const sfnt_u8 *p)
{
    return (sfnt_i32)sfnt_read_u32(p);
}

void sfnt_tag_to_chars(sfnt_u32 tag, char out5[5])
{
    if (out5 == 0) return;
    out5[0] = (char)((tag >> 24) & 255u);
    out5[1] = (char)((tag >> 16) & 255u);
    out5[2] = (char)((tag >> 8) & 255u);
    out5[3] = (char)(tag & 255u);
    out5[4] = '\0';
}

static int sfnt_parse_head(sfnt_face *face)
{
    sfnt_table_ref ref;
    const sfnt_u8 *p;

    if (sfnt_find_table(face, SFNT_TAG_HEAD, &ref) != SFNT_OK) return SFNT_ERR_NOT_FOUND;
    if (ref.length < 54u) return SFNT_ERR_BAD_TABLE;
    p = ref.ptr;

    face->units_per_em = sfnt_read_u16(p + 18u);
    face->x_min = sfnt_read_i16(p + 36u);
    face->y_min = sfnt_read_i16(p + 38u);
    face->x_max = sfnt_read_i16(p + 40u);
    face->y_max = sfnt_read_i16(p + 42u);
    face->index_to_loc_format = sfnt_read_i16(p + 50u);
    face->glyph_data_format = sfnt_read_i16(p + 52u);
    face->has_head = 1u;
    return SFNT_OK;
}

static int sfnt_parse_maxp(sfnt_face *face)
{
    sfnt_table_ref ref;

    if (sfnt_find_table(face, SFNT_TAG_MAXP, &ref) != SFNT_OK) return SFNT_ERR_NOT_FOUND;
    if (ref.length < 6u) return SFNT_ERR_BAD_TABLE;
    face->num_glyphs = sfnt_read_u16(ref.ptr + 4u);
    face->has_maxp = 1u;
    return SFNT_OK;
}

static int sfnt_parse_hhea(sfnt_face *face)
{
    sfnt_table_ref ref;

    if (sfnt_find_table(face, SFNT_TAG_HHEA, &ref) != SFNT_OK) return SFNT_ERR_NOT_FOUND;
    if (ref.length < 36u) return SFNT_ERR_BAD_TABLE;
    face->ascender = sfnt_read_i16(ref.ptr + 4u);
    face->descender = sfnt_read_i16(ref.ptr + 6u);
    face->line_gap = sfnt_read_i16(ref.ptr + 8u);
    face->num_h_metrics = sfnt_read_u16(ref.ptr + 34u);
    face->has_hhea = 1u;
    return SFNT_OK;
}

static void sfnt_mark_known_tables(sfnt_face *face)
{
    if (sfnt_table_index(face, SFNT_TAG_HMTX) >= 0) face->has_hmtx = 1u;
    if (sfnt_table_index(face, SFNT_TAG_CMAP) >= 0) face->has_cmap = 1u;
    if (sfnt_table_index(face, SFNT_TAG_LOCA) >= 0) face->has_loca = 1u;
    if (sfnt_table_index(face, SFNT_TAG_GLYF) >= 0) face->has_glyf = 1u;
    if (sfnt_table_index(face, SFNT_TAG_NAME) >= 0) face->has_name = 1u;
    if (sfnt_table_index(face, SFNT_TAG_OS2) >= 0) face->has_os2 = 1u;
    if (sfnt_table_index(face, SFNT_TAG_KERN) >= 0) face->has_kern = 1u;
    if (sfnt_table_index(face, SFNT_TAG_CFF) >= 0) face->has_cff = 1u;
    if (sfnt_table_index(face, SFNT_TAG_CFF2) >= 0) face->has_cff2 = 1u;
}

int sfnt_open(sfnt_face *face, const void *font_bytes, sfnt_u32 font_size)
{
    const sfnt_u8 *p;
    sfnt_u32 tag;

    if (face == 0 || font_bytes == 0) return SFNT_ERR_NULL;
    if (font_size < 4u) return SFNT_ERR_RANGE;
    p = (const sfnt_u8 *)font_bytes;
    tag = sfnt_read_u32(p);
    if (tag == SFNT_TAG_TTCF) return sfnt_open_ttc(face, font_bytes, font_size, 0u);
    return sfnt_open_at(face, font_bytes, font_size, 0u);
}

int sfnt_open_ttc(sfnt_face *face, const void *font_bytes, sfnt_u32 font_size, sfnt_u32 font_index)
{
    const sfnt_u8 *p;
    sfnt_u32 num_fonts;
    sfnt_u32 off;
    sfnt_u32 rec_off;

    if (face == 0 || font_bytes == 0) return SFNT_ERR_NULL;
    p = (const sfnt_u8 *)font_bytes;
    if (font_size < 12u) return SFNT_ERR_RANGE;
    if (sfnt_read_u32(p) != SFNT_TAG_TTCF) return sfnt_open_at(face, font_bytes, font_size, 0u);
    num_fonts = sfnt_read_u32(p + 8u);
    if (font_index >= num_fonts) return SFNT_ERR_RANGE;
    rec_off = 12u + font_index * 4u;
    if (!sfnt_range_ok(font_size, rec_off, 4u)) return SFNT_ERR_RANGE;
    off = sfnt_read_u32(p + rec_off);
    return sfnt_open_at(face, font_bytes, font_size, off);
}

int sfnt_open_at(sfnt_face *face, const void *font_bytes, sfnt_u32 font_size, sfnt_u32 offset)
{
    const sfnt_u8 *data;
    sfnt_u32 dir_off;
    sfnt_u32 i;
    sfnt_u32 tag;
    sfnt_u16 num_tables;
    sfnt_u32 directory_len;
    sfnt_u32 end;
    int r;

    if (face == 0 || font_bytes == 0) return SFNT_ERR_NULL;
    data = (const sfnt_u8 *)font_bytes;
    if (!sfnt_range_ok(font_size, offset, 12u)) return SFNT_ERR_RANGE;

    tag = sfnt_read_u32(data + offset);
    if (!(tag == 0x00010000u || tag == SFNT_TAG('O','T','T','O') ||
          tag == SFNT_TAG('t','r','u','e') || tag == SFNT_TAG('t','y','p','1')))
        return SFNT_ERR_BAD_MAGIC;

    num_tables = sfnt_read_u16(data + offset + 4u);
    if (num_tables > (sfnt_u16)SFNT_MAX_TABLES) return SFNT_ERR_TOO_MANY;

    directory_len = 12u + ((sfnt_u32)num_tables) * 16u;
    if (!sfnt_range_ok(font_size, offset, directory_len)) return SFNT_ERR_RANGE;

    face->data = data;
    face->size = font_size;
    face->base_offset = offset;
    face->sfnt_version = tag;
    face->num_tables = num_tables;
    face->search_range = sfnt_read_u16(data + offset + 6u);
    face->entry_selector = sfnt_read_u16(data + offset + 8u);
    face->range_shift = sfnt_read_u16(data + offset + 10u);

    face->has_head = 0u; face->has_maxp = 0u; face->has_hhea = 0u; face->has_hmtx = 0u;
    face->has_cmap = 0u; face->has_loca = 0u; face->has_glyf = 0u; face->has_name = 0u;
    face->has_os2 = 0u; face->has_kern = 0u; face->has_cff = 0u; face->has_cff2 = 0u;
    face->units_per_em = 0u; face->num_glyphs = 0u; face->index_to_loc_format = 0;
    face->glyph_data_format = 0; face->x_min = 0; face->y_min = 0; face->x_max = 0; face->y_max = 0;
    face->ascender = 0; face->descender = 0; face->line_gap = 0; face->num_h_metrics = 0u;

    dir_off = offset + 12u;
    for (i = 0u; i < (sfnt_u32)num_tables; ++i) {
        const sfnt_u8 *rec = data + dir_off + i * 16u;
        face->tables[i].tag = sfnt_read_u32(rec + 0u);
        face->tables[i].checksum = sfnt_read_u32(rec + 4u);
        face->tables[i].offset = sfnt_read_u32(rec + 8u);
        face->tables[i].length = sfnt_read_u32(rec + 12u);
        if (sfnt_add_overflow_u32(face->tables[i].offset, face->tables[i].length, &end) != SFNT_OK)
            return SFNT_ERR_RANGE;
        if (end > font_size) return SFNT_ERR_RANGE;
    }

    sfnt_mark_known_tables(face);
    r = sfnt_parse_head(face); if (r != SFNT_OK && r != SFNT_ERR_NOT_FOUND) return r;
    r = sfnt_parse_maxp(face); if (r != SFNT_OK && r != SFNT_ERR_NOT_FOUND) return r;
    r = sfnt_parse_hhea(face); if (r != SFNT_OK && r != SFNT_ERR_NOT_FOUND) return r;

    return SFNT_OK;
}

int sfnt_table_index(const sfnt_face *face, sfnt_u32 tag)
{
    sfnt_u32 i;
    if (face == 0) return SFNT_ERR_NULL;
    for (i = 0u; i < (sfnt_u32)face->num_tables; ++i) {
        if (face->tables[i].tag == tag) return (int)i;
    }
    return SFNT_ERR_NOT_FOUND;
}

int sfnt_find_table(const sfnt_face *face, sfnt_u32 tag, sfnt_table_ref *out_ref)
{
    int idx;
    const sfnt_table_record *r;

    if (face == 0 || out_ref == 0) return SFNT_ERR_NULL;
    idx = sfnt_table_index(face, tag);
    if (idx < 0) return idx;
    r = &face->tables[idx];
    if (!sfnt_range_ok(face->size, r->offset, r->length)) return SFNT_ERR_RANGE;
    out_ref->ptr = face->data + r->offset;
    out_ref->offset = r->offset;
    out_ref->length = r->length;
    out_ref->checksum = r->checksum;
    return SFNT_OK;
}

sfnt_u32 sfnt_table_checksum(const sfnt_u8 *bytes, sfnt_u32 length)
{
    sfnt_u32 sum;
    sfnt_u32 i;
    sfnt_u32 nlongs;

    if (bytes == 0) return 0u;
    sum = 0u;
    nlongs = (length + 3u) >> 2;
    for (i = 0u; i < nlongs; ++i) {
        sfnt_u32 base = i << 2;
        sfnt_u32 v = 0u;
        if (base + 0u < length) v |= ((sfnt_u32)bytes[base + 0u]) << 24;
        if (base + 1u < length) v |= ((sfnt_u32)bytes[base + 1u]) << 16;
        if (base + 2u < length) v |= ((sfnt_u32)bytes[base + 2u]) << 8;
        if (base + 3u < length) v |= ((sfnt_u32)bytes[base + 3u]);
        sum += v;
    }
    return sum;
}


static sfnt_u32 sfnt_head_table_checksum_zero_adjustment(const sfnt_u8 *bytes, sfnt_u32 length)
{
    sfnt_u32 sum;
    sfnt_u32 i;
    sfnt_u32 nlongs;

    if (bytes == 0) return 0u;
    sum = 0u;
    nlongs = (length + 3u) >> 2;
    for (i = 0u; i < nlongs; ++i) {
        sfnt_u32 base = i << 2;
        sfnt_u32 v = 0u;
        if (base == SFNT_HEAD_CHECKSUM_ADJUSTMENT_OFFSET) {
            v = 0u;
        } else {
            if (base + 0u < length) v |= ((sfnt_u32)bytes[base + 0u]) << 24;
            if (base + 1u < length) v |= ((sfnt_u32)bytes[base + 1u]) << 16;
            if (base + 2u < length) v |= ((sfnt_u32)bytes[base + 2u]) << 8;
            if (base + 3u < length) v |= ((sfnt_u32)bytes[base + 3u]);
        }
        sum += v;
    }
    return sum;
}

int sfnt_validate_table_checksum(const sfnt_face *face, sfnt_u32 tag)
{
    sfnt_table_ref ref;
    sfnt_u32 sum;
    int r;

    r = sfnt_find_table(face, tag, &ref);
    if (r != SFNT_OK) return r;
    if (tag == SFNT_TAG_HEAD) sum = sfnt_head_table_checksum_zero_adjustment(ref.ptr, ref.length);
    else sum = sfnt_table_checksum(ref.ptr, ref.length);
    if (sum != ref.checksum) return SFNT_ERR_CHECKSUM;
    return SFNT_OK;
}

int sfnt_validate_font_checksum(const sfnt_face *face)
{
    sfnt_u32 sum;
    if (face == 0 || face->data == 0) return SFNT_ERR_NULL;
    sum = sfnt_table_checksum(face->data + face->base_offset, face->size - face->base_offset);
    if (sum != SFNT_FONT_MAGIC_CHECKSUM) return SFNT_ERR_CHECKSUM;
    return SFNT_OK;
}

int sfnt_validate_directory_bounds(const sfnt_face *face)
{
    sfnt_u32 i;
    if (face == 0) return SFNT_ERR_NULL;
    for (i = 0u; i < (sfnt_u32)face->num_tables; ++i) {
        if (!sfnt_range_ok(face->size, face->tables[i].offset, face->tables[i].length))
            return SFNT_ERR_RANGE;
    }
    return SFNT_OK;
}

static int sfnt_cmap_score(sfnt_u16 platform, sfnt_u16 encoding, sfnt_u16 format)
{
    int score;
    score = 0;
    if (format == 12u) score += 600;
    else if (format == 13u) score += 550;
    else if (format == 4u) score += 500;
    else if (format == 6u) score += 220;
    else if (format == 0u) score += 150;
    else return -1;

    if (platform == SFNT_CMAP_PLATFORM_WINDOWS && encoding == 10u) score += 80;
    else if (platform == SFNT_CMAP_PLATFORM_WINDOWS && encoding == 1u) score += 70;
    else if (platform == SFNT_CMAP_PLATFORM_UNICODE) score += 60;
    else if (platform == 1u && encoding == 0u) score += 10;

    return score;
}

int sfnt_select_cmap(const sfnt_face *face, sfnt_cmap *out_cmap)
{
    sfnt_table_ref cmap;
    sfnt_u16 version;
    sfnt_u16 count;
    sfnt_u32 i;
    int best_score;
    int found;
    int r;

    if (face == 0 || out_cmap == 0) return SFNT_ERR_NULL;
    r = sfnt_find_table(face, SFNT_TAG_CMAP, &cmap);
    if (r != SFNT_OK) return r;
    if (cmap.length < 4u) return SFNT_ERR_BAD_TABLE;
    version = sfnt_read_u16(cmap.ptr + 0u);
    count = sfnt_read_u16(cmap.ptr + 2u);
    if (version != 0u) return SFNT_ERR_BAD_TABLE;
    if (cmap.length < 4u + ((sfnt_u32)count) * 8u) return SFNT_ERR_BAD_TABLE;

    found = 0;
    best_score = -1;
    out_cmap->platform_id = 0u;
    out_cmap->encoding_id = 0u;
    out_cmap->subtable_offset = 0u;
    out_cmap->length = 0u;
    out_cmap->format = 0u;

    for (i = 0u; i < (sfnt_u32)count; ++i) {
        const sfnt_u8 *rec = cmap.ptr + 4u + i * 8u;
        sfnt_u16 platform = sfnt_read_u16(rec + 0u);
        sfnt_u16 encoding = sfnt_read_u16(rec + 2u);
        sfnt_u32 suboff = sfnt_read_u32(rec + 4u);
        sfnt_u16 format;
        sfnt_u32 length;
        int score;

        if (!sfnt_range_ok(cmap.length, suboff, 2u)) continue;
        format = sfnt_read_u16(cmap.ptr + suboff);
        score = sfnt_cmap_score(platform, encoding, format);
        if (score < 0) continue;

        length = 0u;
        if (format == 0u) length = 262u;
        else if (format == 4u || format == 6u) {
            if (!sfnt_range_ok(cmap.length, suboff, 4u)) continue;
            length = (sfnt_u32)sfnt_read_u16(cmap.ptr + suboff + 2u);
        } else if (format == 12u || format == 13u) {
            if (!sfnt_range_ok(cmap.length, suboff, 8u)) continue;
            length = sfnt_read_u32(cmap.ptr + suboff + 4u);
        }
        if (length == 0u || !sfnt_range_ok(cmap.length, suboff, length)) continue;

        if (!found || score > best_score) {
            best_score = score;
            found = 1;
            out_cmap->platform_id = platform;
            out_cmap->encoding_id = encoding;
            out_cmap->subtable_offset = cmap.offset + suboff;
            out_cmap->length = length;
            out_cmap->format = format;
        }
    }

    if (!found) return SFNT_ERR_UNSUPPORTED;
    return SFNT_OK;
}

static int sfnt_cmap_lookup_format0(const sfnt_u8 *p, sfnt_u32 len, sfnt_u32 codepoint, sfnt_u16 *out_gid)
{
    if (len < 262u) return SFNT_ERR_BAD_TABLE;
    if (codepoint > 255u) { *out_gid = 0u; return SFNT_OK; }
    *out_gid = (sfnt_u16)p[6u + codepoint];
    return SFNT_OK;
}

static int sfnt_cmap_lookup_format6(const sfnt_u8 *p, sfnt_u32 len, sfnt_u32 codepoint, sfnt_u16 *out_gid)
{
    sfnt_u16 first_code;
    sfnt_u16 entry_count;
    sfnt_u32 idx;
    if (len < 10u) return SFNT_ERR_BAD_TABLE;
    first_code = sfnt_read_u16(p + 6u);
    entry_count = sfnt_read_u16(p + 8u);
    if (len < 10u + ((sfnt_u32)entry_count) * 2u) return SFNT_ERR_BAD_TABLE;
    if (codepoint < (sfnt_u32)first_code) { *out_gid = 0u; return SFNT_OK; }
    idx = codepoint - (sfnt_u32)first_code;
    if (idx >= (sfnt_u32)entry_count) { *out_gid = 0u; return SFNT_OK; }
    *out_gid = sfnt_read_u16(p + 10u + idx * 2u);
    return SFNT_OK;
}

static int sfnt_cmap_lookup_format4(const sfnt_u8 *p, sfnt_u32 len, sfnt_u32 codepoint, sfnt_u16 *out_gid)
{
    sfnt_u16 seg_count_x2;
    sfnt_u16 seg_count;
    sfnt_u32 end_off;
    sfnt_u32 start_off;
    sfnt_u32 delta_off;
    sfnt_u32 range_off;
    sfnt_u32 i;

    if (codepoint > 0xffffu) { *out_gid = 0u; return SFNT_OK; }
    if (len < 16u) return SFNT_ERR_BAD_TABLE;
    seg_count_x2 = sfnt_read_u16(p + 6u);
    if ((seg_count_x2 & 1u) != 0u) return SFNT_ERR_BAD_TABLE;
    seg_count = (sfnt_u16)(seg_count_x2 / 2u);
    end_off = 14u;
    start_off = end_off + ((sfnt_u32)seg_count) * 2u + 2u;
    delta_off = start_off + ((sfnt_u32)seg_count) * 2u;
    range_off = delta_off + ((sfnt_u32)seg_count) * 2u;
    if (len < range_off + ((sfnt_u32)seg_count) * 2u) return SFNT_ERR_BAD_TABLE;

    for (i = 0u; i < (sfnt_u32)seg_count; ++i) {
        sfnt_u16 end_code = sfnt_read_u16(p + end_off + i * 2u);
        sfnt_u16 start_code;
        sfnt_i16 id_delta;
        sfnt_u16 id_range_offset;
        sfnt_u32 ro_addr;
        sfnt_u32 glyph_pos;
        sfnt_u16 gid;

        if (codepoint > (sfnt_u32)end_code) continue;
        start_code = sfnt_read_u16(p + start_off + i * 2u);
        if (codepoint < (sfnt_u32)start_code) { *out_gid = 0u; return SFNT_OK; }

        id_delta = sfnt_read_i16(p + delta_off + i * 2u);
        ro_addr = range_off + i * 2u;
        id_range_offset = sfnt_read_u16(p + ro_addr);
        if (id_range_offset == 0u) {
            gid = (sfnt_u16)((codepoint + (sfnt_i32)id_delta) & 0xffffu);
        } else {
            glyph_pos = ro_addr + (sfnt_u32)id_range_offset + (codepoint - (sfnt_u32)start_code) * 2u;
            if (!sfnt_range_ok(len, glyph_pos, 2u)) return SFNT_ERR_BAD_TABLE;
            gid = sfnt_read_u16(p + glyph_pos);
            if (gid != 0u) gid = (sfnt_u16)((gid + (sfnt_i32)id_delta) & 0xffffu);
        }
        *out_gid = gid;
        return SFNT_OK;
    }

    *out_gid = 0u;
    return SFNT_OK;
}

static int sfnt_cmap_lookup_format12_or_13(const sfnt_u8 *p, sfnt_u32 len, sfnt_u32 codepoint, sfnt_u16 format, sfnt_u16 *out_gid)
{
    sfnt_u32 n_groups;
    sfnt_u32 lo;
    sfnt_u32 hi;

    if (len < 16u) return SFNT_ERR_BAD_TABLE;
    n_groups = sfnt_read_u32(p + 12u);
    if (n_groups > ((len - 16u) / 12u)) return SFNT_ERR_BAD_TABLE;

    lo = 0u;
    hi = n_groups;
    while (lo < hi) {
        sfnt_u32 mid = lo + ((hi - lo) >> 1);
        const sfnt_u8 *g = p + 16u + mid * 12u;
        sfnt_u32 start_char = sfnt_read_u32(g + 0u);
        sfnt_u32 end_char = sfnt_read_u32(g + 4u);
        sfnt_u32 start_gid = sfnt_read_u32(g + 8u);
        sfnt_u32 gid32;

        if (codepoint < start_char) {
            hi = mid;
        } else if (codepoint > end_char) {
            lo = mid + 1u;
        } else {
            if (format == 13u) gid32 = start_gid;
            else gid32 = start_gid + (codepoint - start_char);
            if (gid32 > 0xffffu) return SFNT_ERR_RANGE;
            *out_gid = (sfnt_u16)gid32;
            return SFNT_OK;
        }
    }

    *out_gid = 0u;
    return SFNT_OK;
}

int sfnt_cmap_lookup(const sfnt_face *face, const sfnt_cmap *cmap, sfnt_u32 codepoint, sfnt_u16 *out_gid)
{
    const sfnt_u8 *p;
    if (face == 0 || cmap == 0 || out_gid == 0) return SFNT_ERR_NULL;
    if (!sfnt_range_ok(face->size, cmap->subtable_offset, cmap->length)) return SFNT_ERR_RANGE;
    p = face->data + cmap->subtable_offset;
    if (cmap->format == 0u) return sfnt_cmap_lookup_format0(p, cmap->length, codepoint, out_gid);
    if (cmap->format == 4u) return sfnt_cmap_lookup_format4(p, cmap->length, codepoint, out_gid);
    if (cmap->format == 6u) return sfnt_cmap_lookup_format6(p, cmap->length, codepoint, out_gid);
    if (cmap->format == 12u || cmap->format == 13u)
        return sfnt_cmap_lookup_format12_or_13(p, cmap->length, codepoint, cmap->format, out_gid);
    return SFNT_ERR_UNSUPPORTED;
}

int sfnt_lookup_glyph(const sfnt_face *face, sfnt_u32 codepoint, sfnt_u16 *out_gid)
{
    sfnt_cmap cmap;
    int r;
    r = sfnt_select_cmap(face, &cmap);
    if (r != SFNT_OK) return r;
    return sfnt_cmap_lookup(face, &cmap, codepoint, out_gid);
}

int sfnt_get_hmetric(const sfnt_face *face, sfnt_u16 glyph_id, sfnt_hmetric *out_metric)
{
    sfnt_table_ref hmtx;
    sfnt_u32 long_count;
    sfnt_u32 off;
    sfnt_u16 advance;
    int r;

    if (face == 0 || out_metric == 0) return SFNT_ERR_NULL;
    if (!face->has_hhea || !face->has_hmtx) return SFNT_ERR_NOT_FOUND;
    if (face->num_h_metrics == 0u) return SFNT_ERR_BAD_TABLE;
    r = sfnt_find_table(face, SFNT_TAG_HMTX, &hmtx);
    if (r != SFNT_OK) return r;

    long_count = (sfnt_u32)face->num_h_metrics;
    if ((sfnt_u32)glyph_id < long_count) {
        off = ((sfnt_u32)glyph_id) * 4u;
        if (!sfnt_range_ok(hmtx.length, off, 4u)) return SFNT_ERR_BAD_TABLE;
        out_metric->advance_width = sfnt_read_u16(hmtx.ptr + off);
        out_metric->left_side_bearing = sfnt_read_i16(hmtx.ptr + off + 2u);
        return SFNT_OK;
    }

    off = (long_count - 1u) * 4u;
    if (!sfnt_range_ok(hmtx.length, off, 2u)) return SFNT_ERR_BAD_TABLE;
    advance = sfnt_read_u16(hmtx.ptr + off);

    off = long_count * 4u + (((sfnt_u32)glyph_id) - long_count) * 2u;
    if (!sfnt_range_ok(hmtx.length, off, 2u)) return SFNT_ERR_BAD_TABLE;
    out_metric->advance_width = advance;
    out_metric->left_side_bearing = sfnt_read_i16(hmtx.ptr + off);
    return SFNT_OK;
}

int sfnt_get_glyph_data(const sfnt_face *face, sfnt_u16 glyph_id, sfnt_table_ref *out_ref)
{
    sfnt_table_ref loca;
    sfnt_table_ref glyf;
    sfnt_u32 start;
    sfnt_u32 end;
    sfnt_u32 count;
    int r;

    if (face == 0 || out_ref == 0) return SFNT_ERR_NULL;
    if (!face->has_loca || !face->has_glyf || !face->has_maxp || !face->has_head) return SFNT_ERR_NOT_FOUND;
    if ((sfnt_u32)glyph_id >= (sfnt_u32)face->num_glyphs) return SFNT_ERR_RANGE;
    r = sfnt_find_table(face, SFNT_TAG_LOCA, &loca); if (r != SFNT_OK) return r;
    r = sfnt_find_table(face, SFNT_TAG_GLYF, &glyf); if (r != SFNT_OK) return r;

    count = ((sfnt_u32)face->num_glyphs) + 1u;
    if (face->index_to_loc_format == 0) {
        sfnt_u32 pos = ((sfnt_u32)glyph_id) * 2u;
        if (loca.length < count * 2u) return SFNT_ERR_BAD_TABLE;
        start = ((sfnt_u32)sfnt_read_u16(loca.ptr + pos)) * 2u;
        end = ((sfnt_u32)sfnt_read_u16(loca.ptr + pos + 2u)) * 2u;
    } else if (face->index_to_loc_format == 1) {
        sfnt_u32 pos2 = ((sfnt_u32)glyph_id) * 4u;
        if (loca.length < count * 4u) return SFNT_ERR_BAD_TABLE;
        start = sfnt_read_u32(loca.ptr + pos2);
        end = sfnt_read_u32(loca.ptr + pos2 + 4u);
    } else return SFNT_ERR_BAD_TABLE;

    if (end < start) return SFNT_ERR_BAD_TABLE;
    if (end > glyf.length) return SFNT_ERR_BAD_TABLE;
    out_ref->ptr = glyf.ptr + start;
    out_ref->offset = glyf.offset + start;
    out_ref->length = end - start;
    out_ref->checksum = 0u;
    return SFNT_OK;
}

int sfnt_get_glyph_header(const sfnt_face *face, sfnt_u16 glyph_id, sfnt_glyph_header *out_header)
{
    sfnt_table_ref g;
    int r;

    if (out_header == 0) return SFNT_ERR_NULL;
    r = sfnt_get_glyph_data(face, glyph_id, &g);
    if (r != SFNT_OK) return r;
    if (g.length == 0u) {
        out_header->number_of_contours = 0;
        out_header->x_min = 0; out_header->y_min = 0; out_header->x_max = 0; out_header->y_max = 0;
        return SFNT_OK;
    }
    if (g.length < 10u) return SFNT_ERR_BAD_TABLE;
    out_header->number_of_contours = sfnt_read_i16(g.ptr + 0u);
    out_header->x_min = sfnt_read_i16(g.ptr + 2u);
    out_header->y_min = sfnt_read_i16(g.ptr + 4u);
    out_header->x_max = sfnt_read_i16(g.ptr + 6u);
    out_header->y_max = sfnt_read_i16(g.ptr + 8u);
    return SFNT_OK;
}

int sfnt_decode_simple_glyph(const sfnt_face *face, sfnt_u16 glyph_id, sfnt_point *points, sfnt_u16 max_points, sfnt_outline *out_outline)
{
    sfnt_table_ref g;
    sfnt_i16 contour_count_i;
    sfnt_u16 contour_count;
    sfnt_u16 point_count;
    sfnt_u32 end_pts_off;
    sfnt_u32 instr_len_off;
    sfnt_u16 instr_len;
    sfnt_u32 flag_off;
    sfnt_u32 pos;
    sfnt_u32 i;
    sfnt_u32 contour_i;
    sfnt_u16 next_end;
    sfnt_i32 x;
    sfnt_i32 y;
    int r;

    if (face == 0 || points == 0 || out_outline == 0) return SFNT_ERR_NULL;
    r = sfnt_get_glyph_data(face, glyph_id, &g);
    if (r != SFNT_OK) return r;
    out_outline->contour_count = 0u;
    out_outline->point_count = 0u;
    out_outline->instruction_length = 0u;
    out_outline->x_min = 0; out_outline->y_min = 0; out_outline->x_max = 0; out_outline->y_max = 0;

    if (g.length == 0u) return SFNT_OK;
    if (g.length < 10u) return SFNT_ERR_BAD_TABLE;
    contour_count_i = sfnt_read_i16(g.ptr + 0u);
    if (contour_count_i < 0) return SFNT_ERR_COMPOUND;
    if (contour_count_i == 0) return SFNT_OK;
    contour_count = (sfnt_u16)contour_count_i;
    end_pts_off = 10u;
    if (!sfnt_range_ok(g.length, end_pts_off, ((sfnt_u32)contour_count) * 2u)) return SFNT_ERR_BAD_TABLE;
    point_count = (sfnt_u16)(sfnt_read_u16(g.ptr + end_pts_off + (((sfnt_u32)contour_count) - 1u) * 2u) + 1u);
    if (point_count > max_points) {
        out_outline->point_count = point_count;
        out_outline->contour_count = contour_count;
        return SFNT_ERR_TOO_SMALL;
    }

    instr_len_off = end_pts_off + ((sfnt_u32)contour_count) * 2u;
    if (!sfnt_range_ok(g.length, instr_len_off, 2u)) return SFNT_ERR_BAD_TABLE;
    instr_len = sfnt_read_u16(g.ptr + instr_len_off);
    flag_off = instr_len_off + 2u + (sfnt_u32)instr_len;
    if (!sfnt_range_ok(g.length, flag_off, 0u)) return SFNT_ERR_BAD_TABLE;

    pos = flag_off;
    i = 0u;
    while (i < (sfnt_u32)point_count) {
        sfnt_u8 fl;
        sfnt_u8 repeat;
        sfnt_u32 j;
        if (!sfnt_range_ok(g.length, pos, 1u)) return SFNT_ERR_BAD_TABLE;
        fl = g.ptr[pos++];
        repeat = 0u;
        if ((fl & 8u) != 0u) {
            if (!sfnt_range_ok(g.length, pos, 1u)) return SFNT_ERR_BAD_TABLE;
            repeat = g.ptr[pos++];
        }
        for (j = 0u; j <= (sfnt_u32)repeat; ++j) {
            if (i >= (sfnt_u32)point_count) return SFNT_ERR_BAD_TABLE;
            points[i].raw_flags = fl;
            points[i].on_curve = (sfnt_u8)((fl & 1u) ? 1u : 0u);
            points[i].end_contour = 0u;
            points[i].x = 0;
            points[i].y = 0;
            ++i;
        }
    }

    x = 0;
    for (i = 0u; i < (sfnt_u32)point_count; ++i) {
        sfnt_u8 fl = points[i].raw_flags;
        sfnt_i32 dx;
        if ((fl & 2u) != 0u) {
            if (!sfnt_range_ok(g.length, pos, 1u)) return SFNT_ERR_BAD_TABLE;
            dx = (sfnt_i32)g.ptr[pos++];
            if ((fl & 16u) == 0u) dx = -dx;
        } else {
            if ((fl & 16u) != 0u) dx = 0;
            else {
                if (!sfnt_range_ok(g.length, pos, 2u)) return SFNT_ERR_BAD_TABLE;
                dx = (sfnt_i32)sfnt_read_i16(g.ptr + pos);
                pos += 2u;
            }
        }
        x += dx;
        if (x < -32768 || x > 32767) return SFNT_ERR_RANGE;
        points[i].x = (sfnt_i16)x;
    }

    y = 0;
    for (i = 0u; i < (sfnt_u32)point_count; ++i) {
        sfnt_u8 fl2 = points[i].raw_flags;
        sfnt_i32 dy;
        if ((fl2 & 4u) != 0u) {
            if (!sfnt_range_ok(g.length, pos, 1u)) return SFNT_ERR_BAD_TABLE;
            dy = (sfnt_i32)g.ptr[pos++];
            if ((fl2 & 32u) == 0u) dy = -dy;
        } else {
            if ((fl2 & 32u) != 0u) dy = 0;
            else {
                if (!sfnt_range_ok(g.length, pos, 2u)) return SFNT_ERR_BAD_TABLE;
                dy = (sfnt_i32)sfnt_read_i16(g.ptr + pos);
                pos += 2u;
            }
        }
        y += dy;
        if (y < -32768 || y > 32767) return SFNT_ERR_RANGE;
        points[i].y = (sfnt_i16)y;
    }

    contour_i = 0u;
    next_end = sfnt_read_u16(g.ptr + end_pts_off);
    for (i = 0u; i < (sfnt_u32)point_count; ++i) {
        if (i == (sfnt_u32)next_end) {
            points[i].end_contour = 1u;
            ++contour_i;
            if (contour_i < (sfnt_u32)contour_count)
                next_end = sfnt_read_u16(g.ptr + end_pts_off + contour_i * 2u);
        }
    }

    out_outline->contour_count = contour_count;
    out_outline->point_count = point_count;
    out_outline->instruction_length = instr_len;
    out_outline->x_min = sfnt_read_i16(g.ptr + 2u);
    out_outline->y_min = sfnt_read_i16(g.ptr + 4u);
    out_outline->x_max = sfnt_read_i16(g.ptr + 6u);
    out_outline->y_max = sfnt_read_i16(g.ptr + 8u);
    return SFNT_OK;
}

static sfnt_fixed sfnt_f2dot14_to_fixed(sfnt_i16 v)
{
    return ((sfnt_fixed)v) << 2;
}

int sfnt_visit_compound_glyph(const sfnt_face *face, sfnt_u16 glyph_id, sfnt_component_visitor visitor, void *user)
{
    sfnt_table_ref g;
    sfnt_i16 contours;
    sfnt_u32 pos;
    sfnt_u16 flags;
    int r;

    if (face == 0 || visitor == 0) return SFNT_ERR_NULL;
    r = sfnt_get_glyph_data(face, glyph_id, &g);
    if (r != SFNT_OK) return r;
    if (g.length < 10u) return SFNT_ERR_BAD_TABLE;
    contours = sfnt_read_i16(g.ptr + 0u);
    if (contours >= 0) return SFNT_ERR_SIMPLE;

    pos = 10u;
    do {
        sfnt_component c;
        if (!sfnt_range_ok(g.length, pos, 4u)) return SFNT_ERR_BAD_TABLE;
        flags = sfnt_read_u16(g.ptr + pos); pos += 2u;
        c.flags = flags;
        c.glyph_index = sfnt_read_u16(g.ptr + pos); pos += 2u;
        c.arg1 = 0;
        c.arg2 = 0;
        c.a = SFNT_FIXED_ONE; c.b = 0; c.c = 0; c.d = SFNT_FIXED_ONE;

        if ((flags & 1u) != 0u) {
            if (!sfnt_range_ok(g.length, pos, 4u)) return SFNT_ERR_BAD_TABLE;
            c.arg1 = sfnt_read_i16(g.ptr + pos); pos += 2u;
            c.arg2 = sfnt_read_i16(g.ptr + pos); pos += 2u;
        } else {
            if (!sfnt_range_ok(g.length, pos, 2u)) return SFNT_ERR_BAD_TABLE;
            if ((flags & 2u) != 0u) {
                c.arg1 = (sfnt_i16)((sfnt_i8)g.ptr[pos]);
                c.arg2 = (sfnt_i16)((sfnt_i8)g.ptr[pos + 1u]);
            } else {
                c.arg1 = (sfnt_i16)g.ptr[pos];
                c.arg2 = (sfnt_i16)g.ptr[pos + 1u];
            }
            pos += 2u;
        }

        if ((flags & 8u) != 0u) {
            sfnt_i16 scale;
            if (!sfnt_range_ok(g.length, pos, 2u)) return SFNT_ERR_BAD_TABLE;
            scale = sfnt_read_i16(g.ptr + pos); pos += 2u;
            c.a = sfnt_f2dot14_to_fixed(scale);
            c.d = sfnt_f2dot14_to_fixed(scale);
        } else if ((flags & 64u) != 0u) {
            sfnt_i16 sx;
            sfnt_i16 sy;
            if (!sfnt_range_ok(g.length, pos, 4u)) return SFNT_ERR_BAD_TABLE;
            sx = sfnt_read_i16(g.ptr + pos); pos += 2u;
            sy = sfnt_read_i16(g.ptr + pos); pos += 2u;
            c.a = sfnt_f2dot14_to_fixed(sx);
            c.d = sfnt_f2dot14_to_fixed(sy);
        } else if ((flags & 128u) != 0u) {
            sfnt_i16 a;
            sfnt_i16 b;
            sfnt_i16 cc;
            sfnt_i16 d;
            if (!sfnt_range_ok(g.length, pos, 8u)) return SFNT_ERR_BAD_TABLE;
            a = sfnt_read_i16(g.ptr + pos); pos += 2u;
            b = sfnt_read_i16(g.ptr + pos); pos += 2u;
            cc = sfnt_read_i16(g.ptr + pos); pos += 2u;
            d = sfnt_read_i16(g.ptr + pos); pos += 2u;
            c.a = sfnt_f2dot14_to_fixed(a);
            c.b = sfnt_f2dot14_to_fixed(b);
            c.c = sfnt_f2dot14_to_fixed(cc);
            c.d = sfnt_f2dot14_to_fixed(d);
        }

        r = visitor(user, &c);
        if (r != SFNT_OK) return r;
    } while ((flags & 32u) != 0u);

    if ((flags & 256u) != 0u) {
        sfnt_u16 instr_len;
        if (!sfnt_range_ok(g.length, pos, 2u)) return SFNT_ERR_BAD_TABLE;
        instr_len = sfnt_read_u16(g.ptr + pos); pos += 2u;
        if (!sfnt_range_ok(g.length, pos, (sfnt_u32)instr_len)) return SFNT_ERR_BAD_TABLE;
    }

    return SFNT_OK;
}

sfnt_u16 sfnt_name_count(const sfnt_face *face)
{
    sfnt_table_ref name;
    if (face == 0) return 0u;
    if (sfnt_find_table(face, SFNT_TAG_NAME, &name) != SFNT_OK) return 0u;
    if (name.length < 6u) return 0u;
    return sfnt_read_u16(name.ptr + 2u);
}

int sfnt_get_name_record(const sfnt_face *face, sfnt_u16 index, sfnt_name_record *out_record)
{
    sfnt_table_ref name;
    sfnt_u16 count;
    sfnt_u16 storage_offset;
    sfnt_u32 rec_off;
    sfnt_u16 length;
    sfnt_u16 offset;
    sfnt_u32 string_pos;
    int r;

    if (face == 0 || out_record == 0) return SFNT_ERR_NULL;
    r = sfnt_find_table(face, SFNT_TAG_NAME, &name);
    if (r != SFNT_OK) return r;
    if (name.length < 6u) return SFNT_ERR_BAD_TABLE;
    count = sfnt_read_u16(name.ptr + 2u);
    storage_offset = sfnt_read_u16(name.ptr + 4u);
    if (index >= count) return SFNT_ERR_RANGE;
    rec_off = 6u + ((sfnt_u32)index) * 12u;
    if (!sfnt_range_ok(name.length, rec_off, 12u)) return SFNT_ERR_BAD_TABLE;

    out_record->platform_id = sfnt_read_u16(name.ptr + rec_off + 0u);
    out_record->encoding_id = sfnt_read_u16(name.ptr + rec_off + 2u);
    out_record->language_id = sfnt_read_u16(name.ptr + rec_off + 4u);
    out_record->name_id = sfnt_read_u16(name.ptr + rec_off + 6u);
    length = sfnt_read_u16(name.ptr + rec_off + 8u);
    offset = sfnt_read_u16(name.ptr + rec_off + 10u);
    string_pos = (sfnt_u32)storage_offset + (sfnt_u32)offset;
    if (!sfnt_range_ok(name.length, string_pos, (sfnt_u32)length)) return SFNT_ERR_BAD_TABLE;
    out_record->bytes = name.ptr + string_pos;
    out_record->length = length;
    return SFNT_OK;
}

int sfnt_get_os2_metrics(const sfnt_face *face, sfnt_os2_metrics *out_metrics)
{
    sfnt_table_ref os2;
    int r;

    if (face == 0 || out_metrics == 0) return SFNT_ERR_NULL;
    r = sfnt_find_table(face, SFNT_TAG_OS2, &os2);
    if (r != SFNT_OK) return r;
    if (os2.length < 78u) return SFNT_ERR_BAD_TABLE;
    out_metrics->version = sfnt_read_u16(os2.ptr + 0u);
    out_metrics->x_avg_char_width = sfnt_read_i16(os2.ptr + 2u);
    out_metrics->us_weight_class = sfnt_read_u16(os2.ptr + 4u);
    out_metrics->us_width_class = sfnt_read_u16(os2.ptr + 6u);
    out_metrics->fs_type = sfnt_read_u16(os2.ptr + 8u);
    out_metrics->s_typo_ascender = sfnt_read_i16(os2.ptr + 68u);
    out_metrics->s_typo_descender = sfnt_read_i16(os2.ptr + 70u);
    out_metrics->s_typo_line_gap = sfnt_read_i16(os2.ptr + 72u);
    out_metrics->us_win_ascent = sfnt_read_u16(os2.ptr + 74u);
    out_metrics->us_win_descent = sfnt_read_u16(os2.ptr + 76u);
    out_metrics->sx_height = 0;
    out_metrics->s_cap_height = 0;
    out_metrics->us_default_char = 0u;
    out_metrics->us_break_char = 0u;
    out_metrics->us_max_context = 0u;

    if (out_metrics->version >= 2u && os2.length >= 96u) {
        out_metrics->sx_height = sfnt_read_i16(os2.ptr + 86u);
        out_metrics->s_cap_height = sfnt_read_i16(os2.ptr + 88u);
        out_metrics->us_default_char = sfnt_read_u16(os2.ptr + 90u);
        out_metrics->us_break_char = sfnt_read_u16(os2.ptr + 92u);
        out_metrics->us_max_context = sfnt_read_u16(os2.ptr + 94u);
    }
    return SFNT_OK;
}
