#include "gmyy_spritefont.h"
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

static void gmyy_copy_str(char* dst, int cap, const char* src) {
    int i;
    if (!dst || cap <= 0) return;
    if (!src) { dst[0] = '\0'; return; }
    for (i = 0; i < cap - 1 && src[i]; ++i) dst[i] = src[i];
    dst[i] = '\0';
}

static void gmyy_set_status(GMYY_Status* st, int code, const char* msg) {
    if (!st) return;
    st->code = code;
    gmyy_copy_str(st->message, (int)sizeof(st->message), msg ? msg : "");
}

void gmyy_status_clear(GMYY_Status* st) {
    if (!st) return;
    st->code = GMYY_OK;
    st->warnings = 0;
    st->message[0] = '\0';
}

void gmyy_sprite_clear(GMYY_Sprite* s) {
    if (s) memset(s, 0, sizeof(*s));
}
void gmyy_font_call_clear(GMYY_FontCall* c) {
    if (c) memset(c, 0, sizeof(*c));
}
void gmyy_font_clear(GMYY_SpriteFont* f) {
    int i;
    if (!f) return;
    memset(f, 0, sizeof(*f));
    for (i = 0; i < 256; ++i) f->ascii_lut[i] = -1;
}

static const char* gmyy_find_key(const char* text, const char* key) {
    char pat[128];
    int i = 0;
    if (!text || !key) return 0;
    pat[i++] = '"';
    while (*key && i < 120) pat[i++] = *key++;
    pat[i++] = '"';
    pat[i] = '\0';
    return strstr(text, pat);
}

static const char* gmyy_after_colon(const char* p) {
    if (!p) return 0;
    while (*p && *p != ':') ++p;
    if (*p == ':') ++p;
    while (*p && isspace((unsigned char)*p)) ++p;
    return p;
}

static int gmyy_parse_json_string_at(const char* p, char* out, int cap) {
    int i = 0;
    if (!p || !out || cap <= 0) return GMYY_ERR;
    while (*p && *p != '"') ++p;
    if (*p != '"') return GMYY_ERR;
    ++p;
    while (*p && *p != '"' && i < cap - 1) {
        if (*p == '\\') {
            ++p;
            if (!*p) break;
            if (*p == 'n') out[i++] = '\n';
            else if (*p == 't') out[i++] = '\t';
            else if (*p == 'r') out[i++] = '\r';
            else out[i++] = *p;
            ++p;
        } else {
            out[i++] = *p++;
        }
    }
    out[i] = '\0';
    return GMYY_OK;
}

static int gmyy_json_get_string(const char* text, const char* key, char* out, int cap) {
    const char* p = gmyy_find_key(text, key);
    if (!p) return GMYY_ERR;
    p = gmyy_after_colon(p);
    return gmyy_parse_json_string_at(p, out, cap);
}

static int gmyy_json_get_int(const char* text, const char* key, int* out) {
    const char* p = gmyy_find_key(text, key);
    int sign = 1, v = 0, any = 0;
    if (!p || !out) return GMYY_ERR;
    p = gmyy_after_colon(p);
    if (*p == '-') { sign = -1; ++p; }
    while (*p >= '0' && *p <= '9') { any = 1; v = v * 10 + (*p - '0'); ++p; }
    if (!any) return GMYY_ERR;
    *out = v * sign;
    return GMYY_OK;
}

static int gmyy_count_occurrences(const char* text, const char* needle) {
    int n = 0;
    const char* p = text;
    while (p && *p) {
        p = strstr(p, needle);
        if (!p) break;
        ++n;
        p += strlen(needle);
    }
    return n;
}

static int gmyy_extract_name_after_marker(const char* p, char* out, int cap) {
    const char* q;
    if (!p) return GMYY_ERR;
    q = strstr(p, "%Name");
    if (!q) q = strstr(p, "name");
    if (!q) return GMYY_ERR;
    q = gmyy_after_colon(q);
    return gmyy_parse_json_string_at(q, out, cap);
}

