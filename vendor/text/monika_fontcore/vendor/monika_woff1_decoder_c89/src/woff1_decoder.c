#include "woff1_decoder.h"
#include "woff1_inflate.h"

struct woff1_entry_s {
    woff1_u32 tag;
    woff1_u32 offset;
    woff1_u32 comp_length;
    woff1_u32 orig_length;
    woff1_u32 orig_checksum;
    woff1_u32 out_offset;
};

static woff1_u32 woff1_read_be32(const woff1_u8 *p)
{
    return ((woff1_u32)p[0] << 24) | ((woff1_u32)p[1] << 16) |
           ((woff1_u32)p[2] << 8) | (woff1_u32)p[3];
}

static woff1_u16 woff1_read_be16(const woff1_u8 *p)
{
    return (woff1_u16)(((woff1_u16)p[0] << 8) | (woff1_u16)p[1]);
}

static void woff1_write_be32(woff1_u8 *p, woff1_u32 v)
{
    p[0] = (woff1_u8)((v >> 24) & 255u);
    p[1] = (woff1_u8)((v >> 16) & 255u);
    p[2] = (woff1_u8)((v >> 8) & 255u);
    p[3] = (woff1_u8)(v & 255u);
}

static void woff1_write_be16(woff1_u8 *p, woff1_u16 v)
{
    p[0] = (woff1_u8)((v >> 8) & 255u);
    p[1] = (woff1_u8)(v & 255u);
}

static woff1_u32 woff1_pad4(woff1_u32 v)
{
    return (v + 3u) & ~((woff1_u32)3u);
}

static int woff1_add_overflow(woff1_u32 a, woff1_u32 b, woff1_u32 *out)
{
    if (a > ((woff1_u32)0xffffffffu) - b) {
        return 1;
    }
    *out = a + b;
    return 0;
}

static void woff1_zero(woff1_u8 *p, woff1_u32 n)
{
    woff1_u32 i;
    for (i = 0; i < n; ++i) {
        p[i] = 0;
    }
}

void woff1_report_clear(woff1_report *report)
{
    if (report != 0) {
        report->flavor = 0;
        report->num_tables = 0;
        report->major_version = 0;
        report->minor_version = 0;
        report->woff_length = 0;
        report->total_sfnt_size = 0;
        report->metadata_offset = 0;
        report->metadata_length = 0;
        report->metadata_orig_length = 0;
        report->private_offset = 0;
        report->private_length = 0;
        report->repaired_checksum_adjustment = 0;
        report->last_inflate_error = 0;
    }
}

void woff1_tag_to_cstr(woff1_u32 tag, char out5[5])
{
    out5[0] = (char)((tag >> 24) & 255u);
    out5[1] = (char)((tag >> 16) & 255u);
    out5[2] = (char)((tag >> 8) & 255u);
    out5[3] = (char)(tag & 255u);
    out5[4] = '\0';
}

static woff1_u32 woff1_table_checksum(const woff1_u8 *data,
                                      woff1_u32 len,
                                      woff1_u32 tag)
{
    woff1_u32 padded;
    woff1_u32 i;
    woff1_u32 b0;
    woff1_u32 b1;
    woff1_u32 b2;
    woff1_u32 b3;
    woff1_u32 sum;

    padded = woff1_pad4(len);
    sum = 0;
    for (i = 0; i < padded; i += 4u) {
        if (tag == WOFF1_TAG('h','e','a','d') && i == 8u) {
            b0 = 0;
            b1 = 0;
            b2 = 0;
            b3 = 0;
        } else {
            b0 = (i < len) ? (woff1_u32)data[i] : 0u;
            b1 = ((i + 1u) < len) ? (woff1_u32)data[i + 1u] : 0u;
            b2 = ((i + 2u) < len) ? (woff1_u32)data[i + 2u] : 0u;
            b3 = ((i + 3u) < len) ? (woff1_u32)data[i + 3u] : 0u;
        }
        sum += (b0 << 24) | (b1 << 16) | (b2 << 8) | b3;
        sum &= 0xffffffffu;
    }
    return sum;
}

