#include "giff_internal.h"

typedef struct giff_palette_candidate {
    giff_rgb8 palette[256];
    giff_u8 remap[256];
    giff_u16 entries;
    giff_u16 similarity_q8;
    giff_u32 estimated_cost;
    giff_u8 has_transparency;
    giff_u8 transparency_index;
    giff_u8 sorted;
    giff_u8 changed;
    giff_u8 used_temporal;
    giff_u8 mode;
    giff_u8 min_code_bits;
} giff_palette_candidate;

static giff_u8 giff_palette_bits(giff_u16 entries)
{
    giff_u8 bits;
    giff_u16 size;

    if (entries <= 1u) {
        return 1u;
    }

    bits = 1u;
    size = 2u;
    while (size < entries && bits < 8u) {
        size = (giff_u16)(size << 1u);
        bits = (giff_u8)(bits + 1u);
    }

    return bits;
}

static giff_u16 giff_encoder_interlaced_row(giff_u16 sequence_row, giff_u16 height)
{
    giff_u16 pass1;
    giff_u16 pass2;
    giff_u16 pass3;
    giff_u16 idx;

    pass1 = (giff_u16)((height + 7u) / 8u);
    pass2 = 0u;
    pass3 = 0u;

    if (height > 4u) {
        pass2 = (giff_u16)(((height - 5u) / 8u) + 1u);
    }
    if (height > 2u) {
        pass3 = (giff_u16)(((height - 3u) / 4u) + 1u);
    }

    if (sequence_row < pass1) {
        return (giff_u16)(sequence_row * 8u);
    }

    idx = (giff_u16)(sequence_row - pass1);
    if (idx < pass2) {
        return (giff_u16)(4u + idx * 8u);
    }

    idx = (giff_u16)(idx - pass2);
    if (idx < pass3) {
        return (giff_u16)(2u + idx * 4u);
    }

    idx = (giff_u16)(idx - pass3);
    return (giff_u16)(1u + idx * 2u);
}

static void giff_u16_to_le(giff_u8* dst, giff_u16 value)
{
    dst[0] = (giff_u8)(value & 0xFFu);
    dst[1] = (giff_u8)((value >> 8) & 0xFFu);
}

static giff_result giff_encoder_write_u8(giff_encoder* enc, giff_u8 value)
{
    return giff_encoder_write_bytes(enc, &value, 1u);
}

static giff_u16 giff_encoder_active_palette_entries(const giff_encoder* enc)
{
    if (enc->frame.has_local_palette) {
        if (enc->frame.local_palette_entries != 0u) {
            return enc->frame.local_palette_entries;
        }
        return enc->local_palette_entries;
    }
    return enc->global_palette_entries;
}

static giff_result giff_encoder_write_palette(
    giff_encoder* enc,
    const giff_rgb8* palette,
    giff_u16 entries
)
{
    giff_result rc;
    giff_u8 bits;
    giff_u16 table_entries;
    giff_u16 i;
    giff_u8 rgb[3];

    bits = giff_palette_bits(entries);
    table_entries = (giff_u16)(1u << bits);

    for (i = 0u; i < table_entries; ++i) {
        if (i < entries) {
            rgb[0] = palette[i].r;
            rgb[1] = palette[i].g;
            rgb[2] = palette[i].b;
        } else {
            rgb[0] = 0u;
            rgb[1] = 0u;
            rgb[2] = 0u;
        }
        rc = giff_encoder_write_bytes(enc, rgb, 3u);
        if (rc != GIFF_OK) {
            return rc;
        }
    }

    return GIFF_OK;
}

static giff_result giff_encoder_write_header(giff_encoder* enc, const giff_info* stream_info)
{
    giff_result rc;
    giff_u8 hdr[13];
    giff_u8 packed;
    giff_u8 bits;
    giff_u8 version_minor;

    version_minor = stream_info->version_minor;
    if (version_minor != (giff_u8)'7' && version_minor != (giff_u8)'9') {
        version_minor = (giff_u8)'9';
    }

    if (stream_info->loop_count != 1u) {
        version_minor = (giff_u8)'9';
    }

    hdr[0] = (giff_u8)'G';
    hdr[1] = (giff_u8)'I';
    hdr[2] = (giff_u8)'F';
    hdr[3] = (giff_u8)'8';
    hdr[4] = version_minor;
    hdr[5] = (giff_u8)'a';
    giff_u16_to_le(hdr + 6, stream_info->width);
    giff_u16_to_le(hdr + 8, stream_info->height);

    bits = giff_palette_bits(enc->global_palette_entries);
    packed = 0u;
    packed = (giff_u8)(packed | 0x80u);
    packed = (giff_u8)(packed | (giff_u8)((bits - 1u) << 4u));
    if (stream_info->global_palette_sorted) {
        packed = (giff_u8)(packed | 0x08u);
    }
    packed = (giff_u8)(packed | (giff_u8)(bits - 1u));

    hdr[10] = packed;
    hdr[11] = stream_info->background_index;
    hdr[12] = 0u;

    rc = giff_encoder_write_bytes(enc, hdr, 13u);
    if (rc != GIFF_OK) {
        return rc;
    }

    rc = giff_encoder_write_palette(enc, enc->global_palette, enc->global_palette_entries);
    if (rc != GIFF_OK) {
        return rc;
    }

    if (stream_info->loop_count != 1u) {
        giff_u8 app[19];
        app[0] = 0x21u;
        app[1] = 0xFFu;
        app[2] = 0x0Bu;
        app[3] = (giff_u8)'N';
        app[4] = (giff_u8)'E';
        app[5] = (giff_u8)'T';
        app[6] = (giff_u8)'S';
        app[7] = (giff_u8)'C';
        app[8] = (giff_u8)'A';
        app[9] = (giff_u8)'P';
        app[10] = (giff_u8)'E';
        app[11] = (giff_u8)'2';
        app[12] = (giff_u8)'.';
        app[13] = (giff_u8)'0';
        app[14] = 0x03u;
        app[15] = 0x01u;
        giff_u16_to_le(app + 16, stream_info->loop_count);
        app[18] = 0x00u;
        rc = giff_encoder_write_bytes(enc, app, 19u);
        if (rc != GIFF_OK) {
            return rc;
        }
    }

    return GIFF_OK;
}

static giff_result giff_encoder_write_gce(giff_encoder* enc, const giff_frame_info* frame)
{
    giff_result rc;
    giff_u8 gce[8];
    giff_u8 packed;

    packed = 0u;
    packed = (giff_u8)(packed | (giff_u8)((frame->disposal_method & 0x07u) << 2u));
    if (frame->has_transparency) {
        packed = (giff_u8)(packed | 0x01u);
    }

    gce[0] = 0x21u;
    gce[1] = 0xF9u;
    gce[2] = 0x04u;
    gce[3] = packed;
    giff_u16_to_le(gce + 4, frame->delay_cs);
    gce[6] = frame->transparency_index;
    gce[7] = 0x00u;

    rc = giff_encoder_write_bytes(enc, gce, 8u);
    return rc;
}

static giff_result giff_encoder_write_image_descriptor(giff_encoder* enc, const giff_frame_info* frame)
{
    giff_result rc;
    giff_u8 desc[10];
    giff_u8 packed;
    giff_u8 bits;
    giff_u16 local_entries;

    packed = 0u;
    local_entries = 0u;
    if (frame->interlaced) {
        packed = (giff_u8)(packed | 0x40u);
    }
    if (frame->has_local_palette) {
        local_entries = frame->local_palette_entries;
        if (local_entries == 0u) {
            local_entries = enc->local_palette_entries;
        }
        bits = giff_palette_bits(local_entries);
        packed = (giff_u8)(packed | 0x80u);
        if (enc->frame_sorted_local_palette) {
            packed = (giff_u8)(packed | 0x20u);
        }
        packed = (giff_u8)(packed | (giff_u8)(bits - 1u));
    }

    desc[0] = 0x2Cu;
    giff_u16_to_le(desc + 1, frame->left);
    giff_u16_to_le(desc + 3, frame->top);
    giff_u16_to_le(desc + 5, frame->width);
    giff_u16_to_le(desc + 7, frame->height);
    desc[9] = packed;

    rc = giff_encoder_write_bytes(enc, desc, 10u);
    if (rc != GIFF_OK) {
        return rc;
    }

    if (frame->has_local_palette) {
        rc = giff_encoder_write_palette(enc, enc->local_palette, local_entries);
        if (rc != GIFF_OK) {
            return rc;
        }
    }

    return GIFF_OK;
}

static giff_result giff_encoder_validate_stream_info(
    giff_encoder* enc,
    const giff_info* stream_info
)
{
    if (enc == 0 || stream_info == 0) {
        return GIFF_E_INVALID_ARGUMENT;
    }

    if (stream_info->width == 0u || stream_info->height == 0u) {
        return GIFF_E_BAD_DIMENSIONS;
    }

    if (stream_info->width > enc->cfg.max_width || stream_info->height > enc->cfg.max_height) {
        return GIFF_E_BAD_DIMENSIONS;
    }

    if (enc->global_palette_set == 0u || enc->global_palette_entries == 0u) {
        return GIFF_E_BAD_COLOR_TABLE;
    }

    if ((giff_u16)stream_info->background_index >= enc->global_palette_entries) {
        return GIFF_E_BAD_COLOR_TABLE;
    }

    return GIFF_OK;
}

static giff_result giff_encoder_validate_frame(
    giff_encoder* enc,
    const giff_frame_info* frame
)
{
    giff_u16 palette_entries;

    if (enc == 0 || frame == 0) {
        return GIFF_E_INVALID_ARGUMENT;
    }

    if (frame->width == 0u || frame->height == 0u) {
        return GIFF_E_BAD_DIMENSIONS;
    }

    if ((giff_u32)frame->left + (giff_u32)frame->width > (giff_u32)enc->stream_info.width ||
        (giff_u32)frame->top + (giff_u32)frame->height > (giff_u32)enc->stream_info.height) {
        return GIFF_E_BAD_DIMENSIONS;
    }


    if (frame->has_local_palette) {
        if (!enc->local_palette_set || enc->local_palette_entries == 0u) {
            return GIFF_E_BAD_COLOR_TABLE;
        }
        palette_entries = enc->local_palette_entries;
    } else {
        if (!enc->global_palette_set || enc->global_palette_entries == 0u) {
            return GIFF_E_BAD_COLOR_TABLE;
        }
        palette_entries = enc->global_palette_entries;
    }

    if (frame->has_transparency && (giff_u16)frame->transparency_index >= palette_entries) {
        return GIFF_E_BAD_COLOR_TABLE;
    }

    return GIFF_OK;
}

static giff_u8 giff_encoder_frame_min_code_size(const giff_encoder* enc)
{
    giff_u32 pixels;
    giff_u32 i;
    giff_u8 max_index;
    giff_u8 bits;

    if (enc == 0 || enc->frame_indexed == 0) {
        return 2u;
    }

    pixels = (giff_u32)enc->frame.width * (giff_u32)enc->frame.height;
    max_index = 0u;
    for (i = 0u; i < pixels; ++i) {
        if (enc->frame_indexed[i] > max_index) {
            max_index = enc->frame_indexed[i];
        }
    }

    bits = giff_palette_bits((giff_u16)max_index + 1u);
    if (bits < 2u) {
        bits = 2u;
    }
    return bits;
}

static giff_u16 giff_encoder_palette_table_entries(giff_u16 entries)
{
    giff_u8 bits;

    bits = giff_palette_bits(entries);
    return (giff_u16)(1u << bits);
}

static giff_u32 giff_encoder_palette_table_bytes(giff_u16 entries)
{
    return (giff_u32)giff_encoder_palette_table_entries(entries) * 3u;
}

static giff_u32 giff_encoder_log2_q12(giff_u32 value)
{
    giff_u32 base;
    giff_u32 result;
    giff_u32 frac;

    if (value <= 1u) {
        return 0u;
    }

    base = 1u;
    result = 0u;
    while ((base << 1u) != 0u && (base << 1u) <= value) {
        base <<= 1u;
        result += 4096u;
    }

    frac = ((value - base) << 12u) / base;
    return result + frac;
}

static giff_u16 giff_encoder_entropy_q8(const giff_u32* hist, giff_u16 entries, giff_u32 total)
{
    giff_u32 entropy_q12;
    giff_u16 i;

    if (hist == 0 || total == 0u) {
        return 0u;
    }

    entropy_q12 = 0u;
    for (i = 0u; i < entries; ++i) {
        giff_u32 count;
        giff_u32 p_q12;
        giff_u32 logp_q12;
        giff_u32 neglog_q12;
        giff_u32 term_q12;

        count = hist[i];
        if (count == 0u) {
            continue;
        }

        p_q12 = (count << 12u) / total;
        if (p_q12 == 0u) {
            p_q12 = 1u;
        }
        logp_q12 = giff_encoder_log2_q12(p_q12);
        if (logp_q12 >= 12u * 4096u) {
            neglog_q12 = 0u;
        } else {
            neglog_q12 = 12u * 4096u - logp_q12;
        }
        term_q12 = (p_q12 * neglog_q12 + 2048u) / 4096u;
        entropy_q12 += term_q12;
    }

    if (entropy_q12 > 8u * 4096u) {
        entropy_q12 = 8u * 4096u;
    }

    return (giff_u16)((entropy_q12 * 256u + 2048u) / 4096u);
}

static void giff_encoder_analyze_frame(
    const giff_encoder* enc,
    const giff_frame_info* frame,
    giff_u16 palette_entries,
    giff_u32* hist,
    giff_u16* out_used_count,
    giff_u16* out_max_index,
    giff_u32* out_same_adjacent,
    giff_u32* out_dominant_count,
    giff_u16* out_entropy_q8
)
{
    giff_u32 pixels;
    giff_u32 i;
    giff_u8 prev;
    giff_u16 used_count;
    giff_u16 max_index;
    giff_u32 same_adjacent;
    giff_u32 dominant_count;

    if (hist == 0) {
        return;
    }

    for (i = 0u; i < 256u; ++i) {
        hist[i] = 0u;
    }

    if (enc == 0 || frame == 0 || enc->frame_indexed == 0) {
        if (out_used_count != 0) {
            *out_used_count = 0u;
        }
        if (out_max_index != 0) {
            *out_max_index = 0u;
        }
        if (out_same_adjacent != 0) {
            *out_same_adjacent = 0u;
        }
        if (out_dominant_count != 0) {
            *out_dominant_count = 0u;
        }
        if (out_entropy_q8 != 0) {
            *out_entropy_q8 = 0u;
        }
        return;
    }

    pixels = (giff_u32)frame->width * (giff_u32)frame->height;
    used_count = 0u;
    max_index = 0u;
    same_adjacent = 0u;
    dominant_count = 0u;
    prev = 0u;

    for (i = 0u; i < pixels; ++i) {
        giff_u8 idx;
        idx = enc->frame_indexed[i];
        if ((giff_u16)idx >= palette_entries) {
            continue;
        }
        hist[idx] += 1u;
        if (hist[idx] == 1u) {
            used_count = (giff_u16)(used_count + 1u);
        }
        if (idx > max_index) {
            max_index = idx;
        }
        if (hist[idx] > dominant_count) {
            dominant_count = hist[idx];
        }
        if (i != 0u && idx == prev) {
            same_adjacent += 1u;
        }
        prev = idx;
    }

    if (out_used_count != 0) {
        *out_used_count = used_count;
    }
    if (out_max_index != 0) {
        *out_max_index = max_index;
    }
    if (out_same_adjacent != 0) {
        *out_same_adjacent = same_adjacent;
    }
    if (out_dominant_count != 0) {
        *out_dominant_count = dominant_count;
    }
    if (out_entropy_q8 != 0) {
        *out_entropy_q8 = giff_encoder_entropy_q8(hist, palette_entries, pixels);
    }
}

