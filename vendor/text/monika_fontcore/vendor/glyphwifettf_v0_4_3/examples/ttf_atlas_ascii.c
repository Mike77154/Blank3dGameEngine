#include "glyphwifettf.h"
#include "common_io.h"
#include <stdio.h>

#define ATLAS_W 512
#define ATLAS_H 512
#define MAX_GLYPHS 128
#define MAX_POINTS 4096
#define MAX_CONTOURS 512

static unsigned char atlas_pixels[ATLAS_W * ATLAS_H];
static GWT_AtlasGlyph atlas_glyphs[MAX_GLYPHS];
static GWT_Point points[MAX_POINTS];
static GWT_Contour contours[MAX_CONTOURS];

int main(int argc, char **argv){
    GWT_Font font;
    GWT_Atlas atlas;
    GWT_Outline outline;
    unsigned long size;
    FILE *fp;
    int r;
    char name[128];

    if(argc < 3){
        printf("usage: %s font.ttf out_ascii_atlas.pgm\n", argv[0]);
        return 1;
    }

    size = read_file_static(argv[1], g_font_buf, FONT_BUF_MAX);
    if(size == 0){ printf("could not read font\n"); return 1; }

    r = gwt_font_init(&font, g_font_buf, size);
    if(r != GWT_OK){ printf("font init: %s\n", gwt_error_string(r)); return 1; }

    if(gwt_get_name_ascii(&font, GWT_NAME_FULL, name, sizeof(name)) == GWT_OK){
        printf("font: %s\n", name);
    }

    outline.points = points;
    outline.max_points = MAX_POINTS;
    outline.contours = contours;
    outline.max_contours = MAX_CONTOURS;
    gwt_outline_reset(&outline);

    gwt_atlas_init(&atlas, atlas_pixels, ATLAS_W, ATLAS_H, ATLAS_W, atlas_glyphs, MAX_GLYPHS);
    r = gwt_atlas_add_ascii(&font, &atlas, 32, 1, &outline);
    if(r != GWT_OK){ printf("atlas ascii: %s\n", gwt_error_string(r)); return 1; }

    fp = fopen(argv[2], "wb");
    if(!fp){ printf("could not write output\n"); return 1; }
    fprintf(fp, "P5\n%d %d\n255\n", ATLAS_W, ATLAS_H);
    fwrite(atlas_pixels, 1, ATLAS_W * ATLAS_H, fp);
    fclose(fp);

    printf("glyphs: %u\n", (unsigned)atlas.glyph_count);
    printf("wrote: %s\n", argv[2]);
    return 0;
}
