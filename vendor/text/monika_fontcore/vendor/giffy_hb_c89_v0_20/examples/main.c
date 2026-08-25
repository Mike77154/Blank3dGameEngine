#include <stdio.h>
#include "giffy_hb.h"
#include "demo_font.h"

static void trace_cb(void *user, const char *stage, int index, ghb_u16 gid, ghb_u16 out_gid)
{
    (void)user;
    printf("trace %-12s i=%d gid=%u out=%u\n", stage, index, (unsigned)gid, (unsigned)out_gid);
}

static void record_cb(void *user, int index, ghb_u16 gid, ghb_u32 unicode, ghb_f26d6 xa, ghb_f26d6 xo, ghb_f26d6 yo, ghb_u16 flags, ghb_u8 syllable, ghb_u8 syllable_kind)
{
    (void)user;
    printf("record i=%d gid=%u uni=U+%04lX adv=%ld off=(%ld,%ld) flags=0x%04x syll=%u kind=%u\n",
           index, (unsigned)gid, (unsigned long)unicode, (long)xa, (long)xo, (long)yo,
           (unsigned)flags, (unsigned)syllable, (unsigned)syllable_kind);
}

static void numeric_trace_cb(void *user, int row, ghb_u8 stage, ghb_u8 event_id, ghb_s16 index, ghb_u16 gid, ghb_u16 value)
{
    (void)user;
    printf("numtrace row=%d stage=%u event=%u index=%d gid=%u value=%u\n",
           row, (unsigned)stage, (unsigned)event_id, (int)index, (unsigned)gid, (unsigned)value);
}

static void dump(const char *label, const ghb_buffer *b)
{
    int i;
    printf("%s len=%d\n", label, b->len);
    for (i = 0; i < b->len; ++i) {
        printf("  [%02d] gid=%lu uni=U+%04lX adv=%ld off=(%ld,%ld) flags=0x%04x\n",
               i,
               (unsigned long)b->info[i].codepoint,
               (unsigned long)b->info[i].unicode,
               (long)b->pos[i].x_advance,
               (long)b->pos[i].x_offset,
               (long)b->pos[i].y_offset,
               (unsigned)b->info[i].glyph_flags);
    }
}

