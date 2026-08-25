#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "giff/giff.h"

static const giff_u8 k_white_gif[] = {
    0x47,0x49,0x46,0x38,0x37,0x61,0x01,0x00,0x01,0x00,0x81,0x00,0x00,
    0xFF,0xFF,0xFF,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x2C,0x00,0x00,0x00,0x00,0x01,0x00,0x01,0x00,0x00,
    0x08,0x04,0x00,0x01,0x04,0x04,0x00,0x3B
};

static const giff_u8 k_comment_gif[] = {
    0x47,0x49,0x46,0x38,0x39,0x61,0x01,0x00,0x01,0x00,0x81,0x00,0x00,
    0x00,0x00,0x00,0xFF,0xFF,0xFF,0x00,0x00,0x00,0x00,0x00,0x00,
    0x21,0xFE,0x03,'H','i','!',0x00,
    0x2C,0x00,0x00,0x00,0x00,0x01,0x00,0x01,0x00,0x00,
    0x02,0x02,0x4C,0x01,0x00,
    0x3B
};

typedef struct mem_writer {
    giff_u8* dst;
    giff_u32 cap;
    giff_u32 size;
} mem_writer;

typedef struct mem_reader {
    const giff_u8* src;
    giff_u32 size;
    giff_u32 pos;
    giff_u32 chunk;
} mem_reader;

static unsigned char k_workspace[1 << 20];
static unsigned char k_outbuf[1 << 20];

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

static int mem_read(void* user, giff_u8* dst, giff_u32* io_size)
{
    mem_reader* r;
    giff_u32 take;

    r = (mem_reader*)user;
    if (r == 0 || dst == 0 || io_size == 0) {
        return 0;
    }

    if (r->pos >= r->size) {
        *io_size = 0u;
        return 1;
    }

    take = *io_size;
    if (take > r->chunk) {
        take = r->chunk;
    }
    if (take > r->size - r->pos) {
        take = r->size - r->pos;
    }

    memcpy(dst, r->src + r->pos, (size_t)take);
    r->pos += take;
    *io_size = take;
    return 1;
}


static giff_u32 read_file_bytes(const char* path, giff_u8* dst, giff_u32 cap)
{
    FILE* fp;
    size_t got;

    fp = fopen(path, "rb");
    assert(fp != 0);
    got = fread(dst, 1u, (size_t)cap, fp);
    fclose(fp);
    return (giff_u32)got;
}

static void init_cfg(giff_decoder_config* cfg, giff_u8 mode, giff_u8 capture)
{
    cfg->max_width = 32u;
    cfg->max_height = 32u;
    cfg->strict_mode = 1u;
    cfg->output_mode = mode;
    cfg->restore_previous_mode = (giff_u8)GIFF_RESTORE_PREVIOUS_BOUNDS;
    cfg->capture_comments = capture;
    cfg->user_canvas_stride = 0u;
    cfg->user_canvas_rgba = 0;
    cfg->user_previous_stride = 0u;
    cfg->user_previous_rgba = 0;
}

static void init_ecfg(giff_encoder_config* cfg)
{
    cfg->max_width = 32u;
    cfg->max_height = 32u;
    cfg->global_palette_entries = 256u;
    cfg->reserve_transparent = 1u;
    cfg->dither_mode = (giff_u8)GIFF_DITHER_ORDERED4X4;
    cfg->quantizer_hist_bits = 5u;
    cfg->quantizer_mode = (giff_u8)GIFF_QUANTIZER_MEDIAN_CUT;
    cfg->enable_cross_frame_cluster = 1u;
    cfg->enable_temporal_remap = 1u;
    cfg->lzw_band_rows = 8u;
    cfg->temporal_decay_shift = 2u;
    cfg->temporal_window_frames = 4u;
    cfg->enable_segment_sharing = 1u;
    cfg->segment_max_frames = 6u;
    cfg->segment_similarity_q8_threshold = 176u;
    cfg->user_indexed_stride = 0u;
}

static void test_one_shot_decode(void)
{
    giff_info info;
    giff_result rc;
    giff_decoder dec;
    giff_decoder_config cfg;

    rc = giff_decoder_inspect_header(k_white_gif, (giff_u32)sizeof(k_white_gif), &info);
    assert(rc == GIFF_OK);
    assert(info.width == 1u);
    assert(info.height == 1u);
    assert(info.has_global_palette == 1u);
    assert(info.global_palette_entries == 4u);

    init_cfg(&cfg, (giff_u8)GIFF_OUTPUT_RGBA8888, 0u);
    rc = giff_decoder_init(&dec, &cfg, k_workspace, (giff_u32)sizeof(k_workspace));
    assert(rc == GIFF_OK);

    rc = giff_decoder_decode_memory(&dec, k_white_gif, (giff_u32)sizeof(k_white_gif));
    assert(rc == GIFF_OK);
    assert(dec.frames_seen == 1u);
    assert(dec.canvas_rgba[0] == 255u);
    assert(dec.canvas_rgba[1] == 255u);
    assert(dec.canvas_rgba[2] == 255u);
    assert(dec.canvas_rgba[3] == 255u);
}

static void test_stream_comment_decode(void)
{
    giff_decoder dec;
    giff_decoder_config cfg;
    giff_result rc;
    giff_event ev;
    giff_io io;
    mem_reader reader;
    int saw_comment;

    init_cfg(&cfg, (giff_u8)GIFF_OUTPUT_RGBA8888, 1u);
    rc = giff_decoder_init(&dec, &cfg, k_workspace, (giff_u32)sizeof(k_workspace));
    assert(rc == GIFF_OK);

    reader.src = k_comment_gif;
    reader.size = (giff_u32)sizeof(k_comment_gif);
    reader.pos = 0u;
    reader.chunk = 3u;
    io.user = &reader;
    io.read = mem_read;
    io.write = 0;
    io.seek = 0;
    giff_decoder_attach_io(&dec, &io);

    rc = giff_decoder_begin_io(&dec);
    assert(rc == GIFF_OK);

    saw_comment = 0;
    while ((rc = giff_decoder_next_event(&dec, &ev)) == GIFF_OK) {
        if (ev.type == GIFF_EVENT_COMMENT) {
            saw_comment = 1;
            assert(ev.text_size == 3u);
            assert(memcmp(ev.text_bytes, "Hi!", 3u) == 0);
        }
        if (ev.type == GIFF_EVENT_TRAILER) {
            break;
        }
    }

    assert(rc == GIFF_OK);
    assert(saw_comment);
    assert(dec.frames_seen == 1u);
    assert(dec.canvas_rgba[0] == 255u);
    assert(dec.canvas_rgba[1] == 255u);
    assert(dec.canvas_rgba[2] == 255u);
}

static void test_encode_roundtrip_indexed(void)
{
    giff_encoder enc;
    giff_encoder_config ecfg;
    giff_decoder dec;
    giff_decoder_config dcfg;
    giff_result rc;
    giff_io io;
    mem_writer writer;
    giff_rgb8 palette[3];
    giff_info stream;
    giff_frame_info frame;
    giff_u8 pixels[2];

    init_ecfg(&ecfg);
    rc = giff_encoder_init(&enc, &ecfg, k_workspace, (giff_u32)sizeof(k_workspace));
    assert(rc == GIFF_OK);

    palette[0].r = 0u; palette[0].g = 0u; palette[0].b = 0u;
    palette[1].r = 255u; palette[1].g = 0u; palette[1].b = 0u;
    palette[2].r = 0u; palette[2].g = 255u; palette[2].b = 0u;
    rc = giff_encoder_set_global_palette(&enc, palette, 3u);
    assert(rc == GIFF_OK);

    writer.dst = k_outbuf;
    writer.cap = (giff_u32)sizeof(k_outbuf);
    writer.size = 0u;
    io.user = &writer;
    io.read = 0;
    io.write = mem_write;
    io.seek = 0;
    giff_encoder_attach_io(&enc, &io);

    memset(&stream, 0, sizeof(stream));
    stream.width = 2u;
    stream.height = 1u;
    stream.version_major = 8u;
    stream.version_minor = (giff_u8)'9';
    stream.background_index = 0u;
    stream.loop_count = 1u;

    rc = giff_encoder_begin(&enc, &stream);
    assert(rc == GIFF_OK);

    memset(&frame, 0, sizeof(frame));
    frame.width = 2u;
    frame.height = 1u;
    pixels[0] = 1u;
    pixels[1] = 2u;

    rc = giff_encoder_write_indexed_frame(&enc, &frame, pixels, 2u);
    assert(rc == GIFF_OK);
    rc = giff_encoder_end(&enc);
    assert(rc == GIFF_OK);

    init_cfg(&dcfg, (giff_u8)GIFF_OUTPUT_RGBA8888, 0u);
    rc = giff_decoder_init(&dec, &dcfg, k_workspace, (giff_u32)sizeof(k_workspace));
    assert(rc == GIFF_OK);
    rc = giff_decoder_decode_memory(&dec, k_outbuf, writer.size);
    assert(rc == GIFF_OK);
    assert(dec.canvas_rgba[0] == 255u);
    assert(dec.canvas_rgba[1] == 0u);
    assert(dec.canvas_rgba[2] == 0u);
    assert(dec.canvas_rgba[4] == 0u);
    assert(dec.canvas_rgba[5] == 255u);
    assert(dec.canvas_rgba[6] == 0u);
}

