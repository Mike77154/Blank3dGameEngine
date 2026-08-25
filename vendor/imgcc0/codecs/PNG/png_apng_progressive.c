#include "png_mem89.h"
#include "png_decoder_internal.h"

#include <string.h>

typedef struct apng_prog_raw_chunk_blob_s {
    png_u8* bytes; /* length+type+data+crc */
    png_u32 size;
    png_u32 type;
} apng_prog_raw_chunk_blob;

typedef struct apng_prog_frame_build_s {
    png_apng_frame_control control;
    png_u8* compressed;
    png_u32 compressed_size;
    png_u32 compressed_capacity;
    int uses_idat;
} apng_prog_frame_build;

struct png_apng_decoder_s {
    png_zlib_decompress_func zfunc;
    png_decode_options options;
    int options_set;
    png_apng_progressive_callbacks callbacks;
    png_progressive_control prog_ctl;
    png_u32 work_parse_bytes;
    png_u32 work_row_callbacks;
    png_u32 work_chunk_callbacks;
    png_u32 work_chunk_bytes;
    png_u32 work_zlib_bytes;
    png_u32 work_zlib_steps;
    int yield_requested;

    png_u8* buffer;
    png_u32 size;
    png_u32 capacity;
    png_u32 parse_pos;
    png_u32 buffer_file_offset;

    int pause_requested;
    int paused;
    int pause_save;
    png_u32 pause_unprocessed_bytes;
    png_u32 pause_resume_offset;
    png_u32 feed_start_offset;
    png_u32 feed_end_offset;
    int feed_active;
    int active_chunk_waiting_resume;
    png_u32 active_chunk_start;
    png_u32 active_chunk_next_pos;
    png_u32 active_chunk_type;

    int sig_checked;
    int parse_done;
    int info_emitted;

    int seen_IHDR;
    int seen_acTL;
    int seen_first_IDAT;
    int seen_IEND;
    int default_image_is_first_frame;

    png_u32 width;
    png_u32 height;
    png_u8  bit_depth;
    png_u8  color_type;
    png_u8  compression_method;
    png_u8  filter_method;
    png_u8  interlace_method;
    png_u32 num_frames_declared;
    png_u32 num_plays;
    png_u32 next_sequence_number;
    png_u32 chunk_count;

    apng_prog_raw_chunk_blob* pre_chunks;
    png_u32 pre_chunk_count;
    png_u32 pre_chunk_capacity;

    apng_prog_frame_build current_frame;
    int current_frame_open;

    png_apng animation;
    png_u32 animation_frame_capacity;

    png_u8* canvas;
    png_u8* frame_canvas;
    png_u32 internal_rowbytes;
    png_u32 canvas_size;
    png_u32 pixel_bytes;
    int internal_16;

    png_decoder* frame_decoder;
    png_u8* frame_row_emitted_flags;
    int frame_info_emitted;
    int frame_live_rows;
};

static const png_u8 APNG_PROG_SIG[8] = { 137u, 80u, 78u, 71u, 13u, 10u, 26u, 10u };

static png_u32 apng_prog_read_be32(const png_u8* p)
{
    return ((png_u32)p[0] << 24) |
           ((png_u32)p[1] << 16) |
           ((png_u32)p[2] << 8) |
           (png_u32)p[3];
}

static png_u16 apng_prog_read_be16(const png_u8* p)
{
    return (png_u16)(((png_u16)p[0] << 8) | (png_u16)p[1]);
}

static void apng_prog_write_be32(png_u8* p, png_u32 v)
{
    p[0] = (png_u8)(v >> 24);
    p[1] = (png_u8)(v >> 16);
    p[2] = (png_u8)(v >> 8);
    p[3] = (png_u8)(v & 255u);
}

static int apng_prog_safe_add_u32(png_u32 a, png_u32 b, png_u32* out);

static png_u32 apng_prog_compute_unprocessed_bytes(const png_apng_decoder* dec, png_u32 unread_abs)
{
    if (!dec)
        return 0u;
    if (dec->feed_active)
    {
        if (unread_abs <= dec->feed_start_offset)
            return dec->feed_end_offset - dec->feed_start_offset;
        if (unread_abs < dec->feed_end_offset)
            return dec->feed_end_offset - unread_abs;
        return 0u;
    }
    if (unread_abs >= dec->size)
        return 0u;
    return dec->size - unread_abs;
}

static int apng_prog_pause_here(png_apng_decoder* dec, png_u32 unread_abs)
{
    if (!dec)
        return PNG_DEC_ERR_FORMAT;
    if (unread_abs > dec->size)
        unread_abs = dec->size;
    dec->paused = 1;
    dec->pause_resume_offset = unread_abs;
    dec->pause_unprocessed_bytes = apng_prog_compute_unprocessed_bytes(dec, unread_abs);
    dec->pause_requested = 0;
    dec->pause_save = 1;
    return PNG_DEC_PAUSED;
}

static png_u32 apng_prog_current_unread_offset(const png_apng_decoder* dec)
{
    if (!dec)
        return 0u;
    if (dec->active_chunk_waiting_resume)
    {
        if (dec->active_chunk_start > dec->size)
            return dec->size;
        return dec->active_chunk_start;
    }
    if (dec->parse_pos > dec->size)
        return dec->size;
    return dec->parse_pos;
}

png_u32 png_apng_decoder_unprocessed_bytes(const png_apng_decoder* dec)
{
    if (!dec)
        return 0u;
    if (dec->paused && dec->pause_resume_offset < dec->size)
        return dec->size - dec->pause_resume_offset;
    return apng_prog_compute_unprocessed_bytes(dec, apng_prog_current_unread_offset(dec));
}

png_u32 png_apng_decoder_input_room(const png_apng_decoder* dec)
{
    png_u32 unread;
    if (!dec)
        return 0u;
    if (dec->prog_ctl.max_buffered_bytes == 0u)
        return 0xFFFFFFFFu;
    unread = png_apng_decoder_unprocessed_bytes(dec);
    if (unread >= dec->prog_ctl.max_buffered_bytes)
        return 0u;
    return dec->prog_ctl.max_buffered_bytes - unread;
}

static png_u32 apng_prog_suggest_read_bytes(const png_apng_decoder* dec, png_u32 room)
{
    png_u32 suggested;

    if (!dec || room == 0u)
        return 0u;
    suggested = room;
    if (suggested == 0xFFFFFFFFu)
        suggested = dec->prog_ctl.max_feed_bytes ? dec->prog_ctl.max_feed_bytes : 65536u;
    else if (dec->prog_ctl.max_feed_bytes != 0u && suggested > dec->prog_ctl.max_feed_bytes)
        suggested = dec->prog_ctl.max_feed_bytes;
    if (suggested == 0u)
        suggested = 1u;
    return suggested;
}

static int apng_prog_can_drain_no_input(const png_apng_decoder* dec)
{
    png_u32 length;
    png_u32 next_pos;

    if (!dec || dec->parse_done)
        return 0;
    if (dec->active_chunk_waiting_resume)
        return 1;
    if (dec->seen_IEND && dec->parse_pos == dec->size)
        return 1;
    if (!dec->sig_checked)
        return dec->size >= 8u;
    if (dec->parse_pos + 8u > dec->size)
        return 0;
    length = apng_prog_read_be32(dec->buffer + dec->parse_pos + 0u);
    if (length > 0x7FFFFFFFu)
        return 1;
    if (!apng_prog_safe_add_u32(dec->parse_pos, 12u + length, &next_pos))
        return 1;
    return next_pos <= dec->size;
}

static void apng_prog_fill_poll_state(const png_apng_decoder* dec, png_progressive_poll_state* st)
{
    png_u32 room;
    png_u32 unread;
    png_u32 pending;

    if (!st)
        return;

    png_progressive_poll_state_init(st);
    if (!dec)
        return;

    room = png_apng_decoder_input_room(dec);
    unread = png_apng_decoder_unprocessed_bytes(dec);
    pending = png_apng_decoder_pending_bytes(dec);
    st->input_room = room;
    st->unprocessed_bytes = unread;
    st->pending_bytes = pending;

    if (dec->parse_done)
    {
        st->events |= PNG_PROGRESSIVE_POLL_DONE;
        return;
    }

    if (unread != 0u || pending != 0u)
        st->events |= PNG_PROGRESSIVE_POLL_HAVE_BUFFERED;

    if (dec->paused)
    {
        st->events |= PNG_PROGRESSIVE_POLL_PAUSED;
        if (dec->pause_save && pending != 0u)
            st->events |= PNG_PROGRESSIVE_POLL_CAN_DRAIN;
        else if (pending != 0u)
            st->events |= PNG_PROGRESSIVE_POLL_WANT_INPUT;
    }
    else if (apng_prog_can_drain_no_input(dec))
        st->events |= PNG_PROGRESSIVE_POLL_CAN_DRAIN;

    if (room != 0u && !dec->parse_done && (!dec->paused || (pending != 0u && !dec->pause_save) || !apng_prog_can_drain_no_input(dec)))
        st->events |= PNG_PROGRESSIVE_POLL_WANT_INPUT;

    if ((st->events & PNG_PROGRESSIVE_POLL_WANT_INPUT) != 0u)
        st->suggested_read_bytes = apng_prog_suggest_read_bytes(dec, room);
}

int png_apng_decoder_poll(const png_apng_decoder* dec, png_progressive_poll_state* st)
{
    apng_prog_fill_poll_state(dec, st);
    return dec ? PNG_DEC_OK : PNG_DEC_ERR_FORMAT;
}

static void apng_prog_work_reset(png_apng_decoder* dec)
{
    if (!dec)
        return;
    dec->work_parse_bytes = 0u;
    dec->work_row_callbacks = 0u;
    dec->work_chunk_callbacks = 0u;
    dec->work_chunk_bytes = 0u;
    dec->work_zlib_bytes = 0u;
    dec->work_zlib_steps = 0u;
    dec->yield_requested = 0;
}

static void apng_prog_fill_chunk_progress_info(png_chunk_progress_info* info,
                                               png_u32 type,
                                               png_u32 length,
                                               png_u32 file_offset)
{
    png_u8 c0;
    png_u8 c1;
    png_u8 c2;
    png_u8 c3;

    if (!info)
        return;

    memset(info, 0, sizeof(*info));
    info->type = type;
    info->length = length;
    info->file_offset = file_offset;
    info->data_offset = file_offset + 8u;
    info->total_size = length + 12u;
    c0 = (png_u8)(type >> 24);
    c1 = (png_u8)(type >> 16);
    c2 = (png_u8)(type >> 8);
    c3 = (png_u8)(type);
    info->ancillary = (png_u8)((c0 & 0x20u) ? 1u : 0u);
    info->private_bit = (png_u8)((c1 & 0x20u) ? 1u : 0u);
    info->reserved_bit = (png_u8)((c2 & 0x20u) ? 1u : 0u);
    info->safe_to_copy = (png_u8)((c3 & 0x20u) ? 1u : 0u);
}

static int apng_prog_budget_hit_result(const png_apng_decoder* dec)
{
    if (dec && dec->prog_ctl.hard_fail_on_budget_exhaustion)
        return PNG_DEC_ERR_WORK_BUDGET;
    return PNG_DEC_YIELDED;
}

static int apng_prog_note_parse_bytes(png_apng_decoder* dec, png_u32 bytes)
{
    if (!dec || bytes == 0u)
        return PNG_DEC_OK;
    if (0xFFFFFFFFu - dec->work_parse_bytes < bytes)
        dec->work_parse_bytes = 0xFFFFFFFFu;
    else
        dec->work_parse_bytes += bytes;
    if (dec->prog_ctl.max_parse_bytes_per_call != 0u &&
        dec->work_parse_bytes >= dec->prog_ctl.max_parse_bytes_per_call)
        return apng_prog_budget_hit_result(dec);
    return PNG_DEC_OK;
}

static int apng_prog_note_row_callback(png_apng_decoder* dec)
{
    if (!dec)
        return PNG_DEC_OK;
    if (dec->work_row_callbacks != 0xFFFFFFFFu)
        dec->work_row_callbacks += 1u;
    if (dec->prog_ctl.max_row_callbacks_per_call != 0u &&
        dec->work_row_callbacks >= dec->prog_ctl.max_row_callbacks_per_call)
        return apng_prog_budget_hit_result(dec);
    return PNG_DEC_OK;
}

