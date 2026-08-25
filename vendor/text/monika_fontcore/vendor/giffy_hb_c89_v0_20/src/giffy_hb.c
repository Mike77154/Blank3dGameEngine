#include "giffy_hb.h"

static int ghb_tag_default_on(ghb_u32 tag)
{
    if (tag == GHB_FEATURE_CCMP) return 1;
    if (tag == GHB_FEATURE_LIGA) return 1;
    if (tag == GHB_FEATURE_CLIG) return 1;
    if (tag == GHB_FEATURE_RLIG) return 1;
    if (tag == GHB_FEATURE_KERN) return 1;
    if (tag == GHB_FEATURE_DIST) return 1;
    if (tag == GHB_FEATURE_MARK) return 1;
    if (tag == GHB_FEATURE_MKMK) return 1;
    if (tag == GHB_FEATURE_CALT) return 1;
    if (tag == GHB_FEATURE_INIT) return 1;
    if (tag == GHB_FEATURE_MEDI) return 1;
    if (tag == GHB_FEATURE_FINA) return 1;
    if (tag == GHB_FEATURE_ISOL) return 1;
    if (tag == GHB_FEATURE_ABVS) return 1;
    if (tag == GHB_FEATURE_BLWS) return 1;
    if (tag == GHB_FEATURE_HALN) return 1;
    if (tag == GHB_FEATURE_PRES) return 1;
    if (tag == GHB_FEATURE_PSTS) return 1;
    if (tag == GHB_FEATURE_NUKT) return 1;
    if (tag == GHB_FEATURE_AKHN) return 1;
    if (tag == GHB_FEATURE_RPHF) return 1;
    if (tag == GHB_FEATURE_HALF) return 1;
    if (tag == GHB_FEATURE_VATU) return 1;
    if (tag == GHB_FEATURE_CJCT) return 1;
    if (tag == GHB_FEATURE_BLWF) return 1;
    if (tag == GHB_FEATURE_PSTF) return 1;
    return 0;
}

void ghb_buffer_init(ghb_buffer *b)
{
    int i;
    if (!b) return;
    b->len = 0;
    b->direction = GHB_DIR_LTR;
    b->script = GHB_SCRIPT_DFLT;
    b->language = GHB_LANG_DFLT;
    b->variation_count = 0;
    b->trace_func = 0;
    b->trace_user = 0;
    b->trace_records = 0;
    b->trace_record_capacity = 0;
    b->trace_record_count = 0;
    b->trace_record_head = 0;
    b->trace_stage_mask = 0xffffffffUL;
    for (i = 0; i < 4; ++i) b->variations[i] = 0;
    for (i = 0; i < GHB_MAX_GLYPHS; ++i) {
        b->info[i].codepoint = 0;
        b->info[i].cluster = 0;
        b->info[i].unicode = 0;
        b->info[i].mask = 0;
        b->info[i].glyph_flags = 0;
        b->info[i].syllable = 0;
        b->info[i].syllable_kind = GHB_SYLL_NONE;
        b->pos[i].x_advance = 0;
        b->pos[i].y_advance = 0;
        b->pos[i].x_offset = 0;
        b->pos[i].y_offset = 0;
    }
}

void ghb_buffer_clear(ghb_buffer *b)
{
    ghb_u32 script;
    ghb_u32 lang;
    int dir;
    ghb_f26d6 vars[4];
    ghb_trace_func tf;
    void *tu;
    ghb_trace_record *tr;
    int trcap, trcnt, trhead;
    ghb_u32 tmask;
    int vc;
    int i;
    if (!b) return;
    script = b->script;
    lang = b->language;
    dir = b->direction;
    vc = b->variation_count;
    tf = b->trace_func;
    tu = b->trace_user;
    tr = b->trace_records; trcap = b->trace_record_capacity; trcnt = b->trace_record_count; trhead = b->trace_record_head; tmask = b->trace_stage_mask;
    for (i = 0; i < 4; ++i) vars[i] = b->variations[i];
    ghb_buffer_init(b);
    b->script = script;
    b->language = lang;
    b->direction = dir;
    b->variation_count = vc;
    b->trace_func = tf;
    b->trace_user = tu;
    b->trace_records = tr; b->trace_record_capacity = trcap; b->trace_record_count = trcnt; b->trace_record_head = trhead; b->trace_stage_mask = tmask;
    for (i = 0; i < 4; ++i) b->variations[i] = vars[i];
}

void ghb_buffer_set_direction(ghb_buffer *b, int direction) { if (b) b->direction = direction; }
void ghb_buffer_set_script(ghb_buffer *b, ghb_u32 script) { if (b) b->script = script; }
void ghb_buffer_set_language(ghb_buffer *b, ghb_u32 language) { if (b) b->language = language; }
void ghb_buffer_set_trace(ghb_buffer *b, ghb_trace_func fn, void *user) { if (b) { b->trace_func = fn; b->trace_user = user; } }

#ifndef GHB_NO_TRACE
static int ghb_streq(const char *a, const char *b)
{
    int i;
    if (!a || !b) return 0;
    for (i = 0; a[i] || b[i]; ++i) if (a[i] != b[i]) return 0;
    return 1;
}

static ghb_u8 ghb_trace_stage_id(const char *stage)
{
    if (!stage) return GHB_TRACE_STAGE_NONE;
    if (ghb_streq(stage, "single")) return GHB_TRACE_STAGE_SINGLE;
    if (ghb_streq(stage, "ligature")) return GHB_TRACE_STAGE_LIGATURE;
    if (ghb_streq(stage, "context")) return GHB_TRACE_STAGE_CONTEXT;
    if (ghb_streq(stage, "context_class")) return GHB_TRACE_STAGE_CONTEXT_CLASS;
    if (ghb_streq(stage, "reverse")) return GHB_TRACE_STAGE_REVERSE;
    if (ghb_streq(stage, "context_pair")) return GHB_TRACE_STAGE_CONTEXT_PAIR;
    if (ghb_streq(stage, "kern")) return GHB_TRACE_STAGE_KERN;
    if (ghb_streq(stage, "mark_base")) return GHB_TRACE_STAGE_MARK_BASE;
    if (ghb_streq(stage, "mark_mark")) return GHB_TRACE_STAGE_MARK_MARK;
    if (ghb_streq(stage, "indic_reph")) return GHB_TRACE_STAGE_INDIC_REPH;
    if (ghb_streq(stage, "indic_half")) return GHB_TRACE_STAGE_INDIC_HALF;
    if (ghb_streq(stage, "hvar")) return GHB_TRACE_STAGE_HVAR;
    if (ghb_streq(stage, "dotted")) return GHB_TRACE_STAGE_DOTTED;
    if (ghb_streq(stage, "indic_matra")) return GHB_TRACE_STAGE_INDIC_MATRA;
    if (ghb_streq(stage, "script_lang")) return GHB_TRACE_STAGE_SCRIPT_LANG;
    if (ghb_streq(stage, "cluster")) return GHB_TRACE_STAGE_CLUSTER;
    if (ghb_streq(stage, "hvar_map")) return GHB_TRACE_STAGE_HVAR_MAP;
    if (ghb_streq(stage, "feature_inv")) return GHB_TRACE_STAGE_FEATURE_INV;
    if (ghb_streq(stage, "thai_lao")) return GHB_TRACE_STAGE_THAI_LAO;
    if (ghb_streq(stage, "hvar_validate")) return GHB_TRACE_STAGE_HVAR_VALIDATE;
    if (ghb_streq(stage, "syllable_status")) return GHB_TRACE_STAGE_SYLLABLE_STATUS;
    if (ghb_streq(stage, "hvar_region")) return GHB_TRACE_STAGE_HVAR_REGION;
    if (ghb_streq(stage, "context_pair_apply")) return GHB_TRACE_STAGE_CONTEXT_PAIR_APPLY;
    if (ghb_streq(stage, "overlay")) return GHB_TRACE_STAGE_OVERLAY;
    if (ghb_streq(stage, "hvar_explain")) return GHB_TRACE_STAGE_HVAR_EXPLAIN;
    if (ghb_streq(stage, "region_validate")) return GHB_TRACE_STAGE_REGION_VALIDATE;
    if (ghb_streq(stage, "context_pair_emit")) return GHB_TRACE_STAGE_CONTEXT_PAIR_EMIT;
    return GHB_TRACE_STAGE_NONE;
}

#endif

const char *ghb_trace_stage_name(int stage)
{
#ifdef GHB_NO_TRACE_NAMES
    (void)stage;
    return "";
#else
    switch (stage) {
    case GHB_TRACE_STAGE_SINGLE: return "single";
    case GHB_TRACE_STAGE_LIGATURE: return "ligature";
    case GHB_TRACE_STAGE_CONTEXT: return "context";
    case GHB_TRACE_STAGE_CONTEXT_CLASS: return "context_class";
    case GHB_TRACE_STAGE_REVERSE: return "reverse";
    case GHB_TRACE_STAGE_CONTEXT_PAIR: return "context_pair";
    case GHB_TRACE_STAGE_KERN: return "kern";
    case GHB_TRACE_STAGE_MARK_BASE: return "mark_base";
    case GHB_TRACE_STAGE_MARK_MARK: return "mark_mark";
    case GHB_TRACE_STAGE_INDIC_REPH: return "indic_reph";
    case GHB_TRACE_STAGE_INDIC_HALF: return "indic_half";
    case GHB_TRACE_STAGE_HVAR: return "hvar";
    case GHB_TRACE_STAGE_DOTTED: return "dotted";
    case GHB_TRACE_STAGE_INDIC_MATRA: return "indic_matra";
    case GHB_TRACE_STAGE_SCRIPT_LANG: return "script_lang";
    case GHB_TRACE_STAGE_CLUSTER: return "cluster";
    case GHB_TRACE_STAGE_HVAR_MAP: return "hvar_map";
    case GHB_TRACE_STAGE_FEATURE_INV: return "feature_inv";
    case GHB_TRACE_STAGE_THAI_LAO: return "thai_lao";
    case GHB_TRACE_STAGE_HVAR_VALIDATE: return "hvar_validate";
    case GHB_TRACE_STAGE_SYLLABLE_STATUS: return "syllable_status";
    case GHB_TRACE_STAGE_HVAR_REGION: return "hvar_region";
    case GHB_TRACE_STAGE_CONTEXT_PAIR_APPLY: return "context_pair_apply";
    case GHB_TRACE_STAGE_OVERLAY: return "overlay";
    case GHB_TRACE_STAGE_HVAR_EXPLAIN: return "hvar_explain";
    case GHB_TRACE_STAGE_REGION_VALIDATE: return "region_validate";
    case GHB_TRACE_STAGE_CONTEXT_PAIR_EMIT: return "context_pair_emit";
    default: return "none";
    }
#endif
}

int ghb_trace_event_id(int stage)
{
    switch (stage) {
    case GHB_TRACE_STAGE_SINGLE: return GHB_EVENT_SUB_SINGLE;
    case GHB_TRACE_STAGE_LIGATURE: return GHB_EVENT_SUB_LIGATURE;
    case GHB_TRACE_STAGE_CONTEXT:
    case GHB_TRACE_STAGE_CONTEXT_CLASS:
    case GHB_TRACE_STAGE_REVERSE: return GHB_EVENT_CONTEXT_MATCH;
    case GHB_TRACE_STAGE_CONTEXT_PAIR:
    case GHB_TRACE_STAGE_CONTEXT_PAIR_APPLY:
    case GHB_TRACE_STAGE_CONTEXT_PAIR_EMIT: return GHB_EVENT_CONTEXT_PAIR;
    case GHB_TRACE_STAGE_KERN: return GHB_EVENT_KERN_PAIR;
    case GHB_TRACE_STAGE_MARK_BASE:
    case GHB_TRACE_STAGE_MARK_MARK: return GHB_EVENT_MARK_ATTACH;
    case GHB_TRACE_STAGE_INDIC_REPH:
    case GHB_TRACE_STAGE_INDIC_HALF:
    case GHB_TRACE_STAGE_INDIC_MATRA:
    case GHB_TRACE_STAGE_CLUSTER:
    case GHB_TRACE_STAGE_SYLLABLE_STATUS: return GHB_EVENT_CLUSTER_STATUS;
    case GHB_TRACE_STAGE_HVAR:
    case GHB_TRACE_STAGE_HVAR_MAP:
    case GHB_TRACE_STAGE_HVAR_VALIDATE:
    case GHB_TRACE_STAGE_HVAR_EXPLAIN: return GHB_EVENT_HVAR_EXPLAIN;
    case GHB_TRACE_STAGE_HVAR_REGION: return GHB_EVENT_HVAR_REGION;
    case GHB_TRACE_STAGE_DOTTED: return GHB_EVENT_DOTTED_CIRCLE;
    case GHB_TRACE_STAGE_THAI_LAO: return GHB_EVENT_THAI_LAO_MARK;
    case GHB_TRACE_STAGE_FEATURE_INV: return GHB_EVENT_FEATURE_INVENTORY;
    case GHB_TRACE_STAGE_REGION_VALIDATE: return GHB_EVENT_REGION_VALIDATE;
    default: return GHB_EVENT_NONE;
    }
}