static void test_encode_rgba_global_quantized(void)
{
    giff_encoder enc;
    giff_encoder_config ecfg;
    giff_decoder dec;
    giff_decoder_config dcfg;
    giff_result rc;
    giff_io io;
    mem_writer writer;
    giff_info stream;
    giff_frame_info frame;
    giff_rgba8 pixels[4];

    init_ecfg(&ecfg);
    ecfg.dither_mode = (giff_u8)GIFF_DITHER_NONE;
    rc = giff_encoder_init(&enc, &ecfg, k_workspace, (giff_u32)sizeof(k_workspace));
    assert(rc == GIFF_OK);

    pixels[0].r = 255u; pixels[0].g = 0u;   pixels[0].b = 0u;   pixels[0].a = 255u;
    pixels[1].r = 0u;   pixels[1].g = 255u; pixels[1].b = 0u;   pixels[1].a = 255u;
    pixels[2].r = 0u;   pixels[2].g = 0u;   pixels[2].b = 255u; pixels[2].a = 255u;
    pixels[3].r = 255u; pixels[3].g = 255u; pixels[3].b = 255u; pixels[3].a = 255u;

    rc = giff_encoder_build_palette_from_rgba(&enc, pixels, 2u, 2u, 0u, 0u, 4u);
    assert(rc == GIFF_OK);

    writer.dst = k_outbuf;
    writer.cap = (giff_u32)sizeof(k_outbuf);
    writer.size = 0u;
    io.user = &writer;
    io.read = 0;
    io.write = mem_write;
    io.seek = 0;
    giff_encoder_attach_io(&enc, &io);

    memset(&stream, 0, sizeof(stream));
    stream.width = 2u;
    stream.height = 2u;
    stream.version_major = 8u;
    stream.version_minor = (giff_u8)'9';
    stream.background_index = 0u;
    stream.loop_count = 1u;

    rc = giff_encoder_begin(&enc, &stream);
    assert(rc == GIFF_OK);

    memset(&frame, 0, sizeof(frame));
    frame.width = 2u;
    frame.height = 2u;

    rc = giff_encoder_write_rgba_frame(&enc, &frame, pixels, 0u, 0u);
    assert(rc == GIFF_OK);
    rc = giff_encoder_end(&enc);
    assert(rc == GIFF_OK);

    init_cfg(&dcfg, (giff_u8)GIFF_OUTPUT_RGBA8888, 0u);
    rc = giff_decoder_init(&dec, &dcfg, k_workspace, (giff_u32)sizeof(k_workspace));
    assert(rc == GIFF_OK);
    rc = giff_decoder_decode_memory(&dec, k_outbuf, writer.size);
    assert(rc == GIFF_OK);

    assert(dec.canvas_rgba[0] == 255u && dec.canvas_rgba[1] == 0u && dec.canvas_rgba[2] == 0u);
    assert(dec.canvas_rgba[4] == 0u && dec.canvas_rgba[5] == 255u && dec.canvas_rgba[6] == 0u);
    assert(dec.canvas_rgba[8] == 0u && dec.canvas_rgba[9] == 0u && dec.canvas_rgba[10] == 255u);
    assert(dec.canvas_rgba[12] == 255u && dec.canvas_rgba[13] == 255u && dec.canvas_rgba[14] == 255u);
}

static void test_encode_rgba_local_palette(void)
{
    giff_encoder enc;
    giff_encoder_config ecfg;
    giff_decoder dec;
    giff_decoder_config dcfg;
    giff_result rc;
    giff_io io;
    mem_writer writer;
    giff_rgb8 palette[2];
    giff_info stream;
    giff_frame_info frame;
    giff_rgba8 pixels[2];

    init_ecfg(&ecfg);
    ecfg.dither_mode = (giff_u8)GIFF_DITHER_NONE;
    rc = giff_encoder_init(&enc, &ecfg, k_workspace, (giff_u32)sizeof(k_workspace));
    assert(rc == GIFF_OK);

    palette[0].r = 0u; palette[0].g = 0u; palette[0].b = 0u;
    palette[1].r = 255u; palette[1].g = 255u; palette[1].b = 255u;
    rc = giff_encoder_set_global_palette(&enc, palette, 2u);
    assert(rc == GIFF_OK);

    writer.dst = k_outbuf;
    writer.cap = (giff_u32)sizeof(k_outbuf);
    writer.size = 0u;
    io.user = &writer;
    io.read = 0;
    io.write = mem_write;
    io.seek = 0;
    giff_encoder_attach_io(&enc, &io);

    memset(&stream, 0, sizeof(stream));
    stream.width = 2u;
    stream.height = 1u;
    stream.version_major = 8u;
    stream.version_minor = (giff_u8)'9';
    stream.background_index = 0u;
    stream.loop_count = 1u;
    rc = giff_encoder_begin(&enc, &stream);
    assert(rc == GIFF_OK);

    pixels[0].r = 255u; pixels[0].g = 0u;   pixels[0].b = 255u; pixels[0].a = 255u;
    pixels[1].r = 0u;   pixels[1].g = 255u; pixels[1].b = 255u; pixels[1].a = 255u;

    memset(&frame, 0, sizeof(frame));
    frame.width = 2u;
    frame.height = 1u;
    frame.local_palette_entries = 2u;

    rc = giff_encoder_write_rgba_frame(&enc, &frame, pixels, 0u, 1u);
    assert(rc == GIFF_OK);
    rc = giff_encoder_end(&enc);
    assert(rc == GIFF_OK);

    init_cfg(&dcfg, (giff_u8)GIFF_OUTPUT_RGBA8888, 0u);
    rc = giff_decoder_init(&dec, &dcfg, k_workspace, (giff_u32)sizeof(k_workspace));
    assert(rc == GIFF_OK);
    rc = giff_decoder_decode_memory(&dec, k_outbuf, writer.size);
    assert(rc == GIFF_OK);

    assert(dec.frame.has_local_palette == 1u);
    assert(dec.canvas_rgba[0] == 255u && dec.canvas_rgba[1] == 0u && dec.canvas_rgba[2] == 255u);
    assert(dec.canvas_rgba[4] == 0u && dec.canvas_rgba[5] == 255u && dec.canvas_rgba[6] == 255u);
}


