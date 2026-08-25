#ifndef DEMO_FONT_H
#define DEMO_FONT_H
#include "giffy_hb.h"

/* Demo glyph ids: ASCII-ish, marks, handmade ligatures, Arabic presentation-ish glyphs. */
#define GID_MISSING 0
#define GID_A 1
#define GID_F 2
#define GID_I 3
#define GID_L 4
#define GID_V 5
#define GID_T 6
#define GID_O 7
#define GID_COMBINING_ACUTE 8
#define GID_S 9
#define GID_X 10
#define GID_COMBINING_DOT 11
#define GID_DEV_KA 20
#define GID_DEV_I 21
#define GID_AR_BEH 30
#define GID_AR_ALEF 31
#define GID_AR_MEEM 32
#define GID_AR_LAM 41
#define GID_AR_LAM_INIT 42
#define GID_AR_LAM_MEDI 43
#define GID_AR_LAM_FINA 44
#define GID_AR_LAM_ISOL 45
#define GID_AR_LAM_ALEF 46
#define GID_AR_BEH_INIT 33
#define GID_AR_BEH_MEDI 34
#define GID_AR_BEH_FINA 35
#define GID_AR_BEH_ISOL 36
#define GID_AR_MEEM_INIT 37
#define GID_AR_MEEM_MEDI 38
#define GID_AR_MEEM_FINA 39
#define GID_AR_MEEM_ISOL 40
#define GID_FI 50
#define GID_FL 51
#define GID_FFI 52
#define GID_S_ALT 60
#define GID_A_REV 61
#define GID_DEV_RA 70
#define GID_DEV_HALANT 71
#define GID_DEV_KA_HALF 72
#define GID_DEV_RA_REPH 73
#define GID_THAI_KO_KAI 80
#define GID_THAI_MAI_EK 81
#define GID_THAI_SARA_I 82
#define GID_DOTTED_CIRCLE 90
#define GID_S_CLASS_ALT 91

/* Sorted by Unicode because runtime glyph lookup uses binary search. */
static const ghb_cmap_pair demo_cmap[] = {
    { 'A', GID_A }, { 'T', GID_T }, { 'V', GID_V }, { 'X', GID_X },
    { 'f', GID_F }, { 'i', GID_I }, { 'l', GID_L }, { 'o', GID_O }, { 's', GID_S },
    { 0x0301UL, GID_COMBINING_ACUTE }, { 0x0307UL, GID_COMBINING_DOT },
    { 0x0627UL, GID_AR_ALEF }, { 0x0628UL, GID_AR_BEH }, { 0x0644UL, GID_AR_LAM }, { 0x0645UL, GID_AR_MEEM },
    { 0x0915UL, GID_DEV_KA }, { 0x0930UL, GID_DEV_RA }, { 0x093fUL, GID_DEV_I }, { 0x094dUL, GID_DEV_HALANT },
    { 0x0e01UL, GID_THAI_KO_KAI }, { 0x0e34UL, GID_THAI_SARA_I }, { 0x0e48UL, GID_THAI_MAI_EK },
    { 0x25CCUL, GID_DOTTED_CIRCLE }
};

