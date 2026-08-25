#include "giff_internal.h"

typedef struct giff_subreader {
    const giff_u8* p;
    const giff_u8* end;
    giff_u32 remaining;
    giff_u8 finished;
} giff_subreader;

typedef struct giff_lzw_reader {
    giff_subreader sub;
    giff_u32 bitbuf;
    giff_u8 bits_in_buf;
} giff_lzw_reader;

typedef struct giff_io_subreader {
    giff_decoder* dec;
    giff_u32 remaining;
    giff_u8 finished;
} giff_io_subreader;

typedef struct giff_io_lzw_reader {
    giff_io_subreader sub;
    giff_u32 bitbuf;
    giff_u8 bits_in_buf;
} giff_io_lzw_reader;

static giff_u16 giff_u16le(const giff_u8* p)
{
    return (giff_u16)((giff_u16)p[0] | ((giff_u16)p[1] << 8));
}

static giff_u32 giff_decoder_feed_capacity(const giff_decoder_config* cfg)
{
    giff_u32 pixels;
    giff_u32 bytes;

    if (cfg == 0) {
        return (giff_u32)GIFF_FEED_BUFFER_MIN_BYTES;
    }

    pixels = (giff_u32)cfg->max_width * (giff_u32)cfg->max_height;
    bytes = pixels;
    if (bytes > 0x7FFFFFFFu) {
        bytes = 0xFFFFFFFFu;
    } else {
        bytes *= 2u;
    }
    if (bytes < (giff_u32)GIFF_FEED_BUFFER_MIN_BYTES) {
        bytes = (giff_u32)GIFF_FEED_BUFFER_MIN_BYTES;
    }
    return bytes;
}

static giff_result giff_need_more_or_truncated(const giff_decoder* dec)
{
    if (dec != 0 && dec->source_mode == 3u && !dec->stream_eof) {
        return GIFF_E_NEED_MORE_INPUT;
    }
    return GIFF_E_TRUNCATED;
}

static giff_result giff_memory_subblocks_complete(
    const giff_u8* p,
    const giff_u8* end,
    const giff_u8** out_after
)
{
    giff_u32 block_size;

    while (1) {
        if (p >= end) {
            return GIFF_E_NEED_MORE_INPUT;
        }
        block_size = (giff_u32)(*p++);
        if (block_size == 0u) {
            if (out_after != 0) {
                *out_after = p;
            }
            return GIFF_OK;
        }
        if ((giff_u32)(end - p) < block_size) {
            return GIFF_E_NEED_MORE_INPUT;
        }
        p += block_size;
    }
}

static giff_result giff_memory_extension_complete(
    const giff_u8* p,
    const giff_u8* end,
    giff_u8 extension_label
)
{
    giff_u32 block_size;

    if (p >= end) {
        return GIFF_E_NEED_MORE_INPUT;
    }

    if (extension_label == 0xF9u) {
        block_size = (giff_u32)(*p++);
        if ((giff_u32)(end - p) < (block_size + 1u)) {
            return GIFF_E_NEED_MORE_INPUT;
        }
        return GIFF_OK;
    }

    if (extension_label == 0xFFu || extension_label == 0x01u) {
        block_size = (giff_u32)(*p++);
        if ((giff_u32)(end - p) < block_size) {
            return GIFF_E_NEED_MORE_INPUT;
        }
        p += block_size;
        return giff_memory_subblocks_complete(p, end, 0);
    }

    return giff_memory_subblocks_complete(p, end, 0);
}

static giff_result giff_memory_image_complete(
    const giff_u8* p,
    const giff_u8* end
)
{
    giff_u8 packed;
    giff_u8 local_bits;
    giff_u16 entries;
    giff_u32 palette_bytes;

    if ((giff_u32)(end - p) < 9u) {
        return GIFF_E_NEED_MORE_INPUT;
    }

    packed = p[8];
    p += 9u;

    if (packed & 0x80u) {
        local_bits = (giff_u8)((packed & 0x07u) + 1u);
        entries = (giff_u16)(1u << local_bits);
        palette_bytes = (giff_u32)entries * 3u;
        if ((giff_u32)(end - p) < palette_bytes) {
            return GIFF_E_NEED_MORE_INPUT;
        }
        p += palette_bytes;
    }

    if (p >= end) {
        return GIFF_E_NEED_MORE_INPUT;
    }
    p += 1u;
    return giff_memory_subblocks_complete(p, end, 0);
}

static void giff_event_zero(giff_event* ev)
{
    if (ev != 0) {
        giff_mem_zero(ev, (giff_u32)sizeof(*ev));
    }
}

static void giff_event_fill(giff_decoder* dec, giff_event* ev, giff_event_type type)
{
    if (ev == 0 || dec == 0) {
        return;
    }

    giff_event_zero(ev);
    ev->type = type;
    ev->code = GIFF_OK;
    ev->info = dec->info;
    ev->frame = dec->frame;
}

static void giff_decoder_clear_pending_gce(giff_decoder* dec)
{
    if (dec == 0) {
        return;
    }

    dec->pending_delay_cs = 0u;
    dec->pending_transparency_index = 0u;
    dec->pending_has_transparency = 0u;
    dec->pending_disposal_method = 0u;
}

static void giff_decoder_sync_offset(giff_decoder* dec)
{
    if (dec == 0 || dec->stream_data == 0 || dec->parse_ptr == 0) {
        return;
    }

    dec->input_offset = dec->stream_base_offset + (giff_u32)(dec->parse_ptr - dec->stream_data);
}

static void giff_decoder_compact_feed_buffer(giff_decoder* dec)
{
    giff_u32 parse_offset;
    giff_u32 unread;

    if (dec == 0 || dec->stream_buf == 0 || dec->source_mode != 3u || !dec->started || dec->parse_ptr == 0) {
        return;
    }

    if (dec->parse_ptr < dec->stream_buf) {
        return;
    }

    parse_offset = (giff_u32)(dec->parse_ptr - dec->stream_buf);
    if (parse_offset == 0u || parse_offset > dec->stream_buf_fill) {
        return;
    }

    unread = dec->stream_buf_fill - parse_offset;
    if (unread != 0u) {
        giff_mem_move(dec->stream_buf, dec->stream_buf + parse_offset, unread);
    }

    dec->stream_base_offset += parse_offset;
    dec->stream_data = dec->stream_buf;
    dec->parse_ptr = dec->stream_buf;
    dec->parse_end = dec->stream_buf + unread;
    dec->stream_size = unread;
    dec->stream_buf_fill = unread;
    dec->input_offset = dec->stream_base_offset;
}

static void giff_set_default_stride(giff_decoder* dec)
{
    if (dec->cfg.output_mode == (giff_u8)GIFF_OUTPUT_RGBA8888) {
        dec->canvas_stride = dec->cfg.user_canvas_stride;
        if (dec->canvas_stride == 0u) {
            dec->canvas_stride = (giff_u32)dec->info.width * 4u;
        }

        dec->previous_stride = dec->cfg.user_previous_stride;
        if (dec->previous_stride == 0u) {
            dec->previous_stride = dec->canvas_stride;
        }
    } else {
        dec->canvas_stride = (giff_u32)dec->info.width;
        dec->previous_stride = dec->canvas_stride;
    }
}

static giff_result giff_decoder_layout(giff_decoder* dec)
{
    giff_ws_cursor ws;
    giff_result rc;
    giff_u32 canvas_bytes;

    ws.base = (giff_u8*)dec->workspace;
    ws.size = dec->workspace_size;
    ws.used = 0u;

    rc = giff_ws_take(&ws, (giff_u32)(GIFF_LZW_TABLE_SIZE * sizeof(giff_u16)), sizeof(giff_u16), (void**)&dec->lzw_prefix);
    if (rc != GIFF_OK) {
        return rc;
    }

    rc = giff_ws_take(&ws, (giff_u32)GIFF_LZW_TABLE_SIZE, 1u, (void**)&dec->lzw_suffix);
    if (rc != GIFF_OK) {
        return rc;
    }

    rc = giff_ws_take(&ws, (giff_u32)GIFF_LZW_TABLE_SIZE, 1u, (void**)&dec->lzw_stack);
    if (rc != GIFF_OK) {
        return rc;
    }

    rc = giff_ws_take(&ws, (giff_u32)(GIFF_DATA_SUBBLOCK_MAX + 1u), 1u, (void**)&dec->subblock);
    if (rc != GIFF_OK) {
        return rc;
    }

    rc = giff_ws_take(&ws, (giff_u32)dec->cfg.max_width, 1u, (void**)&dec->row_indexed);
    if (rc != GIFF_OK) {
        return rc;
    }

    dec->stream_buf_cap = giff_decoder_feed_capacity(&dec->cfg);
    rc = giff_ws_take(&ws, dec->stream_buf_cap, 1u, (void**)&dec->stream_buf);
    if (rc != GIFF_OK) {
        return rc;
    }

    dec->canvas_rgba = dec->cfg.user_canvas_rgba;
    dec->previous_rgba = dec->cfg.user_previous_rgba;
    dec->canvas_indexed = 0;
    dec->previous_indexed = 0;
    dec->canvas_bytes = 0u;
    dec->previous_bytes = 0u;

    if (dec->cfg.output_mode == (giff_u8)GIFF_OUTPUT_RGBA8888) {
        canvas_bytes = (giff_u32)dec->cfg.max_width;
        canvas_bytes *= (giff_u32)dec->cfg.max_height;
        canvas_bytes *= 4u;
        dec->canvas_bytes = canvas_bytes;

        if (dec->canvas_rgba == 0) {
            rc = giff_ws_take(&ws, canvas_bytes, 4u, (void**)&dec->canvas_rgba);
            if (rc != GIFF_OK) {
                return rc;
            }
        }

        if (dec->cfg.restore_previous_mode != (giff_u8)GIFF_RESTORE_PREVIOUS_DISABLE) {
            dec->previous_bytes = canvas_bytes;
            if (dec->previous_rgba == 0) {
                rc = giff_ws_take(&ws, canvas_bytes, 4u, (void**)&dec->previous_rgba);
                if (rc != GIFF_OK) {
                    return rc;
                }
            }
        }
    } else {
        canvas_bytes = (giff_u32)dec->cfg.max_width;
        canvas_bytes *= (giff_u32)dec->cfg.max_height;
        dec->canvas_bytes = canvas_bytes;

        rc = giff_ws_take(&ws, canvas_bytes, 1u, (void**)&dec->canvas_indexed);
        if (rc != GIFF_OK) {
            return rc;
        }

        if (dec->cfg.restore_previous_mode != (giff_u8)GIFF_RESTORE_PREVIOUS_DISABLE) {
            dec->previous_bytes = canvas_bytes;
            rc = giff_ws_take(&ws, canvas_bytes, 1u, (void**)&dec->previous_indexed);
            if (rc != GIFF_OK) {
                return rc;
            }
        }
    }

    return GIFF_OK;
}

