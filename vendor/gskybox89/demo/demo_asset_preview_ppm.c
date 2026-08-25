#include <stdio.h>
#include "gskybox89.h"
#include "gskybox89_assets.h"

#define PREVIEW_W 360
#define PREVIEW_H 240
#define PREVIEW_NEAR 256L
#define PREVIEW_FOCAL 150L
#define PREVIEW_MAX_CLIP 8
#define FACE_TEX_W 256
#define FACE_TEX_H 256
#define MAX_SOURCE_W 4096
#define MAX_ROW_BYTES (MAX_SOURCE_W * 4)

static unsigned char g_pixels[PREVIEW_W * PREVIEW_H * 3];
static unsigned char g_face_tex[GSKYBOX89_CUBE_FACE_COUNT][FACE_TEX_W * FACE_TEX_H * 3];
static int g_face_present[GSKYBOX89_CUBE_FACE_COUNT];
static unsigned char g_row[MAX_ROW_BYTES];

typedef struct PreviewV {
    long x;
    long y;
    long z;
    long u;
    long v;
    int sx;
    int sy;
    int r;
    int g;
    int b;
    int a;
    int layer;
    int face;
} PreviewV;

typedef struct PreviewState {
    int pass;
    int tri_total;
    int tri_screen;
    int tri_cube;
    int tri_dome;
    int image_faces_loaded;
} PreviewState;