static void giff_encoder_apply_remap(
    giff_encoder* enc,
    const giff_u8* remap,
    const giff_frame_info* frame
)
{
    giff_u32 pixels;
    giff_u32 i;

    if (enc == 0 || remap == 0 || frame == 0 || enc->frame_indexed == 0) {
        return;
    }

    pixels = (giff_u32)frame->width * (giff_u32)frame->height;
    for (i = 0u; i < pixels; ++i) {
        enc->frame_indexed[i] = remap[enc->frame_indexed[i]];
    }
}

static giff_u32 giff_encoder_color_distance_sq(
    const giff_rgb8* a,
    const giff_rgb8* b
)
{
    giff_s32 dr;
    giff_s32 dg;
    giff_s32 db;

    dr = (giff_s32)a->r - (giff_s32)b->r;
    dg = (giff_s32)a->g - (giff_s32)b->g;
    db = (giff_s32)a->b - (giff_s32)b->b;
    return (giff_u32)(dr * dr + dg * dg + db * db);
}

static giff_u16 giff_encoder_segment_threshold(const giff_encoder* enc)
{
    giff_u16 threshold;

    if (enc == 0) {
        return (giff_u16)GIFF_DEFAULT_SEGMENT_SIMILARITY_Q8;
    }

    threshold = (giff_u16)enc->cfg.segment_similarity_q8_threshold;
    if (threshold == 0u) {
        threshold = (giff_u16)GIFF_DEFAULT_SEGMENT_SIMILARITY_Q8;
    }
    return threshold;
}

static giff_u16 giff_encoder_segment_max_frames(const giff_encoder* enc)
{
    giff_u16 frames;

    if (enc == 0) {
        return (giff_u16)GIFF_DEFAULT_SEGMENT_MAX_FRAMES;
    }

    frames = (giff_u16)enc->cfg.segment_max_frames;
    if (frames == 0u) {
        frames = (giff_u16)GIFF_DEFAULT_SEGMENT_MAX_FRAMES;
    }
    return frames;
}

static giff_u16 giff_encoder_palette_similarity_q8_weighted(
    const giff_rgb8* src_palette,
    giff_u16 src_entries,
    giff_u8 src_has_transparency,
    giff_u8 src_transparency_index,
    const giff_rgb8* ref_palette,
    giff_u16 ref_entries,
    giff_u8 ref_has_transparency,
    giff_u8 ref_transparency_index,
    const giff_u32* hist
)
{
    giff_u32 total;
    giff_u32 matched;
    giff_u16 i;

    if (src_palette == 0 || ref_palette == 0 || src_entries == 0u || ref_entries == 0u) {
        return 0u;
    }

    total = 0u;
    matched = 0u;

    for (i = 0u; i < src_entries; ++i) {
        giff_u32 weight;
        giff_u16 j;
        giff_u32 best_distance;

        if (src_has_transparency && i == (giff_u16)src_transparency_index) {
            continue;
        }

        weight = (hist != 0) ? hist[i] : 1u;
        if (weight == 0u) {
            continue;
        }

        total += weight;
        best_distance = GIFF_SEGMENT_COLOR_THRESHOLD + 1u;
        for (j = 0u; j < ref_entries; ++j) {
            giff_u32 distance;

            if (ref_has_transparency && j == (giff_u16)ref_transparency_index) {
                continue;
            }

            distance = giff_encoder_color_distance_sq(&src_palette[i], &ref_palette[j]);
            if (distance < best_distance) {
                best_distance = distance;
                if (distance == 0u) {
                    break;
                }
            }
        }

        if (best_distance <= GIFF_SEGMENT_COLOR_THRESHOLD) {
            matched += weight;
        }
    }

    if (src_has_transparency) {
        giff_u32 weight;

        weight = (hist != 0 && (giff_u16)src_transparency_index < src_entries) ? hist[src_transparency_index] : 1u;
        if (weight != 0u) {
            total += weight;
            if (ref_has_transparency) {
                matched += weight;
            }
        }
    }

    if (total == 0u) {
        return 0u;
    }

    return (giff_u16)((matched << 8u) / total);
}

static giff_u16 giff_encoder_window_similarity_q8(
    const giff_encoder* enc,
    const giff_rgb8* palette,
    giff_u16 entries,
    giff_u8 has_transparency,
    giff_u8 transparency_index,
    const giff_u32* hist
)
{
    if (enc == 0 || !enc->temporal_valid || enc->temporal_entries == 0u) {
        return 0u;
    }

    return giff_encoder_palette_similarity_q8_weighted(
        palette,
        entries,
        has_transparency,
        transparency_index,
        enc->temporal_palette,
        enc->temporal_entries,
        enc->temporal_has_transparency,
        enc->temporal_transparency_index,
        hist
    );
}

static giff_u16 giff_encoder_segment_similarity_q8_weighted(
    const giff_encoder* enc,
    const giff_rgb8* palette,
    giff_u16 entries,
    giff_u8 has_transparency,
    giff_u8 transparency_index,
    const giff_u32* hist
)
{
    if (enc == 0 || !enc->segment_valid || enc->segment_palette_entries == 0u) {
        return 0u;
    }

    return giff_encoder_palette_similarity_q8_weighted(
        palette,
        entries,
        has_transparency,
        transparency_index,
        enc->segment_palette,
        enc->segment_palette_entries,
        enc->segment_has_transparency,
        enc->segment_transparency_index,
        hist
    );
}

static giff_u16 giff_encoder_prev_match(
    const giff_encoder* enc,
    const giff_rgb8* src_palette,
    giff_u16 src_entries,
    const giff_u32* hist,
    const giff_u8* assigned,
    const giff_rgb8* target,
    giff_u32 max_distance
)
{
    giff_u16 i;
    giff_u16 best_index;
    giff_u32 best_distance;
    giff_u32 best_count;

    best_index = 0xFFFFu;
    best_distance = max_distance + 1u;
    best_count = 0u;

    (void)enc;

    for (i = 0u; i < src_entries; ++i) {
        giff_u32 distance;

        if (assigned[i] || hist[i] == 0u) {
            continue;
        }

        distance = giff_encoder_color_distance_sq(&src_palette[i], target);
        if (distance > max_distance) {
            continue;
        }

        if (best_index == 0xFFFFu || distance < best_distance ||
            (distance == best_distance && hist[i] > best_count) ||
            (distance == best_distance && hist[i] == best_count && i < best_index)) {
            best_index = i;
            best_distance = distance;
            best_count = hist[i];
        }
    }

    return best_index;
}

static giff_u32 giff_encoder_temporal_weight_for_color(
    const giff_encoder* enc,
    const giff_rgb8* color,
    giff_u8* out_exact_match
)
{
    giff_u16 i;
    giff_u16 best_index;
    giff_u32 best_distance;
    giff_u32 best_weight;

    if (out_exact_match != 0) {
        *out_exact_match = 0u;
    }

    if (enc == 0 || !enc->temporal_valid || enc->temporal_entries == 0u) {
        return 0u;
    }

    best_index = 0xFFFFu;
    best_distance = GIFF_TEMPORAL_COLOR_THRESHOLD + 1u;
    best_weight = 0u;

    for (i = 0u; i < enc->temporal_entries; ++i) {
        giff_u32 distance;

        distance = giff_encoder_color_distance_sq(&enc->temporal_palette[i], color);
        if (distance == 0u) {
            if (out_exact_match != 0) {
                *out_exact_match = 1u;
            }
            return enc->temporal_weights[i];
        }
        if (distance > GIFF_TEMPORAL_COLOR_THRESHOLD) {
            continue;
        }
        if (best_index == 0xFFFFu || distance < best_distance ||
            (distance == best_distance && enc->temporal_weights[i] > best_weight)) {
            best_index = i;
            best_distance = distance;
            best_weight = enc->temporal_weights[i];
        }
    }

    if (best_index == 0xFFFFu) {
        return 0u;
    }
    return best_weight;
}

#if 0
static void giff_encoder_temporal_decay(giff_encoder* enc)
{
    giff_u16 i;
    giff_u16 dst;
    giff_u8 shift;

    if (enc == 0 || !enc->temporal_valid || enc->temporal_entries == 0u) {
        return;
    }

    shift = enc->cfg.temporal_decay_shift;
    if (shift == 0u) {
        shift = 2u;
    }

    dst = 0u;
    for (i = 0u; i < enc->temporal_entries; ++i) {
        giff_u32 weight;

        weight = enc->temporal_weights[i];
        if (shift < 31u) {
            weight -= (weight >> shift);
        }
        if (weight == 0u) {
            continue;
        }

        if (dst != i) {
            enc->temporal_palette[dst] = enc->temporal_palette[i];
            enc->temporal_weights[dst] = weight;
        } else {
            enc->temporal_weights[dst] = weight;
        }
        dst = (giff_u16)(dst + 1u);
    }

    enc->temporal_entries = dst;
    enc->temporal_valid = (dst != 0u) ? 1u : 0u;
}

#endif

static void giff_encoder_temporal_sort(giff_encoder* enc)
{
    giff_u16 i;
    giff_u16 j;

    if (enc == 0 || !enc->temporal_valid || enc->temporal_entries < 2u) {
        return;
    }

    for (i = 0u; i + 1u < enc->temporal_entries; ++i) {
        giff_u16 best;

        best = i;
        for (j = (giff_u16)(i + 1u); j < enc->temporal_entries; ++j) {
            if (enc->temporal_weights[j] > enc->temporal_weights[best]) {
                best = j;
            }
        }

        if (best != i) {
            giff_rgb8 color_tmp;
            giff_u32 weight_tmp;

            color_tmp = enc->temporal_palette[i];
            enc->temporal_palette[i] = enc->temporal_palette[best];
            enc->temporal_palette[best] = color_tmp;

            weight_tmp = enc->temporal_weights[i];
            enc->temporal_weights[i] = enc->temporal_weights[best];
            enc->temporal_weights[best] = weight_tmp;
        }
    }
}

static giff_u16 giff_encoder_temporal_window_capacity(const giff_encoder* enc)
{
    giff_u16 cap;

    if (enc == 0) {
        return 0u;
    }

    cap = (giff_u16)enc->cfg.temporal_window_frames;
    if (cap == 0u) {
        cap = (giff_u16)GIFF_DEFAULT_TEMPORAL_WINDOW;
    }
    if (cap > (giff_u16)GIFF_TEMPORAL_WINDOW_MAX) {
        cap = (giff_u16)GIFF_TEMPORAL_WINDOW_MAX;
    }
    return cap;
}

static giff_rgb8* giff_encoder_window_palette_slot(giff_encoder* enc, giff_u16 slot)
{
    if (enc == 0 || enc->temporal_window_palette == 0) {
        return 0;
    }
    return enc->temporal_window_palette + (giff_u32)slot * 256u;
}

static giff_u32* giff_encoder_window_hist_slot(giff_encoder* enc, giff_u16 slot)
{
    if (enc == 0 || enc->temporal_window_hist == 0) {
        return 0;
    }
    return enc->temporal_window_hist + (giff_u32)slot * 256u;
}

static void giff_encoder_temporal_window_clear(giff_encoder* enc)
{
    giff_u16 cap;

    if (enc == 0) {
        return;
    }

    cap = giff_encoder_temporal_window_capacity(enc);
    enc->temporal_window_count = 0u;
    enc->temporal_window_head = 0u;
    enc->frame_window_used = 0u;
    enc->temporal_window_similarity_q8 = 0u;

    if (enc->temporal_window_palette != 0 && cap != 0u) {
        giff_mem_zero(enc->temporal_window_palette,
                      (giff_u32)cap * 256u * (giff_u32)sizeof(giff_rgb8));
    }
    if (enc->temporal_window_hist != 0 && cap != 0u) {
        giff_mem_zero(enc->temporal_window_hist,
                      (giff_u32)cap * 256u * (giff_u32)sizeof(giff_u32));
    }
    giff_mem_zero(enc->temporal_window_entries,
                  (giff_u32)sizeof(enc->temporal_window_entries));
    giff_mem_zero(enc->temporal_window_has_transparency,
                  (giff_u32)sizeof(enc->temporal_window_has_transparency));
    giff_mem_zero(enc->temporal_window_transparency_index,
                  (giff_u32)sizeof(enc->temporal_window_transparency_index));
}

static void giff_encoder_temporal_accumulate(
    giff_encoder* enc,
    const giff_rgb8* color,
    giff_u32 weight
)
{
    giff_u16 slot;
    giff_u16 best_slot;
    giff_u32 best_distance;

    if (enc == 0 || color == 0 || weight == 0u) {
        return;
    }

    best_slot = 0xFFFFu;
    best_distance = GIFF_TEMPORAL_COLOR_THRESHOLD + 1u;
    for (slot = 0u; slot < enc->temporal_entries; ++slot) {
        giff_u32 distance;

        distance = giff_encoder_color_distance_sq(&enc->temporal_palette[slot], color);
        if (distance == 0u) {
            best_slot = slot;
            best_distance = 0u;
            break;
        }
        if (distance > GIFF_TEMPORAL_COLOR_THRESHOLD) {
            continue;
        }
        if (best_slot == 0xFFFFu || distance < best_distance) {
            best_slot = slot;
            best_distance = distance;
        }
    }

    if (best_slot == 0xFFFFu) {
        if (enc->temporal_entries >= 256u) {
            return;
        }
        best_slot = enc->temporal_entries;
        enc->temporal_palette[best_slot] = *color;
        enc->temporal_weights[best_slot] = 0u;
        enc->temporal_entries = (giff_u16)(enc->temporal_entries + 1u);
    } else if (best_distance != 0u) {
        enc->temporal_palette[best_slot].r = (giff_u8)(((giff_u32)enc->temporal_palette[best_slot].r * 3u + (giff_u32)color->r + 2u) / 4u);
        enc->temporal_palette[best_slot].g = (giff_u8)(((giff_u32)enc->temporal_palette[best_slot].g * 3u + (giff_u32)color->g + 2u) / 4u);
        enc->temporal_palette[best_slot].b = (giff_u8)(((giff_u32)enc->temporal_palette[best_slot].b * 3u + (giff_u32)color->b + 2u) / 4u);
    }

    if (0xFFFFFFFFu - enc->temporal_weights[best_slot] < weight) {
        enc->temporal_weights[best_slot] = 0xFFFFFFFFu;
    } else {
        enc->temporal_weights[best_slot] += weight;
    }
}