giff_u32 giff_decoder_workspace_size(const giff_decoder_config* cfg)
{
    giff_u32 total;
    giff_u32 canvas_bytes;

    if (cfg == 0) {
        return 0u;
    }

    total = 0u;
    total = giff_align_up(total, (giff_u32)sizeof(giff_u16));
    total += (giff_u32)(GIFF_LZW_TABLE_SIZE * sizeof(giff_u16));
    total += (giff_u32)GIFF_LZW_TABLE_SIZE;
    total += (giff_u32)GIFF_LZW_TABLE_SIZE;
    total += (giff_u32)(GIFF_DATA_SUBBLOCK_MAX + 1u);
    total += (giff_u32)cfg->max_width;
    total += giff_decoder_feed_capacity(cfg);

    canvas_bytes = (giff_u32)cfg->max_width;
    canvas_bytes *= (giff_u32)cfg->max_height;

    if (cfg->output_mode == (giff_u8)GIFF_OUTPUT_RGBA8888) {
        canvas_bytes *= 4u;

        if (cfg->user_canvas_rgba == 0) {
            total = giff_align_up(total, 4u);
            total += canvas_bytes;
        }

        if (cfg->restore_previous_mode != (giff_u8)GIFF_RESTORE_PREVIOUS_DISABLE &&
            cfg->user_previous_rgba == 0) {
            total = giff_align_up(total, 4u);
            total += canvas_bytes;
        }
    } else {
        total += canvas_bytes;
        if (cfg->restore_previous_mode != (giff_u8)GIFF_RESTORE_PREVIOUS_DISABLE) {
            total += canvas_bytes;
        }
    }

    return total;
}

giff_result giff_decoder_init(
    giff_decoder* dec,
    const giff_decoder_config* cfg,
    void* workspace,
    giff_u32 workspace_size
)
{
    giff_result rc;

    if (dec == 0 || cfg == 0 || workspace == 0) {
        return GIFF_E_INVALID_ARGUMENT;
    }

    if (cfg->max_width == 0u || cfg->max_height == 0u) {
        return GIFF_E_BAD_DIMENSIONS;
    }

    giff_mem_zero(dec, (giff_u32)sizeof(*dec));
    dec->cfg = *cfg;
    dec->workspace = workspace;
    dec->workspace_size = workspace_size;

    rc = giff_decoder_layout(dec);
    if (rc != GIFF_OK) {
        dec->last_result = (giff_u8)(-rc);
        return rc;
    }

    giff_lzw_dec_reset(dec);
    dec->last_result = (giff_u8)GIFF_OK;
    return GIFF_OK;
}

void giff_decoder_reset(giff_decoder* dec)
{
    if (dec == 0) {
        return;
    }

    dec->stream_data = 0;
    dec->parse_ptr = 0;
    dec->parse_end = 0;
    dec->stream_size = 0u;
    dec->stream_base_offset = 0u;
    dec->input_offset = 0u;
    dec->frames_seen = 0u;
    dec->events_emitted = 0u;
    dec->header_filled = 0u;
    dec->capture_size = 0u;
    dec->started = 0u;
    dec->finished = 0u;
    dec->info_emitted = 0u;
    dec->event_phase = 0u;
    dec->have_previous_frame = 0u;
    dec->trailer_emitted = 0u;
    dec->emit_row_cursor = 0u;
    dec->emit_row_count = 0u;
    dec->canvas_stride = 0u;
    dec->previous_stride = 0u;
    dec->stream_buf_pos = 0u;
    dec->stream_buf_fill = 0u;
    dec->stream_eof = 0u;
    dec->source_mode = 0u;
    giff_mem_zero(&dec->info, (giff_u32)sizeof(dec->info));
    giff_mem_zero(&dec->frame, (giff_u32)sizeof(dec->frame));
    giff_mem_zero(&dec->previous_frame, (giff_u32)sizeof(dec->previous_frame));
    giff_mem_zero(dec->header_bytes, (giff_u32)sizeof(dec->header_bytes));
    giff_mem_zero(dec->global_palette, (giff_u32)sizeof(dec->global_palette));
    giff_mem_zero(dec->local_palette, (giff_u32)sizeof(dec->local_palette));
    giff_decoder_clear_pending_gce(dec);

    if (dec->subblock != 0) {
        giff_mem_zero(dec->subblock, (giff_u32)(GIFF_DATA_SUBBLOCK_MAX + 1u));
    }

    if (dec->canvas_rgba != 0 && dec->canvas_bytes != 0u) {
        giff_mem_zero(dec->canvas_rgba, dec->canvas_bytes);
    }
    if (dec->previous_rgba != 0 && dec->previous_bytes != 0u) {
        giff_mem_zero(dec->previous_rgba, dec->previous_bytes);
    }
    if (dec->canvas_indexed != 0 && dec->canvas_bytes != 0u) {
        giff_mem_zero(dec->canvas_indexed, dec->canvas_bytes);
    }
    if (dec->previous_indexed != 0 && dec->previous_bytes != 0u) {
        giff_mem_zero(dec->previous_indexed, dec->previous_bytes);
    }

    giff_lzw_dec_reset(dec);
    dec->last_result = (giff_u8)GIFF_OK;
}

void giff_decoder_attach_io(giff_decoder* dec, const giff_io* io)
{
    if (dec == 0) {
        return;
    }

    if (io == 0) {
        giff_mem_zero(&dec->io, (giff_u32)sizeof(dec->io));
        return;
    }

    dec->io = *io;
}

giff_result giff_decoder_inspect_header(
    const giff_u8* bytes,
    giff_u32 size,
    giff_info* out_info
)
{
    giff_u8 packed;
    giff_u8 gct_bits;
    giff_info info;

    if (bytes == 0 || out_info == 0) {
        return GIFF_E_INVALID_ARGUMENT;
    }

    if (size < 13u) {
        return GIFF_E_NEED_MORE_INPUT;
    }

    if (!(bytes[0] == 'G' && bytes[1] == 'I' && bytes[2] == 'F')) {
        return GIFF_E_INVALID_SIGNATURE;
    }

    if (!(bytes[3] == '8' && bytes[4] == '7' && bytes[5] == 'a') &&
        !(bytes[3] == '8' && bytes[4] == '9' && bytes[5] == 'a')) {
        return GIFF_E_UNSUPPORTED_VERSION;
    }

    giff_mem_zero(&info, (giff_u32)sizeof(info));
    info.version_major = 8u;
    info.version_minor = bytes[4];
    info.width = giff_u16le(bytes + 6);
    info.height = giff_u16le(bytes + 8);
    packed = bytes[10];
    info.has_global_palette = (giff_u8)((packed & 0x80u) ? 1u : 0u);
    info.color_resolution_bits = (giff_u8)(((packed >> 4) & 0x07u) + 1u);
    info.global_palette_sorted = (giff_u8)((packed & 0x08u) ? 1u : 0u);
    gct_bits = (giff_u8)((packed & 0x07u) + 1u);
    info.global_palette_entries = info.has_global_palette ? (giff_u16)(1u << gct_bits) : 0u;
    info.background_index = bytes[11];
    info.aspect_byte = bytes[12];
    info.loop_count = 1u;

    if (info.width == 0u || info.height == 0u) {
        return GIFF_E_BAD_DIMENSIONS;
    }

    *out_info = info;
    return GIFF_OK;
}

giff_result giff_decoder_feed(
    giff_decoder* dec,
    const giff_u8* data,
    giff_u32 size,
    giff_u32* consumed
)
{
    giff_result rc;
    giff_info info;
    giff_u32 available;
    giff_u32 take;
    giff_u32 need;
    giff_u32 buffered;
    giff_u8 eof;

    if (dec == 0 || (data == 0 && size != 0u)) {
        return GIFF_E_INVALID_ARGUMENT;
    }

    if (consumed != 0) {
        *consumed = 0u;
    }

    if (dec->finished) {
        return GIFF_E_DONE;
    }

    if (dec->started && dec->source_mode != 3u) {
        return GIFF_E_INVALID_ARGUMENT;
    }

    if (dec->source_mode != 3u && !dec->started) {
        giff_decoder_reset(dec);
        dec->source_mode = 3u;
    }

    if (size != 0u) {
        if (dec->stream_buf == 0) {
            return GIFF_E_INVALID_ARGUMENT;
        }
        if (dec->started && dec->source_mode == 3u) {
            giff_decoder_compact_feed_buffer(dec);
        }
        available = dec->stream_buf_cap - dec->stream_buf_fill;
        take = size;
        if (take > available) {
            take = available;
        }
        if (take != 0u) {
            giff_mem_copy(dec->stream_buf + dec->stream_buf_fill, data, take);
            dec->stream_buf_fill += take;
        }
        if (consumed != 0) {
            *consumed = take;
        }
        if (take != size) {
            if (dec->started && dec->source_mode == 3u) {
                dec->stream_size = dec->stream_buf_fill;
                dec->parse_end = dec->stream_buf + dec->stream_buf_fill;
            }
            return GIFF_E_OUTPUT_OVERFLOW;
        }
    }

    if (data == 0 && size == 0u) {
        dec->stream_eof = 1u;
    }

    if (!dec->started) {
        if (dec->stream_buf_fill < 13u) {
            return dec->stream_eof ? GIFF_E_TRUNCATED : GIFF_E_NEED_MORE_INPUT;
        }

        rc = giff_decoder_inspect_header(dec->stream_buf, dec->stream_buf_fill, &info);
        if (rc != GIFF_OK) {
            if (rc == GIFF_E_NEED_MORE_INPUT && !dec->stream_eof) {
                return rc;
            }
            dec->last_result = (giff_u8)(-rc);
            return rc;
        }

        need = 13u;
        if (info.has_global_palette) {
            need += (giff_u32)info.global_palette_entries * 3u;
        }
        if (dec->stream_buf_fill < need) {
            return dec->stream_eof ? GIFF_E_TRUNCATED : GIFF_E_NEED_MORE_INPUT;
        }

        buffered = dec->stream_buf_fill;
        eof = dec->stream_eof;
        rc = giff_decoder_begin_memory(dec, dec->stream_buf, buffered);
        if (rc != GIFF_OK) {
            return rc;
        }
        dec->source_mode = 3u;
        dec->stream_base_offset = 0u;
        dec->stream_buf_fill = buffered;
        dec->stream_buf_pos = 0u;
        dec->stream_size = buffered;
        dec->parse_end = dec->stream_buf + buffered;
        dec->stream_eof = eof;
        return GIFF_OK;
    }

    if (dec->source_mode == 3u) {
        dec->stream_size = dec->stream_buf_fill;
        dec->parse_end = dec->stream_buf + dec->stream_buf_fill;
    }

    return GIFF_OK;
}

void giff_decoder_finish_input(giff_decoder* dec)
{
    if (dec != 0) {
        dec->stream_eof = 1u;
    }
}

static giff_result giff_stream_fill(giff_decoder* dec, giff_u32 need)
{
    giff_u32 unread;
    giff_u32 want;
    giff_u32 got;
    int ok;

    if (dec == 0 || dec->stream_buf == 0 || dec->io.read == 0) {
        return GIFF_E_INVALID_ARGUMENT;
    }

    if (need > dec->stream_buf_cap) {
        return GIFF_E_INTERNAL;
    }

    unread = dec->stream_buf_fill - dec->stream_buf_pos;
    if (unread >= need) {
        return GIFF_OK;
    }

    if (unread != 0u && dec->stream_buf_pos != 0u) {
        giff_mem_copy(dec->stream_buf, dec->stream_buf + dec->stream_buf_pos, unread);
    }
    dec->stream_buf_pos = 0u;
    dec->stream_buf_fill = unread;

    while ((dec->stream_buf_fill - dec->stream_buf_pos) < need) {
        want = dec->stream_buf_cap - dec->stream_buf_fill;
        if (want == 0u) {
            return GIFF_E_INTERNAL;
        }
        got = want;
        ok = dec->io.read(dec->io.user, dec->stream_buf + dec->stream_buf_fill, &got);
        if (!ok) {
            return GIFF_E_IO;
        }
        if (got == 0u) {
            dec->stream_eof = 1u;
            return GIFF_E_TRUNCATED;
        }
        dec->stream_buf_fill += got;
    }

    return GIFF_OK;
}

