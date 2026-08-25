#include "monika_fontcore.h"

static mfc_u32 mfc_rd32be(const mfc_u8 *p)
{
    return (((mfc_u32)p[0]) << 24) | (((mfc_u32)p[1]) << 16) | (((mfc_u32)p[2]) << 8) | ((mfc_u32)p[3]);
}

static mfc_u32 mfc_rd32le(const mfc_u8 *p)
{
    return (((mfc_u32)p[3]) << 24) | (((mfc_u32)p[2]) << 16) | (((mfc_u32)p[1]) << 8) | ((mfc_u32)p[0]);
}

static int mfc_mem_has(const mfc_u8 *p, mfc_u32 n, const char *needle)
{
    mfc_u32 i;
    mfc_u32 j;
    mfc_u32 nl;
    if (!p || !needle) return 0;
    nl = 0u;
    while (needle[nl] != '\0') ++nl;
    if (nl == 0u || n < nl) return 0;
    for (i = 0u; i <= n - nl; ++i) {
        j = 0u;
        while (j < nl && p[i + j] == (mfc_u8)needle[j]) ++j;
        if (j == nl) return 1;
    }
    return 0;
}

static int mfc_is_sfnt_magic(mfc_u32 tag)
{
    if (tag == 0x00010000ul) return MFC_KIND_SFNT_TTF;
    if (tag == MFC_TAG('O','T','T','O')) return MFC_KIND_SFNT_OTF;
    if (tag == MFC_TAG('t','t','c','f')) return MFC_KIND_SFNT_TTC;
    if (tag == MFC_TAG('t','r','u','e')) return MFC_KIND_SFNT_TTF;
    if (tag == MFC_TAG('t','y','p','1')) return MFC_KIND_SFNT_OTF;
    return MFC_KIND_UNKNOWN;
}

static mfc_u32 mfc_find_sfnt_magic(const mfc_u8 *p, mfc_u32 n)
{
    mfc_u32 i;
    mfc_u32 limit;
    int k;
    if (!p || n < 4u) return 0xfffffffful;
    limit = n;
    if (limit > 32768u) limit = 32768u;
    for (i = 0u; i + 4u <= limit; ++i) {
        k = mfc_is_sfnt_magic(mfc_rd32be(p + i));
        if (k == MFC_KIND_SFNT_TTF || k == MFC_KIND_SFNT_OTF || k == MFC_KIND_SFNT_TTC) return i;
    }
    return 0xfffffffful;
}

static void mfc_zero_bytes(void *ptr, mfc_u32 size)
{
    mfc_u8 *p;
    mfc_u32 i;
    if (!ptr) return;
    p = (mfc_u8*)ptr;
    for (i = 0u; i < size; ++i) p[i] = 0u;
}

void mfc_scratch_init(MFC_Scratch *scratch,
                      mfc_u8 *sfnt_bytes,
                      mfc_u32 sfnt_cap,
                      mfc_u8 *work_bytes,
                      mfc_u32 work_cap)
{
    if (!scratch) return;
    scratch->sfnt_bytes = sfnt_bytes;
    scratch->sfnt_cap = sfnt_cap;
    scratch->sfnt_len = 0u;
    scratch->work_bytes = work_bytes;
    scratch->work_cap = work_cap;
    scratch->work_len = 0u;
}

void mfc_font_clear(MFC_Font *font)
{
    if (!font) return;
    mfc_zero_bytes(font, (mfc_u32)sizeof(MFC_Font));
    font->requested_kind = MFC_KIND_UNKNOWN;
    font->source_kind = MFC_KIND_UNKNOWN;
    font->effective_kind = MFC_KIND_UNKNOWN;
    font->last_error = MFC_OK;
}