static woff1_u32 woff1_file_checksum(const woff1_u8 *data, woff1_u32 len)
{
    woff1_u32 padded;
    woff1_u32 i;
    woff1_u32 b0;
    woff1_u32 b1;
    woff1_u32 b2;
    woff1_u32 b3;
    woff1_u32 sum;

    padded = woff1_pad4(len);
    sum = 0;
    for (i = 0; i < padded; i += 4u) {
        b0 = (i < len) ? (woff1_u32)data[i] : 0u;
        b1 = ((i + 1u) < len) ? (woff1_u32)data[i + 1u] : 0u;
        b2 = ((i + 2u) < len) ? (woff1_u32)data[i + 2u] : 0u;
        b3 = ((i + 3u) < len) ? (woff1_u32)data[i + 3u] : 0u;
        sum += (b0 << 24) | (b1 << 16) | (b2 << 8) | b3;
        sum &= 0xffffffffu;
    }
    return sum;
}

static void woff1_calc_sfnt_search(woff1_u16 num_tables,
                                   woff1_u16 *search_range,
                                   woff1_u16 *entry_selector,
                                   woff1_u16 *range_shift)
{
    woff1_u16 max_power;
    woff1_u16 selector;

    max_power = 1;
    selector = 0;
    while ((woff1_u16)(max_power * 2u) <= num_tables) {
        max_power = (woff1_u16)(max_power * 2u);
        selector = (woff1_u16)(selector + 1u);
    }
    *search_range = (woff1_u16)(max_power * 16u);
    *entry_selector = selector;
    *range_shift = (woff1_u16)(num_tables * 16u - *search_range);
}

static int woff1_range_ok(woff1_u32 off, woff1_u32 len, woff1_u32 size)
{
    if (len == 0u) {
        return off <= size;
    }
    if (off > size) {
        return 0;
    }
    if (len > size - off) {
        return 0;
    }
    return 1;
}

static int woff1_read_header_and_entries(const woff1_u8 *woff, woff1_u32 woff_size,
                                         struct woff1_entry_s entries[WOFF1_MAX_TABLES],
                                         woff1_report *report)
{
    woff1_u16 num_tables;
    woff1_u32 dir_size;
    woff1_u32 expected_header_end;
    woff1_u32 i;
    const woff1_u8 *p;

    if (woff == 0 || report == 0) {
        return WOFF1_ERR_NULL;
    }
    if (woff_size < 44u) {
        return WOFF1_ERR_TOO_SMALL;
    }
    if (woff1_read_be32(woff) != WOFF1_SIGNATURE) {
        return WOFF1_ERR_BAD_SIGNATURE;
    }

    report->flavor = woff1_read_be32(woff + 4);
    report->woff_length = woff1_read_be32(woff + 8);
    num_tables = woff1_read_be16(woff + 12);
    report->num_tables = num_tables;
    if (woff1_read_be16(woff + 14) != 0u) {
        return WOFF1_ERR_BAD_HEADER;
    }
    report->total_sfnt_size = woff1_read_be32(woff + 16);
    report->major_version = woff1_read_be16(woff + 20);
    report->minor_version = woff1_read_be16(woff + 22);
    report->metadata_offset = woff1_read_be32(woff + 24);
    report->metadata_length = woff1_read_be32(woff + 28);
    report->metadata_orig_length = woff1_read_be32(woff + 32);
    report->private_offset = woff1_read_be32(woff + 36);
    report->private_length = woff1_read_be32(woff + 40);

    if (report->woff_length != woff_size) {
        return WOFF1_ERR_BAD_HEADER;
    }
    if (num_tables == 0u || num_tables > (woff1_u16)WOFF1_MAX_TABLES) {
        return WOFF1_ERR_TOO_MANY_TABLES;
    }
    dir_size = (woff1_u32)num_tables * 20u;
    if (woff1_add_overflow(44u, dir_size, &expected_header_end)) {
        return WOFF1_ERR_RANGE;
    }
    if (expected_header_end > woff_size) {
        return WOFF1_ERR_TOO_SMALL;
    }

    for (i = 0; i < (woff1_u32)num_tables; ++i) {
        p = woff + 44u + i * 20u;
        entries[i].tag = woff1_read_be32(p);
        entries[i].offset = woff1_read_be32(p + 4);
        entries[i].comp_length = woff1_read_be32(p + 8);
        entries[i].orig_length = woff1_read_be32(p + 12);
        entries[i].orig_checksum = woff1_read_be32(p + 16);
        entries[i].out_offset = 0;
        if (entries[i].comp_length > entries[i].orig_length) {
            return WOFF1_ERR_BAD_COMPRESSION_LENGTH;
        }
        if ((entries[i].offset & 3u) != 0u) {
            return WOFF1_ERR_RANGE;
        }
        if (!woff1_range_ok(entries[i].offset, entries[i].comp_length, woff_size)) {
            return WOFF1_ERR_RANGE;
        }
        if (i > 0u && entries[i - 1u].tag >= entries[i].tag) {
            return WOFF1_ERR_UNSORTED_TAGS;
        }
    }

    return WOFF1_OK;
}

