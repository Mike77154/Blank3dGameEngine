#include "monika_fontcore.h"
#include <stdio.h>

#define FONT_BUF_MAX   (1024ul * 1024ul)
#define SFNT_BUF_MAX   (2ul * 1024ul * 1024ul)
#define WORK_BUF_MAX   (2ul * 1024ul * 1024ul)
#define ATLAS_W 512
#define ATLAS_H 512
#define ATLAS_MAX_GLYPHS 64
#define MAX_POINTS 4096
#define MAX_CONTOURS 512
#define LAYOUT_MAX 128
#define OUT_W 640
#define OUT_H 260
#define IDAT_MAX 190000ul

static unsigned char g_font_buf[FONT_BUF_MAX];
static unsigned char g_sfnt_buf[SFNT_BUF_MAX];
static unsigned char g_work_buf[WORK_BUF_MAX];
static unsigned char g_atlas_pixels[ATLAS_W * ATLAS_H];
static GWT_AtlasGlyph g_atlas_glyphs[ATLAS_MAX_GLYPHS];
static GWT_Point g_points[MAX_POINTS];
static GWT_Contour g_contours[MAX_CONTOURS];
static GWT_LayoutGlyph g_layout[LAYOUT_MAX];
static unsigned char g_out[OUT_W * OUT_H];
static unsigned char g_idat[IDAT_MAX];

static unsigned long read_file_static(const char *path, unsigned char *dst, unsigned long cap)
{
    FILE *fp;
    unsigned long n;
    if (!path || !dst || cap == 0ul) return 0ul;
    fp = fopen(path, "rb");
    if (!fp) return 0ul;
    n = (unsigned long)fread(dst, 1u, (size_t)cap, fp);
    fclose(fp);
    return n;
}

static void clear_img(unsigned char v)
{
    unsigned long i;
    for (i = 0ul; i < (unsigned long)(OUT_W * OUT_H); ++i) g_out[i] = v;
}

static void put_px(int x, int y, unsigned char v)
{
    if (x < 0 || y < 0 || x >= OUT_W || y >= OUT_H) return;
    g_out[y * OUT_W + x] = v;
}

static void fill_rect(int x, int y, int w, int h, unsigned char v)
{
    int xx, yy;
    for (yy = y; yy < y + h; ++yy) for (xx = x; xx < x + w; ++xx) put_px(xx, yy, v);
}