static int apng_prog_note_chunk_budget(png_apng_decoder* dec, png_u32 chunk_total_size, int chunk_callback_emitted)
{
    if (!dec)
        return PNG_DEC_OK;

    if (chunk_callback_emitted && dec->work_chunk_callbacks != 0xFFFFFFFFu)
        dec->work_chunk_callbacks += 1u;

    if (chunk_total_size != 0u)
    {
        if (0xFFFFFFFFu - dec->work_chunk_bytes < chunk_total_size)
            dec->work_chunk_bytes = 0xFFFFFFFFu;
        else
            dec->work_chunk_bytes += chunk_total_size;
    }

    if (dec->prog_ctl.max_chunk_callbacks_per_call != 0u &&
        dec->work_chunk_callbacks >= dec->prog_ctl.max_chunk_callbacks_per_call)
        return apng_prog_budget_hit_result(dec);

    if (dec->prog_ctl.max_chunk_bytes_per_call != 0u &&
        dec->work_chunk_bytes >= dec->prog_ctl.max_chunk_bytes_per_call)
        return apng_prog_budget_hit_result(dec);

    return PNG_DEC_OK;
}

static int apng_prog_emit_chunk_callback(png_apng_decoder* dec, png_u32 type, png_u32 length, png_u32 file_offset)
{
    png_chunk_progress_info info;
    int emitted = 0;
    int err;
    png_i32 frame_index = -1;
    png_u32 absolute_offset = file_offset;

    if (!dec)
        return PNG_DEC_ERR_FORMAT;

    if (0xFFFFFFFFu - dec->buffer_file_offset < file_offset)
        absolute_offset = 0xFFFFFFFFu;
    else
        absolute_offset = dec->buffer_file_offset + file_offset;

    if (dec->current_frame_open)
        frame_index = (png_i32)dec->animation.frame_count;

    if (dec->callbacks.chunk_fn)
    {
        apng_prog_fill_chunk_progress_info(&info, type, length, absolute_offset);
        dec->callbacks.chunk_fn(dec->callbacks.user_ptr, &info, frame_index);
        emitted = 1;
    }

    if (dec->pause_requested)
        return PNG_DEC_PAUSED;

    err = apng_prog_note_chunk_budget(dec, length + 12u, emitted);
    if (err != PNG_DEC_OK)
        return err;

    return PNG_DEC_OK;
}

static void apng_prog_drop_prefix(png_apng_decoder* dec, png_u32 drop)
{
    if (!dec || drop == 0u)
        return;
    if (drop > dec->size)
        drop = dec->size;
    if (drop < dec->size)
        memmove(dec->buffer, dec->buffer + drop, dec->size - drop);
    dec->size -= drop;
    if (0xFFFFFFFFu - dec->buffer_file_offset < drop)
        dec->buffer_file_offset = 0xFFFFFFFFu;
    else
        dec->buffer_file_offset += drop;
    if (dec->parse_pos >= drop) dec->parse_pos -= drop; else dec->parse_pos = 0u;
    if (dec->pause_resume_offset >= drop) dec->pause_resume_offset -= drop; else dec->pause_resume_offset = 0u;
    if (dec->feed_start_offset >= drop) dec->feed_start_offset -= drop; else dec->feed_start_offset = 0u;
    if (dec->feed_end_offset >= drop) dec->feed_end_offset -= drop; else dec->feed_end_offset = dec->size;
    if (dec->active_chunk_start >= drop) dec->active_chunk_start -= drop; else dec->active_chunk_start = 0u;
    if (dec->active_chunk_next_pos >= drop) dec->active_chunk_next_pos -= drop; else dec->active_chunk_next_pos = 0u;
}

static void apng_prog_maybe_compact_buffer(png_apng_decoder* dec)
{
    png_u32 unread_abs;
    png_u32 unread;
    if (!dec || !dec->buffer || dec->size == 0u)
        return;
    unread_abs = dec->paused ? dec->pause_resume_offset : apng_prog_current_unread_offset(dec);
    if (unread_abs == 0u || unread_abs > dec->size)
        return;
    unread = dec->size - unread_abs;
    if (unread_abs >= 65536u || unread_abs >= unread)
        apng_prog_drop_prefix(dec, unread_abs);
}

static int apng_prog_safe_add_u32(png_u32 a, png_u32 b, png_u32* out)
{
    if (a > 0xFFFFFFFFu - b)
        return 0;
    *out = a + b;
    return 1;
}

static int apng_prog_safe_mul_u32(png_u32 a, png_u32 b, png_u32* out)
{
    if (a != 0u && b > 0xFFFFFFFFu / a)
        return 0;
    *out = a * b;
    return 1;
}

static int apng_prog_chunk_is_type(png_u32 type, char a, char b, char c, char d)
{
    return type == (((png_u32)(png_u8)a << 24) |
                    ((png_u32)(png_u8)b << 16) |
                    ((png_u32)(png_u8)c << 8) |
                    (png_u32)(png_u8)d);
}

static png_u32 apng_prog_limit_or_default(png_u32 value, png_u32 fallback)
{
    return value != 0u ? value : fallback;
}

static int apng_prog_buffer_append(png_u8** buf, png_u32* size, png_u32* cap,
                                   const png_u8* data, png_u32 data_size)
{
    png_u32 need;
    png_u32 new_cap;
    png_u8* new_buf;

    if (data_size == 0u)
        return PNG_DEC_OK;
    if (!buf || !size || !cap || (!data && data_size != 0u))
        return PNG_DEC_ERR_FORMAT;
    if (!apng_prog_safe_add_u32(*size, data_size, &need))
        return PNG_DEC_ERR_UNSUPPORTED;
    if (need > *cap)
    {
        new_cap = (*cap == 0u) ? 256u : *cap;
        while (new_cap < need)
        {
            if (new_cap > 0x7FFFFFFFu)
                return PNG_DEC_ERR_UNSUPPORTED;
            new_cap *= 2u;
        }
        new_buf = (png_u8*)png_mem89_resize(*buf, new_cap);
        if (!new_buf)
            return PNG_DEC_ERR_OOM;
        *buf = new_buf;
        *cap = new_cap;
    }
    memcpy(*buf + *size, data, data_size);
    *size = need;
    return PNG_DEC_OK;
}

static int apng_prog_raw_chunk_push(apng_prog_raw_chunk_blob** arr,
                                    png_u32* count,
                                    png_u32* cap,
                                    const png_u8* data,
                                    png_u32 size,
                                    png_u32 type)
{
    png_u32 new_cap;
    apng_prog_raw_chunk_blob* new_arr;
    apng_prog_raw_chunk_blob* dst;

    if (!arr || !count || !cap || (!data && size != 0u))
        return PNG_DEC_ERR_FORMAT;

    if (*count == *cap)
    {
        new_cap = (*cap == 0u) ? 8u : (*cap * 2u);
        new_arr = (apng_prog_raw_chunk_blob*)png_mem89_resize(*arr, new_cap * sizeof(apng_prog_raw_chunk_blob));
        if (!new_arr)
            return PNG_DEC_ERR_OOM;
        *arr = new_arr;
        *cap = new_cap;
    }

    dst = &(*arr)[*count];
    memset(dst, 0, sizeof(*dst));
    dst->bytes = (png_u8*)png_mem89_alloc(size == 0u ? 1u : size);
    if (!dst->bytes)
        return PNG_DEC_ERR_OOM;
    memcpy(dst->bytes, data, size);
    dst->size = size;
    dst->type = type;
    *count += 1u;
    return PNG_DEC_OK;
}

static void apng_prog_raw_chunks_free(apng_prog_raw_chunk_blob* arr, png_u32 count)
{
    png_u32 i;
    if (!arr)
        return;
    for (i = 0u; i < count; ++i)
        png_mem89_release(arr[i].bytes);
    png_mem89_release(arr);
}

static void apng_prog_frame_build_reset(apng_prog_frame_build* frame)
{
    if (!frame)
        return;
    png_mem89_release(frame->compressed);
    memset(frame, 0, sizeof(*frame));
}

static void apng_prog_decoder_release_buffers(png_apng_decoder* dec)
{
    if (!dec)
        return;
    png_mem89_release(dec->buffer);
    dec->buffer = 0;
    dec->size = 0u;
    dec->capacity = 0u;
    apng_prog_raw_chunks_free(dec->pre_chunks, dec->pre_chunk_count);
    dec->pre_chunks = 0;
    dec->pre_chunk_count = 0u;
    dec->pre_chunk_capacity = 0u;
    apng_prog_frame_build_reset(&dec->current_frame);
    dec->current_frame_open = 0;
    if (dec->frame_decoder)
        png_decoder_free(dec->frame_decoder);
    dec->frame_decoder = 0;
    png_mem89_release(dec->canvas);
    png_mem89_release(dec->frame_canvas);
    png_mem89_release(dec->frame_row_emitted_flags);
    dec->canvas = 0;
    dec->frame_canvas = 0;
    dec->frame_row_emitted_flags = 0;
    dec->internal_rowbytes = 0u;
    dec->canvas_size = 0u;
    dec->pixel_bytes = 0u;
    dec->internal_16 = 0;
    dec->frame_info_emitted = 0;
    dec->frame_live_rows = 0;
}

static int apng_prog_validate_crc(const png_u8* type_and_data, png_u32 length, const png_u8* crc_bytes)
{
    png_u32 crc = png_crc32_start();
    png_u32 stored;
    crc = png_crc32_update(crc, type_and_data, 4u + length);
    crc = png_crc32_finish(crc);
    stored = apng_prog_read_be32(crc_bytes);
    return (crc == stored) ? PNG_DEC_OK : PNG_DEC_ERR_CRC;
}

static int apng_prog_append_chunk_bytes(png_u8** out,
                                        png_u32* out_size,
                                        png_u32* out_cap,
                                        const png_u8 type[4],
                                        const png_u8* data,
                                        png_u32 data_size)
{
    png_u8 header[8];
    png_u8 crc_buf[4];
    png_u32 crc;
    int err;

    if (!out || !out_size || !out_cap || !type || (!data && data_size != 0u))
        return PNG_DEC_ERR_FORMAT;

    apng_prog_write_be32(header, data_size);
    memcpy(header + 4u, type, 4u);
    err = apng_prog_buffer_append(out, out_size, out_cap, header, 8u);
    if (err != PNG_DEC_OK)
        return err;
    err = apng_prog_buffer_append(out, out_size, out_cap, data, data_size);
    if (err != PNG_DEC_OK)
        return err;

    crc = png_crc32_start();
    crc = png_crc32_update(crc, type, 4u);
    if (data_size != 0u)
        crc = png_crc32_update(crc, data, data_size);
    crc = png_crc32_finish(crc);
    apng_prog_write_be32(crc_buf, crc);
    return apng_prog_buffer_append(out, out_size, out_cap, crc_buf, 4u);
}