static int woff1_check_metadata_private(const struct woff1_entry_s entries[WOFF1_MAX_TABLES],
                                        const woff1_report *report,
                                        woff1_u32 woff_size)
{
    woff1_u32 i;
    woff1_u32 a0;
    woff1_u32 a1;
    woff1_u32 b0;
    woff1_u32 b1;

    if (report->metadata_offset == 0u || report->metadata_length == 0u) {
        if (report->metadata_offset != 0u || report->metadata_length != 0u ||
            report->metadata_orig_length != 0u) {
            return WOFF1_ERR_METADATA_RANGE;
        }
    } else {
        if (!woff1_range_ok(report->metadata_offset, report->metadata_length, woff_size)) {
            return WOFF1_ERR_METADATA_RANGE;
        }
    }

    if (report->private_offset == 0u || report->private_length == 0u) {
        if (report->private_offset != 0u || report->private_length != 0u) {
            return WOFF1_ERR_PRIVATE_RANGE;
        }
    } else {
        if (!woff1_range_ok(report->private_offset, report->private_length, woff_size)) {
            return WOFF1_ERR_PRIVATE_RANGE;
        }
    }

    if (report->metadata_offset != 0u && report->private_offset != 0u) {
        a0 = report->metadata_offset;
        a1 = report->metadata_offset + report->metadata_length;
        b0 = report->private_offset;
        b1 = report->private_offset + report->private_length;
        if (a0 < b1 && b0 < a1) {
            return WOFF1_ERR_OVERLAP;
        }
    }

    for (i = 0; i < (woff1_u32)report->num_tables; ++i) {
        b0 = entries[i].offset;
        b1 = entries[i].offset + entries[i].comp_length;
        if (report->metadata_offset != 0u) {
            a0 = report->metadata_offset;
            a1 = report->metadata_offset + report->metadata_length;
            if (a0 < b1 && b0 < a1) {
                return WOFF1_ERR_OVERLAP;
            }
        }
        if (report->private_offset != 0u) {
            a0 = report->private_offset;
            a1 = report->private_offset + report->private_length;
            if (a0 < b1 && b0 < a1) {
                return WOFF1_ERR_OVERLAP;
            }
        }
    }
    return WOFF1_OK;
}