int mfc_detect_kind(const void *bytes, mfc_u32 size)
{
    const mfc_u8 *p;
    mfc_u32 tag;
    mfc_u32 le_size;
    if (!bytes || size < 4u) return MFC_KIND_UNKNOWN;
    p = (const mfc_u8*)bytes;
    tag = mfc_rd32be(p);
    if (tag == MFC_TAG('w','O','F','F')) return MFC_KIND_WOFF1;
    if (tag == MFC_TAG('w','O','F','2')) return MFC_KIND_WOFF2;
    if (tag == MFC_TAG('E','l','e','c')) {
        if (size >= 12u && mfc_mem_has(p, 12u, "ElecbyteFnt")) return MFC_KIND_MUGEN_FNT;
    }
    if (mfc_is_sfnt_magic(tag) != MFC_KIND_UNKNOWN) return mfc_is_sfnt_magic(tag);

    if (mfc_mem_has(p, size < 512u ? size : 512u, "GMSprite")) return MFC_KIND_GMYY_SPRITE_YY;
    if (mfc_mem_has(p, size < 512u ? size : 512u, "font_add_sprite")) return MFC_KIND_GMYY_GML_CALL;
    if (mfc_mem_has(p, size < 1024u ? size : 1024u, "[Def]") ||
        mfc_mem_has(p, size < 1024u ? size : 1024u, "Type =") ||
        mfc_mem_has(p, size < 1024u ? size : 1024u, "BankType")) {
        return MFC_KIND_MUGEN_TEXT;
    }

    le_size = mfc_rd32le(p);
    if ((le_size == size || (le_size > 64u && le_size <= size + 16u)) &&
        mfc_find_sfnt_magic(p, size) != 0xfffffffful) {
        return MFC_KIND_EOT;
    }

    return MFC_KIND_UNKNOWN;
}

const char *mfc_kind_name(int kind)
{
    switch (kind) {
    case MFC_KIND_SFNT_TTF: return "SFNT/TrueType glyf";
    case MFC_KIND_SFNT_OTF: return "SFNT/OpenType CFF";
    case MFC_KIND_SFNT_TTC: return "TrueType Collection";
    case MFC_KIND_WOFF1: return "WOFF1 webfont";
    case MFC_KIND_WOFF2: return "WOFF2 webfont";
    case MFC_KIND_EOT: return "EOT embedded opentype";
    case MFC_KIND_GMYY_SPRITE_YY: return "GameMaker sprite .yy";
    case MFC_KIND_GMYY_GML_CALL: return "GameMaker font_add_sprite call";
    case MFC_KIND_MUGEN_TEXT: return "MUGEN FNT text";
    case MFC_KIND_MUGEN_FNT: return "MUGEN ElecbyteFnt binary";
    default: return "unknown";
    }
}

const char *mfc_error_string(int code)
{
    switch (code) {
    case MFC_OK: return "ok";
    case MFC_ERR_NULL: return "null argument";
    case MFC_ERR_RANGE: return "range error";
    case MFC_ERR_UNKNOWN_FORMAT: return "unknown font format";
    case MFC_ERR_OUTPUT_TOO_SMALL: return "output buffer too small";
    case MFC_ERR_DECODE: return "decode failed";
    case MFC_ERR_UNSUPPORTED: return "unsupported operation";
    case MFC_ERR_VENDOR: return "vendor decoder returned error";
    case MFC_ERR_BAD_ARG: return "bad argument";
    default: return "unknown error";
    }
}

static void mfc_set_name_from_sfnt_or_ttf(MFC_Font *font)
{
    char tmp[MFC_NAME_MAX];
    int r;
    unsigned short i;
    if (!font) return;
    font->name[0] = '\0';
    if ((font->features & MFC_FEATURE_TTF_GLYF) != 0ul) {
        r = gwt_get_name_ascii(&font->ttf, GWT_NAME_FULL, tmp, (unsigned short)sizeof(tmp));
        if (r == GWT_OK) {
            i = 0u;
            while (i + 1u < MFC_NAME_MAX && tmp[i] != '\0') { font->name[i] = tmp[i]; ++i; }
            font->name[i] = '\0';
            return;
        }
    }
    if ((font->features & MFC_FEATURE_OTF_CFF) != 0ul) {
        r = otf_get_name_ascii(&font->otf, 4u, tmp, (unsigned short)sizeof(tmp));
        if (r == OTF_OK) {
            i = 0u;
            while (i + 1u < MFC_NAME_MAX && tmp[i] != '\0') { font->name[i] = tmp[i]; ++i; }
            font->name[i] = '\0';
        }
    }
}