static void test_feed_incremental_decode(void)
{
    giff_decoder dec;
    giff_decoder_config cfg;
    giff_result rc;
    giff_event ev;
    giff_u32 pos;
    int saw_comment;
    int saw_trailer;

    init_cfg(&cfg, (giff_u8)GIFF_OUTPUT_RGBA8888, 1u);
    rc = giff_decoder_init(&dec, &cfg, k_workspace, (giff_u32)sizeof(k_workspace));
    assert(rc == GIFF_OK);

    pos = 0u;
    saw_comment = 0;
    saw_trailer = 0;

    while (pos < (giff_u32)sizeof(k_comment_gif)) {
        giff_u32 consumed;
        giff_u32 chunk;

        chunk = 2u;
        if (chunk > (giff_u32)sizeof(k_comment_gif) - pos) {
            chunk = (giff_u32)sizeof(k_comment_gif) - pos;
        }
        rc = giff_decoder_feed(&dec, k_comment_gif + pos, chunk, &consumed);
        assert(consumed == chunk);
        assert(rc == GIFF_OK || rc == GIFF_E_NEED_MORE_INPUT);
        pos += consumed;

        while ((rc = giff_decoder_next_event(&dec, &ev)) == GIFF_OK) {
            if (ev.type == GIFF_EVENT_COMMENT) {
                saw_comment = 1;
                assert(ev.text_size == 3u);
            }
            if (ev.type == GIFF_EVENT_TRAILER) {
                saw_trailer = 1;
                break;
            }
        }
        assert(rc == GIFF_E_NEED_MORE_INPUT || rc == GIFF_OK || rc == GIFF_E_DONE);
        if (saw_trailer) {
            break;
        }
    }

    giff_decoder_finish_input(&dec);
    while (!saw_trailer && (rc = giff_decoder_next_event(&dec, &ev)) == GIFF_OK) {
        if (ev.type == GIFF_EVENT_COMMENT) {
            saw_comment = 1;
        }
        if (ev.type == GIFF_EVENT_TRAILER) {
            saw_trailer = 1;
            break;
        }
    }

    assert(saw_comment);
    assert(saw_trailer);
}

static void test_quantizer_median_cut(void)
{
    giff_encoder enc;
    giff_encoder_config ecfg;
    giff_result rc;
    giff_rgba8 pixels[16];
    giff_u16 i;
    giff_u8 minv;
    giff_u8 maxv;

    init_ecfg(&ecfg);
    ecfg.dither_mode = (giff_u8)GIFF_DITHER_NONE;
    rc = giff_encoder_init(&enc, &ecfg, k_workspace, (giff_u32)sizeof(k_workspace));
    assert(rc == GIFF_OK);

    for (i = 0u; i < 16u; ++i) {
        pixels[i].r = (giff_u8)(i * 16u);
        pixels[i].g = (giff_u8)(255u - i * 8u);
        pixels[i].b = (giff_u8)(i * 12u);
        pixels[i].a = 255u;
    }

    rc = giff_encoder_build_palette_from_rgba(&enc, pixels, 4u, 4u, 0u, 0u, 4u);
    assert(rc == GIFF_OK);
    assert(enc.global_palette_entries == 4u);

    minv = enc.global_palette[0].r;
    maxv = enc.global_palette[0].r;
    for (i = 1u; i < enc.global_palette_entries; ++i) {
        if (enc.global_palette[i].r < minv) minv = enc.global_palette[i].r;
        if (enc.global_palette[i].r > maxv) maxv = enc.global_palette[i].r;
    }
    assert((giff_u16)(maxv - minv) > 32u);
}

static void test_dither_floyd_steinberg(void)
{
    giff_encoder enc;
    giff_encoder_config ecfg;
    giff_decoder dec;
    giff_decoder_config dcfg;
    giff_result rc;
    giff_io io;
    mem_writer writer;
    giff_rgb8 palette[2];
    giff_info stream;
    giff_frame_info frame;
    giff_rgba8 pixels[8];
    giff_u16 i;
    int saw_black;
    int saw_white;

    init_ecfg(&ecfg);
    ecfg.dither_mode = (giff_u8)GIFF_DITHER_FLOYD_STEINBERG;
    rc = giff_encoder_init(&enc, &ecfg, k_workspace, (giff_u32)sizeof(k_workspace));
    assert(rc == GIFF_OK);

    palette[0].r = 0u; palette[0].g = 0u; palette[0].b = 0u;
    palette[1].r = 255u; palette[1].g = 255u; palette[1].b = 255u;
    rc = giff_encoder_set_global_palette(&enc, palette, 2u);
    assert(rc == GIFF_OK);

    for (i = 0u; i < 8u; ++i) {
        pixels[i].r = 128u;
        pixels[i].g = 128u;
        pixels[i].b = 128u;
        pixels[i].a = 255u;
    }

    writer.dst = k_outbuf;
    writer.cap = (giff_u32)sizeof(k_outbuf);
    writer.size = 0u;
    io.user = &writer;
    io.read = 0;
    io.write = mem_write;
    io.seek = 0;
    giff_encoder_attach_io(&enc, &io);

    memset(&stream, 0, sizeof(stream));
    stream.width = 8u;
    stream.height = 1u;
    stream.version_major = 8u;
    stream.version_minor = (giff_u8)'9';
    stream.background_index = 0u;
    stream.loop_count = 1u;

    rc = giff_encoder_begin(&enc, &stream);
    assert(rc == GIFF_OK);

    memset(&frame, 0, sizeof(frame));
    frame.width = 8u;
    frame.height = 1u;
    rc = giff_encoder_write_rgba_frame(&enc, &frame, pixels, 0u, 0u);
    assert(rc == GIFF_OK);
    rc = giff_encoder_end(&enc);
    assert(rc == GIFF_OK);

    init_cfg(&dcfg, (giff_u8)GIFF_OUTPUT_INDEXED, 0u);
    rc = giff_decoder_init(&dec, &dcfg, k_workspace, (giff_u32)sizeof(k_workspace));
    assert(rc == GIFF_OK);
    rc = giff_decoder_decode_memory(&dec, k_outbuf, writer.size);
    assert(rc == GIFF_OK);

    saw_black = 0;
    saw_white = 0;
    for (i = 0u; i < 8u; ++i) {
        if (dec.canvas_indexed[i] == 0u) saw_black = 1;
        if (dec.canvas_indexed[i] == 1u) saw_white = 1;
    }
    assert(saw_black && saw_white);
}

static void test_quantizer_octree(void)
{
    giff_encoder enc;
    giff_encoder_config ecfg;
    giff_result rc;
    giff_rgba8 pixels[16];
    giff_u16 i;
    giff_u8 minv;
    giff_u8 maxv;

    init_ecfg(&ecfg);
    ecfg.dither_mode = (giff_u8)GIFF_DITHER_NONE;
    ecfg.quantizer_mode = (giff_u8)GIFF_QUANTIZER_OCTREE;
    rc = giff_encoder_init(&enc, &ecfg, k_workspace, (giff_u32)sizeof(k_workspace));
    assert(rc == GIFF_OK);

    for (i = 0u; i < 16u; ++i) {
        pixels[i].r = (giff_u8)(i * 16u);
        pixels[i].g = (giff_u8)(255u - i * 8u);
        pixels[i].b = (giff_u8)(i * 12u);
        pixels[i].a = 255u;
    }

    rc = giff_encoder_build_palette_from_rgba(&enc, pixels, 4u, 4u, 0u, 0u, 4u);
    assert(rc == GIFF_OK);
    assert(enc.global_palette_entries >= 2u);

    minv = enc.global_palette[0].g;
    maxv = enc.global_palette[0].g;
    for (i = 1u; i < enc.global_palette_entries; ++i) {
        if (enc.global_palette[i].g < minv) minv = enc.global_palette[i].g;
        if (enc.global_palette[i].g > maxv) maxv = enc.global_palette[i].g;
    }
    assert((giff_u16)(maxv - minv) > 16u);
}