static void woff1_sort_indices_by_offset(const struct woff1_entry_s entries[WOFF1_MAX_TABLES],
                                         woff1_u16 num_tables,
                                         woff1_u16 order[WOFF1_MAX_TABLES])
{
    woff1_u16 i;
    woff1_u16 j;
    woff1_u16 key;

    for (i = 0; i < num_tables; ++i) {
        order[i] = i;
    }
    for (i = 1; i < num_tables; ++i) {
        key = order[i];
        j = i;
        while (j > 0u && entries[order[j - 1u]].offset > entries[key].offset) {
            order[j] = order[j - 1u];
            j--;
        }
        order[j] = key;
    }
}

static int woff1_check_table_overlaps(const struct woff1_entry_s entries[WOFF1_MAX_TABLES],
                                      woff1_u16 num_tables)
{
    woff1_u16 order[WOFF1_MAX_TABLES];
    woff1_u16 i;
    woff1_u32 end;
    woff1_u32 cur;

    woff1_sort_indices_by_offset(entries, num_tables, order);
    end = 44u + (woff1_u32)num_tables * 20u;
    for (i = 0; i < num_tables; ++i) {
        cur = order[i];
        if (entries[cur].offset < end) {
            return WOFF1_ERR_OVERLAP;
        }
        if (i > 0u) {
            if (entries[order[i - 1u]].offset + entries[order[i - 1u]].comp_length > entries[cur].offset) {
                return WOFF1_ERR_OVERLAP;
            }
        }
        end = entries[cur].offset + entries[cur].comp_length;
    }
    return WOFF1_OK;
}

static int woff1_compute_output_offsets(struct woff1_entry_s entries[WOFF1_MAX_TABLES],
                                        const woff1_report *report,
                                        woff1_u32 *computed_total)
{
    woff1_u16 order[WOFF1_MAX_TABLES];
    woff1_u16 i;
    woff1_u32 pos;
    woff1_u32 padded;
    woff1_u32 idx;

    pos = 12u + (woff1_u32)report->num_tables * 16u;
    woff1_sort_indices_by_offset(entries, report->num_tables, order);
    for (i = 0; i < report->num_tables; ++i) {
        idx = (woff1_u32)order[i];
        if ((pos & 3u) != 0u) {
            return WOFF1_ERR_TOTAL_SFNT_SIZE;
        }
        entries[idx].out_offset = pos;
        padded = woff1_pad4(entries[idx].orig_length);
        if (woff1_add_overflow(pos, padded, &pos)) {
            return WOFF1_ERR_TOTAL_SFNT_SIZE;
        }
    }
    *computed_total = pos;
    return WOFF1_OK;
}

static int woff1_find_entry_by_tag(const struct woff1_entry_s entries[WOFF1_MAX_TABLES],
                                   woff1_u16 num_tables, woff1_u32 tag)
{
    woff1_u16 i;
    for (i = 0; i < num_tables; ++i) {
        if (entries[i].tag == tag) {
            return (int)i;
        }
    }
    return -1;
}