static int apng_prog_build_frame_png(const png_apng_decoder* dec,
                                     const apng_prog_frame_build* frame,
                                     png_u8** out_png,
                                     png_u32* out_png_size)
{
    png_u8* out;
    png_u32 out_size;
    png_u32 out_cap;
    png_u8 ihdr[13];
    png_u32 i;
    int err;
    static const png_u8 ihdr_type[4] = { 'I', 'H', 'D', 'R' };
    static const png_u8 idat_type[4] = { 'I', 'D', 'A', 'T' };
    static const png_u8 iend_type[4] = { 'I', 'E', 'N', 'D' };

    if (!dec || !frame || !out_png || !out_png_size)
        return PNG_DEC_ERR_FORMAT;

    *out_png = 0;
    *out_png_size = 0u;
    out = 0;
    out_size = 0u;
    out_cap = 0u;

    err = apng_prog_buffer_append(&out, &out_size, &out_cap, APNG_PROG_SIG, 8u);
    if (err != PNG_DEC_OK)
        goto fail;

    apng_prog_write_be32(ihdr + 0u, frame->control.width);
    apng_prog_write_be32(ihdr + 4u, frame->control.height);
    ihdr[8] = dec->bit_depth;
    ihdr[9] = dec->color_type;
    ihdr[10] = dec->compression_method;
    ihdr[11] = dec->filter_method;
    ihdr[12] = dec->interlace_method;
    err = apng_prog_append_chunk_bytes(&out, &out_size, &out_cap, ihdr_type, ihdr, 13u);
    if (err != PNG_DEC_OK)
        goto fail;

    for (i = 0u; i < dec->pre_chunk_count; ++i)
    {
        err = apng_prog_buffer_append(&out, &out_size, &out_cap,
                                      dec->pre_chunks[i].bytes,
                                      dec->pre_chunks[i].size);
        if (err != PNG_DEC_OK)
            goto fail;
    }

    err = apng_prog_append_chunk_bytes(&out, &out_size, &out_cap,
                                       idat_type,
                                       frame->compressed,
                                       frame->compressed_size);
    if (err != PNG_DEC_OK)
        goto fail;
    err = apng_prog_append_chunk_bytes(&out, &out_size, &out_cap, iend_type, 0, 0u);
    if (err != PNG_DEC_OK)
        goto fail;

    *out_png = out;
    *out_png_size = out_size;
    return PNG_DEC_OK;

fail:
    png_mem89_release(out);
    return err;
}

static void apng_prog_apply_post_transforms_rgba8(png_u8* p, png_u32 count, png_u32 flags)
{
    png_u32 i;
    for (i = 0u; i < count; ++i)
    {
        png_u8 r = p[0];
        png_u8 g = p[1];
        png_u8 b = p[2];
        png_u8 a = p[3];
        if (flags & PNG_DEC_TRANSFORM_PREMULTIPLY_ALPHA)
        {
            r = (png_u8)(((png_u32)r * (png_u32)a + 127u) / 255u);
            g = (png_u8)(((png_u32)g * (png_u32)a + 127u) / 255u);
            b = (png_u8)(((png_u32)b * (png_u32)a + 127u) / 255u);
        }
        if (flags & PNG_DEC_TRANSFORM_SWAP_RB)
        {
            png_u8 t = r; r = b; b = t;
        }
        if (flags & PNG_DEC_TRANSFORM_INVERT_ALPHA)
            a = (png_u8)(255u - a);
        if (flags & PNG_DEC_TRANSFORM_STRIP_ALPHA)
            a = 255u;
        p[0] = r; p[1] = g; p[2] = b; p[3] = a;
        p += 4;
    }
}

static void apng_prog_apply_post_transforms_rgba16(png_u8* p, png_u32 count, png_u32 flags)
{
    png_u32 i;
    for (i = 0u; i < count; ++i)
    {
        png_u16 r = apng_prog_read_be16(p + 0u);
        png_u16 g = apng_prog_read_be16(p + 2u);
        png_u16 b = apng_prog_read_be16(p + 4u);
        png_u16 a = apng_prog_read_be16(p + 6u);
        if (flags & PNG_DEC_TRANSFORM_PREMULTIPLY_ALPHA)
        {
            r = (png_u16)(((png_u32)r * (png_u32)a + 32767u) / 65535u);
            g = (png_u16)(((png_u32)g * (png_u32)a + 32767u) / 65535u);
            b = (png_u16)(((png_u32)b * (png_u32)a + 32767u) / 65535u);
        }
        if (flags & PNG_DEC_TRANSFORM_SWAP_RB)
        {
            png_u16 t = r; r = b; b = t;
        }
        if (flags & PNG_DEC_TRANSFORM_INVERT_ALPHA)
            a = (png_u16)(65535u - a);
        if (flags & PNG_DEC_TRANSFORM_STRIP_ALPHA)
            a = 65535u;
        p[0] = (png_u8)(r >> 8); p[1] = (png_u8)(r & 255u);
        p[2] = (png_u8)(g >> 8); p[3] = (png_u8)(g & 255u);
        p[4] = (png_u8)(b >> 8); p[5] = (png_u8)(b & 255u);
        p[6] = (png_u8)(a >> 8); p[7] = (png_u8)(a & 255u);
        p += 8;
    }
}

static void apng_prog_clear_region_rgba8(png_u8* canvas, png_u32 canvas_rowbytes,
                                         png_u32 x, png_u32 y, png_u32 w, png_u32 h)
{
    png_u32 row;
    for (row = 0u; row < h; ++row)
        memset(canvas + (y + row) * canvas_rowbytes + x * 4u, 0, (w * 4u));
}

static void apng_prog_clear_region_rgba16(png_u8* canvas, png_u32 canvas_rowbytes,
                                          png_u32 x, png_u32 y, png_u32 w, png_u32 h)
{
    png_u32 row;
    for (row = 0u; row < h; ++row)
        memset(canvas + (y + row) * canvas_rowbytes + x * 8u, 0, (w * 8u));
}

static void apng_prog_blend_over_rgba8_pixel(png_u8* dst, const png_u8* src)
{
    png_u32 sa = src[3];
    png_u32 da = dst[3];
    png_u32 out_a = sa + (da * (255u - sa) + 127u) / 255u;
    png_u32 i;
    if (out_a == 0u)
    {
        dst[0] = dst[1] = dst[2] = dst[3] = 0u;
        return;
    }
    for (i = 0u; i < 3u; ++i)
    {
        png_u32 src_p = (png_u32)src[i] * sa;
        png_u32 dst_p = ((png_u32)dst[i] * da * (255u - sa) + 127u) / 255u;
        dst[i] = (png_u8)((src_p + dst_p + out_a / 2u) / out_a);
    }
    dst[3] = (png_u8)out_a;
}

static void apng_prog_blend_over_rgba16_pixel(png_u8* dst, const png_u8* src)
{
    png_u32 sa = apng_prog_read_be16(src + 6u);
    png_u32 da = apng_prog_read_be16(dst + 6u);
    png_u32 out_a = sa + (da * (65535u - sa) + 32767u) / 65535u;
    png_u32 src_c, dst_c;
    png_u16 out_c;
    if (out_a == 0u)
    {
        memset(dst, 0, 8u);
        return;
    }
    src_c = (png_u32)apng_prog_read_be16(src + 0u) * sa;
    dst_c = ((png_u32)apng_prog_read_be16(dst + 0u) * da * (65535u - sa) + 32767u) / 65535u;
    out_c = (png_u16)((src_c + dst_c + out_a / 2u) / out_a);
    dst[0] = (png_u8)(out_c >> 8); dst[1] = (png_u8)(out_c & 255u);
    src_c = (png_u32)apng_prog_read_be16(src + 2u) * sa;
    dst_c = ((png_u32)apng_prog_read_be16(dst + 2u) * da * (65535u - sa) + 32767u) / 65535u;
    out_c = (png_u16)((src_c + dst_c + out_a / 2u) / out_a);
    dst[2] = (png_u8)(out_c >> 8); dst[3] = (png_u8)(out_c & 255u);
    src_c = (png_u32)apng_prog_read_be16(src + 4u) * sa;
    dst_c = ((png_u32)apng_prog_read_be16(dst + 4u) * da * (65535u - sa) + 32767u) / 65535u;
    out_c = (png_u16)((src_c + dst_c + out_a / 2u) / out_a);
    dst[4] = (png_u8)(out_c >> 8); dst[5] = (png_u8)(out_c & 255u);
    out_c = (png_u16)out_a;
    dst[6] = (png_u8)(out_c >> 8); dst[7] = (png_u8)(out_c & 255u);
}

static void apng_prog_composite_frame_rgba8(png_u8* canvas, png_u32 canvas_rowbytes,
                                            const png_apng_frame_control* ctl,
                                            const png_u8* frame, png_u32 frame_rowbytes)
{
    png_u32 row, col;
    for (row = 0u; row < ctl->height; ++row)
    {
        png_u8* dst = canvas + (ctl->y_offset + row) * canvas_rowbytes + ctl->x_offset * 4u;
        const png_u8* src = frame + row * frame_rowbytes;
        if (ctl->blend_op == PNG_APNG_BLEND_OP_SOURCE)
            memcpy(dst, src, (ctl->width * 4u));
        else
            for (col = 0u; col < ctl->width; ++col)
                apng_prog_blend_over_rgba8_pixel(dst + col * 4u, src + col * 4u);
    }
}

static void apng_prog_composite_frame_rgba16(png_u8* canvas, png_u32 canvas_rowbytes,
                                             const png_apng_frame_control* ctl,
                                             const png_u8* frame, png_u32 frame_rowbytes)
{
    png_u32 row, col;
    for (row = 0u; row < ctl->height; ++row)
    {
        png_u8* dst = canvas + (ctl->y_offset + row) * canvas_rowbytes + ctl->x_offset * 8u;
        const png_u8* src = frame + row * frame_rowbytes;
        if (ctl->blend_op == PNG_APNG_BLEND_OP_SOURCE)
            memcpy(dst, src, (ctl->width * 8u));
        else
            for (col = 0u; col < ctl->width; ++col)
                apng_prog_blend_over_rgba16_pixel(dst + col * 8u, src + col * 8u);
    }
}

static int apng_prog_canvas_to_public_frame(const png_u8* canvas,
                                            png_u32 canvas_rowbytes,
                                            png_u32 width,
                                            png_u32 height,
                                            int internal_16,
                                            png_u32 post_flags,
                                            png_u8 output_format,
                                            png_apng_frame* out_frame)
{
    png_image tmp;
    png_u32 size;
    int err;

    if (!canvas || !out_frame)
        return PNG_DEC_ERR_FORMAT;

    memset(&tmp, 0, sizeof(tmp));
    memset(out_frame, 0, sizeof(*out_frame));
    if (!apng_prog_safe_mul_u32(canvas_rowbytes, height, &size))
        return PNG_DEC_ERR_UNSUPPORTED;
    tmp.pixels = (png_u8*)png_mem89_alloc(size == 0u ? 1u : size);
    if (!tmp.pixels)
        return PNG_DEC_ERR_OOM;
    memcpy(tmp.pixels, canvas, size);
    tmp.width = width;
    tmp.height = height;
    tmp.pixel_rowbytes = canvas_rowbytes;
    tmp.output_format = internal_16 ? PNG_OUTPUT_RGBA16 : PNG_OUTPUT_RGBA8;
    tmp.output_channels = 4u;
    tmp.output_sample_depth = internal_16 ? 16u : 8u;
    tmp.output_bytes_per_channel = internal_16 ? 2u : 1u;
    tmp.rgba = internal_16 ? 0 : tmp.pixels;

    if (internal_16)
        apng_prog_apply_post_transforms_rgba16(tmp.pixels, width * height, post_flags);
    else
        apng_prog_apply_post_transforms_rgba8(tmp.pixels, width * height, post_flags);

    if (output_format != tmp.output_format)
    {
        err = png_image_convert_format(&tmp, output_format);
        if (err != PNG_DEC_OK)
        {
            png_free_image(&tmp);
            return err;
        }
    }

    if ((post_flags & PNG_DEC_TRANSFORM_SWAP_16_ENDIAN) &&
        tmp.output_sample_depth == 16u && tmp.pixels)
    {
        png_u32 swap_size;
        if (!apng_prog_safe_mul_u32(tmp.pixel_rowbytes, tmp.height, &swap_size))
        {
            png_free_image(&tmp);
            return PNG_DEC_ERR_UNSUPPORTED;
        }
        png_swap_16_buffer(tmp.pixels, swap_size);
    }

    out_frame->pixels = tmp.pixels;
    out_frame->pixel_rowbytes = tmp.pixel_rowbytes;
    out_frame->output_format = tmp.output_format;
    out_frame->output_channels = tmp.output_channels;
    out_frame->output_sample_depth = tmp.output_sample_depth;
    out_frame->output_bytes_per_channel = tmp.output_bytes_per_channel;
    tmp.pixels = 0;
    tmp.rgba = 0;
    png_free_image(&tmp);
    return PNG_DEC_OK;
}

