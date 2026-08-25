#include "mfont_internal.h"

#define MFT_TEXT_LINE_MAX 1024

static int mft_is_comment_or_empty(const char *s)
{
    if (s == 0) {
        return 1;
    }
    while (*s != 0 && isspace((unsigned char)*s)) {
        ++s;
    }
    return (*s == 0 || *s == ';') ? 1 : 0;
}

static void mft_font_text_reset(mft_font_text *font)
{
    mft_memzero(font, (mft_u32)sizeof(*font));
    font->type = MFT_FONT_FIXED;
    font->colors = 256;
}

static int mft_parse_pair(const char *value, mft_s32 *a, mft_s32 *b)
{
    char tmp[128];
    char *comma;
    char *left;
    char *right;
    if (value == 0 || strlen(value) >= sizeof(tmp)) {
        return 0;
    }
    strcpy(tmp, value);
    comma = strchr(tmp, ',');
    if (comma == 0) {
        return 0;
    }
    *comma = 0;
    left = mft_trim(tmp);
    right = mft_trim(comma + 1);
    return mft_parse_int(left, a) && mft_parse_int(right, b);
}

static mft_status mft_parse_def_line(mft_font_text *out, char *trimmed, int *saw_def)
{
    char *eq;
    char *key;
    char *value;

    eq = strchr(trimmed, '=');
    if (eq == 0) {
        return MFT_ERR_TEXT;
    }
    *eq = 0;
    key = mft_trim(trimmed);
    value = mft_trim(eq + 1);
    *saw_def = 1;

    if (mft_ascii_ieq(key, "type")) {
        if (mft_ascii_ieq(value, "fixed")) {
            out->type = MFT_FONT_FIXED;
        } else if (mft_ascii_ieq(value, "variable")) {
            out->type = MFT_FONT_VARIABLE;
        } else {
            return MFT_ERR_TEXT;
        }
    } else if (mft_ascii_ieq(key, "offset")) {
        if (!mft_parse_pair(value, &out->offset_x, &out->offset_y)) {
            return MFT_ERR_TEXT;
        }
    } else if (mft_ascii_ieq(key, "size")) {
        if (!mft_parse_pair(value, &out->size_w, &out->size_h)) {
            return MFT_ERR_TEXT;
        }
    } else if (mft_ascii_ieq(key, "spacing")) {
        if (!mft_parse_pair(value, &out->spacing_x, &out->spacing_y)) {
            return MFT_ERR_TEXT;
        }
    } else if (mft_ascii_ieq(key, "colors")) {
        if (!mft_parse_int(value, &out->colors)) {
            return MFT_ERR_TEXT;
        }
    } else if (mft_ascii_ieq(key, "sprites")) {
        strncpy(out->sprites, value, sizeof(out->sprites) - 1U);
        out->sprites[sizeof(out->sprites) - 1U] = 0;
    }
    return MFT_OK;
}

static mft_status mft_parse_map_line(mft_font_text *out,
                                          char *trimmed,
                                          int *saw_map,
                                          mft_s32 *fixed_offset)
{
    char token0[64];
    char token1[64];
    char token2[64];
    int count;
    mft_u8 code;
    mft_s32 offset;
    mft_s32 width;

    token0[0] = 0;
    token1[0] = 0;
    token2[0] = 0;
    count = sscanf(trimmed, "%63s %63s %63s", token0, token1, token2);
    if (count < 1) {
        return MFT_ERR_TEXT;
    }
    if (!mft_parse_char_token(token0, &code)) {
        return MFT_ERR_TEXT;
    }

    *saw_map = 1;
    out->glyphs[code].code = code;
    if (!out->glyphs[code].present) {
        out->glyph_count++;
    }
    out->glyphs[code].present = 1;

    if (out->type == MFT_FONT_VARIABLE) {
        if (count >= 3) {
            if (!mft_parse_int(token1, &offset) || !mft_parse_int(token2, &width)) {
                return MFT_ERR_TEXT;
            }
        } else if (count == 2) {
            if (!mft_parse_pair(token1, &offset, &width)) {
                return MFT_ERR_TEXT;
            }
        } else {
            return MFT_ERR_TEXT;
        }
        out->glyphs[code].offset = offset;
        out->glyphs[code].width = width;
    } else {
        /* FNT v1 fixed fonts store glyphs left-to-right in the same
           order as the [Map] entries.  The map line normally contains
           only the character token, so the offset must advance by the
           fixed cell width for every entry.  Assigning zero here makes
           every character sample the first cell (usually '!'). */
        if (fixed_offset == 0 || out->size_w <= 0) {
            return MFT_ERR_TEXT;
        }
        out->glyphs[code].offset = *fixed_offset;
        out->glyphs[code].width = out->size_w;
        *fixed_offset += out->size_w;
    }
    return MFT_OK;
}