static const ghb_advance_pair demo_advances[] = {
    { GID_MISSING, 8 * 64 }, { GID_A, 10 * 64 }, { GID_F, 7 * 64 }, { GID_I, 4 * 64 },
    { GID_L, 4 * 64 }, { GID_V, 10 * 64 }, { GID_T, 9 * 64 }, { GID_O, 9 * 64 },
    { GID_COMBINING_ACUTE, 0 }, { GID_S, 8 * 64 }, { GID_X, 8 * 64 }, { GID_COMBINING_DOT, 0 },
    { GID_DEV_KA, 9 * 64 }, { GID_DEV_I, 0 },
    { GID_AR_BEH, 9 * 64 }, { GID_AR_ALEF, 6 * 64 }, { GID_AR_MEEM, 9 * 64 }, { GID_AR_LAM, 8 * 64 },
    { GID_AR_BEH_INIT, 8 * 64 }, { GID_AR_BEH_MEDI, 8 * 64 }, { GID_AR_BEH_FINA, 8 * 64 }, { GID_AR_BEH_ISOL, 9 * 64 },
    { GID_AR_MEEM_INIT, 8 * 64 }, { GID_AR_MEEM_MEDI, 8 * 64 }, { GID_AR_MEEM_FINA, 8 * 64 }, { GID_AR_MEEM_ISOL, 9 * 64 },
    { GID_AR_LAM_INIT, 7 * 64 }, { GID_AR_LAM_MEDI, 7 * 64 }, { GID_AR_LAM_FINA, 7 * 64 }, { GID_AR_LAM_ISOL, 8 * 64 }, { GID_AR_LAM_ALEF, 9 * 64 },
    { GID_FI, 10 * 64 }, { GID_FL, 10 * 64 }, { GID_FFI, 15 * 64 }, { GID_S_ALT, 8 * 64 }, { GID_A_REV, 10 * 64 },
    { GID_DEV_RA, 9 * 64 }, { GID_DEV_HALANT, 0 }, { GID_DEV_KA_HALF, 6 * 64 }, { GID_DEV_RA_REPH, 0 },
    { GID_THAI_KO_KAI, 9 * 64 }, { GID_THAI_MAI_EK, 0 }, { GID_THAI_SARA_I, 0 },
    { GID_DOTTED_CIRCLE, 8 * 64 }, { GID_S_CLASS_ALT, 8 * 64 }
};

static const ghb_glyph_class demo_classes[] = {
    { GID_MISSING, GHB_GLYPH_BASE }, { GID_A, GHB_GLYPH_BASE }, { GID_F, GHB_GLYPH_BASE },
    { GID_I, GHB_GLYPH_BASE }, { GID_L, GHB_GLYPH_BASE }, { GID_V, GHB_GLYPH_BASE },
    { GID_T, GHB_GLYPH_BASE }, { GID_O, GHB_GLYPH_BASE }, { GID_COMBINING_ACUTE, GHB_GLYPH_MARK },
    { GID_S, GHB_GLYPH_BASE }, { GID_X, GHB_GLYPH_BASE }, { GID_COMBINING_DOT, GHB_GLYPH_MARK },
    { GID_DEV_KA, GHB_GLYPH_BASE }, { GID_DEV_I, GHB_GLYPH_MARK },
    { GID_AR_BEH, GHB_GLYPH_BASE | GHB_GLYPH_JOIN_LEFT | GHB_GLYPH_JOIN_RIGHT },
    { GID_AR_ALEF, GHB_GLYPH_BASE | GHB_GLYPH_JOIN_RIGHT },
    { GID_AR_MEEM, GHB_GLYPH_BASE | GHB_GLYPH_JOIN_LEFT | GHB_GLYPH_JOIN_RIGHT },
    { GID_AR_LAM, GHB_GLYPH_BASE | GHB_GLYPH_JOIN_LEFT | GHB_GLYPH_JOIN_RIGHT },
    { GID_AR_BEH_INIT, GHB_GLYPH_BASE }, { GID_AR_BEH_MEDI, GHB_GLYPH_BASE },
    { GID_AR_BEH_FINA, GHB_GLYPH_BASE }, { GID_AR_BEH_ISOL, GHB_GLYPH_BASE },
    { GID_AR_MEEM_INIT, GHB_GLYPH_BASE }, { GID_AR_MEEM_MEDI, GHB_GLYPH_BASE },
    { GID_AR_MEEM_FINA, GHB_GLYPH_BASE }, { GID_AR_MEEM_ISOL, GHB_GLYPH_BASE },
    { GID_AR_LAM_INIT, GHB_GLYPH_BASE }, { GID_AR_LAM_MEDI, GHB_GLYPH_BASE },
    { GID_AR_LAM_FINA, GHB_GLYPH_BASE }, { GID_AR_LAM_ISOL, GHB_GLYPH_BASE }, { GID_AR_LAM_ALEF, GHB_GLYPH_LIGATURE },
    { GID_FI, GHB_GLYPH_LIGATURE }, { GID_FL, GHB_GLYPH_LIGATURE }, { GID_FFI, GHB_GLYPH_LIGATURE },
    { GID_S_ALT, GHB_GLYPH_BASE }, { GID_A_REV, GHB_GLYPH_BASE },
    { GID_DEV_RA, GHB_GLYPH_BASE }, { GID_DEV_HALANT, GHB_GLYPH_MARK }, { GID_DEV_KA_HALF, GHB_GLYPH_BASE }, { GID_DEV_RA_REPH, GHB_GLYPH_MARK },
    { GID_THAI_KO_KAI, GHB_GLYPH_BASE }, { GID_THAI_MAI_EK, GHB_GLYPH_MARK }, { GID_THAI_SARA_I, GHB_GLYPH_MARK },
    { GID_DOTTED_CIRCLE, GHB_GLYPH_BASE }, { GID_S_CLASS_ALT, GHB_GLYPH_BASE }
};

