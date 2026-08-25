#include "gmyy_spritefont.h"
#include <stdio.h>
#include <string.h>

/* tiny no-heap provider for demo: four 8x12 glyphs with different alpha bounds */
static int demo_rgba_provider(const char* path, unsigned char* out, long cap, int* w, int* h, int* stride, void* user) {
    int x, y;
    int glyph = 0;
    (void)user;
    if (cap < 8L * 12L * 4L) return GMYY_ERR;
    *w = 8; *h = 12; *stride = 8 * 4;
    memset(out, 0, (size_t)(8 * 12 * 4));
    if (strstr(path, "f1")) glyph = 1;
    else if (strstr(path, "f2")) glyph = 2;
    else if (strstr(path, "f3")) glyph = 3;
    for (y = 2; y < 10; ++y) {
        int left = glyph;
        int right = 7 - glyph;
        for (x = left; x <= right; ++x) {
            unsigned char* p = out + y * (*stride) + x * 4;
            p[0] = 255; p[1] = 255; p[2] = 255; p[3] = 255;
        }
    }
    return GMYY_OK;
}

int main(void) {
    const char* yy =
        "{\n"
        "  \"resourceType\": \"GMSprite\",\n"
        "  \"%Name\": \"spr_hud_digits\",\n"
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
    const char* gml = "global.fnt_digits = font_add_sprite_ext(spr_hud_digits, \"0123\", true, 1);";
    GMYY_Sprite spr;
    GMYY_FontCall call;
    GMYY_SpriteFont font;
    GMYY_Status st;
    unsigned char scratch[8 * 12 * 4];
    int w, h, i;

    if (gmyy_decode_sprite_yy(yy, &spr, &st) != GMYY_OK) { printf("sprite error: %s\n", st.message); return 1; }
    if (gmyy_decode_font_call_gml(gml, &call, &st) != GMYY_OK) { printf("gml error: %s\n", st.message); return 1; }
    if (gmyy_build_spritefont(&spr, &call, &font, &st) != GMYY_OK) { printf("build error: %s\n", st.message); return 1; }

    printf("before scan:\n");
    for (i = 0; i < font.glyph_count; ++i) printf(" glyph %d cp=%lu advance=%d\n", i, font.glyphs[i].codepoint, font.glyphs[i].xadvance);

    if (gmyy_font_apply_proportional_rgba_provider(&font, &spr, demo_rgba_provider, scratch, (long)sizeof(scratch), 1, 0, &st) != GMYY_OK) {
        printf("scan error: %s warnings=%d\n", st.message, st.warnings); return 1;
    }

    printf("after scan: %s warnings=%d\n", st.message, st.warnings);
    for (i = 0; i < font.glyph_count; ++i) {
        printf(" glyph %d cp=%lu trim=%d,%d,%d,%d w=%d adv=%d fx=%ld path=%s\n",
            i, font.glyphs[i].codepoint,
            font.glyphs[i].trim_left, font.glyphs[i].trim_top,
            font.glyphs[i].trim_right, font.glyphs[i].trim_bottom,
            font.glyphs[i].w, font.glyphs[i].xadvance, font.glyphs[i].fxadvance,
            spr.frames[font.glyphs[i].frame_index].image_path);
    }

    gmyy_text_measure(&font, "0123 10", &w, &h);
    printf("measure '0123 10' = %d x %d\n", w, h);
    return 0;
}