static giff_result giff_stream_read_bytes(giff_decoder* dec, giff_u8* dst, giff_u32 size)
{
    giff_result rc;
    giff_u32 chunk;
    giff_u32 unread;

    if (dec == 0) {
        return GIFF_E_INVALID_ARGUMENT;
    }

    while (size != 0u) {
        rc = giff_stream_fill(dec, 1u);
        if (rc != GIFF_OK) {
            return rc;
        }
        unread = dec->stream_buf_fill - dec->stream_buf_pos;
        chunk = unread;
        if (chunk > size) {
            chunk = size;
        }
        if (dst != 0) {
            giff_mem_copy(dst, dec->stream_buf + dec->stream_buf_pos, chunk);
            dst += chunk;
        }
        dec->stream_buf_pos += chunk;
        dec->input_offset += chunk;
        size -= chunk;
    }

    return GIFF_OK;
}

static giff_result giff_stream_read_u8(giff_decoder* dec, giff_u8* out)
{
    return giff_stream_read_bytes(dec, out, 1u);
}

static giff_result giff_stream_parse_color_table(
    giff_decoder* dec,
    giff_rgb8* palette,
    giff_u16 entries
)
{
    giff_u16 i;
    giff_u8 rgb[3];
    giff_result rc;

    if (dec == 0 || palette == 0) {
        return GIFF_E_INVALID_ARGUMENT;
    }

    for (i = 0u; i < entries; ++i) {
        rc = giff_stream_read_bytes(dec, rgb, 3u);
        if (rc != GIFF_OK) {
            return rc;
        }
        palette[i].r = rgb[0];
        palette[i].g = rgb[1];
        palette[i].b = rgb[2];
    }

    return GIFF_OK;
}

static giff_result giff_stream_skip_subblocks(giff_decoder* dec)
{
    giff_result rc;
    giff_u8 block_size;

    if (dec == 0) {
        return GIFF_E_INVALID_ARGUMENT;
    }

    while (1) {
        rc = giff_stream_read_u8(dec, &block_size);
        if (rc != GIFF_OK) {
            return rc;
        }
        if (block_size == 0u) {
            break;
        }
        rc = giff_stream_read_bytes(dec, 0, (giff_u32)block_size);
        if (rc != GIFF_OK) {
            return rc;
        }
    }

    return GIFF_OK;
}

static giff_result giff_stream_capture_subblocks(giff_decoder* dec, giff_u8* dst, giff_u8* out_size)
{
    giff_result rc;
    giff_u8 block_size;
    giff_u8 copied;
    giff_u8 take;
    giff_u8 scratch[GIFF_DATA_SUBBLOCK_MAX];

    if (dec == 0 || out_size == 0) {
        return GIFF_E_INVALID_ARGUMENT;
    }

    copied = 0u;
    while (1) {
        rc = giff_stream_read_u8(dec, &block_size);
        if (rc != GIFF_OK) {
            return rc;
        }
        if (block_size == 0u) {
            break;
        }
        take = block_size;
        if ((giff_u32)copied + (giff_u32)take > (giff_u32)GIFF_DATA_SUBBLOCK_MAX) {
            take = (giff_u8)((giff_u32)GIFF_DATA_SUBBLOCK_MAX - (giff_u32)copied);
        }
        if (take != 0u) {
            rc = giff_stream_read_bytes(dec, scratch, (giff_u32)take);
            if (rc != GIFF_OK) {
                return rc;
            }
            if (dst != 0) {
                giff_mem_copy(dst + copied, scratch, (giff_u32)take);
            }
            copied = (giff_u8)(copied + take);
        }
        if (block_size > take) {
            rc = giff_stream_read_bytes(dec, 0, (giff_u32)(block_size - take));
            if (rc != GIFF_OK) {
                return rc;
            }
        }
    }

    *out_size = copied;
    return GIFF_OK;
}

static void giff_init_fallback_palette(giff_rgb8* palette, giff_u16* entries)
{
    palette[0].r = 0u;
    palette[0].g = 0u;
    palette[0].b = 0u;
    palette[1].r = 255u;
    palette[1].g = 255u;
    palette[1].b = 255u;
    *entries = 2u;
}

static giff_result giff_parse_color_table(
    const giff_u8** pp,
    const giff_u8* end,
    giff_rgb8* palette,
    giff_u16 entries
)
{
    const giff_u8* p;
    giff_u32 bytes;
    giff_u16 i;

    if (pp == 0 || *pp == 0 || palette == 0) {
        return GIFF_E_INVALID_ARGUMENT;
    }

    p = *pp;
    bytes = (giff_u32)entries * 3u;
    if ((giff_u32)(end - p) < bytes) {
        return GIFF_E_TRUNCATED;
    }

    for (i = 0u; i < entries; ++i) {
        palette[i].r = *p++;
        palette[i].g = *p++;
        palette[i].b = *p++;
    }

    *pp = p;
    return GIFF_OK;
}

static giff_result giff_skip_subblocks(const giff_u8** pp, const giff_u8* end)
{
    const giff_u8* p;
    giff_u32 block_size;

    if (pp == 0 || *pp == 0) {
        return GIFF_E_INVALID_ARGUMENT;
    }

    p = *pp;
    while (1) {
        if (p >= end) {
            return GIFF_E_TRUNCATED;
        }
        block_size = (giff_u32)(*p++);
        if (block_size == 0u) {
            break;
        }
        if ((giff_u32)(end - p) < block_size) {
            return GIFF_E_TRUNCATED;
        }
        p += block_size;
    }

    *pp = p;
    return GIFF_OK;
}

static giff_result giff_capture_subblocks(
    const giff_u8** pp,
    const giff_u8* end,
    giff_u8* dst,
    giff_u8* out_size
)
{
    const giff_u8* p;
    giff_u32 block_size;
    giff_u32 copy_bytes;
    giff_u32 copied;

    if (pp == 0 || *pp == 0 || out_size == 0) {
        return GIFF_E_INVALID_ARGUMENT;
    }

    p = *pp;
    copied = 0u;

    while (1) {
        if (p >= end) {
            return GIFF_E_TRUNCATED;
        }

        block_size = (giff_u32)(*p++);
        if (block_size == 0u) {
            break;
        }
        if ((giff_u32)(end - p) < block_size) {
            return GIFF_E_TRUNCATED;
        }

        copy_bytes = block_size;
        if (copy_bytes > ((giff_u32)GIFF_DATA_SUBBLOCK_MAX - copied)) {
            copy_bytes = (giff_u32)GIFF_DATA_SUBBLOCK_MAX - copied;
        }

        if (dst != 0 && copy_bytes != 0u) {
            giff_mem_copy(dst + copied, p, copy_bytes);
        }
        copied += copy_bytes;
        p += block_size;
    }

    *pp = p;
    *out_size = (giff_u8)copied;
    return GIFF_OK;
}

static giff_u16 giff_interlaced_row(giff_u16 sequence_row, giff_u16 height)
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

static void giff_background_rgba(const giff_decoder* dec, giff_rgba8* out)
{
    giff_u16 idx;

    idx = (giff_u16)dec->info.background_index;
    if (dec->info.has_global_palette && idx < dec->info.global_palette_entries) {
        out->r = dec->global_palette[idx].r;
        out->g = dec->global_palette[idx].g;
        out->b = dec->global_palette[idx].b;
        out->a = 255u;
    } else {
        out->r = 0u;
        out->g = 0u;
        out->b = 0u;
        out->a = 0u;
    }
}

static giff_u8 giff_background_index(const giff_decoder* dec)
{
    if (dec->info.has_global_palette &&
        (giff_u16)dec->info.background_index < dec->info.global_palette_entries) {
        return dec->info.background_index;
    }
    return 0u;
}

static void giff_fill_canvas_background(giff_decoder* dec)
{
    giff_u32 y;
    giff_u32 x;
    giff_rgba8 bg;
    giff_u8 bg_index;
    giff_u8* row;

    if (dec->cfg.output_mode == (giff_u8)GIFF_OUTPUT_RGBA8888) {
        giff_background_rgba(dec, &bg);
        for (y = 0u; y < (giff_u32)dec->info.height; ++y) {
            row = dec->canvas_rgba + y * dec->canvas_stride;
            for (x = 0u; x < (giff_u32)dec->info.width; ++x) {
                row[x * 4u + 0u] = bg.r;
                row[x * 4u + 1u] = bg.g;
                row[x * 4u + 2u] = bg.b;
                row[x * 4u + 3u] = bg.a;
            }
        }
    } else {
        bg_index = giff_background_index(dec);
        for (y = 0u; y < (giff_u32)dec->info.height; ++y) {
            row = dec->canvas_indexed + y * dec->canvas_stride;
            memset(row, (int)bg_index, (size_t)dec->info.width);
        }
    }
}

static void giff_copy_rect_rgba(
    giff_u8* dst,
    giff_u32 dst_stride,
    const giff_u8* src,
    giff_u32 src_stride,
    giff_u16 left,
    giff_u16 top,
    giff_u16 width,
    giff_u16 height
)
{
    giff_u32 y;
    giff_u8* drow;
    const giff_u8* srow;

    for (y = 0u; y < (giff_u32)height; ++y) {
        drow = dst + ((giff_u32)top + y) * dst_stride + (giff_u32)left * 4u;
        srow = src + ((giff_u32)top + y) * src_stride + (giff_u32)left * 4u;
        memcpy(drow, srow, (size_t)((giff_u32)width * 4u));
    }
}

static void giff_copy_rect_indexed(
    giff_u8* dst,
    giff_u32 dst_stride,
    const giff_u8* src,
    giff_u32 src_stride,
    giff_u16 left,
    giff_u16 top,
    giff_u16 width,
    giff_u16 height
)
{
    giff_u32 y;
    giff_u8* drow;
    const giff_u8* srow;

    for (y = 0u; y < (giff_u32)height; ++y) {
        drow = dst + ((giff_u32)top + y) * dst_stride + (giff_u32)left;
        srow = src + ((giff_u32)top + y) * src_stride + (giff_u32)left;
        memcpy(drow, srow, (size_t)width);
    }
}

static void giff_fill_rect_background(giff_decoder* dec, const giff_frame_info* frame)
{
    giff_u32 y;
    giff_u32 x;
    giff_rgba8 bg;
    giff_u8 bg_index;
    giff_u8* row;

    if (dec->cfg.output_mode == (giff_u8)GIFF_OUTPUT_RGBA8888) {
        giff_background_rgba(dec, &bg);
        for (y = 0u; y < (giff_u32)frame->height; ++y) {
            row = dec->canvas_rgba + ((giff_u32)frame->top + y) * dec->canvas_stride + (giff_u32)frame->left * 4u;
            for (x = 0u; x < (giff_u32)frame->width; ++x) {
                row[x * 4u + 0u] = bg.r;
                row[x * 4u + 1u] = bg.g;
                row[x * 4u + 2u] = bg.b;
                row[x * 4u + 3u] = bg.a;
            }
        }
    } else {
        bg_index = giff_background_index(dec);
        for (y = 0u; y < (giff_u32)frame->height; ++y) {
            row = dec->canvas_indexed + ((giff_u32)frame->top + y) * dec->canvas_stride + (giff_u32)frame->left;
            memset(row, (int)bg_index, (size_t)frame->width);
        }
    }
}