static void test_interlaced_encode_roundtrip(void)
{
    giff_encoder enc;
    giff_encoder_config ecfg;
    giff_decoder dec;
    giff_decoder_config dcfg;
    giff_result rc;
    giff_io io;
    mem_writer writer;
    giff_rgb8 palette[4];
    giff_info stream;
    giff_frame_info frame;
    giff_u8 pixels[16];
    giff_u16 i;

    init_ecfg(&ecfg);
    rc = giff_encoder_init(&enc, &ecfg, k_workspace, (giff_u32)sizeof(k_workspace));
    assert(rc == GIFF_OK);

    palette[0].r = 0u;   palette[0].g = 0u;   palette[0].b = 0u;
    palette[1].r = 255u; palette[1].g = 0u;   palette[1].b = 0u;
    palette[2].r = 0u;   palette[2].g = 255u; palette[2].b = 0u;
    palette[3].r = 0u;   palette[3].g = 0u;   palette[3].b = 255u;
    rc = giff_encoder_set_global_palette(&enc, palette, 4u);
    assert(rc == GIFF_OK);

    for (i = 0u; i < 16u; ++i) {
        pixels[i] = (giff_u8)(i & 3u);
    }

    writer.dst = k_outbuf;
    writer.cap = (giff_u32)sizeof(k_outbuf);
    writer.size = 0u;
    io.user = &writer;
    io.read = 0;
    io.write = mem_write;
    io.seek = 0;
    giff_encoder_attach_io(&enc, &io);

    memset(&stream, 0, sizeof(stream));
    stream.width = 4u;
    stream.height = 4u;
    stream.version_major = 8u;
    stream.version_minor = (giff_u8)'9';
    stream.background_index = 0u;
    stream.loop_count = 1u;
    rc = giff_encoder_begin(&enc, &stream);
    assert(rc == GIFF_OK);

    memset(&frame, 0, sizeof(frame));
    frame.width = 4u;
    frame.height = 4u;
    frame.interlaced = 1u;
    rc = giff_encoder_write_indexed_frame(&enc, &frame, pixels, 4u);
    assert(rc == GIFF_OK);
    rc = giff_encoder_end(&enc);
    assert(rc == GIFF_OK);

    init_cfg(&dcfg, (giff_u8)GIFF_OUTPUT_INDEXED, 0u);
    rc = giff_decoder_init(&dec, &dcfg, k_workspace, (giff_u32)sizeof(k_workspace));
    assert(rc == GIFF_OK);
    rc = giff_decoder_decode_memory(&dec, k_outbuf, writer.size);
    assert(rc == GIFF_OK);
    for (i = 0u; i < 16u; ++i) {
        assert(dec.canvas_indexed[i] == pixels[i]);
    }
}

static void test_feed_compactor_large_stream(void)
{
    giff_encoder enc;
    giff_encoder_config ecfg;
    giff_decoder dec;
    giff_decoder_config dcfg;
    giff_result rc;
    giff_io io;
    mem_writer writer;
    giff_rgb8 palette[256];
    giff_info stream;
    giff_frame_info frame;
    giff_u8 pixels[64u * 64u];
    giff_u32 frame_bytes;
    giff_u16 i;
    giff_u16 f;
    giff_u32 pos;
    int saw_trailer;

    init_ecfg(&ecfg);
    ecfg.max_width = 64u;
    ecfg.max_height = 64u;
    rc = giff_encoder_init(&enc, &ecfg, k_workspace, (giff_u32)sizeof(k_workspace));
    assert(rc == GIFF_OK);

    for (i = 0u; i < 256u; ++i) {
        palette[i].r = (giff_u8)i;
        palette[i].g = (giff_u8)((i * 29u) & 255u);
        palette[i].b = (giff_u8)((i * 53u) & 255u);
    }
    rc = giff_encoder_set_global_palette(&enc, palette, 256u);
    assert(rc == GIFF_OK);

    writer.dst = k_outbuf;
    writer.cap = (giff_u32)sizeof(k_outbuf);
    writer.size = 0u;
    io.user = &writer;
    io.read = 0;
    io.write = mem_write;
    io.seek = 0;
    giff_encoder_attach_io(&enc, &io);

    memset(&stream, 0, sizeof(stream));
    stream.width = 64u;
    stream.height = 64u;
    stream.version_major = 8u;
    stream.version_minor = (giff_u8)'9';
    stream.background_index = 0u;
    stream.loop_count = 0u;
    rc = giff_encoder_begin(&enc, &stream);
    assert(rc == GIFF_OK);

    frame_bytes = 64u * 64u;
    for (f = 0u; f < 64u; ++f) {
        for (i = 0u; i < frame_bytes; ++i) {
            pixels[i] = (giff_u8)((i * 37u + f * 17u + (i >> 3)) & 255u);
        }
        memset(&frame, 0, sizeof(frame));
        frame.width = 64u;
        frame.height = 64u;
        frame.delay_cs = 1u;
        frame.disposal_method = 1u;
        rc = giff_encoder_write_indexed_frame(&enc, &frame, pixels, 64u);
        assert(rc == GIFF_OK);
    }
    rc = giff_encoder_end(&enc);
    assert(rc == GIFF_OK);
    assert(writer.size > 65536u);

    dcfg.max_width = 64u;
    dcfg.max_height = 64u;
    dcfg.strict_mode = 1u;
    dcfg.output_mode = (giff_u8)GIFF_OUTPUT_INDEXED;
    dcfg.restore_previous_mode = (giff_u8)GIFF_RESTORE_PREVIOUS_BOUNDS;
    dcfg.capture_comments = 0u;
    dcfg.user_canvas_stride = 0u;
    dcfg.user_canvas_rgba = 0;
    dcfg.user_previous_stride = 0u;
    dcfg.user_previous_rgba = 0;

    rc = giff_decoder_init(&dec, &dcfg, k_workspace, (giff_u32)sizeof(k_workspace));
    assert(rc == GIFF_OK);

    pos = 0u;
    saw_trailer = 0;
    while (pos < writer.size) {
        giff_u32 chunk;
        giff_u32 consumed;
        giff_event ev;

        chunk = 4096u;
        if (chunk > writer.size - pos) {
            chunk = writer.size - pos;
        }
        rc = giff_decoder_feed(&dec, writer.dst + pos, chunk, &consumed);
        assert(consumed == chunk);
        assert(rc == GIFF_OK || rc == GIFF_E_NEED_MORE_INPUT);
        pos += consumed;

        while ((rc = giff_decoder_next_event(&dec, &ev)) == GIFF_OK) {
            if (ev.type == GIFF_EVENT_TRAILER) {
                saw_trailer = 1;
                break;
            }
        }
        assert(rc == GIFF_E_NEED_MORE_INPUT || rc == GIFF_OK || rc == GIFF_E_DONE);
        if (saw_trailer) {
            break;
        }
    }

    giff_decoder_finish_input(&dec);
    while (!saw_trailer) {
        giff_event ev;
        rc = giff_decoder_next_event(&dec, &ev);
        if (rc != GIFF_OK) {
            break;
        }
        if (ev.type == GIFF_EVENT_TRAILER) {
            saw_trailer = 1;
            break;
        }
    }

    assert(saw_trailer);
    assert(dec.stream_base_offset > 0u);
    assert(dec.frames_seen == 64u);
}


static void test_malformed_inputs(void)
{
    giff_info info;
    giff_decoder dec;
    giff_decoder_config cfg;
    giff_result rc;
    giff_u8 bad_hdr[13];

    memcpy(bad_hdr, k_white_gif, 13u);
    bad_hdr[0] = (giff_u8)'B';
    rc = giff_decoder_inspect_header(bad_hdr, 13u, &info);
    assert(rc == GIFF_E_INVALID_SIGNATURE);

    init_cfg(&cfg, (giff_u8)GIFF_OUTPUT_RGBA8888, 0u);
    rc = giff_decoder_init(&dec, &cfg, k_workspace, (giff_u32)sizeof(k_workspace));
    assert(rc == GIFF_OK);
    rc = giff_decoder_decode_memory(&dec, k_comment_gif, (giff_u32)sizeof(k_comment_gif) - 1u);
    assert(rc == GIFF_E_TRUNCATED);

    {
        giff_event ev;
        rc = giff_decoder_init(&dec, &cfg, k_workspace, (giff_u32)sizeof(k_workspace));
        assert(rc == GIFF_OK);
        rc = giff_decoder_feed(&dec, k_comment_gif, (giff_u32)sizeof(k_comment_gif) - 2u, 0);
        assert(rc == GIFF_OK || rc == GIFF_E_NEED_MORE_INPUT);
        giff_decoder_finish_input(&dec);
        while ((rc = giff_decoder_next_event(&dec, &ev)) == GIFF_OK) {
        }
        assert(rc == GIFF_E_TRUNCATED);
    }
}