static const unsigned char *glyph5(char c)
{
    static const unsigned char sp[7] = {0,0,0,0,0,0,0};
    static const unsigned char A[7] = {14,17,17,31,17,17,17};
    static const unsigned char B[7] = {30,17,17,30,17,17,30};
    static const unsigned char C[7] = {14,17,16,16,16,17,14};
    static const unsigned char D[7] = {30,17,17,17,17,17,30};
    static const unsigned char E[7] = {31,16,16,30,16,16,31};
    static const unsigned char F[7] = {31,16,16,30,16,16,16};
    static const unsigned char G[7] = {14,17,16,23,17,17,14};
    static const unsigned char H[7] = {17,17,17,31,17,17,17};
    static const unsigned char I[7] = {14,4,4,4,4,4,14};
    static const unsigned char J[7] = {7,2,2,2,18,18,12};
    static const unsigned char K[7] = {17,18,20,24,20,18,17};
    static const unsigned char L[7] = {16,16,16,16,16,16,31};
    static const unsigned char M[7] = {17,27,21,21,17,17,17};
    static const unsigned char N[7] = {17,25,21,19,17,17,17};
    static const unsigned char O[7] = {14,17,17,17,17,17,14};
    static const unsigned char P[7] = {30,17,17,30,16,16,16};
    static const unsigned char Q[7] = {14,17,17,17,21,18,13};
    static const unsigned char R[7] = {30,17,17,30,20,18,17};
    static const unsigned char S[7] = {15,16,16,14,1,1,30};
    static const unsigned char T[7] = {31,4,4,4,4,4,4};
    static const unsigned char U[7] = {17,17,17,17,17,17,14};
    static const unsigned char V[7] = {17,17,17,17,17,10,4};
    static const unsigned char W[7] = {17,17,17,21,21,21,10};
    static const unsigned char X[7] = {17,17,10,4,10,17,17};
    static const unsigned char Y[7] = {17,17,10,4,4,4,4};
    static const unsigned char Z[7] = {31,1,2,4,8,16,31};
    static const unsigned char N0[7] = {14,17,19,21,25,17,14};
    static const unsigned char N1[7] = {4,12,4,4,4,4,14};
    static const unsigned char N2[7] = {14,17,1,2,4,8,31};
    static const unsigned char N3[7] = {30,1,1,14,1,1,30};
    static const unsigned char N4[7] = {2,6,10,18,31,2,2};
    static const unsigned char N5[7] = {31,16,30,1,1,17,14};
    static const unsigned char N6[7] = {6,8,16,30,17,17,14};
    static const unsigned char N7[7] = {31,1,2,4,8,8,8};
    static const unsigned char N8[7] = {14,17,17,14,17,17,14};
    static const unsigned char N9[7] = {14,17,17,15,1,2,12};
    static const unsigned char DASH[7] = {0,0,0,31,0,0,0};
    static const unsigned char COLON[7] = {0,4,4,0,4,4,0};
    static const unsigned char DOT[7] = {0,0,0,0,0,12,12};
    if (c >= 'a' && c <= 'z') c = (char)(c - 'a' + 'A');
    switch(c) {
    case 'A': return A; case 'B': return B; case 'C': return C; case 'D': return D;
    case 'E': return E; case 'F': return F; case 'G': return G; case 'H': return H;
    case 'I': return I; case 'J': return J; case 'K': return K; case 'L': return L;
    case 'M': return M; case 'N': return N; case 'O': return O; case 'P': return P;
    case 'Q': return Q; case 'R': return R; case 'S': return S; case 'T': return T;
    case 'U': return U; case 'V': return V; case 'W': return W; case 'X': return X;
    case 'Y': return Y; case 'Z': return Z;
    case '0': return N0; case '1': return N1; case '2': return N2; case '3': return N3; case '4': return N4;
    case '5': return N5; case '6': return N6; case '7': return N7; case '8': return N8; case '9': return N9;
    case '-': return DASH; case ':': return COLON; case '.': return DOT;
    default: return sp;
    }
}

static void draw_char5(int x, int y, char c, unsigned char v)
{
    const unsigned char *g;
    int row, col;
    g = glyph5(c);
    for (row = 0; row < 7; ++row) {
        for (col = 0; col < 5; ++col) {
            if ((g[row] & (unsigned char)(1u << (4 - col))) != 0u) put_px(x + col, y + row, v);
        }
    }
}

static void draw_text5(int x, int y, const char *s, unsigned char v)
{
    int cx;
    cx = x;
    while (*s) {
        draw_char5(cx, y, *s, v);
        cx += 6;
        ++s;
    }
}

static unsigned long crc32_update(unsigned long crc, const unsigned char *data, unsigned long len)
{
    unsigned long i, j;
    unsigned long c;
    c = crc ^ 0xfffffffful;
    for (i = 0ul; i < len; ++i) {
        c ^= (unsigned long)data[i];
        for (j = 0ul; j < 8ul; ++j) {
            if ((c & 1ul) != 0ul) c = 0xedb88320ul ^ (c >> 1);
            else c >>= 1;
        }
    }
    return c ^ 0xfffffffful;
}

static unsigned long adler32_calc(const unsigned char *data, unsigned long len)
{
    unsigned long a, b, i;
    a = 1ul;
    b = 0ul;
    for (i = 0ul; i < len; ++i) {
        a += data[i];
        if (a >= 65521ul) a -= 65521ul;
        b += a;
        b %= 65521ul;
    }
    return (b << 16) | a;
}