static void gmyy_make_layer_path(const char* sprite_name, const char* frame, const char* layer, char* out, int cap) {
    char tmp[GMYY_MAX_PATH];
    tmp[0] = '\0';
    strcat(tmp, "sprites/");
    strncat(tmp, sprite_name, sizeof(tmp) - strlen(tmp) - 1);
    strncat(tmp, "/layers/", sizeof(tmp) - strlen(tmp) - 1);
    strncat(tmp, frame, sizeof(tmp) - strlen(tmp) - 1);
    strncat(tmp, "/", sizeof(tmp) - strlen(tmp) - 1);
    strncat(tmp, layer, sizeof(tmp) - strlen(tmp) - 1);
    strncat(tmp, ".png", sizeof(tmp) - strlen(tmp) - 1);
    gmyy_copy_str(out, cap, tmp);
}

int gmyy_decode_sprite_yy(const char* yy_text, GMYY_Sprite* out, GMYY_Status* st) {
    const char* p;
    int i;
    char layer0[GMYY_MAX_NAME];
    if (st) gmyy_status_clear(st);
    if (!yy_text || !out) { gmyy_set_status(st, GMYY_ERR, "null argument"); return GMYY_ERR; }
    gmyy_sprite_clear(out);

    if (gmyy_json_get_string(yy_text, "resourceType", out->resource_type, GMYY_MAX_NAME) != GMYY_OK)
        gmyy_copy_str(out->resource_type, GMYY_MAX_NAME, "GMSprite");
    if (strcmp(out->resource_type, "GMSprite") != 0) {
        gmyy_set_status(st, GMYY_ERR, "not a GMSprite yy");
        return GMYY_ERR;
    }
    if (gmyy_json_get_string(yy_text, "%Name", out->name, GMYY_MAX_NAME) != GMYY_OK)
        gmyy_json_get_string(yy_text, "name", out->name, GMYY_MAX_NAME);
    gmyy_json_get_int(yy_text, "width", &out->width);
    gmyy_json_get_int(yy_text, "height", &out->height);
    gmyy_json_get_int(yy_text, "xorigin", &out->xorigin);
    gmyy_json_get_int(yy_text, "yorigin", &out->yorigin);
    gmyy_json_get_int(yy_text, "bbox_left", &out->bbox_left);
    gmyy_json_get_int(yy_text, "bbox_top", &out->bbox_top);
    gmyy_json_get_int(yy_text, "bbox_right", &out->bbox_right);
    gmyy_json_get_int(yy_text, "bbox_bottom", &out->bbox_bottom);

    /* Modern GameMaker frames are GMSpriteFrame entries with %Name UUID-ish ids. */
    p = yy_text;
    while ((p = strstr(p, "\"$GMSpriteFrame\"")) != 0 && out->frame_count < GMYY_MAX_FRAMES) {
        GMYY_Frame* fr = &out->frames[out->frame_count];
        if (gmyy_extract_name_after_marker(p, fr->name, GMYY_MAX_NAME) != GMYY_OK) {
            char fallback[32];
            fallback[0] = 'f'; fallback[1] = 'r'; fallback[2] = '\0';
            gmyy_copy_str(fr->name, GMYY_MAX_NAME, fallback);
        }
        fr->x = 0; fr->y = 0; fr->w = out->width; fr->h = out->height;
        ++out->frame_count;
        p += 13;
    }

    /* Layers. Usually one layer is enough for the composited image path. */
    p = yy_text;
    while ((p = strstr(p, "\"$GMImageLayer\"")) != 0 && out->layer_count < GMYY_MAX_LAYERS) {
        GMYY_Layer* ly = &out->layers[out->layer_count];
        gmyy_extract_name_after_marker(p, ly->name, GMYY_MAX_NAME);
        ++out->layer_count;
        p += 12;
    }

    /* Compact demo compatibility. */
    if (out->frame_count == 0) {
        int fc = 0;
        if (gmyy_json_get_int(yy_text, "frameCount", &fc) == GMYY_OK && fc > 0) {
            if (fc > GMYY_MAX_FRAMES) { fc = GMYY_MAX_FRAMES; if (st) st->warnings++; }
            for (i = 0; i < fc; ++i) {
                out->frames[i].x = 0; out->frames[i].y = 0; out->frames[i].w = out->width; out->frames[i].h = out->height;
                out->frames[i].name[0] = 'f'; out->frames[i].name[1] = (char)('0' + (i % 10)); out->frames[i].name[2] = '\0';
            }
            out->frame_count = fc;
        } else {
            int n = gmyy_count_occurrences(yy_text, "\"image\"");
            if (n <= 0) n = 1;
            if (n > GMYY_MAX_FRAMES) { n = GMYY_MAX_FRAMES; if (st) st->warnings++; }
            for (i = 0; i < n; ++i) {
                out->frames[i].x = 0; out->frames[i].y = 0; out->frames[i].w = out->width; out->frames[i].h = out->height;
                out->frames[i].name[0] = 'f'; out->frames[i].name[1] = (char)('0' + (i % 10)); out->frames[i].name[2] = '\0';
            }
            out->frame_count = n;
        }
    }
    if (out->layer_count == 0) {
        gmyy_copy_str(out->layers[0].name, GMYY_MAX_NAME, "layer0");
        out->layer_count = 1;
    }

    layer0[0] = '\0';
    gmyy_copy_str(layer0, GMYY_MAX_NAME, out->layers[0].name);
    for (i = 0; i < out->frame_count; ++i) {
        if (out->frames[i].image_path[0] == '\0')
            gmyy_make_layer_path(out->name, out->frames[i].name, layer0, out->frames[i].image_path, GMYY_MAX_PATH);
        if (out->frames[i].w <= 0) out->frames[i].w = out->width;
        if (out->frames[i].h <= 0) out->frames[i].h = out->height;
    }
    gmyy_set_status(st, GMYY_OK, "decoded GMSprite yy");
    return GMYY_OK;
}