static void giff_snapshot_previous(giff_decoder* dec, const giff_frame_info* frame)
{
    if (dec->cfg.restore_previous_mode == (giff_u8)GIFF_RESTORE_PREVIOUS_DISABLE) {
        return;
    }

    if (dec->cfg.output_mode == (giff_u8)GIFF_OUTPUT_RGBA8888) {
        if (dec->previous_rgba == 0) {
            return;
        }
        if (dec->cfg.restore_previous_mode == (giff_u8)GIFF_RESTORE_PREVIOUS_FULL) {
            giff_mem_copy(dec->previous_rgba, dec->canvas_rgba,
                          (giff_u32)dec->info.height * dec->canvas_stride);
        } else {
            giff_copy_rect_rgba(dec->previous_rgba, dec->previous_stride,
                                dec->canvas_rgba, dec->canvas_stride,
                                frame->left, frame->top, frame->width, frame->height);
        }
    } else {
        if (dec->previous_indexed == 0) {
            return;
        }
        if (dec->cfg.restore_previous_mode == (giff_u8)GIFF_RESTORE_PREVIOUS_FULL) {
            giff_mem_copy(dec->previous_indexed, dec->canvas_indexed,
                          (giff_u32)dec->info.height * dec->canvas_stride);
        } else {
            giff_copy_rect_indexed(dec->previous_indexed, dec->previous_stride,
                                   dec->canvas_indexed, dec->canvas_stride,
                                   frame->left, frame->top, frame->width, frame->height);
        }
    }
}

static void giff_restore_previous(giff_decoder* dec, const giff_frame_info* frame)
{
    if (dec->cfg.restore_previous_mode == (giff_u8)GIFF_RESTORE_PREVIOUS_DISABLE) {
        giff_fill_rect_background(dec, frame);
        return;
    }

    if (dec->cfg.output_mode == (giff_u8)GIFF_OUTPUT_RGBA8888) {
        if (dec->previous_rgba == 0) {
            giff_fill_rect_background(dec, frame);
            return;
        }
        if (dec->cfg.restore_previous_mode == (giff_u8)GIFF_RESTORE_PREVIOUS_FULL) {
            giff_mem_copy(dec->canvas_rgba, dec->previous_rgba,
                          (giff_u32)dec->info.height * dec->canvas_stride);
        } else {
            giff_copy_rect_rgba(dec->canvas_rgba, dec->canvas_stride,
                                dec->previous_rgba, dec->previous_stride,
                                frame->left, frame->top, frame->width, frame->height);
        }
    } else {
        if (dec->previous_indexed == 0) {
            giff_fill_rect_background(dec, frame);
            return;
        }
        if (dec->cfg.restore_previous_mode == (giff_u8)GIFF_RESTORE_PREVIOUS_FULL) {
            giff_mem_copy(dec->canvas_indexed, dec->previous_indexed,
                          (giff_u32)dec->info.height * dec->canvas_stride);
        } else {
            giff_copy_rect_indexed(dec->canvas_indexed, dec->canvas_stride,
                                   dec->previous_indexed, dec->previous_stride,
                                   frame->left, frame->top, frame->width, frame->height);
        }
    }
}

static void giff_apply_disposal(giff_decoder* dec, const giff_frame_info* frame)
{
    giff_u8 disposal;

    disposal = frame->disposal_method;
    if (disposal == 2u) {
        giff_fill_rect_background(dec, frame);
    } else if (disposal == 3u) {
        giff_restore_previous(dec, frame);
    }
}

static giff_result giff_subreader_next_byte(giff_subreader* sr, giff_u8* out_byte)
{
    giff_u32 block_size;

    if (sr == 0 || out_byte == 0) {
        return GIFF_E_INVALID_ARGUMENT;
    }

    while (sr->remaining == 0u) {
        if (sr->finished) {
            return GIFF_E_DONE;
        }
        if (sr->p >= sr->end) {
            return GIFF_E_TRUNCATED;
        }
        block_size = (giff_u32)(*sr->p++);
        if (block_size == 0u) {
            sr->finished = 1u;
            return GIFF_E_DONE;
        }
        if ((giff_u32)(sr->end - sr->p) < block_size) {
            return GIFF_E_TRUNCATED;
        }
        sr->remaining = block_size;
    }

    *out_byte = *sr->p++;
    sr->remaining -= 1u;
    return GIFF_OK;
}

static giff_result giff_subreader_finish(giff_subreader* sr)
{
    giff_u32 block_size;

    if (sr == 0) {
        return GIFF_E_INVALID_ARGUMENT;
    }

    if (sr->remaining != 0u) {
        if ((giff_u32)(sr->end - sr->p) < sr->remaining) {
            return GIFF_E_TRUNCATED;
        }
        sr->p += sr->remaining;
        sr->remaining = 0u;
    }

    if (sr->finished) {
        return GIFF_OK;
    }

    while (1) {
        if (sr->p >= sr->end) {
            return GIFF_E_TRUNCATED;
        }
        block_size = (giff_u32)(*sr->p++);
        if (block_size == 0u) {
            sr->finished = 1u;
            break;
        }
        if ((giff_u32)(sr->end - sr->p) < block_size) {
            return GIFF_E_TRUNCATED;
        }
        sr->p += block_size;
    }

    return GIFF_OK;
}

static giff_result giff_lzw_read_code(giff_lzw_reader* lr, giff_u8 code_size, giff_u16* out_code)
{
    giff_result rc;
    giff_u8 byte;

    if (lr == 0 || out_code == 0) {
        return GIFF_E_INVALID_ARGUMENT;
    }

    while (lr->bits_in_buf < code_size) {
        rc = giff_subreader_next_byte(&lr->sub, &byte);
        if (rc != GIFF_OK) {
            return rc;
        }
        lr->bitbuf |= ((giff_u32)byte << lr->bits_in_buf);
        lr->bits_in_buf = (giff_u8)(lr->bits_in_buf + 8u);
    }

    *out_code = (giff_u16)(lr->bitbuf & ((1u << code_size) - 1u));
    lr->bitbuf >>= code_size;
    lr->bits_in_buf = (giff_u8)(lr->bits_in_buf - code_size);
    return GIFF_OK;
}

static giff_result giff_write_pixel(
    giff_decoder* dec,
    const giff_frame_info* frame,
    const giff_rgb8* palette,
    giff_u16 palette_entries,
    giff_u8 index,
    giff_u32 output_pos
)
{
    giff_u32 total_pixels;
    giff_u32 local_row_seq;
    giff_u32 local_col;
    giff_u32 local_row;
    giff_u32 screen_x;
    giff_u32 screen_y;
    giff_u8* dst;
    giff_u8 use_index;

    total_pixels = (giff_u32)frame->width * (giff_u32)frame->height;
    if (output_pos >= total_pixels) {
        if (dec->cfg.strict_mode) {
            return GIFF_E_OUTPUT_OVERFLOW;
        }
        return GIFF_OK;
    }

    local_row_seq = output_pos / (giff_u32)frame->width;
    local_col = output_pos - local_row_seq * (giff_u32)frame->width;
    local_row = frame->interlaced ? (giff_u32)giff_interlaced_row((giff_u16)local_row_seq, frame->height) : local_row_seq;
    screen_x = (giff_u32)frame->left + local_col;
    screen_y = (giff_u32)frame->top + local_row;

    if (frame->has_transparency && index == frame->transparency_index) {
        return GIFF_OK;
    }

    use_index = index;
    if ((giff_u16)use_index >= palette_entries) {
        if (dec->cfg.strict_mode) {
            return GIFF_E_BAD_COLOR_TABLE;
        }
        use_index = 0u;
    }

    if (dec->cfg.output_mode == (giff_u8)GIFF_OUTPUT_RGBA8888) {
        dst = dec->canvas_rgba + screen_y * dec->canvas_stride + screen_x * 4u;
        dst[0] = palette[use_index].r;
        dst[1] = palette[use_index].g;
        dst[2] = palette[use_index].b;
        dst[3] = 255u;
    } else {
        dec->canvas_indexed[screen_y * dec->canvas_stride + screen_x] = use_index;
    }

    return GIFF_OK;
}

static giff_result giff_decode_image_data(
    giff_decoder* dec,
    const giff_u8** pp,
    const giff_u8* end,
    const giff_frame_info* frame,
    const giff_rgb8* palette,
    giff_u16 palette_entries
)
{
    const giff_u8* p;
    giff_lzw_reader lr;
    giff_result rc;
    giff_u8 min_code_size;
    giff_u16 clear_code;
    giff_u16 end_code;
    giff_u16 next_code;
    giff_u8 code_size;
    giff_u16 old_code;
    giff_u8 first_char;
    giff_u16 code;
    giff_u16 in_code;
    giff_u32 out_count;
    giff_u32 top;
    giff_u32 total_pixels;

    if (pp == 0 || *pp == 0 || dec == 0 || frame == 0 || palette == 0) {
        return GIFF_E_INVALID_ARGUMENT;
    }

    p = *pp;
    if (p >= end) {
        return GIFF_E_TRUNCATED;
    }

    min_code_size = *p++;
    if (min_code_size < 2u) {
        if (dec->cfg.strict_mode) {
            return GIFF_E_BAD_BLOCK;
        }
        min_code_size = 2u;
    }
    if (min_code_size > 8u) {
        return GIFF_E_BAD_BLOCK;
    }

    giff_lzw_dec_reset(dec);

    lr.sub.p = p;
    lr.sub.end = end;
    lr.sub.remaining = 0u;
    lr.sub.finished = 0u;
    lr.bitbuf = 0u;
    lr.bits_in_buf = 0u;

    clear_code = (giff_u16)(1u << min_code_size);
    end_code = (giff_u16)(clear_code + 1u);
    next_code = (giff_u16)(end_code + 1u);
    code_size = (giff_u8)(min_code_size + 1u);
    old_code = 0xFFFFu;
    first_char = 0u;
    out_count = 0u;
    total_pixels = (giff_u32)frame->width * (giff_u32)frame->height;

    while (1) {
        rc = giff_lzw_read_code(&lr, code_size, &code);
        if (rc == GIFF_E_DONE) {
            rc = GIFF_E_TRUNCATED;
        }
        if (rc != GIFF_OK) {
            return rc;
        }

        if (code == clear_code) {
            next_code = (giff_u16)(end_code + 1u);
            code_size = (giff_u8)(min_code_size + 1u);
            old_code = 0xFFFFu;
            continue;
        }

        if (code == end_code) {
            break;
        }

        in_code = code;
        top = 0u;

        if (old_code == 0xFFFFu) {
            if (code >= clear_code) {
                return GIFF_E_BAD_LZW_CODE;
            }
            first_char = (giff_u8)code;
            rc = giff_write_pixel(dec, frame, palette, palette_entries, first_char, out_count);
            if (rc != GIFF_OK) {
                return rc;
            }
            out_count += 1u;
            old_code = code;
            continue;
        }

        if (code >= next_code) {
            if (code != next_code) {
                return GIFF_E_BAD_LZW_CODE;
            }
            if (top >= (giff_u32)GIFF_LZW_TABLE_SIZE) {
                return GIFF_E_BAD_LZW_CODE;
            }
            dec->lzw_stack[top++] = first_char;
            code = old_code;
        }

        while (code >= clear_code) {
            if (code >= (giff_u16)GIFF_LZW_TABLE_SIZE || top >= (giff_u32)GIFF_LZW_TABLE_SIZE) {
                return GIFF_E_BAD_LZW_CODE;
            }
            dec->lzw_stack[top++] = dec->lzw_suffix[code];
            code = dec->lzw_prefix[code];
        }

        first_char = dec->lzw_suffix[code];
        if (top >= (giff_u32)GIFF_LZW_TABLE_SIZE) {
            return GIFF_E_BAD_LZW_CODE;
        }
        dec->lzw_stack[top++] = first_char;

        while (top != 0u) {
            top -= 1u;
            rc = giff_write_pixel(dec, frame, palette, palette_entries, dec->lzw_stack[top], out_count);
            if (rc != GIFF_OK) {
                return rc;
            }
            out_count += 1u;
        }

        if (next_code < (giff_u16)GIFF_LZW_TABLE_SIZE) {
            dec->lzw_prefix[next_code] = old_code;
            dec->lzw_suffix[next_code] = first_char;
            next_code = (giff_u16)(next_code + 1u);
            if (next_code == (giff_u16)(1u << code_size) && code_size < (giff_u8)GIFF_LZW_MAX_BITS) {
                code_size = (giff_u8)(code_size + 1u);
            }
        }

        old_code = in_code;
    }

    rc = giff_subreader_finish(&lr.sub);
    if (rc != GIFF_OK) {
        return rc;
    }

    if (out_count < total_pixels && dec->cfg.strict_mode) {
        return GIFF_E_BAD_LZW_CODE;
    }

    *pp = lr.sub.p;
    return GIFF_OK;
}