static const ghb_single_sub demo_single_subs[] = {
    { GID_AR_BEH, GID_AR_BEH_INIT, GHB_FEATURE_INIT, 0 },
    { GID_AR_BEH, GID_AR_BEH_MEDI, GHB_FEATURE_MEDI, 0 },
    { GID_AR_BEH, GID_AR_BEH_FINA, GHB_FEATURE_FINA, 0 },
    { GID_AR_BEH, GID_AR_BEH_ISOL, GHB_FEATURE_ISOL, 0 },
    { GID_AR_MEEM, GID_AR_MEEM_INIT, GHB_FEATURE_INIT, 0 },
    { GID_AR_MEEM, GID_AR_MEEM_MEDI, GHB_FEATURE_MEDI, 0 },
    { GID_AR_MEEM, GID_AR_MEEM_FINA, GHB_FEATURE_FINA, 0 },
    { GID_AR_MEEM, GID_AR_MEEM_ISOL, GHB_FEATURE_ISOL, 0 },
    { GID_AR_LAM, GID_AR_LAM_INIT, GHB_FEATURE_INIT, 0 },
    { GID_AR_LAM, GID_AR_LAM_MEDI, GHB_FEATURE_MEDI, 0 },
    { GID_AR_LAM, GID_AR_LAM_FINA, GHB_FEATURE_FINA, 0 },
    { GID_AR_LAM, GID_AR_LAM_ISOL, GHB_FEATURE_ISOL, 0 },
    { GID_DEV_RA, GID_DEV_RA_REPH, GHB_FEATURE_RPHF, 0 },
    { GID_DEV_KA, GID_DEV_KA_HALF, GHB_FEATURE_HALF, 0 }
};

static const ghb_ligature demo_ligatures[] = {
    { { GID_AR_LAM_INIT, GID_AR_ALEF, 0, 0, 0, 0, 0, 0 }, 2, GID_AR_LAM_ALEF, GHB_FEATURE_RLIG, 0 },
    { { GID_AR_LAM_MEDI, GID_AR_ALEF, 0, 0, 0, 0, 0, 0 }, 2, GID_AR_LAM_ALEF, GHB_FEATURE_RLIG, 0 },
    { { GID_AR_LAM, GID_AR_ALEF, 0, 0, 0, 0, 0, 0 }, 2, GID_AR_LAM_ALEF, GHB_FEATURE_RLIG, 0 },
    { { GID_F, GID_F, GID_I, 0, 0, 0, 0, 0 }, 3, GID_FFI, GHB_FEATURE_LIGA, 0 },
    { { GID_F, GID_I, 0, 0, 0, 0, 0, 0 }, 2, GID_FI, GHB_FEATURE_LIGA, 0 },
    { { GID_F, GID_L, 0, 0, 0, 0, 0, 0 }, 2, GID_FL, GHB_FEATURE_LIGA, 0 }
};

static const ghb_kern_pair demo_kerns[] = {
    { GID_T, GID_O, -2 * 64, GHB_FEATURE_KERN, 0 },
    { GID_A, GID_V, -1 * 64, GHB_FEATURE_KERN, 0 },
    { GID_T, GID_A, -1 * 64, GHB_FEATURE_KERN, 0 }
};