static const char* gmyy_skip_ws(const char* p) { while (p && *p && isspace((unsigned char)*p)) ++p; return p; }

static int gmyy_parse_ident(const char** pp, char* out, int cap) {
    const char* p = gmyy_skip_ws(*pp);
    int i = 0;
    if (!p || !out) return GMYY_ERR;
    while (*p && (isalnum((unsigned char)*p) || *p == '_' || *p == '.')) {
        if (i < cap - 1) out[i++] = *p;
        ++p;
    }
    out[i] = '\0';
    *pp = p;
    return i > 0 ? GMYY_OK : GMYY_ERR;
}

static int gmyy_parse_gml_string(const char** pp, char* out, int cap) {
    const char* p = gmyy_skip_ws(*pp);
    int r = gmyy_parse_json_string_at(p, out, cap);
    if (r == GMYY_OK) {
        ++p; while (*p && *p != '"') { if (*p == '\\' && p[1]) ++p; ++p; }
        if (*p == '"') ++p;
        *pp = p;
    }
    return r;
}

static int gmyy_parse_bool_or_int(const char** pp, int* out) {
    const char* p = gmyy_skip_ws(*pp);
    if (strncmp(p, "true", 4) == 0) { *out = 1; *pp = p + 4; return GMYY_OK; }
    if (strncmp(p, "false", 5) == 0) { *out = 0; *pp = p + 5; return GMYY_OK; }
    *out = atoi(p);
    while (*p && (*p == '-' || (*p >= '0' && *p <= '9'))) ++p;
    *pp = p;
    return GMYY_OK;
}