void ghb_buffer_set_trace_ring(ghb_buffer *b, ghb_trace_record *records, int capacity)
{
    int i;
    if (!b) return;
    b->trace_records = records;
    b->trace_record_capacity = capacity > 0 ? capacity : 0;
    b->trace_record_count = 0;
    b->trace_record_head = 0;
    if (records && capacity > 0) {
        for (i = 0; i < capacity; ++i) {
            records[i].stage = 0; records[i].index = 0; records[i].gid = 0; records[i].out_gid = 0;
        }
    }
}

void ghb_buffer_set_trace_stage_mask(ghb_buffer *b, ghb_u32 stage_mask)
{
    if (!b) return;
    b->trace_stage_mask = stage_mask;
}

ghb_u32 ghb_trace_stage_mask(int stage)
{
    if (stage < 0 || stage >= 32) return 0UL;
    return (ghb_u32)(1UL << stage);
}

void ghb_buffer_set_trace_stage_enabled(ghb_buffer *b, int stage, int enabled)
{
    ghb_u32 bit;
    if (!b) return;
    bit = ghb_trace_stage_mask(stage);
    if (!bit) return;
    if (enabled) b->trace_stage_mask |= bit;
    else b->trace_stage_mask &= ~bit;
}

int ghb_buffer_get_trace_ring_count(const ghb_buffer *b)
{
    if (!b) return 0;
    return b->trace_record_count;
}

int ghb_buffer_copy_trace_ring(const ghb_buffer *b, ghb_trace_record *out_records, int max_records)
{
    int n, i, src;
    if (!b || !out_records || max_records <= 0 || !b->trace_records || b->trace_record_capacity <= 0) return 0;
    n = b->trace_record_count;
    if (n > max_records) n = max_records;
    src = b->trace_record_head - b->trace_record_count;
    while (src < 0) src += b->trace_record_capacity;
    for (i = 0; i < n; ++i) {
        out_records[i] = b->trace_records[(src + i) % b->trace_record_capacity];
    }
    return n;
}

int ghb_buffer_copy_trace_ring_stage(const ghb_buffer *b, ghb_trace_record *out_records, int max_records, int stage)
{
    int i, src, total, n;
    ghb_trace_record rec;
    if (!b || !out_records || max_records <= 0 || !b->trace_records || b->trace_record_capacity <= 0) return 0;
    total = b->trace_record_count;
    src = b->trace_record_head - total;
    while (src < 0) src += b->trace_record_capacity;
    n = 0;
    for (i = 0; i < total && n < max_records; ++i) {
        rec = b->trace_records[(src + i) % b->trace_record_capacity];
        if ((int)rec.stage == stage) out_records[n++] = rec;
    }
    return n;
}



int ghb_buffer_copy_trace_ring_mask(const ghb_buffer *b, ghb_trace_record *out_records, int max_records, ghb_u32 stage_mask)
{
    int i, src, total, n;
    ghb_trace_record rec;
    ghb_u32 bit;
    if (!b || !out_records || max_records <= 0 || !b->trace_records || b->trace_record_capacity <= 0) return 0;
    total = b->trace_record_count;
    src = b->trace_record_head - total;
    while (src < 0) src += b->trace_record_capacity;
    n = 0;
    for (i = 0; i < total && n < max_records; ++i) {
        rec = b->trace_records[(src + i) % b->trace_record_capacity];
        bit = ghb_trace_stage_mask((int)rec.stage);
        if ((stage_mask & bit) != 0UL) out_records[n++] = rec;
    }
    return n;
}


int ghb_buffer_copy_trace_overlay(const ghb_buffer *b, ghb_trace_overlay_record *out_records, int max_records, ghb_u32 stage_mask)
{
    int i, src, total, n;
    ghb_trace_record rec;
    ghb_u32 bit;
    if (!b || !out_records || max_records <= 0 || !b->trace_records || b->trace_record_capacity <= 0) return 0;
    total = b->trace_record_count;
    src = b->trace_record_head - total;
    while (src < 0) src += b->trace_record_capacity;
    n = 0;
    for (i = 0; i < total && n < max_records; ++i) {
        rec = b->trace_records[(src + i) % b->trace_record_capacity];
        bit = ghb_trace_stage_mask((int)rec.stage);
        if ((stage_mask & bit) == 0UL) continue;
        out_records[n].stage = rec.stage;
        out_records[n].event_id = (ghb_u8)ghb_trace_event_id((int)rec.stage);
        out_records[n].map_kind = (ghb_u8)((rec.out_gid >> 12) & 0x000fu);
        out_records[n].index = rec.index;
        out_records[n].gid = rec.gid;
        out_records[n].value = rec.out_gid;
        n++;
    }
    return n;
}


int ghb_buffer_copy_trace_overlay_events(const ghb_buffer *b, ghb_trace_overlay_record *out_records, int max_records, ghb_u32 event_mask)
{
    int i;
    int src;
    int total;
    int n;
    ghb_trace_record rec;
    ghb_u32 bit;
    if (!b || !out_records || max_records <= 0 || !b->trace_records || b->trace_record_capacity <= 0) return 0;
    total = b->trace_record_count;
    src = b->trace_record_head - total;
    while (src < 0) src += b->trace_record_capacity;
    n = 0;
    for (i = 0; i < total && n < max_records; ++i) {
        rec = b->trace_records[(src + i) % b->trace_record_capacity];
        bit = ghb_trace_stage_mask(ghb_trace_event_id((int)rec.stage));
        if ((event_mask & bit) == 0UL) continue;
        out_records[n].stage = rec.stage;
        out_records[n].event_id = (ghb_u8)ghb_trace_event_id((int)rec.stage);
        out_records[n].map_kind = (ghb_u8)((rec.out_gid >> 12) & 0x000fu);
        out_records[n].index = rec.index;
        out_records[n].gid = rec.gid;
        out_records[n].value = rec.out_gid;
        n++;
    }
    return n;
}


int ghb_buffer_copy_trace_overlay_compact(const ghb_buffer *b, ghb_trace_overlay_compact *out_records, int max_records, ghb_u32 event_mask)
{
    int i;
    int src;
    int total;
    int n;
    ghb_trace_record rec;
    ghb_u8 eid;
    ghb_u32 bit;
    if (!b || !out_records || max_records <= 0 || !b->trace_records || b->trace_record_capacity <= 0) return 0;
    total = b->trace_record_count;
    src = b->trace_record_head - total;
    while (src < 0) src += b->trace_record_capacity;
    n = 0;
    for (i = 0; i < total && n < max_records; ++i) {
        rec = b->trace_records[(src + i) % b->trace_record_capacity];
        eid = (ghb_u8)ghb_trace_event_id((int)rec.stage);
        bit = ghb_trace_stage_mask((int)eid);
        if ((event_mask & bit) == 0UL) continue;
        out_records[n].event_id = eid;
        out_records[n].flags = (ghb_u8)(((rec.out_gid >> 12) & 0x0f) | ((rec.stage & 0x0f) << 4));
        out_records[n].index = rec.index;
        out_records[n].a = rec.gid;
        out_records[n].b = rec.out_gid;
        n++;
    }
    return n;
}

int ghb_buffer_dump_trace_events_numeric(const ghb_buffer *b, ghb_trace_numeric_func fn, void *user, ghb_u32 event_mask)
{
    int i;
    int src;
    int total;
    int n;
    ghb_trace_record rec;
    ghb_u8 eid;
    ghb_u32 bit;
    if (!b || !fn || !b->trace_records || b->trace_record_capacity <= 0) return GHB_ERR_BAD_ARG;
    total = b->trace_record_count;
    src = b->trace_record_head - total;
    while (src < 0) src += b->trace_record_capacity;
    n = 0;
    for (i = 0; i < total; ++i) {
        rec = b->trace_records[(src + i) % b->trace_record_capacity];
        eid = (ghb_u8)ghb_trace_event_id((int)rec.stage);
        bit = ghb_trace_stage_mask((int)eid);
        if ((event_mask & bit) == 0UL) continue;
        fn(user, n, rec.stage, eid, rec.index, rec.gid, rec.out_gid);
        n++;
    }
    return n;
}

int ghb_buffer_dump_trace_records(const ghb_buffer *b, ghb_trace_dump_func fn, void *user)
{
    int i;
    if (!b || !fn) return GHB_ERR_BAD_ARG;
    for (i = 0; i < b->len; ++i) {
        fn(user, i, (ghb_u16)b->info[i].codepoint, b->info[i].unicode,
           b->pos[i].x_advance, b->pos[i].x_offset, b->pos[i].y_offset,
           b->info[i].glyph_flags, b->info[i].syllable, b->info[i].syllable_kind);
    }
    return GHB_OK;
}

static void ghb_trace(const ghb_buffer *b, const char *stage, int index, ghb_u16 gid, ghb_u16 out_gid)
{
#ifndef GHB_NO_TRACE
    ghb_buffer *w;
    ghb_u8 sid;
    ghb_u32 bit;
    sid = ghb_trace_stage_id(stage);
    bit = sid < 32 ? (1UL << sid) : 0UL;
    if (b && bit && (b->trace_stage_mask & bit) == 0UL) return;
    if (b && b->trace_func) b->trace_func(b->trace_user, stage, index, gid, out_gid);
    if (b && b->trace_records && b->trace_record_capacity > 0) {
        w = (ghb_buffer *)b;
        w->trace_records[w->trace_record_head].stage = sid;
        w->trace_records[w->trace_record_head].index = (ghb_s16)index;
        w->trace_records[w->trace_record_head].gid = gid;
        w->trace_records[w->trace_record_head].out_gid = out_gid;
        w->trace_record_head++;
        if (w->trace_record_head >= w->trace_record_capacity) w->trace_record_head = 0;
        if (w->trace_record_count < w->trace_record_capacity) w->trace_record_count++;
    }
#else
    (void)b; (void)stage; (void)index; (void)gid; (void)out_gid;
#endif
}

ghb_u32 ghb_guess_script_from_text(const ghb_u32 *text, int count)
{
    int i;
    ghb_u32 cp;
    if (!text || count <= 0) return GHB_SCRIPT_DFLT;
    for (i = 0; i < count; ++i) {
        cp = text[i];
        if ((cp >= 0x0600UL && cp <= 0x06ffUL) || (cp >= 0x0750UL && cp <= 0x077fUL) || (cp >= 0x08a0UL && cp <= 0x08ffUL)) return GHB_SCRIPT_ARAB;
        if (cp >= 0x0900UL && cp <= 0x097fUL) return GHB_SCRIPT_DEVA;
        if (cp >= 0x0980UL && cp <= 0x09ffUL) return GHB_SCRIPT_BENG;
        if (cp >= 0x0a00UL && cp <= 0x0a7fUL) return GHB_SCRIPT_GURU;
        if (cp >= 0x0a80UL && cp <= 0x0affUL) return GHB_SCRIPT_GUJR;
        if (cp >= 0x0b80UL && cp <= 0x0bffUL) return GHB_SCRIPT_TAML;
        if (cp >= 0x0c00UL && cp <= 0x0c7fUL) return GHB_SCRIPT_TELU;
        if (cp >= 0x0c80UL && cp <= 0x0cffUL) return GHB_SCRIPT_KNDA;
        if (cp >= 0x0d00UL && cp <= 0x0d7fUL) return GHB_SCRIPT_MLYM;
        if (cp >= 0x0d80UL && cp <= 0x0dffUL) return GHB_SCRIPT_SINH;
        if (cp >= 0x0e00UL && cp <= 0x0e7fUL) return GHB_SCRIPT_THAI;
        if (cp >= 0x0e80UL && cp <= 0x0effUL) return GHB_SCRIPT_LAO;
    }
    return GHB_SCRIPT_LATN;
}


void ghb_buffer_set_script_language_from_text(const ghb_static_font *font, ghb_buffer *b)
{
    ghb_u32 text[GHB_MAX_GLYPHS];
    int i;
    ghb_u32 script;
    if (!b) return;
    for (i = 0; i < b->len && i < GHB_MAX_GLYPHS; ++i) text[i] = b->info[i].unicode;
    script = ghb_guess_script_from_text(text, b->len);
    b->script = script;
    ghb_trace(b, "script_lang", 0, (ghb_u16)(script >> 16), (ghb_u16)(script & 0xffffu));
    if (font && ghb_font_has_script_lang(font, script, b->language)) return;
    if (font && ghb_font_has_script_lang(font, script, GHB_LANG_DFLT)) { b->language = GHB_LANG_DFLT; return; }
    if (font && ghb_font_has_script_lang(font, GHB_SCRIPT_DFLT, GHB_LANG_DFLT)) { b->script = GHB_SCRIPT_DFLT; b->language = GHB_LANG_DFLT; return; }
    if (b->language == 0) b->language = GHB_LANG_DFLT;
}

