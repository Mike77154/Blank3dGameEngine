#include "glyphwifettf.h"
#include "common_io.h"

#define MAX_POINTS 4096
#define MAX_CONTOURS 512
#define ATLAS_W 512
#define ATLAS_H 512
#define ATLAS_GLYPHS 256
#define LAYOUT_MAX 512
#define OUT_W 640
#define OUT_H 160

static GWT_Point points[MAX_POINTS];
static GWT_Contour contours[MAX_CONTOURS];
static unsigned char atlas_pixels[ATLAS_W * ATLAS_H];
static GWT_AtlasGlyph atlas_glyphs[ATLAS_GLYPHS];
static GWT_LayoutGlyph layout[LAYOUT_MAX];
static unsigned char out_pixels[OUT_W * OUT_H];

static void write_pgm(const char *path, const unsigned char *pix, int w, int h){
    FILE *fp;
    fp = fopen(path, "wb");
    if(!fp) return;
    fprintf(fp, "P5\n%d %d\n255\n", w, h);
    fwrite(pix, 1, (size_t)(w*h), fp);
    fclose(fp);
}

int main(int argc, char **argv){
    GWT_Font font;
    GWT_Outline scratch;
    GWT_Atlas atlas;
    GWT_Bitmap out;
    unsigned long sz;
    int r;
    const char *text;
    if(argc < 2){ printf("usage: %s font.ttf [text]\n", argv[0]); return 1; }
    text = (argc >= 3) ? argv[2] : "GlyphWifeTTF v0.4.0 - text bitmap";
    sz = read_file_static(argv[1], g_font_buf, FONT_BUF_MAX);
    if(!sz){ printf("could not read font\n"); return 1; }
    r = gwt_font_init(&font, g_font_buf, sz);
    if(r != GWT_OK){ printf("font init: %s\n", gwt_error_string(r)); return 1; }
    scratch.points = points; scratch.max_points = MAX_POINTS; scratch.point_count = 0;
    scratch.contours = contours; scratch.max_contours = MAX_CONTOURS; scratch.contour_count = 0;
    gwt_atlas_init(&atlas, atlas_pixels, ATLAS_W, ATLAS_H, ATLAS_W, atlas_glyphs, ATLAS_GLYPHS);
    r = gwt_atlas_add_ascii(&font, &atlas, 24, 2, &scratch);
    if(r != GWT_OK){ printf("atlas ascii: %s\n", gwt_error_string(r)); return 1; }
    r = gwt_atlas_add_utf8(&font, &atlas, text, 24, 2, &scratch);
    if(r != GWT_OK){ printf("atlas utf8: %s\n", gwt_error_string(r)); return 1; }
    out.pixels = out_pixels; out.width = OUT_W; out.height = OUT_H; out.stride = OUT_W;
    gwt_bitmap_clear(&out, 0);
    r = gwt_draw_text_bitmap_utf8(&font, &atlas, &out, text, 24, 16, 64, 255, layout, LAYOUT_MAX);
    if(r != GWT_OK){ printf("draw: %s\n", gwt_error_string(r)); return 1; }
    write_pgm("text_bitmap.pgm", out_pixels, OUT_W, OUT_H);
    write_pgm("text_atlas.pgm", atlas_pixels, ATLAS_W, ATLAS_H);
    printf("wrote text_bitmap.pgm and text_atlas.pgm, glyphs=%u\n", atlas.glyph_count);
    return 0;
}