static int mfc_open_sfnt_bytes(MFC_Font *font, const mfc_u8 *bytes, mfc_u32 size, int source_kind)
{
    int r;
    int gr;
    if (!font || !bytes || size < 4u) return MFC_ERR_NULL;
    font->font_bytes = bytes;
    font->font_size = size;
    font->effective_kind = mfc_detect_kind(bytes, size);
    if (font->effective_kind == MFC_KIND_SFNT_TTC) {
        r = sfnt_open_ttc(&font->sfnt, bytes, (sfnt_u32)size, 0u);
    } else {
        r = sfnt_open(&font->sfnt, bytes, (sfnt_u32)size);
    }
    if (r != SFNT_OK) {
        font->last_error = r;
        return MFC_ERR_VENDOR;
    }
    font->features |= MFC_FEATURE_SFNT;
    if (font->sfnt.has_kern) font->features |= MFC_FEATURE_KERN;

    if (font->sfnt.has_glyf) {
        gr = gwt_font_init(&font->ttf, bytes, (unsigned long)size);
        if (gr == GWT_OK) {
            font->features |= MFC_FEATURE_TTF_GLYF;
        }
    }

    if (font->sfnt.has_cff || font->sfnt.has_cff2 || mfc_detect_kind(bytes, size) == MFC_KIND_SFNT_OTF) {
        r = otf_parse(&font->otf, bytes, (unsigned long)size);
        if (r == OTF_OK) {
            font->features |= MFC_FEATURE_OTF_CFF;
            if (otf_is_otf_cff2(&font->otf)) font->features |= MFC_FEATURE_CFF2;
            if (otf_has_table(&font->otf, MFC_TAG('H','V','A','R'))) font->features |= MFC_FEATURE_HVAR;
            if (otf_has_table(&font->otf, MFC_TAG('V','V','A','R'))) font->features |= MFC_FEATURE_VVAR;
            if (otf_has_table(&font->otf, MFC_TAG('M','V','A','R'))) font->features |= MFC_FEATURE_MVAR;
        }
    }
    if (source_kind == MFC_KIND_WOFF1) font->features |= MFC_FEATURE_WOFF1;
    if (source_kind == MFC_KIND_WOFF2) font->features |= MFC_FEATURE_WOFF2;
    if (source_kind == MFC_KIND_EOT) font->features |= MFC_FEATURE_EOT_SCAN;
    mfc_set_name_from_sfnt_or_ttf(font);
    return MFC_OK;
}