static const ghb_mark_anchor demo_marks[] = {
    { GID_A, GID_COMBINING_ACUTE, 4 * 64, 9 * 64, GHB_FEATURE_MARK, 0 },
    { GID_O, GID_COMBINING_ACUTE, 4 * 64, 8 * 64, GHB_FEATURE_MARK, 0 }
};

static const ghb_mark_mark_anchor demo_mkmks[] = {
    { GID_COMBINING_ACUTE, GID_COMBINING_DOT, 1 * 64, 3 * 64, GHB_FEATURE_MKMK, 0 }
};

static const ghb_mark_meta demo_mark_metas[] = {
    { GID_COMBINING_ACUTE, GHB_MARK_CLASS_ABOVE, 1, 1 },
    { GID_COMBINING_DOT, GHB_MARK_CLASS_ABOVE, 1, 1 },
    { GID_DEV_I, GHB_MARK_CLASS_POST, 2, 2 },
    { GID_DEV_HALANT, GHB_MARK_CLASS_BELOW, 2, 2 },
    { GID_DEV_RA_REPH, GHB_MARK_CLASS_ABOVE, 2, 2 },
    { GID_THAI_SARA_I, GHB_MARK_CLASS_ABOVE, 3, 3 },
    { GID_THAI_MAI_EK, GHB_MARK_CLASS_ABOVE, 3, 3 }
};

static const ghb_context_sub demo_context_subs[] = {
    /* X s X -> contextual alternate on s */
    { { GID_X, 0, 0, 0 }, 1, { GID_S, 0, 0, 0, 0, 0, 0, 0 }, 1, { GID_X, 0, 0, 0 }, 1, 0, GID_S_ALT, GHB_FEATURE_CALT, 0 }
};

static const ghb_reverse_context_sub demo_reverse_context_subs[] = {
    /* Reverse chaining: X A -> replace A when looking backward from end. */
    { { GID_X, 0, 0, 0 }, 1, GID_A, { 0, 0, 0, 0 }, 0, GID_A_REV, GHB_FEATURE_CALT, 0 }
};

static const ghb_context_class_range demo_context_class_ranges[] = {
    /* class 1 = Latin uppercase demo range A..X */
    { 1, GID_A, GID_X }
};

static const ghb_context_class_sub demo_context_class_subs[] = {
    /* any uppercase-ish demo glyph + s + any uppercase-ish demo glyph -> class contextual alt */
    { { (ghb_u16)(0x8000u | 1u), 0, 0, 0 }, 1, { GID_S, 0, 0, 0, 0, 0, 0, 0 }, 1,
      { (ghb_u16)(0x8000u | 1u), 0, 0, 0 }, 1, 0, GID_S_CLASS_ALT, GHB_FEATURE_CALT, 0 }
};

static const ghb_context_pair_adjust demo_context_pair_adjusts[] = {
    /* v0.9: contextual positioning demo: A + V in any uppercase context tightens A. */
    { { GHB_CONTEXT_ANY_GLYPH, 0, 0, 0 }, 1, { GID_A, GID_V, 0, 0, 0, 0, 0, 0 }, 2,
      { GHB_CONTEXT_ANY_GLYPH, 0, 0, 0 }, 1, 0, 1, -1 * 64, GHB_FEATURE_DIST, 0 }
};

static const ghb_reorder_rule demo_reorder_rules[] = {
    /* Devanagari vowel sign i: simple pre-base demonstration hook. */
    { 0x093fUL, 1, GHB_TAG('d','e','v','a') }
};

static const ghb_var_axis demo_axes[] = {
    { GHB_TAG('w','g','h','t'), 100 * 64L, 400 * 64L, 900 * 64L }
};

static const ghb_var_region demo_var_regions[] = {
    { 0, 400 * 64L, 900 * 64L, 1000 * 64L }
};

static const ghb_var_advance_delta demo_var_adv[] = {
    { GID_A, 0, 2 * 64 }, { GID_V, 0, 2 * 64 }
};