static void put_u32be(FILE *fp, unsigned long v)
{
    fputc((int)((v >> 24) & 255ul), fp);
    fputc((int)((v >> 16) & 255ul), fp);
    fputc((int)((v >> 8) & 255ul), fp);
    fputc((int)(v & 255ul), fp);
}

static void write_chunk(FILE *fp, const char type[4], const unsigned char *data, unsigned long len)
{
    unsigned long crc;
    put_u32be(fp, len);
    fwrite(type, 1u, 4u, fp);
    if (len > 0ul) fwrite(data, 1u, (size_t)len, fp);
    crc = crc32_update(0ul, (const unsigned char*)type, 4ul);
    if (len > 0ul) crc = crc32_update(crc, data, len);
    put_u32be(fp, crc);
}

static int write_png_gray8(const char *path, const unsigned char *pix, int w, int h)
{
    FILE *fp;
    unsigned char ihdr[13];
    unsigned long raw_len;
    unsigned long pos;
    unsigned long y, x;
    unsigned long idp;
    unsigned long remaining;
    unsigned long block;
    unsigned long start;
    unsigned long ad;
    unsigned int nlen;
    static const unsigned char sig[8] = {137,80,78,71,13,10,26,10};
    static const unsigned char iend_dummy[1] = {0};
    (void)iend_dummy;
    if (!path || !pix || w <= 0 || h <= 0) return 0;
    raw_len = (unsigned long)(w + 1) * (unsigned long)h;
    if (raw_len + 64ul + (raw_len / 65535ul + 1ul) * 5ul > IDAT_MAX) return 0;
    idp = 0ul;
    g_idat[idp++] = 0x78u;
    g_idat[idp++] = 0x01u;
    start = idp;
    remaining = raw_len;
    pos = 0ul;
    while (remaining > 0ul) {
        block = remaining;
        if (block > 65535ul) block = 65535ul;
        g_idat[idp++] = (remaining <= 65535ul) ? 1u : 0u;
        g_idat[idp++] = (unsigned char)(block & 255ul);
        g_idat[idp++] = (unsigned char)((block >> 8) & 255ul);
        nlen = (unsigned int)(~block) & 0xffffu;
        g_idat[idp++] = (unsigned char)(nlen & 255u);
        g_idat[idp++] = (unsigned char)((nlen >> 8) & 255u);
        while (block > 0ul) {
            y = pos / (unsigned long)(w + 1);
            x = pos % (unsigned long)(w + 1);
            if (x == 0ul) g_idat[idp++] = 0u;
            else g_idat[idp++] = pix[y * (unsigned long)w + (x - 1ul)];
            ++pos;
            --block;
            --remaining;
        }
    }
    ad = adler32_calc(g_idat + start + 5ul, 0ul); /* overwritten below */
    (void)ad;
    /* Compute Adler from raw stream by walking the encoded copy. */
    {
        unsigned long a, b, rp, raw_seen;
        a = 1ul; b = 0ul; rp = start; raw_seen = 0ul;
        while (raw_seen < raw_len) {
            unsigned long len;
            rp += 1ul;
            len = (unsigned long)g_idat[rp] | ((unsigned long)g_idat[rp + 1ul] << 8);
            rp += 4ul;
            while (len > 0ul) {
                a += g_idat[rp++];
                if (a >= 65521ul) a -= 65521ul;
                b += a;
                b %= 65521ul;
                --len;
                ++raw_seen;
            }
        }
        ad = (b << 16) | a;
    }
    g_idat[idp++] = (unsigned char)((ad >> 24) & 255ul);
    g_idat[idp++] = (unsigned char)((ad >> 16) & 255ul);
    g_idat[idp++] = (unsigned char)((ad >> 8) & 255ul);
    g_idat[idp++] = (unsigned char)(ad & 255ul);

    fp = fopen(path, "wb");
    if (!fp) return 0;
    fwrite(sig, 1u, 8u, fp);
    ihdr[0] = (unsigned char)((w >> 24) & 255); ihdr[1] = (unsigned char)((w >> 16) & 255); ihdr[2] = (unsigned char)((w >> 8) & 255); ihdr[3] = (unsigned char)(w & 255);
    ihdr[4] = (unsigned char)((h >> 24) & 255); ihdr[5] = (unsigned char)((h >> 16) & 255); ihdr[6] = (unsigned char)((h >> 8) & 255); ihdr[7] = (unsigned char)(h & 255);
    ihdr[8] = 8u; ihdr[9] = 0u; ihdr[10] = 0u; ihdr[11] = 0u; ihdr[12] = 0u;
    write_chunk(fp, "IHDR", ihdr, 13ul);
    write_chunk(fp, "IDAT", g_idat, idp);
    write_chunk(fp, "IEND", 0, 0ul);
    fclose(fp);
    return 1;
}