static int apng_prog_decoder_reserve(png_apng_decoder* dec, png_u32 extra)
{
    png_u32 need;
    png_u32 new_cap;
    png_u8* new_buf;
    png_u32 max_file_bytes;

    if (!dec)
        return PNG_DEC_ERR_FORMAT;
    if (!apng_prog_safe_add_u32(dec->size, extra, &need))
        return PNG_DEC_ERR_CHUNK_TOO_LARGE;

    max_file_bytes = apng_prog_limit_or_default(dec->options.max_file_bytes, PNG_DEC_MAX_FILE_BYTES);
    if (need > max_file_bytes)
        return PNG_DEC_ERR_CHUNK_TOO_LARGE;

    if (need <= dec->capacity)
        return PNG_DEC_OK;

    new_cap = (dec->capacity == 0u) ? 4096u : dec->capacity;
    while (new_cap < need)
    {
        if (new_cap > 0x7FFFFFFFu)
            return PNG_DEC_ERR_TEMP_MEMORY_LIMIT;
        new_cap *= 2u;
    }
    if (dec->options.max_temp_bytes != 0u && new_cap > dec->options.max_temp_bytes)
        return PNG_DEC_ERR_TEMP_MEMORY_LIMIT;
    new_buf = (png_u8*)png_mem89_resize(dec->buffer, new_cap);
    if (!new_buf)
        return PNG_DEC_ERR_OOM;
    dec->buffer = new_buf;
    dec->capacity = new_cap;
    return PNG_DEC_OK;
}

static int apng_prog_append_result_frame(png_apng_decoder* dec, const png_apng_frame* frame)
{
    png_apng_frame* new_frames;
    png_u32 new_cap;

    if (!dec || !frame)
        return PNG_DEC_ERR_FORMAT;
    if (dec->options.max_apng_frames != 0u && dec->animation.frame_count >= dec->options.max_apng_frames)
        return PNG_DEC_ERR_FRAME_LIMIT;
    if (dec->animation.frame_count == dec->animation_frame_capacity)
    {
        new_cap = (dec->animation_frame_capacity == 0u) ? 4u : (dec->animation_frame_capacity * 2u);
        new_frames = (png_apng_frame*)png_mem89_resize(dec->animation.frames, new_cap * sizeof(png_apng_frame));
        if (!new_frames)
            return PNG_DEC_ERR_OOM;
        dec->animation.frames = new_frames;
        dec->animation_frame_capacity = new_cap;
    }
    dec->animation.frames[dec->animation.frame_count] = *frame;
    dec->animation.frame_count += 1u;
    return PNG_DEC_OK;
}

static void apng_prog_emit_info(png_apng_decoder* dec)
{
    png_apng_info info;
    png_u8 fmt;

    if (!dec || dec->info_emitted || !dec->callbacks.info_fn || !dec->seen_IHDR || !dec->seen_acTL)
        return;

    memset(&info, 0, sizeof(info));
    fmt = dec->options.output_format;
    if (png_output_format_channels(fmt) == 0u)
        fmt = PNG_OUTPUT_RGBA8;
    info.width = dec->width;
    info.height = dec->height;
    info.num_frames_declared = dec->num_frames_declared;
    info.num_plays = dec->num_plays;
    info.output_format = fmt;
    info.output_channels = png_output_format_channels(fmt);
    info.output_sample_depth = png_output_format_sample_depth(fmt);
    info.output_bytes_per_channel = png_output_format_bytes_per_channel(fmt);
    info.source_color_type = dec->color_type;
    info.source_bit_depth = dec->bit_depth;
    info.interlace_method = dec->interlace_method;
    dec->callbacks.info_fn(dec->callbacks.user_ptr, &info);
    dec->info_emitted = 1;
}

static int apng_prog_setup_canvas(png_apng_decoder* dec)
{
    png_u8 fmt;
    int err;

    if (!dec)
        return PNG_DEC_ERR_FORMAT;
    if (dec->canvas)
        return PNG_DEC_OK;

    dec->internal_16 = (dec->bit_depth == 16u) ? 1 : 0;
    dec->pixel_bytes = dec->internal_16 ? 8u : 4u;
    fmt = dec->internal_16 ? PNG_OUTPUT_RGBA16 : PNG_OUTPUT_RGBA8;
    err = png_output_format_rowbytes(fmt, dec->width, &dec->internal_rowbytes);
    if (err != PNG_DEC_OK)
        return err;
    if (!apng_prog_safe_mul_u32(dec->internal_rowbytes, dec->height, &dec->canvas_size))
        return PNG_DEC_ERR_TOO_MANY_PIXELS;
    if (dec->options.max_temp_bytes != 0u && dec->canvas_size > dec->options.max_temp_bytes)
        return PNG_DEC_ERR_TEMP_MEMORY_LIMIT;
    dec->canvas = (png_u8*)png_mem89_alloc(dec->canvas_size == 0u ? 1u : dec->canvas_size);
    dec->frame_canvas = (png_u8*)png_mem89_alloc(dec->canvas_size == 0u ? 1u : dec->canvas_size);
    dec->frame_row_emitted_flags = (png_u8*)png_mem89_alloc(dec->height == 0u ? 1u : dec->height);
    if (!dec->canvas || !dec->frame_canvas || !dec->frame_row_emitted_flags)
        return PNG_DEC_ERR_OOM;
    memset(dec->canvas, 0, dec->canvas_size);
    memset(dec->frame_canvas, 0, dec->canvas_size);
    memset(dec->frame_row_emitted_flags, 0, dec->height == 0u ? 1u : dec->height);

    dec->animation.width = dec->width;
    dec->animation.height = dec->height;
    dec->animation.num_plays = dec->num_plays;
    dec->animation.has_default_image = 0;
    dec->animation.frame_count = 0u;
    dec->animation.output_format = 0u;
    dec->animation.output_channels = 0u;
    dec->animation.output_sample_depth = 0u;
    dec->animation.output_bytes_per_channel = 0u;
    return PNG_DEC_OK;
}

static int apng_prog_frame_feed_raw_bytes(png_apng_decoder* dec,
                                         const png_u8* data,
                                         png_u32 size)
{
    int err;

    if (!dec || !dec->frame_decoder || (!data && size != 0u))
        return PNG_DEC_ERR_FORMAT;

    err = png_decoder_feed(dec->frame_decoder, data, size);
    if (err == PNG_DEC_PAUSED)
        return dec->yield_requested ? apng_prog_budget_hit_result(dec) : PNG_DEC_PAUSED;
    if (err == PNG_DEC_YIELDED)
        return apng_prog_budget_hit_result(dec);
    if (err != PNG_DEC_OK && err != PNG_DEC_DONE)
        return err;
    return PNG_DEC_OK;
}

static int apng_prog_frame_feed_chunk(png_apng_decoder* dec,
                                      const png_u8 type[4],
                                      const png_u8* data,
                                      png_u32 size)
{
    png_u8* raw;
    png_u32 raw_size;
    png_u32 crc;
    int err;

    if (!dec || !type || (!data && size != 0u))
        return PNG_DEC_ERR_FORMAT;

    raw_size = 12u + size;
    raw = (png_u8*)png_mem89_alloc(raw_size == 0u ? 1u : raw_size);
    if (!raw)
        return PNG_DEC_ERR_OOM;

    apng_prog_write_be32(raw + 0u, size);
    memcpy(raw + 4u, type, 4u);
    if (size != 0u)
        memcpy(raw + 8u, data, size);
    crc = png_crc32_start();
    crc = png_crc32_update(crc, type, 4u);
    if (size != 0u)
        crc = png_crc32_update(crc, data, size);
    crc = png_crc32_finish(crc);
    apng_prog_write_be32(raw + 8u + size, crc);

    err = apng_prog_frame_feed_raw_bytes(dec, raw, raw_size);
    png_mem89_release(raw);
    return err;
}

static int apng_prog_canvas_row_to_public(const png_apng_decoder* dec,
                                          png_u32 row_index,
                                          png_u8** out_row,
                                          png_u32* out_rowbytes)
{
    png_image tmp;
    png_u8 requested_format;
    png_u8* src;
    int err;

    if (!dec || !out_row || !out_rowbytes || !dec->frame_canvas)
        return PNG_DEC_ERR_FORMAT;
    if (row_index >= dec->height)
        return PNG_DEC_ERR_FORMAT;

    memset(&tmp, 0, sizeof(tmp));
    src = dec->frame_canvas + row_index * dec->internal_rowbytes;
    tmp.pixels = (png_u8*)png_mem89_alloc(dec->internal_rowbytes == 0u ? 1u : dec->internal_rowbytes);
    if (!tmp.pixels)
        return PNG_DEC_ERR_OOM;
    memcpy(tmp.pixels, src, dec->internal_rowbytes);
    tmp.width = dec->width;
    tmp.height = 1u;
    tmp.pixel_rowbytes = dec->internal_rowbytes;
    tmp.output_format = dec->internal_16 ? PNG_OUTPUT_RGBA16 : PNG_OUTPUT_RGBA8;
    tmp.output_channels = 4u;
    tmp.output_sample_depth = dec->internal_16 ? 16u : 8u;
    tmp.output_bytes_per_channel = dec->internal_16 ? 2u : 1u;
    tmp.rgba = dec->internal_16 ? 0 : tmp.pixels;

    if (dec->internal_16)
        apng_prog_apply_post_transforms_rgba16(tmp.pixels, dec->width, dec->options.transform_flags & ~PNG_DEC_TRANSFORM_APPLY_GAMMA);
    else
        apng_prog_apply_post_transforms_rgba8(tmp.pixels, dec->width, dec->options.transform_flags & ~PNG_DEC_TRANSFORM_APPLY_GAMMA);

    requested_format = dec->options.output_format;
    if (png_output_format_channels(requested_format) == 0u)
        requested_format = PNG_OUTPUT_RGBA8;
    if (requested_format != tmp.output_format)
    {
        err = png_image_convert_format(&tmp, requested_format);
        if (err != PNG_DEC_OK)
        {
            png_free_image(&tmp);
            return err;
        }
    }

    if ((dec->options.transform_flags & PNG_DEC_TRANSFORM_SWAP_16_ENDIAN) &&
        tmp.output_sample_depth == 16u && tmp.pixels)
        png_swap_16_buffer(tmp.pixels, tmp.pixel_rowbytes);

    *out_row = tmp.pixels;
    *out_rowbytes = tmp.pixel_rowbytes;
    tmp.pixels = 0;
    tmp.rgba = 0;
    png_free_image(&tmp);
    return PNG_DEC_OK;
}

static void apng_prog_composite_row_rgba8(png_u8* canvas_row,
                                          png_u32 x_offset,
                                          png_u32 width,
                                          png_u8 blend_op,
                                          const png_u8* src_row,
                                          png_u32 src_rowbytes)
{
    png_u32 col;
    png_u8* dst;
    (void)src_rowbytes;
    dst = canvas_row + x_offset * 4u;
    if (blend_op == PNG_APNG_BLEND_OP_SOURCE)
        memcpy(dst, src_row, (width * 4u));
    else
        for (col = 0u; col < width; ++col)
            apng_prog_blend_over_rgba8_pixel(dst + col * 4u, src_row + col * 4u);
}

static void apng_prog_composite_row_rgba16(png_u8* canvas_row,
                                           png_u32 x_offset,
                                           png_u32 width,
                                           png_u8 blend_op,
                                           const png_u8* src_row,
                                           png_u32 src_rowbytes)
{
    png_u32 col;
    png_u8* dst;
    (void)src_rowbytes;
    dst = canvas_row + x_offset * 8u;
    if (blend_op == PNG_APNG_BLEND_OP_SOURCE)
        memcpy(dst, src_row, (width * 8u));
    else
        for (col = 0u; col < width; ++col)
            apng_prog_blend_over_rgba16_pixel(dst + col * 8u, src_row + col * 8u);
}

static int apng_prog_get_pass_geometry(png_u32 row_index, int pass, png_u32* out_x_start, png_u32* out_x_step)
{
    png_u32 pass_index;
    png_u32 y_start;
    png_u32 y_step;

    if (!out_x_start || !out_x_step)
        return 0;
    if (pass < 1 || pass > 7)
        return 0;

    pass_index = (png_u32)(pass - 1);
    y_start = (png_u32)PNG_ADAM7_Y_START[pass_index];
    y_step = (png_u32)PNG_ADAM7_Y_STEP[pass_index];
    if (row_index < y_start)
        return 0;
    if (((row_index - y_start) % y_step) != 0u)
        return 0;

    *out_x_start = (png_u32)PNG_ADAM7_X_START[pass_index];
    *out_x_step = (png_u32)PNG_ADAM7_X_STEP[pass_index];
    return 1;
}