static void giff_encoder_temporal_rebuild_from_window(giff_encoder* enc)
{
    giff_u16 cap;
    giff_u16 used;
    giff_u16 age;
    giff_u8 shift;

    if (enc == 0) {
        return;
    }

    enc->temporal_entries = 0u;
    enc->temporal_valid = 0u;
    enc->temporal_has_transparency = 0u;
    enc->temporal_transparency_index = 0u;
    enc->frame_window_used = 0u;
    enc->temporal_window_similarity_q8 = 0u;
    giff_mem_zero(enc->temporal_weights, (giff_u32)sizeof(enc->temporal_weights));

    if (!enc->cfg.enable_cross_frame_cluster) {
        return;
    }

    cap = giff_encoder_temporal_window_capacity(enc);
    used = enc->temporal_window_count;
    if (cap == 0u || used == 0u || enc->temporal_window_palette == 0 || enc->temporal_window_hist == 0) {
        return;
    }

    shift = enc->cfg.temporal_decay_shift;
    if (shift == 0u) {
        shift = 2u;
    }

    for (age = 0u; age < used; ++age) {
        giff_u16 slot;
        giff_rgb8* palette_slot;
        giff_u32* hist_slot;
        giff_u16 entries;
        giff_u16 i;
        giff_u32 age_scale;
        giff_u8 has_transparency;
        giff_u8 transparency_index;

        slot = (giff_u16)((enc->temporal_window_head + cap - 1u - age) % cap);
        palette_slot = giff_encoder_window_palette_slot(enc, slot);
        hist_slot = giff_encoder_window_hist_slot(enc, slot);
        entries = enc->temporal_window_entries[slot];
        has_transparency = enc->temporal_window_has_transparency[slot];
        transparency_index = enc->temporal_window_transparency_index[slot];

        if (palette_slot == 0 || hist_slot == 0 || entries == 0u) {
            continue;
        }

        age_scale = (giff_u32)(used - age);
        if (age_scale == 0u) {
            age_scale = 1u;
        }

        for (i = 0u; i < entries; ++i) {
            giff_u32 weight;
            giff_u16 s;

            if (has_transparency && i == (giff_u16)transparency_index) {
                continue;
            }

            weight = hist_slot[i];
            if (weight == 0u) {
                continue;
            }

            for (s = 0u; s < age; ++s) {
                if (shift < 31u) {
                    weight -= (weight >> shift);
                }
                if (weight == 0u) {
                    break;
                }
            }
            if (weight == 0u) {
                weight = 1u;
            }

            if (0xFFFFFFFFu / age_scale < weight) {
                weight = 0xFFFFFFFFu;
            } else {
                weight *= age_scale;
            }
            giff_encoder_temporal_accumulate(enc, &palette_slot[i], weight);
        }

        if (age == 0u) {
            enc->temporal_has_transparency = has_transparency;
            enc->temporal_transparency_index = transparency_index;
        }
    }

    enc->temporal_valid = (enc->temporal_entries != 0u) ? 1u : 0u;
    enc->frame_window_used = (giff_u8)used;
    enc->temporal_window_similarity_q8 = (giff_u16)(((giff_u32)used << 8u) / (giff_u32)cap);
    giff_encoder_temporal_sort(enc);
}

static void giff_encoder_capture_previous_palette(
    giff_encoder* enc,
    const giff_rgb8* palette,
    giff_u16 entries,
    giff_u8 has_transparency,
    giff_u8 transparency_index
)
{
    if (enc == 0 || palette == 0 || entries == 0u) {
        return;
    }

    giff_mem_copy(enc->prev_palette, palette, (giff_u32)entries * (giff_u32)sizeof(giff_rgb8));
    enc->prev_palette_entries = entries;
    enc->prev_palette_valid = 1u;
    enc->prev_has_transparency = has_transparency;
    enc->prev_transparency_index = transparency_index;
}

static void giff_encoder_update_temporal_model(
    giff_encoder* enc,
    const giff_rgb8* palette,
    giff_u16 entries,
    const giff_u32* hist,
    giff_u8 has_transparency,
    giff_u8 transparency_index
)
{
    giff_u16 cap;
    giff_u16 slot;
    giff_rgb8* palette_slot;
    giff_u32* hist_slot;
    giff_u16 i;

    if (enc == 0 || palette == 0 || hist == 0) {
        return;
    }

    if (!enc->cfg.enable_cross_frame_cluster) {
        enc->temporal_entries = 0u;
        enc->temporal_valid = 0u;
        enc->temporal_window_count = 0u;
        enc->temporal_window_head = 0u;
        return;
    }

    cap = giff_encoder_temporal_window_capacity(enc);
    if (cap == 0u || enc->temporal_window_palette == 0 || enc->temporal_window_hist == 0) {
        return;
    }

    slot = enc->temporal_window_head;
    if (slot >= cap) {
        slot = 0u;
    }

    palette_slot = giff_encoder_window_palette_slot(enc, slot);
    hist_slot = giff_encoder_window_hist_slot(enc, slot);
    if (palette_slot == 0 || hist_slot == 0) {
        return;
    }

    giff_mem_zero(palette_slot, 256u * (giff_u32)sizeof(giff_rgb8));
    giff_mem_zero(hist_slot, 256u * (giff_u32)sizeof(giff_u32));

    if (entries != 0u) {
        giff_mem_copy(palette_slot, palette, (giff_u32)entries * (giff_u32)sizeof(giff_rgb8));
    }
    for (i = 0u; i < entries; ++i) {
        hist_slot[i] = hist[i];
    }

    enc->temporal_window_entries[slot] = entries;
    enc->temporal_window_has_transparency[slot] = has_transparency;
    enc->temporal_window_transparency_index[slot] = transparency_index;
    enc->temporal_window_head = (giff_u8)((slot + 1u) % cap);
    if (enc->temporal_window_count < cap) {
        enc->temporal_window_count = (giff_u8)(enc->temporal_window_count + 1u);
    }

    giff_encoder_temporal_rebuild_from_window(enc);
}

static giff_u16 giff_encoder_build_dense_palette(
    giff_encoder* enc,
    const giff_rgb8* src_palette,
    giff_u16 src_entries,
    const giff_u32* hist,
    giff_u8 has_transparency,
    giff_u8 transparent_index,
    giff_rgb8* dst_palette,
    giff_u8* remap,
    giff_u8* out_has_transparency,
    giff_u8* out_transparency_index,
    giff_u8* out_sorted,
    giff_u8* out_changed,
    giff_u16* out_similarity_q8,
    giff_u8* out_used_temporal
)
{
    giff_u8 assigned[256];
    giff_u16 i;
    giff_u16 dst;
    giff_u8 keep_transparency;
    giff_u8 changed;
    giff_u16 used_total;
    giff_u16 temporal_matches;
    giff_u8 used_temporal;
    giff_u8 sorted;

    for (i = 0u; i < 256u; ++i) {
        remap[i] = 0u;
        assigned[i] = 0u;
    }

    dst = 0u;
    changed = 0u;
    keep_transparency = 0u;
    used_total = 0u;
    temporal_matches = 0u;
    used_temporal = 0u;
    sorted = 1u;

    for (i = 0u; i < src_entries; ++i) {
        if (hist[i] != 0u && (!has_transparency || i != (giff_u16)transparent_index)) {
            used_total = (giff_u16)(used_total + 1u);
        }
    }

    if (has_transparency && (giff_u16)transparent_index < src_entries && hist[transparent_index] != 0u) {
        keep_transparency = 1u;
        assigned[transparent_index] = 1u;
        remap[transparent_index] = 0u;
        dst_palette[0] = src_palette[transparent_index];
        if (transparent_index != 0u) {
            changed = 1u;
        }
        dst = 1u;
    }

    if (enc != 0 && enc->cfg.enable_temporal_remap && enc->prev_palette_valid && enc->frames_written != 0u) {
        for (i = 0u; i < enc->prev_palette_entries; ++i) {
            giff_u16 best_index;

            if (enc->prev_has_transparency && i == (giff_u16)enc->prev_transparency_index) {
                continue;
            }

            best_index = giff_encoder_prev_match(
                enc,
                src_palette,
                src_entries,
                hist,
                assigned,
                &enc->prev_palette[i],
                GIFF_TEMPORAL_COLOR_THRESHOLD
            );

            if (best_index == 0xFFFFu) {
                continue;
            }

            assigned[best_index] = 1u;
            remap[best_index] = (giff_u8)dst;
            dst_palette[dst] = src_palette[best_index];
            if (best_index != dst) {
                changed = 1u;
            }
            dst = (giff_u16)(dst + 1u);
            temporal_matches = (giff_u16)(temporal_matches + 1u);
            used_temporal = 1u;
            sorted = 0u;
        }
    }

    while (1) {
        giff_u16 best_index;
        giff_u32 best_count;
        giff_u32 best_temporal;
        giff_u8 best_exact;

        best_index = 0xFFFFu;
        best_count = 0u;
        best_temporal = 0u;
        best_exact = 0u;

        for (i = 0u; i < src_entries; ++i) {
            giff_u32 temporal_weight;
            giff_u8 exact_match;

            if (assigned[i] || hist[i] == 0u) {
                continue;
            }

            exact_match = 0u;
            temporal_weight = giff_encoder_temporal_weight_for_color(enc, &src_palette[i], &exact_match);

            if (best_index == 0xFFFFu ||
                temporal_weight > best_temporal ||
                (temporal_weight == best_temporal && exact_match > best_exact) ||
                (temporal_weight == best_temporal && exact_match == best_exact && hist[i] > best_count) ||
                (temporal_weight == best_temporal && exact_match == best_exact && hist[i] == best_count && i < best_index)) {
                best_index = i;
                best_count = hist[i];
                best_temporal = temporal_weight;
                best_exact = exact_match;
            }
        }

        if (best_index == 0xFFFFu) {
            break;
        }

        assigned[best_index] = 1u;
        remap[best_index] = (giff_u8)dst;
        dst_palette[dst] = src_palette[best_index];
        if (best_index != dst) {
            changed = 1u;
        }
        if (best_temporal != 0u) {
            temporal_matches = (giff_u16)(temporal_matches + 1u);
            used_temporal = 1u;
            sorted = 0u;
        }
        dst = (giff_u16)(dst + 1u);
    }

    if (dst == 0u) {
        dst_palette[0] = src_palette[0];
        dst = 1u;
        remap[0] = 0u;
    }

    if (out_has_transparency != 0) {
        *out_has_transparency = keep_transparency;
    }
    if (out_transparency_index != 0) {
        *out_transparency_index = keep_transparency ? 0u : 0u;
    }
    if (out_sorted != 0) {
        *out_sorted = (dst > 1u && sorted) ? 1u : 0u;
    }
    if (out_changed != 0) {
        *out_changed = changed;
    }
    if (out_similarity_q8 != 0) {
        if (used_total != 0u) {
            *out_similarity_q8 = (giff_u16)(((giff_u32)temporal_matches << 8u) / (giff_u32)used_total);
        } else {
            *out_similarity_q8 = 0u;
        }
    }
    if (out_used_temporal != 0) {
        *out_used_temporal = used_temporal;
    }

    return dst;
}

static giff_u16 giff_encoder_build_frequency_palette(
    giff_encoder* enc,
    const giff_rgb8* src_palette,
    giff_u16 src_entries,
    const giff_u32* hist,
    giff_u8 has_transparency,
    giff_u8 transparent_index,
    giff_rgb8* dst_palette,
    giff_u8* remap,
    giff_u8* out_has_transparency,
    giff_u8* out_transparency_index,
    giff_u8* out_sorted,
    giff_u8* out_changed,
    giff_u16* out_similarity_q8,
    giff_u8* out_used_temporal
)
{
    giff_u8 assigned[256];
    giff_u16 i;
    giff_u16 dst;
    giff_u8 keep_transparency;
    giff_u8 changed;
    giff_u16 used_total;
    giff_u16 temporal_matches;
    giff_u8 used_temporal;

    for (i = 0u; i < 256u; ++i) {
        assigned[i] = 0u;
        remap[i] = 0u;
    }

    dst = 0u;
    keep_transparency = 0u;
    changed = 0u;
    used_total = 0u;
    temporal_matches = 0u;
    used_temporal = 0u;

    for (i = 0u; i < src_entries; ++i) {
        if (hist[i] != 0u && (!has_transparency || i != (giff_u16)transparent_index)) {
            used_total = (giff_u16)(used_total + 1u);
        }
    }

    if (has_transparency && (giff_u16)transparent_index < src_entries && hist[transparent_index] != 0u) {
        keep_transparency = 1u;
        assigned[transparent_index] = 1u;
        remap[transparent_index] = 0u;
        dst_palette[0] = src_palette[transparent_index];
        if (transparent_index != 0u) {
            changed = 1u;
        }
        dst = 1u;
    }

    while (1) {
        giff_u16 best_index;
        giff_u32 best_count;
        giff_u32 best_temporal;
        giff_u8 best_exact;

        best_index = 0xFFFFu;
        best_count = 0u;
        best_temporal = 0u;
        best_exact = 0u;

        for (i = 0u; i < src_entries; ++i) {
            giff_u32 temporal_weight;
            giff_u8 exact_match;

            if (assigned[i] || hist[i] == 0u) {
                continue;
            }

            exact_match = 0u;
            temporal_weight = giff_encoder_temporal_weight_for_color(enc, &src_palette[i], &exact_match);
            if (best_index == 0xFFFFu ||
                hist[i] > best_count ||
                (hist[i] == best_count && temporal_weight > best_temporal) ||
                (hist[i] == best_count && temporal_weight == best_temporal && exact_match > best_exact) ||
                (hist[i] == best_count && temporal_weight == best_temporal && exact_match == best_exact && i < best_index)) {
                best_index = i;
                best_count = hist[i];
                best_temporal = temporal_weight;
                best_exact = exact_match;
            }
        }

        if (best_index == 0xFFFFu) {
            break;
        }

        assigned[best_index] = 1u;
        remap[best_index] = (giff_u8)dst;
        dst_palette[dst] = src_palette[best_index];
        if (best_index != dst) {
            changed = 1u;
        }
        if (best_temporal != 0u) {
            temporal_matches = (giff_u16)(temporal_matches + 1u);
            used_temporal = 1u;
        }
        dst = (giff_u16)(dst + 1u);
    }

    if (dst == 0u) {
        dst_palette[0] = src_palette[0];
        remap[0] = 0u;
        dst = 1u;
    }

    if (out_has_transparency != 0) {
        *out_has_transparency = keep_transparency;
    }
    if (out_transparency_index != 0) {
        *out_transparency_index = keep_transparency ? 0u : 0u;
    }
    if (out_sorted != 0) {
        *out_sorted = (dst > 1u) ? 1u : 0u;
    }
    if (out_changed != 0) {
        *out_changed = changed;
    }
    if (out_similarity_q8 != 0) {
        if (used_total != 0u) {
            *out_similarity_q8 = (giff_u16)(((giff_u32)temporal_matches << 8u) / (giff_u32)used_total);
        } else {
            *out_similarity_q8 = 0u;
        }
    }
    if (out_used_temporal != 0) {
        *out_used_temporal = used_temporal;
    }

    return dst;
}

static giff_u8 giff_encoder_bits_for_max_index(giff_u16 max_index)
{
    giff_u8 bits;

    bits = giff_palette_bits((giff_u16)(max_index + 1u));
    if (bits < 2u) {
        bits = 2u;
    }
    return bits;
}

static giff_u32 giff_encoder_transition_q8(giff_u32 same_adjacent, giff_u32 pixels)
{
    if (pixels <= 1u) {
        return 0u;
    }
    return (((pixels - 1u) - same_adjacent) << 8u) / (pixels - 1u);
}