void ghb_buffer_set_variation(ghb_buffer *b, int axis_index, ghb_f26d6 coord_26d6)
{
    if (!b) return;
    if (axis_index < 0 || axis_index >= 4) return;
    b->variations[axis_index] = coord_26d6;
    if (axis_index + 1 > b->variation_count) b->variation_count = axis_index + 1;
}

int ghb_font_has_script_lang(const ghb_static_font *font, ghb_u32 script, ghb_u32 language)
{
    int i;
    if (!font || !font->script_langs || font->script_lang_count <= 0) return 1;
    for (i = 0; i < font->script_lang_count; ++i) {
        if (font->script_langs[i].script_tag == script &&
            (font->script_langs[i].lang_tag == language || font->script_langs[i].lang_tag == GHB_LANG_DFLT)) return 1;
    }
    return 0;
}

int ghb_buffer_add_utf32(ghb_buffer *b, const ghb_u32 *text, int count)
{
    int i;
    if (!b || !text || count < 0) return GHB_ERR_BAD_ARG;
    if (b->len + count > GHB_MAX_GLYPHS) return GHB_ERR_OVERFLOW;
    for (i = 0; i < count; ++i) {
        b->info[b->len].codepoint = text[i];
        b->info[b->len].unicode = text[i];
        b->info[b->len].cluster = (ghb_u32)i;
        b->info[b->len].mask = 0xffffffffUL;
        b->info[b->len].glyph_flags = 0;
        b->info[b->len].syllable = 0;
        b->info[b->len].syllable_kind = GHB_SYLL_NONE;
        b->pos[b->len].x_advance = 0;
        b->pos[b->len].y_advance = 0;
        b->pos[b->len].x_offset = 0;
        b->pos[b->len].y_offset = 0;
        b->len++;
    }
    return GHB_OK;
}

static int ghb_decode_utf8_one(const unsigned char *s, int n, ghb_u32 *cp, int *used)
{
    ghb_u32 c;
    if (n <= 0) return GHB_ERR_TRUNCATED_UTF8;
    if (s[0] < 0x80u) { *cp = (ghb_u32)s[0]; *used = 1; return GHB_OK; }
    if ((s[0] & 0xe0u) == 0xc0u) {
        if (n < 2) return GHB_ERR_TRUNCATED_UTF8;
        if ((s[1] & 0xc0u) != 0x80u) return GHB_ERR_BAD_UTF8;
        c = ((ghb_u32)(s[0] & 0x1fu) << 6) | (ghb_u32)(s[1] & 0x3fu);
        if (c < 0x80UL) return GHB_ERR_BAD_UTF8;
        *cp = c; *used = 2; return GHB_OK;
    }
    if ((s[0] & 0xf0u) == 0xe0u) {
        if (n < 3) return GHB_ERR_TRUNCATED_UTF8;
        if ((s[1] & 0xc0u) != 0x80u || (s[2] & 0xc0u) != 0x80u) return GHB_ERR_BAD_UTF8;
        c = ((ghb_u32)(s[0] & 0x0fu) << 12) | ((ghb_u32)(s[1] & 0x3fu) << 6) | (ghb_u32)(s[2] & 0x3fu);
        if (c < 0x800UL) return GHB_ERR_BAD_UTF8;
        if (c >= 0xd800UL && c <= 0xdfffUL) return GHB_ERR_BAD_UTF8;
        *cp = c; *used = 3; return GHB_OK;
    }
    if ((s[0] & 0xf8u) == 0xf0u) {
        if (n < 4) return GHB_ERR_TRUNCATED_UTF8;
        if ((s[1] & 0xc0u) != 0x80u || (s[2] & 0xc0u) != 0x80u || (s[3] & 0xc0u) != 0x80u) return GHB_ERR_BAD_UTF8;
        c = ((ghb_u32)(s[0] & 0x07u) << 18) | ((ghb_u32)(s[1] & 0x3fu) << 12) | ((ghb_u32)(s[2] & 0x3fu) << 6) | (ghb_u32)(s[3] & 0x3fu);
        if (c < 0x10000UL || c > 0x10ffffUL) return GHB_ERR_BAD_UTF8;
        *cp = c; *used = 4; return GHB_OK;
    }
    return GHB_ERR_BAD_UTF8;
}

int ghb_buffer_add_utf8(ghb_buffer *b, const char *text, int byte_count)
{
    int i, used, rc, n;
    ghb_u32 cp;
    const unsigned char *s;
    if (!b || !text) return GHB_ERR_BAD_ARG;
    if (byte_count < 0) { byte_count = 0; while (text[byte_count] != '\0') byte_count++; }
    i = 0; s = (const unsigned char *)text;
    while (i < byte_count) {
        if (b->len >= GHB_MAX_GLYPHS) return GHB_ERR_OVERFLOW;
        n = byte_count - i;
        rc = ghb_decode_utf8_one(s + i, n, &cp, &used);
        if (rc != GHB_OK) return rc;
        b->info[b->len].codepoint = cp;
        b->info[b->len].unicode = cp;
        b->info[b->len].cluster = (ghb_u32)i;
        b->info[b->len].mask = 0xffffffffUL;
        b->info[b->len].glyph_flags = 0;
        b->info[b->len].syllable = 0;
        b->info[b->len].syllable_kind = GHB_SYLL_NONE;
        b->pos[b->len].x_advance = 0;
        b->pos[b->len].y_advance = 0;
        b->pos[b->len].x_offset = 0;
        b->pos[b->len].y_offset = 0;
        b->len++;
        i += used;
    }
    return GHB_OK;
}

int ghb_feature_enabled(const ghb_feature *features, int feature_count, ghb_u32 tag, int glyph_index)
{
    int i;
    int enabled = ghb_tag_default_on(tag);
    if (!features || feature_count <= 0) return enabled;
    for (i = 0; i < feature_count; ++i) {
        if (features[i].tag == tag) {
            if ((features[i].start < 0 || glyph_index >= features[i].start) &&
                (features[i].end < 0 || glyph_index < features[i].end)) enabled = features[i].value ? 1 : 0;
        }
    }
    return enabled;
}

ghb_u16 ghb_font_get_glyph(const ghb_static_font *font, ghb_u32 unicode)
{
    int lo, hi, mid;
    ghb_u32 v;
    if (!font || !font->cmap || font->cmap_count <= 0) return 0;
    lo = 0; hi = font->cmap_count - 1;
    while (lo <= hi) {
        mid = lo + ((hi - lo) / 2);
        v = font->cmap[mid].unicode;
        if (v == unicode) return font->cmap[mid].gid;
        if (v < unicode) lo = mid + 1; else hi = mid - 1;
    }
    return font->missing_gid;
}

ghb_f26d6 ghb_font_get_advance(const ghb_static_font *font, ghb_u16 gid)
{
    int lo, hi, mid;
    ghb_u16 v;
    if (!font || !font->advances || font->advance_count <= 0) return 0;
    lo = 0; hi = font->advance_count - 1;
    while (lo <= hi) {
        mid = lo + ((hi - lo) / 2);
        v = font->advances[mid].gid;
        if (v == gid) return font->advances[mid].advance_26d6;
        if (v < gid) lo = mid + 1; else hi = mid - 1;
    }
    return font->default_advance_26d6;
}


static ghb_f26d6 ghb_region_support(const ghb_var_region *r, ghb_f26d6 coord)
{
    ghb_f26d6 den;
    if (!r) return 0;
    if (r->start_coord == r->peak_coord && r->peak_coord == r->end_coord) return 0;
    if (coord == r->peak_coord) return GHB_F26D6_ONE;
    if (coord <= r->start_coord || coord >= r->end_coord) return 0;
    if (coord < r->peak_coord) {
        den = r->peak_coord - r->start_coord;
        if (den == 0) return 0;
        return GHB_F26D6_DIV(coord - r->start_coord, den);
    }
    den = r->end_coord - r->peak_coord;
    if (den == 0) return 0;
    return GHB_F26D6_DIV(r->end_coord - coord, den);
}

int ghb_font_get_hvar_item_index(const ghb_static_font *font, ghb_u16 gid, ghb_u16 *item_index)
{
    int i;
    if (!font || !item_index) return 0;
    if (font->hvar_advance_maps) {
        for (i = 0; i < font->hvar_advance_map_count; ++i) {
            if (font->hvar_advance_maps[i].gid == gid) { *item_index = font->hvar_advance_maps[i].item_index; return 1; }
        }
    }
    if (font->hvar_advance_map_segments) {
        for (i = 0; i < font->hvar_advance_map_segment_count; ++i) {
            if (gid >= font->hvar_advance_map_segments[i].first_gid && gid <= font->hvar_advance_map_segments[i].last_gid) {
                *item_index = (ghb_u16)(font->hvar_advance_map_segments[i].first_item_index + (gid - font->hvar_advance_map_segments[i].first_gid));
                return 1;
            }
        }
    }
    return 0;
}


int ghb_font_validate_hvar_gid_item_map(const ghb_static_font *font, ghb_u16 gid, ghb_u16 *item_index, ghb_u8 *map_kind)
{
    int i;
    if (item_index) *item_index = 0;
    if (map_kind) *map_kind = GHB_HVAR_MAP_NONE;
    if (!font) return 0;
    if (font->hvar_advance_maps) {
        int lo, hi, mid;
        lo = 0;
        hi = font->hvar_advance_map_count - 1;
        while (lo <= hi) {
            mid = lo + ((hi - lo) / 2);
            if (font->hvar_advance_maps[mid].gid == gid) {
                if (item_index) *item_index = font->hvar_advance_maps[mid].item_index;
                if (map_kind) *map_kind = GHB_HVAR_MAP_SPARSE;
                return 1;
            }
            if (font->hvar_advance_maps[mid].gid < gid) lo = mid + 1;
            else hi = mid - 1;
        }
    }
    if (font->hvar_advance_map_segments) {
        for (i = 0; i < font->hvar_advance_map_segment_count; ++i) {
            const ghb_hvar_advance_map_segment *seg;
            seg = &font->hvar_advance_map_segments[i];
            if (gid >= seg->first_gid && gid <= seg->last_gid) {
                if (item_index) *item_index = (ghb_u16)(seg->first_item_index + (gid - seg->first_gid));
                if (map_kind) *map_kind = GHB_HVAR_MAP_SEGMENT;
                return 1;
            }
        }
    }
    return 0;
}

int ghb_font_validate_hvar_gid_range(const ghb_static_font *font, ghb_u16 first_gid, ghb_u16 last_gid, int *mapped_count, int *unmapped_count, int *sparse_count, int *segment_count)
{
    ghb_u16 gid;
    ghb_u16 item;
    ghb_u8 kind;
    int mapped, unmapped, sparse, segment;
    if (!font || first_gid > last_gid) return GHB_ERR_BAD_ARG;
    mapped = unmapped = sparse = segment = 0;
    for (gid = first_gid; ; ++gid) {
        if (ghb_font_validate_hvar_gid_item_map(font, gid, &item, &kind)) {
            (void)item;
            mapped++;
            if (kind == GHB_HVAR_MAP_SPARSE) sparse++;
            else if (kind == GHB_HVAR_MAP_SEGMENT) segment++;
        } else {
            unmapped++;
        }
        if (gid == last_gid) break;
    }
    if (mapped_count) *mapped_count = mapped;
    if (unmapped_count) *unmapped_count = unmapped;
    if (sparse_count) *sparse_count = sparse;
    if (segment_count) *segment_count = segment;
    return GHB_OK;
}


int ghb_font_validate_hvar_item_index(const ghb_static_font *font, ghb_u16 item_index, int *delta_count, int *bad_region_count)
{
    int i;
    int dc;
    int br;
    if (delta_count) *delta_count = 0;
    if (bad_region_count) *bad_region_count = 0;
    if (!font) return GHB_ERR_BAD_ARG;
    dc = 0;
    br = 0;
    if (font->hvar_item_deltas) {
        for (i = 0; i < font->hvar_item_delta_count; ++i) {
            if (font->hvar_item_deltas[i].item_index == item_index) {
                dc++;
                if ((int)font->hvar_item_deltas[i].region_index >= font->hvar_region_record_count) br++;
            }
        }
    }
    if (delta_count) *delta_count = dc;
    if (bad_region_count) *bad_region_count = br;
    return (dc > 0 && br == 0) ? GHB_OK : GHB_ERR_UNSUPPORTED;
}