int mfc_open_memory(MFC_Font *font,
                    const void *bytes,
                    mfc_u32 size,
                    int kind_hint,
                    MFC_Scratch *scratch)
{
    const mfc_u8 *p;
    int kind;
    int r;
    mfc_u32 out_len;
    woff1_report w1_report;
    W2F_DecoderInfo w2_info;
    mfc_u32 off;

    if (!font || !bytes) return MFC_ERR_NULL;
    mfc_font_clear(font);
    p = (const mfc_u8*)bytes;
    font->source_bytes = p;
    font->source_size = size;
    font->requested_kind = kind_hint;
    kind = kind_hint == MFC_KIND_UNKNOWN ? mfc_detect_kind(bytes, size) : kind_hint;
    font->source_kind = kind;

    if (kind == MFC_KIND_UNKNOWN) {
        font->last_error = MFC_ERR_UNKNOWN_FORMAT;
        return MFC_ERR_UNKNOWN_FORMAT;
    }

    if (kind == MFC_KIND_SFNT_TTF || kind == MFC_KIND_SFNT_OTF || kind == MFC_KIND_SFNT_TTC) {
        return mfc_open_sfnt_bytes(font, p, size, kind);
    }

    if (kind == MFC_KIND_WOFF1) {
        if (!scratch || !scratch->sfnt_bytes || scratch->sfnt_cap == 0u) return MFC_ERR_OUTPUT_TOO_SMALL;
        scratch->sfnt_len = 0u;
        woff1_report_clear(&w1_report);
        r = woff1_decode_to_sfnt(p, (woff1_u32)size,
                                  scratch->sfnt_bytes, (woff1_u32)scratch->sfnt_cap,
                                  (woff1_u32*)&out_len, &w1_report);
        if (r != WOFF1_OK) {
            font->last_error = r;
            return MFC_ERR_VENDOR;
        }
        scratch->sfnt_len = out_len;
        return mfc_open_sfnt_bytes(font, scratch->sfnt_bytes, scratch->sfnt_len, MFC_KIND_WOFF1);
    }

    if (kind == MFC_KIND_WOFF2) {
        if (!scratch || !scratch->sfnt_bytes || !scratch->work_bytes) return MFC_ERR_OUTPUT_TOO_SMALL;
        scratch->sfnt_len = 0u;
        scratch->work_len = 0u;
        r = w2f_decode_woff2_to_sfnt(p, (W2F_U32)size,
                                      scratch->sfnt_bytes, (W2F_U32)scratch->sfnt_cap,
                                      (W2F_U32*)&out_len,
                                      scratch->work_bytes, (W2F_U32)scratch->work_cap,
                                      
#ifdef MFC_USE_GOOGLE_BROTLI
                                      w2f_brotli_google_static_decode,
#else
                                      w2f_brotli_stub_decode,
#endif
                                      &w2_info);
        if (r != W2F_OK) {
            font->last_error = r;
            return MFC_ERR_VENDOR;
        }
        scratch->sfnt_len = out_len;
        return mfc_open_sfnt_bytes(font, scratch->sfnt_bytes, scratch->sfnt_len, MFC_KIND_WOFF2);
    }

    if (kind == MFC_KIND_EOT) {
        off = mfc_find_sfnt_magic(p, size);
        if (off == 0xfffffffful || off >= size) {
            font->last_error = MFC_ERR_UNSUPPORTED;
            return MFC_ERR_UNSUPPORTED;
        }
        return mfc_open_sfnt_bytes(font, p + off, size - off, MFC_KIND_EOT);
    }

    if (kind == MFC_KIND_MUGEN_FNT) {
        r = (int)mft_fnt_parse_header(&font->mugen_header, p, (mft_u32)size);
        if (r != MFT_OK) {
            font->last_error = r;
            return MFC_ERR_VENDOR;
        }
        font->effective_kind = MFC_KIND_MUGEN_FNT;
        font->features |= MFC_FEATURE_MUGEN_FNT;
        return MFC_OK;
    }

    if (kind == MFC_KIND_MUGEN_TEXT) {
        return mfc_open_mugen_text(font, (const char*)bytes, size);
    }

    font->last_error = MFC_ERR_UNSUPPORTED;
    return MFC_ERR_UNSUPPORTED;
}

int mfc_open_gmyy_spritefont(MFC_Font *font,
                             const char *sprite_yy_text,
                             const char *font_call_gml_text)
{
    int r;
    GMYY_Status st;
    if (!font || !sprite_yy_text || !font_call_gml_text) return MFC_ERR_NULL;
    mfc_font_clear(font);
    gmyy_status_clear(&st);
    r = gmyy_decode_sprite_yy(sprite_yy_text, &font->gmyy_sprite, &st);
    if (r != GMYY_OK) {
        font->last_error = st.code;
        return MFC_ERR_VENDOR;
    }
    r = gmyy_decode_font_call_gml(font_call_gml_text, &font->gmyy_call, &st);
    if (r != GMYY_OK) {
        font->last_error = st.code;
        return MFC_ERR_VENDOR;
    }
    r = gmyy_build_spritefont(&font->gmyy_sprite, &font->gmyy_call, &font->gmyy_font, &st);
    if (r != GMYY_OK) {
        font->last_error = st.code;
        return MFC_ERR_VENDOR;
    }
    font->source_kind = MFC_KIND_GMYY_SPRITE_YY;
    font->effective_kind = MFC_KIND_GMYY_SPRITE_YY;
    font->features |= MFC_FEATURE_GMYY;
    return MFC_OK;
}