static int check_open_file(const char *path, int kind_hint)
{
    MFC_Font fc;
    MFC_Scratch scratch;
    unsigned long n;
    int r;
    n = read_file_static(path, g_font_buf, FONT_BUF_MAX);
    if (n == 0ul) return 0;
    mfc_scratch_init(&scratch, g_sfnt_buf, SFNT_BUF_MAX, g_work_buf, WORK_BUF_MAX);
    r = mfc_open_memory(&fc, g_font_buf, (mfc_u32)n, kind_hint, &scratch);
    return r == MFC_OK ? 1 : 0;
}

static int gmyy_demo_ok(void)
{
    const char *yy =
        "{\n"
        "  \"resourceType\": \"GMSprite\",\n"
        "  \"%Name\": \"spr_digits\",\n"
        "  \"width\": 8, \"height\": 12,\n"
        "  \"frames\": [\n"
        "    {\"$GMSpriteFrame\":\"\", \"%Name\":\"f0\"},\n"
        "    {\"$GMSpriteFrame\":\"\", \"%Name\":\"f1\"},\n"
        "    {\"$GMSpriteFrame\":\"\", \"%Name\":\"f2\"},\n"
        "    {\"$GMSpriteFrame\":\"\", \"%Name\":\"f3\"}\n"
        "  ],\n"
        "  \"layers\": [{\"$GMImageLayer\":\"\", \"%Name\":\"layer0\"}],\n"
        "  \"sequence\": {\"xorigin\":0, \"yorigin\":0}\n"
        "}\n";
    const char *gml = "global.fnt_digits = font_add_sprite_ext(spr_digits, \"0123\", true, 1);";
    MFC_Font f;
    int w, h;
    if (mfc_open_gmyy_spritefont(&f, yy, gml) != MFC_OK) return 0;
    if (mfc_measure_utf8(&f, "0123", 12, &w, &h) != MFC_OK) return 0;
    return (w > 0 && h > 0) ? 1 : 0;
}

static int mugen_demo_ok(void)
{
    MFC_Font f;
    unsigned long n;
    n = read_file_static("vendor/mugen_fnt_codec/tmp_test/font.txt", g_font_buf, FONT_BUF_MAX);
    if (n == 0ul) return 0;
    return mfc_open_mugen_text(&f, (const char*)g_font_buf, (mfc_u32)n) == MFC_OK ? 1 : 0;
}

static void status_row(int y, const char *label, int ok)
{
    fill_rect(24, y, 14, 10, ok ? 220u : 70u);
    fill_rect(26, y + 2, 10, 6, ok ? 255u : 110u);
    draw_text5(46, y + 1, label, ok ? 250u : 130u);
    draw_text5(160, y + 1, ok ? "OK" : "FAIL", ok ? 250u : 130u);
}