int ghb_font_get_hvar_contributions(const ghb_static_font *font, const ghb_buffer *b, ghb_u16 gid, ghb_hvar_contribution *out_items, int max_items, ghb_f26d6 *total_delta)
{
    ghb_u16 item_index;
    ghb_u8 map_kind;
    int d;
    int rr;
    int ra;
    int n;
    ghb_f26d6 total;
    ghb_f26d6 support;
    ghb_f26d6 part;
    const ghb_var_region_record *rec;
    ghb_u8 ax;
    if (total_delta) *total_delta = 0;
    if (!font || !b) return GHB_ERR_BAD_ARG;
    if (!ghb_font_validate_hvar_gid_item_map(font, gid, &item_index, &map_kind)) return GHB_ERR_UNSUPPORTED;
    n = 0;
    total = 0;
    if (!font->hvar_item_deltas || !font->hvar_region_records || !font->var_regions) return GHB_ERR_UNSUPPORTED;
    for (d = 0; d < font->hvar_item_delta_count; ++d) {
        if (font->hvar_item_deltas[d].item_index != item_index) continue;
        rr = (int)font->hvar_item_deltas[d].region_index;
        if (rr < 0 || rr >= font->hvar_region_record_count) continue;
        rec = &font->hvar_region_records[rr];
        support = GHB_F26D6_ONE;
        for (ra = 0; ra < (int)rec->region_axis_count; ++ra) {
            int idx;
            idx = (int)rec->first_region_axis + ra;
            if (idx < 0 || idx >= font->var_region_count) { support = 0; break; }
            ax = font->var_regions[idx].axis_index;
            if ((int)ax >= b->variation_count) { support = 0; break; }
            part = ghb_region_support(&font->var_regions[idx], b->variations[ax]);
            support = GHB_F26D6_MUL(support, part);
        }
        part = GHB_F26D6_MUL((ghb_f26d6)font->hvar_item_deltas[d].delta_26d6, support);
        total += part;
        if (out_items && n < max_items) {
            out_items[n].gid = gid;
            out_items[n].item_index = item_index;
            out_items[n].region_index = (ghb_u16)rr;
            out_items[n].map_kind = map_kind;
            out_items[n].delta_26d6 = font->hvar_item_deltas[d].delta_26d6;
            out_items[n].support_26d6 = support;
            out_items[n].contribution_26d6 = part;
        }
        n++;
    }
    if (total_delta) *total_delta = total;
    return n;
}


int ghb_font_explain_hvar_gid(const ghb_static_font *font, const ghb_buffer *b, ghb_u16 gid, ghb_hvar_explain_record *out_record)
{
    ghb_u16 item_index;
    ghb_u8 map_kind;
    int dc;
    int br;
    int ret;
    ghb_f26d6 total;
    if (!font || !b || !out_record) return GHB_ERR_BAD_ARG;
    out_record->gid = gid;
    out_record->item_index = 0;
    out_record->map_kind = GHB_HVAR_MAP_NONE;
    out_record->valid_item = 0;
    out_record->region_count = 0;
    out_record->bad_region_count = 0;
    out_record->total_delta_26d6 = 0;
    if (!ghb_font_validate_hvar_gid_item_map(font, gid, &item_index, &map_kind)) return GHB_ERR_UNSUPPORTED;
    out_record->item_index = item_index;
    out_record->map_kind = map_kind;
    ret = ghb_font_validate_hvar_item_index(font, item_index, &dc, &br);
    out_record->valid_item = (ret == GHB_OK) ? 1u : 0u;
    out_record->region_count = (ghb_u8)((dc > 255) ? 255 : dc);
    out_record->bad_region_count = (ghb_u8)((br > 255) ? 255 : br);
    if (ghb_font_get_hvar_contributions(font, b, gid, 0, 0, &total) >= 0) out_record->total_delta_26d6 = total;
    return GHB_OK;
}

int ghb_font_validate_hvar_region_ranges(const ghb_static_font *font, int *bad_range_count, int *bad_axis_count, int *bad_record_count)
{
    int i;
    int bad_range;
    int bad_axis;
    int bad_record;
    if (bad_range_count) *bad_range_count = 0;
    if (bad_axis_count) *bad_axis_count = 0;
    if (bad_record_count) *bad_record_count = 0;
    if (!font) return GHB_ERR_BAD_ARG;
    bad_range = 0;
    bad_axis = 0;
    bad_record = 0;
    for (i = 0; i < font->var_region_count; ++i) {
        const ghb_var_region *vr;
        vr = &font->var_regions[i];
        if (!(vr->start_coord <= vr->peak_coord && vr->peak_coord <= vr->end_coord)) bad_range++;
        if (font->var_axis_count > 0 && (int)vr->axis_index >= font->var_axis_count) bad_axis++;
    }
    for (i = 0; i < font->hvar_region_record_count; ++i) {
        int first;
        int last;
        first = (int)font->hvar_region_records[i].first_region_axis;
        last = first + (int)font->hvar_region_records[i].region_axis_count;
        if (first < 0 || last > font->var_region_count) bad_record++;
    }
    if (bad_range_count) *bad_range_count = bad_range;
    if (bad_axis_count) *bad_axis_count = bad_axis;
    if (bad_record_count) *bad_record_count = bad_record;
    return (bad_range == 0 && bad_axis == 0 && bad_record == 0) ? GHB_OK : GHB_ERR_UNSUPPORTED;
}


int ghb_font_get_hvar_region_diagnostics(const ghb_static_font *font, ghb_hvar_region_diag *diag)
{
    int i;
    if (!font || !diag) return GHB_ERR_BAD_ARG;
    diag->bad_range_count = 0;
    diag->bad_axis_count = 0;
    diag->bad_record_count = 0;
    diag->empty_record_count = 0;
    diag->max_axes_per_record = 0;
    (void)ghb_font_validate_hvar_region_ranges(font, &diag->bad_range_count, &diag->bad_axis_count, &diag->bad_record_count);
    for (i = 0; i < font->hvar_region_record_count; ++i) {
        int n;
        n = (int)font->hvar_region_records[i].region_axis_count;
        if (n == 0) diag->empty_record_count++;
        if (n > diag->max_axes_per_record) diag->max_axes_per_record = n;
    }
    if (diag->bad_range_count || diag->bad_axis_count || diag->bad_record_count) return GHB_ERR_UNSUPPORTED;
    return GHB_OK;
}

int ghb_font_get_hvar_diagnostics(const ghb_static_font *font, ghb_hvar_diag *diag)
{
    int i;
    ghb_u16 item;
    if (!font || !diag) return GHB_ERR_BAD_ARG;
    diag->sparse_map_count = font->hvar_advance_map_count;
    diag->segment_map_count = font->hvar_advance_map_segment_count;
    diag->item_delta_count = font->hvar_item_delta_count;
    diag->mapped_advance_count = 0;
    diag->unmapped_advance_count = 0;
    diag->lossy = 0;
    diag->lossless_map = 1;
    diag->mapped_by_sparse = 0;
    diag->mapped_by_segment = 0;
    diag->max_regions_per_item = 0;
    for (i = 0; i < font->advance_count; ++i) {
        if (font->hvar_advance_maps) {
            int j;
            for (j = 0; j < font->hvar_advance_map_count; ++j) {
                if (font->hvar_advance_maps[j].gid == font->advances[i].gid) { diag->mapped_by_sparse++; break; }
            }
        }
        if (font->hvar_advance_map_segments) {
            int j;
            for (j = 0; j < font->hvar_advance_map_segment_count; ++j) {
                if (font->advances[i].gid >= font->hvar_advance_map_segments[j].first_gid && font->advances[i].gid <= font->hvar_advance_map_segments[j].last_gid) { diag->mapped_by_segment++; break; }
            }
        }
        if (ghb_font_get_hvar_item_index(font, font->advances[i].gid, &item)) diag->mapped_advance_count++;
        else diag->unmapped_advance_count++;
    }
    if (font->hvar_item_deltas) {
        int ii, jj, cnt;
        for (ii = 0; ii < font->hvar_item_delta_count; ++ii) {
            cnt = 0;
            for (jj = 0; jj < font->hvar_item_delta_count; ++jj) {
                if (font->hvar_item_deltas[jj].item_index == font->hvar_item_deltas[ii].item_index) cnt++;
            }
            if (cnt > diag->max_regions_per_item) diag->max_regions_per_item = cnt;
        }
    }
    if (diag->unmapped_advance_count > 0 && font->hvar_item_delta_count > 0) diag->lossless_map = 0;
    if (font->hvar_item_delta_count > 0 && font->hvar_region_record_count == 0) diag->lossy = 1;
    if (font->hvar_advance_delta_count > 0 && font->hvar_item_delta_count == 0) diag->lossy = 1;
    return GHB_OK;
}

static ghb_f26d6 ghb_font_get_var_advance(const ghb_static_font *font, const ghb_buffer *b, ghb_u16 gid)
{
    ghb_f26d6 adv;
    int i;
    ghb_u8 ax;
    ghb_f26d6 coord;
    ghb_f26d6 maxv;
    ghb_f26d6 defv;
    ghb_f26d6 t;
    adv = ghb_font_get_advance(font, gid);
    if (!font || !b) return adv;
    /* v0.8: HVAR ItemVariationStore-style region blending. */
    if (font->hvar_advance_deltas && font->hvar_region_records && font->var_regions) {
        int h, rr, ra;
        ghb_f26d6 support;
        ghb_f26d6 part;
        const ghb_var_region_record *rec;
        for (h = 0; h < font->hvar_advance_delta_count; ++h) {
            if (font->hvar_advance_deltas[h].gid != gid) continue;
            rr = (int)font->hvar_advance_deltas[h].region_index;
            if (rr < 0 || rr >= font->hvar_region_record_count) continue;
            rec = &font->hvar_region_records[rr];
            support = GHB_F26D6_ONE;
            for (ra = 0; ra < (int)rec->region_axis_count; ++ra) {
                int idx = (int)rec->first_region_axis + ra;
                if (idx < 0 || idx >= font->var_region_count) { support = 0; break; }
                ax = font->var_regions[idx].axis_index;
                if ((int)ax >= b->variation_count) { support = 0; break; }
                part = ghb_region_support(&font->var_regions[idx], b->variations[ax]);
                support = GHB_F26D6_MUL(support, part);
            }
            adv += GHB_F26D6_MUL((ghb_f26d6)font->hvar_advance_deltas[h].delta_26d6, support);
        }
    }
    /* v0.11: explicit VarStore item path: gid -> item_index via sparse map or range map -> region deltas. */
    if (font->hvar_item_deltas && font->hvar_region_records && font->var_regions) {
        int d, rr, ra;
        ghb_u16 item_index;
        ghb_f26d6 support;
        ghb_f26d6 part;
        const ghb_var_region_record *rec;
        ghb_u8 map_kind;
        if (ghb_font_validate_hvar_gid_item_map(font, gid, &item_index, &map_kind)) {
            int item_delta_count;
            int bad_region_count;
            item_delta_count = 0;
            bad_region_count = 0;
            (void)ghb_font_validate_hvar_item_index(font, item_index, &item_delta_count, &bad_region_count);
            ghb_trace(b, "hvar_validate", -1, gid, (ghb_u16)(((ghb_u16)map_kind << 12) | (item_index & 0x0fffu)));
            if (bad_region_count != 0) ghb_trace(b, "hvar_map", -1, gid, (ghb_u16)bad_region_count);
            for (d = 0; d < font->hvar_item_delta_count; ++d) {
                if (font->hvar_item_deltas[d].item_index != item_index) continue;
                rr = (int)font->hvar_item_deltas[d].region_index;
                if (rr < 0 || rr >= font->hvar_region_record_count) continue;
                rec = &font->hvar_region_records[rr];
                support = GHB_F26D6_ONE;
                for (ra = 0; ra < (int)rec->region_axis_count; ++ra) {
                    int idx = (int)rec->first_region_axis + ra;
                    if (idx < 0 || idx >= font->var_region_count) { support = 0; break; }
                    ax = font->var_regions[idx].axis_index;
                    if ((int)ax >= b->variation_count) { support = 0; break; }
                    part = ghb_region_support(&font->var_regions[idx], b->variations[ax]);
                    support = GHB_F26D6_MUL(support, part);
                }
                {
                    ghb_f26d6 contrib;
                    contrib = GHB_F26D6_MUL((ghb_f26d6)font->hvar_item_deltas[d].delta_26d6, support);
                    adv += contrib;
                    ghb_trace(b, "hvar_region", -1, gid, (ghb_u16)(((rr & 0x00ff) << 8) | ((contrib >> 6) & 0x00ff)));
                }
            }
            ghb_trace(b, "hvar", -1, gid, item_index);
        }
    }
    if (!font->var_axes || !font->var_advance_deltas) return adv;
    for (i = 0; i < font->var_advance_delta_count; ++i) {
        if (font->var_advance_deltas[i].gid != gid) continue;
        ax = font->var_advance_deltas[i].axis_index;
        if ((int)ax >= b->variation_count) continue;
        if (font->var_regions && (int)ax < font->var_region_count) {
            coord = b->variations[font->var_regions[ax].axis_index];
            t = ghb_region_support(&font->var_regions[ax], coord);
            adv += GHB_F26D6_MUL((ghb_f26d6)font->var_advance_deltas[i].delta_at_max_26d6, t);
        } else {
            if ((int)ax >= font->var_axis_count) continue;
            coord = b->variations[ax];
            defv = font->var_axes[ax].default_value;
            maxv = font->var_axes[ax].max_value;
            if (coord <= defv || maxv == defv) continue;
            t = GHB_F26D6_DIV(coord - defv, maxv - defv);
            adv += GHB_F26D6_MUL((ghb_f26d6)font->var_advance_deltas[i].delta_at_max_26d6, t);
        }
    }
    return adv;
}