static giff_result giff_parse_graphic_control(
    const giff_u8** pp,
    const giff_u8* end,
    giff_decoder* dec
)
{
    const giff_u8* p;
    giff_u8 block_size;
    giff_u8 packed;

    if (pp == 0 || *pp == 0 || dec == 0) {
        return GIFF_E_INVALID_ARGUMENT;
    }

    p = *pp;
    if ((giff_u32)(end - p) < 6u) {
        return GIFF_E_TRUNCATED;
    }

    block_size = p[0];
    if (block_size != 4u && dec->cfg.strict_mode) {
        return GIFF_E_BAD_BLOCK;
    }

    packed = p[1];
    dec->pending_disposal_method = (giff_u8)((packed >> 2) & 0x07u);
    dec->pending_has_transparency = (giff_u8)(packed & 0x01u);
    dec->pending_delay_cs = giff_u16le(p + 2);
    dec->pending_transparency_index = p[4];

    if (p[5] != 0u && dec->cfg.strict_mode) {
        return GIFF_E_BAD_BLOCK;
    }

    *pp = p + 6;
    return GIFF_OK;
}

static giff_result giff_parse_image_descriptor(
    const giff_u8** pp,
    const giff_u8* end,
    giff_decoder* dec,
    giff_frame_info* out_frame,
    const giff_rgb8** out_palette,
    giff_u16* out_palette_entries
)
{
    const giff_u8* p;
    giff_u8 packed;
    giff_u8 has_local_palette;
    giff_u8 local_bits;
    giff_result rc;
    giff_u16 entries;
    giff_rgb8* active_palette;

    if (pp == 0 || *pp == 0 || dec == 0 || out_frame == 0 || out_palette == 0 || out_palette_entries == 0) {
        return GIFF_E_INVALID_ARGUMENT;
    }

    p = *pp;
    if ((giff_u32)(end - p) < 9u) {
        return GIFF_E_TRUNCATED;
    }

    out_frame->left = giff_u16le(p + 0);
    out_frame->top = giff_u16le(p + 2);
    out_frame->width = giff_u16le(p + 4);
    out_frame->height = giff_u16le(p + 6);
    packed = p[8];
    p += 9;

    if (out_frame->width == 0u || out_frame->height == 0u) {
        return GIFF_E_BAD_DIMENSIONS;
    }

    if ((giff_u32)out_frame->left + (giff_u32)out_frame->width > (giff_u32)dec->info.width ||
        (giff_u32)out_frame->top + (giff_u32)out_frame->height > (giff_u32)dec->info.height) {
        return GIFF_E_BAD_DIMENSIONS;
    }

    out_frame->delay_cs = dec->pending_delay_cs;
    out_frame->disposal_method = dec->pending_disposal_method;
    out_frame->has_transparency = dec->pending_has_transparency;
    out_frame->transparency_index = dec->pending_transparency_index;
    out_frame->interlaced = (giff_u8)((packed & 0x40u) ? 1u : 0u);

    has_local_palette = (giff_u8)((packed & 0x80u) ? 1u : 0u);
    out_frame->has_local_palette = has_local_palette;
    out_frame->local_palette_entries = 0u;

    if (has_local_palette) {
        local_bits = (giff_u8)((packed & 0x07u) + 1u);
        entries = (giff_u16)(1u << local_bits);
        out_frame->local_palette_entries = entries;
        rc = giff_parse_color_table(&p, end, dec->local_palette, entries);
        if (rc != GIFF_OK) {
            return rc;
        }
        active_palette = dec->local_palette;
        *out_palette_entries = entries;
        *out_palette = active_palette;
    } else if (dec->info.has_global_palette && dec->info.global_palette_entries != 0u) {
        *out_palette = dec->global_palette;
        *out_palette_entries = dec->info.global_palette_entries;
    } else {
        giff_init_fallback_palette(dec->local_palette, out_palette_entries);
        *out_palette = dec->local_palette;
    }

    *pp = p;
    return GIFF_OK;
}

static giff_result giff_parse_application_extension(
    const giff_u8** pp,
    const giff_u8* end,
    giff_decoder* dec,
    giff_event* out_event
)
{
    const giff_u8* p;
    giff_u32 block_size;
    giff_u32 first_size;
    giff_u8 matched;

    if (pp == 0 || *pp == 0 || dec == 0) {
        return GIFF_E_INVALID_ARGUMENT;
    }

    p = *pp;
    if (p >= end) {
        return GIFF_E_TRUNCATED;
    }

    block_size = (giff_u32)(*p++);
    if ((giff_u32)(end - p) < block_size) {
        return GIFF_E_TRUNCATED;
    }

    matched = 0u;
    if (block_size == 11u) {
        if ((memcmp(p, "NETSCAPE2.0", 11u) == 0) ||
            (memcmp(p, "ANIMEXTS1.0", 11u) == 0)) {
            matched = 1u;
        }
    }

    if (dec->cfg.capture_comments && out_event != 0) {
        dec->capture_size = (block_size > (giff_u32)GIFF_DATA_SUBBLOCK_MAX) ?
            (giff_u8)GIFF_DATA_SUBBLOCK_MAX : (giff_u8)block_size;
        if (dec->capture_size != 0u) {
            giff_mem_copy(dec->subblock, p, dec->capture_size);
        }
    }

    p += block_size;

    if (matched) {
        if (p >= end) {
            return GIFF_E_TRUNCATED;
        }
        first_size = (giff_u32)(*p++);
        if ((giff_u32)(end - p) < first_size) {
            return GIFF_E_TRUNCATED;
        }
        if (first_size >= 3u && p[0] == 1u) {
            dec->info.loop_count = giff_u16le(p + 1);
        }
        p += first_size;
        while (1) {
            if (p >= end) {
                return GIFF_E_TRUNCATED;
            }
            block_size = (giff_u32)(*p++);
            if (block_size == 0u) {
                break;
            }
            if ((giff_u32)(end - p) < block_size) {
                return GIFF_E_TRUNCATED;
            }
            p += block_size;
        }
    } else {
        giff_result rc;
        rc = giff_skip_subblocks(&p, end);
        if (rc != GIFF_OK) {
            return rc;
        }
    }

    *pp = p;

    if (dec->cfg.capture_comments && out_event != 0) {
        giff_event_fill(dec, out_event, GIFF_EVENT_APPLICATION);
        out_event->text_bytes = dec->capture_size ? dec->subblock : 0;
        out_event->text_size = dec->capture_size;
    }

    return GIFF_OK;
}

static giff_result giff_parse_comment_extension(
    const giff_u8** pp,
    const giff_u8* end,
    giff_decoder* dec,
    giff_event* out_event
)
{
    giff_result rc;

    if (pp == 0 || *pp == 0 || dec == 0) {
        return GIFF_E_INVALID_ARGUMENT;
    }

    dec->capture_size = 0u;
    if (dec->cfg.capture_comments) {
        rc = giff_capture_subblocks(pp, end, dec->subblock, &dec->capture_size);
        if (rc != GIFF_OK) {
            return rc;
        }
        if (out_event != 0) {
            giff_event_fill(dec, out_event, GIFF_EVENT_COMMENT);
            out_event->text_bytes = dec->capture_size ? dec->subblock : 0;
            out_event->text_size = dec->capture_size;
        }
    } else {
        rc = giff_skip_subblocks(pp, end);
        if (rc != GIFF_OK) {
            return rc;
        }
    }

    return GIFF_OK;
}

static giff_result giff_parse_plain_text_extension(
    const giff_u8** pp,
    const giff_u8* end,
    giff_decoder* dec
)
{
    const giff_u8* p;
    giff_u32 block_size;
    giff_result rc;

    if (pp == 0 || *pp == 0 || dec == 0) {
        return GIFF_E_INVALID_ARGUMENT;
    }

    p = *pp;
    if (p >= end) {
        return GIFF_E_TRUNCATED;
    }

    block_size = (giff_u32)(*p++);
    if ((giff_u32)(end - p) < block_size) {
        return GIFF_E_TRUNCATED;
    }
    p += block_size;

    rc = giff_skip_subblocks(&p, end);
    if (rc != GIFF_OK) {
        return rc;
    }

    *pp = p;
    return GIFF_OK;
}

static giff_result giff_io_subreader_next_byte(giff_io_subreader* sr, giff_u8* out_byte)
{
    giff_result rc;
    giff_u8 block_size;

    if (sr == 0 || out_byte == 0 || sr->dec == 0) {
        return GIFF_E_INVALID_ARGUMENT;
    }

    while (sr->remaining == 0u) {
        if (sr->finished) {
            return GIFF_E_DONE;
        }
        rc = giff_stream_read_u8(sr->dec, &block_size);
        if (rc != GIFF_OK) {
            return rc;
        }
        if (block_size == 0u) {
            sr->finished = 1u;
            return GIFF_E_DONE;
        }
        sr->remaining = (giff_u32)block_size;
    }

    rc = giff_stream_read_u8(sr->dec, out_byte);
    if (rc != GIFF_OK) {
        return rc;
    }
    sr->remaining -= 1u;
    return GIFF_OK;
}

static giff_result giff_io_subreader_finish(giff_io_subreader* sr)
{
    giff_result rc;
    giff_u8 block_size;

    if (sr == 0 || sr->dec == 0) {
        return GIFF_E_INVALID_ARGUMENT;
    }

    if (sr->remaining != 0u) {
        rc = giff_stream_read_bytes(sr->dec, 0, sr->remaining);
        if (rc != GIFF_OK) {
            return rc;
        }
        sr->remaining = 0u;
    }

    if (sr->finished) {
        return GIFF_OK;
    }

    while (1) {
        rc = giff_stream_read_u8(sr->dec, &block_size);
        if (rc != GIFF_OK) {
            return rc;
        }
        if (block_size == 0u) {
            sr->finished = 1u;
            break;
        }
        rc = giff_stream_read_bytes(sr->dec, 0, (giff_u32)block_size);
        if (rc != GIFF_OK) {
            return rc;
        }
    }

    return GIFF_OK;
}