static void test_fuzz_like_mutations(void)
{
    giff_u8 mutated[sizeof(k_comment_gif)];
    giff_decoder dec;
    giff_decoder_config cfg;
    giff_result rc;
    giff_u16 i;
    giff_u16 j;

    init_cfg(&cfg, (giff_u8)GIFF_OUTPUT_RGBA8888, 0u);

    for (i = 0u; i < 64u; ++i) {
        memcpy(mutated, k_comment_gif, sizeof(k_comment_gif));
        mutated[i % (giff_u16)sizeof(k_comment_gif)] ^= (giff_u8)(1u << (i & 7u));
        if ((i & 3u) == 0u) {
            mutated[(giff_u16)((i * 7u) % (giff_u16)sizeof(k_comment_gif))] = 0xFFu;
        }

        rc = giff_decoder_init(&dec, &cfg, k_workspace, (giff_u32)sizeof(k_workspace));
        assert(rc == GIFF_OK);
        rc = giff_decoder_decode_memory(&dec, mutated, (giff_u32)sizeof(mutated));
        assert(rc != GIFF_E_INTERNAL);

        rc = giff_decoder_init(&dec, &cfg, k_workspace, (giff_u32)sizeof(k_workspace));
        assert(rc == GIFF_OK);
        for (j = 0u; j < (giff_u16)sizeof(mutated); j = (giff_u16)(j + 3u)) {
            giff_u32 take;
            take = 3u;
            if ((giff_u32)j + take > (giff_u32)sizeof(mutated)) {
                take = (giff_u32)sizeof(mutated) - (giff_u32)j;
            }
            rc = giff_decoder_feed(&dec, mutated + j, take, 0);
            assert(rc != GIFF_E_INTERNAL);
        }
        giff_decoder_finish_input(&dec);
        do {
            giff_event ev;
            rc = giff_decoder_next_event(&dec, &ev);
        } while (rc == GIFF_OK);
        assert(rc != GIFF_E_INTERNAL);
    }
}


static void test_octree_rgba_roundtrip_direct(void)
{
    giff_encoder enc;
    giff_encoder_config ecfg;
    giff_decoder dec;
    giff_decoder_config dcfg;
    giff_result rc;
    giff_io io;
    mem_writer writer;
    giff_info stream;
    giff_frame_info frame;
    giff_rgba8 pixels[16];
    giff_u16 i;
    giff_u8 saw_change;

    init_ecfg(&ecfg);
    ecfg.dither_mode = (giff_u8)GIFF_DITHER_NONE;
    ecfg.quantizer_mode = (giff_u8)GIFF_QUANTIZER_OCTREE;
    rc = giff_encoder_init(&enc, &ecfg, k_workspace, (giff_u32)sizeof(k_workspace));
    assert(rc == GIFF_OK);

    for (i = 0u; i < 16u; ++i) {
        pixels[i].r = (giff_u8)(i * 15u);
        pixels[i].g = (giff_u8)(255u - i * 10u);
        pixels[i].b = (giff_u8)(32u + i * 11u);
        pixels[i].a = 255u;
    }

    rc = giff_encoder_build_palette_from_rgba(&enc, pixels, 4u, 4u, 0u, 0u, 8u);
    assert(rc == GIFF_OK);
    assert(enc.octree_map_valid);
    assert(enc.octree_map_is_local == 0u);

    writer.dst = k_outbuf;
    writer.cap = (giff_u32)sizeof(k_outbuf);
    writer.size = 0u;
    io.user = &writer;
    io.read = 0;
    io.write = mem_write;
    io.seek = 0;
    giff_encoder_attach_io(&enc, &io);

    memset(&stream, 0, sizeof(stream));
    stream.width = 4u;
    stream.height = 4u;
    stream.version_major = 8u;
    stream.version_minor = (giff_u8)'9';
    stream.background_index = 0u;
    stream.loop_count = 1u;
    rc = giff_encoder_begin(&enc, &stream);
    assert(rc == GIFF_OK);

    memset(&frame, 0, sizeof(frame));
    frame.width = 4u;
    frame.height = 4u;
    rc = giff_encoder_write_rgba_frame(&enc, &frame, pixels, 0u, 0u);
    assert(rc == GIFF_OK);
    rc = giff_encoder_end(&enc);
    assert(rc == GIFF_OK);

    init_cfg(&dcfg, (giff_u8)GIFF_OUTPUT_INDEXED, 0u);
    rc = giff_decoder_init(&dec, &dcfg, k_workspace, (giff_u32)sizeof(k_workspace));
    assert(rc == GIFF_OK);
    rc = giff_decoder_decode_memory(&dec, k_outbuf, writer.size);
    assert(rc == GIFF_OK);

    saw_change = 0u;
    for (i = 1u; i < 16u; ++i) {
        if (dec.canvas_indexed[i] != dec.canvas_indexed[0]) {
            saw_change = 1u;
            break;
        }
    }
    assert(saw_change);
}

static void test_frame_min_code_size_optimization(void)
{
    giff_encoder enc;
    giff_encoder_config ecfg;
    giff_result rc;
    giff_io io;
    mem_writer writer;
    giff_rgb8 palette[256];
    giff_info stream;
    giff_frame_info frame;
    giff_u8 pixels[8];
    giff_u16 i;
    giff_u32 pos;
    giff_u8 found;

    init_ecfg(&ecfg);
    rc = giff_encoder_init(&enc, &ecfg, k_workspace, (giff_u32)sizeof(k_workspace));
    assert(rc == GIFF_OK);

    for (i = 0u; i < 256u; ++i) {
        palette[i].r = (giff_u8)i;
        palette[i].g = (giff_u8)i;
        palette[i].b = (giff_u8)i;
    }
    rc = giff_encoder_set_global_palette(&enc, palette, 256u);
    assert(rc == GIFF_OK);

    writer.dst = k_outbuf;
    writer.cap = (giff_u32)sizeof(k_outbuf);
    writer.size = 0u;
    io.user = &writer;
    io.read = 0;
    io.write = mem_write;
    io.seek = 0;
    giff_encoder_attach_io(&enc, &io);

    memset(&stream, 0, sizeof(stream));
    stream.width = 8u;
    stream.height = 1u;
    stream.version_major = 8u;
    stream.version_minor = (giff_u8)'9';
    stream.background_index = 0u;
    stream.loop_count = 1u;
    rc = giff_encoder_begin(&enc, &stream);
    assert(rc == GIFF_OK);

    memset(&frame, 0, sizeof(frame));
    frame.width = 8u;
    frame.height = 1u;
    for (i = 0u; i < 8u; ++i) {
        pixels[i] = (giff_u8)(i & 1u);
    }

    rc = giff_encoder_write_indexed_frame(&enc, &frame, pixels, 8u);
    assert(rc == GIFF_OK);
    rc = giff_encoder_end(&enc);
    assert(rc == GIFF_OK);

    found = 0u;
    pos = 13u + 256u * 3u;
    assert(pos + 10u < writer.size);
    assert(k_outbuf[pos] == 0x2Cu);
    {
        giff_u8 packed;
        giff_u8 min_code_size;
        giff_u32 code_pos;
        packed = k_outbuf[pos + 9u];
        code_pos = pos + 10u;
        if ((packed & 0x80u) != 0u) {
            code_pos += (giff_u32)(1u << ((packed & 0x07u) + 1u)) * 3u;
        }
        assert(code_pos < writer.size);
        min_code_size = k_outbuf[code_pos];
        assert(min_code_size == 0x02u);
    }
    found = 1u;
    assert(found);
}