ghb_f26d6 ghb_font_get_advance_var(const ghb_static_font *font, const ghb_buffer *b, ghb_u16 gid)
{
    return ghb_font_get_var_advance(font, b, gid);
}

ghb_u16 ghb_font_get_glyph_flags(const ghb_static_font *font, ghb_u16 gid)
{
    int lo, hi, mid;
    ghb_u16 v;
    if (!font || !font->glyph_classes || font->glyph_class_count <= 0) return GHB_GLYPH_BASE;
    lo = 0; hi = font->glyph_class_count - 1;
    while (lo <= hi) {
        mid = lo + ((hi - lo) / 2);
        v = font->glyph_classes[mid].gid;
        if (v == gid) return font->glyph_classes[mid].flags;
        if (v < gid) lo = mid + 1; else hi = mid - 1;
    }
    return GHB_GLYPH_BASE;
}


static const ghb_mark_meta *ghb_font_get_mark_meta(const ghb_static_font *font, ghb_u16 gid)
{
    int lo, hi, mid;
    ghb_u16 v;
    if (!font || !font->mark_metas || font->mark_meta_count <= 0) return 0;
    lo = 0; hi = font->mark_meta_count - 1;
    while (lo <= hi) {
        mid = lo + ((hi - lo) / 2);
        v = font->mark_metas[mid].gid;
        if (v == gid) return &font->mark_metas[mid];
        if (v < gid) lo = mid + 1; else hi = mid - 1;
    }
    return 0;
}

static int ghb_lookup_allows(ghb_u16 flags, ghb_u16 glyph_flags)
{
    if ((flags & GHB_LOOKUP_IGNORE_BASE_GLYPHS) && (glyph_flags & GHB_GLYPH_BASE)) return 0;
    if ((flags & GHB_LOOKUP_IGNORE_LIGATURES) && (glyph_flags & GHB_GLYPH_LIGATURE)) return 0;
    if ((flags & GHB_LOOKUP_IGNORE_MARKS) && (glyph_flags & GHB_GLYPH_MARK)) return 0;
    return 1;
}


static int ghb_lookup_allows_mark(const ghb_static_font *font, ghb_u16 flags, ghb_u16 gid, ghb_u16 glyph_flags)
{
    const ghb_mark_meta *m;
    ghb_u16 attach;
    ghb_u8 want_set;
    if (!ghb_lookup_allows(flags, glyph_flags)) return 0;
    if ((glyph_flags & GHB_GLYPH_MARK) == 0u) return 1;
    m = ghb_font_get_mark_meta(font, gid);
    if ((flags & GHB_LOOKUP_USE_MARK_FILTERING) != 0u) {
        if (!m) return 0;
        /* Low byte of lookup_flags is used as a compact mark filtering set id in static packs. */
        want_set = (ghb_u8)((flags & GHB_LOOKUP_MARK_FILTERING_SET_MASK) >> GHB_LOOKUP_MARK_FILTERING_SET_SHIFT);
        if (m->mark_set != want_set && m->mark_set != GHB_MARK_SET_ALL) return 0;
    }
    attach = (ghb_u16)((flags & GHB_LOOKUP_MARK_ATTACHMENT_TYPE_MASK) >> 8);
    if (attach != 0u) {
        if (!m) return 0;
        if (m->mark_attachment_type != attach) return 0;
    }
    return 1;
}

static void ghb_map_unicode_to_glyphs(const ghb_static_font *font, ghb_buffer *b)
{
    int i;
    ghb_u16 gid;
    for (i = 0; i < b->len; ++i) {
        gid = ghb_font_get_glyph(font, b->info[i].codepoint);
        b->info[i].codepoint = (ghb_u32)gid;
        b->info[i].glyph_flags = ghb_font_get_glyph_flags(font, gid);
    }
}

static void ghb_init_positions(const ghb_static_font *font, ghb_buffer *b)
{
    int i;
    ghb_u16 gid, flags;
    for (i = 0; i < b->len; ++i) {
        gid = (ghb_u16)b->info[i].codepoint;
        flags = b->info[i].glyph_flags;
        b->pos[i].x_advance = ((flags & GHB_GLYPH_MARK) != 0u) ? 0 : ghb_font_get_var_advance(font, b, gid);
        b->pos[i].y_advance = 0;
        b->pos[i].x_offset = 0;
        b->pos[i].y_offset = 0;
    }
}

static void ghb_reverse_buffer(ghb_buffer *b)
{
    int i = 0, j;
    ghb_glyph_info ti;
    ghb_glyph_pos tp;
    if (!b) return;
    j = b->len - 1;
    while (i < j) {
        ti = b->info[i]; tp = b->pos[i];
        b->info[i] = b->info[j]; b->pos[i] = b->pos[j];
        b->info[j] = ti; b->pos[j] = tp;
        i++; j--;
    }
}

static int ghb_is_transparent_unicode(ghb_u32 cp)
{
    if (cp >= 0x064bUL && cp <= 0x065fUL) return 1;
    if (cp == 0x0670UL) return 1;
    if (cp >= 0x06d6UL && cp <= 0x06edUL) return 1;
    return 0;
}

static int ghb_arab_join_type(ghb_u32 cp)
{
    if (ghb_is_transparent_unicode(cp)) return GHB_JOIN_TRANSPARENT;
    switch (cp) {
    case 0x0622UL: case 0x0623UL: case 0x0624UL: case 0x0625UL: case 0x0627UL:
    case 0x0629UL: case 0x062fUL: case 0x0630UL: case 0x0631UL: case 0x0632UL:
    case 0x0648UL: case 0x0671UL: case 0x0688UL: case 0x0691UL: case 0x06c2UL:
    case 0x06c3UL: case 0x06c4UL: case 0x06c5UL: case 0x06c6UL: case 0x06c7UL:
    case 0x06c8UL: case 0x06c9UL: case 0x06cbUL: case 0x06cdUL: case 0x06d2UL:
    case 0x06d3UL:
        return GHB_JOIN_RIGHT;
    default:
        break;
    }
    if ((cp >= 0x0626UL && cp <= 0x064aUL) || (cp >= 0x066eUL && cp <= 0x06d3UL)) return GHB_JOIN_DUAL;
    return GHB_JOIN_NONE;
}

static int ghb_prev_join_index(const ghb_buffer *b, int at)
{
    int i = at - 1;
    while (i >= 0) {
        if (ghb_arab_join_type(b->info[i].unicode) != GHB_JOIN_TRANSPARENT) return i;
        i--;
    }
    return -1;
}

static int ghb_next_join_index(const ghb_buffer *b, int at)
{
    int i = at + 1;
    while (i < b->len) {
        if (ghb_arab_join_type(b->info[i].unicode) != GHB_JOIN_TRANSPARENT) return i;
        i++;
    }
    return -1;
}

static void ghb_apply_one_single_feature(const ghb_static_font *font, ghb_buffer *b, int index, ghb_u32 feature_tag, const ghb_feature *features, int feature_count)
{
    int r;
    ghb_u16 gid;
    if (!font->single_subs) return;
    if (!ghb_feature_enabled(features, feature_count, feature_tag, index)) return;
    gid = (ghb_u16)b->info[index].codepoint;
    for (r = 0; r < font->single_sub_count; ++r) {
        if (font->single_subs[r].from_gid == gid && font->single_subs[r].feature_tag == feature_tag &&
            ghb_lookup_allows_mark(font, font->single_subs[r].lookup_flags, gid, b->info[index].glyph_flags)) {
            b->info[index].codepoint = font->single_subs[r].to_gid;
            b->info[index].glyph_flags = ghb_font_get_glyph_flags(font, font->single_subs[r].to_gid);
            return;
        }
    }
}

static void ghb_apply_arabic_joining(const ghb_static_font *font, ghb_buffer *b, const ghb_feature *features, int feature_count)
{
    int i, p, n, jt, pjt, njt;
    int joins_prev, joins_next;
    ghb_u32 tag;
    if (b->script != GHB_SCRIPT_ARAB) return;
    for (i = 0; i < b->len; ++i) {
        jt = ghb_arab_join_type(b->info[i].unicode);
        if (jt == GHB_JOIN_NONE || jt == GHB_JOIN_TRANSPARENT) continue;
        p = ghb_prev_join_index(b, i);
        n = ghb_next_join_index(b, i);
        joins_prev = 0; joins_next = 0;
        if (p >= 0) {
            pjt = ghb_arab_join_type(b->info[p].unicode);
            if ((pjt == GHB_JOIN_DUAL) && (jt == GHB_JOIN_DUAL || jt == GHB_JOIN_RIGHT)) joins_prev = 1;
        }
        if (n >= 0) {
            njt = ghb_arab_join_type(b->info[n].unicode);
            if ((jt == GHB_JOIN_DUAL) && (njt == GHB_JOIN_DUAL || njt == GHB_JOIN_RIGHT)) joins_next = 1;
        }
        if (joins_prev && joins_next) tag = GHB_FEATURE_MEDI;
        else if (joins_prev) tag = GHB_FEATURE_FINA;
        else if (joins_next) tag = GHB_FEATURE_INIT;
        else tag = GHB_FEATURE_ISOL;
        ghb_apply_one_single_feature(font, b, i, tag, features, feature_count);
    }
}

static void ghb_apply_single_subs(const ghb_static_font *font, ghb_buffer *b, const ghb_feature *features, int feature_count)
{
    int i, r;
    ghb_u16 gid;
    if (!font->single_subs) return;
    for (i = 0; i < b->len; ++i) {
        gid = (ghb_u16)b->info[i].codepoint;
        for (r = 0; r < font->single_sub_count; ++r) {
            if (font->single_subs[r].from_gid == gid &&
                ghb_lookup_allows_mark(font, font->single_subs[r].lookup_flags, gid, b->info[i].glyph_flags) &&
                ghb_feature_enabled(features, feature_count, font->single_subs[r].feature_tag, i)) {
                /* Arabic joining features are picked by ghb_apply_arabic_joining, not blanket-applied here. */
                if (font->single_subs[r].feature_tag == GHB_FEATURE_INIT || font->single_subs[r].feature_tag == GHB_FEATURE_MEDI ||
                    font->single_subs[r].feature_tag == GHB_FEATURE_FINA || font->single_subs[r].feature_tag == GHB_FEATURE_ISOL) continue;
                ghb_trace(b, "single", i, gid, font->single_subs[r].to_gid);
                b->info[i].codepoint = font->single_subs[r].to_gid;
                b->info[i].glyph_flags = ghb_font_get_glyph_flags(font, font->single_subs[r].to_gid);
                break;
            }
        }
    }
}

static int ghb_lig_matches(const ghb_ligature *lig, const ghb_buffer *b, int at)
{
    int k;
    if (lig->component_count < 2) return 0;
    if (at + (int)lig->component_count > b->len) return 0;
    for (k = 0; k < (int)lig->component_count; ++k) if ((ghb_u16)b->info[at + k].codepoint != lig->components[k]) return 0;
    return 1;
}

static void ghb_delete_range_after_lig(ghb_buffer *b, int at, int count_removed)
{
    int src = at + 1 + count_removed;
    int dst = at + 1;
    while (src < b->len) { b->info[dst] = b->info[src]; b->pos[dst] = b->pos[src]; dst++; src++; }
    b->len -= count_removed;
}

static void ghb_apply_ligatures(const ghb_static_font *font, ghb_buffer *b, const ghb_feature *features, int feature_count)
{
    int i = 0, r, best, removed;
    ghb_u8 best_count;
    if (!font->ligatures) return;
    while (i < b->len) {
        best = -1; best_count = 0;
        for (r = 0; r < font->ligature_count; ++r) {
            if (font->ligatures[r].component_count > best_count &&
                ghb_feature_enabled(features, feature_count, font->ligatures[r].feature_tag, i) &&
                ghb_lookup_allows_mark(font, font->ligatures[r].lookup_flags, (ghb_u16)b->info[i].codepoint, b->info[i].glyph_flags) &&
                ghb_lig_matches(&font->ligatures[r], b, i)) { best = r; best_count = font->ligatures[r].component_count; }
        }
        if (best >= 0) {
            removed = (int)font->ligatures[best].component_count - 1;
            ghb_trace(b, "ligature", i, (ghb_u16)b->info[i].codepoint, font->ligatures[best].lig_gid);
            b->info[i].codepoint = font->ligatures[best].lig_gid;
            b->info[i].glyph_flags = ghb_font_get_glyph_flags(font, font->ligatures[best].lig_gid) | GHB_GLYPH_LIGATURE;
            ghb_delete_range_after_lig(b, i, removed);
        }
        i++;
    }
}

static int ghb_match_context_side(const ghb_buffer *b, int start, const ghb_u16 *pat, int count, int forward)
{
    int k;
    if (count <= 0) return 1;
    for (k = 0; k < count; ++k) {
        int idx = forward ? start + k : start - k;
        if (idx < 0 || idx >= b->len) return 0;
        if (pat[k] != GHB_CONTEXT_ANY_GLYPH && (ghb_u16)b->info[idx].codepoint != pat[k]) return 0;
    }
    return 1;
}