static giff_result giff_io_lzw_read_code(giff_io_lzw_reader* lr, giff_u8 code_size, giff_u16* out_code)
{
    giff_result rc;
    giff_u8 byte;

    if (lr == 0 || out_code == 0) {
        return GIFF_E_INVALID_ARGUMENT;
    }

    while (lr->bits_in_buf < code_size) {
        rc = giff_io_subreader_next_byte(&lr->sub, &byte);
        if (rc != GIFF_OK) {
            return rc;
        }
        lr->bitbuf |= ((giff_u32)byte << lr->bits_in_buf);
        lr->bits_in_buf = (giff_u8)(lr->bits_in_buf + 8u);
    }

    *out_code = (giff_u16)(lr->bitbuf & ((1u << code_size) - 1u));
    lr->bitbuf >>= code_size;
    lr->bits_in_buf = (giff_u8)(lr->bits_in_buf - code_size);
    return GIFF_OK;
}

static giff_result giff_stream_decode_image_data(
    giff_decoder* dec,
    const giff_frame_info* frame,
    const giff_rgb8* palette,
    giff_u16 palette_entries
)
{
    giff_io_lzw_reader lr;
    giff_result rc;
    giff_u8 min_code_size;
    giff_u16 clear_code;
    giff_u16 end_code;
    giff_u16 next_code;
    giff_u8 code_size;
    giff_u16 old_code;
    giff_u8 first_char;
    giff_u16 code;
    giff_u16 in_code;
    giff_u32 out_count;
    giff_u32 top;
    giff_u32 total_pixels;

    if (dec == 0 || frame == 0 || palette == 0) {
        return GIFF_E_INVALID_ARGUMENT;
    }

    rc = giff_stream_read_u8(dec, &min_code_size);
    if (rc != GIFF_OK) {
        return rc;
    }
    if (min_code_size < 2u) {
        if (dec->cfg.strict_mode) {
            return GIFF_E_BAD_BLOCK;
        }
        min_code_size = 2u;
    }
    if (min_code_size > 8u) {
        return GIFF_E_BAD_BLOCK;
    }

    giff_lzw_dec_reset(dec);

    lr.sub.dec = dec;
    lr.sub.remaining = 0u;
    lr.sub.finished = 0u;
    lr.bitbuf = 0u;
    lr.bits_in_buf = 0u;

    clear_code = (giff_u16)(1u << min_code_size);
    end_code = (giff_u16)(clear_code + 1u);
    next_code = (giff_u16)(end_code + 1u);
    code_size = (giff_u8)(min_code_size + 1u);
    old_code = 0xFFFFu;
    first_char = 0u;
    out_count = 0u;
    total_pixels = (giff_u32)frame->width * (giff_u32)frame->height;

    while (1) {
        rc = giff_io_lzw_read_code(&lr, code_size, &code);
        if (rc == GIFF_E_DONE) {
            rc = GIFF_E_TRUNCATED;
        }
        if (rc != GIFF_OK) {
            return rc;
        }

        if (code == clear_code) {
            next_code = (giff_u16)(end_code + 1u);
            code_size = (giff_u8)(min_code_size + 1u);
            old_code = 0xFFFFu;
            continue;
        }

        if (code == end_code) {
            break;
        }

        in_code = code;
        top = 0u;

        if (old_code == 0xFFFFu) {
            if (code >= clear_code) {
                return GIFF_E_BAD_LZW_CODE;
            }
            first_char = (giff_u8)code;
            rc = giff_write_pixel(dec, frame, palette, palette_entries, first_char, out_count);
            if (rc != GIFF_OK) {
                return rc;
            }
            out_count += 1u;
            old_code = code;
            continue;
        }

        if (code >= next_code) {
            if (code != next_code) {
                return GIFF_E_BAD_LZW_CODE;
            }
            if (top >= (giff_u32)GIFF_LZW_TABLE_SIZE) {
                return GIFF_E_BAD_LZW_CODE;
            }
            dec->lzw_stack[top++] = first_char;
            code = old_code;
        }

        while (code >= clear_code) {
            if (code >= (giff_u16)GIFF_LZW_TABLE_SIZE || top >= (giff_u32)GIFF_LZW_TABLE_SIZE) {
                return GIFF_E_BAD_LZW_CODE;
            }
            dec->lzw_stack[top++] = dec->lzw_suffix[code];
            code = dec->lzw_prefix[code];
        }

        first_char = dec->lzw_suffix[code];
        if (top >= (giff_u32)GIFF_LZW_TABLE_SIZE) {
            return GIFF_E_BAD_LZW_CODE;
        }
        dec->lzw_stack[top++] = first_char;

        while (top != 0u) {
            top -= 1u;
            rc = giff_write_pixel(dec, frame, palette, palette_entries, dec->lzw_stack[top], out_count);
            if (rc != GIFF_OK) {
                return rc;
            }
            out_count += 1u;
        }

        if (next_code < (giff_u16)GIFF_LZW_TABLE_SIZE) {
            dec->lzw_prefix[next_code] = old_code;
            dec->lzw_suffix[next_code] = first_char;
            next_code = (giff_u16)(next_code + 1u);
            if (next_code == (giff_u16)(1u << code_size) && code_size < (giff_u8)GIFF_LZW_MAX_BITS) {
                code_size = (giff_u8)(code_size + 1u);
            }
        }

        old_code = in_code;
    }

    rc = giff_io_subreader_finish(&lr.sub);
    if (rc != GIFF_OK) {
        return rc;
    }

    if (out_count < total_pixels && dec->cfg.strict_mode) {
        return GIFF_E_BAD_LZW_CODE;
    }

    return GIFF_OK;
}

static giff_result giff_stream_parse_graphic_control(giff_decoder* dec)
{
    giff_result rc;
    giff_u8 block_size;
    giff_u8 packed;
    giff_u8 terminator;
    giff_u8 payload[4];

    if (dec == 0) {
        return GIFF_E_INVALID_ARGUMENT;
    }

    rc = giff_stream_read_u8(dec, &block_size);
    if (rc != GIFF_OK) {
        return rc;
    }

    payload[0] = 0u;
    payload[1] = 0u;
    payload[2] = 0u;
    payload[3] = 0u;

    if (block_size >= 4u) {
        rc = giff_stream_read_bytes(dec, payload, 4u);
        if (rc != GIFF_OK) {
            return rc;
        }
        if (block_size > 4u) {
            rc = giff_stream_read_bytes(dec, 0, (giff_u32)(block_size - 4u));
            if (rc != GIFF_OK) {
                return rc;
            }
        }
    } else {
        if (dec->cfg.strict_mode) {
            return GIFF_E_BAD_BLOCK;
        }
        if (block_size != 0u) {
            rc = giff_stream_read_bytes(dec, payload, (giff_u32)block_size);
            if (rc != GIFF_OK) {
                return rc;
            }
        }
    }

    rc = giff_stream_read_u8(dec, &terminator);
    if (rc != GIFF_OK) {
        return rc;
    }
    if (terminator != 0u && dec->cfg.strict_mode) {
        return GIFF_E_BAD_BLOCK;
    }

    packed = payload[0];
    dec->pending_disposal_method = (giff_u8)((packed >> 2) & 0x07u);
    dec->pending_has_transparency = (giff_u8)(packed & 0x01u);
    dec->pending_delay_cs = (giff_u16)((giff_u16)payload[1] | ((giff_u16)payload[2] << 8));
    dec->pending_transparency_index = payload[3];

    return GIFF_OK;
}

static giff_result giff_stream_parse_image_descriptor(
    giff_decoder* dec,
    giff_frame_info* out_frame,
    const giff_rgb8** out_palette,
    giff_u16* out_palette_entries
)
{
    giff_result rc;
    giff_u8 bytes[9];
    giff_u8 packed;
    giff_u8 has_local_palette;
    giff_u8 local_bits;
    giff_u16 entries;

    if (dec == 0 || out_frame == 0 || out_palette == 0 || out_palette_entries == 0) {
        return GIFF_E_INVALID_ARGUMENT;
    }

    rc = giff_stream_read_bytes(dec, bytes, 9u);
    if (rc != GIFF_OK) {
        return rc;
    }

    out_frame->left = (giff_u16)((giff_u16)bytes[0] | ((giff_u16)bytes[1] << 8));
    out_frame->top = (giff_u16)((giff_u16)bytes[2] | ((giff_u16)bytes[3] << 8));
    out_frame->width = (giff_u16)((giff_u16)bytes[4] | ((giff_u16)bytes[5] << 8));
    out_frame->height = (giff_u16)((giff_u16)bytes[6] | ((giff_u16)bytes[7] << 8));
    packed = bytes[8];

    if (out_frame->width == 0u || out_frame->height == 0u) {
        return GIFF_E_BAD_DIMENSIONS;
    }

    if ((giff_u32)out_frame->left + (giff_u32)out_frame->width > (giff_u32)dec->info.width ||
        (giff_u32)out_frame->top + (giff_u32)out_frame->height > (giff_u32)dec->info.height) {
        return GIFF_E_BAD_DIMENSIONS;
    }

    out_frame->delay_cs = dec->pending_delay_cs;
    out_frame->disposal_method = dec->pending_disposal_method;
    out_frame->has_transparency = dec->pending_has_transparency;
    out_frame->transparency_index = dec->pending_transparency_index;
    out_frame->interlaced = (giff_u8)((packed & 0x40u) ? 1u : 0u);
    out_frame->has_local_palette = (giff_u8)((packed & 0x80u) ? 1u : 0u);
    out_frame->local_palette_entries = 0u;

    has_local_palette = out_frame->has_local_palette;
    if (has_local_palette) {
        local_bits = (giff_u8)((packed & 0x07u) + 1u);
        entries = (giff_u16)(1u << local_bits);
        out_frame->local_palette_entries = entries;
        rc = giff_stream_parse_color_table(dec, dec->local_palette, entries);
        if (rc != GIFF_OK) {
            return rc;
        }
        *out_palette = dec->local_palette;
        *out_palette_entries = entries;
    } else if (dec->info.has_global_palette && dec->info.global_palette_entries != 0u) {
        *out_palette = dec->global_palette;
        *out_palette_entries = dec->info.global_palette_entries;
    } else {
        giff_init_fallback_palette(dec->local_palette, out_palette_entries);
        *out_palette = dec->local_palette;
    }

    return GIFF_OK;
}

