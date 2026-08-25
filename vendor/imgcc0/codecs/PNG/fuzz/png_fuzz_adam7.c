#include "fuzz_common.h"

typedef struct fuzz_adam7_ctx_s {
    png_u32 rows;
} fuzz_adam7_ctx;

static void fuzz_adam7_row_cb(void* user_ptr,
                              png_u32 row_index,
                              const png_u8* row_data,
                              png_u32 rowbytes,
                              int pass)
{
    fuzz_adam7_ctx* ctx;
    (void)row_index;
    (void)row_data;
    (void)rowbytes;
    (void)pass;
    ctx = (fuzz_adam7_ctx*)user_ptr;
    if (ctx)
        ctx->rows += 1u;
}

int LLVMFuzzerTestOneInput(const unsigned char* data, png_u32 size)
{
    fuzz_cursor cur;
    png_encode_options eopt;
    png_decode_options dopt;
    png_progressive_control ctl;
    png_progressive_callbacks cb;
    png_decoder* dec;
    png_image img;
    fuzz_adam7_ctx ctx;
    png_u8* pixels;
    png_u32 rowbytes;
    png_u32 width;
    png_u32 height;
    png_u8 format;
    png_u8* png_data;
    png_u32 png_size;
    png_u32 offset;
    int guard;
    int err;

    png_mem89_reset();

    if (!data || size > PNG_FUZZ_MAX_INPUT_BYTES)
        return 0;

    fuzz_cursor_init(&cur, data, size);
    memset(&img, 0, sizeof(img));
    memset(&ctx, 0, sizeof(ctx));
    dec = 0;
    pixels = 0;
    png_data = 0;
    png_size = 0u;
    offset = 0u;
    guard = 0;

    width = fuzz_range(&cur, 1u, 64u);
    height = fuzz_range(&cur, 1u, 64u);
    format = (png_u8)(fuzz_bool(&cur) ? PNG_OUTPUT_RGBA8 : PNG_OUTPUT_RGBA16);
    if (!fuzz_alloc_public_pixels(&cur, format, width, height, &pixels, &rowbytes))
        return 0;

    fuzz_init_encode_options(&eopt);
    eopt.input_format = format;
    eopt.stride_bytes = rowbytes;
    eopt.color_type = PNG_COLOR_TRUECOLOR_ALPHA;
    eopt.bit_depth = (png_u8)(png_output_format_sample_depth(format) > 8u ? 16u : 8u);
    eopt.interlace_method = 1u;
    err = png_encode_memory_ex_zlib(pixels, width, height, &eopt, &png_data, &png_size);
    png_mem89_release(pixels);
    if (err != PNG_DEC_OK)
        return 0;

    fuzz_init_decode_options(&dopt, &cur);
    dopt.output_format = format;
    if (png_decode_memory_ex_zlib(png_data, png_size, &dopt, &img) == PNG_DEC_OK)
        png_free_image(&img);

    err = png_decoder_init(&dec, png_zlib_decompress);
    if (err != PNG_DEC_OK)
    {
        png_free_file(png_data);
        return 0;
    }
    err = png_decoder_set_options(dec, &dopt);
    if (err != PNG_DEC_OK)
    {
        png_decoder_free(dec);
        png_free_file(png_data);
        return 0;
    }
    fuzz_init_progressive_control(&ctl, &cur);
    ctl.max_feed_bytes = fuzz_range(&cur, 1u, 64u);
    err = png_decoder_set_progressive_control(dec, &ctl);
    if (err != PNG_DEC_OK)
    {
        png_decoder_free(dec);
        png_free_file(png_data);
        return 0;
    }
    memset(&cb, 0, sizeof(cb));
    cb.user_ptr = &ctx;
    cb.row_fn = fuzz_adam7_row_cb;
    err = png_decoder_set_callbacks(dec, &cb);
    if (err != PNG_DEC_OK)
    {
        png_decoder_free(dec);
        png_free_file(png_data);
        return 0;
    }

    while (guard++ < 4096)
    {
        png_progressive_poll_state poll;
        png_u32 consumed;
        png_u32 take;
        consumed = 0u;
        png_progressive_poll_state_init(&poll);
        err = png_decoder_poll(dec, &poll);
        if (err != PNG_DEC_OK)
            break;
        if ((poll.events & PNG_PROGRESSIVE_POLL_DONE) != 0u)
            break;
        if ((poll.events & PNG_PROGRESSIVE_POLL_CAN_DRAIN) != 0u)
        {
            err = png_decoder_feed_ex(dec, 0, 0u, &consumed);
        }
        else if ((poll.events & PNG_PROGRESSIVE_POLL_WANT_INPUT) != 0u && offset < png_size)
        {
            take = poll.suggested_read_bytes;
            if (take == 0u)
                take = fuzz_range(&cur, 1u, 32u);
            if (take > png_size - offset)
                take = png_size - offset;
            err = png_decoder_feed_ex(dec, png_data + offset, take, &consumed);
            offset += consumed;
        }
        else
        {
            break;
        }
        if (err == PNG_DEC_DONE)
            break;
        if (err != PNG_DEC_OK && err != PNG_DEC_YIELDED)
            break;
    }

    if (png_decoder_take_image(dec, &img) == PNG_DEC_OK)
        png_free_image(&img);
    png_decoder_free(dec);
    png_free_file(png_data);
    return 0;
}