int main(void)
{
    ghb_buffer b;
    ghb_trace_record ring[16];
    ghb_trace_record copy[16];
    ghb_hvar_diag hdiag;
    ghb_hvar_contribution hcontrib[8];
    ghb_trace_overlay_record overlay[16];
    ghb_trace_overlay_compact compact[16];
    ghb_hvar_explain_record hexplain;
    ghb_hvar_region_diag rdiag;
    int bad_range_count, bad_axis_count, bad_record_count;
    int base_count, mark_count;
    ghb_u16 hitem;
    ghb_u8 hkind;
    int mapped_count, unmapped_count, sparse_count, segment_count;
    int ri, rn;
    ghb_f26d6 htotal;
    ghb_buffer_init(&b);
    ghb_buffer_set_trace(&b, trace_cb, 0);
    ghb_buffer_set_trace_ring(&b, ring, 16);
    ghb_buffer_set_trace_stage_mask(&b, 0xffffffffUL);

    ghb_shape_utf8(&demo_font, &b, "ffiToAV", -1, 0, 0);
    dump("latin liga+kern", &b);

    ghb_shape_utf8(&demo_font, &b, "A\xCC\x81\xCC\x87", -1, 0, 0);
    dump("mark-to-base + mark-to-mark", &b);

    ghb_shape_utf8(&demo_font, &b, "XsX", -1, 0, 0);
    dump("contextual GSUB calt", &b);

    ghb_shape_utf8(&demo_font, &b, "AsV", -1, 0, 0);
    dump("class-based contextual calt", &b);

    ghb_shape_utf8(&demo_font, &b, "TAVX", -1, 0, 0);
    dump("contextual GPOS pair adjust", &b);

    ghb_shape_utf8(&demo_font, &b, "\314\201A", -1, 0, 0);
    dump("dotted-circle fallback", &b);

    ghb_shape_utf8(&demo_font, &b, "XA", -1, 0, 0);
    dump("reverse chaining substitution", &b);

    ghb_buffer_set_script(&b, GHB_SCRIPT_ARAB);
    ghb_buffer_set_direction(&b, GHB_DIR_RTL);
    ghb_shape_utf8(&demo_font, &b, "\xD8""\xA8""\xD9""\x85""\xD8""\xA7", -1, 0, 0); /* beh meem alef */
    dump("arabic joining + rtl", &b);

    ghb_shape_utf8(&demo_font, &b, "\xD9""\x84""\xD8""\xA7", -1, 0, 0); /* lam alef */
    dump("arabic lam-alef rlig", &b);

    ghb_buffer_set_direction(&b, GHB_DIR_LTR);
    ghb_buffer_set_script(&b, GHB_TAG('d','e','v','a'));
    ghb_shape_utf8(&demo_font, &b, "\xE0""\xA4""\x95""\xE0""\xA4""\xBF", -1, 0, 0); /* ka + vowel sign i */
    dump("indic pre-base reorder hook", &b);

    ghb_shape_utf8(&demo_font, &b, "\xE0""\xA4""\xB0""\xE0""\xA5""\x8D""\xE0""\xA4""\x95", -1, 0, 0); /* ra + halant + ka */
    dump("indic reph/half skeleton", &b);

    ghb_buffer_set_script(&b, GHB_SCRIPT_THAI);
    ghb_shape_utf8(&demo_font, &b, "\xE0""\xB8""\x81""\xE0""\xB9""\x88""\xE0""\xB8""\xB4", -1, 0, 0); /* ko kai + mai ek + sara i */
    dump("thai/lao mark normalization", &b);

    ghb_buffer_set_script(&b, GHB_SCRIPT_LATN);
    ghb_buffer_set_variation(&b, 0, 900 * 64L);
    ghb_shape_utf8(&demo_font, &b, "AV", -1, 0, 0);
    dump("HVAR-style variation advance", &b);
    ghb_buffer_dump_trace_records(&b, record_cb, 0);

    if (ghb_font_get_hvar_diagnostics(&demo_font, &hdiag) == GHB_OK) {
        printf("hvar diag sparse=%d segments=%d item_delta=%d mapped=%d unmapped=%d lossy=%d\n",
               hdiag.sparse_map_count, hdiag.segment_map_count, hdiag.item_delta_count,
               hdiag.mapped_advance_count, hdiag.unmapped_advance_count, hdiag.lossy);
    }

    rn = ghb_buffer_copy_trace_ring_stage(&b, copy, 16, GHB_TRACE_STAGE_HVAR_VALIDATE);
    printf("filtered hvar-validate trace count=%d\n", rn);
    if (ghb_font_validate_hvar_gid_item_map(&demo_font, 2, &hitem, &hkind)) {
        printf("hvar validate gid=2 item=%u kind=%u\n", (unsigned)hitem, (unsigned)hkind);
    }
    if (ghb_font_validate_hvar_gid_range(&demo_font, 1, 8, &mapped_count, &unmapped_count, &sparse_count, &segment_count) == GHB_OK) {
        printf("hvar range 1..8 mapped=%d unmapped=%d sparse=%d segment=%d\n",
               mapped_count, unmapped_count, sparse_count, segment_count);
    }
    rn = ghb_font_get_hvar_contributions(&demo_font, &b, GID_A, hcontrib, 8, &htotal);
    printf("hvar contrib gid=A count=%d total=%ld\n", rn, (long)htotal);
    for (ri = 0; ri < rn && ri < 8; ++ri) {
        printf("  hcontrib[%d] item=%u region=%u kind=%u delta=%d support=%ld contribution=%ld\n",
               ri, (unsigned)hcontrib[ri].item_index, (unsigned)hcontrib[ri].region_index,
               (unsigned)hcontrib[ri].map_kind, (int)hcontrib[ri].delta_26d6,
               (long)hcontrib[ri].support_26d6, (long)hcontrib[ri].contribution_26d6);
    }
    rn = ghb_buffer_copy_trace_overlay(&b, overlay, 16,
         ghb_trace_stage_mask(GHB_TRACE_STAGE_HVAR_VALIDATE) |
         ghb_trace_stage_mask(GHB_TRACE_STAGE_HVAR_REGION) |
         ghb_trace_stage_mask(GHB_TRACE_STAGE_CONTEXT_PAIR_APPLY));
    printf("overlay trace count=%d\n", rn);
    for (ri = 0; ri < rn; ++ri) {
        printf("  overlay[%02d] stage=%u event=%u kind=%u index=%d gid=%u value=%u\n",
               ri, (unsigned)overlay[ri].stage, (unsigned)overlay[ri].event_id,
               (unsigned)overlay[ri].map_kind, (int)overlay[ri].index,
               (unsigned)overlay[ri].gid, (unsigned)overlay[ri].value);
    }
    if (ghb_font_explain_hvar_gid(&demo_font, &b, GID_A, &hexplain) == GHB_OK) {
        printf("hvar explain gid=%u item=%u kind=%u valid=%u regions=%u bad_regions=%u total=%ld\n",
               (unsigned)hexplain.gid, (unsigned)hexplain.item_index,
               (unsigned)hexplain.map_kind, (unsigned)hexplain.valid_item,
               (unsigned)hexplain.region_count, (unsigned)hexplain.bad_region_count,
               (long)hexplain.total_delta_26d6);
    }
    if (ghb_font_validate_hvar_region_ranges(&demo_font, &bad_range_count, &bad_axis_count, &bad_record_count) == GHB_OK) {
        printf("hvar region validate bad_range=%d bad_axis=%d bad_record=%d\n",
               bad_range_count, bad_axis_count, bad_record_count);
    }
    if (ghb_font_get_hvar_region_diagnostics(&demo_font, &rdiag) == GHB_OK) {
        printf("hvar region diag bad_range=%d bad_axis=%d bad_record=%d empty=%d max_axes=%d\n",
               rdiag.bad_range_count, rdiag.bad_axis_count, rdiag.bad_record_count,
               rdiag.empty_record_count, rdiag.max_axes_per_record);
    }
    rn = ghb_buffer_copy_trace_overlay_events(&b, overlay, 16, ghb_trace_stage_mask(GHB_EVENT_HVAR_MAP) | ghb_trace_stage_mask(GHB_EVENT_HVAR_REGION));
    printf("overlay event-filter hvar count=%d\n", rn);
    rn = ghb_buffer_copy_trace_overlay_compact(&b, compact, 16, ghb_trace_stage_mask(GHB_EVENT_HVAR_MAP) | ghb_trace_stage_mask(GHB_EVENT_HVAR_REGION));
    printf("overlay compact hvar count=%d\n", rn);
    for (ri = 0; ri < rn; ++ri) {
        printf("  compact[%02d] event=%u flags=0x%02x index=%d a=%u b=%u\n",
               ri, (unsigned)compact[ri].event_id, (unsigned)compact[ri].flags,
               (int)compact[ri].index, (unsigned)compact[ri].a, (unsigned)compact[ri].b);
    }
    (void)ghb_buffer_dump_trace_events_numeric(&b, numeric_trace_cb, 0, ghb_trace_stage_mask(GHB_EVENT_HVAR_MAP) | ghb_trace_stage_mask(GHB_EVENT_HVAR_REGION));
    ghb_buffer_get_syllable_status_counts(&b, b.info[0].syllable, &base_count, &mark_count);
    printf("thai mark class U+0E48=%d dotted-needed=%d syll-status=0x%02x cluster=%s bases=%d marks=%d\n",
           ghb_classify_thai_lao_mark(0x0e48UL),
           ghb_buffer_cluster_needs_dotted(&b, b.info[0].syllable),
           (unsigned)ghb_buffer_get_syllable_status(&b, b.info[0].syllable),
           ghb_cluster_status_name(ghb_buffer_get_cluster_status(&b, b.info[0].syllable)),
           base_count, mark_count);
    rn = ghb_buffer_copy_trace_ring(&b, copy, 16);
    printf("trace ring count=%d\n", rn);
    for (ri = 0; ri < rn; ++ri) {
        printf("  ring[%02d] stage=%s index=%d gid=%u out=%u\n",
               ri, ghb_trace_stage_name(copy[ri].stage), (int)copy[ri].index,
               (unsigned)copy[ri].gid, (unsigned)copy[ri].out_gid);
    }

    return 0;
}