static giff_result giff_stream_parse_application_extension(
    giff_decoder* dec,
    giff_event* out_event
)
{
    giff_result rc;
    giff_u8 block_size;
    giff_u8 scratch[GIFF_DATA_SUBBLOCK_MAX];
    giff_u8 first_size;
    giff_u8 matched;

    if (dec == 0) {
        return GIFF_E_INVALID_ARGUMENT;
    }

    rc = giff_stream_read_u8(dec, &block_size);
    if (rc != GIFF_OK) {
        return rc;
    }

    if (block_size != 0u) {
        rc = giff_stream_read_bytes(dec, scratch, (giff_u32)block_size);
        if (rc != GIFF_OK) {
            return rc;
        }
    }

    matched = 0u;
    if (block_size == 11u) {
        if ((memcmp(scratch, "NETSCAPE2.0", 11u) == 0) ||
            (memcmp(scratch, "ANIMEXTS1.0", 11u) == 0)) {
            matched = 1u;
        }
    }

    dec->capture_size = 0u;
    if (dec->cfg.capture_comments && out_event != 0) {
        dec->capture_size = block_size;
        if (dec->capture_size != 0u) {
            giff_mem_copy(dec->subblock, scratch, dec->capture_size);
        }
    }

    if (matched) {
        rc = giff_stream_read_u8(dec, &first_size);
        if (rc != GIFF_OK) {
            return rc;
        }
        if (first_size != 0u) {
            rc = giff_stream_read_bytes(dec, scratch, (giff_u32)first_size);
            if (rc != GIFF_OK) {
                return rc;
            }
            if (first_size >= 3u && scratch[0] == 1u) {
                dec->info.loop_count = (giff_u16)((giff_u16)scratch[1] | ((giff_u16)scratch[2] << 8));
            }
        }
        rc = giff_stream_skip_subblocks(dec);
        if (rc != GIFF_OK) {
            return rc;
        }
    } else {
        rc = giff_stream_skip_subblocks(dec);
        if (rc != GIFF_OK) {
            return rc;
        }
    }

    if (dec->cfg.capture_comments && out_event != 0) {
        giff_event_fill(dec, out_event, GIFF_EVENT_APPLICATION);
        out_event->text_bytes = dec->capture_size ? dec->subblock : 0;
        out_event->text_size = dec->capture_size;
    }

    return GIFF_OK;
}

static giff_result giff_stream_parse_comment_extension(
    giff_decoder* dec,
    giff_event* out_event
)
{
    giff_result rc;

    if (dec == 0) {
        return GIFF_E_INVALID_ARGUMENT;
    }

    dec->capture_size = 0u;
    if (dec->cfg.capture_comments) {
        rc = giff_stream_capture_subblocks(dec, dec->subblock, &dec->capture_size);
        if (rc != GIFF_OK) {
            return rc;
        }
        if (out_event != 0) {
            giff_event_fill(dec, out_event, GIFF_EVENT_COMMENT);
            out_event->text_bytes = dec->capture_size ? dec->subblock : 0;
            out_event->text_size = dec->capture_size;
        }
    } else {
        rc = giff_stream_skip_subblocks(dec);
        if (rc != GIFF_OK) {
            return rc;
        }
    }

    return GIFF_OK;
}

static giff_result giff_stream_parse_plain_text_extension(giff_decoder* dec)
{
    giff_result rc;
    giff_u8 block_size;

    if (dec == 0) {
        return GIFF_E_INVALID_ARGUMENT;
    }

    rc = giff_stream_read_u8(dec, &block_size);
    if (rc != GIFF_OK) {
        return rc;
    }
    if (block_size != 0u) {
        rc = giff_stream_read_bytes(dec, 0, (giff_u32)block_size);
        if (rc != GIFF_OK) {
            return rc;
        }
    }
    return giff_stream_skip_subblocks(dec);
}

giff_result giff_decoder_begin_io(giff_decoder* dec)
{
    giff_result rc;

    if (dec == 0 || dec->io.read == 0) {
        return GIFF_E_INVALID_ARGUMENT;
    }

    giff_decoder_reset(dec);

    rc = giff_stream_read_bytes(dec, dec->header_bytes, 13u);
    if (rc != GIFF_OK) {
        dec->last_result = (giff_u8)(-rc);
        return rc;
    }

    rc = giff_decoder_inspect_header(dec->header_bytes, 13u, &dec->info);
    if (rc != GIFF_OK) {
        dec->last_result = (giff_u8)(-rc);
        return rc;
    }

    if (dec->info.width > dec->cfg.max_width || dec->info.height > dec->cfg.max_height) {
        dec->last_result = (giff_u8)(-GIFF_E_BAD_DIMENSIONS);
        return GIFF_E_BAD_DIMENSIONS;
    }

    if (dec->info.has_global_palette) {
        rc = giff_stream_parse_color_table(dec, dec->global_palette, dec->info.global_palette_entries);
        if (rc != GIFF_OK) {
            dec->last_result = (giff_u8)(-rc);
            return rc;
        }
    }

    giff_set_default_stride(dec);
    giff_fill_canvas_background(dec);

    dec->started = 1u;
    dec->finished = 0u;
    dec->info_emitted = 0u;
    dec->trailer_emitted = 0u;
    dec->source_mode = 2u;
    giff_decoder_clear_pending_gce(dec);
    dec->last_result = (giff_u8)GIFF_OK;
    return GIFF_OK;
}

static giff_result giff_decoder_next_event_io(
    giff_decoder* dec,
    giff_event* out_event
)
{
    giff_result rc;
    const giff_rgb8* palette;
    giff_u16 palette_entries;
    giff_u8 label;
    giff_u8 extension_label;
    giff_u32 screen_y;

    if (dec == 0 || out_event == 0) {
        return GIFF_E_INVALID_ARGUMENT;
    }

    giff_event_zero(out_event);

    if (!dec->started) {
        return GIFF_E_INVALID_ARGUMENT;
    }

    if (dec->finished && dec->trailer_emitted) {
        return GIFF_E_DONE;
    }

    if (!dec->info_emitted) {
        dec->info_emitted = 1u;
        giff_event_fill(dec, out_event, GIFF_EVENT_INFO);
        dec->events_emitted += 1u;
        return GIFF_OK;
    }

    if (dec->event_phase == 1u) {
        if (dec->emit_row_cursor < dec->emit_row_count) {
            giff_event_fill(dec, out_event, GIFF_EVENT_FRAME_ROW);
            out_event->row_index = dec->emit_row_cursor;
            screen_y = (giff_u32)dec->frame.top + (giff_u32)dec->emit_row_cursor;

            if (dec->cfg.output_mode == (giff_u8)GIFF_OUTPUT_RGBA8888) {
                out_event->rgba_row = (const giff_rgba8*)(const void*)
                    (dec->canvas_rgba + screen_y * dec->canvas_stride + (giff_u32)dec->frame.left * 4u);
            } else {
                out_event->indexed_row =
                    dec->canvas_indexed + screen_y * dec->canvas_stride + (giff_u32)dec->frame.left;
            }

            dec->emit_row_cursor = (giff_u16)(dec->emit_row_cursor + 1u);
            dec->events_emitted += 1u;
            return GIFF_OK;
        }

        dec->event_phase = 0u;
        giff_event_fill(dec, out_event, GIFF_EVENT_FRAME_END);
        dec->events_emitted += 1u;
        return GIFF_OK;
    }

    while (1) {
        rc = giff_stream_read_u8(dec, &label);
        if (rc != GIFF_OK) {
            out_event->type = GIFF_EVENT_ERROR;
            out_event->code = rc;
            dec->last_result = (giff_u8)(-rc);
            return rc;
        }

        if (label == 0x3Bu) {
            dec->finished = 1u;
            dec->trailer_emitted = 1u;
            giff_event_fill(dec, out_event, GIFF_EVENT_TRAILER);
            dec->events_emitted += 1u;
            return GIFF_OK;
        }

        if (label == 0x21u) {
            rc = giff_stream_read_u8(dec, &extension_label);
            if (rc != GIFF_OK) {
                out_event->type = GIFF_EVENT_ERROR;
                out_event->code = rc;
                dec->last_result = (giff_u8)(-rc);
                return rc;
            }
            if (extension_label == 0xF9u) {
                rc = giff_stream_parse_graphic_control(dec);
                if (rc != GIFF_OK) {
                    out_event->type = GIFF_EVENT_ERROR;
                    out_event->code = rc;
                    dec->last_result = (giff_u8)(-rc);
                    return rc;
                }
                continue;
            } else if (extension_label == 0xFFu) {
                rc = giff_stream_parse_application_extension(dec, dec->cfg.capture_comments ? out_event : 0);
                if (rc != GIFF_OK) {
                    out_event->type = GIFF_EVENT_ERROR;
                    out_event->code = rc;
                    dec->last_result = (giff_u8)(-rc);
                    return rc;
                }
                if (dec->cfg.capture_comments) {
                    dec->events_emitted += 1u;
                    return GIFF_OK;
                }
                continue;
            } else if (extension_label == 0xFEu) {
                rc = giff_stream_parse_comment_extension(dec, dec->cfg.capture_comments ? out_event : 0);
                if (rc != GIFF_OK) {
                    out_event->type = GIFF_EVENT_ERROR;
                    out_event->code = rc;
                    dec->last_result = (giff_u8)(-rc);
                    return rc;
                }
                if (dec->cfg.capture_comments) {
                    dec->events_emitted += 1u;
                    return GIFF_OK;
                }
                continue;
            } else if (extension_label == 0x01u) {
                rc = giff_stream_parse_plain_text_extension(dec);
                if (rc != GIFF_OK) {
                    out_event->type = GIFF_EVENT_ERROR;
                    out_event->code = rc;
                    dec->last_result = (giff_u8)(-rc);
                    return rc;
                }
                giff_decoder_clear_pending_gce(dec);
                continue;
            }

            rc = giff_stream_skip_subblocks(dec);
            if (rc != GIFF_OK) {
                out_event->type = GIFF_EVENT_ERROR;
                out_event->code = rc;
                dec->last_result = (giff_u8)(-rc);
                return rc;
            }
            continue;
        }

        if (label == 0x2Cu) {
            if (dec->have_previous_frame) {
                giff_apply_disposal(dec, &dec->previous_frame);
                dec->have_previous_frame = 0u;
            }

            palette = 0;
            palette_entries = 0u;
            rc = giff_stream_parse_image_descriptor(dec, &dec->frame, &palette, &palette_entries);
            if (rc != GIFF_OK) {
                out_event->type = GIFF_EVENT_ERROR;
                out_event->code = rc;
                dec->last_result = (giff_u8)(-rc);
                return rc;
            }

            if (dec->frame.disposal_method == 3u) {
                giff_snapshot_previous(dec, &dec->frame);
            }

            rc = giff_stream_decode_image_data(dec, &dec->frame, palette, palette_entries);
            if (rc != GIFF_OK) {
                out_event->type = GIFF_EVENT_ERROR;
                out_event->code = rc;
                dec->last_result = (giff_u8)(-rc);
                return rc;
            }

            dec->previous_frame = dec->frame;
            dec->have_previous_frame = 1u;
            dec->frames_seen += 1u;
            dec->emit_row_cursor = 0u;
            dec->emit_row_count = dec->frame.height;
            dec->event_phase = 1u;
            giff_decoder_clear_pending_gce(dec);

            giff_event_fill(dec, out_event, GIFF_EVENT_FRAME_BEGIN);
            dec->events_emitted += 1u;
            return GIFF_OK;
        }

        out_event->type = GIFF_EVENT_ERROR;
        out_event->code = GIFF_E_BAD_BLOCK;
        dec->last_result = (giff_u8)(-GIFF_E_BAD_BLOCK);
        return GIFF_E_BAD_BLOCK;
    }
}