static int ghb_class_range_has(const ghb_static_font *font, ghb_u16 class_id, ghb_u16 gid)
{
    int i;
    if (!font || !font->context_class_ranges) return 0;
    for (i = 0; i < font->context_class_range_count; ++i) {
        if (font->context_class_ranges[i].class_id == class_id &&
            gid >= font->context_class_ranges[i].first_gid && gid <= font->context_class_ranges[i].last_gid) return 1;
    }
    return 0;
}

static int ghb_match_context_token(const ghb_static_font *font, ghb_u16 token, ghb_u16 gid)
{
    if (token == GHB_CONTEXT_ANY_GLYPH) return 1;
    if ((token & 0x8000u) != 0u) return ghb_class_range_has(font, (ghb_u16)(token & 0x7fffu), gid);
    return token == gid;
}

static int ghb_match_context_class_side(const ghb_static_font *font, const ghb_buffer *b, int start, const ghb_u16 *pat, int count, int forward)
{
    int k;
    if (count <= 0) return 1;
    for (k = 0; k < count; ++k) {
        int idx = forward ? start + k : start - k;
        if (idx < 0 || idx >= b->len) return 0;
        if (!ghb_match_context_token(font, pat[k], (ghb_u16)b->info[idx].codepoint)) return 0;
    }
    return 1;
}

static void ghb_apply_context_subs(const ghb_static_font *font, ghb_buffer *b, const ghb_feature *features, int feature_count)
{
    int i, r, repl;
    const ghb_context_sub *c;
    if (!font->context_subs) return;
    for (i = 0; i < b->len; ++i) {
        for (r = 0; r < font->context_sub_count; ++r) {
            c = &font->context_subs[r];
            if (!ghb_feature_enabled(features, feature_count, c->feature_tag, i)) continue;
            if (c->input_count == 0) continue;
            if (i + (int)c->input_count > b->len) continue;
            if (!ghb_match_context_side(b, i, c->input, c->input_count, 1)) continue;
            if (!ghb_match_context_side(b, i - 1, c->backtrack, c->backtrack_count, 0)) continue;
            if (!ghb_match_context_side(b, i + (int)c->input_count, c->lookahead, c->lookahead_count, 1)) continue;
            repl = i + (int)c->replace_index;
            if (repl < i || repl >= i + (int)c->input_count) continue;
            if (!ghb_lookup_allows_mark(font, c->lookup_flags, (ghb_u16)b->info[repl].codepoint, b->info[repl].glyph_flags)) continue;
            ghb_trace(b, "context", repl, (ghb_u16)b->info[repl].codepoint, c->to_gid);
            b->info[repl].codepoint = c->to_gid;
            b->info[repl].glyph_flags = ghb_font_get_glyph_flags(font, c->to_gid);
        }
    }
}


static void ghb_apply_context_class_subs(const ghb_static_font *font, ghb_buffer *b, const ghb_feature *features, int feature_count)
{
    int i, r, target;
    const ghb_context_class_sub *c;
    ghb_u16 gid;
    if (!font || !font->context_class_subs) return;
    for (i = 0; i < b->len; ++i) {
        for (r = 0; r < font->context_class_sub_count; ++r) {
            c = &font->context_class_subs[r];
            if (!ghb_feature_enabled(features, feature_count, c->feature_tag, i)) continue;
            if (!ghb_match_context_class_side(font, b, i, c->input, c->input_count, 1)) continue;
            if (!ghb_match_context_class_side(font, b, i - 1, c->backtrack, c->backtrack_count, 0)) continue;
            if (!ghb_match_context_class_side(font, b, i + (int)c->input_count, c->lookahead, c->lookahead_count, 1)) continue;
            target = i + (int)c->replace_index;
            if (target < 0 || target >= b->len) continue;
            gid = (ghb_u16)b->info[target].codepoint;
            if (!ghb_lookup_allows_mark(font, c->lookup_flags, gid, b->info[target].glyph_flags)) continue;
            ghb_trace(b, "context_class", target, gid, c->to_gid);
            b->info[target].codepoint = c->to_gid;
            b->info[target].glyph_flags = ghb_font_get_glyph_flags(font, c->to_gid);
        }
    }
}

static void ghb_apply_reverse_context_subs(const ghb_static_font *font, ghb_buffer *b, const ghb_feature *features, int feature_count)
{
    int i, r;
    const ghb_reverse_context_sub *c;
    if (!font->reverse_context_subs) return;
    for (i = b->len - 1; i >= 0; --i) {
        for (r = 0; r < font->reverse_context_sub_count; ++r) {
            c = &font->reverse_context_subs[r];
            if (!ghb_feature_enabled(features, feature_count, c->feature_tag, i)) continue;
            if ((ghb_u16)b->info[i].codepoint != c->input_gid) continue;
            if (!ghb_match_context_side(b, i - 1, c->backtrack, c->backtrack_count, 0)) continue;
            if (!ghb_match_context_side(b, i + 1, c->lookahead, c->lookahead_count, 1)) continue;
            if (!ghb_lookup_allows_mark(font, c->lookup_flags, (ghb_u16)b->info[i].codepoint, b->info[i].glyph_flags)) continue;
            ghb_trace(b, "reverse", i, (ghb_u16)b->info[i].codepoint, c->to_gid);
            b->info[i].codepoint = c->to_gid;
            b->info[i].glyph_flags = ghb_font_get_glyph_flags(font, c->to_gid);
        }
    }
}

static int ghb_prev_non_mark(const ghb_buffer *b, int at)
{
    int j = at - 1;
    while (j >= 0) { if ((b->info[j].glyph_flags & GHB_GLYPH_MARK) == 0u) return j; j--; }
    return -1;
}
static int ghb_prev_mark(const ghb_buffer *b, int at)
{
    int j = at - 1;
    while (j >= 0) { if ((b->info[j].glyph_flags & GHB_GLYPH_MARK) != 0u) return j; if ((b->info[j].glyph_flags & GHB_GLYPH_MARK) == 0u) return -1; j--; }
    return -1;
}
static int ghb_next_non_mark(const ghb_buffer *b, int at)
{
    int j = at + 1;
    while (j < b->len) { if ((b->info[j].glyph_flags & GHB_GLYPH_MARK) == 0u) return j; j++; }
    return -1;
}


static void ghb_apply_context_pair_adjusts(const ghb_static_font *font, ghb_buffer *b, const ghb_feature *features, int feature_count)
{
    int i, r, left, right;
    const ghb_context_pair_adjust *c;
    ghb_u16 lgid, rgid;
    if (!font || !font->context_pair_adjusts) return;
    for (i = 0; i < b->len; ++i) {
        for (r = 0; r < font->context_pair_adjust_count; ++r) {
            c = &font->context_pair_adjusts[r];
            if (c->input_count == 0) continue;
            if (!ghb_feature_enabled(features, feature_count, c->feature_tag, i)) continue;
            if (!ghb_match_context_class_side(font, b, i, c->input, c->input_count, 1)) continue;
            if (!ghb_match_context_class_side(font, b, i - 1, c->backtrack, c->backtrack_count, 0)) continue;
            if (!ghb_match_context_class_side(font, b, i + (int)c->input_count, c->lookahead, c->lookahead_count, 1)) continue;
            left = i + (int)c->left_index;
            right = i + (int)c->right_index;
            if (left < 0 || left >= b->len || right < 0 || right >= b->len) continue;
            lgid = (ghb_u16)b->info[left].codepoint;
            rgid = (ghb_u16)b->info[right].codepoint;
            if (!ghb_lookup_allows_mark(font, c->lookup_flags, lgid, b->info[left].glyph_flags)) continue;
            if (!ghb_lookup_allows_mark(font, c->lookup_flags, rgid, b->info[right].glyph_flags)) continue;
            b->pos[left].x_advance += c->x_advance_delta_26d6;
            ghb_trace(b, "context_pair", left, lgid, rgid);
            ghb_trace(b, "context_pair_apply", right, rgid, (ghb_u16)c->x_advance_delta_26d6);
        }
    }
}

static void ghb_apply_kern(const ghb_static_font *font, ghb_buffer *b, const ghb_feature *features, int feature_count)
{
    int i, r, j;
    ghb_u16 left, right;
    if (!font->kern_pairs) return;
    for (i = 0; i < b->len; ++i) {
        if ((b->info[i].glyph_flags & GHB_GLYPH_MARK) != 0u) continue;
        j = ghb_next_non_mark(b, i); if (j < 0) continue;
        left = (ghb_u16)b->info[i].codepoint; right = (ghb_u16)b->info[j].codepoint;
        for (r = 0; r < font->kern_pair_count; ++r) {
            if (font->kern_pairs[r].left_gid == left && font->kern_pairs[r].right_gid == right &&
                ghb_lookup_allows_mark(font, font->kern_pairs[r].lookup_flags, left, b->info[i].glyph_flags) &&
                ghb_feature_enabled(features, feature_count, font->kern_pairs[r].feature_tag, i)) {
                b->pos[i].x_advance += font->kern_pairs[r].x_advance_delta_26d6;
                ghb_trace(b, "kern", i, left, right);
                break;
            }
        }
    }
}

static void ghb_apply_mark_to_base(const ghb_static_font *font, ghb_buffer *b, const ghb_feature *features, int feature_count)
{
    int i, r, base_index;
    ghb_u16 base, mark;
    if (!font->mark_anchors) return;
    for (i = 0; i < b->len; ++i) {
        if ((b->info[i].glyph_flags & GHB_GLYPH_MARK) == 0u) continue;
        base_index = ghb_prev_non_mark(b, i); if (base_index < 0) continue;
        base = (ghb_u16)b->info[base_index].codepoint; mark = (ghb_u16)b->info[i].codepoint;
        for (r = 0; r < font->mark_anchor_count; ++r) {
            if (font->mark_anchors[r].base_gid == base && font->mark_anchors[r].mark_gid == mark &&
                ghb_lookup_allows_mark(font, font->mark_anchors[r].lookup_flags, mark, b->info[i].glyph_flags) &&
                ghb_feature_enabled(features, feature_count, font->mark_anchors[r].feature_tag, i)) {
                b->pos[i].x_advance = 0;
                ghb_trace(b, "mark_base", i, base, mark);
                b->pos[i].x_offset = font->mark_anchors[r].x_offset_26d6;
                b->pos[i].y_offset = font->mark_anchors[r].y_offset_26d6;
                break;
            }
        }
    }
}

static void ghb_apply_mark_to_mark(const ghb_static_font *font, ghb_buffer *b, const ghb_feature *features, int feature_count)
{
    int i, r, base_mark_index;
    ghb_u16 base_mark, mark;
    if (!font->mark_mark_anchors) return;
    for (i = 0; i < b->len; ++i) {
        if ((b->info[i].glyph_flags & GHB_GLYPH_MARK) == 0u) continue;
        base_mark_index = ghb_prev_mark(b, i); if (base_mark_index < 0) continue;
        base_mark = (ghb_u16)b->info[base_mark_index].codepoint; mark = (ghb_u16)b->info[i].codepoint;
        for (r = 0; r < font->mark_mark_anchor_count; ++r) {
            if (font->mark_mark_anchors[r].base_mark_gid == base_mark && font->mark_mark_anchors[r].mark_gid == mark &&
                ghb_lookup_allows_mark(font, font->mark_mark_anchors[r].lookup_flags, mark, b->info[i].glyph_flags) &&
                ghb_feature_enabled(features, feature_count, font->mark_mark_anchors[r].feature_tag, i)) {
                b->pos[i].x_advance = 0;
                ghb_trace(b, "mark_mark", i, base_mark, mark);
                b->pos[i].x_offset = b->pos[base_mark_index].x_offset + font->mark_mark_anchors[r].x_offset_26d6;
                b->pos[i].y_offset = b->pos[base_mark_index].y_offset + font->mark_mark_anchors[r].y_offset_26d6;
                break;
            }
        }
    }
}


static int ghb_is_indic_script(ghb_u32 script);

static int ghb_is_universal_leading_mark(ghb_u32 cp)
{
    if (cp >= 0x0300UL && cp <= 0x036fUL) return 1;
    if (cp >= 0x0591UL && cp <= 0x05bdUL) return 1;
    if (cp >= 0x064bUL && cp <= 0x065fUL) return 1;
    if (cp >= 0x0900UL && cp <= 0x0dffUL) {
        if ((cp & 0x00ffUL) >= 0x01UL && (cp & 0x00ffUL) <= 0x57UL) return 1;
    }
    if (cp >= 0x0e31UL && cp <= 0x0e4eUL) return 1;
    if (cp >= 0x0eb1UL && cp <= 0x0ecdUL) return 1;
    return 0;
}