static unsigned long gmyy_parse_ord_or_int(const char** pp) {
    const char* p = gmyy_skip_ws(*pp);
    unsigned long v = 0;
    if (strncmp(p, "ord", 3) == 0) {
        char tmp[16];
        p = strchr(p, '(');
        if (p) { ++p; if (gmyy_parse_gml_string(&p, tmp, (int)sizeof(tmp)) == GMYY_OK) { const char* q = tmp; v = gmyy_utf8_decode_one(&q); } }
        p = strchr(p, ')'); if (p) ++p;
        *pp = p;
        return v;
    }
    while (*p >= '0' && *p <= '9') { v = v * 10UL + (unsigned long)(*p - '0'); ++p; }
    *pp = p;
    return v;
}

int gmyy_decode_font_call_gml(const char* gml_text, GMYY_FontCall* out, GMYY_Status* st) {
    const char* p;
    const char* q;
    char lhs[GMYY_MAX_NAME];
    if (st) gmyy_status_clear(st);
    if (!gml_text || !out) { gmyy_set_status(st, GMYY_ERR, "null argument"); return GMYY_ERR; }
    gmyy_font_call_clear(out);

    p = strstr(gml_text, "font_add_sprite_ext");
    if (p) out->kind = GMYY_FONT_CALL_ADD_EXT;
    else {
        p = strstr(gml_text, "font_add_sprite");
        if (p) out->kind = GMYY_FONT_CALL_ADD;
    }
    if (!p) { gmyy_set_status(st, GMYY_ERR, "no font_add_sprite call found"); return GMYY_ERR; }

    lhs[0] = '\0';
    q = p;
    while (q > gml_text && *q != '\n' && *q != ';') --q;
    if (*q == '\n' || *q == ';') ++q;
    {
        const char* eq = strchr(q, '=');
        if (eq && eq < p) {
            int n = (int)(eq - q);
            while (n > 0 && isspace((unsigned char)q[n-1])) --n;
            if (n >= GMYY_MAX_NAME) n = GMYY_MAX_NAME - 1;
            memcpy(lhs, q, n); lhs[n] = '\0';
            gmyy_copy_str(out->out_name, GMYY_MAX_NAME, lhs);
        }
    }

    p = strchr(p, '(');
    if (!p) { gmyy_set_status(st, GMYY_ERR, "bad call"); return GMYY_ERR; }
    ++p;
    if (gmyy_parse_ident(&p, out->sprite_name, GMYY_MAX_NAME) != GMYY_OK) { gmyy_set_status(st, GMYY_ERR, "bad sprite argument"); return GMYY_ERR; }
    p = strchr(p, ','); if (!p) { gmyy_set_status(st, GMYY_ERR, "missing second arg"); return GMYY_ERR; } ++p;

    if (out->kind == GMYY_FONT_CALL_ADD_EXT) {
        if (gmyy_parse_gml_string(&p, out->string_map, GMYY_MAX_MAP_BYTES) != GMYY_OK) { gmyy_set_status(st, GMYY_ERR, "bad string_map"); return GMYY_ERR; }
    } else {
        out->first_codepoint = gmyy_parse_ord_or_int(&p);
    }
    p = strchr(p, ','); if (!p) { gmyy_set_status(st, GMYY_ERR, "missing prop arg"); return GMYY_ERR; } ++p;
    gmyy_parse_bool_or_int(&p, &out->proportional);
    p = strchr(p, ','); if (!p) { gmyy_set_status(st, GMYY_ERR, "missing sep arg"); return GMYY_ERR; } ++p;
    out->separation = atoi(p);
    gmyy_set_status(st, GMYY_OK, "decoded font_add_sprite call");
    return GMYY_OK;
}

unsigned long gmyy_utf8_decode_one(const char** pp) {
    const unsigned char* s = (const unsigned char*)(*pp);
    unsigned long cp;
    if (!s || !*s) return 0;
    if (s[0] < 0x80) { *pp = (const char*)(s + 1); return s[0]; }
    if ((s[0] & 0xE0) == 0xC0 && s[1]) { cp = ((unsigned long)(s[0] & 0x1F) << 6) | (s[1] & 0x3F); *pp = (const char*)(s + 2); return cp; }
    if ((s[0] & 0xF0) == 0xE0 && s[1] && s[2]) { cp = ((unsigned long)(s[0] & 0x0F) << 12) | ((unsigned long)(s[1] & 0x3F) << 6) | (s[2] & 0x3F); *pp = (const char*)(s + 3); return cp; }
    if ((s[0] & 0xF8) == 0xF0 && s[1] && s[2] && s[3]) { cp = ((unsigned long)(s[0] & 0x07) << 18) | ((unsigned long)(s[1] & 0x3F) << 12) | ((unsigned long)(s[2] & 0x3F) << 6) | (s[3] & 0x3F); *pp = (const char*)(s + 4); return cp; }
    *pp = (const char*)(s + 1); return 0xFFFDUL;
}