static int clamp_i(int v, int lo, int hi)
{
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

static int read_u16_le(FILE *f, unsigned int *out_v)
{
    int b0;
    int b1;
    b0 = fgetc(f);
    b1 = fgetc(f);
    if (b0 == EOF || b1 == EOF) return 0;
    *out_v = ((unsigned int)b1 << 8) | (unsigned int)b0;
    return 1;
}

static int read_u32_le(FILE *f, unsigned long *out_v)
{
    int b0;
    int b1;
    int b2;
    int b3;
    b0 = fgetc(f);
    b1 = fgetc(f);
    b2 = fgetc(f);
    b3 = fgetc(f);
    if (b0 == EOF || b1 == EOF || b2 == EOF || b3 == EOF) return 0;
    *out_v = ((unsigned long)b3 << 24) | ((unsigned long)b2 << 16) | ((unsigned long)b1 << 8) | (unsigned long)b0;
    return 1;
}

static long read_s32_le_from_ulong(unsigned long v)
{
    if ((v & 0x80000000UL) != 0UL) {
        return -((long)((~v + 1UL) & 0xFFFFFFFFUL));
    }
    return (long)v;
}

static void set_pixel_alpha(int x, int y, int r, int g, int b, int a)
{
    long idx;
    int dr;
    int dg;
    int db;
    int ia;
    if (x < 0 || x >= PREVIEW_W || y < 0 || y >= PREVIEW_H) return;
    a = clamp_i(a, 0, 255);
    r = clamp_i(r, 0, 255);
    g = clamp_i(g, 0, 255);
    b = clamp_i(b, 0, 255);
    idx = ((long)y * (long)PREVIEW_W + (long)x) * 3L;
    if (a >= 255) {
        g_pixels[idx + 0] = (unsigned char)r;
        g_pixels[idx + 1] = (unsigned char)g;
        g_pixels[idx + 2] = (unsigned char)b;
    } else if (a > 0) {
        ia = 255 - a;
        dr = (int)g_pixels[idx + 0];
        dg = (int)g_pixels[idx + 1];
        db = (int)g_pixels[idx + 2];
        g_pixels[idx + 0] = (unsigned char)((r * a + dr * ia) / 255);
        g_pixels[idx + 1] = (unsigned char)((g * a + dg * ia) / 255);
        g_pixels[idx + 2] = (unsigned char)((b * a + db * ia) / 255);
    }
}

static long edge_fn(int ax, int ay, int bx, int by, int cx, int cy)
{
    return (long)(cx - ax) * (long)(by - ay) - (long)(cy - ay) * (long)(bx - ax);
}

static void draw_line(int x0, int y0, int x1, int y1, int r, int g, int b, int a)
{
    int dx;
    int sx;
    int dy;
    int sy;
    int err;
    int e2;
    dx = x1 > x0 ? x1 - x0 : x0 - x1;
    sx = x0 < x1 ? 1 : -1;
    dy = y1 > y0 ? y0 - y1 : y1 - y0;
    sy = y0 < y1 ? 1 : -1;
    err = dx + dy;
    while (1) {
        set_pixel_alpha(x0, y0, r, g, b, a);
        if (x0 == x1 && y0 == y1) break;
        e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

static int ppm_next_token(FILE *f, char *tok, int cap)
{
    int c;
    int i;
    if (tok == 0 || cap <= 0) return 0;
    i = 0;
    c = fgetc(f);
    while (c != EOF) {
        if (c == '#') {
            while (c != EOF && c != '\n' && c != '\r') c = fgetc(f);
        } else if (c > ' ') {
            break;
        }
        c = fgetc(f);
    }
    if (c == EOF) return 0;
    while (c != EOF && c > ' ') {
        if (i < cap - 1) tok[i++] = (char)c;
        c = fgetc(f);
    }
    tok[i] = '\0';
    return i > 0;
}

static int str_to_int(const char *s)
{
    int v;
    int neg;
    v = 0;
    neg = 0;
    if (s != 0 && *s == '-') { neg = 1; ++s; }
    while (s != 0 && *s >= '0' && *s <= '9') {
        v = v * 10 + (*s - '0');
        ++s;
    }
    return neg ? -v : v;
}

static int load_ppm_downsample(const char *path, int face)
{
    FILE *f;
    char tok[32];
    int w;
    int h;
    int maxv;
    int y;
    int x;
    int tx;
    int src_y;
    int want_y;
    long idx;
    if (face < 0 || face >= GSKYBOX89_CUBE_FACE_COUNT) return 0;
    f = fopen(path, "rb");
    if (f == 0) return 0;
    if (!ppm_next_token(f, tok, (int)sizeof(tok)) || tok[0] != 'P' || tok[1] != '6') { fclose(f); return 0; }
    if (!ppm_next_token(f, tok, (int)sizeof(tok))) { fclose(f); return 0; }
    w = str_to_int(tok);
    if (!ppm_next_token(f, tok, (int)sizeof(tok))) { fclose(f); return 0; }
    h = str_to_int(tok);
    if (!ppm_next_token(f, tok, (int)sizeof(tok))) { fclose(f); return 0; }
    maxv = str_to_int(tok);
    if (w <= 0 || h <= 0 || w > MAX_SOURCE_W || maxv != 255) { fclose(f); return 0; }
    want_y = -1;
    for (y = 0; y < h; ++y) {
        if (fread(g_row, 1, (size_t)(w * 3), f) != (size_t)(w * 3)) { fclose(f); return 0; }
        src_y = (int)(((long)want_y + 1L) * (long)h / (long)FACE_TEX_H);
        while (src_y == y && want_y + 1 < FACE_TEX_H) {
            ++want_y;
            for (tx = 0; tx < FACE_TEX_W; ++tx) {
                x = (int)((long)tx * (long)w / (long)FACE_TEX_W);
                idx = ((long)want_y * (long)FACE_TEX_W + (long)tx) * 3L;
                g_face_tex[face][idx + 0] = g_row[x * 3 + 0];
                g_face_tex[face][idx + 1] = g_row[x * 3 + 1];
                g_face_tex[face][idx + 2] = g_row[x * 3 + 2];
            }
            src_y = (int)(((long)want_y + 1L) * (long)h / (long)FACE_TEX_H);
        }
    }
    fclose(f);
    g_face_present[face] = 1;
    return 1;
}

static int load_bmp_downsample(const char *path, int face)
{
    FILE *f;
    int c0;
    int c1;
    unsigned long tmp;
    unsigned long off_bits;
    unsigned int planes;
    unsigned int bpp;
    unsigned long compression;
    long sw;
    long sh_signed;
    int top_down;
    int h;
    int row_stride;
    int ty;
    int tx;
    int src_y;
    int file_y;
    int sx;
    long idx;
    if (face < 0 || face >= GSKYBOX89_CUBE_FACE_COUNT) return 0;
    f = fopen(path, "rb");
    if (f == 0) return 0;
    c0 = fgetc(f);
    c1 = fgetc(f);
    if (c0 != 'B' || c1 != 'M') { fclose(f); return 0; }
    if (!read_u32_le(f, &tmp)) { fclose(f); return 0; }
    if (!read_u32_le(f, &tmp)) { fclose(f); return 0; }
    if (!read_u32_le(f, &off_bits)) { fclose(f); return 0; }
    if (!read_u32_le(f, &tmp)) { fclose(f); return 0; }
    if (tmp < 40UL) { fclose(f); return 0; }
    if (!read_u32_le(f, &tmp)) { fclose(f); return 0; }
    sw = read_s32_le_from_ulong(tmp);
    if (!read_u32_le(f, &tmp)) { fclose(f); return 0; }
    sh_signed = read_s32_le_from_ulong(tmp);
    if (!read_u16_le(f, &planes)) { fclose(f); return 0; }
    if (!read_u16_le(f, &bpp)) { fclose(f); return 0; }
    if (!read_u32_le(f, &compression)) { fclose(f); return 0; }
    if (planes != 1U || compression != 0UL || sw <= 0 || (bpp != 24U && bpp != 32U)) { fclose(f); return 0; }
    top_down = 0;
    if (sh_signed < 0) { top_down = 1; h = (int)(-sh_signed); } else { h = (int)sh_signed; }
    if (h <= 0 || sw > MAX_SOURCE_W) { fclose(f); return 0; }
    row_stride = (int)((((long)bpp * sw + 31L) / 32L) * 4L);
    for (ty = 0; ty < FACE_TEX_H; ++ty) {
        src_y = (int)((long)ty * (long)h / (long)FACE_TEX_H);
        file_y = top_down ? src_y : (h - 1 - src_y);
        if (fseek(f, (long)off_bits + (long)file_y * (long)row_stride, SEEK_SET) != 0) { fclose(f); return 0; }
        if (fread(g_row, 1, (size_t)row_stride, f) != (size_t)row_stride) { fclose(f); return 0; }
        for (tx = 0; tx < FACE_TEX_W; ++tx) {
            sx = (int)((long)tx * sw / (long)FACE_TEX_W);
            idx = ((long)ty * (long)FACE_TEX_W + (long)tx) * 3L;
            g_face_tex[face][idx + 0] = g_row[sx * (int)(bpp / 8U) + 2];
            g_face_tex[face][idx + 1] = g_row[sx * (int)(bpp / 8U) + 1];
            g_face_tex[face][idx + 2] = g_row[sx * (int)(bpp / 8U) + 0];
        }
    }
    fclose(f);
    g_face_present[face] = 1;
    return 1;
}

static int load_tga_downsample(const char *path, int face)
{
    FILE *f;
    int id_len;
    int cmap_type;
    int img_type;
    unsigned int tmp16;
    unsigned int w16;
    unsigned int h16;
    int bpp;
    int desc;
    int origin_top;
    int bytes;
    int h;
    int w;
    int ty;
    int tx;
    int src_y;
    int file_y;
    int sx;
    long data_off;
    long row_stride;
    long idx;
    if (face < 0 || face >= GSKYBOX89_CUBE_FACE_COUNT) return 0;
    f = fopen(path, "rb");
    if (f == 0) return 0;
    id_len = fgetc(f);
    cmap_type = fgetc(f);
    img_type = fgetc(f);
    if (id_len == EOF || cmap_type == EOF || img_type == EOF) { fclose(f); return 0; }
    if (!read_u16_le(f, &tmp16)) { fclose(f); return 0; } /* color map first entry */
    if (!read_u16_le(f, &tmp16)) { fclose(f); return 0; } /* color map length */
    if (fgetc(f) == EOF) { fclose(f); return 0; }      /* color map depth */
    if (!read_u16_le(f, &tmp16)) { fclose(f); return 0; } /* x origin */
    if (!read_u16_le(f, &tmp16)) { fclose(f); return 0; } /* y origin */
    if (!read_u16_le(f, &w16)) { fclose(f); return 0; }
    if (!read_u16_le(f, &h16)) { fclose(f); return 0; }
    bpp = fgetc(f);
    desc = fgetc(f);
    if (bpp == EOF || desc == EOF) { fclose(f); return 0; }
    if (cmap_type != 0 || img_type != 2 || (bpp != 24 && bpp != 32)) { fclose(f); return 0; }
    w = (int)w16;
    h = (int)h16;
    if (w <= 0 || h <= 0 || w > MAX_SOURCE_W) { fclose(f); return 0; }
    bytes = bpp / 8;
    row_stride = (long)w * (long)bytes;
    data_off = 18L + (long)id_len;
    origin_top = (desc & 0x20) != 0;
    for (ty = 0; ty < FACE_TEX_H; ++ty) {
        src_y = (int)((long)ty * (long)h / (long)FACE_TEX_H);
        file_y = origin_top ? src_y : (h - 1 - src_y);
        if (fseek(f, data_off + (long)file_y * row_stride, SEEK_SET) != 0) { fclose(f); return 0; }
        if (fread(g_row, 1, (size_t)row_stride, f) != (size_t)row_stride) { fclose(f); return 0; }
        for (tx = 0; tx < FACE_TEX_W; ++tx) {
            sx = (int)((long)tx * (long)w / (long)FACE_TEX_W);
            idx = ((long)ty * (long)FACE_TEX_W + (long)tx) * 3L;
            g_face_tex[face][idx + 0] = g_row[sx * bytes + 2];
            g_face_tex[face][idx + 1] = g_row[sx * bytes + 1];
            g_face_tex[face][idx + 2] = g_row[sx * bytes + 0];
        }
    }
    fclose(f);
    g_face_present[face] = 1;
    return 1;
}

static int ends_with_ci(const char *s, const char *suffix)
{
    int sl;
    int tl;
    int i;
    if (s == 0 || suffix == 0) return 0;
    sl = 0;
    tl = 0;
    while (s[sl] != '\0') ++sl;
    while (suffix[tl] != '\0') ++tl;
    if (tl > sl) return 0;
    for (i = 0; i < tl; ++i) {
        int ca;
        int cb;
        ca = s[sl - tl + i];
        cb = suffix[i];
        if (ca >= 'A' && ca <= 'Z') ca += 'a' - 'A';
        if (cb >= 'A' && cb <= 'Z') cb += 'a' - 'A';
        if (ca != cb) return 0;
    }
    return 1;
}

static int load_face_image(const char *path, int face)
{
    if (ends_with_ci(path, ".ppm")) return load_ppm_downsample(path, face);
    if (ends_with_ci(path, ".bmp")) return load_bmp_downsample(path, face);
    if (ends_with_ci(path, ".tga")) return load_tga_downsample(path, face);
    return 0;
}

static void fill_missing_face(int face)
{
    int y;
    int x;
    long idx;
    int br;
    int bg;
    int bb;
    br = 90; bg = 110; bb = 150;
    if (face == GSKYBOX89_FACE_POS_X) { br = 115; bg = 80; bb = 170; }
    if (face == GSKYBOX89_FACE_NEG_X) { br = 60; bg = 130; bb = 170; }
    if (face == GSKYBOX89_FACE_POS_Y) { br = 25; bg = 60; bb = 140; }
    if (face == GSKYBOX89_FACE_NEG_Y) { br = 80; bg = 80; bb = 80; }
    if (face == GSKYBOX89_FACE_POS_Z) { br = 135; bg = 140; bb = 190; }
    if (face == GSKYBOX89_FACE_NEG_Z) { br = 70; bg = 70; bb = 135; }
    for (y = 0; y < FACE_TEX_H; ++y) {
        for (x = 0; x < FACE_TEX_W; ++x) {
            idx = ((long)y * (long)FACE_TEX_W + (long)x) * 3L;
            g_face_tex[face][idx + 0] = (unsigned char)((br * (FACE_TEX_H - y) + 20 * y) / FACE_TEX_H);
            g_face_tex[face][idx + 1] = (unsigned char)((bg * (FACE_TEX_H - y) + 20 * y) / FACE_TEX_H);
            g_face_tex[face][idx + 2] = (unsigned char)((bb * (FACE_TEX_H - y) + 30 * y) / FACE_TEX_H);
        }
    }
}

static void clear_preview(void)
{
    long i;
    int f;
    for (i = 0; i < (long)PREVIEW_W * (long)PREVIEW_H * 3L; ++i) g_pixels[i] = 0;
    for (f = 0; f < GSKYBOX89_CUBE_FACE_COUNT; ++f) {
        g_face_present[f] = 0;
        fill_missing_face(f);
    }
}

static void sample_face_tex(int face, long u, long v, int *r, int *g, int *b)
{
    int tx;
    int ty;
    long idx;
    if (face < 0 || face >= GSKYBOX89_CUBE_FACE_COUNT) face = 0;
    if (u < 0L) u = 0L;
    if (v < 0L) v = 0L;
    if (u > GSKYBOX89_FX_ONE) u = GSKYBOX89_FX_ONE;
    if (v > GSKYBOX89_FX_ONE) v = GSKYBOX89_FX_ONE;
    tx = (int)((u * (long)(FACE_TEX_W - 1)) >> GSKYBOX89_FX_SHIFT);
    ty = (int)((v * (long)(FACE_TEX_H - 1)) >> GSKYBOX89_FX_SHIFT);
    idx = ((long)ty * (long)FACE_TEX_W + (long)tx) * 3L;
    *r = (int)g_face_tex[face][idx + 0];
    *g = (int)g_face_tex[face][idx + 1];
    *b = (int)g_face_tex[face][idx + 2];
}

static void fill_tri(const PreviewV *a, const PreviewV *b, const PreviewV *c, int draw_wire)
{
    int minx;
    int maxx;
    int miny;
    int maxy;
    int x;
    int y;
    long raw_area;
    long area;
    long w0;
    long w1;
    long w2;
    long rr;
    long gg;
    long bb;
    long aa;
    long uu;
    long vv;
    int sr;
    int sg;
    int sb;
    minx = a->sx;
    if (b->sx < minx) minx = b->sx;
    if (c->sx < minx) minx = c->sx;
    maxx = a->sx;
    if (b->sx > maxx) maxx = b->sx;
    if (c->sx > maxx) maxx = c->sx;
    miny = a->sy;
    if (b->sy < miny) miny = b->sy;
    if (c->sy < miny) miny = c->sy;
    maxy = a->sy;
    if (b->sy > maxy) maxy = b->sy;
    if (c->sy > maxy) maxy = c->sy;
    if (maxx < 0 || maxy < 0 || minx >= PREVIEW_W || miny >= PREVIEW_H) return;
    minx = clamp_i(minx, 0, PREVIEW_W - 1);
    maxx = clamp_i(maxx, 0, PREVIEW_W - 1);
    miny = clamp_i(miny, 0, PREVIEW_H - 1);
    maxy = clamp_i(maxy, 0, PREVIEW_H - 1);
    raw_area = edge_fn(a->sx, a->sy, b->sx, b->sy, c->sx, c->sy);
    if (raw_area == 0L) return;
    area = raw_area < 0L ? -raw_area : raw_area;
    for (y = miny; y <= maxy; ++y) {
        for (x = minx; x <= maxx; ++x) {
            w0 = edge_fn(b->sx, b->sy, c->sx, c->sy, x, y);
            w1 = edge_fn(c->sx, c->sy, a->sx, a->sy, x, y);
            w2 = edge_fn(a->sx, a->sy, b->sx, b->sy, x, y);
            if (raw_area < 0L) { w0 = -w0; w1 = -w1; w2 = -w2; }
            if (w0 >= 0L && w1 >= 0L && w2 >= 0L) {
                if (a->layer == GSKYBOX89_LAYER_CUBE6) {
                    uu = (a->u * w0 + b->u * w1 + c->u * w2) / area;
                    vv = (a->v * w0 + b->v * w1 + c->v * w2) / area;
                    sample_face_tex(a->face, uu, vv, &sr, &sg, &sb);
                    set_pixel_alpha(x, y, sr, sg, sb, 255);
                } else {
                    rr = ((long)a->r * w0 + (long)b->r * w1 + (long)c->r * w2) / area;
                    gg = ((long)a->g * w0 + (long)b->g * w1 + (long)c->g * w2) / area;
                    bb = ((long)a->b * w0 + (long)b->b * w1 + (long)c->b * w2) / area;
                    aa = ((long)a->a * w0 + (long)b->a * w1 + (long)c->a * w2) / area;
                    set_pixel_alpha(x, y, (int)rr, (int)gg, (int)bb, (int)aa);
                }
            }
        }
    }
    if (draw_wire != 0) {
        draw_line(a->sx, a->sy, b->sx, b->sy, 255, 255, 255, draw_wire);
        draw_line(b->sx, b->sy, c->sx, c->sy, 255, 255, 255, draw_wire);
        draw_line(c->sx, c->sy, a->sx, a->sy, 255, 255, 255, draw_wire);
    }
}

static PreviewV make_v_from_lib(const Gskybox89_Vertex *in_v, int pass)
{
    PreviewV out;
    out.layer = in_v->layer;
    out.face = in_v->face;
    out.u = in_v->u;
    out.v = in_v->v;
    if (in_v->layer == GSKYBOX89_LAYER_SCREEN) {
        out.x = in_v->x;
        out.y = in_v->y;
        out.z = GSKYBOX89_FX_ONE;
        out.sx = (int)(((in_v->x + GSKYBOX89_FX_ONE) * (long)PREVIEW_W) / (2L * GSKYBOX89_FX_ONE));
        out.sy = (int)(((GSKYBOX89_FX_ONE - in_v->y) * (long)PREVIEW_H) / (2L * GSKYBOX89_FX_ONE));
        out.r = (int)in_v->color.r;
        out.g = (int)in_v->color.g;
        out.b = (int)in_v->color.b;
        out.a = (int)in_v->color.a;
    } else {
        out.x = in_v->dir_x;
        out.y = in_v->dir_y;
        out.z = in_v->dir_z;
        out.sx = 0;
        out.sy = 0;
        out.r = (int)in_v->color.r;
        out.g = (int)in_v->color.g;
        out.b = (int)in_v->color.b;
        out.a = (int)in_v->color.a;
        if (pass == GSKYBOX89_PASS_DOME && out.a > 64) out.a = 64;
    }
    return out;
}

static PreviewV interp_v(const PreviewV *a, const PreviewV *b, long t)
{
    PreviewV out;
    long it;
    it = GSKYBOX89_FX_ONE - t;
    out.x = (a->x * it + b->x * t) >> GSKYBOX89_FX_SHIFT;
    out.y = (a->y * it + b->y * t) >> GSKYBOX89_FX_SHIFT;
    out.z = (a->z * it + b->z * t) >> GSKYBOX89_FX_SHIFT;
    out.u = (a->u * it + b->u * t) >> GSKYBOX89_FX_SHIFT;
    out.v = (a->v * it + b->v * t) >> GSKYBOX89_FX_SHIFT;
    out.sx = 0;
    out.sy = 0;
    out.r = (int)(((long)a->r * it + (long)b->r * t) >> GSKYBOX89_FX_SHIFT);
    out.g = (int)(((long)a->g * it + (long)b->g * t) >> GSKYBOX89_FX_SHIFT);
    out.b = (int)(((long)a->b * it + (long)b->b * t) >> GSKYBOX89_FX_SHIFT);
    out.a = (int)(((long)a->a * it + (long)b->a * t) >> GSKYBOX89_FX_SHIFT);
    out.layer = a->layer;
    out.face = a->face;
    return out;
}

static int clip_near(const PreviewV *in_v, int in_n, PreviewV *out_v)
{
    int i;
    int out_n;
    PreviewV s;
    PreviewV e;
    int s_in;
    int e_in;
    long den;
    long t;
    out_n = 0;
    if (in_n <= 0) return 0;
    s = in_v[in_n - 1];
    s_in = s.z >= PREVIEW_NEAR;
    for (i = 0; i < in_n; ++i) {
        e = in_v[i];
        e_in = e.z >= PREVIEW_NEAR;
        if (s_in != 0 && e_in != 0) {
            if (out_n < PREVIEW_MAX_CLIP) out_v[out_n++] = e;
        } else if (s_in != 0 && e_in == 0) {
            den = e.z - s.z;
            if (den != 0L && out_n < PREVIEW_MAX_CLIP) {
                t = ((PREVIEW_NEAR - s.z) << GSKYBOX89_FX_SHIFT) / den;
                out_v[out_n++] = interp_v(&s, &e, t);
            }
        } else if (s_in == 0 && e_in != 0) {
            den = e.z - s.z;
            if (den != 0L && out_n < PREVIEW_MAX_CLIP) {
                t = ((PREVIEW_NEAR - s.z) << GSKYBOX89_FX_SHIFT) / den;
                out_v[out_n++] = interp_v(&s, &e, t);
            }
            if (out_n < PREVIEW_MAX_CLIP) out_v[out_n++] = e;
        }
        s = e;
        s_in = e_in;
    }
    return out_n;
}

static void project_v(PreviewV *v)
{
    long z;
    z = v->z;
    if (z < PREVIEW_NEAR) z = PREVIEW_NEAR;
    v->sx = (PREVIEW_W / 2) + (int)((v->x * PREVIEW_FOCAL) / z);
    v->sy = (PREVIEW_H / 2) - (int)((v->y * PREVIEW_FOCAL) / z);
}

static void preview_begin(void *user, int pass)
{
    PreviewState *s;
    s = (PreviewState *)user;
    s->pass = pass;
}

static void preview_tri(void *user, const Gskybox89_Vertex *a, const Gskybox89_Vertex *b, const Gskybox89_Vertex *c)
{
    PreviewState *s;
    PreviewV in_v[3];
    PreviewV clipped[PREVIEW_MAX_CLIP];
    PreviewV ta;
    PreviewV tb;
    PreviewV tc;
    int n;
    int i;
    int wire;
    s = (PreviewState *)user;
    s->tri_total += 1;
    if (s->pass == GSKYBOX89_PASS_SCREEN) s->tri_screen += 1;
    if (s->pass == GSKYBOX89_PASS_CUBE6) s->tri_cube += 1;
    if (s->pass == GSKYBOX89_PASS_DOME) s->tri_dome += 1;
    in_v[0] = make_v_from_lib(a, s->pass);
    in_v[1] = make_v_from_lib(b, s->pass);
    in_v[2] = make_v_from_lib(c, s->pass);
    if (s->pass == GSKYBOX89_PASS_SCREEN) {
        fill_tri(&in_v[0], &in_v[1], &in_v[2], 0);
        return;
    }
    n = clip_near(in_v, 3, clipped);
    if (n < 3) return;
    for (i = 0; i < n; ++i) project_v(&clipped[i]);
    wire = 0;
    if (s->pass == GSKYBOX89_PASS_CUBE6) wire = 20;
    if (s->pass == GSKYBOX89_PASS_DOME) wire = 10;
    for (i = 1; i < n - 1; ++i) {
        ta = clipped[0];
        tb = clipped[i];
        tc = clipped[i + 1];
        fill_tri(&ta, &tb, &tc, wire);
    }
}

static int write_ppm(const char *path)
{
    FILE *f;
    size_t written;
    f = fopen(path, "wb");
    if (f == 0) return 0;
    fprintf(f, "P6\n%d %d\n255\n", PREVIEW_W, PREVIEW_H);
    written = fwrite(g_pixels, 1, sizeof(g_pixels), f);
    fclose(f);
    return written == sizeof(g_pixels);
}

static int load_assets_from_args(int argc, char **argv, int start)
{
    Gskybox89_AssetSet set;
    int i;
    int face;
    int conv;
    int loaded;
    int rc;
    gskybox89_asset_set_clear(&set);
    loaded = 0;
    for (i = start; i < argc; ++i) {
        face = GSKYBOX89_FACE_NONE;
        conv = GSKYBOX89_ASSET_CONV_UNKNOWN;
        rc = gskybox89_asset_face_from_name(argv[i], &face, &conv);
        if (rc == GSKYBOX89_ASSET_OK) {
            gskybox89_asset_add_path(&set, argv[i], GSKYBOX89_ASSET_FLAG_DEFAULT);
            if (load_face_image(argv[i], face) != 0) {
                ++loaded;
                printf("loaded %-2s %s (%s)\n", gskybox89_face_name(face), argv[i], gskybox89_asset_convention_name(conv));
            } else {
                printf("mapped %-2s but did not decode image: %s\n", gskybox89_face_name(face), argv[i]);
            }
        }
    }
    printf("asset map mask=0x%02X mapped=%d loaded_images=%d\n", gskybox89_asset_complete_mask(&set), set.present_count, loaded);
    return loaded;
}

int main(int argc, char **argv)
{
    Gskybox89_Context ctx;
    Gskybox89_Config cfg;
    Gskybox89_Backend be;
    Gskybox89_Mat3 rot;
    PreviewState st;
    const char *out_path;
    int rc;
    int loaded;
    if (argc < 3) {
        printf("usage: demo_asset_preview_ppm <out.ppm> <sky face images...>\n");
        printf("decoders: uncompressed BMP 24/32, uncompressed TGA 24/32, PPM P6. PNG/JPG are mapper-only unless converted by a tool first.\n");
        return 1;
    }
    out_path = argv[1];
    clear_preview();
    loaded = load_assets_from_args(argc, argv, 2);
    st.pass = 0;
    st.tri_total = 0;
    st.tri_screen = 0;
    st.tri_cube = 0;
    st.tri_dome = 0;
    st.image_faces_loaded = loaded;

    gskybox89_default_config(&cfg);
    cfg.layer_mask = GSKYBOX89_LAYER_SCREEN | GSKYBOX89_LAYER_CUBE6 | GSKYBOX89_LAYER_DOME;
    cfg.cube_fast_shared_vertices = 1;
    cfg.dome_segments = 16;
    cfg.dome_rings = 5;
    cfg.screen_top.r = 6; cfg.screen_top.g = 10; cfg.screen_top.b = 26; cfg.screen_top.a = 255;
    cfg.screen_bottom.r = 26; cfg.screen_bottom.g = 35; cfg.screen_bottom.b = 60; cfg.screen_bottom.a = 255;
    cfg.dome_top.r = 255; cfg.dome_top.g = 255; cfg.dome_top.b = 255; cfg.dome_top.a = 18;
    cfg.dome_horizon.r = 255; cfg.dome_horizon.g = 245; cfg.dome_horizon.b = 210; cfg.dome_horizon.a = 22;

    be.user = &st;
    be.begin_pass = preview_begin;
    be.end_pass = 0;
    be.set_state = 0;
    be.bind_face = 0;
    be.emit_tri = preview_tri;

    gskybox89_mat3_yaw16(&rot, 1);
    gskybox89_init(&ctx, &cfg, &be);
    rc = gskybox89_render(&ctx, &rot);
    if (rc != GSKYBOX89_OK) return 2;
    if (!write_ppm(out_path)) return 3;
    printf("asset preview written: %s\n", out_path);
    printf("triangles total=%d screen=%d cube=%d dome=%d loaded_faces=%d\n", st.tri_total, st.tri_screen, st.tri_cube, st.tri_dome, st.image_faces_loaded);
    return 0;
}