static void apng_prog_update_pass_pixels_rgba8(png_u8* dst_row,
                                                png_u32 width,
                                                png_u8 blend_op,
                                                const png_u8* src_row,
                                                png_u32 x_start,
                                                png_u32 x_step)
{
    png_u32 x;

    if (!dst_row || !src_row || x_step == 0u)
        return;

    for (x = x_start; x < width; x += x_step)
    {
        png_u8* dst_px = dst_row + x * 4u;
        const png_u8* src_px = src_row + x * 4u;
        if (blend_op == PNG_APNG_BLEND_OP_SOURCE)
            memcpy(dst_px, src_px, 4u);
        else
            apng_prog_blend_over_rgba8_pixel(dst_px, src_px);
    }
}

static void apng_prog_update_pass_pixels_rgba16(png_u8* dst_row,
                                                 png_u32 width,
                                                 png_u8 blend_op,
                                                 const png_u8* src_row,
                                                 png_u32 x_start,
                                                 png_u32 x_step)
{
    png_u32 x;

    if (!dst_row || !src_row || x_step == 0u)
        return;

    for (x = x_start; x < width; x += x_step)
    {
        png_u8* dst_px = dst_row + x * 8u;
        const png_u8* src_px = src_row + x * 8u;
        if (blend_op == PNG_APNG_BLEND_OP_SOURCE)
            memcpy(dst_px, src_px, 8u);
        else
            apng_prog_blend_over_rgba16_pixel(dst_px, src_px);
    }
}

static void apng_prog_emit_frame_row_callbacks(png_apng_decoder* dec,
                                               png_u32 frame_index,
                                               png_u32 row_index,
                                               const png_u8* row_data,
                                               png_u32 rowbytes,
                                               int pass)
{
    int err;

    if (!dec || !row_data)
        return;

    if (dec->callbacks.frame_row_pass_fn)
        dec->callbacks.frame_row_pass_fn(dec->callbacks.user_ptr,
                                         frame_index,
                                         row_index,
                                         row_data,
                                         rowbytes,
                                         pass);

    if (dec->callbacks.frame_row_fn)
        dec->callbacks.frame_row_fn(dec->callbacks.user_ptr,
                                    frame_index,
                                    row_index,
                                    row_data,
                                    rowbytes);

    err = apng_prog_note_row_callback(dec);
    if (err != PNG_DEC_OK)
        dec->yield_requested = 1;

    if ((dec->pause_requested || dec->yield_requested) && dec->frame_decoder)
        (void)png_decoder_process_data_pause(dec->frame_decoder, 1);
}

static void apng_prog_frame_row_cb_internal(void* user_ptr,
                                            png_u32 row_index,
                                            const png_u8* row_data,
                                            png_u32 rowbytes,
                                            int pass)
{
    png_apng_decoder* dec = (png_apng_decoder*)user_ptr;
    png_u32 canvas_y;
    png_u8* public_row;
    png_u32 public_rowbytes;
    png_u8* dst_row;
    png_u32 x_start;
    png_u32 x_step;
    int have_pass_geometry;
    int err;

    if (!dec || !dec->current_frame_open || !row_data || !dec->frame_canvas || !dec->canvas)
        return;
    if (row_index >= dec->current_frame.control.height)
        return;

    canvas_y = dec->current_frame.control.y_offset + row_index;
    if (canvas_y >= dec->height)
        return;

    dst_row = dec->frame_canvas + canvas_y * dec->internal_rowbytes +
              dec->current_frame.control.x_offset * dec->pixel_bytes;
    have_pass_geometry = 0;
    if (dec->interlace_method == 1u && pass > 0)
        have_pass_geometry = apng_prog_get_pass_geometry(row_index, pass, &x_start, &x_step);

    if (have_pass_geometry)
    {
        if (dec->internal_16)
            apng_prog_update_pass_pixels_rgba16(dst_row,
                                                dec->current_frame.control.width,
                                                dec->current_frame.control.blend_op,
                                                row_data,
                                                x_start,
                                                x_step);
        else
            apng_prog_update_pass_pixels_rgba8(dst_row,
                                               dec->current_frame.control.width,
                                               dec->current_frame.control.blend_op,
                                               row_data,
                                               x_start,
                                               x_step);
    }
    else if (dec->internal_16)
        apng_prog_composite_row_rgba16(dec->frame_canvas + canvas_y * dec->internal_rowbytes,
                                       dec->current_frame.control.x_offset,
                                       dec->current_frame.control.width,
                                       dec->current_frame.control.blend_op,
                                       row_data,
                                       rowbytes);
    else
        apng_prog_composite_row_rgba8(dec->frame_canvas + canvas_y * dec->internal_rowbytes,
                                      dec->current_frame.control.x_offset,
                                      dec->current_frame.control.width,
                                      dec->current_frame.control.blend_op,
                                      row_data,
                                      rowbytes);

    dec->frame_live_rows = 1;
    if (dec->frame_row_emitted_flags && canvas_y < dec->height)
        dec->frame_row_emitted_flags[canvas_y] = 1u;

    if (dec->callbacks.frame_row_fn || dec->callbacks.frame_row_pass_fn)
    {
        err = apng_prog_canvas_row_to_public(dec, canvas_y, &public_row, &public_rowbytes);
        if (err == PNG_DEC_OK)
        {
            apng_prog_emit_frame_row_callbacks(dec,
                                               dec->animation.frame_count,
                                               canvas_y,
                                               public_row,
                                               public_rowbytes,
                                               pass);
            png_mem89_release(public_row);
        }
    }
}

static int apng_prog_init_frame_decoder(png_apng_decoder* dec,
                                        const png_apng_frame_control* ctl)
{
    png_decode_options dopt_internal;
    png_progressive_callbacks pcb;
    png_u8 ihdr[13];
    png_u8 fmt;
    png_u32 i;
    png_u32 rowbytes;
    int err;
    static const png_u8 ihdr_type[4] = { 'I', 'H', 'D', 'R' };

    if (!dec || !ctl)
        return PNG_DEC_ERR_FORMAT;

    err = apng_prog_setup_canvas(dec);
    if (err != PNG_DEC_OK)
        return err;

    if (dec->frame_decoder)
    {
        png_decoder_free(dec->frame_decoder);
        dec->frame_decoder = 0;
    }

    if (dec->canvas_size != 0u)
        memcpy(dec->frame_canvas, dec->canvas, dec->canvas_size);
    if (ctl->blend_op == PNG_APNG_BLEND_OP_SOURCE)
    {
        if (dec->internal_16)
            apng_prog_clear_region_rgba16(dec->frame_canvas, dec->internal_rowbytes,
                                          ctl->x_offset, ctl->y_offset,
                                          ctl->width, ctl->height);
        else
            apng_prog_clear_region_rgba8(dec->frame_canvas, dec->internal_rowbytes,
                                         ctl->x_offset, ctl->y_offset,
                                         ctl->width, ctl->height);
    }
    if (dec->frame_row_emitted_flags)
        memset(dec->frame_row_emitted_flags, 0, dec->height == 0u ? 1u : dec->height);
    dec->frame_live_rows = 0;
    dec->frame_info_emitted = 0;

    err = png_decoder_init(&dec->frame_decoder, dec->zfunc);
    if (err != PNG_DEC_OK)
        return err;

    if (dec->options_set)
        dopt_internal = dec->options;
    else
        png_decode_options_init(&dopt_internal);
    dopt_internal.transform_flags &= PNG_DEC_TRANSFORM_APPLY_GAMMA;
    dopt_internal.output_format = dec->internal_16 ? PNG_OUTPUT_RGBA16 : PNG_OUTPUT_RGBA8;

    err = png_decoder_set_options(dec->frame_decoder, &dopt_internal);
    if (err != PNG_DEC_OK)
        return err;

    if (dec->prog_ctl.max_parse_bytes_per_call != 0u ||
        dec->prog_ctl.max_zlib_work_bytes_per_call != 0u ||
        dec->prog_ctl.max_zlib_steps_per_call != 0u)
    {
        png_progressive_control inner_ctl;
        png_progressive_control_init(&inner_ctl);
        inner_ctl.max_parse_bytes_per_call = dec->prog_ctl.max_parse_bytes_per_call;
        inner_ctl.max_zlib_work_bytes_per_call = dec->prog_ctl.max_zlib_work_bytes_per_call;
        inner_ctl.max_zlib_steps_per_call = dec->prog_ctl.max_zlib_steps_per_call;
        err = png_decoder_set_progressive_control(dec->frame_decoder, &inner_ctl);
        if (err != PNG_DEC_OK)
            return err;
    }

    memset(&pcb, 0, sizeof(pcb));
    pcb.user_ptr = dec;
    pcb.row_fn = apng_prog_frame_row_cb_internal;
    err = png_decoder_set_callbacks(dec->frame_decoder, &pcb);
    if (err != PNG_DEC_OK)
        return err;

    err = apng_prog_frame_feed_raw_bytes(dec, APNG_PROG_SIG, 8u);
    if (err != PNG_DEC_OK)
        return err;

    apng_prog_write_be32(ihdr + 0u, ctl->width);
    apng_prog_write_be32(ihdr + 4u, ctl->height);
    ihdr[8] = dec->bit_depth;
    ihdr[9] = dec->color_type;
    ihdr[10] = dec->compression_method;
    ihdr[11] = dec->filter_method;
    ihdr[12] = dec->interlace_method;
    err = apng_prog_frame_feed_chunk(dec, ihdr_type, ihdr, 13u);
    if (err != PNG_DEC_OK)
        return err;

    for (i = 0u; i < dec->pre_chunk_count; ++i)
    {
        err = apng_prog_frame_feed_raw_bytes(dec, dec->pre_chunks[i].bytes, dec->pre_chunks[i].size);
        if (err != PNG_DEC_OK)
            return err;
    }

    if (dec->callbacks.frame_info_fn)
    {
        fmt = dec->options.output_format;
        if (png_output_format_channels(fmt) == 0u)
            fmt = PNG_OUTPUT_RGBA8;
        err = png_output_format_rowbytes(fmt, dec->width, &rowbytes);
        if (err != PNG_DEC_OK)
            return err;
        dec->callbacks.frame_info_fn(dec->callbacks.user_ptr,
                                     dec->animation.frame_count,
                                     ctl,
                                     rowbytes);
        dec->frame_info_emitted = 1;
    }

    return PNG_DEC_OK;
}

static int apng_prog_start_frame(png_apng_decoder* dec,
                                 const png_apng_frame_control* ctl,
                                 int uses_idat)
{
    int err;

    if (!dec || !ctl)
        return PNG_DEC_ERR_FORMAT;
    if (dec->current_frame_open)
        return PNG_DEC_ERR_FORMAT;
    memset(&dec->current_frame, 0, sizeof(dec->current_frame));
    dec->current_frame.control = *ctl;
    dec->current_frame.uses_idat = uses_idat;
    dec->current_frame_open = 1;
    err = apng_prog_init_frame_decoder(dec, ctl);
    if (err != PNG_DEC_OK)
    {
        apng_prog_frame_build_reset(&dec->current_frame);
        dec->current_frame_open = 0;
        return err;
    }
    return PNG_DEC_OK;
}

static int apng_prog_append_current_frame_data(png_apng_decoder* dec,
                                               const png_u8* data,
                                               png_u32 size)
{
    if (!dec || (!data && size != 0u) || !dec->current_frame_open)
        return PNG_DEC_ERR_FORMAT;
    return apng_prog_buffer_append(&dec->current_frame.compressed,
                                   &dec->current_frame.compressed_size,
                                   &dec->current_frame.compressed_capacity,
                                   data,
                                   size);
}