GMYY_Fixed gmyy_int_to_fx(int v) { return ((GMYY_Fixed)v) << GMYY_FX_SHIFT; }
int gmyy_fx_to_int_ceil(GMYY_Fixed v) { return (int)((v + GMYY_FX_ONE - 1) >> GMYY_FX_SHIFT); }

int gmyy_build_spritefont(const GMYY_Sprite* sprite, const GMYY_FontCall* call, GMYY_SpriteFont* out, GMYY_Status* st) {
    int i;
    const char* p;
    if (st) gmyy_status_clear(st);
    if (!sprite || !call || !out) { gmyy_set_status(st, GMYY_ERR, "null argument"); return GMYY_ERR; }
    gmyy_font_clear(out);
    gmyy_copy_str(out->name, GMYY_MAX_NAME, call->out_name[0] ? call->out_name : "spritefont");
    gmyy_copy_str(out->sprite_name, GMYY_MAX_NAME, sprite->name);
    out->proportional = call->proportional;
    out->separation = call->separation;
    out->space_width = sprite->width / 2;
    if (out->space_width < 1) out->space_width = 1;
    out->line_height = sprite->height;
    out->baseline = sprite->height;
    out->missing_codepoint = (unsigned long)'?';

    p = call->string_map;
    i = 0;
    while (i < sprite->frame_count && i < GMYY_MAX_GLYPHS) {
        unsigned long cp;
        if (call->kind == GMYY_FONT_CALL_ADD_EXT) {
            if (!p || !*p) break;
            cp = gmyy_utf8_decode_one(&p);
        } else {
            cp = call->first_codepoint + (unsigned long)i;
        }
        out->glyphs[i].codepoint = cp;
        out->glyphs[i].frame_index = i;
        out->glyphs[i].x = 0;
        out->glyphs[i].y = 0;
        out->glyphs[i].w = sprite->frames[i].w > 0 ? sprite->frames[i].w : sprite->width;
        out->glyphs[i].h = sprite->frames[i].h > 0 ? sprite->frames[i].h : sprite->height;
        out->glyphs[i].xoffset = -sprite->xorigin;
        out->glyphs[i].yoffset = -sprite->yorigin;
        out->glyphs[i].xadvance = out->glyphs[i].w + call->separation;
        out->glyphs[i].fxadvance = gmyy_int_to_fx(out->glyphs[i].xadvance);
        if (cp < 256UL) out->ascii_lut[(int)cp] = i;
        ++i;
    }
    out->glyph_count = i;
    if (i == 0) { gmyy_set_status(st, GMYY_ERR, "no glyphs built"); return GMYY_ERR; }
    gmyy_set_status(st, GMYY_OK, "built sprite font");
    return GMYY_OK;
}

const GMYY_Glyph* gmyy_font_find_glyph(const GMYY_SpriteFont* font, unsigned long cp) {
    int i;
    if (!font) return 0;
    if (cp < 256UL && font->ascii_lut[(int)cp] >= 0) return &font->glyphs[font->ascii_lut[(int)cp]];
    for (i = 0; i < font->glyph_count; ++i) if (font->glyphs[i].codepoint == cp) return &font->glyphs[i];
    if (font->missing_codepoint && cp != font->missing_codepoint) return gmyy_font_find_glyph(font, font->missing_codepoint);
    return 0;
}