static void test_auto_local_sparse_global_remap(void)
{
    giff_encoder enc;
    giff_encoder_config ecfg;
    giff_result rc;
    giff_io io;
    mem_writer writer;
    giff_rgb8 palette[256];
    giff_info stream;
    giff_frame_info frame;
    giff_u8 pixels[32u * 32u];
    giff_u16 i;
    giff_u32 pos;

    init_ecfg(&ecfg);
    rc = giff_encoder_init(&enc, &ecfg, k_workspace, (giff_u32)sizeof(k_workspace));
    assert(rc == GIFF_OK);

    for (i = 0u; i < 256u; ++i) {
        palette[i].r = (giff_u8)i;
        palette[i].g = (giff_u8)i;
        palette[i].b = (giff_u8)i;
    }
    rc = giff_encoder_set_global_palette(&enc, palette, 256u);
    assert(rc == GIFF_OK);

    for (i = 0u; i < (giff_u16)(sizeof(pixels) / sizeof(pixels[0])); ++i) {
        pixels[i] = (giff_u8)((i & 1u) ? 128u : 0u);
    }

    writer.dst = k_outbuf;
    writer.cap = (giff_u32)sizeof(k_outbuf);
    writer.size = 0u;
    io.user = &writer;
    io.read = 0;
    io.write = mem_write;
    io.seek = 0;
    giff_encoder_attach_io(&enc, &io);

    memset(&stream, 0, sizeof(stream));
    stream.width = 32u;
    stream.height = 32u;
    stream.version_major = 8u;
    stream.version_minor = (giff_u8)'9';
    stream.background_index = 0u;
    stream.loop_count = 1u;
    rc = giff_encoder_begin(&enc, &stream);
    assert(rc == GIFF_OK);

    memset(&frame, 0, sizeof(frame));
    frame.width = 32u;
    frame.height = 32u;
    rc = giff_encoder_write_indexed_frame(&enc, &frame, pixels, 32u);
    assert(rc == GIFF_OK);
    assert(enc.frame_auto_local_palette == 1u);
    assert(enc.frame_sparse_local_remap == 1u);
    assert(enc.frame_encoded_palette_entries == 2u);
    rc = giff_encoder_end(&enc);
    assert(rc == GIFF_OK);

    pos = 13u + 256u * 3u;
    assert(writer.size > pos + 20u);
    assert(k_outbuf[pos] == 0x2Cu);
    assert((k_outbuf[pos + 9u] & 0x80u) != 0u);
    assert((k_outbuf[pos + 9u] & 0x20u) != 0u);
}

static void test_cost_model_prefers_keep_for_dense_indices(void)
{
    giff_encoder enc;
    giff_encoder_config ecfg;
    giff_result rc;
    giff_io io;
    mem_writer writer;
    giff_rgb8 palette[256];
    giff_info stream;
    giff_frame_info frame;
    giff_u8 pixels[16u * 16u];
    giff_u16 i;

    init_ecfg(&ecfg);
    ecfg.dither_mode = (giff_u8)GIFF_DITHER_NONE;
    rc = giff_encoder_init(&enc, &ecfg, k_workspace, (giff_u32)sizeof(k_workspace));
    assert(rc == GIFF_OK);

    for (i = 0u; i < 256u; ++i) {
        palette[i].r = (giff_u8)i;
        palette[i].g = (giff_u8)i;
        palette[i].b = (giff_u8)i;
    }
    rc = giff_encoder_set_global_palette(&enc, palette, 256u);
    assert(rc == GIFF_OK);

    for (i = 0u; i < (giff_u16)(sizeof(pixels) / sizeof(pixels[0])); ++i) {
        pixels[i] = (giff_u8)i;
    }

    writer.dst = k_outbuf;
    writer.cap = (giff_u32)sizeof(k_outbuf);
    writer.size = 0u;
    io.user = &writer;
    io.read = 0;
    io.write = mem_write;
    io.seek = 0;
    giff_encoder_attach_io(&enc, &io);

    memset(&stream, 0, sizeof(stream));
    stream.width = 16u;
    stream.height = 16u;
    stream.version_major = 8u;
    stream.version_minor = (giff_u8)'9';
    stream.background_index = 0u;
    stream.loop_count = 1u;
    rc = giff_encoder_begin(&enc, &stream);
    assert(rc == GIFF_OK);

    memset(&frame, 0, sizeof(frame));
    frame.width = 16u;
    frame.height = 16u;
    rc = giff_encoder_write_indexed_frame(&enc, &frame, pixels, 16u);
    assert(rc == GIFF_OK);
    assert(enc.frame_cost_mode == (giff_u8)GIFF_PALETTE_DECISION_KEEP);
    assert(enc.frame_auto_local_palette == 0u);
    assert(enc.frame_cost_keep <= enc.frame_cost_local);
    assert(enc.frame_cost_keep <= enc.frame_cost_temporal);

    rc = giff_encoder_end(&enc);
    assert(rc == GIFF_OK);
}

static void test_multi_frame_temporal_window_cost_model(void)
{
    giff_encoder enc;
    giff_encoder_config ecfg;
    giff_result rc;
    giff_io io;
    mem_writer writer;
    giff_rgb8 palette[256];
    giff_info stream;
    giff_frame_info frame;
    giff_u8 pixels_a[16u * 16u];
    giff_u8 pixels_b[16u * 16u];
    giff_u8 pixels_c[16u * 16u];
    giff_u16 i;

    init_ecfg(&ecfg);
    ecfg.dither_mode = (giff_u8)GIFF_DITHER_NONE;
    ecfg.temporal_window_frames = 3u;
    ecfg.lzw_band_rows = 4u;
    rc = giff_encoder_init(&enc, &ecfg, k_workspace, (giff_u32)sizeof(k_workspace));
    assert(rc == GIFF_OK);

    for (i = 0u; i < 256u; ++i) {
        palette[i].r = (giff_u8)i;
        palette[i].g = (giff_u8)i;
        palette[i].b = (giff_u8)i;
    }
    rc = giff_encoder_set_global_palette(&enc, palette, 256u);
    assert(rc == GIFF_OK);

    for (i = 0u; i < (giff_u16)(sizeof(pixels_a) / sizeof(pixels_a[0])); ++i) {
        pixels_a[i] = (giff_u8)((i & 1u) ? 64u : 192u);
        pixels_b[i] = (giff_u8)((i & 1u) ? 192u : 64u);
        pixels_c[i] = (giff_u8)((i & 3u) == 0u ? 64u : ((i & 3u) == 1u ? 192u : ((i & 3u) == 2u ? 128u : 0u)));
    }

    writer.dst = k_outbuf;
    writer.cap = (giff_u32)sizeof(k_outbuf);
    writer.size = 0u;
    io.user = &writer;
    io.read = 0;
    io.write = mem_write;
    io.seek = 0;
    giff_encoder_attach_io(&enc, &io);

    memset(&stream, 0, sizeof(stream));
    stream.width = 16u;
    stream.height = 16u;
    stream.version_major = 8u;
    stream.version_minor = (giff_u8)'9';
    stream.background_index = 0u;
    stream.loop_count = 0u;
    rc = giff_encoder_begin(&enc, &stream);
    assert(rc == GIFF_OK);

    memset(&frame, 0, sizeof(frame));
    frame.width = 16u;
    frame.height = 16u;

    rc = giff_encoder_write_indexed_frame(&enc, &frame, pixels_a, 16u);
    assert(rc == GIFF_OK);
    assert(enc.frame_window_used == 1u);
    assert(enc.frame_cost_mode != (giff_u8)GIFF_PALETTE_DECISION_KEEP);

    rc = giff_encoder_write_indexed_frame(&enc, &frame, pixels_b, 16u);
    assert(rc == GIFF_OK);
    assert(enc.frame_window_used >= 2u);
    assert(enc.frame_cost_mode == (giff_u8)GIFF_PALETTE_DECISION_TEMPORAL_REMAP ||
           enc.frame_cost_mode == (giff_u8)GIFF_PALETTE_DECISION_SEGMENT_SHARE);
    assert(enc.frame_temporal_palette == 1u || enc.frame_clustered_palette == 1u || enc.frame_segment_shared == 1u);
    assert(enc.frame_cost_chosen <= enc.frame_cost_keep);

    rc = giff_encoder_write_indexed_frame(&enc, &frame, pixels_c, 16u);
    assert(rc == GIFF_OK);
    assert(enc.frame_window_used == 3u);
    assert(enc.frame_cost_mode != (giff_u8)GIFF_PALETTE_DECISION_KEEP);
    assert(enc.temporal_window_similarity_q8 != 0u);
    assert(enc.frame_cost_chosen <= enc.frame_cost_keep);

    rc = giff_encoder_end(&enc);
    assert(rc == GIFF_OK);
}