mft_status mft_font_text_parse(mft_font_text *out,
                               const char *text,
                               mft_u32 text_size)
{
    mft_u32 pos;
    char line_buf[MFT_TEXT_LINE_MAX];
    char section[32];
    int saw_def;
    int saw_map;
    mft_s32 fixed_offset;

    if (out == 0 || text == 0) {
        return MFT_ERR_ARGS;
    }
    if (text_size == 0 || text_size >= MFT_CFG_MAX_TEXT) {
        return MFT_ERR_CAPACITY;
    }

    mft_font_text_reset(out);
    section[0] = 0;
    saw_def = 0;
    saw_map = 0;
    fixed_offset = 0;
    pos = 0UL;

    while (pos < text_size) {
        mft_u32 start;
        mft_u32 end;
        mft_u32 len;
        char *trimmed;
        char *comment;
        mft_status st;

        start = pos;
        while (pos < text_size && text[pos] != '\n') {
            ++pos;
        }
        end = pos;
        if (pos < text_size && text[pos] == '\n') {
            ++pos;
        }
        if (end > start && text[end - 1UL] == '\r') {
            --end;
        }
        len = end - start;
        if (len >= (mft_u32)sizeof(line_buf)) {
            return MFT_ERR_TEXT;
        }
        memcpy(line_buf, text + start, (size_t)len);
        line_buf[len] = 0;

        comment = strchr(line_buf, ';');
        if (comment != 0) {
            *comment = 0;
        }
        trimmed = mft_trim(line_buf);
        if (mft_is_comment_or_empty(trimmed)) {
            continue;
        }

        if (trimmed[0] == '[') {
            char *end_bracket;
            end_bracket = strchr(trimmed, ']');
            if (end_bracket == 0) {
                return MFT_ERR_TEXT;
            }
            *end_bracket = 0;
            strncpy(section, mft_trim(trimmed + 1), sizeof(section) - 1U);
            section[sizeof(section) - 1U] = 0;
            continue;
        }

        if (mft_ascii_ieq(section, "Def")) {
            st = mft_parse_def_line(out, trimmed, &saw_def);
            if (st != MFT_OK) {
                return st;
            }
        } else if (mft_ascii_ieq(section, "Map")) {
            st = mft_parse_map_line(out, trimmed, &saw_map,
                                    &fixed_offset);
            if (st != MFT_OK) {
                return st;
            }
        }
    }

    if (!saw_def || !saw_map) {
        return MFT_ERR_TEXT;
    }
    return MFT_OK;
}

mft_status mft_font_text_write(char *dst,
                               mft_u32 *dst_size,
                               const mft_font_text *font)
{
    mft_u32 pos;
    int i;
    int need;
    char line[512];

    if (dst_size == 0 || font == 0) {
        return MFT_ERR_ARGS;
    }
    pos = 0;
#define MFT_APPEND_FMT0(fmt) \
    do { \
        if (dst == 0) { \
            need = sprintf(line, fmt); \
        } else { \
            if (pos >= *dst_size) { \
                return MFT_ERR_CAPACITY; \
            } \
            need = sprintf(dst + pos, fmt); \
        } \
        pos += (mft_u32)need; \
    } while (0)
#define MFT_APPEND_FMT1(fmt, a) \
    do { \
        if (dst == 0) { \
            need = sprintf(line, fmt, a); \
        } else { \
            if (pos >= *dst_size) { \
                return MFT_ERR_CAPACITY; \
            } \
            need = sprintf(dst + pos, fmt, a); \
        } \
        pos += (mft_u32)need; \
    } while (0)
#define MFT_APPEND_FMT2(fmt, a, b) \
    do { \
        if (dst == 0) { \
            need = sprintf(line, fmt, a, b); \
        } else { \
            if (pos >= *dst_size) { \
                return MFT_ERR_CAPACITY; \
            } \
            need = sprintf(dst + pos, fmt, a, b); \
        } \
        pos += (mft_u32)need; \
    } while (0)

    MFT_APPEND_FMT0("[Def]\n");
    MFT_APPEND_FMT1("Type = %s\n", (font->type == MFT_FONT_VARIABLE) ? "Variable" : "Fixed");
    MFT_APPEND_FMT2("Offset = %ld,%ld\n", (long)font->offset_x, (long)font->offset_y);
    MFT_APPEND_FMT2("Size = %ld,%ld\n", (long)font->size_w, (long)font->size_h);
    MFT_APPEND_FMT2("Spacing = %ld,%ld\n", (long)font->spacing_x, (long)font->spacing_y);
    MFT_APPEND_FMT1("Colors = %ld\n", (long)font->colors);
    MFT_APPEND_FMT1("Sprites = %s\n\n", font->sprites);
    MFT_APPEND_FMT0("[Map]\n");
    for (i = 0; i < 256; ++i) {
        if (font->glyphs[i].present) {
            if (font->type == MFT_FONT_VARIABLE) {
                MFT_APPEND_FMT2("0x%02X %ld ", i, (long)font->glyphs[i].offset);
                MFT_APPEND_FMT1("%ld\n", (long)font->glyphs[i].width);
            } else {
                MFT_APPEND_FMT1("0x%02X\n", i);
            }
        }
    }
#undef MFT_APPEND_FMT0
#undef MFT_APPEND_FMT1
#undef MFT_APPEND_FMT2
    if (dst == 0 || pos + 1UL > *dst_size) {
        *dst_size = pos + 1UL;
        return MFT_ERR_CAPACITY;
    }
    dst[pos] = 0;
    *dst_size = pos;
    return MFT_OK;
}