int gmyy_text_measure(const GMYY_SpriteFont* font, const char* utf8, int* w_out, int* h_out) {
    const char* p;
    int x = 0, maxx = 0, lines = 1;
    if (!font || !utf8 || !w_out || !h_out) return GMYY_ERR;
    p = utf8;
    while (*p) {
        unsigned long cp = gmyy_utf8_decode_one(&p);
        if (cp == '\n') { if (x > maxx) maxx = x; x = 0; ++lines; }
        else if (cp == ' ') x += font->space_width + font->separation;
        else {
            const GMYY_Glyph* g = gmyy_font_find_glyph(font, cp);
            if (g) x += g->xadvance;
        }
    }
    if (x > maxx) maxx = x;
    *w_out = maxx;
    *h_out = lines * font->line_height;
    return GMYY_OK;
}

int gmyy_scan_rgba_bounds(const unsigned char* rgba, int width, int height, int stride, int threshold, GMYY_Bounds* out) {
    int x, y;
    int l, t, r, b;
    if (!rgba || !out || width <= 0 || height <= 0 || stride < width * 4) return GMYY_ERR;
    if (threshold < 0) threshold = 0;
    if (threshold > 255) threshold = 255;
    l = width; t = height; r = -1; b = -1;
    for (y = 0; y < height; ++y) {
        const unsigned char* row = rgba + (long)y * stride;
        for (x = 0; x < width; ++x) {
            unsigned char a = row[x * 4 + 3];
            if ((int)a >= threshold) {
                if (x < l) l = x;
                if (x > r) r = x;
                if (y < t) t = y;
                if (y > b) b = y;
            }
        }
    }
    out->found = (r >= l && b >= t);
    if (!out->found) { out->left = out->top = out->right = out->bottom = out->width = out->height = 0; return GMYY_OK; }
    out->left = l; out->top = t; out->right = r; out->bottom = b; out->width = r - l + 1; out->height = b - t + 1;
    return GMYY_OK;
}

static unsigned short rd16le(const unsigned char* p) { return (unsigned short)(p[0] | (p[1] << 8)); }
static unsigned long rd32le(const unsigned char* p) { return ((unsigned long)p[0]) | ((unsigned long)p[1] << 8) | ((unsigned long)p[2] << 16) | ((unsigned long)p[3] << 24); }
static long rd32s_le(const unsigned char* p) { return (long)rd32le(p); }

static int rgb_matches_key(unsigned char r, unsigned char g, unsigned char b, unsigned long key) {
    return r == ((key >> 16) & 255UL) && g == ((key >> 8) & 255UL) && b == (key & 255UL);
}

int gmyy_scan_bmp_bounds(const unsigned char* data, long size, int threshold, unsigned long color_key_rgb, int flags, GMYY_Bounds* out, GMYY_ImageInfo* info) {
    long off, dib, w, h, abs_h, row_bytes, y;
    int bpp, top_down;
    int l, t, r, b;
    if (!data || size < 54 || !out) return GMYY_ERR;
    if (data[0] != 'B' || data[1] != 'M') return GMYY_ERR;
    off = (long)rd32le(data + 10);
    dib = (long)rd32le(data + 14);
    if (dib < 40 || off < 0 || off >= size) return GMYY_ERR;
    w = rd32s_le(data + 18);
    h = rd32s_le(data + 22);
    bpp = (int)rd16le(data + 28);
    if (w <= 0 || h == 0 || (bpp != 24 && bpp != 32)) return GMYY_ERR;
    top_down = h < 0;
    abs_h = top_down ? -h : h;
    row_bytes = ((w * bpp + 31) / 32) * 4;
    if (off + row_bytes * abs_h > size) return GMYY_ERR;
    if (info) { info->width = (int)w; info->height = (int)abs_h; info->bpp = bpp; info->top_down = top_down; info->has_alpha = (bpp == 32); }
    l = (int)w; t = (int)abs_h; r = -1; b = -1;
    for (y = 0; y < abs_h; ++y) {
        long src_y = top_down ? y : (abs_h - 1 - y);
        const unsigned char* row = data + off + src_y * row_bytes;
        long x;
        for (x = 0; x < w; ++x) {
            unsigned char bb = row[x * (bpp / 8) + 0];
            unsigned char gg = row[x * (bpp / 8) + 1];
            unsigned char rr = row[x * (bpp / 8) + 2];
            unsigned char aa = (bpp == 32) ? row[x * 4 + 3] : 255;
            int visible = 1;
            if ((flags & GMYY_SCAN_USE_ALPHA) && bpp == 32) visible = ((int)aa >= threshold);
            if ((flags & GMYY_SCAN_USE_COLORKEY) && rgb_matches_key(rr, gg, bb, color_key_rgb)) visible = 0;
            if (visible) { if ((int)x < l) l = (int)x; if ((int)x > r) r = (int)x; if ((int)y < t) t = (int)y; if ((int)y > b) b = (int)y; }
        }
    }
    out->found = (r >= l && b >= t);
    if (!out->found) { out->left = out->top = out->right = out->bottom = out->width = out->height = 0; return GMYY_OK; }
    out->left = l; out->top = t; out->right = r; out->bottom = b; out->width = r - l + 1; out->height = b - t + 1;
    return GMYY_OK;
}