static void test_segment_palette_sharing(void)
{
    giff_encoder enc;
    giff_encoder_config ecfg;
    giff_result rc;
    giff_io io;
    mem_writer writer;
    giff_rgb8 palette[256];
    giff_info stream;
    giff_frame_info frame;
    giff_u8 pixels_a[16u * 16u];
    giff_u8 pixels_b[16u * 16u];
    giff_u8 pixels_c[16u * 16u];
    giff_u16 i;

    init_ecfg(&ecfg);
    ecfg.dither_mode = (giff_u8)GIFF_DITHER_NONE;
    ecfg.temporal_window_frames = 3u;
    ecfg.lzw_band_rows = 4u;
    rc = giff_encoder_init(&enc, &ecfg, k_workspace, (giff_u32)sizeof(k_workspace));
    assert(rc == GIFF_OK);

    for (i = 0u; i < 256u; ++i) {
        palette[i].r = (giff_u8)i;
        palette[i].g = (giff_u8)i;
        palette[i].b = (giff_u8)i;
    }
    rc = giff_encoder_set_global_palette(&enc, palette, 256u);
    assert(rc == GIFF_OK);

    for (i = 0u; i < (giff_u16)(sizeof(pixels_a) / sizeof(pixels_a[0])); ++i) {
        pixels_a[i] = (giff_u8)((i & 1u) ? 64u : 192u);
        pixels_b[i] = (giff_u8)((i & 1u) ? 192u : 64u);
        pixels_c[i] = (giff_u8)((i & 3u) == 0u ? 64u : ((i & 3u) == 1u ? 192u : ((i & 3u) == 2u ? 128u : 0u)));
    }

    writer.dst = k_outbuf;
    writer.cap = (giff_u32)sizeof(k_outbuf);
    writer.size = 0u;
    io.user = &writer;
    io.read = 0;
    io.write = mem_write;
    io.seek = 0;
    giff_encoder_attach_io(&enc, &io);

    memset(&stream, 0, sizeof(stream));
    stream.width = 16u;
    stream.height = 16u;
    stream.version_major = 8u;
    stream.version_minor = (giff_u8)'9';
    stream.background_index = 0u;
    stream.loop_count = 0u;
    rc = giff_encoder_begin(&enc, &stream);
    assert(rc == GIFF_OK);

    memset(&frame, 0, sizeof(frame));
    frame.width = 16u;
    frame.height = 16u;

    rc = giff_encoder_write_indexed_frame(&enc, &frame, pixels_a, 16u);
    assert(rc == GIFF_OK);
    assert(enc.segment_frame_count == 1u);
    assert(enc.frame_segment_shared == 0u);

    rc = giff_encoder_write_indexed_frame(&enc, &frame, pixels_b, 16u);
    assert(rc == GIFF_OK);
    assert(enc.frame_cost_mode == (giff_u8)GIFF_PALETTE_DECISION_SEGMENT_SHARE);
    assert(enc.frame_segment_shared == 1u);
    assert(enc.segment_frame_count >= 2u);
    assert(enc.frame_cost_segment <= enc.frame_cost_keep);

    rc = giff_encoder_write_indexed_frame(&enc, &frame, pixels_c, 16u);
    assert(rc == GIFF_OK);
    assert(enc.frame_segment_break == 1u || enc.segment_frame_count == 1u);

    rc = giff_encoder_end(&enc);
    assert(rc == GIFF_OK);
}

static void test_lzw_aggressive_clears(void)
{
    giff_encoder enc;
    giff_encoder_config ecfg;
    giff_result rc;
    giff_io io;
    mem_writer writer;
    giff_rgb8 palette[256];
    giff_info stream;
    giff_frame_info frame;
    giff_u8 pixels[32u * 32u];
    giff_u16 i;

    init_ecfg(&ecfg);
    ecfg.dither_mode = (giff_u8)GIFF_DITHER_NONE;
    rc = giff_encoder_init(&enc, &ecfg, k_workspace, (giff_u32)sizeof(k_workspace));
    assert(rc == GIFF_OK);

    for (i = 0u; i < 256u; ++i) {
        palette[i].r = (giff_u8)i;
        palette[i].g = (giff_u8)(255u - i);
        palette[i].b = (giff_u8)(i ^ 0x55u);
    }
    rc = giff_encoder_set_global_palette(&enc, palette, 256u);
    assert(rc == GIFF_OK);

    for (i = 0u; i < (giff_u16)(sizeof(pixels) / sizeof(pixels[0])); ++i) {
        pixels[i] = (giff_u8)((i * 73u + (i >> 2u) + 19u) & 0xFFu);
    }

    writer.dst = k_outbuf;
    writer.cap = (giff_u32)sizeof(k_outbuf);
    writer.size = 0u;
    io.user = &writer;
    io.read = 0;
    io.write = mem_write;
    io.seek = 0;
    giff_encoder_attach_io(&enc, &io);

    memset(&stream, 0, sizeof(stream));
    stream.width = 32u;
    stream.height = 32u;
    stream.version_major = 8u;
    stream.version_minor = (giff_u8)'9';
    stream.background_index = 0u;
    stream.loop_count = 1u;
    rc = giff_encoder_begin(&enc, &stream);
    assert(rc == GIFF_OK);

    memset(&frame, 0, sizeof(frame));
    frame.width = 32u;
    frame.height = 32u;
    rc = giff_encoder_write_indexed_frame(&enc, &frame, pixels, 32u);
    assert(rc == GIFF_OK);
    assert(enc.lzw_strategy == (giff_u8)GIFF_LZW_STRATEGY_AGGRESSIVE);
    assert(enc.lzw_clear_count > 1u);
    rc = giff_encoder_end(&enc);
    assert(rc == GIFF_OK);
}


static void test_cross_frame_palette_clustering(void)
{
    giff_encoder enc;
    giff_encoder_config ecfg;
    giff_result rc;
    giff_io io;
    mem_writer writer;
    giff_rgb8 palette[256];
    giff_info stream;
    giff_frame_info frame;
    giff_u8 pixels_a[16u * 16u];
    giff_u8 pixels_b[16u * 16u];
    giff_u16 i;

    init_ecfg(&ecfg);
    ecfg.lzw_band_rows = 4u;
    rc = giff_encoder_init(&enc, &ecfg, k_workspace, (giff_u32)sizeof(k_workspace));
    assert(rc == GIFF_OK);

    for (i = 0u; i < 256u; ++i) {
        palette[i].r = (giff_u8)i;
        palette[i].g = (giff_u8)(255u - i);
        palette[i].b = (giff_u8)(i ^ 0x33u);
    }
    rc = giff_encoder_set_global_palette(&enc, palette, 256u);
    assert(rc == GIFF_OK);

    for (i = 0u; i < (giff_u16)(sizeof(pixels_a) / sizeof(pixels_a[0])); ++i) {
        pixels_a[i] = (giff_u8)((i & 3u) == 0u ? 0u : ((i & 3u) == 1u ? 64u : ((i & 3u) == 2u ? 128u : 192u)));
        pixels_b[i] = (giff_u8)((i & 3u) == 0u ? 64u : ((i & 3u) == 1u ? 0u : ((i & 3u) == 2u ? 192u : 128u)));
    }

    writer.dst = k_outbuf;
    writer.cap = (giff_u32)sizeof(k_outbuf);
    writer.size = 0u;
    io.user = &writer;
    io.read = 0;
    io.write = mem_write;
    io.seek = 0;
    giff_encoder_attach_io(&enc, &io);

    memset(&stream, 0, sizeof(stream));
    stream.width = 16u;
    stream.height = 16u;
    stream.version_major = 8u;
    stream.version_minor = (giff_u8)'9';
    stream.background_index = 0u;
    stream.loop_count = 0u;
    rc = giff_encoder_begin(&enc, &stream);
    assert(rc == GIFF_OK);

    memset(&frame, 0, sizeof(frame));
    frame.width = 16u;
    frame.height = 16u;

    rc = giff_encoder_write_indexed_frame(&enc, &frame, pixels_a, 16u);
    assert(rc == GIFF_OK);
    assert(enc.temporal_valid == 1u);
    assert(enc.prev_palette_valid == 1u);

    rc = giff_encoder_write_indexed_frame(&enc, &frame, pixels_b, 16u);
    assert(rc == GIFF_OK);
    assert(enc.frame_auto_local_palette == 1u);
    assert(enc.frame_temporal_palette == 1u || enc.frame_clustered_palette == 1u);
    assert(enc.temporal_similarity_q8 != 0u);

    rc = giff_encoder_end(&enc);
    assert(rc == GIFF_OK);
    assert(writer.size != 0u);
}