static const ghb_var_region_record demo_hvar_regions[] = {
    { 0, 1 }
};

static const ghb_hvar_advance_delta demo_hvar_adv[] = {
    { GID_A, 0, 1 * 64 }, { GID_V, 0, 1 * 64 }
};

static const ghb_hvar_advance_map demo_hvar_maps[] = {
    { GID_A, 0 }, { GID_V, 1 }
};

static const ghb_hvar_item_delta demo_hvar_items[] = {
    { 0, 0, 1 * 64 }, { 1, 0, 1 * 64 }
};

static const ghb_script_lang demo_script_langs[] = {
    { GHB_SCRIPT_LATN, GHB_LANG_DFLT, 6 },
    { GHB_SCRIPT_ARAB, GHB_LANG_DFLT, 5 },
    { GHB_SCRIPT_DEVA, GHB_LANG_DFLT, 8 },
    { GHB_SCRIPT_THAI, GHB_LANG_DFLT, 2 },
    { GHB_SCRIPT_DFLT, GHB_LANG_DFLT, 1 }
};

static const ghb_static_font demo_font = {
    "DemoStaticFont", 1000,
    demo_cmap, (int)(sizeof(demo_cmap) / sizeof(demo_cmap[0])),
    demo_advances, (int)(sizeof(demo_advances) / sizeof(demo_advances[0])),
    8 * 64, GID_MISSING,
    demo_classes, (int)(sizeof(demo_classes) / sizeof(demo_classes[0])),
    demo_single_subs, (int)(sizeof(demo_single_subs) / sizeof(demo_single_subs[0])),
    demo_ligatures, (int)(sizeof(demo_ligatures) / sizeof(demo_ligatures[0])),
    demo_kerns, (int)(sizeof(demo_kerns) / sizeof(demo_kerns[0])),
    demo_marks, (int)(sizeof(demo_marks) / sizeof(demo_marks[0])),
    demo_mkmks, (int)(sizeof(demo_mkmks) / sizeof(demo_mkmks[0])),
    demo_context_subs, (int)(sizeof(demo_context_subs) / sizeof(demo_context_subs[0])),
    demo_reverse_context_subs, (int)(sizeof(demo_reverse_context_subs) / sizeof(demo_reverse_context_subs[0])),
    demo_context_class_ranges, (int)(sizeof(demo_context_class_ranges) / sizeof(demo_context_class_ranges[0])),
    demo_context_class_subs, (int)(sizeof(demo_context_class_subs) / sizeof(demo_context_class_subs[0])),
    demo_context_pair_adjusts, (int)(sizeof(demo_context_pair_adjusts) / sizeof(demo_context_pair_adjusts[0])),
    demo_mark_metas, (int)(sizeof(demo_mark_metas) / sizeof(demo_mark_metas[0])),
    demo_reorder_rules, (int)(sizeof(demo_reorder_rules) / sizeof(demo_reorder_rules[0])),
    demo_script_langs, (int)(sizeof(demo_script_langs) / sizeof(demo_script_langs[0])),
    demo_axes, (int)(sizeof(demo_axes) / sizeof(demo_axes[0])),
    demo_var_regions, (int)(sizeof(demo_var_regions) / sizeof(demo_var_regions[0])),
    demo_var_adv, (int)(sizeof(demo_var_adv) / sizeof(demo_var_adv[0])),
    demo_hvar_regions, (int)(sizeof(demo_hvar_regions) / sizeof(demo_hvar_regions[0])),
    demo_hvar_adv, (int)(sizeof(demo_hvar_adv) / sizeof(demo_hvar_adv[0])),
    demo_hvar_maps, (int)(sizeof(demo_hvar_maps) / sizeof(demo_hvar_maps[0])),
    0, 0, /* hvar_advance_map_segments */
    demo_hvar_items, (int)(sizeof(demo_hvar_items) / sizeof(demo_hvar_items[0])),
    0, 0, /* indic_matra_ranges */
    0, 0  /* thai_lao_mark_ranges */
};

#endif
