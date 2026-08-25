#include "gf_gff.h"
#include <stdio.h>
#include <string.h>

#define GF_MAGIC "GFF0"

int gf_gff_write(const char *path, const GF_Font *font) {
    FILE *fp; unsigned short i;
    fp = fopen(path, "wb"); if (!fp) return -1;
    fwrite(GF_MAGIC, 1, 4, fp);
    fwrite(&font->glyph_count, sizeof(font->glyph_count), 1, fp);
    fwrite(&font->units_per_em, sizeof(font->units_per_em), 1, fp);
    fwrite(&font->ascender, sizeof(font->ascender), 1, fp);
    fwrite(&font->descender, sizeof(font->descender), 1, fp);
    for (i = 0; i < font->glyph_count; ++i) {
        const GF_Glyph *g = &font->glyphs[i];
        fwrite(&g->codepoint, sizeof(g->codepoint), 1, fp);
        fwrite(&g->advance_x, sizeof(g->advance_x), 1, fp);
        fwrite(&g->point_count, sizeof(g->point_count), 1, fp);
        fwrite(&g->contour_count, sizeof(g->contour_count), 1, fp);
        fwrite(g->points, sizeof(GF_Point), g->point_count, fp);
        fwrite(g->contours, sizeof(GF_Contour), g->contour_count, fp);
    }
    fclose(fp); return 0;
}
int gf_gff_read(const char *path, GF_Font *font) {
    FILE *fp; char magic[4]; unsigned short i, gc;
    fp = fopen(path, "rb"); if (!fp) return -1;
    if (fread(magic, 1, 4, fp) != 4 || memcmp(magic, GF_MAGIC, 4) != 0) { fclose(fp); return -2; }
    gf_font_clear(font);
    fread(&gc, sizeof(gc), 1, fp); if (gc > GF_MAX_GLYPHS) { fclose(fp); return -3; }
    font->glyph_count = gc;
    fread(&font->units_per_em, sizeof(font->units_per_em), 1, fp);
    fread(&font->ascender, sizeof(font->ascender), 1, fp);
    fread(&font->descender, sizeof(font->descender), 1, fp);
    for (i = 0; i < gc; ++i) {
        GF_Glyph *g = &font->glyphs[i]; gf_glyph_clear(g);
        fread(&g->codepoint, sizeof(g->codepoint), 1, fp);
        fread(&g->advance_x, sizeof(g->advance_x), 1, fp);
        fread(&g->point_count, sizeof(g->point_count), 1, fp);
        fread(&g->contour_count, sizeof(g->contour_count), 1, fp);
        if (g->point_count > GF_MAX_POINTS || g->contour_count > GF_MAX_CONTOURS) { fclose(fp); return -4; }
        fread(g->points, sizeof(GF_Point), g->point_count, fp);
        fread(g->contours, sizeof(GF_Contour), g->contour_count, fp);
        gf_glyph_bbox(g);
    }
    fclose(fp); return 0;
}