static int apng_prog_finish_current_frame_buffered(png_apng_decoder* dec)
{
    png_u8* frame_png;
    png_u32 frame_png_size;
    png_image frame_img;
    png_apng_frame public_frame;
    png_apng_frame_control ctl;
    png_decode_options dopt_user;
    png_decode_options dopt_internal;
    png_u8 requested_format;
    png_u32 post_flags;
    png_u8* saved;
    png_u32 saved_size;
    png_u32 row;
    int err;
    png_u32 frame_index;

    if (!dec)
        return PNG_DEC_ERR_FORMAT;
    if (!dec->current_frame_open)
        return PNG_DEC_OK;

    err = apng_prog_setup_canvas(dec);
    if (err != PNG_DEC_OK)
        return err;

    frame_png = 0;
    frame_png_size = 0u;
    memset(&frame_img, 0, sizeof(frame_img));
    memset(&public_frame, 0, sizeof(public_frame));
    saved = 0;
    saved_size = 0u;
    ctl = dec->current_frame.control;
    frame_index = dec->animation.frame_count;

    err = apng_prog_build_frame_png(dec, &dec->current_frame, &frame_png, &frame_png_size);
    if (err != PNG_DEC_OK)
        goto cleanup;

    if (dec->options_set)
        dopt_user = dec->options;
    else
        png_decode_options_init(&dopt_user);

    png_decode_options_init(&dopt_internal);
    dopt_internal.keep_text = dopt_user.keep_text;
    dopt_internal.keep_unknown_chunks = dopt_user.keep_unknown_chunks;
    dopt_internal.strict_trailing_data = dopt_user.strict_trailing_data;
    dopt_internal.max_width = dopt_user.max_width;
    dopt_internal.max_height = dopt_user.max_height;
    dopt_internal.max_image_bytes = dopt_user.max_image_bytes;
    dopt_internal.max_file_bytes = dopt_user.max_file_bytes;
    dopt_internal.max_text_entries = dopt_user.max_text_entries;
    dopt_internal.max_unknown_chunks = dopt_user.max_unknown_chunks;
    dopt_internal.max_chunk_bytes = dopt_user.max_chunk_bytes;
    dopt_internal.max_pixels = dopt_user.max_pixels;
    dopt_internal.max_inflated_bytes = dopt_user.max_inflated_bytes;
    dopt_internal.max_chunks = dopt_user.max_chunks;
    dopt_internal.max_text_bytes = dopt_user.max_text_bytes;
    dopt_internal.max_apng_frames = dopt_user.max_apng_frames;
    dopt_internal.max_temp_bytes = dopt_user.max_temp_bytes;
    dopt_internal.max_conversion_expansion = dopt_user.max_conversion_expansion;
    dopt_internal.transform_flags = dopt_user.transform_flags & PNG_DEC_TRANSFORM_APPLY_GAMMA;
    dopt_internal.output_format = dec->internal_16 ? PNG_OUTPUT_RGBA16 : PNG_OUTPUT_RGBA8;

    err = png_decode_memory_ex(frame_png, frame_png_size, dec->zfunc, &dopt_internal, &frame_img);
    if (err != PNG_DEC_OK)
        goto cleanup;

    if (frame_index == 0u && ctl.dispose_op == PNG_APNG_DISPOSE_OP_PREVIOUS)
        ctl.dispose_op = PNG_APNG_DISPOSE_OP_BACKGROUND;

    if (ctl.dispose_op == PNG_APNG_DISPOSE_OP_PREVIOUS)
    {
        if (!apng_prog_safe_mul_u32(ctl.width * dec->pixel_bytes, ctl.height, &saved_size))
        {
            err = PNG_DEC_ERR_TOO_MANY_PIXELS;
            goto cleanup;
        }
        if (dopt_user.max_temp_bytes != 0u && saved_size > dopt_user.max_temp_bytes)
        {
            err = PNG_DEC_ERR_TEMP_MEMORY_LIMIT;
            goto cleanup;
        }
        saved = (png_u8*)png_mem89_alloc(saved_size == 0u ? 1u : saved_size);
        if (!saved)
        {
            err = PNG_DEC_ERR_OOM;
            goto cleanup;
        }
        for (row = 0u; row < ctl.height; ++row)
        {
            memcpy(saved + row * ctl.width * dec->pixel_bytes,
                   dec->canvas + (ctl.y_offset + row) * dec->internal_rowbytes + ctl.x_offset * dec->pixel_bytes,
                   (ctl.width * dec->pixel_bytes));
        }
    }

    if (dec->internal_16)
        apng_prog_composite_frame_rgba16(dec->canvas, dec->internal_rowbytes, &ctl,
                                         frame_img.pixels, frame_img.pixel_rowbytes);
    else
        apng_prog_composite_frame_rgba8(dec->canvas, dec->internal_rowbytes, &ctl,
                                        frame_img.pixels, frame_img.pixel_rowbytes);

    requested_format = dopt_user.output_format;
    if (png_output_format_channels(requested_format) == 0u)
        requested_format = PNG_OUTPUT_RGBA8;
    post_flags = dopt_user.transform_flags & ~PNG_DEC_TRANSFORM_APPLY_GAMMA;
    err = apng_prog_canvas_to_public_frame(dec->canvas, dec->internal_rowbytes,
                                           dec->width, dec->height,
                                           dec->internal_16, post_flags,
                                           requested_format, &public_frame);
    if (err != PNG_DEC_OK)
        goto cleanup;

    public_frame.control = dec->current_frame.control;
    err = apng_prog_append_result_frame(dec, &public_frame);
    if (err != PNG_DEC_OK)
        goto cleanup;
    memset(&public_frame, 0, sizeof(public_frame));

    if (dec->animation.frame_count == 1u)
    {
        dec->animation.output_format = dec->animation.frames[0].output_format;
        dec->animation.output_channels = dec->animation.frames[0].output_channels;
        dec->animation.output_sample_depth = dec->animation.frames[0].output_sample_depth;
        dec->animation.output_bytes_per_channel = dec->animation.frames[0].output_bytes_per_channel;
    }

    if (dec->callbacks.frame_info_fn)
        dec->callbacks.frame_info_fn(dec->callbacks.user_ptr,
                                     frame_index,
                                     &dec->animation.frames[frame_index].control,
                                     dec->animation.frames[frame_index].pixel_rowbytes);

    if (dec->callbacks.frame_row_fn || dec->callbacks.frame_row_pass_fn)
    {
        for (row = 0u; row < dec->height; ++row)
        {
            apng_prog_emit_frame_row_callbacks(dec,
                                               frame_index,
                                               row,
                                               dec->animation.frames[frame_index].pixels + row * dec->animation.frames[frame_index].pixel_rowbytes,
                                               dec->animation.frames[frame_index].pixel_rowbytes,
                                               0);
        }
    }

    if (dec->callbacks.frame_end_fn)
        dec->callbacks.frame_end_fn(dec->callbacks.user_ptr,
                                    frame_index,
                                    &dec->animation.frames[frame_index]);

    if (ctl.dispose_op == PNG_APNG_DISPOSE_OP_BACKGROUND)
    {
        if (dec->internal_16)
            apng_prog_clear_region_rgba16(dec->canvas, dec->internal_rowbytes,
                                          ctl.x_offset, ctl.y_offset,
                                          ctl.width, ctl.height);
        else
            apng_prog_clear_region_rgba8(dec->canvas, dec->internal_rowbytes,
                                         ctl.x_offset, ctl.y_offset,
                                         ctl.width, ctl.height);
    }
    else if (ctl.dispose_op == PNG_APNG_DISPOSE_OP_PREVIOUS)
    {
        for (row = 0u; row < ctl.height; ++row)
        {
            memcpy(dec->canvas + (ctl.y_offset + row) * dec->internal_rowbytes + ctl.x_offset * dec->pixel_bytes,
                   saved + row * ctl.width * dec->pixel_bytes,
                   (ctl.width * dec->pixel_bytes));
        }
    }

    err = PNG_DEC_OK;

cleanup:
    png_mem89_release(saved);
    png_mem89_release(frame_png);
    png_free_image(&frame_img);
    if (err != PNG_DEC_OK)
        png_mem89_release(public_frame.pixels);
    apng_prog_frame_build_reset(&dec->current_frame);
    dec->current_frame_open = 0;
    return err;
}

static int apng_prog_finish_current_frame(png_apng_decoder* dec)
{
    png_apng_frame public_frame;
    png_apng_frame_control ctl;
    png_image frame_img;
    png_decode_options dopt_user;
    png_u8 requested_format;
    png_u32 post_flags;
    png_u32 row;
    int err;
    png_u32 frame_index;
    int final_feed;
    static const png_u8 iend_type[4] = { 'I', 'E', 'N', 'D' };

    if (!dec)
        return PNG_DEC_ERR_FORMAT;
    if (!dec->current_frame_open)
        return PNG_DEC_OK;
    if (!dec->frame_decoder)
        return apng_prog_finish_current_frame_buffered(dec);

    memset(&public_frame, 0, sizeof(public_frame));
    memset(&frame_img, 0, sizeof(frame_img));
    ctl = dec->current_frame.control;
    frame_index = dec->animation.frame_count;
    if (frame_index == 0u && ctl.dispose_op == PNG_APNG_DISPOSE_OP_PREVIOUS)
        ctl.dispose_op = PNG_APNG_DISPOSE_OP_BACKGROUND;

    final_feed = apng_prog_frame_feed_chunk(dec, iend_type, 0, 0u);
    if (final_feed != PNG_DEC_OK)
    {
        err = final_feed;
        goto cleanup;
    }

    err = png_decoder_take_image(dec->frame_decoder, &frame_img);
    if (err != PNG_DEC_OK)
        goto cleanup;

    if (!dec->frame_live_rows)
    {
        if (dec->canvas_size != 0u)
            memcpy(dec->frame_canvas, dec->canvas, dec->canvas_size);
        if (ctl.blend_op == PNG_APNG_BLEND_OP_SOURCE)
        {
            if (dec->internal_16)
                apng_prog_clear_region_rgba16(dec->frame_canvas, dec->internal_rowbytes,
                                              ctl.x_offset, ctl.y_offset,
                                              ctl.width, ctl.height);
            else
                apng_prog_clear_region_rgba8(dec->frame_canvas, dec->internal_rowbytes,
                                             ctl.x_offset, ctl.y_offset,
                                             ctl.width, ctl.height);
        }

        if (dec->internal_16)
            apng_prog_composite_frame_rgba16(dec->frame_canvas, dec->internal_rowbytes, &ctl,
                                             frame_img.pixels, frame_img.pixel_rowbytes);
        else
            apng_prog_composite_frame_rgba8(dec->frame_canvas, dec->internal_rowbytes, &ctl,
                                            frame_img.pixels, frame_img.pixel_rowbytes);
    }

    if (dec->options_set)
        dopt_user = dec->options;
    else
        png_decode_options_init(&dopt_user);
    requested_format = dopt_user.output_format;
    if (png_output_format_channels(requested_format) == 0u)
        requested_format = PNG_OUTPUT_RGBA8;
    post_flags = dopt_user.transform_flags & ~PNG_DEC_TRANSFORM_APPLY_GAMMA;

    err = apng_prog_canvas_to_public_frame(dec->frame_canvas, dec->internal_rowbytes,
                                           dec->width, dec->height,
                                           dec->internal_16, post_flags,
                                           requested_format, &public_frame);
    if (err != PNG_DEC_OK)
        goto cleanup;

    public_frame.control = dec->current_frame.control;
    err = apng_prog_append_result_frame(dec, &public_frame);
    if (err != PNG_DEC_OK)
        goto cleanup;
    memset(&public_frame, 0, sizeof(public_frame));

    if (dec->animation.frame_count == 1u)
    {
        dec->animation.output_format = dec->animation.frames[0].output_format;
        dec->animation.output_channels = dec->animation.frames[0].output_channels;
        dec->animation.output_sample_depth = dec->animation.frames[0].output_sample_depth;
        dec->animation.output_bytes_per_channel = dec->animation.frames[0].output_bytes_per_channel;
    }

    if (!dec->frame_info_emitted && dec->callbacks.frame_info_fn)
    {
        dec->callbacks.frame_info_fn(dec->callbacks.user_ptr,
                                     frame_index,
                                     &dec->animation.frames[frame_index].control,
                                     dec->animation.frames[frame_index].pixel_rowbytes);
        dec->frame_info_emitted = 1;
    }

    if (dec->callbacks.frame_row_fn || dec->callbacks.frame_row_pass_fn)
    {
        for (row = 0u; row < dec->height; ++row)
        {
            if (!dec->frame_live_rows || !dec->frame_row_emitted_flags || !dec->frame_row_emitted_flags[row])
            {
                apng_prog_emit_frame_row_callbacks(dec,
                                                   frame_index,
                                                   row,
                                                   dec->animation.frames[frame_index].pixels + row * dec->animation.frames[frame_index].pixel_rowbytes,
                                                   dec->animation.frames[frame_index].pixel_rowbytes,
                                                   0);
            }
        }
    }

    if (dec->callbacks.frame_end_fn)
        dec->callbacks.frame_end_fn(dec->callbacks.user_ptr,
                                    frame_index,
                                    &dec->animation.frames[frame_index]);

    if (ctl.dispose_op == PNG_APNG_DISPOSE_OP_NONE)
        memcpy(dec->canvas, dec->frame_canvas, dec->canvas_size);
    else if (ctl.dispose_op == PNG_APNG_DISPOSE_OP_BACKGROUND)
    {
        memcpy(dec->canvas, dec->frame_canvas, dec->canvas_size);
        if (dec->internal_16)
            apng_prog_clear_region_rgba16(dec->canvas, dec->internal_rowbytes,
                                          ctl.x_offset, ctl.y_offset,
                                          ctl.width, ctl.height);
        else
            apng_prog_clear_region_rgba8(dec->canvas, dec->internal_rowbytes,
                                         ctl.x_offset, ctl.y_offset,
                                         ctl.width, ctl.height);
    }

    err = PNG_DEC_OK;

cleanup:
    png_free_image(&frame_img);
    if (err != PNG_DEC_OK)
        png_mem89_release(public_frame.pixels);
    if (dec->frame_decoder)
    {
        png_decoder_free(dec->frame_decoder);
        dec->frame_decoder = 0;
    }
    dec->frame_info_emitted = 0;
    dec->frame_live_rows = 0;
    apng_prog_frame_build_reset(&dec->current_frame);
    dec->current_frame_open = 0;
    return err;
}