int gmyy_scan_tga_bounds(const unsigned char* data, long size, int threshold, unsigned long color_key_rgb, int flags, GMYY_Bounds* out, GMYY_ImageInfo* info) {
    int idlen, cmap, type, w, h, bpp, desc, top_origin;
    long off, x, y;
    int l, t, r, b;
    if (!data || size < 18 || !out) return GMYY_ERR;
    idlen = data[0]; cmap = data[1]; type = data[2];
    if (cmap != 0 || (type != 2 && type != 3)) return GMYY_ERR; /* uncompressed truecolor or grayscale */
    w = (int)rd16le(data + 12); h = (int)rd16le(data + 14); bpp = data[16]; desc = data[17];
    if (w <= 0 || h <= 0 || !(bpp == 8 || bpp == 24 || bpp == 32)) return GMYY_ERR;
    off = 18 + idlen;
    if (off + (long)w * h * (bpp / 8) > size) return GMYY_ERR;
    top_origin = (desc & 0x20) != 0;
    if (info) { info->width = w; info->height = h; info->bpp = bpp; info->top_down = top_origin; info->has_alpha = (bpp == 32); }
    l = w; t = h; r = -1; b = -1;
    for (y = 0; y < h; ++y) {
        long src_y = top_origin ? y : (h - 1 - y);
        const unsigned char* row = data + off + src_y * w * (bpp / 8);
        for (x = 0; x < w; ++x) {
            unsigned char rr, gg, bb, aa;
            int visible = 1;
            if (bpp == 8) { rr = gg = bb = row[x]; aa = 255; }
            else { bb = row[x * (bpp / 8) + 0]; gg = row[x * (bpp / 8) + 1]; rr = row[x * (bpp / 8) + 2]; aa = (bpp == 32) ? row[x * 4 + 3] : 255; }
            if ((flags & GMYY_SCAN_USE_ALPHA) && bpp == 32) visible = ((int)aa >= threshold);
            if ((flags & GMYY_SCAN_USE_COLORKEY) && rgb_matches_key(rr, gg, bb, color_key_rgb)) visible = 0;
            if (visible) { if ((int)x < l) l = (int)x; if ((int)x > r) r = (int)x; if ((int)y < t) t = (int)y; if ((int)y > b) b = (int)y; }
        }
    }
    out->found = (r >= l && b >= t);
    if (!out->found) { out->left = out->top = out->right = out->bottom = out->width = out->height = 0; return GMYY_OK; }
    out->left = l; out->top = t; out->right = r; out->bottom = b; out->width = r - l + 1; out->height = b - t + 1;
    return GMYY_OK;
}