static giff_u32 giff_encoder_estimate_candidate_cost(
    const giff_encoder* enc,
    const giff_palette_candidate* candidate,
    giff_u32 pixels,
    giff_u16 entropy_q8,
    giff_u32 same_adjacent,
    giff_u16 used_count
)
{
    giff_u32 cost;
    giff_u32 transition_q8;
    giff_u32 texture_bias;
    giff_u32 remap_penalty;
    giff_u32 temporal_bonus;
    giff_u16 window_similarity_q8;
    giff_u16 segment_similarity_q8;
    giff_u16 cross_frame_q8;
    giff_u16 segment_threshold_q8;
    giff_u32 break_penalty;

    if (candidate == 0 || candidate->entries == 0u || candidate->min_code_bits == 0u) {
        return 0xFFFFFFFFu;
    }

    cost = ((pixels * (giff_u32)candidate->min_code_bits) + 7u) / 8u;
    if (candidate->mode != (giff_u8)GIFF_PALETTE_DECISION_KEEP) {
        cost += giff_encoder_palette_table_bytes(candidate->entries);
    }

    transition_q8 = giff_encoder_transition_q8(same_adjacent, pixels);
    texture_bias = ((giff_u32)entropy_q8 * (giff_u32)candidate->min_code_bits + 255u) / 256u;
    texture_bias += ((transition_q8 * (giff_u32)candidate->min_code_bits) + 511u) / 512u;
    if (used_count > candidate->entries && candidate->mode != (giff_u8)GIFF_PALETTE_DECISION_KEEP) {
        texture_bias += (giff_u32)(used_count - candidate->entries) * 2u;
    }
    if (candidate->mode == (giff_u8)GIFF_PALETTE_DECISION_KEEP && used_count + 4u < candidate->entries) {
        texture_bias += (giff_u32)(candidate->entries - used_count);
    }
    cost += texture_bias;

    remap_penalty = 0u;
    if (candidate->changed) {
        remap_penalty += (pixels + 63u) / 64u;
        remap_penalty += (giff_u32)candidate->entries * 2u;
    }
    if (candidate->mode == (giff_u8)GIFF_PALETTE_DECISION_LOCAL && enc != 0 && enc->frames_written != 0u) {
        remap_penalty += 4u;
    }
    cost += remap_penalty;

    temporal_bonus = 0u;
    if (candidate->used_temporal) {
        temporal_bonus += ((giff_u32)candidate->similarity_q8 * ((pixels > 128u) ? 128u : pixels) + 255u) / 256u;
        temporal_bonus += (giff_u32)candidate->similarity_q8 / 8u;
    }
    if (candidate->entries <= 8u && candidate->mode != (giff_u8)GIFF_PALETTE_DECISION_KEEP) {
        temporal_bonus += 4u;
    }

    window_similarity_q8 = 0u;
    segment_similarity_q8 = 0u;
    cross_frame_q8 = candidate->similarity_q8;
    break_penalty = 0u;

    if (enc != 0) {
        window_similarity_q8 = giff_encoder_window_similarity_q8(
            enc,
            candidate->palette,
            candidate->entries,
            candidate->has_transparency,
            candidate->transparency_index,
            0
        );
        segment_similarity_q8 = giff_encoder_segment_similarity_q8_weighted(
            enc,
            candidate->palette,
            candidate->entries,
            candidate->has_transparency,
            candidate->transparency_index,
            0
        );
        if (window_similarity_q8 > cross_frame_q8) {
            cross_frame_q8 = window_similarity_q8;
        }
        if (segment_similarity_q8 > cross_frame_q8) {
            cross_frame_q8 = segment_similarity_q8;
        }

        temporal_bonus += ((giff_u32)cross_frame_q8 * ((pixels > 192u) ? 192u : pixels) + 255u) / 256u;
        temporal_bonus += (giff_u32)enc->frame_window_used * (giff_u32)cross_frame_q8 / 32u;

        if (candidate->mode == (giff_u8)GIFF_PALETTE_DECISION_SEGMENT_SHARE) {
            temporal_bonus += (giff_u32)(enc->segment_frame_count + 1u) * 12u;
        }

        segment_threshold_q8 = giff_encoder_segment_threshold(enc);
        if (enc->segment_valid && candidate->mode != (giff_u8)GIFF_PALETTE_DECISION_SEGMENT_SHARE &&
            segment_similarity_q8 >= segment_threshold_q8) {
            break_penalty += (giff_u32)(enc->segment_frame_count + 1u) * 6u;
        }

        if (candidate->mode == (giff_u8)GIFF_PALETTE_DECISION_KEEP && enc->prev_palette_valid) {
            temporal_bonus += 2u;
        }

        if (candidate->mode == (giff_u8)GIFF_PALETTE_DECISION_KEEP &&
            enc->segment_valid && used_count + 8u < candidate->entries && segment_similarity_q8 < segment_threshold_q8) {
            break_penalty += 8u;
        }
    }

    cost += break_penalty;

    if (cost > temporal_bonus) {
        cost -= temporal_bonus;
    } else {
        cost = 0u;
    }
    return cost;
}

static void giff_encoder_prepare_keep_candidate(
    giff_palette_candidate* candidate,
    const giff_rgb8* source_palette,
    giff_u16 source_entries,
    giff_u16 max_index,
    giff_u8 has_transparency,
    giff_u8 transparency_index
)
{
    if (candidate == 0) {
        return;
    }

    giff_mem_zero(candidate, (giff_u32)sizeof(*candidate));
    if (source_palette != 0 && source_entries != 0u) {
        giff_mem_copy(candidate->palette,
                      source_palette,
                      (giff_u32)source_entries * (giff_u32)sizeof(giff_rgb8));
    }
    candidate->entries = source_entries;
    candidate->has_transparency = has_transparency;
    candidate->transparency_index = transparency_index;
    candidate->mode = (giff_u8)GIFF_PALETTE_DECISION_KEEP;
    candidate->min_code_bits = giff_encoder_bits_for_max_index(max_index);
}

static void giff_encoder_prepare_local_candidate(
    giff_encoder* enc,
    giff_palette_candidate* candidate,
    const giff_rgb8* src_palette,
    giff_u16 src_entries,
    const giff_u32* hist,
    giff_u8 has_transparency,
    giff_u8 transparency_index,
    giff_u8 temporal_mode
)
{
    giff_u16 entries;

    if (candidate == 0) {
        return;
    }

    giff_mem_zero(candidate, (giff_u32)sizeof(*candidate));
    if (src_palette == 0 || src_entries == 0u || hist == 0) {
        return;
    }

    if (temporal_mode) {
        entries = giff_encoder_build_dense_palette(
            enc,
            src_palette,
            src_entries,
            hist,
            has_transparency,
            transparency_index,
            candidate->palette,
            candidate->remap,
            &candidate->has_transparency,
            &candidate->transparency_index,
            &candidate->sorted,
            &candidate->changed,
            &candidate->similarity_q8,
            &candidate->used_temporal
        );
        candidate->mode = (giff_u8)GIFF_PALETTE_DECISION_TEMPORAL_REMAP;
    } else {
        entries = giff_encoder_build_frequency_palette(
            enc,
            src_palette,
            src_entries,
            hist,
            has_transparency,
            transparency_index,
            candidate->palette,
            candidate->remap,
            &candidate->has_transparency,
            &candidate->transparency_index,
            &candidate->sorted,
            &candidate->changed,
            &candidate->similarity_q8,
            &candidate->used_temporal
        );
        candidate->mode = (giff_u8)GIFF_PALETTE_DECISION_LOCAL;
    }

    candidate->entries = entries;
    if (entries != 0u) {
        candidate->min_code_bits = giff_encoder_bits_for_max_index((giff_u16)(entries - 1u));
    }
}

static void giff_encoder_prepare_segment_candidate(
    giff_encoder* enc,
    giff_palette_candidate* candidate,
    const giff_rgb8* src_palette,
    giff_u16 src_entries,
    const giff_u32* hist,
    giff_u8 has_transparency,
    giff_u8 transparency_index
)
{
    giff_u16 i;
    giff_u32 total_weight;
    giff_u32 matched_weight;
    giff_u16 threshold_q8;

    if (candidate == 0) {
        return;
    }

    giff_mem_zero(candidate, (giff_u32)sizeof(*candidate));
    if (enc == 0 || src_palette == 0 || hist == 0) {
        return;
    }
    if (!enc->cfg.enable_segment_sharing || !enc->segment_valid || enc->segment_palette_entries == 0u) {
        return;
    }

    threshold_q8 = giff_encoder_segment_threshold(enc);
    giff_mem_copy(candidate->palette,
                  enc->segment_palette,
                  (giff_u32)enc->segment_palette_entries * (giff_u32)sizeof(giff_rgb8));
    candidate->entries = enc->segment_palette_entries;
    candidate->has_transparency = has_transparency ? enc->segment_has_transparency : 0u;
    candidate->transparency_index = candidate->has_transparency ? enc->segment_transparency_index : 0u;
    candidate->mode = (giff_u8)GIFF_PALETTE_DECISION_SEGMENT_SHARE;
    candidate->sorted = 0u;
    candidate->used_temporal = 1u;
    candidate->changed = 0u;

    if (has_transparency && !enc->segment_has_transparency) {
        candidate->entries = 0u;
        return;
    }

    total_weight = 0u;
    matched_weight = 0u;
    for (i = 0u; i < 256u; ++i) {
        candidate->remap[i] = 0u;
    }

    if (has_transparency && (giff_u16)transparency_index < src_entries) {
        candidate->remap[transparency_index] = enc->segment_transparency_index;
        if (transparency_index != enc->segment_transparency_index) {
            candidate->changed = 1u;
        }
        total_weight += hist[transparency_index];
        matched_weight += hist[transparency_index];
    }

    for (i = 0u; i < src_entries; ++i) {
        giff_u32 weight;
        giff_u16 j;
        giff_u16 best_index;
        giff_u32 best_distance;

        if (has_transparency && i == (giff_u16)transparency_index) {
            continue;
        }

        weight = hist[i];
        if (weight == 0u) {
            continue;
        }

        total_weight += weight;
        best_index = 0xFFFFu;
        best_distance = GIFF_SEGMENT_COLOR_THRESHOLD + 1u;
        for (j = 0u; j < enc->segment_palette_entries; ++j) {
            giff_u32 distance;

            if (enc->segment_has_transparency && j == (giff_u16)enc->segment_transparency_index) {
                continue;
            }

            distance = giff_encoder_color_distance_sq(&src_palette[i], &enc->segment_palette[j]);
            if (distance < best_distance) {
                best_distance = distance;
                best_index = j;
                if (distance == 0u) {
                    break;
                }
            }
        }

        if (best_index == 0xFFFFu || best_distance > GIFF_SEGMENT_COLOR_THRESHOLD) {
            candidate->entries = 0u;
            return;
        }

        candidate->remap[i] = (giff_u8)best_index;
        if (best_index != i || best_distance != 0u) {
            candidate->changed = 1u;
        }
        matched_weight += weight;
    }

    if (total_weight == 0u) {
        candidate->entries = 0u;
        return;
    }

    candidate->similarity_q8 = (giff_u16)((matched_weight << 8u) / total_weight);
    if (candidate->similarity_q8 < threshold_q8 && enc->segment_frame_count >= giff_encoder_segment_max_frames(enc)) {
        candidate->entries = 0u;
        return;
    }

    candidate->min_code_bits = giff_encoder_bits_for_max_index((giff_u16)(candidate->entries - 1u));
}

static void giff_encoder_update_segment_model(
    giff_encoder* enc,
    const giff_rgb8* palette,
    giff_u16 entries,
    const giff_u32* hist,
    giff_u8 has_transparency,
    giff_u8 transparency_index
)
{
    giff_u16 i;
    giff_u16 similarity_q8;
    giff_u16 threshold_q8;

    if (enc == 0 || palette == 0 || hist == 0) {
        return;
    }

    if (!enc->cfg.enable_segment_sharing || entries == 0u) {
        enc->segment_valid = 0u;
        enc->segment_palette_entries = 0u;
        enc->segment_frame_count = 0u;
        enc->segment_similarity_q8 = 0u;
        enc->frame_segment_break = 0u;
        return;
    }

    threshold_q8 = giff_encoder_segment_threshold(enc);
    similarity_q8 = giff_encoder_segment_similarity_q8_weighted(
        enc,
        palette,
        entries,
        has_transparency,
        transparency_index,
        hist
    );
    enc->segment_similarity_q8 = similarity_q8;

    if (enc->frame_cost_mode == (giff_u8)GIFF_PALETTE_DECISION_SEGMENT_SHARE &&
        enc->segment_valid && enc->segment_palette_entries != 0u) {
        enc->frame_segment_break = 0u;
        for (i = 0u; i < entries && i < 256u; ++i) {
            if (0xFFFFFFFFu - enc->segment_hist[i] < hist[i]) {
                enc->segment_hist[i] = 0xFFFFFFFFu;
            } else {
                enc->segment_hist[i] += hist[i];
            }
        }
        if (enc->segment_frame_count < 0xFFFFu) {
            enc->segment_frame_count = (giff_u16)(enc->segment_frame_count + 1u);
        }
        return;
    }

    if (enc->frame_cost_mode == (giff_u8)GIFF_PALETTE_DECISION_KEEP) {
        enc->frame_segment_break = 0u;
        if (enc->segment_valid && similarity_q8 + 32u < threshold_q8) {
            enc->segment_valid = 0u;
            enc->segment_palette_entries = 0u;
            enc->segment_frame_count = 0u;
            giff_mem_zero(enc->segment_hist, (giff_u32)sizeof(enc->segment_hist));
            enc->frame_segment_break = 1u;
        }
        return;
    }

    if (enc->segment_valid && similarity_q8 >= threshold_q8 &&
        enc->segment_frame_count < giff_encoder_segment_max_frames(enc) &&
        entries == enc->segment_palette_entries) {
        enc->frame_segment_break = 0u;
        for (i = 0u; i < entries && i < 256u; ++i) {
            if (0xFFFFFFFFu - enc->segment_hist[i] < hist[i]) {
                enc->segment_hist[i] = 0xFFFFFFFFu;
            } else {
                enc->segment_hist[i] += hist[i];
            }
        }
        if (enc->segment_frame_count < 0xFFFFu) {
            enc->segment_frame_count = (giff_u16)(enc->segment_frame_count + 1u);
        }
        return;
    }

    enc->frame_segment_break = enc->segment_valid ? 1u : 0u;
    giff_mem_copy(enc->segment_palette, palette, (giff_u32)entries * (giff_u32)sizeof(giff_rgb8));
    if (entries < 256u) {
        giff_mem_zero(enc->segment_palette + entries,
                      (giff_u32)(256u - entries) * (giff_u32)sizeof(giff_rgb8));
    }
    giff_mem_zero(enc->segment_hist, (giff_u32)sizeof(enc->segment_hist));
    for (i = 0u; i < entries && i < 256u; ++i) {
        enc->segment_hist[i] = hist[i];
    }
    enc->segment_palette_entries = entries;
    enc->segment_has_transparency = has_transparency;
    enc->segment_transparency_index = transparency_index;
    enc->segment_frame_count = 1u;
    enc->segment_valid = 1u;
    enc->segment_similarity_q8 = 256u;
}

static void giff_encoder_apply_palette_candidate(
    giff_encoder* enc,
    giff_frame_info* frame,
    const giff_palette_candidate* candidate
)
{
    if (enc == 0 || frame == 0 || candidate == 0) {
        return;
    }

    enc->frame_cost_mode = candidate->mode;
    enc->frame_cost_chosen = candidate->estimated_cost;
    enc->frame_segment_shared = (candidate->mode == (giff_u8)GIFF_PALETTE_DECISION_SEGMENT_SHARE) ? 1u : 0u;

    if (candidate->mode == (giff_u8)GIFF_PALETTE_DECISION_KEEP) {
        return;
    }

    giff_encoder_apply_remap(enc, candidate->remap, frame);
    giff_mem_copy(enc->local_palette,
                  candidate->palette,
                  (giff_u32)candidate->entries * (giff_u32)sizeof(giff_rgb8));
    enc->local_palette_set = 1u;
    enc->local_palette_entries = candidate->entries;
    enc->local_has_transparency = candidate->has_transparency;
    enc->local_transparency_index = candidate->transparency_index;
    frame->has_local_palette = 1u;
    frame->local_palette_entries = candidate->entries;
    frame->has_transparency = candidate->has_transparency;
    frame->transparency_index = candidate->transparency_index;
    enc->frame_sorted_local_palette = candidate->sorted;
    enc->frame_sparse_local_remap = candidate->changed ? 1u : 0u;
    enc->frame_auto_local_palette = 1u;
    if (candidate->used_temporal) {
        enc->frame_temporal_palette = 1u;
        enc->frame_clustered_palette = 1u;
    }
}