static int apng_prog_finish_stream(png_apng_decoder* dec)
{
    int err;
    png_decode_options dopt_user;

    if (!dec)
        return PNG_DEC_ERR_FORMAT;
    if (!dec->seen_IEND)
        return PNG_DEC_ERR_FORMAT;

    err = apng_prog_finish_current_frame(dec);
    if (err != PNG_DEC_OK)
        return err;

    if (!dec->seen_acTL || dec->animation.frame_count == 0u)
        return PNG_DEC_ERR_FORMAT;
    if (dec->num_frames_declared != 0u && dec->animation.frame_count != dec->num_frames_declared)
        return PNG_DEC_ERR_FORMAT;

    if (dec->options_set)
        dopt_user = dec->options;
    else
        png_decode_options_init(&dopt_user);
    if (png_decode_memory_ex(dec->buffer, dec->parse_pos, dec->zfunc, &dopt_user, &dec->animation.default_image) == PNG_DEC_OK)
        dec->animation.has_default_image = 1;
    else if (dec->default_image_is_first_frame && dec->animation.frame_count != 0u)
    {
        const png_apng_frame* first = &dec->animation.frames[0];
        png_image* dst = &dec->animation.default_image;
        png_u32 total = first->pixel_rowbytes * dec->animation.height;
        memset(dst, 0, sizeof(*dst));
        dst->width = dec->animation.width;
        dst->height = dec->animation.height;
        dst->pixel_rowbytes = first->pixel_rowbytes;
        dst->output_format = first->output_format;
        dst->output_channels = first->output_channels;
        dst->output_sample_depth = first->output_sample_depth;
        dst->output_bytes_per_channel = first->output_bytes_per_channel;
        dst->pixels = (png_u8*)png_mem89_alloc(total == 0u ? 1u : total);
        if (dst->pixels)
        {
            memcpy(dst->pixels, first->pixels, total);
            if (dst->output_format == PNG_OUTPUT_RGBA8)
                dst->rgba = dst->pixels;
            dst->interlace_method = dec->interlace_method;
            dec->animation.has_default_image = 1;
        }
    }

    dec->parse_done = 1;
    if (dec->callbacks.end_fn)
        dec->callbacks.end_fn(dec->callbacks.user_ptr, &dec->animation);
    return PNG_DEC_DONE;
}

static int apng_prog_handle_ihdr(png_apng_decoder* dec, const png_u8* data, png_u32 length)
{
    png_u32 max_width;
    png_u32 max_height;
    png_u32 max_pixels;

    if (!dec || !data)
        return PNG_DEC_ERR_FORMAT;
    if (dec->seen_IHDR || length != 13u)
        return PNG_DEC_ERR_FORMAT;

    dec->width = apng_prog_read_be32(data + 0u);
    dec->height = apng_prog_read_be32(data + 4u);
    dec->bit_depth = data[8];
    dec->color_type = data[9];
    dec->compression_method = data[10];
    dec->filter_method = data[11];
    dec->interlace_method = data[12];

    max_width = apng_prog_limit_or_default(dec->options.max_width, PNG_DEC_MAX_WIDTH);
    max_height = apng_prog_limit_or_default(dec->options.max_height, PNG_DEC_MAX_HEIGHT);
    max_pixels = apng_prog_limit_or_default(dec->options.max_pixels, PNG_DEC_MAX_PIXELS);
    if (dec->width == 0u || dec->height == 0u ||
        dec->width > max_width || dec->height > max_height)
        return PNG_DEC_ERR_DIMENSIONS_TOO_LARGE;
    if (dec->width > 0u && dec->height > 0u && dec->width > (max_pixels / dec->height))
        return PNG_DEC_ERR_TOO_MANY_PIXELS;

    dec->seen_IHDR = 1;
    return PNG_DEC_OK;
}

static int apng_prog_validate_frame_rect(const png_apng_decoder* dec,
                                         const png_apng_frame_control* ctl)
{
    if (!dec || !ctl)
        return PNG_DEC_ERR_FORMAT;
    if (ctl->width == 0u || ctl->height == 0u)
        return PNG_DEC_ERR_FORMAT;
    if (ctl->width > dec->width || ctl->height > dec->height)
        return PNG_DEC_ERR_FORMAT;
    if (ctl->x_offset > dec->width - ctl->width)
        return PNG_DEC_ERR_FORMAT;
    if (ctl->y_offset > dec->height - ctl->height)
        return PNG_DEC_ERR_FORMAT;
    if (ctl->dispose_op > PNG_APNG_DISPOSE_OP_PREVIOUS ||
        ctl->blend_op > PNG_APNG_BLEND_OP_OVER)
        return PNG_DEC_ERR_FORMAT;
    return PNG_DEC_OK;
}

static int apng_prog_handle_fctl(png_apng_decoder* dec, const png_u8* data, png_u32 length)
{
    png_apng_frame_control ctl;
    png_u32 seq;
    int err;

    if (!dec || !data || length != 26u || !dec->seen_acTL || !dec->seen_IHDR)
        return PNG_DEC_ERR_FORMAT;

    seq = apng_prog_read_be32(data + 0u);
    if (seq != dec->next_sequence_number)
        return PNG_DEC_ERR_FORMAT;
    dec->next_sequence_number += 1u;

    ctl.width = apng_prog_read_be32(data + 4u);
    ctl.height = apng_prog_read_be32(data + 8u);
    ctl.x_offset = apng_prog_read_be32(data + 12u);
    ctl.y_offset = apng_prog_read_be32(data + 16u);
    ctl.delay_num = apng_prog_read_be16(data + 20u);
    ctl.delay_den = apng_prog_read_be16(data + 22u);
    ctl.dispose_op = data[24];
    ctl.blend_op = data[25];

    err = apng_prog_validate_frame_rect(dec, &ctl);
    if (err != PNG_DEC_OK)
        return err;

    if (!dec->seen_first_IDAT)
    {
        if (dec->animation.frame_count == 0u && !dec->current_frame_open)
        {
            if (ctl.width != dec->width || ctl.height != dec->height ||
                ctl.x_offset != 0u || ctl.y_offset != 0u)
                return PNG_DEC_ERR_FORMAT;
            dec->default_image_is_first_frame = 1;
            apng_prog_emit_info(dec);
            return apng_prog_start_frame(dec, &ctl, 1);
        }
    }

    err = apng_prog_finish_current_frame(dec);
    if (err != PNG_DEC_OK)
        return err;

    apng_prog_emit_info(dec);
    return apng_prog_start_frame(dec, &ctl, 0);
}

static int apng_prog_handle_actl(png_apng_decoder* dec, const png_u8* data, png_u32 length)
{
    if (!dec || !data || !dec->seen_IHDR || dec->seen_acTL || dec->seen_first_IDAT || length != 8u)
        return PNG_DEC_ERR_FORMAT;

    dec->num_frames_declared = apng_prog_read_be32(data + 0u);
    dec->num_plays = apng_prog_read_be32(data + 4u);
    if (dec->num_frames_declared == 0u)
        return PNG_DEC_ERR_FORMAT;
    if (dec->options.max_apng_frames != 0u && dec->num_frames_declared > dec->options.max_apng_frames)
        return PNG_DEC_ERR_FRAME_LIMIT;
    dec->seen_acTL = 1;
    apng_prog_emit_info(dec);
    return PNG_DEC_OK;
}

static int apng_prog_handle_idat(png_apng_decoder* dec, const png_u8* data, png_u32 length)
{
    int err;
    static const png_u8 idat_type[4] = { 'I', 'D', 'A', 'T' };

    if (!dec || (!data && length != 0u) || !dec->seen_IHDR || !dec->seen_acTL)
        return PNG_DEC_ERR_FORMAT;

    dec->seen_first_IDAT = 1;
    if (!dec->current_frame_open)
    {
        dec->default_image_is_first_frame = 0;
        return PNG_DEC_OK;
    }

    if (!dec->current_frame.uses_idat)
        return PNG_DEC_ERR_FORMAT;

    err = apng_prog_append_current_frame_data(dec, data, length);
    if (err != PNG_DEC_OK)
        return err;
    if (dec->frame_decoder)
    {
        err = apng_prog_frame_feed_chunk(dec, idat_type, data, length);
        if (err != PNG_DEC_OK)
            return err;
    }
    return PNG_DEC_OK;
}

static int apng_prog_handle_fdat(png_apng_decoder* dec, const png_u8* data, png_u32 length)
{
    png_u32 seq;
    int err;
    static const png_u8 idat_type[4] = { 'I', 'D', 'A', 'T' };

    if (!dec || !data || length < 4u || !dec->seen_acTL || !dec->current_frame_open)
        return PNG_DEC_ERR_FORMAT;
    if (dec->current_frame.uses_idat)
        return PNG_DEC_ERR_FORMAT;

    seq = apng_prog_read_be32(data + 0u);
    if (seq != dec->next_sequence_number)
        return PNG_DEC_ERR_FORMAT;
    dec->next_sequence_number += 1u;

    err = apng_prog_append_current_frame_data(dec, data + 4u, length - 4u);
    if (err != PNG_DEC_OK)
        return err;
    if (dec->frame_decoder)
    {
        err = apng_prog_frame_feed_chunk(dec, idat_type, data + 4u, length - 4u);
        if (err != PNG_DEC_OK)
            return err;
    }
    return PNG_DEC_OK;
}

