#include "fuzz_common.h"

typedef struct fuzz_prog_ctx_s {
    png_decoder* dec;
    int want_pause;
    int pause_save;
    int pause_requested;
    png_u32 rows_seen;
    png_u32 chunks_seen;
} fuzz_prog_ctx;

static void fuzz_prog_row_cb(void* user_ptr,
                             png_u32 row_index,
                             const png_u8* row_data,
                             png_u32 rowbytes,
                             int pass)
{
    fuzz_prog_ctx* ctx;
    (void)row_index;
    (void)row_data;
    (void)rowbytes;
    (void)pass;
    ctx = (fuzz_prog_ctx*)user_ptr;
    if (!ctx)
        return;
    ctx->rows_seen += 1u;
    if (ctx->want_pause && !ctx->pause_requested && ctx->dec)
    {
        ctx->pause_requested = 1;
        (void)png_decoder_process_data_pause(ctx->dec, ctx->pause_save);
    }
}

static void fuzz_prog_chunk_cb(void* user_ptr, const png_chunk_progress_info* info)
{
    fuzz_prog_ctx* ctx;
    (void)info;
    ctx = (fuzz_prog_ctx*)user_ptr;
    if (!ctx)
        return;
    ctx->chunks_seen += 1u;
}

int LLVMFuzzerTestOneInput(const unsigned char* data, png_u32 size)
{
    fuzz_cursor cur;
    png_decoder* dec;
    png_decode_options opt;
    png_progressive_control ctl;
    png_progressive_callbacks cb;
    png_progressive_poll_state poll;
    fuzz_prog_ctx ctx;
    png_image out;
    png_u32 offset;
    int guard;
    int err;

    png_mem89_reset();

    if (!data || size > PNG_FUZZ_MAX_INPUT_BYTES)
        return 0;

    fuzz_cursor_init(&cur, data, size);
    dec = 0;
    memset(&ctx, 0, sizeof(ctx));
    memset(&out, 0, sizeof(out));
    offset = 0u;
    guard = 0;

    err = png_decoder_init(&dec, png_zlib_decompress);
    if (err != PNG_DEC_OK)
        return 0;

    fuzz_init_decode_options(&opt, &cur);
    err = png_decoder_set_options(dec, &opt);
    if (err != PNG_DEC_OK)
    {
        png_decoder_free(dec);
        return 0;
    }

    fuzz_init_progressive_control(&ctl, &cur);
    err = png_decoder_set_progressive_control(dec, &ctl);
    if (err != PNG_DEC_OK)
    {
        png_decoder_free(dec);
        return 0;
    }

    memset(&cb, 0, sizeof(cb));
    ctx.dec = dec;
    ctx.want_pause = fuzz_bool(&cur);
    ctx.pause_save = fuzz_bool(&cur);
    cb.user_ptr = &ctx;
    cb.row_fn = fuzz_prog_row_cb;
    cb.chunk_fn = fuzz_prog_chunk_cb;
    err = png_decoder_set_callbacks(dec, &cb);
    if (err != PNG_DEC_OK)
    {
        png_decoder_free(dec);
        return 0;
    }

    while (guard++ < 4096)
    {
        png_u32 consumed;
        png_u32 take;

        consumed = 0u;
        png_progressive_poll_state_init(&poll);
        err = png_decoder_poll(dec, &poll);
        if (err != PNG_DEC_OK)
            break;
        if ((poll.events & PNG_PROGRESSIVE_POLL_DONE) != 0u)
            break;

        if (png_decoder_is_paused(dec))
        {
            if ((fuzz_take_u8(&cur) & 1u) != 0u)
                (void)png_decoder_process_data_skip(dec);
            err = png_decoder_feed_ex(dec, 0, 0u, &consumed);
        }
        else if ((poll.events & PNG_PROGRESSIVE_POLL_CAN_DRAIN) != 0u)
        {
            err = png_decoder_feed_ex(dec, 0, 0u, &consumed);
        }
        else if ((poll.events & PNG_PROGRESSIVE_POLL_WANT_INPUT) != 0u && offset < (png_u32)size)
        {
            take = poll.suggested_read_bytes;
            if (take == 0u)
                take = fuzz_range(&cur, 1u, 64u);
            if (take > (png_u32)size - offset)
                take = (png_u32)size - offset;
            if (take == 0u)
                break;
            err = png_decoder_feed_ex(dec, data + offset, take, &consumed);
            offset += consumed;
        }
        else
        {
            break;
        }

        if (err == PNG_DEC_DONE)
            break;
        if (err != PNG_DEC_OK && err != PNG_DEC_YIELDED && err != PNG_DEC_PAUSED)
            break;
    }

    if (png_decoder_take_image(dec, &out) == PNG_DEC_OK)
        png_free_image(&out);
    png_decoder_free(dec);
    return 0;
}