static void giff_encoder_select_palette_strategy(
    giff_encoder* enc,
    giff_frame_info* frame,
    const giff_u32* hist,
    giff_u16* io_used_count,
    giff_u16* io_max_index,
    giff_u32 same_adjacent,
    giff_u16 entropy_q8,
    giff_u32 pixels
)
{
    const giff_rgb8* src_palette;
    giff_u16 src_entries;
    giff_u8 src_has_transparency;
    giff_u8 src_transparency_index;
    giff_palette_candidate keep_candidate;
    giff_palette_candidate local_candidate;
    giff_palette_candidate temporal_candidate;
    giff_palette_candidate segment_candidate;
    const giff_palette_candidate* best_candidate;
    giff_u16 used_count;
    giff_u16 max_index;

    if (enc == 0 || frame == 0 || hist == 0 || io_used_count == 0 || io_max_index == 0) {
        return;
    }

    used_count = *io_used_count;
    max_index = *io_max_index;

    if (frame->has_local_palette) {
        src_palette = enc->local_palette;
        src_entries = frame->local_palette_entries;
    } else {
        src_palette = enc->global_palette;
        src_entries = enc->global_palette_entries;
    }
    src_has_transparency = frame->has_transparency;
    src_transparency_index = frame->transparency_index;

    giff_encoder_prepare_keep_candidate(
        &keep_candidate,
        src_palette,
        src_entries,
        max_index,
        src_has_transparency,
        src_transparency_index
    );
    keep_candidate.estimated_cost = giff_encoder_estimate_candidate_cost(
        enc,
        &keep_candidate,
        pixels,
        entropy_q8,
        same_adjacent,
        used_count
    );

    giff_encoder_prepare_local_candidate(
        enc,
        &local_candidate,
        src_palette,
        src_entries,
        hist,
        src_has_transparency,
        src_transparency_index,
        0u
    );
    local_candidate.estimated_cost = giff_encoder_estimate_candidate_cost(
        enc,
        &local_candidate,
        pixels,
        entropy_q8,
        same_adjacent,
        used_count
    );

    giff_encoder_prepare_local_candidate(
        enc,
        &temporal_candidate,
        src_palette,
        src_entries,
        hist,
        src_has_transparency,
        src_transparency_index,
        1u
    );
    temporal_candidate.estimated_cost = giff_encoder_estimate_candidate_cost(
        enc,
        &temporal_candidate,
        pixels,
        entropy_q8,
        same_adjacent,
        used_count
    );

    giff_encoder_prepare_segment_candidate(
        enc,
        &segment_candidate,
        src_palette,
        src_entries,
        hist,
        src_has_transparency,
        src_transparency_index
    );
    segment_candidate.estimated_cost = giff_encoder_estimate_candidate_cost(
        enc,
        &segment_candidate,
        pixels,
        entropy_q8,
        same_adjacent,
        used_count
    );

    enc->frame_cost_keep = keep_candidate.estimated_cost;
    enc->frame_cost_local = local_candidate.estimated_cost;
    enc->frame_cost_temporal = temporal_candidate.estimated_cost;
    enc->frame_cost_segment = segment_candidate.estimated_cost;
    enc->frame_cost_chosen = keep_candidate.estimated_cost;
    enc->frame_cost_mode = (giff_u8)GIFF_PALETTE_DECISION_KEEP;
    enc->temporal_similarity_q8 = temporal_candidate.similarity_q8;
    enc->segment_similarity_q8 = segment_candidate.similarity_q8;

    best_candidate = &keep_candidate;

    if (local_candidate.entries != 0u && local_candidate.estimated_cost + 4u < best_candidate->estimated_cost) {
        best_candidate = &local_candidate;
    }
    if (temporal_candidate.entries != 0u &&
        (temporal_candidate.estimated_cost + 2u < best_candidate->estimated_cost ||
         (temporal_candidate.estimated_cost <= best_candidate->estimated_cost &&
          temporal_candidate.similarity_q8 > local_candidate.similarity_q8))) {
        best_candidate = &temporal_candidate;
    }
    if (segment_candidate.entries != 0u &&
        (segment_candidate.estimated_cost + 2u < best_candidate->estimated_cost ||
         (segment_candidate.estimated_cost <= best_candidate->estimated_cost &&
          segment_candidate.similarity_q8 >= temporal_candidate.similarity_q8))) {
        best_candidate = &segment_candidate;
    }

    if (frame->has_local_palette && best_candidate != &keep_candidate &&
        keep_candidate.estimated_cost <= best_candidate->estimated_cost + 8u) {
        best_candidate = &keep_candidate;
    }

    if (segment_candidate.entries != 0u && enc->segment_valid &&
        segment_candidate.similarity_q8 >= giff_encoder_segment_threshold(enc) &&
        segment_candidate.estimated_cost <= best_candidate->estimated_cost + 4u) {
        best_candidate = &segment_candidate;
    }

    giff_encoder_apply_palette_candidate(enc, frame, best_candidate);

    if (best_candidate->mode != (giff_u8)GIFF_PALETTE_DECISION_KEEP) {
        *io_used_count = best_candidate->entries;
        if (best_candidate->entries != 0u) {
            *io_max_index = (giff_u16)(best_candidate->entries - 1u);
        } else {
            *io_max_index = 0u;
        }
    }
}

static void giff_encoder_choose_lzw_strategy(
    giff_encoder* enc,
    giff_u16 palette_entries,
    giff_u16 used_count,
    giff_u32 same_adjacent,
    giff_u32 dominant_count,
    giff_u16 entropy_q8,
    giff_u32 pixels
)
{
    giff_u32 transition_q8;
    giff_u32 dominant_q8;

    if (enc == 0) {
        return;
    }

    transition_q8 = 0u;
    if (pixels > 1u) {
        transition_q8 = (((pixels - 1u) - same_adjacent) << 8u) / (pixels - 1u);
    }

    dominant_q8 = 0u;
    if (pixels != 0u) {
        dominant_q8 = (dominant_count << 8u) / pixels;
    }

    enc->frame_used_colors = used_count;
    enc->frame_encoded_palette_entries = palette_entries;
    enc->frame_entropy_q8 = entropy_q8;

    if (entropy_q8 >= 1664u || (used_count >= 96u && transition_q8 >= 224u)) {
        enc->lzw_strategy = (giff_u8)GIFF_LZW_STRATEGY_AGGRESSIVE;
        enc->frame_reset_threshold = 1408u;
        enc->frame_stall_threshold = 80u;
        if (pixels >= 384u) {
            enc->frame_periodic_clear = (giff_u16)(pixels / 3u);
            if (enc->frame_periodic_clear < 256u) {
                enc->frame_periodic_clear = 256u;
            }
            if (enc->frame_periodic_clear > 1536u) {
                enc->frame_periodic_clear = 1536u;
            }
        } else {
            enc->frame_periodic_clear = 0u;
        }
    } else if (entropy_q8 >= 1216u || used_count >= 24u || transition_q8 >= 176u) {
        enc->lzw_strategy = (giff_u8)GIFF_LZW_STRATEGY_BALANCED;
        enc->frame_reset_threshold = 2496u;
        enc->frame_stall_threshold = 160u;
        enc->frame_periodic_clear = 0u;
    } else {
        enc->lzw_strategy = (giff_u8)GIFF_LZW_STRATEGY_COMPACT;
        enc->frame_reset_threshold = 3584u;
        enc->frame_stall_threshold = 320u;
        enc->frame_periodic_clear = 0u;
    }

    if (dominant_q8 >= 224u && used_count <= 4u) {
        enc->lzw_strategy = (giff_u8)GIFF_LZW_STRATEGY_COMPACT;
        enc->frame_reset_threshold = 3840u;
        enc->frame_stall_threshold = 384u;
        enc->frame_periodic_clear = 0u;
    }

    if (palette_entries <= 8u && enc->frame_reset_threshold > 2048u) {
        enc->frame_reset_threshold = 2048u;
    }
}


static giff_u16 giff_encoder_default_band_rows(
    const giff_encoder* enc,
    const giff_frame_info* frame
)
{
    giff_u16 rows;

    if (enc == 0 || frame == 0 || frame->height < 2u) {
        return 0u;
    }

    rows = (giff_u16)enc->cfg.lzw_band_rows;
    if (rows == 0u) {
        rows = (giff_u16)GIFF_DEFAULT_BAND_ROWS;
    }
    if (rows > frame->height) {
        rows = frame->height;
    }
    if (rows < 2u) {
        rows = 2u;
    }
    return rows;
}

static void giff_encoder_analyze_band(
    const giff_encoder* enc,
    const giff_frame_info* frame,
    giff_u16 palette_entries,
    giff_u16 seq_row_start,
    giff_u16 seq_row_end,
    giff_u32* hist,
    giff_u16* out_used_count,
    giff_u16* out_max_index,
    giff_u32* out_same_adjacent,
    giff_u32* out_dominant_count,
    giff_u16* out_entropy_q8,
    giff_u16* out_row_repeat_q8
)
{
    giff_u16 i;
    giff_u16 used_count;
    giff_u16 max_index;
    giff_u32 same_adjacent;
    giff_u32 dominant_count;
    giff_u32 pixels;
    giff_u8 prev_index;
    const giff_u8* prev_row;
    giff_u16 repeated_rows;
    giff_u16 total_rows;
    giff_u16 seq_row;

    if (hist == 0) {
        return;
    }

    for (i = 0u; i < 256u; ++i) {
        hist[i] = 0u;
    }

    used_count = 0u;
    max_index = 0u;
    same_adjacent = 0u;
    dominant_count = 0u;
    pixels = 0u;
    prev_index = 0u;
    prev_row = 0;
    repeated_rows = 0u;
    total_rows = 0u;

    if (enc == 0 || frame == 0 || enc->frame_indexed == 0 || seq_row_start >= seq_row_end) {
        if (out_used_count != 0) {
            *out_used_count = 0u;
        }
        if (out_max_index != 0) {
            *out_max_index = 0u;
        }
        if (out_same_adjacent != 0) {
            *out_same_adjacent = 0u;
        }
        if (out_dominant_count != 0) {
            *out_dominant_count = 0u;
        }
        if (out_entropy_q8 != 0) {
            *out_entropy_q8 = 0u;
        }
        if (out_row_repeat_q8 != 0) {
            *out_row_repeat_q8 = 0u;
        }
        return;
    }

    for (seq_row = seq_row_start; seq_row < seq_row_end; ++seq_row) {
        giff_u16 actual_row;
        const giff_u8* row;
        giff_u16 x;
        giff_u8 row_equal;

        actual_row = frame->interlaced ? giff_encoder_interlaced_row(seq_row, frame->height) : seq_row;
        row = enc->frame_indexed + (giff_u32)actual_row * (giff_u32)enc->frame_stride;
        row_equal = 1u;

        if (prev_row == 0) {
            row_equal = 0u;
        } else {
            for (x = 0u; x < frame->width; ++x) {
                if (prev_row[x] != row[x]) {
                    row_equal = 0u;
                    break;
                }
            }
        }
        if (row_equal) {
            repeated_rows = (giff_u16)(repeated_rows + 1u);
        }

        for (x = 0u; x < frame->width; ++x) {
            giff_u8 idx;

            idx = row[x];
            if ((giff_u16)idx >= palette_entries) {
                continue;
            }
            hist[idx] += 1u;
            if (hist[idx] == 1u) {
                used_count = (giff_u16)(used_count + 1u);
            }
            if (idx > max_index) {
                max_index = idx;
            }
            if (hist[idx] > dominant_count) {
                dominant_count = hist[idx];
            }
            if (pixels != 0u && idx == prev_index) {
                same_adjacent += 1u;
            }
            prev_index = idx;
            pixels += 1u;
        }

        prev_row = row;
        total_rows = (giff_u16)(total_rows + 1u);
    }

    if (out_used_count != 0) {
        *out_used_count = used_count;
    }
    if (out_max_index != 0) {
        *out_max_index = max_index;
    }
    if (out_same_adjacent != 0) {
        *out_same_adjacent = same_adjacent;
    }
    if (out_dominant_count != 0) {
        *out_dominant_count = dominant_count;
    }
    if (out_entropy_q8 != 0) {
        *out_entropy_q8 = giff_encoder_entropy_q8(hist, palette_entries, pixels);
    }
    if (out_row_repeat_q8 != 0) {
        if (total_rows > 1u) {
            *out_row_repeat_q8 = (giff_u16)(((giff_u32)repeated_rows << 8u) / (giff_u32)(total_rows - 1u));
        } else {
            *out_row_repeat_q8 = 0u;
        }
    }
}

static giff_result giff_encoder_apply_band_lzw(
    giff_encoder* enc,
    const giff_frame_info* frame,
    giff_u16 band_index,
    giff_u16 seq_row_start,
    giff_u16 seq_row_end,
    giff_u16* io_prev_entropy_q8,
    giff_u16* io_prev_row_repeat_q8,
    giff_u8* io_prev_strategy
)
{
    giff_u32 hist[256];
    giff_u16 used_count;
    giff_u16 max_index;
    giff_u32 same_adjacent;
    giff_u32 dominant_count;
    giff_u16 entropy_q8;
    giff_u16 row_repeat_q8;
    giff_u16 saved_used_colors;
    giff_u16 saved_encoded_palette_entries;
    giff_u16 saved_entropy_q8;
    giff_u8 old_strategy;
    giff_u8 should_break;
    giff_u32 band_pixels;
    giff_result rc;
    giff_u16 entropy_delta;
    giff_u16 row_repeat_delta;

    if (enc == 0 || frame == 0) {
        return GIFF_E_INVALID_ARGUMENT;
    }

    giff_encoder_analyze_band(
        enc,
        frame,
        enc->current_palette_entries,
        seq_row_start,
        seq_row_end,
        hist,
        &used_count,
        &max_index,
        &same_adjacent,
        &dominant_count,
        &entropy_q8,
        &row_repeat_q8
    );

    band_pixels = (giff_u32)(seq_row_end - seq_row_start) * (giff_u32)frame->width;
    saved_used_colors = enc->frame_used_colors;
    saved_encoded_palette_entries = enc->frame_encoded_palette_entries;
    saved_entropy_q8 = enc->frame_entropy_q8;
    old_strategy = enc->lzw_strategy;

    giff_encoder_choose_lzw_strategy(
        enc,
        enc->current_palette_entries,
        used_count,
        same_adjacent,
        dominant_count,
        entropy_q8,
        band_pixels
    );

    if (row_repeat_q8 >= 160u && used_count <= 16u) {
        enc->lzw_strategy = (giff_u8)GIFF_LZW_STRATEGY_COMPACT;
        enc->frame_reset_threshold = 3840u;
        enc->frame_stall_threshold = 384u;
        enc->frame_periodic_clear = 0u;
    } else if (entropy_q8 >= 1536u && row_repeat_q8 < 64u && used_count >= 32u) {
        enc->lzw_strategy = (giff_u8)GIFF_LZW_STRATEGY_AGGRESSIVE;
        if (band_pixels >= 256u) {
            enc->frame_periodic_clear = (giff_u16)(band_pixels / 2u);
        }
        if (enc->frame_periodic_clear < 128u && band_pixels >= 128u) {
            enc->frame_periodic_clear = 128u;
        }
    }

    enc->frame_used_colors = saved_used_colors;
    enc->frame_encoded_palette_entries = saved_encoded_palette_entries;
    enc->frame_entropy_q8 = saved_entropy_q8;

    should_break = 0u;
    if (band_index != 0u) {
        entropy_delta = 0u;
        row_repeat_delta = 0u;
        if (io_prev_entropy_q8 != 0) {
            if (*io_prev_entropy_q8 > entropy_q8) {
                entropy_delta = (giff_u16)(*io_prev_entropy_q8 - entropy_q8);
            } else {
                entropy_delta = (giff_u16)(entropy_q8 - *io_prev_entropy_q8);
            }
        }
        if (io_prev_row_repeat_q8 != 0) {
            if (*io_prev_row_repeat_q8 > row_repeat_q8) {
                row_repeat_delta = (giff_u16)(*io_prev_row_repeat_q8 - row_repeat_q8);
            } else {
                row_repeat_delta = (giff_u16)(row_repeat_q8 - *io_prev_row_repeat_q8);
            }
        }

        if ((io_prev_strategy != 0 && *io_prev_strategy != enc->lzw_strategy) ||
            entropy_delta >= 224u ||
            row_repeat_delta >= 96u) {
            should_break = 1u;
        }
    }

    if (should_break && enc->lzw_have_prefix && enc->lzw_codes_since_clear > 8u) {
        rc = giff_lzw_enc_soft_break(enc);
        if (rc != GIFF_OK) {
            return rc;
        }
        enc->lzw_band_clears = (giff_u16)(enc->lzw_band_clears + 1u);
        enc->lzw_band_switches = (giff_u16)(enc->lzw_band_switches + 1u);
    } else if (band_index != 0u && io_prev_strategy != 0 && *io_prev_strategy != enc->lzw_strategy) {
        enc->lzw_band_switches = (giff_u16)(enc->lzw_band_switches + 1u);
    }

    if (io_prev_entropy_q8 != 0) {
        *io_prev_entropy_q8 = entropy_q8;
    }
    if (io_prev_row_repeat_q8 != 0) {
        *io_prev_row_repeat_q8 = row_repeat_q8;
    }
    if (io_prev_strategy != 0) {
        *io_prev_strategy = enc->lzw_strategy;
    }

    if (enc->frame_band_count > 1u) {
        enc->band_lzw_tuned = 1u;
    }

    (void)max_index;
    (void)old_strategy;
    return GIFF_OK;
}