int mfc_open_mugen_text(MFC_Font *font,
                        const char *font_text,
                        mfc_u32 text_size)
{
    int r;
    if (!font || !font_text) return MFC_ERR_NULL;
    mfc_font_clear(font);
    r = (int)mft_font_text_parse(&font->mugen_text, font_text, (mft_u32)text_size);
    if (r != MFT_OK) {
        font->last_error = r;
        return MFC_ERR_VENDOR;
    }
    font->source_kind = MFC_KIND_MUGEN_TEXT;
    font->effective_kind = MFC_KIND_MUGEN_TEXT;
    font->features |= MFC_FEATURE_MUGEN_TEXT;
    return MFC_OK;
}

int mfc_lookup_glyph(const MFC_Font *font,
                     unsigned long codepoint,
                     unsigned short *glyph_id_out)
{
    int r;
    sfnt_u16 sgid;
    unsigned short gid;
    if (!font || !glyph_id_out) return MFC_ERR_NULL;
    *glyph_id_out = 0u;
    if ((font->features & MFC_FEATURE_TTF_GLYF) != 0ul) {
        r = gwt_codepoint_to_glyph(&font->ttf, codepoint, &gid);
        if (r == GWT_OK) { *glyph_id_out = gid; return MFC_OK; }
    }
    if ((font->features & MFC_FEATURE_OTF_CFF) != 0ul) {
        gid = otf_glyph_index_for_codepoint(&font->otf, codepoint);
        *glyph_id_out = gid;
        return MFC_OK;
    }
    if ((font->features & MFC_FEATURE_SFNT) != 0ul) {
        r = sfnt_lookup_glyph(&font->sfnt, (sfnt_u32)codepoint, &sgid);
        if (r == SFNT_OK) { *glyph_id_out = (unsigned short)sgid; return MFC_OK; }
    }
    if ((font->features & MFC_FEATURE_GMYY) != 0ul) {
        const GMYY_Glyph *gg;
        gg = gmyy_font_find_glyph(&font->gmyy_font, codepoint);
        if (gg) { *glyph_id_out = (unsigned short)gg->frame_index; return MFC_OK; }
    }
    return MFC_ERR_UNSUPPORTED;
}

int mfc_get_glyph_metrics(const MFC_Font *font,
                          unsigned long codepoint,
                          MFC_GlyphMetrics *out_metrics)
{
    unsigned short gid;
    int r;
    GWT_GlyphMetrics gm;
    sfnt_hmetric hm;
    if (!font || !out_metrics) return MFC_ERR_NULL;
    mfc_zero_bytes(out_metrics, (mfc_u32)sizeof(MFC_GlyphMetrics));
    out_metrics->codepoint = codepoint;
    r = mfc_lookup_glyph(font, codepoint, &gid);
    if (r != MFC_OK) return r;
    out_metrics->glyph_id = gid;
    if ((font->features & MFC_FEATURE_TTF_GLYF) != 0ul) {
        r = gwt_get_glyph_metrics(&font->ttf, gid, &gm);
        if (r == GWT_OK) {
            out_metrics->advance = gm.advance_width;
            out_metrics->left_side_bearing = gm.left_side_bearing;
            out_metrics->x_min = gm.x_min;
            out_metrics->y_min = gm.y_min;
            out_metrics->x_max = gm.x_max;
            out_metrics->y_max = gm.y_max;
            return MFC_OK;
        }
    }
    if ((font->features & MFC_FEATURE_OTF_CFF) != 0ul) {
        unsigned short adv;
        short lsb;
        r = otf_get_hmetric_var(&font->otf, gid, &adv, &lsb);
        if (r != OTF_OK) r = otf_get_hmetric(&font->otf, gid, &adv, &lsb);
        if (r == OTF_OK) {
            out_metrics->advance = (int)adv;
            out_metrics->left_side_bearing = (int)lsb;
            return MFC_OK;
        }
    }
    if ((font->features & MFC_FEATURE_SFNT) != 0ul) {
        r = sfnt_get_hmetric(&font->sfnt, (sfnt_u16)gid, &hm);
        if (r == SFNT_OK) {
            out_metrics->advance = (int)hm.advance_width;
            out_metrics->left_side_bearing = (int)hm.left_side_bearing;
            return MFC_OK;
        }
    }
    if ((font->features & MFC_FEATURE_GMYY) != 0ul) {
        const GMYY_Glyph *gg;
        gg = gmyy_font_find_glyph(&font->gmyy_font, codepoint);
        if (gg) {
            out_metrics->advance = gg->xadvance;
            out_metrics->left_side_bearing = gg->xoffset;
            out_metrics->x_min = (short)gg->x;
            out_metrics->y_min = (short)gg->y;
            out_metrics->x_max = (short)(gg->x + gg->w);
            out_metrics->y_max = (short)(gg->y + font->gmyy_font.line_height);
            return MFC_OK;
        }
    }
    return MFC_ERR_UNSUPPORTED;
}

