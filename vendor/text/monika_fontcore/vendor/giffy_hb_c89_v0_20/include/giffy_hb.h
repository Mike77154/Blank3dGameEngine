#ifndef GIFFY_HB_H
#define GIFFY_HB_H

/*
   giffy_hb.h - static OpenType-like shaping core, C89.
   Target runtime constraints:
   - no malloc/free/realloc
   - no heap ownership
   - no float/double
   - 26.6 fixed point metrics

   The runtime consumes static font packs generated offline.
*/

#ifdef __cplusplus
extern "C" {
#endif

#define GHB_VERSION_MAJOR 0
#define GHB_VERSION_MINOR 20
#define GHB_VERSION_PATCH 0

#ifndef GHB_MAX_GLYPHS
#define GHB_MAX_GLYPHS 256
#endif

#ifndef GHB_MAX_FEATURES
#define GHB_MAX_FEATURES 32
#endif

#ifndef GHB_MAX_LIG_COMPONENTS
#define GHB_MAX_LIG_COMPONENTS 8
#endif

#define GHB_TAG(a,b,c,d) (((unsigned long)(a)<<24)|((unsigned long)(b)<<16)|((unsigned long)(c)<<8)|((unsigned long)(d)))

#define GHB_FEATURE_LIGA GHB_TAG('l','i','g','a')
#define GHB_FEATURE_CCMP GHB_TAG('c','c','m','p')
#define GHB_FEATURE_CLIG GHB_TAG('c','l','i','g')
#define GHB_FEATURE_DLIG GHB_TAG('d','l','i','g')
#define GHB_FEATURE_RLIG GHB_TAG('r','l','i','g')
#define GHB_FEATURE_KERN GHB_TAG('k','e','r','n')
#define GHB_FEATURE_DIST GHB_TAG('d','i','s','t')
#define GHB_FEATURE_MARK GHB_TAG('m','a','r','k')
#define GHB_FEATURE_MKMK GHB_TAG('m','k','m','k')
#define GHB_FEATURE_CALT GHB_TAG('c','a','l','t')
#define GHB_FEATURE_INIT GHB_TAG('i','n','i','t')
#define GHB_FEATURE_MEDI GHB_TAG('m','e','d','i')
#define GHB_FEATURE_FINA GHB_TAG('f','i','n','a')
#define GHB_FEATURE_ISOL GHB_TAG('i','s','o','l')
#define GHB_FEATURE_ABVS GHB_TAG('a','b','v','s')
#define GHB_FEATURE_BLWS GHB_TAG('b','l','w','s')
#define GHB_FEATURE_HALN GHB_TAG('h','a','l','n')
#define GHB_FEATURE_PRES GHB_TAG('p','r','e','s')
#define GHB_FEATURE_PSTS GHB_TAG('p','s','t','s')
#define GHB_FEATURE_RPHF GHB_TAG('r','p','h','f')
#define GHB_FEATURE_HALF GHB_TAG('h','a','l','f')
#define GHB_FEATURE_VATU GHB_TAG('v','a','t','u')
#define GHB_FEATURE_CJCT GHB_TAG('c','j','c','t')
#define GHB_FEATURE_BLWF GHB_TAG('b','l','w','f')
#define GHB_FEATURE_PSTF GHB_TAG('p','s','t','f')
#define GHB_FEATURE_NUKT GHB_TAG('n','u','k','t')
#define GHB_FEATURE_AKHN GHB_TAG('a','k','h','n')
#define GHB_FEATURE_PREF GHB_TAG('p','r','e','f')
#define GHB_FEATURE_ABVF GHB_TAG('a','b','v','f')
#define GHB_FEATURE_BLWM GHB_TAG('b','l','w','m')
#define GHB_FEATURE_ABVM GHB_TAG('a','b','v','m')

#define GHB_SCRIPT_LATN  GHB_TAG('l','a','t','n')
#define GHB_SCRIPT_ARAB  GHB_TAG('a','r','a','b')
#define GHB_SCRIPT_DEVA  GHB_TAG('d','e','v','a')
#define GHB_SCRIPT_BENG  GHB_TAG('b','e','n','g')
#define GHB_SCRIPT_GURU  GHB_TAG('g','u','r','u')
#define GHB_SCRIPT_GUJR  GHB_TAG('g','u','j','r')
#define GHB_SCRIPT_TAML  GHB_TAG('t','a','m','l')
#define GHB_SCRIPT_TELU  GHB_TAG('t','e','l','u')
#define GHB_SCRIPT_KNDA  GHB_TAG('k','n','d','a')
#define GHB_SCRIPT_MLYM  GHB_TAG('m','l','y','m')
#define GHB_SCRIPT_SINH  GHB_TAG('s','i','n','h')
#define GHB_SCRIPT_THAI  GHB_TAG('t','h','a','i')
#define GHB_SCRIPT_LAO   GHB_TAG('l','a','o',' ')
#define GHB_SCRIPT_DFLT  GHB_TAG('D','F','L','T')
#define GHB_LANG_DFLT    GHB_TAG('d','f','l','t')

#define GHB_OK 0
#define GHB_ERR_OVERFLOW -1
#define GHB_ERR_BAD_ARG -2
#define GHB_ERR_UNSUPPORTED -3
#define GHB_ERR_TRUNCATED_UTF8 -4
#define GHB_ERR_BAD_UTF8 -5

#define GHB_DIR_LTR 0
#define GHB_DIR_RTL 1

#define GHB_GLYPH_BASE      0x0001u
#define GHB_GLYPH_LIGATURE  0x0002u
#define GHB_GLYPH_MARK      0x0004u
#define GHB_GLYPH_COMPONENT 0x0008u
#define GHB_GLYPH_JOIN_LEFT 0x0010u
#define GHB_GLYPH_JOIN_RIGHT 0x0020u

#define GHB_LOOKUP_RIGHT_TO_LEFT       0x0001u
#define GHB_LOOKUP_IGNORE_BASE_GLYPHS  0x0002u
#define GHB_LOOKUP_IGNORE_LIGATURES    0x0004u
#define GHB_LOOKUP_IGNORE_MARKS        0x0008u
#define GHB_LOOKUP_USE_MARK_FILTERING   0x0010u
#define GHB_LOOKUP_MARK_ATTACHMENT_TYPE_MASK 0xff00u
#define GHB_LOOKUP_MARK_FILTERING_SET_MASK 0x00e0u
#define GHB_LOOKUP_MARK_FILTERING_SET_SHIFT 5

#define GHB_JOIN_NONE  0
#define GHB_JOIN_RIGHT 1
#define GHB_JOIN_DUAL  2
#define GHB_JOIN_TRANSPARENT 3

#define GHB_SYLL_NONE       0
#define GHB_SYLL_INDIC_BASE 1
#define GHB_SYLL_INDIC_MARK 2
#define GHB_SYLL_INDIC_HALANT 3
#define GHB_SYLL_INDIC_OTHER 4

#define GHB_MARK_CLASS_NONE 0
#define GHB_MARK_CLASS_ABOVE 1
#define GHB_MARK_CLASS_BELOW 2
#define GHB_MARK_CLASS_POST 3
#define GHB_MARK_CLASS_TONE 4
#define GHB_MATRA_NONE 0
#define GHB_MATRA_PRE  1
#define GHB_MATRA_ABOVE 2
#define GHB_MATRA_BELOW 3
#define GHB_MATRA_POST 4
#define GHB_MARK_SET_ALL 0xffu
#define GHB_CONTEXT_ANY_GLYPH 0xffffu
#define GHB_DOTTED_CIRCLE_UNICODE 0x25CCUL

/* 26.6 fixed helpers */
typedef long ghb_f26d6;
#define GHB_F26D6_ONE ((ghb_f26d6)64)
#define GHB_F26D6_FROM_INT(x) ((ghb_f26d6)((x) << 6))
#define GHB_F26D6_TO_INT(x) ((int)((x) >> 6))
#define GHB_F26D6_MUL(a,b) ((ghb_f26d6)(((a) * (b)) >> 6))
#define GHB_F26D6_DIV(a,b) ((ghb_f26d6)(((a) << 6) / (b)))

typedef unsigned long ghb_u32;
typedef unsigned short ghb_u16;
typedef signed short ghb_s16;
typedef unsigned char ghb_u8;

typedef void (*ghb_trace_func)(void *user, const char *stage, int index, ghb_u16 gid, ghb_u16 out_gid);
typedef void (*ghb_trace_dump_func)(void *user, int index, ghb_u16 gid, ghb_u32 unicode, ghb_f26d6 x_advance, ghb_f26d6 x_offset, ghb_f26d6 y_offset, ghb_u16 flags, ghb_u8 syllable, ghb_u8 syllable_kind);
typedef void (*ghb_trace_numeric_func)(void *user, int row, ghb_u8 stage, ghb_u8 event_id, ghb_s16 index, ghb_u16 gid, ghb_u16 value);

#define GHB_TRACE_STAGE_NONE          0
#define GHB_TRACE_STAGE_SINGLE        1
#define GHB_TRACE_STAGE_LIGATURE      2
#define GHB_TRACE_STAGE_CONTEXT       3
#define GHB_TRACE_STAGE_CONTEXT_CLASS 4
#define GHB_TRACE_STAGE_REVERSE       5
#define GHB_TRACE_STAGE_CONTEXT_PAIR  6
#define GHB_TRACE_STAGE_KERN          7
#define GHB_TRACE_STAGE_MARK_BASE     8
#define GHB_TRACE_STAGE_MARK_MARK     9
#define GHB_TRACE_STAGE_INDIC_REPH    10
#define GHB_TRACE_STAGE_INDIC_HALF    11
#define GHB_TRACE_STAGE_HVAR          12
#define GHB_TRACE_STAGE_DOTTED        13
#define GHB_TRACE_STAGE_INDIC_MATRA   14
#define GHB_TRACE_STAGE_SCRIPT_LANG   15
#define GHB_TRACE_STAGE_CLUSTER       16
#define GHB_TRACE_STAGE_HVAR_MAP      17
#define GHB_TRACE_STAGE_FEATURE_INV   18
#define GHB_TRACE_STAGE_THAI_LAO      19
#define GHB_TRACE_STAGE_HVAR_VALIDATE 20
#define GHB_TRACE_STAGE_SYLLABLE_STATUS 21
#define GHB_TRACE_STAGE_HVAR_REGION 22
#define GHB_TRACE_STAGE_CONTEXT_PAIR_APPLY 23
#define GHB_TRACE_STAGE_OVERLAY 24
#define GHB_TRACE_STAGE_HVAR_EXPLAIN 25
#define GHB_TRACE_STAGE_REGION_VALIDATE 26
#define GHB_TRACE_STAGE_CONTEXT_PAIR_EMIT 27


/* v0.20: stable cluster status enum for HUD / shaping decisions. */
#define GHB_CLUSTER_STATUS_EMPTY       0u
#define GHB_CLUSTER_STATUS_VALID       1u
#define GHB_CLUSTER_STATUS_MARK_ONLY   2u
#define GHB_CLUSTER_STATUS_BROKEN      3u
#define GHB_CLUSTER_STATUS_NEEDS_DOT   4u

/* v0.17: syllable/cluster status flags for dotted-circle and debug overlays. */
#define GHB_SYLL_STATUS_EMPTY       0u
#define GHB_SYLL_STATUS_HAS_BASE    1u
#define GHB_SYLL_STATUS_MARK_ONLY   2u
#define GHB_SYLL_STATUS_NEEDS_DOT   4u
#define GHB_SYLL_STATUS_VALID       8u
#define GHB_SYLL_STATUS_BROKEN      16u

/* v0.16: compact per-script matra categories emitted by the offline packer. */
typedef struct ghb_indic_matra_range_s {
    ghb_u32 script_tag;
    ghb_u32 first_unicode;
    ghb_u32 last_unicode;
    ghb_u8 category;
} ghb_indic_matra_range;

/* v0.16: generated Thai/Lao mark classes. The Unicode fallback remains in
   the runtime, but a font pack may override/extend it with these ranges. */
typedef struct ghb_thai_lao_mark_range_s {
    ghb_u32 first_unicode;
    ghb_u32 last_unicode;
    ghb_u8 mark_class;
} ghb_thai_lao_mark_range;

#define GHB_HVAR_MAP_NONE    0
#define GHB_HVAR_MAP_SPARSE  1
#define GHB_HVAR_MAP_SEGMENT 2

typedef struct ghb_trace_record_s {
    ghb_u8 stage;
    ghb_s16 index;
    ghb_u16 gid;
    ghb_u16 out_gid;
} ghb_trace_record;

/* v0.19: overlay-ready trace record with stable event_id. The old ring
   remains tiny and stable; this view is generated into caller memory. */
#define GHB_EVENT_NONE               0u
#define GHB_EVENT_SUB_SINGLE         1u
#define GHB_EVENT_SUB_LIGATURE       2u
#define GHB_EVENT_CONTEXT_MATCH      3u
#define GHB_EVENT_CONTEXT_PAIR       4u
#define GHB_EVENT_KERN_PAIR          5u
#define GHB_EVENT_MARK_ATTACH        6u
#define GHB_EVENT_INDIC_CLUSTER      7u
#define GHB_EVENT_HVAR_MAP           8u
#define GHB_EVENT_HVAR_REGION        9u
#define GHB_EVENT_DOTTED_CIRCLE      10u
#define GHB_EVENT_THAI_LAO_MARK      11u
#define GHB_EVENT_FEATURE_INVENTORY  12u
#define GHB_EVENT_REGION_VALIDATE    13u
#define GHB_EVENT_HVAR_EXPLAIN        14u
#define GHB_EVENT_CLUSTER_STATUS      15u

typedef struct ghb_trace_overlay_record_s {
    ghb_u8 stage;
    ghb_u8 event_id;
    ghb_u8 map_kind;
    ghb_s16 index;
    ghb_u16 gid;
    ghb_u16 value;
} ghb_trace_overlay_record;

/* v0.20: denser HUD event view. It keeps only stable event id plus two
   payload words, so overlays can draw rows without knowing internal strings. */
typedef struct ghb_trace_overlay_compact_s {
    ghb_u8 event_id;
    ghb_u8 flags;
    ghb_s16 index;
    ghb_u16 a;
    ghb_u16 b;
} ghb_trace_overlay_compact;

/* v0.18: HVAR region contribution explanation. All values are fixed 26.6. */
typedef struct ghb_hvar_contribution_s {
    ghb_u16 gid;
    ghb_u16 item_index;
    ghb_u16 region_index;
    ghb_u8 map_kind;
    ghb_s16 delta_26d6;
    ghb_f26d6 support_26d6;
    ghb_f26d6 contribution_26d6;
} ghb_hvar_contribution;

/* v0.19: compact one-row explanation for HUD/debug overlays. */
typedef struct ghb_hvar_explain_record_s {
    ghb_u16 gid;
    ghb_u16 item_index;
    ghb_u8 map_kind;
    ghb_u8 valid_item;
    ghb_u8 region_count;
    ghb_u8 bad_region_count;
    ghb_f26d6 total_delta_26d6;
} ghb_hvar_explain_record;

/* v0.20: region range diagnostics for packer/runtime sanity checks. */
typedef struct ghb_hvar_region_diag_s {
    int bad_range_count;
    int bad_axis_count;
    int bad_record_count;
    int empty_record_count;
    int max_axes_per_record;
} ghb_hvar_region_diag;

typedef struct ghb_hvar_diag_s {
    int sparse_map_count;
    int segment_map_count;
    int item_delta_count;
    int mapped_advance_count;
    int unmapped_advance_count;
    int lossy;
    int lossless_map;
    int mapped_by_sparse;
    int mapped_by_segment;
    int max_regions_per_item;
} ghb_hvar_diag;

typedef struct ghb_glyph_info_s {
    ghb_u32 codepoint;      /* Unicode on input, glyph id on output */
    ghb_u32 cluster;        /* original input byte/codepoint index */
    ghb_u32 unicode;        /* preserved Unicode scalar, useful for script shapers */
    ghb_u32 mask;           /* caller/feature mask */
    ghb_u16 glyph_flags;    /* base/mark/ligature/joining metadata */
    ghb_u8 syllable;         /* no-heap syllable serial, used by Indic/SEA pass */
    ghb_u8 syllable_kind;    /* coarse category for debug/reorder */
} ghb_glyph_info;

typedef struct ghb_glyph_pos_s {
    ghb_f26d6 x_advance;
    ghb_f26d6 y_advance;
    ghb_f26d6 x_offset;
    ghb_f26d6 y_offset;
} ghb_glyph_pos;

typedef struct ghb_buffer_s {
    ghb_glyph_info info[GHB_MAX_GLYPHS];
    ghb_glyph_pos  pos[GHB_MAX_GLYPHS];
    int len;
    int direction;
    ghb_u32 script;
    ghb_u32 language;
    ghb_f26d6 variations[4];
    int variation_count;
    ghb_trace_func trace_func;
    void *trace_user;
    ghb_trace_record *trace_records;
    int trace_record_capacity;
    int trace_record_count;
    int trace_record_head;
    ghb_u32 trace_stage_mask; /* v0.13: bitmask; bit stage enabled, 0xffffffff default */
} ghb_buffer;

typedef struct ghb_feature_s {
    ghb_u32 tag;
    int value;
    int start;
    int end;
} ghb_feature;

typedef struct ghb_single_sub_s {
    ghb_u16 from_gid;
    ghb_u16 to_gid;
    ghb_u32 feature_tag;
    ghb_u16 lookup_flags;
} ghb_single_sub;

typedef struct ghb_ligature_s {
    ghb_u16 components[GHB_MAX_LIG_COMPONENTS];
    ghb_u8 component_count;
    ghb_u16 lig_gid;
    ghb_u32 feature_tag;
    ghb_u16 lookup_flags;
} ghb_ligature;

typedef struct ghb_kern_pair_s {
    ghb_u16 left_gid;
    ghb_u16 right_gid;
    ghb_s16 x_advance_delta_26d6;
    ghb_u32 feature_tag;
    ghb_u16 lookup_flags;
} ghb_kern_pair;

typedef struct ghb_mark_anchor_s {
    ghb_u16 base_gid;
    ghb_u16 mark_gid;
    ghb_f26d6 x_offset_26d6;
    ghb_f26d6 y_offset_26d6;
    ghb_u32 feature_tag;
    ghb_u16 lookup_flags;
} ghb_mark_anchor;



typedef struct ghb_mark_mark_anchor_s {
    ghb_u16 base_mark_gid;
    ghb_u16 mark_gid;
    ghb_f26d6 x_offset_26d6;
    ghb_f26d6 y_offset_26d6;
    ghb_u32 feature_tag;
    ghb_u16 lookup_flags;
} ghb_mark_mark_anchor;

typedef struct ghb_context_sub_s {
    ghb_u16 backtrack[4];
    ghb_u8 backtrack_count;
    ghb_u16 input[8];
    ghb_u8 input_count;
    ghb_u16 lookahead[4];
    ghb_u8 lookahead_count;
    ghb_u8 replace_index;
    ghb_u16 to_gid;
    ghb_u32 feature_tag;
    ghb_u16 lookup_flags;
} ghb_context_sub;


typedef struct ghb_reverse_context_sub_s {
    ghb_u16 backtrack[4];
    ghb_u8 backtrack_count;
    ghb_u16 input_gid;
    ghb_u16 lookahead[4];
    ghb_u8 lookahead_count;
    ghb_u16 to_gid;
    ghb_u32 feature_tag;
    ghb_u16 lookup_flags;
} ghb_reverse_context_sub;

/* v0.8: compact class/coverage-like contextual substitution.
   Each slot may be an exact glyph id, GHB_CONTEXT_ANY_GLYPH, or a compact
   class id encoded as 0x8000 | class_id and resolved through class ranges. */
typedef struct ghb_context_class_range_s {
    ghb_u16 class_id;
    ghb_u16 first_gid;
    ghb_u16 last_gid;
} ghb_context_class_range;

typedef struct ghb_context_class_sub_s {
    ghb_u16 backtrack[4];
    ghb_u8 backtrack_count;
    ghb_u16 input[8];
    ghb_u8 input_count;
    ghb_u16 lookahead[4];
    ghb_u8 lookahead_count;
    ghb_u8 replace_index;
    ghb_u16 to_gid;
    ghb_u32 feature_tag;
    ghb_u16 lookup_flags;
} ghb_context_class_sub;


/* v0.9: contextual positioning subset. Slots use the same token encoding as
   ghb_context_class_sub: exact gid, GHB_CONTEXT_ANY_GLYPH, or 0x8000|class. */
typedef struct ghb_context_pair_adjust_s {
    ghb_u16 backtrack[4];
    ghb_u8 backtrack_count;
    ghb_u16 input[8];
    ghb_u8 input_count;
    ghb_u16 lookahead[4];
    ghb_u8 lookahead_count;
    ghb_u8 left_index;
    ghb_u8 right_index;
    ghb_s16 x_advance_delta_26d6;
    ghb_u32 feature_tag;
    ghb_u16 lookup_flags;
} ghb_context_pair_adjust;

/* v0.9: offline VarStore map records. The packer may collapse HVAR item
   variation data into per-glyph region deltas, but this map keeps the original
   glyph->advance-item relation visible for debug and future lossless export. */
typedef struct ghb_hvar_advance_map_s {
    ghb_u16 gid;
    ghb_u16 item_index;
} ghb_hvar_advance_map;

/* v0.11: optional segment map equivalent for compact DeltaSetIndexMap.
   If present, runtime may resolve glyph->item_index through ranges instead
   of one record per glyph. */
typedef struct ghb_hvar_advance_map_segment_s {
    ghb_u16 first_gid;
    ghb_u16 last_gid;
    ghb_u16 first_item_index;
} ghb_hvar_advance_map_segment;

/* v0.10: explicit static VarStore item deltas. Packer can emit these
   directly after parsing HVAR ItemVariationStore. Runtime blends them through
   the same 26.6 region support path without owning memory. */
typedef struct ghb_hvar_item_delta_s {
    ghb_u16 item_index;
    ghb_u16 region_index;
    ghb_s16 delta_26d6;
} ghb_hvar_item_delta;

typedef struct ghb_mark_meta_s {
    ghb_u16 gid;
    ghb_u8 mark_class;
    ghb_u8 mark_set;
    ghb_u16 mark_attachment_type;
} ghb_mark_meta;


typedef struct ghb_script_lang_s {
    ghb_u32 script_tag;
    ghb_u32 lang_tag;
    ghb_u16 feature_count;
} ghb_script_lang;

typedef struct ghb_reorder_rule_s {
    ghb_u32 unicode;
    ghb_s16 shift_left;
    ghb_u32 script_tag;
} ghb_reorder_rule;

typedef struct ghb_var_axis_s {
    ghb_u32 tag;
    ghb_f26d6 min_value;
    ghb_f26d6 default_value;
    ghb_f26d6 max_value;
} ghb_var_axis;


typedef struct ghb_var_region_s {
    ghb_u8 axis_index;
    ghb_f26d6 start_coord;
    ghb_f26d6 peak_coord;
    ghb_f26d6 end_coord;
} ghb_var_region;

typedef struct ghb_var_advance_delta_s {
    ghb_u16 gid;
    ghb_u8 axis_index;
    ghb_s16 delta_at_max_26d6;
} ghb_var_advance_delta;

/* v0.8: HVAR ItemVariationStore-style static records.
   Region records point to one or more axis support triangles; item deltas map
   glyph advances to region-scaled deltas. Everything stays static and 26.6. */
typedef struct ghb_var_region_record_s {
    ghb_u16 first_region_axis;
    ghb_u16 region_axis_count;
} ghb_var_region_record;

typedef struct ghb_hvar_advance_delta_s {
    ghb_u16 gid;
    ghb_u16 region_index;
    ghb_s16 delta_26d6;
} ghb_hvar_advance_delta;

typedef struct ghb_cmap_pair_s {
    ghb_u32 unicode;
    ghb_u16 gid;
} ghb_cmap_pair;

typedef struct ghb_advance_pair_s {
    ghb_u16 gid;
    ghb_f26d6 advance_26d6;
} ghb_advance_pair;

typedef struct ghb_glyph_class_s {
    ghb_u16 gid;
    ghb_u16 flags;
} ghb_glyph_class;

typedef struct ghb_static_font_s {
    const char *name;
    ghb_u16 units_per_em;
    const ghb_cmap_pair *cmap;
    int cmap_count;
    const ghb_advance_pair *advances;
    int advance_count;
    ghb_f26d6 default_advance_26d6;
    ghb_u16 missing_gid;

    const ghb_glyph_class *glyph_classes;
    int glyph_class_count;
    const ghb_single_sub *single_subs;
    int single_sub_count;
    const ghb_ligature *ligatures;
    int ligature_count;
    const ghb_kern_pair *kern_pairs;
    int kern_pair_count;
    const ghb_mark_anchor *mark_anchors;
    int mark_anchor_count;
    const ghb_mark_mark_anchor *mark_mark_anchors;
    int mark_mark_anchor_count;
    const ghb_context_sub *context_subs;
    int context_sub_count;
    const ghb_reverse_context_sub *reverse_context_subs;
    int reverse_context_sub_count;
    const ghb_context_class_range *context_class_ranges;
    int context_class_range_count;
    const ghb_context_class_sub *context_class_subs;
    int context_class_sub_count;
    const ghb_context_pair_adjust *context_pair_adjusts;
    int context_pair_adjust_count;
    const ghb_mark_meta *mark_metas;
    int mark_meta_count;
    const ghb_reorder_rule *reorder_rules;
    int reorder_rule_count;
    const ghb_script_lang *script_langs;
    int script_lang_count;
    const ghb_var_axis *var_axes;
    int var_axis_count;
    const ghb_var_region *var_regions;
    int var_region_count;
    const ghb_var_advance_delta *var_advance_deltas;
    int var_advance_delta_count;
    const ghb_var_region_record *hvar_region_records;
    int hvar_region_record_count;
    const ghb_hvar_advance_delta *hvar_advance_deltas;
    int hvar_advance_delta_count;
    const ghb_hvar_advance_map *hvar_advance_maps;
    int hvar_advance_map_count;
    const ghb_hvar_advance_map_segment *hvar_advance_map_segments;
    int hvar_advance_map_segment_count;
    const ghb_hvar_item_delta *hvar_item_deltas;
    int hvar_item_delta_count;

    /* v0.16 optional generated tables; may be NULL/0. */
    const ghb_indic_matra_range *indic_matra_ranges;
    int indic_matra_range_count;
    const ghb_thai_lao_mark_range *thai_lao_mark_ranges;
    int thai_lao_mark_range_count;
} ghb_static_font;

void ghb_buffer_init(ghb_buffer *b);
void ghb_buffer_clear(ghb_buffer *b);
void ghb_buffer_set_direction(ghb_buffer *b, int direction);
void ghb_buffer_set_script(ghb_buffer *b, ghb_u32 script);
void ghb_buffer_set_language(ghb_buffer *b, ghb_u32 language);
void ghb_buffer_set_trace(ghb_buffer *b, ghb_trace_func fn, void *user);
void ghb_buffer_set_trace_ring(ghb_buffer *b, ghb_trace_record *records, int capacity);
void ghb_buffer_set_trace_stage_mask(ghb_buffer *b, ghb_u32 stage_mask);
int ghb_buffer_get_trace_ring_count(const ghb_buffer *b);
int ghb_buffer_copy_trace_ring(const ghb_buffer *b, ghb_trace_record *out_records, int max_records);
int ghb_buffer_copy_trace_ring_stage(const ghb_buffer *b, ghb_trace_record *out_records, int max_records, int stage);
int ghb_buffer_copy_trace_ring_mask(const ghb_buffer *b, ghb_trace_record *out_records, int max_records, ghb_u32 stage_mask);
ghb_u32 ghb_trace_stage_mask(int stage);
void ghb_buffer_set_trace_stage_enabled(ghb_buffer *b, int stage, int enabled);
const char *ghb_trace_stage_name(int stage);
int ghb_trace_event_id(int stage);
int ghb_buffer_dump_trace_records(const ghb_buffer *b, ghb_trace_dump_func fn, void *user);
ghb_u32 ghb_guess_script_from_text(const ghb_u32 *text, int count);
void ghb_buffer_set_script_language_from_text(const ghb_static_font *font, ghb_buffer *b);

int ghb_buffer_add_utf32(ghb_buffer *b, const ghb_u32 *text, int count);
int ghb_buffer_add_utf8(ghb_buffer *b, const char *text, int byte_count);

int ghb_shape(const ghb_static_font *font, ghb_buffer *b,
              const ghb_feature *features, int feature_count);
void ghb_buffer_set_variation(ghb_buffer *b, int axis_index, ghb_f26d6 coord_26d6);

int ghb_shape_utf8(const ghb_static_font *font, ghb_buffer *b,
                   const char *text, int byte_count,
                   const ghb_feature *features, int feature_count);

int ghb_feature_enabled(const ghb_feature *features, int feature_count,
                        ghb_u32 tag, int glyph_index);

ghb_u16 ghb_font_get_glyph(const ghb_static_font *font, ghb_u32 unicode);
ghb_f26d6 ghb_font_get_advance(const ghb_static_font *font, ghb_u16 gid);
ghb_f26d6 ghb_font_get_advance_var(const ghb_static_font *font, const ghb_buffer *b, ghb_u16 gid);
int ghb_font_get_hvar_item_index(const ghb_static_font *font, ghb_u16 gid, ghb_u16 *item_index);
int ghb_font_get_hvar_diagnostics(const ghb_static_font *font, ghb_hvar_diag *diag);
int ghb_font_validate_hvar_gid_item_map(const ghb_static_font *font, ghb_u16 gid, ghb_u16 *item_index, ghb_u8 *map_kind);
int ghb_font_validate_hvar_gid_range(const ghb_static_font *font, ghb_u16 first_gid, ghb_u16 last_gid, int *mapped_count, int *unmapped_count, int *sparse_count, int *segment_count);
int ghb_font_validate_hvar_item_index(const ghb_static_font *font, ghb_u16 item_index, int *delta_count, int *bad_region_count);
int ghb_font_get_hvar_contributions(const ghb_static_font *font, const ghb_buffer *b, ghb_u16 gid, ghb_hvar_contribution *out_items, int max_items, ghb_f26d6 *total_delta);
int ghb_font_explain_hvar_gid(const ghb_static_font *font, const ghb_buffer *b, ghb_u16 gid, ghb_hvar_explain_record *out_record);
int ghb_font_validate_hvar_region_ranges(const ghb_static_font *font, int *bad_range_count, int *bad_axis_count, int *bad_record_count);
int ghb_font_get_hvar_region_diagnostics(const ghb_static_font *font, ghb_hvar_region_diag *diag);
ghb_u16 ghb_font_get_glyph_flags(const ghb_static_font *font, ghb_u16 gid);
int ghb_buffer_scan_indic_syllables(ghb_buffer *b);
int ghb_font_has_script_lang(const ghb_static_font *font, ghb_u32 script, ghb_u32 language);
int ghb_insert_dotted_circle_if_needed(const ghb_static_font *font, ghb_buffer *b);
int ghb_classify_indic_matra(ghb_u32 script, ghb_u32 unicode);
int ghb_font_classify_indic_matra(const ghb_static_font *font, ghb_u32 script, ghb_u32 unicode);
int ghb_is_thai_lao_mark_unicode(ghb_u32 unicode);
int ghb_classify_thai_lao_mark(ghb_u32 unicode);
int ghb_font_classify_thai_lao_mark(const ghb_static_font *font, ghb_u32 unicode);
int ghb_buffer_dump_trace_numeric(const ghb_buffer *b, ghb_trace_dump_func fn, void *user);
int ghb_buffer_dump_trace_events_numeric(const ghb_buffer *b, ghb_trace_numeric_func fn, void *user, ghb_u32 event_mask);
int ghb_buffer_copy_trace_overlay(const ghb_buffer *b, ghb_trace_overlay_record *out_records, int max_records, ghb_u32 stage_mask);
int ghb_buffer_copy_trace_overlay_events(const ghb_buffer *b, ghb_trace_overlay_record *out_records, int max_records, ghb_u32 event_mask);
int ghb_buffer_copy_trace_overlay_compact(const ghb_buffer *b, ghb_trace_overlay_compact *out_records, int max_records, ghb_u32 event_mask);
int ghb_buffer_cluster_needs_dotted(const ghb_buffer *b, ghb_u8 syllable);
ghb_u8 ghb_buffer_get_syllable_status(const ghb_buffer *b, ghb_u8 syllable);
ghb_u8 ghb_buffer_get_syllable_status_counts(const ghb_buffer *b, ghb_u8 syllable, int *base_count, int *mark_count);
ghb_u8 ghb_buffer_get_cluster_status(const ghb_buffer *b, ghb_u8 syllable);
const char *ghb_cluster_status_name(ghb_u8 status);
int ghb_buffer_cluster_has_base(const ghb_buffer *b, ghb_u8 syllable);

#ifdef __cplusplus
}
#endif

#endif