static int apng_prog_handle_chunk(png_apng_decoder* dec,
                                  png_u32 type,
                                  const png_u8* chunk_data,
                                  png_u32 length,
                                  const png_u8* raw_chunk,
                                  png_u32 raw_size)
{
    int err;

    if (apng_prog_chunk_is_type(type, 'I', 'H', 'D', 'R'))
        return apng_prog_handle_ihdr(dec, chunk_data, length);
    if (apng_prog_chunk_is_type(type, 'a', 'c', 'T', 'L'))
        return apng_prog_handle_actl(dec, chunk_data, length);
    if (apng_prog_chunk_is_type(type, 'f', 'c', 'T', 'L'))
        return apng_prog_handle_fctl(dec, chunk_data, length);
    if (apng_prog_chunk_is_type(type, 'I', 'D', 'A', 'T'))
        return apng_prog_handle_idat(dec, chunk_data, length);
    if (apng_prog_chunk_is_type(type, 'f', 'd', 'A', 'T'))
        return apng_prog_handle_fdat(dec, chunk_data, length);
    if (apng_prog_chunk_is_type(type, 'I', 'E', 'N', 'D'))
    {
        dec->seen_IEND = 1;
        return PNG_DEC_OK;
    }

    if (!dec->seen_first_IDAT)
    {
        err = apng_prog_raw_chunk_push(&dec->pre_chunks,
                                       &dec->pre_chunk_count,
                                       &dec->pre_chunk_capacity,
                                       raw_chunk,
                                       raw_size,
                                       type);
        if (err != PNG_DEC_OK)
            return err;
        if (dec->frame_decoder && dec->current_frame_open && dec->current_frame.uses_idat)
        {
            err = apng_prog_frame_feed_raw_bytes(dec, raw_chunk, raw_size);
            if (err != PNG_DEC_OK)
                return err;
        }
    }

    return PNG_DEC_OK;
}

static int apng_prog_parse_available(png_apng_decoder* dec)
{
    if (!dec)
        return PNG_DEC_ERR_FORMAT;

    if (dec->parse_done)
        return PNG_DEC_DONE;

    if (dec->seen_IEND && dec->parse_pos == dec->size)
        return apng_prog_finish_stream(dec);

    if (dec->size < 8u)
        return PNG_DEC_OK;

    if (!dec->sig_checked)
    {
        if (memcmp(dec->buffer, APNG_PROG_SIG, 8u) != 0)
            return PNG_DEC_ERR_SIG;
        dec->sig_checked = 1;
        dec->parse_pos = 8u;
    }

    while (1)
    {
        png_u32 chunk_pos;
        png_u32 length;
        png_u32 type;
        png_u32 next_pos;
        int err;
        png_u32 max_chunk_bytes;

        if (dec->active_chunk_waiting_resume)
        {
            png_u32 resume_start = dec->active_chunk_start;
            png_u32 resume_next = dec->active_chunk_next_pos;
            png_u32 resume_type = dec->active_chunk_type;
            png_u32 resume_length = 0u;

            if (resume_start + 8u <= dec->size)
                resume_length = apng_prog_read_be32(dec->buffer + resume_start + 0u);

            err = png_decoder_feed(dec->frame_decoder, 0, 0u);
            if (err == PNG_DEC_PAUSED)
                return dec->yield_requested ? PNG_DEC_YIELDED : apng_prog_pause_here(dec, dec->active_chunk_start);
            if (err == PNG_DEC_YIELDED)
                return PNG_DEC_YIELDED;
            if (err != PNG_DEC_OK && err != PNG_DEC_DONE)
                return err;
            dec->active_chunk_waiting_resume = 0;
            dec->parse_pos = resume_next;

            err = apng_prog_emit_chunk_callback(dec, resume_type, resume_length, resume_start);
            if (err == PNG_DEC_PAUSED)
                return apng_prog_pause_here(dec, dec->parse_pos);
            if (err != PNG_DEC_OK)
                return err;

            err = apng_prog_note_parse_bytes(dec, resume_next - resume_start);
            if (err != PNG_DEC_OK)
                return err;
        }

        if (dec->prog_ctl.max_parse_bytes_per_call != 0u &&
            dec->work_parse_bytes >= dec->prog_ctl.max_parse_bytes_per_call)
            return apng_prog_budget_hit_result(dec);

        if (dec->parse_pos + 8u > dec->size)
            break;

        chunk_pos = dec->parse_pos;
        length = apng_prog_read_be32(dec->buffer + dec->parse_pos + 0u);
        type = apng_prog_read_be32(dec->buffer + dec->parse_pos + 4u);
        if (length > 0x7FFFFFFFu)
            return PNG_DEC_ERR_FORMAT;

        max_chunk_bytes = apng_prog_limit_or_default(dec->options.max_chunk_bytes, 1024u * 1024u * 16u);
        if (length > max_chunk_bytes)
            return PNG_DEC_ERR_CHUNK_TOO_LARGE;
        if (dec->options.max_chunks != 0u && dec->chunk_count >= dec->options.max_chunks)
            return PNG_DEC_ERR_TOO_MANY_CHUNKS;

        if (!apng_prog_safe_add_u32(dec->parse_pos, 12u + length, &next_pos))
            return PNG_DEC_ERR_CHUNK_TOO_LARGE;
        if (next_pos > dec->size)
            break;

        err = apng_prog_validate_crc(dec->buffer + dec->parse_pos + 4u,
                                     length,
                                     dec->buffer + dec->parse_pos + 8u + length);
        if (err != PNG_DEC_OK)
            return err;

        dec->chunk_count += 1u;

        err = apng_prog_handle_chunk(dec,
                                     type,
                                     dec->buffer + dec->parse_pos + 8u,
                                     length,
                                     dec->buffer + dec->parse_pos,
                                     12u + length);
        if (err == PNG_DEC_PAUSED || err == PNG_DEC_YIELDED)
        {
            dec->active_chunk_waiting_resume = 1;
            dec->active_chunk_start = dec->parse_pos;
            dec->active_chunk_next_pos = next_pos;
            dec->active_chunk_type = type;
            if (err == PNG_DEC_YIELDED || dec->yield_requested)
                return apng_prog_budget_hit_result(dec);
            return apng_prog_pause_here(dec, dec->parse_pos);
        }
        if (err != PNG_DEC_OK)
            return err;

        dec->parse_pos = next_pos;

        err = apng_prog_emit_chunk_callback(dec, type, length, chunk_pos);
        if (err == PNG_DEC_PAUSED)
            return apng_prog_pause_here(dec, dec->parse_pos);
        if (err != PNG_DEC_OK)
            return err;

        err = apng_prog_note_parse_bytes(dec, next_pos - chunk_pos);
        if (err != PNG_DEC_OK)
            return err;

        if (dec->pause_requested)
            return apng_prog_pause_here(dec, dec->parse_pos);

        if (apng_prog_chunk_is_type(type, 'I', 'E', 'N', 'D'))
        {
            if (dec->options.strict_trailing_data && dec->size != dec->parse_pos)
                return PNG_DEC_ERR_FORMAT;
            return apng_prog_finish_stream(dec);
        }
    }

    return PNG_DEC_OK;
}

int png_apng_decoder_init(png_apng_decoder** dec_out, png_zlib_decompress_func zfunc)
{
    png_apng_decoder* dec;

    if (!dec_out || !zfunc)
        return PNG_DEC_ERR_FORMAT;
    *dec_out = 0;

    dec = (png_apng_decoder*)png_mem89_alloc(sizeof(*dec));
    if (!dec)
        return PNG_DEC_ERR_OOM;
    memset(dec, 0, sizeof(*dec));
    dec->zfunc = zfunc;
    png_decode_options_init(&dec->options);
    png_progressive_control_init(&dec->prog_ctl);
    *dec_out = dec;
    return PNG_DEC_OK;
}

int png_apng_decoder_set_options(png_apng_decoder* dec, const png_decode_options* options)
{
    if (!dec)
        return PNG_DEC_ERR_FORMAT;
    if (dec->size != 0u || dec->parse_pos != 0u || dec->seen_IHDR)
        return PNG_DEC_ERR_FORMAT;
    if (options)
        dec->options = *options;
    else
        png_decode_options_init(&dec->options);
    dec->options_set = 1;
    return PNG_DEC_OK;
}

int png_apng_decoder_set_callbacks(png_apng_decoder* dec, const png_apng_progressive_callbacks* callbacks)
{
    if (!dec)
        return PNG_DEC_ERR_FORMAT;
    if (callbacks)
        dec->callbacks = *callbacks;
    else
        memset(&dec->callbacks, 0, sizeof(dec->callbacks));
    return PNG_DEC_OK;
}

int png_apng_decoder_set_progressive_control(png_apng_decoder* dec, const png_progressive_control* ctl)
{
    if (!dec)
        return PNG_DEC_ERR_FORMAT;
    if (dec->size != 0u || dec->parse_pos != 0u || dec->seen_IHDR)
        return PNG_DEC_ERR_FORMAT;
    if (ctl)
        dec->prog_ctl = *ctl;
    else
        png_progressive_control_init(&dec->prog_ctl);
    return PNG_DEC_OK;
}

int png_apng_decoder_feed_ex(png_apng_decoder* dec, const png_u8* data, png_u32 size, png_u32* consumed_out)
{
    int err;
    png_u32 old_size;
    png_u32 accept = size;
    png_u32 room;

    if (consumed_out)
        *consumed_out = 0u;

    if (!dec || (!data && size != 0u))
        return PNG_DEC_ERR_FORMAT;

    if (dec->parse_done)
    {
        if (size != 0u && dec->options.strict_trailing_data)
            return PNG_DEC_ERR_FORMAT;
        return PNG_DEC_DONE;
    }

    dec->paused = 0;
    apng_prog_work_reset(dec);
    apng_prog_maybe_compact_buffer(dec);

    room = png_apng_decoder_input_room(dec);
    if (room == 0u && size != 0u && png_apng_decoder_unprocessed_bytes(dec) != 0u)
    {
        dec->feed_active = 1;
        dec->feed_start_offset = dec->size;
        dec->feed_end_offset = dec->size;
        err = apng_prog_parse_available(dec);
        dec->feed_active = 0;
        apng_prog_maybe_compact_buffer(dec);
        if (err != PNG_DEC_OK)
            return err;
        room = png_apng_decoder_input_room(dec);
    }

    old_size = dec->size;
    if (dec->prog_ctl.max_feed_bytes != 0u && accept > dec->prog_ctl.max_feed_bytes)
        accept = dec->prog_ctl.max_feed_bytes;
    if (room != 0xFFFFFFFFu && accept > room)
        accept = room;

    if (accept != 0u)
    {
        err = apng_prog_decoder_reserve(dec, accept);
        if (err != PNG_DEC_OK)
            return err;
        memcpy(dec->buffer + dec->size, data, accept);
        dec->size += accept;
    }

    if (consumed_out)
        *consumed_out = accept;

    dec->feed_active = 1;
    dec->feed_start_offset = old_size;
    dec->feed_end_offset = old_size + accept;
    err = apng_prog_parse_available(dec);
    dec->feed_active = 0;
    apng_prog_maybe_compact_buffer(dec);

    if (err == PNG_DEC_OK && accept < size)
        return PNG_DEC_YIELDED;
    return err;
}

int png_apng_decoder_feed(png_apng_decoder* dec, const png_u8* data, png_u32 size)
{
    return png_apng_decoder_feed_ex(dec, data, size, (png_u32*)0);
}

png_u32 png_apng_decoder_process_data_pause(png_apng_decoder* dec, int save)
{
    if (!dec)
        return 0u;
    dec->pause_requested = 1;
    dec->pause_save = save ? 1 : 0;
    return apng_prog_compute_unprocessed_bytes(dec, dec->parse_pos);
}

png_u32 png_apng_decoder_pending_bytes(const png_apng_decoder* dec)
{
    if (!dec || !dec->paused || dec->pause_resume_offset >= dec->size)
        return 0u;
    return dec->size - dec->pause_resume_offset;
}

int png_apng_decoder_is_paused(const png_apng_decoder* dec)
{
    return dec ? dec->paused : 0;
}

int png_apng_decoder_take_animation(png_apng_decoder* dec, png_apng* out_apng)
{
    if (!dec || !out_apng)
        return PNG_DEC_ERR_FORMAT;
    if (!dec->parse_done)
        return PNG_DEC_DONE;

    *out_apng = dec->animation;
    memset(&dec->animation, 0, sizeof(dec->animation));
    dec->animation_frame_capacity = 0u;
    return PNG_DEC_OK;
}

void png_apng_decoder_free(png_apng_decoder* dec)
{
    if (!dec)
        return;
    apng_prog_decoder_release_buffers(dec);
    png_free_apng(&dec->animation);
    png_mem89_release(dec);
}