int mfc_measure_utf8(const MFC_Font *font,
                     const char *text,
                     int pixel_size,
                     int *width_out,
                     int *height_out)
{
    int r;
    if (!font || !text || !width_out || !height_out) return MFC_ERR_NULL;
    *width_out = 0;
    *height_out = 0;
    if ((font->features & MFC_FEATURE_TTF_GLYF) != 0ul) {
        r = gwt_measure_utf8(&font->ttf, text, pixel_size, width_out, height_out);
        return r == GWT_OK ? MFC_OK : MFC_ERR_VENDOR;
    }
    if ((font->features & MFC_FEATURE_GMYY) != 0ul) {
        r = gmyy_text_measure(&font->gmyy_font, text, width_out, height_out);
        return r == GMYY_OK ? MFC_OK : MFC_ERR_VENDOR;
    }
    return MFC_ERR_UNSUPPORTED;
}

int mfc_ttf_build_atlas_ascii(const MFC_Font *font,
                              GWT_Atlas *atlas,
                              int pixel_size,
                              unsigned char samples_log2,
                              GWT_Outline *scratch_outline)
{
    int r;
    if (!font || !atlas || !scratch_outline) return MFC_ERR_NULL;
    if ((font->features & MFC_FEATURE_TTF_GLYF) == 0ul) return MFC_ERR_UNSUPPORTED;
    r = gwt_atlas_add_ascii(&font->ttf, atlas, pixel_size, samples_log2, scratch_outline);
    return r == GWT_OK ? MFC_OK : MFC_ERR_VENDOR;
}

int mfc_ttf_draw_text_bitmap_utf8(const MFC_Font *font,
                                  const GWT_Atlas *atlas,
                                  GWT_Bitmap *dst,
                                  const char *text,
                                  int pixel_size,
                                  int x,
                                  int y,
                                  unsigned char color,
                                  GWT_LayoutGlyph *layout_scratch,
                                  unsigned short max_layout)
{
    int r;
    if (!font || !atlas || !dst || !text || !layout_scratch) return MFC_ERR_NULL;
    if ((font->features & MFC_FEATURE_TTF_GLYF) == 0ul) return MFC_ERR_UNSUPPORTED;
    r = gwt_draw_text_bitmap_utf8(&font->ttf, atlas, dst, text, pixel_size, x, y, color, layout_scratch, max_layout);
    return r == GWT_OK ? MFC_OK : MFC_ERR_VENDOR;
}

int mfc_shape_static_utf8(const ghb_static_font *static_font,
                          ghb_buffer *buffer,
                          const char *text,
                          int byte_count,
                          const ghb_feature *features,
                          int feature_count)
{
    int r;
    if (!static_font || !buffer || !text) return MFC_ERR_NULL;
    ghb_buffer_clear(buffer);
    r = ghb_shape_utf8(static_font, buffer, text, byte_count, features, feature_count);
    return r >= 0 ? MFC_OK : MFC_ERR_VENDOR;
}
