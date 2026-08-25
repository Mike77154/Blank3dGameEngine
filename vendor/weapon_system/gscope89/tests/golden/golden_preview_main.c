#include <stdio.h>
#include <string.h>
#include "gscopepresets89.h"

int gpr89_render_preset(const gsvp89_preset *preset, const char *filename);

static FILE *g_cmd_file;

static void put_u16le(FILE *f, unsigned short v)
{
    fputc((int)(v & 255u), f);
    fputc((int)((v >> 8) & 255u), f);
}

static void put_s16le(FILE *f, short v)
{
    put_u16le(f, (unsigned short)v);
}

static void golden_cmd_emit(void *user, const gsp89_draw_cmd *c)
{
    (void)user;
    if (!g_cmd_file || !c) return;
    put_s16le(g_cmd_file, c->kind);
    put_s16le(g_cmd_file, c->layer);
    put_s16le(g_cmd_file, c->part_id);
    put_s16le(g_cmd_file, c->blend_mode);
    put_s16le(g_cmd_file, c->flags);
    put_s16le(g_cmd_file, c->x0); put_s16le(g_cmd_file, c->y0);
    put_s16le(g_cmd_file, c->x1); put_s16le(g_cmd_file, c->y1);
    put_s16le(g_cmd_file, c->x2); put_s16le(g_cmd_file, c->y2);
    put_s16le(g_cmd_file, c->x3); put_s16le(g_cmd_file, c->y3);
    put_s16le(g_cmd_file, c->radius_x); put_s16le(g_cmd_file, c->radius_y);
    put_s16le(g_cmd_file, c->start_deg_x100); put_s16le(g_cmd_file, c->end_deg_x100);
    put_s16le(g_cmd_file, c->thickness_px); put_s16le(g_cmd_file, c->outline_px);
    put_s16le(g_cmd_file, c->asset_id); put_s16le(g_cmd_file, c->glyph_id);
    put_s16le(g_cmd_file, c->uv_x0); put_s16le(g_cmd_file, c->uv_y0);
    put_s16le(g_cmd_file, c->uv_x1); put_s16le(g_cmd_file, c->uv_y1);
    fputc(c->color.r, g_cmd_file); fputc(c->color.g, g_cmd_file);
    fputc(c->color.b, g_cmd_file); fputc(c->color.a, g_cmd_file);
    fputc(c->outline_color.r, g_cmd_file); fputc(c->outline_color.g, g_cmd_file);
    fputc(c->outline_color.b, g_cmd_file); fputc(c->outline_color.a, g_cmd_file);
}

static int dump_commands(const gsvp89_preset *preset, const char *filename)
{
    gsp89_painter painter;
    gsv89_palette palette;
    g_cmd_file = fopen(filename, "wb");
    if (!g_cmd_file) return 0;
    gsp89_painter_init(&painter, 320, 320, golden_cmd_emit, 0);
    gsp89_painter_set_view(&painter, 160, 160, 134, GSP89_FX_ONE, 0, 0, 255);
    gsvp89_default_palette(preset, &palette);
    gsvp89_emit(&painter, preset, &palette, 255);
    fclose(g_cmd_file);
    g_cmd_file = 0;
    return 1;
}

int main(int argc, char **argv)
{
    const char *out_dir;
    FILE *manifest;
    short i;
    char ppm[512];
    char cmd[512];
    char manifest_path[512];
    const gsvp89_preset *preset;
    if (argc < 2) return 2;
    out_dir = argv[1];
#ifdef GOLDEN_RECIPE_BUILD
    if (argc < 3) return 3;
    gsvp89_set_catalog_path(argv[2]);
    if (!gsvp89_reload()) return 4;
#endif
    sprintf(manifest_path, "%s/manifest.txt", out_dir);
    manifest = fopen(manifest_path, "wb");
    if (!manifest) return 5;
    for (i = 0; i < gsvp89_count(); ++i) {
        preset = gsvp89_get(i);
        if (!preset) { fclose(manifest); return 6; }
        sprintf(ppm, "%s/%03d.ppm", out_dir, (int)i);
        sprintf(cmd, "%s/%03d.cmd", out_dir, (int)i);
        if (!gpr89_render_preset(preset, ppm)) { fclose(manifest); return 7; }
        if (!dump_commands(preset, cmd)) { fclose(manifest); return 8; }
        fprintf(manifest, "%03d %s %d\n", (int)i, preset->name, (int)preset->shape_count);
    }
    fclose(manifest);
    return 0;
}