int woff1_decode_to_sfnt(const woff1_u8 *woff, woff1_u32 woff_size,
                         woff1_u8 *sfnt, woff1_u32 sfnt_cap,
                         woff1_u32 *sfnt_size,
                         woff1_report *report)
{
    struct woff1_entry_s entries[WOFF1_MAX_TABLES];
    woff1_u32 computed_total;
    woff1_u32 i;
    woff1_u32 out_len;
    woff1_u32 checksum;
    woff1_u16 search_range;
    woff1_u16 entry_selector;
    woff1_u16 range_shift;
    int r;
    int head_index;
    woff1_u32 adj;

    if (report != 0) {
        woff1_report_clear(report);
    }
    if (sfnt_size != 0) {
        *sfnt_size = 0;
    }
    if (woff == 0 || sfnt == 0 || sfnt_size == 0 || report == 0) {
        return WOFF1_ERR_NULL;
    }

    r = woff1_read_header_and_entries(woff, woff_size, entries, report);
    if (r != WOFF1_OK) return r;
    r = woff1_check_table_overlaps(entries, report->num_tables);
    if (r != WOFF1_OK) return r;
    r = woff1_check_metadata_private(entries, report, woff_size);
    if (r != WOFF1_OK) return r;
    r = woff1_compute_output_offsets(entries, report, &computed_total);
    if (r != WOFF1_OK) return r;
    if (computed_total != report->total_sfnt_size || (computed_total & 3u) != 0u) {
        return WOFF1_ERR_TOTAL_SFNT_SIZE;
    }
    if (sfnt_cap < computed_total) {
        return WOFF1_ERR_OUTPUT_TOO_SMALL;
    }

    woff1_zero(sfnt, computed_total);
    woff1_write_be32(sfnt, report->flavor);
    woff1_write_be16(sfnt + 4, report->num_tables);
    woff1_calc_sfnt_search(report->num_tables, &search_range, &entry_selector, &range_shift);
    woff1_write_be16(sfnt + 6, search_range);
    woff1_write_be16(sfnt + 8, entry_selector);
    woff1_write_be16(sfnt + 10, range_shift);

    for (i = 0; i < (woff1_u32)report->num_tables; ++i) {
        woff1_u8 *dir;
        dir = sfnt + 12u + i * 16u;
        woff1_write_be32(dir, entries[i].tag);
        woff1_write_be32(dir + 4, entries[i].orig_checksum);
        woff1_write_be32(dir + 8, entries[i].out_offset);
        woff1_write_be32(dir + 12, entries[i].orig_length);

        if (entries[i].comp_length == entries[i].orig_length) {
            if (entries[i].orig_length > computed_total - entries[i].out_offset) {
                return WOFF1_ERR_RANGE;
            }
            {
                woff1_u32 j;
                for (j = 0; j < entries[i].orig_length; ++j) {
                    sfnt[entries[i].out_offset + j] = woff[entries[i].offset + j];
                }
            }
            out_len = entries[i].orig_length;
        } else {
            out_len = 0;
            r = woff1_inflate_zlib(woff + entries[i].offset,
                                   entries[i].comp_length,
                                   sfnt + entries[i].out_offset,
                                   entries[i].orig_length,
                                   &out_len,
                                   1);
            if (r != WOFF1_INF_OK) {
                report->last_inflate_error = r;
                return WOFF1_ERR_INFLATE;
            }
        }
        if (out_len != entries[i].orig_length) {
            return WOFF1_ERR_INFLATE;
        }
#if WOFF1_STRICT_CHECKSUMS
        checksum = woff1_table_checksum(sfnt + entries[i].out_offset,
                                         entries[i].orig_length,
                                         entries[i].tag);
        if (checksum != entries[i].orig_checksum) {
            return WOFF1_ERR_CHECKSUM;
        }
#endif
    }

#if WOFF1_REPAIR_CHECKSUM_ADJUSTMENT
    head_index = woff1_find_entry_by_tag(entries, report->num_tables, WOFF1_TAG('h','e','a','d'));
    if (head_index < 0) {
        return WOFF1_ERR_NO_HEAD;
    }
    if (entries[head_index].orig_length < 12u) {
        return WOFF1_ERR_CHECKSUM;
    }
    woff1_write_be32(sfnt + entries[head_index].out_offset + 8u, 0u);
    adj = 0xB1B0AFBAu - woff1_file_checksum(sfnt, computed_total);
    adj &= 0xffffffffu;
    woff1_write_be32(sfnt + entries[head_index].out_offset + 8u, adj);
    report->repaired_checksum_adjustment = adj;
#endif

    *sfnt_size = computed_total;
    return WOFF1_OK;
}