static int ghb_insert_dotted_circle_at(const ghb_static_font *font, ghb_buffer *b, int at)
{
    int i;
    ghb_u16 dot_gid;
    if (!font || !b || at < 0 || at > b->len || b->len >= GHB_MAX_GLYPHS) return GHB_OK;
    dot_gid = ghb_font_get_glyph(font, GHB_DOTTED_CIRCLE_UNICODE);
    if (dot_gid == 0 || dot_gid == font->missing_gid) return GHB_OK;
    for (i = b->len; i > at; --i) { b->info[i] = b->info[i - 1]; b->pos[i] = b->pos[i - 1]; }
    b->len++;
    b->info[at].codepoint = (ghb_u32)dot_gid;
    b->info[at].unicode = GHB_DOTTED_CIRCLE_UNICODE;
    b->info[at].cluster = (at + 1 < b->len) ? b->info[at + 1].cluster : 0;
    b->info[at].mask = 0;
    b->info[at].glyph_flags = ghb_font_get_glyph_flags(font, dot_gid) | GHB_GLYPH_BASE;
    b->info[at].syllable = 0;
    b->info[at].syllable_kind = GHB_SYLL_NONE;
    b->pos[at].x_advance = ghb_font_get_var_advance(font, b, dot_gid);
    b->pos[at].y_advance = 0;
    b->pos[at].x_offset = 0;
    b->pos[at].y_offset = 0;
    ghb_trace(b, "dotted", at, dot_gid, dot_gid);
    return GHB_OK;
}

int ghb_insert_dotted_circle_if_needed(const ghb_static_font *font, ghb_buffer *b)
{
    int i;
    int attachable_base_seen;
    ghb_u8 last_syllable;
    if (!font || !b || b->len <= 0) return GHB_OK;
    attachable_base_seen = 0;
    last_syllable = 0;
    for (i = 0; i < b->len; ++i) {
        if (ghb_is_indic_script(b->script) && b->info[i].syllable != last_syllable) {
            last_syllable = b->info[i].syllable;
            attachable_base_seen = 0;
        }
        if (!ghb_is_universal_leading_mark(b->info[i].unicode) &&
            (b->info[i].glyph_flags & GHB_GLYPH_MARK) == 0u) {
            attachable_base_seen = 1;
            continue;
        }
        if (!attachable_base_seen) {
            if (b->info[i].syllable != 0 && !ghb_buffer_cluster_needs_dotted(b, b->info[i].syllable)) {
                ghb_trace(b, "syllable_status", i, (ghb_u16)b->info[i].codepoint, (ghb_u16)((ghb_buffer_get_syllable_status(b, b->info[i].syllable) << 8) | b->info[i].syllable));
                attachable_base_seen = 1;
                continue;
            }
            ghb_trace(b, "syllable_status", i, (ghb_u16)b->info[i].codepoint, (ghb_u16)((ghb_buffer_get_syllable_status(b, b->info[i].syllable) << 8) | b->info[i].syllable));
            ghb_insert_dotted_circle_at(font, b, i);
            attachable_base_seen = 1;
            last_syllable = b->info[i].syllable;
            i++;
        }
    }
    return GHB_OK;
}



int ghb_font_classify_indic_matra(const ghb_static_font *font, ghb_u32 script, ghb_u32 cp)
{
    int i;
    if (font && font->indic_matra_ranges) {
        for (i = 0; i < font->indic_matra_range_count; i++) {
            const ghb_indic_matra_range *r;
            r = &font->indic_matra_ranges[i];
            if ((r->script_tag == script || r->script_tag == 0UL) &&
                cp >= r->first_unicode && cp <= r->last_unicode) {
                return (int)r->category;
            }
        }
    }
    return ghb_classify_indic_matra(script, cp);
}

int ghb_is_thai_lao_mark_unicode(ghb_u32 cp)
{
    if (cp >= 0x0e31UL && cp <= 0x0e4eUL) return 1;
    if (cp >= 0x0eb1UL && cp <= 0x0ecdUL) return 1;
    return 0;
}

static int ghb_is_indic_mark_unicode(ghb_u32 cp);


ghb_u8 ghb_buffer_get_syllable_status(const ghb_buffer *b, ghb_u8 syllable)
{
    int i;
    ghb_u8 status;
    status = GHB_SYLL_STATUS_EMPTY;
    if (!b || syllable == 0) return status;
    for (i = 0; i < b->len; ++i) {
        if (b->info[i].syllable == syllable) {
            if ((b->info[i].glyph_flags & GHB_GLYPH_MARK) == 0u &&
                !ghb_is_indic_mark_unicode(b->info[i].unicode) &&
                !ghb_is_thai_lao_mark_unicode(b->info[i].unicode)) {
                status |= GHB_SYLL_STATUS_HAS_BASE;
            }
            if ((b->info[i].glyph_flags & GHB_GLYPH_MARK) != 0u ||
                ghb_is_indic_mark_unicode(b->info[i].unicode) ||
                ghb_is_thai_lao_mark_unicode(b->info[i].unicode)) {
                status |= GHB_SYLL_STATUS_MARK_ONLY;
            }
        }
    }
    if ((status & GHB_SYLL_STATUS_MARK_ONLY) != 0u && (status & GHB_SYLL_STATUS_HAS_BASE) == 0u) {
        status |= GHB_SYLL_STATUS_NEEDS_DOT;
        status |= GHB_SYLL_STATUS_BROKEN;
    } else if ((status & GHB_SYLL_STATUS_HAS_BASE) != 0u) {
        status |= GHB_SYLL_STATUS_VALID;
    }
    return status;
}


ghb_u8 ghb_buffer_get_syllable_status_counts(const ghb_buffer *b, ghb_u8 syllable, int *base_count, int *mark_count)
{
    int i;
    int bases;
    int marks;
    ghb_u8 status;
    bases = 0;
    marks = 0;
    if (!b || syllable == 0) {
        if (base_count) *base_count = 0;
        if (mark_count) *mark_count = 0;
        return GHB_SYLL_STATUS_EMPTY;
    }
    for (i = 0; i < b->len; ++i) {
        if (b->info[i].syllable == syllable) {
            if ((b->info[i].glyph_flags & GHB_GLYPH_MARK) != 0u ||
                ghb_is_indic_mark_unicode(b->info[i].unicode) ||
                ghb_is_thai_lao_mark_unicode(b->info[i].unicode)) marks++;
            else bases++;
        }
    }
    if (base_count) *base_count = bases;
    if (mark_count) *mark_count = marks;
    status = ghb_buffer_get_syllable_status(b, syllable);
    if (bases == 0 && marks > 0) status |= GHB_SYLL_STATUS_MARK_ONLY;
    if (bases > 0 && marks >= 0) status |= GHB_SYLL_STATUS_VALID;
    return status;
}

int ghb_buffer_cluster_has_base(const ghb_buffer *b, ghb_u8 syllable)
{
    int i;
    if (!b || syllable == 0) return 0;
    for (i = 0; i < b->len; ++i) {
        if (b->info[i].syllable == syllable && (b->info[i].glyph_flags & GHB_GLYPH_MARK) == 0u && !ghb_is_thai_lao_mark_unicode(b->info[i].unicode)) return 1;
    }
    return 0;
}


ghb_u8 ghb_buffer_get_cluster_status(const ghb_buffer *b, ghb_u8 syllable)
{
    ghb_u8 s;
    s = ghb_buffer_get_syllable_status(b, syllable);
    if ((s & GHB_SYLL_STATUS_NEEDS_DOT) != 0u) return GHB_CLUSTER_STATUS_NEEDS_DOT;
    if ((s & GHB_SYLL_STATUS_BROKEN) != 0u) return GHB_CLUSTER_STATUS_BROKEN;
    if ((s & GHB_SYLL_STATUS_MARK_ONLY) != 0u && (s & GHB_SYLL_STATUS_HAS_BASE) == 0u) return GHB_CLUSTER_STATUS_MARK_ONLY;
    if ((s & GHB_SYLL_STATUS_VALID) != 0u) return GHB_CLUSTER_STATUS_VALID;
    return GHB_CLUSTER_STATUS_EMPTY;
}

const char *ghb_cluster_status_name(ghb_u8 status)
{
#ifndef GHB_NO_TRACE_NAMES
    switch (status) {
    case GHB_CLUSTER_STATUS_EMPTY: return "empty";
    case GHB_CLUSTER_STATUS_VALID: return "valid";
    case GHB_CLUSTER_STATUS_MARK_ONLY: return "mark_only";
    case GHB_CLUSTER_STATUS_BROKEN: return "broken";
    case GHB_CLUSTER_STATUS_NEEDS_DOT: return "needs_dot";
    default: return "unknown";
    }
#else
    (void)status;
    return "";
#endif
}

int ghb_buffer_cluster_needs_dotted(const ghb_buffer *b, ghb_u8 syllable)
{
    return (ghb_buffer_get_syllable_status(b, syllable) & GHB_SYLL_STATUS_NEEDS_DOT) != 0u;
}

static int ghb_is_indic_script(ghb_u32 script)
{
    return script == GHB_SCRIPT_DEVA || script == GHB_SCRIPT_BENG || script == GHB_SCRIPT_GURU ||
           script == GHB_SCRIPT_GUJR || script == GHB_SCRIPT_TAML || script == GHB_SCRIPT_TELU ||
           script == GHB_SCRIPT_KNDA || script == GHB_SCRIPT_MLYM || script == GHB_SCRIPT_SINH;
}

static int ghb_is_indic_halant(ghb_u32 cp)
{
    switch (cp) {
    case 0x094dUL: case 0x09cdUL: case 0x0a4dUL: case 0x0acdUL:
    case 0x0b4dUL: case 0x0bcdUL: case 0x0c4dUL: case 0x0ccdUL:
    case 0x0d4dUL: case 0x0dcaUL:
        return 1;
    default:
        return 0;
    }
}

static int ghb_is_indic_mark_unicode(ghb_u32 cp)
{
    if (cp >= 0x0900UL && cp <= 0x0dffUL) {
        if (ghb_is_indic_halant(cp)) return 1;
        if ((cp & 0x00ffUL) >= 0x3eUL && (cp & 0x00ffUL) <= 0x57UL) return 1;
        if ((cp & 0x00ffUL) >= 0x01UL && (cp & 0x00ffUL) <= 0x03UL) return 1;
    }
    return 0;
}

int ghb_classify_indic_matra(ghb_u32 script, ghb_u32 cp)
{
    (void)script;
    switch (cp) {
    case 0x093fUL: case 0x09bfUL: case 0x0a3fUL: case 0x0abfUL:
    case 0x0b3fUL: case 0x0bbfUL: case 0x0c46UL: case 0x0c47UL:
    case 0x0cc6UL: case 0x0cc7UL: case 0x0d46UL: case 0x0d47UL:
        return GHB_MATRA_PRE;
    case 0x0947UL: case 0x0948UL: case 0x09c7UL: case 0x09c8UL:
    case 0x0a47UL: case 0x0a48UL: case 0x0ac7UL: case 0x0ac8UL:
        return GHB_MATRA_ABOVE;
    case 0x0941UL: case 0x0942UL: case 0x09c1UL: case 0x09c2UL:
    case 0x0a41UL: case 0x0a42UL: case 0x0ac1UL: case 0x0ac2UL:
    case 0x0bc1UL: case 0x0bc2UL: case 0x0c41UL: case 0x0c42UL:
    case 0x0cc1UL: case 0x0cc2UL: case 0x0d41UL: case 0x0d42UL:
        return GHB_MATRA_BELOW;
    case 0x093eUL: case 0x094bUL: case 0x094cUL: case 0x09beUL:
    case 0x09cbUL: case 0x09ccUL: case 0x0abeUL: case 0x0acbUL:
    case 0x0accUL: case 0x0bbeUL: case 0x0bc6UL: case 0x0bcaUL:
    case 0x0bcbUL: case 0x0bccUL: case 0x0cbeUL: case 0x0ccaUL:
    case 0x0ccbUL: case 0x0d3eUL: case 0x0d4aUL: case 0x0d4bUL:
    case 0x0d4cUL:
        return GHB_MATRA_POST;
    default:
        break;
    }
    return GHB_MATRA_NONE;
}

static int ghb_is_prebase_indic_vowel(ghb_u32 script, ghb_u32 cp)
{
    return ghb_classify_indic_matra(script, cp) == GHB_MATRA_PRE;
}

int ghb_buffer_scan_indic_syllables(ghb_buffer *b)
{
    int i;
    ghb_u8 serial;
    int have_base;
    if (!b) return GHB_ERR_BAD_ARG;
    serial = 1;
    have_base = 0;
    for (i = 0; i < b->len; ++i) {
        b->info[i].syllable = serial;
        if (ghb_is_indic_halant(b->info[i].unicode)) {
            b->info[i].syllable_kind = GHB_SYLL_INDIC_HALANT;
            have_base = 1;
        } else if ((b->info[i].glyph_flags & GHB_GLYPH_MARK) != 0u || ghb_is_indic_mark_unicode(b->info[i].unicode)) {
            b->info[i].syllable_kind = GHB_SYLL_INDIC_MARK;
            have_base = 1;
        } else {
            if (have_base && serial < 255u) serial++;
            b->info[i].syllable = serial;
            b->info[i].syllable_kind = GHB_SYLL_INDIC_BASE;
            have_base = 1;
        }
    }
    for (i = 0; i < b->len; ++i) ghb_trace(b, "cluster", i, (ghb_u16)b->info[i].codepoint, (ghb_u16)b->info[i].syllable);
    return GHB_OK;
}