static void test_band_lzw_tuning(void)
{
    giff_encoder enc;
    giff_encoder_config ecfg;
    giff_result rc;
    giff_io io;
    mem_writer writer;
    giff_rgb8 palette[256];
    giff_info stream;
    giff_frame_info frame;
    giff_u8 pixels[32u * 16u];
    giff_u16 x;
    giff_u16 y;
    giff_u16 i;

    init_ecfg(&ecfg);
    ecfg.dither_mode = (giff_u8)GIFF_DITHER_NONE;
    ecfg.lzw_band_rows = 8u;
    rc = giff_encoder_init(&enc, &ecfg, k_workspace, (giff_u32)sizeof(k_workspace));
    assert(rc == GIFF_OK);

    for (i = 0u; i < 256u; ++i) {
        palette[i].r = (giff_u8)i;
        palette[i].g = (giff_u8)i;
        palette[i].b = (giff_u8)i;
    }
    rc = giff_encoder_set_global_palette(&enc, palette, 256u);
    assert(rc == GIFF_OK);

    for (y = 0u; y < 16u; ++y) {
        for (x = 0u; x < 32u; ++x) {
            if (y < 8u) {
                pixels[(giff_u32)y * 32u + x] = (giff_u8)((x & 1u) ? 1u : 1u);
            } else {
                pixels[(giff_u32)y * 32u + x] = (giff_u8)(((giff_u32)y * 37u + (giff_u32)x * 53u + (giff_u32)(x ^ y)) & 0xFFu);
            }
        }
    }

    writer.dst = k_outbuf;
    writer.cap = (giff_u32)sizeof(k_outbuf);
    writer.size = 0u;
    io.user = &writer;
    io.read = 0;
    io.write = mem_write;
    io.seek = 0;
    giff_encoder_attach_io(&enc, &io);

    memset(&stream, 0, sizeof(stream));
    stream.width = 32u;
    stream.height = 16u;
    stream.version_major = 8u;
    stream.version_minor = (giff_u8)'9';
    stream.background_index = 0u;
    stream.loop_count = 1u;
    rc = giff_encoder_begin(&enc, &stream);
    assert(rc == GIFF_OK);

    memset(&frame, 0, sizeof(frame));
    frame.width = 32u;
    frame.height = 16u;
    rc = giff_encoder_write_indexed_frame(&enc, &frame, pixels, 32u);
    assert(rc == GIFF_OK);
    assert(enc.band_lzw_tuned == 1u);
    assert(enc.frame_band_rows == 8u);
    assert(enc.frame_band_count == 2u);
    assert(enc.lzw_band_switches >= 1u);

    rc = giff_encoder_end(&enc);
    assert(rc == GIFF_OK);
}

static void test_corpus_fixtures(void)
{
    static const char* k_paths[] = {
        "tests/corpus/comment.gif",
        "tests/corpus/bad-signature.gif",
        "tests/corpus/truncated-local-table.gif",
        "tests/corpus/bad-min-code-size.gif",
        "tests/corpus/missing-image-terminator.gif",
        "tests/corpus/truncated-lzw-subblock.gif",
        "tests/corpus/truncated-application.gif",
        "tests/corpus/bad-local-color-table-short.gif",
        "tests/corpus/global-loader.gif",
        "tests/corpus/double-gce.gif",
        "tests/corpus/netscape-comment-weird.gif",
        "tests/corpus/zero-delay-loop.gif",
        "tests/corpus/empty-comment.gif",
        "tests/corpus/interlaced-local.gif",
        "tests/corpus/app-comment-empty.gif",
        "tests/corpus/gce-comment-scope.gif",
        "tests/corpus/double-app-loop.gif"
    };
    giff_u8 buf[512];
    giff_u32 size;
    giff_decoder dec;
    giff_decoder_config cfg;
    giff_result rc;
    giff_u16 i;

    init_cfg(&cfg, (giff_u8)GIFF_OUTPUT_RGBA8888, 1u);

    for (i = 0u; i < (giff_u16)(sizeof(k_paths) / sizeof(k_paths[0])); ++i) {
        size = read_file_bytes(k_paths[i], buf, (giff_u32)sizeof(buf));
        assert(size != 0u);

        rc = giff_decoder_init(&dec, &cfg, k_workspace, (giff_u32)sizeof(k_workspace));
        assert(rc == GIFF_OK);
        rc = giff_decoder_decode_memory(&dec, buf, size);
        assert(rc != GIFF_E_INTERNAL);

        if (strcmp(k_paths[i], "tests/corpus/comment.gif") == 0) {
            assert(rc == GIFF_OK);
        } else if (strcmp(k_paths[i], "tests/corpus/bad-signature.gif") == 0) {
            assert(rc == GIFF_E_INVALID_SIGNATURE);
        } else if (strcmp(k_paths[i], "tests/corpus/bad-min-code-size.gif") == 0) {
            assert(rc == GIFF_E_BAD_BLOCK);
        } else if (strcmp(k_paths[i], "tests/corpus/global-loader.gif") == 0) {
            assert(rc == GIFF_OK);
            assert(dec.frames_seen == 0u);
        } else if (strcmp(k_paths[i], "tests/corpus/double-gce.gif") == 0) {
            assert(rc == GIFF_OK);
            assert(dec.frames_seen == 1u);
        } else if (strcmp(k_paths[i], "tests/corpus/netscape-comment-weird.gif") == 0) {
            assert(rc == GIFF_OK);
            assert(dec.info.loop_count == 0u);
        } else if (strcmp(k_paths[i], "tests/corpus/zero-delay-loop.gif") == 0) {
            assert(rc == GIFF_OK);
            assert(dec.frames_seen == 2u);
            assert(dec.info.loop_count == 0u);
        } else if (strcmp(k_paths[i], "tests/corpus/empty-comment.gif") == 0) {
            assert(rc == GIFF_OK);
            assert(dec.frames_seen == 1u);
        } else if (strcmp(k_paths[i], "tests/corpus/interlaced-local.gif") == 0) {
            assert(rc == GIFF_OK);
            assert(dec.frames_seen == 1u);
        } else if (strcmp(k_paths[i], "tests/corpus/app-comment-empty.gif") == 0) {
            assert(rc == GIFF_OK);
            assert(dec.frames_seen == 1u);
            assert(dec.info.loop_count == 0u);
        } else if (strcmp(k_paths[i], "tests/corpus/gce-comment-scope.gif") == 0) {
            assert(rc == GIFF_OK);
            assert(dec.frames_seen == 1u);
        } else if (strcmp(k_paths[i], "tests/corpus/double-app-loop.gif") == 0) {
            assert(rc == GIFF_OK);
            assert(dec.frames_seen == 1u);
            assert(dec.info.loop_count == 5u);
        } else {
            assert(rc == GIFF_E_TRUNCATED || rc == GIFF_E_BAD_BLOCK || rc == GIFF_E_BAD_COLOR_TABLE || rc == GIFF_E_OUTPUT_OVERFLOW);
        }
    }
}

int main(void)
{
    test_one_shot_decode();
    test_stream_comment_decode();
    test_feed_incremental_decode();
    test_encode_roundtrip_indexed();
    test_encode_rgba_global_quantized();
    test_encode_rgba_local_palette();
    test_octree_rgba_roundtrip_direct();
    test_frame_min_code_size_optimization();
    test_auto_local_sparse_global_remap();
    test_cost_model_prefers_keep_for_dense_indices();
    test_multi_frame_temporal_window_cost_model();
    test_segment_palette_sharing();
    test_lzw_aggressive_clears();
    test_cross_frame_palette_clustering();
    test_band_lzw_tuning();
    test_quantizer_median_cut();
    test_quantizer_octree();
    test_dither_floyd_steinberg();
    test_interlaced_encode_roundtrip();
    test_feed_compactor_large_stream();
    test_malformed_inputs();
    test_corpus_fixtures();
    test_fuzz_like_mutations();
    return 0;
}