static int has_ext(const char* path, const char* ext) {
    int lp, le;
    if (!path || !ext) return 0;
    lp = (int)strlen(path); le = (int)strlen(ext);
    if (lp < le) return 0;
    return strcmp(path + lp - le, ext) == 0;
}

int gmyy_scan_image_bounds_by_ext(const char* path, const unsigned char* data, long size, int threshold, unsigned long color_key_rgb, int flags, GMYY_Bounds* out, GMYY_ImageInfo* info) {
    if (path && (has_ext(path, ".bmp") || has_ext(path, ".BMP"))) return gmyy_scan_bmp_bounds(data, size, threshold, color_key_rgb, flags, out, info);
    if (path && (has_ext(path, ".tga") || has_ext(path, ".TGA"))) return gmyy_scan_tga_bounds(data, size, threshold, color_key_rgb, flags, out, info);
    if (data && size >= 2 && data[0] == 'B' && data[1] == 'M') return gmyy_scan_bmp_bounds(data, size, threshold, color_key_rgb, flags, out, info);
    if (data && size >= 18 && (data[2] == 2 || data[2] == 3)) return gmyy_scan_tga_bounds(data, size, threshold, color_key_rgb, flags, out, info);
    return GMYY_ERR;
}

int gmyy_font_apply_glyph_bounds(GMYY_SpriteFont* font, int glyph_index, const GMYY_Bounds* bnd) {
    GMYY_Glyph* g;
    if (!font || !bnd || glyph_index < 0 || glyph_index >= font->glyph_count) return GMYY_ERR;
    g = &font->glyphs[glyph_index];
    if (!bnd->found) {
        g->trim_left = g->trim_top = g->trim_right = g->trim_bottom = 0;
        g->w = 0;
        g->xadvance = font->separation;
        if (g->xadvance < 0) g->xadvance = 0;
        g->fxadvance = gmyy_int_to_fx(g->xadvance);
        g->has_trim = 1;
        return GMYY_OK;
    }
    g->trim_left = bnd->left;
    g->trim_top = bnd->top;
    g->trim_right = bnd->right;
    g->trim_bottom = bnd->bottom;
    g->xoffset += bnd->left;
    g->yoffset += bnd->top;
    g->w = bnd->width;
    g->h = bnd->height;
    g->xadvance = bnd->width + font->separation;
    if (g->xadvance < 0) g->xadvance = 0;
    g->fxadvance = gmyy_int_to_fx(g->xadvance);
    g->has_trim = 1;
    return GMYY_OK;
}

int gmyy_font_apply_proportional_rgba_provider(GMYY_SpriteFont* font, const GMYY_Sprite* sprite, GMYY_RGBAProvider provider, unsigned char* scratch_rgba, long scratch_cap, int threshold, void* user, GMYY_Status* st) {
    int i;
    int applied = 0;
    if (st) gmyy_status_clear(st);
    if (!font || !sprite || !provider || !scratch_rgba || scratch_cap <= 0) { gmyy_set_status(st, GMYY_ERR, "null argument"); return GMYY_ERR; }
    for (i = 0; i < font->glyph_count; ++i) {
        int fi = font->glyphs[i].frame_index;
        int w = 0, h = 0, stride = 0;
        GMYY_Bounds bnd;
        if (fi < 0 || fi >= sprite->frame_count) continue;
        if (provider(sprite->frames[fi].image_path, scratch_rgba, scratch_cap, &w, &h, &stride, user) == GMYY_OK) {
            if (gmyy_scan_rgba_bounds(scratch_rgba, w, h, stride, threshold, &bnd) == GMYY_OK) {
                gmyy_font_apply_glyph_bounds(font, i, &bnd);
                ++applied;
            } else if (st) st->warnings++;
        } else if (st) st->warnings++;
    }
    if (applied <= 0) { gmyy_set_status(st, GMYY_ERR, "no glyph image bounds applied"); return GMYY_ERR; }
    gmyy_set_status(st, GMYY_OK, "applied proportional alpha scan");
    return GMYY_OK;
}