static void ghb_apply_indic_syllable_reorder(ghb_buffer *b)
{
    int i, dst;
    ghb_glyph_info ti;
    ghb_glyph_pos tp;
    if (!b) return;
    for (i = 1; i < b->len; ++i) {
        if (!ghb_is_prebase_indic_vowel(b->script, b->info[i].unicode)) continue;
        dst = i;
        while (dst > 0 && b->info[dst - 1].syllable == b->info[i].syllable) dst--;
        if (dst == i) continue;
        ti = b->info[i]; tp = b->pos[i];
        while (i > dst) { b->info[i] = b->info[i-1]; b->pos[i] = b->pos[i-1]; i--; }
        b->info[dst] = ti; b->pos[dst] = tp;
        ghb_trace(b, "indic_matra", dst, (ghb_u16)ti.codepoint, (ghb_u16)ti.unicode);
    }
}

static void ghb_apply_reorder_rules(const ghb_static_font *font, ghb_buffer *b)
{
    int i, r, dst;
    ghb_glyph_info ti;
    ghb_glyph_pos tp;
    if (!font->reorder_rules) return;
    for (i = 0; i < b->len; ++i) {
        for (r = 0; r < font->reorder_rule_count; ++r) {
            if (font->reorder_rules[r].script_tag != b->script) continue;
            if (font->reorder_rules[r].unicode != b->info[i].unicode) continue;
            dst = i - (int)font->reorder_rules[r].shift_left;
            if (dst < 0) dst = 0;
            if (dst >= i) continue;
            ti = b->info[i]; tp = b->pos[i];
            while (i > dst) { b->info[i] = b->info[i-1]; b->pos[i] = b->pos[i-1]; i--; }
            b->info[dst] = ti; b->pos[dst] = tp;
            break;
        }
    }
}


static int ghb_is_deva_ra(ghb_u32 cp)
{
    switch (cp) {
    case 0x0930UL: case 0x09b0UL: case 0x0a30UL: case 0x0ab0UL:
    case 0x0bb0UL: case 0x0c30UL: case 0x0cb0UL: case 0x0d30UL:
        return 1;
    default: return 0;
    }
}

static int ghb_is_indic_consonant_unicode(ghb_u32 cp)
{
    if (cp >= 0x0915UL && cp <= 0x0939UL) return 1;
    if (cp >= 0x0995UL && cp <= 0x09b9UL) return 1;
    if (cp >= 0x0a15UL && cp <= 0x0a39UL) return 1;
    if (cp >= 0x0a95UL && cp <= 0x0ab9UL) return 1;
    if (cp >= 0x0b95UL && cp <= 0x0bb9UL) return 1;
    if (cp >= 0x0c15UL && cp <= 0x0c39UL) return 1;
    if (cp >= 0x0c95UL && cp <= 0x0cb9UL) return 1;
    if (cp >= 0x0d15UL && cp <= 0x0d39UL) return 1;
    return 0;
}


static int ghb_indic_prefers_last_base(ghb_u32 script)
{
    if (script == GHB_SCRIPT_TAML || script == GHB_SCRIPT_TELU ||
        script == GHB_SCRIPT_KNDA || script == GHB_SCRIPT_MLYM) return 0;
    return 1;
}

static int ghb_find_indic_base_in_syllable(const ghb_buffer *b, int start, int end)
{
    int i;
    int chosen;
    if (!b || start < 0 || end > b->len || start >= end) return start;
    chosen = -1;
    if (ghb_indic_prefers_last_base(b->script)) {
        for (i = start; i < end; ++i) {
            if (ghb_is_indic_consonant_unicode(b->info[i].unicode)) chosen = i;
        }
    } else {
        for (i = start; i < end; ++i) {
            if (ghb_is_indic_consonant_unicode(b->info[i].unicode)) { chosen = i; break; }
        }
    }
    if (chosen >= 0) return chosen;
    return start;
}

static void ghb_move_range(ghb_buffer *b, int first, int count, int before)
{
    ghb_glyph_info tmpi[4];
    ghb_glyph_pos tmpp[4];
    int i;
    int src;
    if (!b || count <= 0 || count > 4 || first < 0 || first + count > b->len) return;
    if (before < 0) before = 0;
    if (before > b->len) before = b->len;
    if (before >= first && before <= first + count) return;
    for (i = 0; i < count; ++i) { tmpi[i] = b->info[first + i]; tmpp[i] = b->pos[first + i]; }
    if (before < first) {
        for (i = first - 1; i >= before; --i) { b->info[i + count] = b->info[i]; b->pos[i + count] = b->pos[i]; }
        for (i = 0; i < count; ++i) { b->info[before + i] = tmpi[i]; b->pos[before + i] = tmpp[i]; }
    } else {
        for (i = first + count; i < before; ++i) { b->info[i - count] = b->info[i]; b->pos[i - count] = b->pos[i]; }
        src = before - count;
        for (i = 0; i < count; ++i) { b->info[src + i] = tmpi[i]; b->pos[src + i] = tmpp[i]; }
    }
}

static void ghb_apply_indic_reph_half_post(const ghb_static_font *font, ghb_buffer *b, const ghb_feature *features, int feature_count)
{
    int start, end, i, base;
    if (!font || !b || !ghb_is_indic_script(b->script)) return;
    for (i = 0; i < b->len; ++i) {
        ghb_apply_one_single_feature(font, b, i, GHB_FEATURE_NUKT, features, feature_count);
        ghb_apply_one_single_feature(font, b, i, GHB_FEATURE_AKHN, features, feature_count);
    }
    start = 0;
    while (start < b->len) {
        end = start + 1;
        while (end < b->len && b->info[end].syllable == b->info[start].syllable) end++;
        if (start + 1 < end && ghb_is_deva_ra(b->info[start].unicode) && ghb_is_indic_halant(b->info[start + 1].unicode)) {
            ghb_apply_one_single_feature(font, b, start, GHB_FEATURE_RPHF, features, feature_count);
            base = ghb_find_indic_base_in_syllable(b, start + 2, end);
            if (base > start + 1 && base < end) { ghb_trace(b, "indic_reph", start, (ghb_u16)b->info[start].codepoint, (ghb_u16)b->info[base].codepoint); ghb_move_range(b, start, 2, base + 1); }
        }
        for (i = start; i + 1 < end; ++i) {
            if (ghb_is_indic_consonant_unicode(b->info[i].unicode) && ghb_is_indic_halant(b->info[i + 1].unicode)) {
                ghb_apply_one_single_feature(font, b, i, GHB_FEATURE_HALF, features, feature_count);
                ghb_apply_one_single_feature(font, b, i, GHB_FEATURE_BLWF, features, feature_count);
                ghb_apply_one_single_feature(font, b, i, GHB_FEATURE_PSTF, features, feature_count);
            }
        }
        start = end;
    }
}

static int ghb_is_thai_lao_mark(ghb_u32 cp)
{
    return ghb_is_thai_lao_mark_unicode(cp);
}


int ghb_font_classify_thai_lao_mark(const ghb_static_font *font, ghb_u32 cp)
{
    int i;
    if (font && font->thai_lao_mark_ranges) {
        for (i = 0; i < font->thai_lao_mark_range_count; i++) {
            const ghb_thai_lao_mark_range *r;
            r = &font->thai_lao_mark_ranges[i];
            if (cp >= r->first_unicode && cp <= r->last_unicode) {
                return (int)r->mark_class;
            }
        }
    }
    return ghb_classify_thai_lao_mark(cp);
}

int ghb_classify_thai_lao_mark(ghb_u32 cp)
{
    /* v0.15: public Thai/Lao mark classes usable by packer-generated tables
       and debug overlays. Values intentionally match generic mark classes. */
    if (cp == 0x0e3aUL || cp == 0x0ebcUL) return GHB_MARK_CLASS_BELOW;
    if (cp == 0x0e31UL || (cp >= 0x0e34UL && cp <= 0x0e37UL) ||
        cp == 0x0eb1UL || (cp >= 0x0eb4UL && cp <= 0x0eb7UL)) return GHB_MARK_CLASS_ABOVE;
    if ((cp >= 0x0e48UL && cp <= 0x0e4cUL) || (cp >= 0x0ec8UL && cp <= 0x0eccUL)) return GHB_MARK_CLASS_TONE;
    if (ghb_is_thai_lao_mark_unicode(cp)) return GHB_MARK_CLASS_POST;
    return GHB_MARK_CLASS_NONE;
}

static int ghb_thai_lao_mark_order(ghb_u32 cp)
{
    int klass;
    klass = ghb_classify_thai_lao_mark(cp);
    if (klass == GHB_MARK_CLASS_BELOW) return 0;
    if (klass == GHB_MARK_CLASS_ABOVE) return 1;
    if (klass == GHB_MARK_CLASS_TONE) return 2;
    if (klass == GHB_MARK_CLASS_POST) return 3;
    return 4;
}

static void ghb_apply_thai_lao_mark_normalization(ghb_buffer *b)
{
    int i, j;
    ghb_glyph_info ti;
    ghb_glyph_pos tp;
    if (!b) return;
    if (b->script != GHB_SCRIPT_THAI && b->script != GHB_SCRIPT_LAO) return;
    for (i = 1; i < b->len; ++i) {
        if (!ghb_is_thai_lao_mark(b->info[i].unicode)) continue;
        b->info[i].glyph_flags |= GHB_GLYPH_MARK;
        b->pos[i].x_advance = 0;
        j = i;
        while (j > 0 && ghb_is_thai_lao_mark(b->info[j - 1].unicode) &&
               ghb_thai_lao_mark_order(b->info[j - 1].unicode) > ghb_thai_lao_mark_order(b->info[j].unicode)) {
            ti = b->info[j - 1]; tp = b->pos[j - 1];
            b->info[j - 1] = b->info[j]; b->pos[j - 1] = b->pos[j];
            b->info[j] = ti; b->pos[j] = tp;
            ghb_trace(b, "thai_lao", j - 1, (ghb_u16)b->info[j - 1].codepoint, (ghb_u16)b->info[j].codepoint);
            j--;
        }
    }
}

int ghb_shape(const ghb_static_font *font, ghb_buffer *b, const ghb_feature *features, int feature_count)
{
    if (!font || !b) return GHB_ERR_BAD_ARG;
    if (feature_count < 0 || feature_count > GHB_MAX_FEATURES) return GHB_ERR_BAD_ARG;
    if (!ghb_font_has_script_lang(font, b->script, b->language) && ghb_font_has_script_lang(font, GHB_SCRIPT_DFLT, GHB_LANG_DFLT)) {
        b->script = GHB_SCRIPT_DFLT;
        b->language = GHB_LANG_DFLT;
    }
    ghb_map_unicode_to_glyphs(font, b);
    ghb_init_positions(font, b);
    if (ghb_is_indic_script(b->script)) ghb_buffer_scan_indic_syllables(b);
    ghb_insert_dotted_circle_if_needed(font, b);

    if (ghb_is_indic_script(b->script)) {
        ghb_buffer_scan_indic_syllables(b);
        ghb_apply_indic_syllable_reorder(b);
        ghb_apply_indic_reph_half_post(font, b, features, feature_count);
    }
    ghb_apply_thai_lao_mark_normalization(b);
    ghb_apply_reorder_rules(font, b);       /* static per-font reorder rules */
    ghb_apply_arabic_joining(font, b, features, feature_count);
    ghb_apply_single_subs(font, b, features, feature_count);
    ghb_apply_context_subs(font, b, features, feature_count);
    ghb_apply_context_class_subs(font, b, features, feature_count);
    ghb_apply_reverse_context_subs(font, b, features, feature_count);
    ghb_apply_ligatures(font, b, features, feature_count);
    ghb_init_positions(font, b);
    ghb_apply_kern(font, b, features, feature_count);
    ghb_apply_context_pair_adjusts(font, b, features, feature_count);
    ghb_apply_mark_to_base(font, b, features, feature_count);
    ghb_apply_mark_to_mark(font, b, features, feature_count);
    if (b->direction == GHB_DIR_RTL) ghb_reverse_buffer(b);
    return GHB_OK;
}

int ghb_shape_utf8(const ghb_static_font *font, ghb_buffer *b, const char *text, int byte_count, const ghb_feature *features, int feature_count)
{
    int rc;
    if (!b) return GHB_ERR_BAD_ARG;
    ghb_buffer_clear(b);
    rc = ghb_buffer_add_utf8(b, text, byte_count);
    if (rc != GHB_OK) return rc;
    return ghb_shape(font, b, features, feature_count);
}


int ghb_buffer_dump_trace_numeric(const ghb_buffer *b, ghb_trace_dump_func fn, void *user)
{
    /* v0.16: numeric-only dump helper. Same payload contract as the normal
       dump callback, but it does not require trace stage names to be present. */
    return ghb_buffer_dump_trace_records(b, fn, user);
}