int main(int argc, char **argv)
{
    const char *font_path;
    const char *png_path;
    MFC_Font font;
    MFC_Scratch scratch;
    GWT_Outline outline;
    GWT_Atlas atlas;
    GWT_Bitmap dst;
    unsigned long n;
    int r;
    int ttf_ok, woff2_ok, woff1_ok, gmyy_ok, mugen_ok;
    int tw, th;

    font_path = argc > 1 ? argv[1] : "vendor/monika_woff2_c89_decoder/tests/tiny.ttf";
    png_path = argc > 2 ? argv[2] : "proof_fontcore.png";

    clear_img(18u);
    fill_rect(0, 0, OUT_W, OUT_H, 18u);
    fill_rect(12, 12, OUT_W - 24, 40, 34u);
    draw_text5(24, 26, "MONIKA FONTCORE GRAND DECODER", 245u);
    draw_text5(24, 42, "C89 NO MALLOC NO FREE NO FLOAT", 190u);

    n = read_file_static(font_path, g_font_buf, FONT_BUF_MAX);
    if (n == 0ul) {
        draw_text5(24, 80, "COULD NOT READ TTF", 240u);
        write_png_gray8(png_path, g_out, OUT_W, OUT_H);
        return 1;
    }

    mfc_scratch_init(&scratch, g_sfnt_buf, SFNT_BUF_MAX, g_work_buf, WORK_BUF_MAX);
    r = mfc_open_memory(&font, g_font_buf, (mfc_u32)n, MFC_KIND_UNKNOWN, &scratch);
    ttf_ok = (r == MFC_OK && (font.features & MFC_FEATURE_TTF_GLYF) != 0ul) ? 1 : 0;

    woff2_ok = check_open_file("vendor/monika_woff2_c89_decoder/tests/tiny.woff2", MFC_KIND_WOFF2);
    woff1_ok = check_open_file("vendor/monika_woff1_decoder_c89/tests/sample_compressed.woff", MFC_KIND_WOFF1);
    gmyy_ok = gmyy_demo_ok();
    mugen_ok = mugen_demo_ok();

    status_row(68, "TTF SFNT", ttf_ok);
    status_row(84, "WOFF2 TO SFNT", woff2_ok);
    status_row(100, "WOFF1 TO SFNT", woff1_ok);
    status_row(116, "GMYY SPRITEFONT", gmyy_ok);
    status_row(132, "MUGEN TEXT FNT", mugen_ok);

    if (ttf_ok) {
        outline.points = g_points;
        outline.max_points = MAX_POINTS;
        outline.contours = g_contours;
        outline.max_contours = MAX_CONTOURS;
        gwt_outline_reset(&outline);
        gwt_atlas_init(&atlas, g_atlas_pixels, ATLAS_W, ATLAS_H, ATLAS_W, g_atlas_glyphs, ATLAS_MAX_GLYPHS);
        r = gwt_atlas_add_utf8(&font.ttf, &atlas, "AAAAA", 72, 2u, &outline);
        if (r == GWT_OK) {
            dst.pixels = g_out;
            dst.width = OUT_W;
            dst.height = OUT_H;
            dst.stride = OUT_W;
            mfc_ttf_draw_text_bitmap_utf8(&font, &atlas, &dst, "AAAAA", 72, 240, 180, 255u, g_layout, LAYOUT_MAX);
            draw_text5(240, 164, "GLYPH A FROM TINY TTF", 210u);
            if (mfc_measure_utf8(&font, "AAAAA", 72, &tw, &th) == MFC_OK) {
                (void)tw;
                (void)th;
                draw_text5(240, 150, "MEASURE UTF8 OK", 200u);
            }
        }
    }

    if (!write_png_gray8(png_path, g_out, OUT_W, OUT_H)) return 1;

    printf("Monika FontCore proof\n");
    printf("font: %s\n", font_path);
    printf("kind: %s\n", mfc_kind_name(font.effective_kind));
    printf("features: 0x%08lx\n", font.features);
    printf("checks: ttf=%d woff2=%d woff1=%d gmyy=%d mugen=%d\n", ttf_ok, woff2_ok, woff1_ok, gmyy_ok, mugen_ok);
    printf("png: %s\n", png_path);
    return 0;
}