int woff1_decode_metadata_xml(const woff1_u8 *woff, woff1_u32 woff_size,
                              woff1_u8 *out, woff1_u32 out_cap,
                              woff1_u32 *out_len,
                              woff1_report *report)
{
    struct woff1_entry_s entries[WOFF1_MAX_TABLES];
    int r;

    if (report != 0) {
        woff1_report_clear(report);
    }
    if (out_len != 0) {
        *out_len = 0;
    }
    if (woff == 0 || out == 0 || out_len == 0 || report == 0) {
        return WOFF1_ERR_NULL;
    }
    r = woff1_read_header_and_entries(woff, woff_size, entries, report);
    if (r != WOFF1_OK) return r;
    r = woff1_check_metadata_private(entries, report, woff_size);
    if (r != WOFF1_OK) return r;
    if (report->metadata_offset == 0u || report->metadata_length == 0u) {
        *out_len = 0;
        return WOFF1_OK;
    }
    if (out_cap < report->metadata_orig_length) {
        return WOFF1_ERR_OUTPUT_TOO_SMALL;
    }
    r = woff1_inflate_zlib(woff + report->metadata_offset,
                           report->metadata_length,
                           out,
                           report->metadata_orig_length,
                           out_len,
                           1);
    if (r != WOFF1_INF_OK) {
        report->last_inflate_error = r;
        return WOFF1_ERR_INFLATE;
    }
    if (*out_len != report->metadata_orig_length) {
        return WOFF1_ERR_INFLATE;
    }
    return WOFF1_OK;
}

int woff1_copy_private_data(const woff1_u8 *woff, woff1_u32 woff_size,
                            woff1_u8 *out, woff1_u32 out_cap,
                            woff1_u32 *out_len,
                            woff1_report *report)
{
    struct woff1_entry_s entries[WOFF1_MAX_TABLES];
    woff1_u32 i;
    int r;

    if (report != 0) {
        woff1_report_clear(report);
    }
    if (out_len != 0) {
        *out_len = 0;
    }
    if (woff == 0 || out == 0 || out_len == 0 || report == 0) {
        return WOFF1_ERR_NULL;
    }
    r = woff1_read_header_and_entries(woff, woff_size, entries, report);
    if (r != WOFF1_OK) return r;
    r = woff1_check_metadata_private(entries, report, woff_size);
    if (r != WOFF1_OK) return r;
    if (report->private_offset == 0u || report->private_length == 0u) {
        *out_len = 0;
        return WOFF1_OK;
    }
    if (out_cap < report->private_length) {
        return WOFF1_ERR_OUTPUT_TOO_SMALL;
    }
    for (i = 0; i < report->private_length; ++i) {
        out[i] = woff[report->private_offset + i];
    }
    *out_len = report->private_length;
    return WOFF1_OK;
}

const char *woff1_error_string(int code)
{
    switch (code) {
    case WOFF1_OK: return "ok";
    case WOFF1_ERR_NULL: return "null pointer";
    case WOFF1_ERR_TOO_SMALL: return "input too small";
    case WOFF1_ERR_BAD_SIGNATURE: return "not a WOFF1 file";
    case WOFF1_ERR_BAD_HEADER: return "bad WOFF header";
    case WOFF1_ERR_TOO_MANY_TABLES: return "too many or zero tables";
    case WOFF1_ERR_RANGE: return "bad offset or length range";
    case WOFF1_ERR_OVERLAP: return "overlapping WOFF blocks";
    case WOFF1_ERR_UNSORTED_TAGS: return "table tags are not sorted";
    case WOFF1_ERR_BAD_COMPRESSION_LENGTH: return "compressed length exceeds original length";
    case WOFF1_ERR_OUTPUT_TOO_SMALL: return "output buffer too small";
    case WOFF1_ERR_TOTAL_SFNT_SIZE: return "bad totalSfntSize";
    case WOFF1_ERR_INFLATE: return "zlib/deflate inflate failed";
    case WOFF1_ERR_CHECKSUM: return "table checksum mismatch";
    case WOFF1_ERR_NO_HEAD: return "missing head table";
    case WOFF1_ERR_PRIVATE_RANGE: return "bad private data range";
    case WOFF1_ERR_METADATA_RANGE: return "bad metadata range";
    default: return "unknown WOFF1 error";
    }
}
