#include <stdio.h>
#include <string.h>
#include "giff/giff.h"

typedef struct mem_writer {
    giff_u8* dst;
    giff_u32 cap;
    giff_u32 size;
} mem_writer;

static int mem_write(void* user, const giff_u8* src, giff_u32 size)
{
    mem_writer* w;

    w = (mem_writer*)user;
    if (w == 0 || src == 0) {
        return 0;
    }
    if (size > w->cap - w->size) {
        return 0;
    }

    memcpy(w->dst + w->size, src, (size_t)size);
    w->size += size;
    return 1;
}

static unsigned char dws[1100000];
static unsigned char ews[1100000];
static unsigned char out_gif[4096];

int main(void)
{
    giff_decoder dec;
    giff_encoder enc;
    giff_decoder_config dcfg;
    giff_encoder_config ecfg;
    giff_result rc;
    giff_event ev;
    giff_io io;
    giff_info stream;
    giff_frame_info frame;
    giff_rgb8 palette[3];
    giff_u8 pixels[2];
    mem_writer writer;

    dcfg.max_width = 256u;
    dcfg.max_height = 256u;
    dcfg.strict_mode = 1u;
    dcfg.output_mode = (giff_u8)GIFF_OUTPUT_RGBA8888;
    dcfg.restore_previous_mode = (giff_u8)GIFF_RESTORE_PREVIOUS_BOUNDS;
    dcfg.capture_comments = 1u;
    dcfg.user_canvas_stride = 0u;
    dcfg.user_canvas_rgba = 0;
    dcfg.user_previous_stride = 0u;
    dcfg.user_previous_rgba = 0;

    ecfg.max_width = 256u;
    ecfg.max_height = 256u;
    ecfg.global_palette_entries = 256u;
    ecfg.reserve_transparent = 1u;
    ecfg.dither_mode = (giff_u8)GIFF_DITHER_NONE;
    ecfg.quantizer_hist_bits = 5u;
    ecfg.quantizer_mode = (giff_u8)GIFF_QUANTIZER_MEDIAN_CUT;
    ecfg.enable_cross_frame_cluster = 1u;
    ecfg.enable_temporal_remap = 1u;
    ecfg.lzw_band_rows = 8u;
    ecfg.temporal_decay_shift = 2u;
    ecfg.temporal_window_frames = 4u;
    ecfg.enable_segment_sharing = 1u;
    ecfg.segment_max_frames = 6u;
    ecfg.segment_similarity_q8_threshold = 176u;
    ecfg.user_indexed_stride = 0u;

    rc = giff_decoder_init(&dec, &dcfg, dws, (giff_u32)sizeof(dws));
    if (rc != GIFF_OK) {
        printf("decoder init failed: %s\n", giff_result_string(rc));
        return 1;
    }

    rc = giff_encoder_init(&enc, &ecfg, ews, (giff_u32)sizeof(ews));
    if (rc != GIFF_OK) {
        printf("encoder init failed: %s\n", giff_result_string(rc));
        return 1;
    }

    palette[0].r = 0u;   palette[0].g = 0u;   palette[0].b = 0u;
    palette[1].r = 255u; palette[1].g = 0u;   palette[1].b = 0u;
    palette[2].r = 0u;   palette[2].g = 255u; palette[2].b = 0u;

    writer.dst = out_gif;
    writer.cap = (giff_u32)sizeof(out_gif);
    writer.size = 0u;

    io.user = &writer;
    io.read = 0;
    io.write = mem_write;
    io.seek = 0;
    giff_encoder_attach_io(&enc, &io);

    rc = giff_encoder_set_global_palette(&enc, palette, 3u);
    if (rc != GIFF_OK) {
        printf("set palette failed: %s\n", giff_result_string(rc));
        return 1;
    }

    memset(&stream, 0, sizeof(stream));
    stream.width = 2u;
    stream.height = 1u;
    stream.loop_count = 0u;
    stream.version_major = 8u;
    stream.version_minor = (giff_u8)'9';
    stream.background_index = 0u;
    stream.global_palette_sorted = 0u;

    rc = giff_encoder_begin(&enc, &stream);
    if (rc != GIFF_OK) {
        printf("begin failed: %s\n", giff_result_string(rc));
        return 1;
    }

    memset(&frame, 0, sizeof(frame));
    frame.width = 2u;
    frame.height = 1u;
    frame.delay_cs = 5u;
    frame.disposal_method = 1u;

    pixels[0] = 1u;
    pixels[1] = 2u;

    rc = giff_encoder_write_indexed_frame(&enc, &frame, pixels, 2u);
    if (rc != GIFF_OK) {
        printf("write frame failed: %s\n", giff_result_string(rc));
        return 1;
    }

    rc = giff_encoder_end(&enc);
    if (rc != GIFF_OK) {
        printf("end failed: %s\n", giff_result_string(rc));
        return 1;
    }

    printf("encoded %lu bytes\n", (unsigned long)writer.size);

    rc = giff_decoder_begin_memory(&dec, out_gif, writer.size);
    if (rc != GIFF_OK) {
        printf("decode begin failed: %s\n", giff_result_string(rc));
        return 1;
    }

    while ((rc = giff_decoder_next_event(&dec, &ev)) == GIFF_OK) {
        switch (ev.type) {
            case GIFF_EVENT_INFO:
                printf("gif version = %u%ca\n", (unsigned)ev.info.version_major, (char)ev.info.version_minor);
                printf("canvas = %ux%u loop=%u\n",
                       (unsigned)ev.info.width,
                       (unsigned)ev.info.height,
                       (unsigned)ev.info.loop_count);
                break;
            case GIFF_EVENT_FRAME_ROW:
                if (ev.rgba_row != 0) {
                    printf("row %u : (%u,%u,%u,%u) (%u,%u,%u,%u)\n",
                           (unsigned)ev.row_index,
                           (unsigned)ev.rgba_row[0].r,
                           (unsigned)ev.rgba_row[0].g,
                           (unsigned)ev.rgba_row[0].b,
                           (unsigned)ev.rgba_row[0].a,
                           (unsigned)ev.rgba_row[1].r,
                           (unsigned)ev.rgba_row[1].g,
                           (unsigned)ev.rgba_row[1].b,
                           (unsigned)ev.rgba_row[1].a);
                }
                break;
            case GIFF_EVENT_TRAILER:
                printf("frames = %lu\n", (unsigned long)dec.frames_seen);
                return 0;
            default:
                break;
        }
    }

    printf("decode failed: %s\n", giff_result_string(rc));
    return 1;
}