#if 0
static void giff_encoder_compact_existing_local_palette(
    giff_encoder* enc,
    giff_frame_info* frame,
    const giff_u32* hist
)
{
    giff_rgb8 compacted[256];
    giff_u8 remap[256];
    giff_u16 entries;
    giff_u8 has_transparency;
    giff_u8 transparency_index;
    giff_u8 sorted;
    giff_u8 changed;
    giff_u16 similarity_q8;
    giff_u8 used_temporal;

    if (enc == 0 || frame == 0 || hist == 0 || !frame->has_local_palette) {
        return;
    }

    similarity_q8 = 0u;
    used_temporal = 0u;
    entries = giff_encoder_build_dense_palette(
        enc,
        enc->local_palette,
        frame->local_palette_entries,
        hist,
        frame->has_transparency,
        frame->transparency_index,
        compacted,
        remap,
        &has_transparency,
        &transparency_index,
        &sorted,
        &changed,
        &similarity_q8,
        &used_temporal
    );

    enc->temporal_similarity_q8 = similarity_q8;
    if (used_temporal) {
        enc->frame_temporal_palette = 1u;
        enc->frame_clustered_palette = 1u;
    }

    if (!changed && entries == frame->local_palette_entries && has_transparency == frame->has_transparency &&
        (!has_transparency || transparency_index == frame->transparency_index)) {
        enc->frame_sorted_local_palette = 0u;
        enc->frame_sparse_local_remap = 0u;
        return;
    }

    giff_encoder_apply_remap(enc, remap, frame);
    giff_mem_copy(enc->local_palette, compacted, (giff_u32)entries * (giff_u32)sizeof(giff_rgb8));
    enc->local_palette_entries = entries;
    enc->local_has_transparency = has_transparency;
    enc->local_transparency_index = transparency_index;
    frame->local_palette_entries = entries;
    frame->has_transparency = has_transparency;
    frame->transparency_index = transparency_index;
    enc->frame_sorted_local_palette = sorted;
    enc->frame_sparse_local_remap = 1u;
}

#if 0
static giff_u8 giff_encoder_try_auto_local_palette(
    giff_encoder* enc,
    giff_frame_info* frame,
    const giff_u32* hist,
    giff_u16 used_count,
    giff_u16 max_index,
    giff_u32 pixels
)
{
    giff_rgb8 compacted[256];
    giff_u8 remap[256];
    giff_u16 entries;
    giff_u8 has_transparency;
    giff_u8 transparency_index;
    giff_u8 sorted;
    giff_u8 changed;
    giff_u8 old_bits;
    giff_u8 new_bits;
    giff_u32 estimated_saved;
    giff_u32 local_overhead;
    giff_u16 similarity_q8;
    giff_u8 used_temporal;

    if (enc == 0 || frame == 0 || hist == 0 || frame->has_local_palette) {
        return 0u;
    }
    if (!enc->global_palette_set || used_count == 0u) {
        return 0u;
    }

    similarity_q8 = 0u;
    used_temporal = 0u;
    entries = giff_encoder_build_dense_palette(
        enc,
        enc->global_palette,
        enc->global_palette_entries,
        hist,
        frame->has_transparency,
        frame->transparency_index,
        compacted,
        remap,
        &has_transparency,
        &transparency_index,
        &sorted,
        &changed,
        &similarity_q8,
        &used_temporal
    );

    enc->temporal_similarity_q8 = similarity_q8;

    if (entries == 0u || entries >= enc->global_palette_entries) {
        return 0u;
    }

    old_bits = giff_palette_bits((giff_u16)max_index + 1u);
    new_bits = giff_palette_bits(entries);
    if (old_bits < 2u) {
        old_bits = 2u;
    }
    if (new_bits < 2u) {
        new_bits = 2u;
    }
    if (new_bits >= old_bits) {
        return 0u;
    }

    estimated_saved = ((giff_u32)(old_bits - new_bits) * pixels + 7u) / 8u;
    local_overhead = giff_encoder_palette_table_bytes(entries);
    if (estimated_saved <= local_overhead + 4u) {
        if (!(used_count <= 8u && (giff_u16)(old_bits - new_bits) >= 2u)) {
            return 0u;
        }
    }

    giff_encoder_apply_remap(enc, remap, frame);
    giff_mem_copy(enc->local_palette, compacted, (giff_u32)entries * (giff_u32)sizeof(giff_rgb8));
    enc->local_palette_set = 1u;
    enc->local_palette_entries = entries;
    enc->local_has_transparency = has_transparency;
    enc->local_transparency_index = transparency_index;
    frame->has_local_palette = 1u;
    frame->local_palette_entries = entries;
    frame->has_transparency = has_transparency;
    frame->transparency_index = transparency_index;
    enc->frame_sorted_local_palette = sorted;
    enc->frame_sparse_local_remap = 1u;
    enc->frame_auto_local_palette = 1u;
    if (used_temporal) {
        enc->frame_temporal_palette = 1u;
        enc->frame_clustered_palette = 1u;
    }
    return 1u;
}

#endif
#endif

static giff_result giff_encoder_layout(giff_encoder* enc)
{
    giff_ws_cursor ws;
    giff_result rc;

    ws.base = (giff_u8*)enc->workspace;
    ws.size = enc->workspace_size;
    ws.used = 0u;

    rc = giff_ws_take(&ws, (giff_u32)(GIFF_LZW_ENC_HASH_SIZE * sizeof(giff_u16)), (giff_u32)sizeof(giff_u16), (void**)&enc->hash_prefix);
    if (rc != GIFF_OK) {
        return rc;
    }

    rc = giff_ws_take(&ws, (giff_u32)GIFF_LZW_ENC_HASH_SIZE, 1u, (void**)&enc->hash_suffix);
    if (rc != GIFF_OK) {
        return rc;
    }

    rc = giff_ws_take(&ws, (giff_u32)(GIFF_LZW_ENC_HASH_SIZE * sizeof(giff_u16)), (giff_u32)sizeof(giff_u16), (void**)&enc->hash_code);
    if (rc != GIFF_OK) {
        return rc;
    }

    rc = giff_ws_take(&ws, (giff_u32)GIFF_LZW_ENC_HASH_SIZE, 1u, (void**)&enc->hash_used);
    if (rc != GIFF_OK) {
        return rc;
    }

    rc = giff_ws_take(&ws, (giff_u32)GIFF_LZW_TABLE_SIZE, 1u, (void**)&enc->lzw_code_len);
    if (rc != GIFF_OK) {
        return rc;
    }

    rc = giff_ws_take(&ws, (giff_u32)GIFF_DATA_SUBBLOCK_MAX, 1u, (void**)&enc->packet_buf);
    if (rc != GIFF_OK) {
        return rc;
    }

    rc = giff_ws_take(&ws, (giff_u32)enc->cfg.max_width, 1u, (void**)&enc->row_indexed);
    if (rc != GIFF_OK) {
        return rc;
    }

    rc = giff_ws_take(&ws, (giff_u32)enc->cfg.max_width * (giff_u32)enc->cfg.max_height, 1u, (void**)&enc->frame_indexed);
    if (rc != GIFF_OK) {
        return rc;
    }

    rc = giff_ws_take(&ws, (giff_u32)(GIFF_QUANT_HIST_SIZE * sizeof(giff_u32)), (giff_u32)sizeof(giff_u32), (void**)&enc->quant_hist);
    if (rc != GIFF_OK) {
        return rc;
    }

    rc = giff_ws_take(&ws, (giff_u32)(GIFF_QUANT_HIST_SIZE * sizeof(giff_u16)), (giff_u32)sizeof(giff_u16), (void**)&enc->quant_bins);
    if (rc != GIFF_OK) {
        return rc;
    }

    rc = giff_ws_take(&ws, (giff_u32)(GIFF_QUANT_HIST_SIZE * sizeof(giff_u16)), (giff_u32)sizeof(giff_u16), (void**)&enc->quant_bins_tmp);
    if (rc != GIFF_OK) {
        return rc;
    }

    rc = giff_ws_take(&ws, (giff_u32)(((giff_u32)enc->cfg.max_width + 2u) * 3u * (giff_u32)sizeof(giff_s32)), (giff_u32)sizeof(giff_s32), (void**)&enc->dither_curr);
    if (rc != GIFF_OK) {
        return rc;
    }

    rc = giff_ws_take(&ws, (giff_u32)(((giff_u32)enc->cfg.max_width + 2u) * 3u * (giff_u32)sizeof(giff_s32)), (giff_u32)sizeof(giff_s32), (void**)&enc->dither_next);
    if (rc != GIFF_OK) {
        return rc;
    }

    rc = giff_ws_take(&ws, (giff_u32)(GIFF_OCTREE_MAX_NODES * (giff_u32)sizeof(giff_oct_node)), (giff_u32)sizeof(giff_u32), (void**)&enc->oct_nodes);
    if (rc != GIFF_OK) {
        return rc;
    }

    rc = giff_ws_take(&ws,
                      (giff_u32)GIFF_TEMPORAL_WINDOW_MAX * 256u * (giff_u32)sizeof(giff_rgb8),
                      (giff_u32)sizeof(giff_u32),
                      (void**)&enc->temporal_window_palette);
    if (rc != GIFF_OK) {
        return rc;
    }

    rc = giff_ws_take(&ws,
                      (giff_u32)GIFF_TEMPORAL_WINDOW_MAX * 256u * (giff_u32)sizeof(giff_u32),
                      (giff_u32)sizeof(giff_u32),
                      (void**)&enc->temporal_window_hist);
    if (rc != GIFF_OK) {
        return rc;
    }

    return GIFF_OK;
}

giff_u32 giff_encoder_workspace_size(const giff_encoder_config* cfg)
{
    giff_u32 total;

    if (cfg == 0) {
        return 0u;
    }

    total = 0u;
    total = giff_align_up(total, (giff_u32)sizeof(giff_u16));
    total += (giff_u32)(GIFF_LZW_ENC_HASH_SIZE * sizeof(giff_u16));
    total += (giff_u32)GIFF_LZW_ENC_HASH_SIZE;
    total = giff_align_up(total, (giff_u32)sizeof(giff_u16));
    total += (giff_u32)(GIFF_LZW_ENC_HASH_SIZE * sizeof(giff_u16));
    total += (giff_u32)GIFF_LZW_ENC_HASH_SIZE;
    total += (giff_u32)GIFF_LZW_TABLE_SIZE;
    total += (giff_u32)GIFF_DATA_SUBBLOCK_MAX;
    total += (giff_u32)cfg->max_width;
    total += (giff_u32)cfg->max_width * (giff_u32)cfg->max_height;
    total = giff_align_up(total, (giff_u32)sizeof(giff_u32));
    total += (giff_u32)(GIFF_QUANT_HIST_SIZE * sizeof(giff_u32));
    total = giff_align_up(total, (giff_u32)sizeof(giff_u16));
    total += (giff_u32)(GIFF_QUANT_HIST_SIZE * sizeof(giff_u16));
    total = giff_align_up(total, (giff_u32)sizeof(giff_u16));
    total += (giff_u32)(GIFF_QUANT_HIST_SIZE * sizeof(giff_u16));
    total = giff_align_up(total, (giff_u32)sizeof(giff_s32));
    total += (giff_u32)(((giff_u32)cfg->max_width + 2u) * 3u * (giff_u32)sizeof(giff_s32));
    total = giff_align_up(total, (giff_u32)sizeof(giff_s32));
    total += (giff_u32)(((giff_u32)cfg->max_width + 2u) * 3u * (giff_u32)sizeof(giff_s32));
    total = giff_align_up(total, (giff_u32)sizeof(giff_u32));
    total += (giff_u32)(GIFF_OCTREE_MAX_NODES * (giff_u32)sizeof(giff_oct_node));
    total = giff_align_up(total, (giff_u32)sizeof(giff_u32));
    total += (giff_u32)GIFF_TEMPORAL_WINDOW_MAX * 256u * (giff_u32)sizeof(giff_rgb8);
    total = giff_align_up(total, (giff_u32)sizeof(giff_u32));
    total += (giff_u32)GIFF_TEMPORAL_WINDOW_MAX * 256u * (giff_u32)sizeof(giff_u32);

    return total;
}

giff_result giff_encoder_write_bytes(
    giff_encoder* enc,
    const giff_u8* src,
    giff_u32 size
)
{
    if (enc == 0) {
        return GIFF_E_INVALID_ARGUMENT;
    }

    if (size == 0u) {
        return GIFF_OK;
    }

    if (src == 0 || enc->io.write == 0) {
        return GIFF_E_INVALID_ARGUMENT;
    }

    if (!enc->io.write(enc->io.user, src, size)) {
        return GIFF_E_OUTPUT_OVERFLOW;
    }

    enc->output_offset += size;
    return GIFF_OK;
}

giff_result giff_encoder_init(
    giff_encoder* enc,
    const giff_encoder_config* cfg,
    void* workspace,
    giff_u32 workspace_size
)
{
    giff_result rc;

    if (enc == 0 || cfg == 0 || workspace == 0) {
        return GIFF_E_INVALID_ARGUMENT;
    }

    if (cfg->max_width == 0u || cfg->max_height == 0u) {
        return GIFF_E_BAD_DIMENSIONS;
    }

    giff_mem_zero(enc, (giff_u32)sizeof(*enc));
    enc->cfg = *cfg;
    if (enc->cfg.quantizer_hist_bits == 0u) {
        enc->cfg.quantizer_hist_bits = (giff_u8)GIFF_QUANT_HIST_BITS;
    }
    if (enc->cfg.quantizer_mode > (giff_u8)GIFF_QUANTIZER_OCTREE) {
        enc->cfg.quantizer_mode = (giff_u8)GIFF_QUANTIZER_MEDIAN_CUT;
    }
    if (enc->cfg.enable_cross_frame_cluster == 0u) {
        enc->cfg.enable_cross_frame_cluster = 1u;
    }
    if (enc->cfg.enable_temporal_remap == 0u) {
        enc->cfg.enable_temporal_remap = 1u;
    }
    if (enc->cfg.lzw_band_rows == 0u) {
        enc->cfg.lzw_band_rows = (giff_u8)GIFF_DEFAULT_BAND_ROWS;
    }
    if (enc->cfg.temporal_decay_shift == 0u) {
        enc->cfg.temporal_decay_shift = 2u;
    }
    if (enc->cfg.temporal_window_frames == 0u) {
        enc->cfg.temporal_window_frames = (giff_u8)GIFF_DEFAULT_TEMPORAL_WINDOW;
    }
    if (enc->cfg.temporal_window_frames > (giff_u8)GIFF_TEMPORAL_WINDOW_MAX) {
        enc->cfg.temporal_window_frames = (giff_u8)GIFF_TEMPORAL_WINDOW_MAX;
    }
    if (enc->cfg.enable_segment_sharing == 0u) {
        enc->cfg.enable_segment_sharing = 1u;
    }
    if (enc->cfg.segment_max_frames == 0u) {
        enc->cfg.segment_max_frames = (giff_u8)GIFF_DEFAULT_SEGMENT_MAX_FRAMES;
    }
    if (enc->cfg.segment_similarity_q8_threshold == 0u) {
        enc->cfg.segment_similarity_q8_threshold = (giff_u8)GIFF_DEFAULT_SEGMENT_SIMILARITY_Q8;
    }
    enc->workspace = workspace;
    enc->workspace_size = workspace_size;

    rc = giff_encoder_layout(enc);
    if (rc != GIFF_OK) {
        enc->last_result = (giff_u8)(-rc);
        return rc;
    }

    giff_encoder_reset(enc);
    enc->last_result = (giff_u8)GIFF_OK;
    return GIFF_OK;
}