giff_result giff_decoder_begin_memory(
    giff_decoder* dec,
    const giff_u8* data,
    giff_u32 size
)
{
    const giff_u8* p;
    const giff_u8* end;
    giff_result rc;

    if (dec == 0 || data == 0) {
        return GIFF_E_INVALID_ARGUMENT;
    }

    giff_decoder_reset(dec);

    rc = giff_decoder_inspect_header(data, size, &dec->info);
    if (rc != GIFF_OK) {
        dec->last_result = (giff_u8)(-rc);
        return rc;
    }

    if (dec->info.width > dec->cfg.max_width || dec->info.height > dec->cfg.max_height) {
        dec->last_result = (giff_u8)(-GIFF_E_BAD_DIMENSIONS);
        return GIFF_E_BAD_DIMENSIONS;
    }

    p = data + 13u;
    end = data + size;

    if (dec->info.has_global_palette) {
        rc = giff_parse_color_table(&p, end, dec->global_palette, dec->info.global_palette_entries);
        if (rc != GIFF_OK) {
            dec->last_result = (giff_u8)(-rc);
            return rc;
        }
    }

    giff_set_default_stride(dec);
    giff_fill_canvas_background(dec);

    dec->stream_data = data;
    dec->stream_size = size;
    dec->parse_ptr = p;
    dec->parse_end = end;
    dec->started = 1u;
    dec->finished = 0u;
    dec->info_emitted = 0u;
    dec->trailer_emitted = 0u;
    dec->source_mode = 1u;
    giff_decoder_clear_pending_gce(dec);
    giff_decoder_sync_offset(dec);
    dec->last_result = (giff_u8)GIFF_OK;
    return GIFF_OK;
}

giff_result giff_decoder_next_event(
    giff_decoder* dec,
    giff_event* out_event
)
{
    const giff_u8* p;
    giff_result rc;
    const giff_rgb8* palette;
    giff_u16 palette_entries;
    giff_u8 label;
    giff_u8 extension_label;
    giff_u32 screen_y;

    if (dec == 0 || out_event == 0) {
        return GIFF_E_INVALID_ARGUMENT;
    }

    giff_event_zero(out_event);

    if (!dec->started) {
        if (dec->source_mode == 3u) {
            return dec->stream_eof ? GIFF_E_TRUNCATED : GIFF_E_NEED_MORE_INPUT;
        }
        return GIFF_E_INVALID_ARGUMENT;
    }

    if (dec->finished && dec->trailer_emitted) {
        return GIFF_E_DONE;
    }

    if (dec->source_mode == 2u) {
        return giff_decoder_next_event_io(dec, out_event);
    }

    if (!dec->info_emitted) {
        dec->info_emitted = 1u;
        giff_event_fill(dec, out_event, GIFF_EVENT_INFO);
        dec->events_emitted += 1u;
        return GIFF_OK;
    }

    if (dec->event_phase == 1u) {
        if (dec->emit_row_cursor < dec->emit_row_count) {
            giff_event_fill(dec, out_event, GIFF_EVENT_FRAME_ROW);
            out_event->row_index = dec->emit_row_cursor;
            screen_y = (giff_u32)dec->frame.top + (giff_u32)dec->emit_row_cursor;

            if (dec->cfg.output_mode == (giff_u8)GIFF_OUTPUT_RGBA8888) {
                out_event->rgba_row = (const giff_rgba8*)(const void*)
                    (dec->canvas_rgba + screen_y * dec->canvas_stride + (giff_u32)dec->frame.left * 4u);
            } else {
                out_event->indexed_row =
                    dec->canvas_indexed + screen_y * dec->canvas_stride + (giff_u32)dec->frame.left;
            }

            dec->emit_row_cursor = (giff_u16)(dec->emit_row_cursor + 1u);
            dec->events_emitted += 1u;
            return GIFF_OK;
        }

        dec->event_phase = 0u;
        giff_event_fill(dec, out_event, GIFF_EVENT_FRAME_END);
        dec->events_emitted += 1u;
        return GIFF_OK;
    }

    while (dec->parse_ptr < dec->parse_end) {
        p = dec->parse_ptr;
        label = *p++;

        if (label == 0x3Bu) {
            dec->parse_ptr = p;
            giff_decoder_sync_offset(dec);
            dec->finished = 1u;
            dec->trailer_emitted = 1u;
            giff_event_fill(dec, out_event, GIFF_EVENT_TRAILER);
            dec->events_emitted += 1u;
            return GIFF_OK;
        }

        if (label == 0x21u) {
            if (p >= dec->parse_end) {
                return giff_need_more_or_truncated(dec);
            }
            extension_label = *p++;

            if (dec->source_mode == 3u) {
                rc = giff_memory_extension_complete(p, dec->parse_end, extension_label);
                if (rc != GIFF_OK) {
                    return giff_need_more_or_truncated(dec);
                }
            }

            if (extension_label == 0xF9u) {
                rc = giff_parse_graphic_control(&p, dec->parse_end, dec);
                if (rc != GIFF_OK) {
                    if (rc == GIFF_E_TRUNCATED && dec->source_mode == 3u && !dec->stream_eof) {
                        return GIFF_E_NEED_MORE_INPUT;
                    }
                    out_event->type = GIFF_EVENT_ERROR;
                    out_event->code = rc;
                    dec->last_result = (giff_u8)(-rc);
                    return rc;
                }
                dec->parse_ptr = p;
                giff_decoder_sync_offset(dec);
                continue;
            } else if (extension_label == 0xFFu) {
                rc = giff_parse_application_extension(&p, dec->parse_end, dec,
                                                      dec->cfg.capture_comments ? out_event : 0);
                if (rc != GIFF_OK) {
                    if (rc == GIFF_E_TRUNCATED && dec->source_mode == 3u && !dec->stream_eof) {
                        return GIFF_E_NEED_MORE_INPUT;
                    }
                    out_event->type = GIFF_EVENT_ERROR;
                    out_event->code = rc;
                    dec->last_result = (giff_u8)(-rc);
                    return rc;
                }
                dec->parse_ptr = p;
                giff_decoder_sync_offset(dec);
                if (dec->cfg.capture_comments) {
                    dec->events_emitted += 1u;
                    return GIFF_OK;
                }
                continue;
            } else if (extension_label == 0xFEu) {
                rc = giff_parse_comment_extension(&p, dec->parse_end, dec,
                                                  dec->cfg.capture_comments ? out_event : 0);
                if (rc != GIFF_OK) {
                    if (rc == GIFF_E_TRUNCATED && dec->source_mode == 3u && !dec->stream_eof) {
                        return GIFF_E_NEED_MORE_INPUT;
                    }
                    out_event->type = GIFF_EVENT_ERROR;
                    out_event->code = rc;
                    dec->last_result = (giff_u8)(-rc);
                    return rc;
                }
                dec->parse_ptr = p;
                giff_decoder_sync_offset(dec);
                if (dec->cfg.capture_comments) {
                    dec->events_emitted += 1u;
                    return GIFF_OK;
                }
                continue;
            } else if (extension_label == 0x01u) {
                rc = giff_parse_plain_text_extension(&p, dec->parse_end, dec);
                if (rc != GIFF_OK) {
                    if (rc == GIFF_E_TRUNCATED && dec->source_mode == 3u && !dec->stream_eof) {
                        return GIFF_E_NEED_MORE_INPUT;
                    }
                    out_event->type = GIFF_EVENT_ERROR;
                    out_event->code = rc;
                    dec->last_result = (giff_u8)(-rc);
                    return rc;
                }
                dec->parse_ptr = p;
                giff_decoder_sync_offset(dec);
                giff_decoder_clear_pending_gce(dec);
                continue;
            }

            rc = giff_skip_subblocks(&p, dec->parse_end);
            if (rc != GIFF_OK) {
                if (rc == GIFF_E_TRUNCATED && dec->source_mode == 3u && !dec->stream_eof) {
                    return GIFF_E_NEED_MORE_INPUT;
                }
                out_event->type = GIFF_EVENT_ERROR;
                out_event->code = rc;
                dec->last_result = (giff_u8)(-rc);
                return rc;
            }
            dec->parse_ptr = p;
            giff_decoder_sync_offset(dec);
            continue;
        }

        if (label == 0x2Cu) {
            if (dec->source_mode == 3u) {
                rc = giff_memory_image_complete(p, dec->parse_end);
                if (rc != GIFF_OK) {
                    return giff_need_more_or_truncated(dec);
                }
            }

            if (dec->have_previous_frame) {
                giff_apply_disposal(dec, &dec->previous_frame);
                dec->have_previous_frame = 0u;
            }

            palette = 0;
            palette_entries = 0u;
            rc = giff_parse_image_descriptor(&p, dec->parse_end, dec, &dec->frame, &palette, &palette_entries);
            if (rc != GIFF_OK) {
                if (rc == GIFF_E_TRUNCATED && dec->source_mode == 3u && !dec->stream_eof) {
                    return GIFF_E_NEED_MORE_INPUT;
                }
                out_event->type = GIFF_EVENT_ERROR;
                out_event->code = rc;
                dec->last_result = (giff_u8)(-rc);
                return rc;
            }

            if (dec->frame.disposal_method == 3u) {
                giff_snapshot_previous(dec, &dec->frame);
            }

            rc = giff_decode_image_data(dec, &p, dec->parse_end, &dec->frame, palette, palette_entries);
            if (rc != GIFF_OK) {
                if (rc == GIFF_E_TRUNCATED && dec->source_mode == 3u && !dec->stream_eof) {
                    return GIFF_E_NEED_MORE_INPUT;
                }
                out_event->type = GIFF_EVENT_ERROR;
                out_event->code = rc;
                dec->last_result = (giff_u8)(-rc);
                return rc;
            }

            dec->previous_frame = dec->frame;
            dec->have_previous_frame = 1u;
            dec->frames_seen += 1u;
            dec->emit_row_cursor = 0u;
            dec->emit_row_count = dec->frame.height;
            dec->event_phase = 1u;
            dec->parse_ptr = p;
            giff_decoder_sync_offset(dec);
            giff_decoder_clear_pending_gce(dec);

            giff_event_fill(dec, out_event, GIFF_EVENT_FRAME_BEGIN);
            dec->events_emitted += 1u;
            return GIFF_OK;
        }

        out_event->type = GIFF_EVENT_ERROR;
        out_event->code = GIFF_E_BAD_BLOCK;
        dec->last_result = (giff_u8)(-GIFF_E_BAD_BLOCK);
        return GIFF_E_BAD_BLOCK;
    }

    if (dec->source_mode == 3u && !dec->stream_eof) {
        return GIFF_E_NEED_MORE_INPUT;
    }

    out_event->type = GIFF_EVENT_ERROR;
    out_event->code = GIFF_E_TRUNCATED;
    dec->last_result = (giff_u8)(-GIFF_E_TRUNCATED);
    return GIFF_E_TRUNCATED;
}

giff_result giff_decoder_decode_memory(
    giff_decoder* dec,
    const giff_u8* data,
    giff_u32 size
)
{
    giff_result rc;
    giff_event ev;

    rc = giff_decoder_begin_memory(dec, data, size);
    if (rc != GIFF_OK) {
        return rc;
    }

    while (1) {
        rc = giff_decoder_next_event(dec, &ev);
        if (rc != GIFF_OK) {
            return rc;
        }
        if (ev.type == GIFF_EVENT_TRAILER) {
            dec->last_result = (giff_u8)GIFF_OK;
            return GIFF_OK;
        }
    }
}

giff_result giff_decoder_decode_io(giff_decoder* dec)
{
    giff_result rc;
    giff_event ev;

    rc = giff_decoder_begin_io(dec);
    if (rc != GIFF_OK) {
        return rc;
    }

    while (1) {
        rc = giff_decoder_next_event(dec, &ev);
        if (rc != GIFF_OK) {
            return rc;
        }
        if (ev.type == GIFF_EVENT_TRAILER) {
            dec->last_result = (giff_u8)GIFF_OK;
            return GIFF_OK;
        }
    }
}
