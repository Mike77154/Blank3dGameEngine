#include <stdio.h>
#include "../src/gf_arena.h"
#include "../src/gf_fontmake.h"
#include "../src/gf_bitmap.h"
#include "../src/gf_raster.h"
#include "../src/gf_svg.h"
#include "../src/gf_svg_path.h"
#include "../src/gf_gff.h"
#include "../src/gf_shape.h"
#include "../src/gf_hb_compat.h"
#include "../src/gf_freetype_compat.h"
#include "../src/gf_hintvm.h"
#include "../src/gf_transform.h"
#include "../src/gf_layout.h"

static void push32(GF_HintProgram *p, long v) {
    p->code[p->size++] = GF_OP_PUSH;
    p->code[p->size++] = (unsigned char)(v & 255);
    p->code[p->size++] = (unsigned char)((v >> 8) & 255);
    p->code[p->size++] = (unsigned char)((v >> 16) & 255);
    p->code[p->size++] = (unsigned char)((v >> 24) & 255);
}

int main(void) {
    GF_Arena arena;
    GF_Font font;
    GF_Font loaded;
    GF_Bitmap bmp;
    GF_Bitmap text_bmp;
    GF_Atlas atlas;
    GF_Glyph *g;
    GF_Glyph tmp_glyph;
    unsigned short ax, ay;
    GF_ShapePlan plan;
    gf_hb_buffer_t hb;
    GF_FT_Face face;
    GF_FT_GlyphSlot slot;
    unsigned int txt[5];
    GF_HintProgram hp;
    GF_TextRun run;
    int si;

    gf_arena_init(&arena);
    gf_build_demo_font(&font);
    gf_atlas_clear(&atlas);

    si = gf_font_add_glyph(&font, 'S', GF_FX(18));
    if (si >= 0) {
        gf_svg_path_to_glyph(&font.glyphs[si], "M 2 18 Q 2 8 8 8 Q 15 8 15 0 L 12 0 Q 12 5 8 5 Q 0 5 0 18 Z", 'S', GF_FX(18), 18);
        gf_svg_write_glyph("out_S_from_svg_path.svg", &font.glyphs[si], 32, 24);
    }

    g = gf_font_find(&font, 'O');
    if (g) {
        gf_rasterize_1bit(g, &bmp, 32, 32, GF_FP_ONE, GF_FX(8), GF_FX(4), &arena);
        gf_write_pbm("out_O.pbm", &bmp);
        gf_svg_write_glyph("out_O.svg", g, 32, 24);
        gf_atlas_pack(&atlas, &bmp, &ax, &ay);
    }

    gf_shape_plan_clear(&plan);
    gf_shape_add_ligature(&plan, 'f', 'i', 0xFB01u);
    gf_shape_add_kern(&plan, 'A', 'V', -GF_FX(2));

    gf_hb_buffer_clear(&hb);
    txt[0] = 'A'; txt[1] = 'V'; txt[2] = 'f'; txt[3] = 'i'; txt[4] = 'S';
    gf_hb_buffer_add_utf32(&hb, txt, 5);
    gf_hb_shape(&font, &plan, &hb);
    printf("shape items: %d\n", hb.item_count);
    printf("item0 cp=%lu xoff=%ld adv=%ld\n", (unsigned long)hb.items[0].codepoint, hb.items[0].x_offset, hb.items[0].x_advance);
    printf("item1 cp=%lu xoff=%ld adv=%ld\n", (unsigned long)hb.items[1].codepoint, hb.items[1].x_offset, hb.items[1].x_advance);
    printf("item2 cp=%lu ligature fi\n", (unsigned long)hb.items[2].codepoint);

    gf_ft_face_init(&face, &font);
    if (gf_ft_load_char(&face, 'A', &slot) == 0) {
        gf_arena_reset(&arena);
        gf_ft_render_glyph(&slot, 32, 32, GF_FP_ONE, GF_FX(8), GF_FX(4), &arena);
        gf_write_pbm("out_A.pbm", &slot.bitmap);
    }

    g = gf_font_find(&font, 'V');
    if (g) {
        hp.size = 0;
        push32(&hp, 0); hp.code[hp.size++] = GF_OP_ROUND_X;
        push32(&hp, 0); hp.code[hp.size++] = GF_OP_ROUND_Y;
        hp.code[hp.size++] = GF_OP_END;
        gf_hint_run(g, &hp);
        gf_svg_write_glyph("out_V_hinted.svg", g, 32, 24);
    }

    g = gf_font_find(&font, 'A');
    if (g) {
        gf_glyph_copy(&tmp_glyph, g);
        gf_glyph_embolden(&tmp_glyph, GF_FP_HALF, 0);
        gf_svg_write_glyph("out_A_embolden.svg", &tmp_glyph, 32, 24);
    }

    gf_text_layout(&font, &plan, txt, 5, &run);
    gf_arena_reset(&arena);
    gf_text_render_1bit(&font, &run, &text_bmp, 128, 32, GF_FP_ONE, GF_FX(4), GF_FX(5), &arena);
    gf_write_pbm("out_text_AVfiS.pbm", &text_bmp);

    gf_gff_write("demo_font.gff", &font);
    gf_font_clear(&loaded);
    if (gf_gff_read("demo_font.gff", &loaded) == 0) printf("gff roundtrip glyphs: %u\n", (unsigned int)loaded.glyph_count);

    gf_write_atlas_pbm("out_atlas.pbm", &atlas);
    printf("arena used: %ld remaining: %ld\n", gf_arena_used(&arena), gf_arena_remaining(&arena));
    printf("wrote v0.2 outputs: PBM/SVG/GFF/text layout\n");
    return 0;
}