void giff_encoder_reset(giff_encoder* enc)
{
    if (enc == 0) {
        return;
    }

    enc->frames_written = 0u;
    enc->frame_cost_keep = 0u;
    enc->frame_cost_local = 0u;
    enc->frame_cost_temporal = 0u;
    enc->frame_cost_segment = 0u;
    enc->frame_cost_chosen = 0u;
    enc->output_offset = 0u;
    enc->started = 0u;
    enc->finished = 0u;
    enc->frame_active = 0u;
    enc->frame_rows_written = 0u;
    enc->local_palette_set = 0u;
    enc->local_palette_entries = 0u;
    enc->local_has_transparency = 0u;
    enc->local_transparency_index = 0u;
    enc->current_palette_entries = 0u;
    enc->octree_map_entries = 0u;
    enc->frame_used_colors = 0u;
    enc->frame_encoded_palette_entries = 0u;
    enc->frame_entropy_q8 = 0u;
    enc->frame_reset_threshold = 0u;
    enc->frame_periodic_clear = 0u;
    enc->frame_stall_threshold = 0u;
    enc->lzw_inputs_since_clear = 0u;
    enc->lzw_codes_since_clear = 0u;
    enc->lzw_stall_run = 0u;
    enc->lzw_clear_count = 0u;
    enc->interlace_buffered = 0u;
    enc->octree_map_valid = 0u;
    enc->octree_map_is_local = 0u;
    enc->octree_map_has_transparency = 0u;
    enc->octree_map_transparency_index = 0u;
    enc->frame_sorted_local_palette = 0u;
    enc->frame_auto_local_palette = 0u;
    enc->frame_sparse_local_remap = 0u;
    enc->lzw_strategy = (giff_u8)GIFF_LZW_STRATEGY_DEFAULT;
    enc->lzw_prefix_len = 0u;
    enc->frame_stride = 0u;
    enc->temporal_entries = 0u;
    enc->prev_palette_entries = 0u;
    enc->temporal_similarity_q8 = 0u;
    enc->segment_palette_entries = 0u;
    enc->segment_similarity_q8 = 0u;
    enc->segment_frame_count = 0u;
    enc->frame_band_rows = 0u;
    enc->frame_band_count = 0u;
    enc->lzw_band_clears = 0u;
    enc->lzw_band_switches = 0u;
    enc->temporal_window_similarity_q8 = 0u;
    enc->temporal_valid = 0u;
    enc->temporal_has_transparency = 0u;
    enc->temporal_transparency_index = 0u;
    enc->prev_palette_valid = 0u;
    enc->prev_has_transparency = 0u;
    enc->prev_transparency_index = 0u;
    enc->frame_temporal_palette = 0u;
    enc->frame_clustered_palette = 0u;
    enc->band_lzw_tuned = 0u;
    enc->frame_window_used = 0u;
    enc->frame_cost_mode = (giff_u8)GIFF_PALETTE_DECISION_KEEP;
    enc->segment_valid = 0u;
    enc->segment_has_transparency = 0u;
    enc->segment_transparency_index = 0u;
    enc->frame_segment_shared = 0u;
    enc->frame_segment_break = 0u;
    giff_encoder_temporal_window_clear(enc);
    giff_mem_zero(enc->segment_palette, (giff_u32)sizeof(enc->segment_palette));
    giff_mem_zero(enc->segment_hist, (giff_u32)sizeof(enc->segment_hist));
    giff_mem_zero(&enc->stream_info, (giff_u32)sizeof(enc->stream_info));
    giff_mem_zero(&enc->frame, (giff_u32)sizeof(enc->frame));
    giff_lzw_enc_reset(enc);
    enc->last_result = (giff_u8)GIFF_OK;
}

void giff_encoder_attach_io(giff_encoder* enc, const giff_io* io)
{
    if (enc == 0) {
        return;
    }

    if (io == 0) {
        giff_mem_zero(&enc->io, (giff_u32)sizeof(enc->io));
        return;
    }

    enc->io = *io;
}

giff_result giff_encoder_set_global_palette(
    giff_encoder* enc,
    const giff_rgb8* palette,
    giff_u16 entries
)
{
    if (enc == 0 || palette == 0) {
        return GIFF_E_INVALID_ARGUMENT;
    }

    if (entries == 0u || entries > 256u) {
        return GIFF_E_BAD_COLOR_TABLE;
    }

    giff_mem_copy(enc->global_palette, palette, (giff_u32)entries * (giff_u32)sizeof(giff_rgb8));
    enc->global_palette_entries = entries;
    enc->global_palette_set = 1u;
    enc->global_has_transparency = 0u;
    enc->global_transparency_index = 0u;
    enc->octree_map_valid = 0u;
    enc->octree_map_entries = 0u;
    return GIFF_OK;
}

giff_result giff_encoder_set_local_palette(
    giff_encoder* enc,
    const giff_rgb8* palette,
    giff_u16 entries
)
{
    if (enc == 0 || palette == 0) {
        return GIFF_E_INVALID_ARGUMENT;
    }

    if (entries == 0u || entries > 256u) {
        return GIFF_E_BAD_COLOR_TABLE;
    }

    giff_mem_copy(enc->local_palette, palette, (giff_u32)entries * (giff_u32)sizeof(giff_rgb8));
    enc->local_palette_entries = entries;
    enc->local_palette_set = 1u;
    enc->local_has_transparency = 0u;
    enc->local_transparency_index = 0u;
    enc->octree_map_valid = 0u;
    enc->octree_map_entries = 0u;
    return GIFF_OK;
}

giff_result giff_encoder_build_palette_from_rgba(
    giff_encoder* enc,
    const giff_rgba8* pixels,
    giff_u16 width,
    giff_u16 height,
    giff_u32 stride,
    giff_u8 to_local_palette,
    giff_u16 max_entries
)
{
    giff_result rc;
    giff_u16 entries;
    giff_u8 transparent_index;
    giff_u8 has_transparent;

    if (enc == 0 || pixels == 0) {
        return GIFF_E_INVALID_ARGUMENT;
    }

    if (max_entries == 0u) {
        max_entries = enc->cfg.global_palette_entries;
        if (max_entries == 0u) {
            max_entries = 256u;
        }
    }

    rc = giff_palette_build_from_rgba(
        enc,
        pixels,
        width,
        height,
        stride,
        to_local_palette ? enc->local_palette : enc->global_palette,
        &entries,
        max_entries,
        enc->cfg.reserve_transparent,
        &transparent_index,
        &has_transparent
    );
    if (rc != GIFF_OK) {
        return rc;
    }

    if (to_local_palette) {
        enc->local_palette_entries = entries;
        enc->local_palette_set = 1u;
        enc->local_transparency_index = transparent_index;
        enc->local_has_transparency = has_transparent;
    } else {
        enc->global_palette_entries = entries;
        enc->global_palette_set = 1u;
        enc->global_transparency_index = transparent_index;
        enc->global_has_transparency = has_transparent;
    }

    if (enc->cfg.quantizer_mode == (giff_u8)GIFF_QUANTIZER_OCTREE) {
        enc->octree_map_valid = 1u;
        enc->octree_map_is_local = to_local_palette ? 1u : 0u;
        enc->octree_map_entries = entries;
        enc->octree_map_has_transparency = has_transparent;
        enc->octree_map_transparency_index = transparent_index;
    } else {
        enc->octree_map_valid = 0u;
        enc->octree_map_entries = 0u;
    }

    return GIFF_OK;
}

giff_result giff_encoder_begin(
    giff_encoder* enc,
    const giff_info* stream_info
)
{
    giff_result rc;

    if (enc == 0 || stream_info == 0) {
        return GIFF_E_INVALID_ARGUMENT;
    }

    if (enc->started || enc->finished || enc->frame_active) {
        return GIFF_E_INVALID_ARGUMENT;
    }

    rc = giff_encoder_validate_stream_info(enc, stream_info);
    if (rc != GIFF_OK) {
        return rc;
    }

    enc->stream_info = *stream_info;
    enc->stream_info.has_global_palette = 1u;
    enc->stream_info.global_palette_entries = enc->global_palette_entries;
    enc->stream_info.color_resolution_bits = giff_palette_bits(enc->global_palette_entries);
    if (enc->stream_info.version_major == 0u) {
        enc->stream_info.version_major = 8u;
    }
    if (enc->stream_info.version_minor == 0u) {
        enc->stream_info.version_minor = (giff_u8)'9';
    }

    rc = giff_encoder_write_header(enc, &enc->stream_info);
    if (rc != GIFF_OK) {
        return rc;
    }

    enc->started = 1u;
    enc->finished = 0u;
    enc->frames_written = 0u;
    return GIFF_OK;
}

giff_result giff_encoder_begin_frame(
    giff_encoder* enc,
    const giff_frame_info* frame
)
{
    giff_result rc;
    giff_frame_info local_frame;

    if (enc == 0 || frame == 0) {
        return GIFF_E_INVALID_ARGUMENT;
    }

    if (!enc->started || enc->finished || enc->frame_active) {
        return GIFF_E_INVALID_ARGUMENT;
    }

    local_frame = *frame;
    if (local_frame.has_local_palette && local_frame.local_palette_entries == 0u) {
        local_frame.local_palette_entries = enc->local_palette_entries;
    }

    rc = giff_encoder_validate_frame(enc, &local_frame);
    if (rc != GIFF_OK) {
        return rc;
    }

    enc->frame = local_frame;
    enc->current_palette_entries = giff_encoder_active_palette_entries(enc);
    enc->frame_rows_written = 0u;
    enc->frame_stride = enc->frame.width;
    enc->interlace_buffered = enc->frame.interlaced ? 1u : 0u;
    enc->frame_sorted_local_palette = 0u;
    enc->frame_auto_local_palette = 0u;
    enc->frame_sparse_local_remap = 0u;
    enc->frame_used_colors = 0u;
    enc->frame_encoded_palette_entries = enc->current_palette_entries;
    enc->frame_entropy_q8 = 0u;
    enc->frame_reset_threshold = 0u;
    enc->frame_periodic_clear = 0u;
    enc->frame_stall_threshold = 0u;
    enc->temporal_similarity_q8 = 0u;
    enc->frame_band_rows = 0u;
    enc->frame_band_count = 0u;
    enc->lzw_band_clears = 0u;
    enc->lzw_band_switches = 0u;
    enc->frame_temporal_palette = 0u;
    enc->frame_clustered_palette = 0u;
    enc->band_lzw_tuned = 0u;
    enc->frame_window_used = enc->temporal_window_count;
    enc->frame_cost_keep = 0u;
    enc->frame_cost_local = 0u;
    enc->frame_cost_temporal = 0u;
    enc->frame_cost_segment = 0u;
    enc->frame_cost_chosen = 0u;
    enc->frame_cost_mode = (giff_u8)GIFF_PALETTE_DECISION_KEEP;
    enc->frame_segment_shared = 0u;
    enc->frame_segment_break = 0u;
    enc->frame_active = 1u;
    return GIFF_OK;
}

giff_result giff_encoder_write_indexed_rows(
    giff_encoder* enc,
    const giff_u8* rows,
    giff_u16 row_count,
    giff_u32 stride
)
{
    giff_u16 y;
    giff_u16 x;
    const giff_u8* row;
    giff_u16 palette_entries;

    if (enc == 0 || rows == 0) {
        return GIFF_E_INVALID_ARGUMENT;
    }

    if (!enc->frame_active) {
        return GIFF_E_INVALID_ARGUMENT;
    }

    if (stride == 0u) {
        stride = (giff_u32)enc->frame.width;
    }

    if ((giff_u32)enc->frame_rows_written + (giff_u32)row_count > (giff_u32)enc->frame.height) {
        return GIFF_E_OUTPUT_OVERFLOW;
    }

    palette_entries = giff_encoder_active_palette_entries(enc);

    for (y = 0u; y < row_count; ++y) {
        row = rows + (giff_u32)y * stride;
        for (x = 0u; x < enc->frame.width; ++x) {
            if ((giff_u16)row[x] >= palette_entries) {
                return GIFF_E_BAD_COLOR_TABLE;
            }
        }

        giff_mem_copy(enc->frame_indexed + (giff_u32)enc->frame_rows_written * (giff_u32)enc->frame_stride,
                      row,
                      (giff_u32)enc->frame.width);

        enc->frame_rows_written = (giff_u16)(enc->frame_rows_written + 1u);
    }

    return GIFF_OK;
}

giff_result giff_encoder_end_frame(giff_encoder* enc)
{
    giff_result rc;
    giff_u8 min_code_size;
    giff_u16 seq_row;
    giff_frame_info out_frame;
    giff_u8 need_gce;
    giff_u32 pixels;
    giff_u32 hist[256];
    giff_u16 used_count;
    giff_u16 max_index;
    giff_u32 same_adjacent;
    giff_u32 dominant_count;
    giff_u16 entropy_q8;
    giff_rgb8 saved_local_palette[256];
    giff_u16 saved_local_entries;
    giff_u8 saved_local_set;
    giff_u8 saved_local_has_transparency;
    giff_u8 saved_local_transparency_index;
    giff_u16 analysis_palette_entries;
    giff_u16 band_rows;
    giff_u16 band_count;
    giff_u16 last_band_index;
    giff_u16 prev_band_entropy_q8;
    giff_u16 prev_band_row_repeat_q8;
    giff_u8 prev_band_strategy;
    const giff_rgb8* active_palette;
    giff_u8 active_has_transparency;
    giff_u8 active_transparency_index;

    if (enc == 0) {
        return GIFF_E_INVALID_ARGUMENT;
    }

    if (!enc->frame_active) {
        return GIFF_E_INVALID_ARGUMENT;
    }

    if (enc->frame_rows_written != enc->frame.height) {
        return GIFF_E_NEED_MORE_INPUT;
    }

    saved_local_entries = enc->local_palette_entries;
    saved_local_set = enc->local_palette_set;
    saved_local_has_transparency = enc->local_has_transparency;
    saved_local_transparency_index = enc->local_transparency_index;
    giff_mem_copy(saved_local_palette, enc->local_palette, (giff_u32)sizeof(saved_local_palette));

    enc->frame_sorted_local_palette = 0u;
    enc->frame_auto_local_palette = 0u;
    enc->frame_sparse_local_remap = 0u;
    enc->frame_temporal_palette = 0u;
    enc->frame_clustered_palette = 0u;
    enc->temporal_similarity_q8 = 0u;
    enc->frame_band_rows = 0u;
    enc->frame_band_count = 0u;
    enc->lzw_band_clears = 0u;
    enc->lzw_band_switches = 0u;
    enc->band_lzw_tuned = 0u;

    out_frame = enc->frame;
    if (out_frame.has_local_palette && out_frame.local_palette_entries == 0u) {
        out_frame.local_palette_entries = enc->local_palette_entries;
    }

    pixels = (giff_u32)out_frame.width * (giff_u32)out_frame.height;
    analysis_palette_entries = out_frame.has_local_palette ? out_frame.local_palette_entries : enc->global_palette_entries;
    giff_encoder_analyze_frame(
        enc,
        &out_frame,
        analysis_palette_entries,
        hist,
        &used_count,
        &max_index,
        &same_adjacent,
        &dominant_count,
        &entropy_q8
    );

    giff_encoder_select_palette_strategy(
        enc,
        &out_frame,
        hist,
        &used_count,
        &max_index,
        same_adjacent,
        entropy_q8,
        pixels
    );

    enc->current_palette_entries = out_frame.has_local_palette ? out_frame.local_palette_entries : enc->global_palette_entries;
    giff_encoder_analyze_frame(
        enc,
        &out_frame,
        enc->current_palette_entries,
        hist,
        &used_count,
        &max_index,
        &same_adjacent,
        &dominant_count,
        &entropy_q8
    );
    giff_encoder_choose_lzw_strategy(
        enc,
        enc->current_palette_entries,
        used_count,
        same_adjacent,
        dominant_count,
        entropy_q8,
        pixels
    );

    band_rows = giff_encoder_default_band_rows(enc, &out_frame);
    band_count = 1u;
    if (band_rows != 0u) {
        band_count = (giff_u16)(((giff_u32)out_frame.height + (giff_u32)band_rows - 1u) / (giff_u32)band_rows);
        if (band_count < 2u) {
            band_rows = 0u;
            band_count = 1u;
        }
    }
    enc->frame_band_rows = band_rows;
    enc->frame_band_count = band_count;
    if (band_count > 1u) {
        enc->band_lzw_tuned = 1u;
    }

    need_gce = 0u;
    if (out_frame.delay_cs != 0u || out_frame.disposal_method != 0u || out_frame.has_transparency) {
        need_gce = 1u;
    }

    enc->frame = out_frame;

    if (need_gce) {
        rc = giff_encoder_write_gce(enc, &enc->frame);
        if (rc != GIFF_OK) {
            goto done;
        }
    }

    rc = giff_encoder_write_image_descriptor(enc, &enc->frame);
    if (rc != GIFF_OK) {
        goto done;
    }

    min_code_size = giff_encoder_frame_min_code_size(enc);
    rc = giff_encoder_write_u8(enc, min_code_size);
    if (rc != GIFF_OK) {
        goto done;
    }

    rc = giff_lzw_enc_begin_image(enc, min_code_size);
    if (rc != GIFF_OK) {
        goto done;
    }

    last_band_index = 0xFFFFu;
    prev_band_entropy_q8 = 0u;
    prev_band_row_repeat_q8 = 0u;
    prev_band_strategy = enc->lzw_strategy;

    if (enc->frame.interlaced) {
        for (seq_row = 0u; seq_row < enc->frame.height; ++seq_row) {
            giff_u16 actual_row;
            const giff_u8* src_row;
            giff_u16 x;

            if (band_rows != 0u) {
                giff_u16 band_index;
                giff_u16 band_start;
                giff_u16 band_end;

                band_index = (giff_u16)(seq_row / band_rows);
                if (band_index != last_band_index) {
                    band_start = (giff_u16)(band_index * band_rows);
                    band_end = (giff_u16)(band_start + band_rows);
                    if (band_end > enc->frame.height) {
                        band_end = enc->frame.height;
                    }
                    rc = giff_encoder_apply_band_lzw(
                        enc,
                        &enc->frame,
                        band_index,
                        band_start,
                        band_end,
                        &prev_band_entropy_q8,
                        &prev_band_row_repeat_q8,
                        &prev_band_strategy
                    );
                    if (rc != GIFF_OK) {
                        goto done;
                    }
                    last_band_index = band_index;
                }
            }

            actual_row = giff_encoder_interlaced_row(seq_row, enc->frame.height);
            src_row = enc->frame_indexed + (giff_u32)actual_row * (giff_u32)enc->frame_stride;
            for (x = 0u; x < enc->frame.width; ++x) {
                rc = giff_lzw_enc_emit_index(enc, src_row[x]);
                if (rc != GIFF_OK) {
                    goto done;
                }
            }
        }
    } else {
        for (seq_row = 0u; seq_row < enc->frame.height; ++seq_row) {
            const giff_u8* src_row;
            giff_u16 x;

            if (band_rows != 0u) {
                giff_u16 band_index;
                giff_u16 band_start;
                giff_u16 band_end;

                band_index = (giff_u16)(seq_row / band_rows);
                if (band_index != last_band_index) {
                    band_start = (giff_u16)(band_index * band_rows);
                    band_end = (giff_u16)(band_start + band_rows);
                    if (band_end > enc->frame.height) {
                        band_end = enc->frame.height;
                    }
                    rc = giff_encoder_apply_band_lzw(
                        enc,
                        &enc->frame,
                        band_index,
                        band_start,
                        band_end,
                        &prev_band_entropy_q8,
                        &prev_band_row_repeat_q8,
                        &prev_band_strategy
                    );
                    if (rc != GIFF_OK) {
                        goto done;
                    }
                    last_band_index = band_index;
                }
            }

            src_row = enc->frame_indexed + (giff_u32)seq_row * (giff_u32)enc->frame_stride;
            for (x = 0u; x < enc->frame.width; ++x) {
                rc = giff_lzw_enc_emit_index(enc, src_row[x]);
                if (rc != GIFF_OK) {
                    goto done;
                }
            }
        }
    }

    rc = giff_lzw_enc_end_image(enc);
    if (rc != GIFF_OK) {
        goto done;
    }

    if (enc->frame.has_local_palette) {
        active_palette = enc->local_palette;
        active_has_transparency = enc->frame.has_transparency;
        active_transparency_index = enc->frame.transparency_index;
    } else {
        active_palette = enc->global_palette;
        active_has_transparency = enc->global_has_transparency;
        active_transparency_index = enc->global_transparency_index;
    }

    giff_encoder_update_temporal_model(
        enc,
        active_palette,
        enc->current_palette_entries,
        hist,
        active_has_transparency,
        active_transparency_index
    );
    giff_encoder_update_segment_model(
        enc,
        active_palette,
        enc->current_palette_entries,
        hist,
        active_has_transparency,
        active_transparency_index
    );
    giff_encoder_capture_previous_palette(
        enc,
        active_palette,
        enc->current_palette_entries,
        active_has_transparency,
        active_transparency_index
    );

    enc->frames_written += 1u;

 done:
    enc->frame_active = 0u;
    enc->interlace_buffered = 0u;
    enc->frame_rows_written = 0u;
    enc->local_palette_entries = saved_local_entries;
    enc->local_palette_set = saved_local_set;
    enc->local_has_transparency = saved_local_has_transparency;
    enc->local_transparency_index = saved_local_transparency_index;
    giff_mem_copy(enc->local_palette, saved_local_palette, (giff_u32)sizeof(saved_local_palette));
    return rc;
}

giff_result giff_encoder_write_indexed_frame(
    giff_encoder* enc,
    const giff_frame_info* frame,
    const giff_u8* pixels,
    giff_u32 stride
)
{
    giff_result rc;

    if (enc == 0 || frame == 0 || pixels == 0) {
        return GIFF_E_INVALID_ARGUMENT;
    }

    rc = giff_encoder_begin_frame(enc, frame);
    if (rc != GIFF_OK) {
        return rc;
    }

    rc = giff_encoder_write_indexed_rows(enc, pixels, frame->height, stride);
    if (rc != GIFF_OK) {
        return rc;
    }

    rc = giff_encoder_end_frame(enc);
    return rc;
}

static giff_s32 giff_fs_scale(giff_s32 value, giff_s32 numerator)
{
    giff_s32 scaled;

    scaled = value * numerator;
    if (scaled >= 0) {
        return (scaled + 8) / 16;
    }
    return -(((-scaled) + 8) / 16);
}

static giff_s32 giff_fs_to_int(giff_s32 value_fx4)
{
    if (value_fx4 >= 0) {
        return (value_fx4 + 8) / 16;
    }
    return -(((-value_fx4) + 8) / 16);
}

giff_result giff_encoder_write_rgba_frame(
    giff_encoder* enc,
    const giff_frame_info* frame,
    const giff_rgba8* pixels,
    giff_u32 stride,
    giff_u8 use_local_palette
)
{
    giff_result rc;
    giff_frame_info temp;
    giff_u16 y;
    giff_u16 x;
    const giff_rgba8* row;
    const giff_rgb8* palette;
    giff_u16 palette_entries;
    giff_u8 transparent_index;
    giff_u8 has_transparency;
    giff_s32* curr_err;
    giff_s32* next_err;
    giff_u32 err_bytes;
    giff_u8 use_octree_direct;

    if (enc == 0 || frame == 0 || pixels == 0) {
        return GIFF_E_INVALID_ARGUMENT;
    }

    if (stride == 0u) {
        stride = (giff_u32)frame->width * (giff_u32)sizeof(giff_rgba8);
    }

    temp = *frame;
    if (use_local_palette) {
        rc = giff_encoder_build_palette_from_rgba(
            enc,
            pixels,
            frame->width,
            frame->height,
            stride,
            1u,
            frame->local_palette_entries ? frame->local_palette_entries : enc->cfg.global_palette_entries
        );
        if (rc != GIFF_OK) {
            return rc;
        }
        temp.has_local_palette = 1u;
        temp.local_palette_entries = enc->local_palette_entries;
        temp.has_transparency = enc->local_has_transparency;
        temp.transparency_index = enc->local_transparency_index;
        palette = enc->local_palette;
        palette_entries = enc->local_palette_entries;
        transparent_index = enc->local_transparency_index;
        has_transparency = enc->local_has_transparency;
    } else {
        if (!enc->global_palette_set || enc->global_palette_entries == 0u) {
            return GIFF_E_BAD_COLOR_TABLE;
        }
        temp.has_local_palette = 0u;
        if (!temp.has_transparency && enc->global_has_transparency) {
            temp.has_transparency = 1u;
            temp.transparency_index = enc->global_transparency_index;
        }
        palette = enc->global_palette;
        palette_entries = enc->global_palette_entries;
        transparent_index = enc->global_transparency_index;
        has_transparency = enc->global_has_transparency;
    }

    rc = giff_encoder_begin_frame(enc, &temp);
    if (rc != GIFF_OK) {
        return rc;
    }

    use_octree_direct = 0u;
    if (enc->cfg.quantizer_mode == (giff_u8)GIFF_QUANTIZER_OCTREE && enc->octree_map_valid) {
        if ((use_local_palette && enc->octree_map_is_local) || (!use_local_palette && !enc->octree_map_is_local)) {
            use_octree_direct = 1u;
        }
    }

    if (enc->cfg.dither_mode == (giff_u8)GIFF_DITHER_FLOYD_STEINBERG) {
        if (enc->dither_curr == 0 || enc->dither_next == 0) {
            return GIFF_E_NO_WORKSPACE;
        }

        curr_err = enc->dither_curr;
        next_err = enc->dither_next;
        err_bytes = (giff_u32)(((giff_u32)temp.width + 2u) * 3u * (giff_u32)sizeof(giff_s32));
        giff_mem_zero(curr_err, err_bytes);
        giff_mem_zero(next_err, err_bytes);

        for (y = 0u; y < temp.height; ++y) {
            row = (const giff_rgba8*)((const giff_u8*)pixels + (giff_u32)y * stride);
            for (x = 0u; x < temp.width; ++x) {
                giff_u32 eidx;
                giff_s32 rfx;
                giff_s32 gfx;
                giff_s32 bfx;
                giff_s32 r;
                giff_s32 g;
                giff_s32 b;
                giff_u8 index;
                giff_rgb8 chosen;
                giff_s32 er;
                giff_s32 eg;
                giff_s32 eb;

                if (has_transparency && row[x].a < 128u) {
                    enc->row_indexed[x] = transparent_index;
                    continue;
                }

                eidx = ((giff_u32)x + 1u) * 3u;
                rfx = (giff_s32)row[x].r * 16 + curr_err[eidx + 0u];
                gfx = (giff_s32)row[x].g * 16 + curr_err[eidx + 1u];
                bfx = (giff_s32)row[x].b * 16 + curr_err[eidx + 2u];

                r = giff_fs_to_int(rfx);
                g = giff_fs_to_int(gfx);
                b = giff_fs_to_int(bfx);

                rc = giff_palette_map_rgb(
                    enc,
                    palette,
                    palette_entries,
                    transparent_index,
                    has_transparency,
                    r,
                    g,
                    b,
                    use_octree_direct,
                    &index,
                    &chosen
                );
                if (rc != GIFF_OK) {
                    return rc;
                }

                enc->row_indexed[x] = index;
                er = rfx - (giff_s32)chosen.r * 16;
                eg = gfx - (giff_s32)chosen.g * 16;
                eb = bfx - (giff_s32)chosen.b * 16;

                curr_err[eidx + 3u + 0u] += giff_fs_scale(er, 7);
                curr_err[eidx + 3u + 1u] += giff_fs_scale(eg, 7);
                curr_err[eidx + 3u + 2u] += giff_fs_scale(eb, 7);

                next_err[eidx - 3u + 0u] += giff_fs_scale(er, 3);
                next_err[eidx - 3u + 1u] += giff_fs_scale(eg, 3);
                next_err[eidx - 3u + 2u] += giff_fs_scale(eb, 3);

                next_err[eidx + 0u] += giff_fs_scale(er, 5);
                next_err[eidx + 1u] += giff_fs_scale(eg, 5);
                next_err[eidx + 2u] += giff_fs_scale(eb, 5);

                next_err[eidx + 3u + 0u] += giff_fs_scale(er, 1);
                next_err[eidx + 3u + 1u] += giff_fs_scale(eg, 1);
                next_err[eidx + 3u + 2u] += giff_fs_scale(eb, 1);
            }

            rc = giff_encoder_write_indexed_rows(enc, enc->row_indexed, 1u, (giff_u32)temp.width);
            if (rc != GIFF_OK) {
                return rc;
            }

            {
                giff_s32* swap_rows;
                swap_rows = curr_err;
                curr_err = next_err;
                next_err = swap_rows;
            }
            giff_mem_zero(next_err, err_bytes);
        }
    } else {
        for (y = 0u; y < temp.height; ++y) {
            row = (const giff_rgba8*)((const giff_u8*)pixels + (giff_u32)y * stride);
            for (x = 0u; x < temp.width; ++x) {
                rc = giff_palette_map_rgba(
                    enc,
                    palette,
                    palette_entries,
                    transparent_index,
                    has_transparency,
                    &row[x],
                    x,
                    y,
                    enc->cfg.dither_mode,
                    use_octree_direct,
                    &enc->row_indexed[x]
                );
                if (rc != GIFF_OK) {
                    return rc;
                }
            }
            rc = giff_encoder_write_indexed_rows(enc, enc->row_indexed, 1u, (giff_u32)temp.width);
            if (rc != GIFF_OK) {
                return rc;
            }
        }
    }

    rc = giff_encoder_end_frame(enc);
    return rc;
}

giff_result giff_encoder_end(giff_encoder* enc)
{
    giff_u8 trailer;

    if (enc == 0) {
        return GIFF_E_INVALID_ARGUMENT;
    }

    if (!enc->started || enc->finished || enc->frame_active) {
        return GIFF_E_INVALID_ARGUMENT;
    }

    trailer = 0x3Bu;
    if (giff_encoder_write_bytes(enc, &trailer, 1u) != GIFF_OK) {
        return GIFF_E_OUTPUT_OVERFLOW;
    }

    enc->finished = 1u;
    return GIFF_OK;
}
