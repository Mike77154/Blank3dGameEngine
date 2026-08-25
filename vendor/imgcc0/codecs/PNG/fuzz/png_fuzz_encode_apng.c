#include "fuzz_common.h"

int LLVMFuzzerTestOneInput(const unsigned char* data, png_u32 size)
{
    fuzz_cursor cur;
    png_encode_options opt;
    png_decode_options dopt;
    png_apng apng;
    png_apng_encode_frame frames[4];
    png_u8* frame_pixels[4];
    png_u32 frame_rowbytes[4];
    png_u32 canvas_width;
    png_u32 canvas_height;
    png_u32 frame_count;
    png_u32 i;
    png_u8* out_png;
    png_u32 out_png_size;
    int err;

    png_mem89_reset();

    if (!data || size > PNG_FUZZ_MAX_INPUT_BYTES)
        return 0;

    fuzz_cursor_init(&cur, data, size);
    memset(&apng, 0, sizeof(apng));
    memset(frames, 0, sizeof(frames));
    memset(frame_pixels, 0, sizeof(frame_pixels));
    memset(frame_rowbytes, 0, sizeof(frame_rowbytes));
    out_png = 0;
    out_png_size = 0u;

    canvas_width = fuzz_range(&cur, 1u, 64u);
    canvas_height = fuzz_range(&cur, 1u, 64u);
    frame_count = fuzz_range(&cur, 1u, 4u);

    for (i = 0u; i < frame_count; ++i)
    {
        png_u32 fw;
        png_u32 fh;
        if (i == 0u)
        {
            fw = canvas_width;
            fh = canvas_height;
            frames[i].x_offset = 0u;
            frames[i].y_offset = 0u;
        }
        else
        {
            fw = fuzz_range(&cur, 1u, canvas_width);
            fh = fuzz_range(&cur, 1u, canvas_height);
            frames[i].x_offset = fuzz_range(&cur, 0u, canvas_width - fw);
            frames[i].y_offset = fuzz_range(&cur, 0u, canvas_height - fh);
        }
        frames[i].width = fw;
        frames[i].height = fh;
        frames[i].delay_num = fuzz_take_u16(&cur);
        frames[i].delay_den = (png_u16)(fuzz_range(&cur, 1u, 1000u));
        frames[i].dispose_op = (png_u8)(fuzz_take_u8(&cur) % 3u);
        frames[i].blend_op = (png_u8)(fuzz_take_u8(&cur) % 2u);
        if (!fuzz_alloc_public_pixels(&cur, PNG_OUTPUT_RGBA8, fw, fh, &frame_pixels[i], &frame_rowbytes[i]))
        {
            for (i = 0u; i < 4u; ++i)
                png_mem89_release(frame_pixels[i]);
            return 0;
        }
        frames[i].pixels = frame_pixels[i];
        frames[i].stride_bytes = frame_rowbytes[i];
    }

    fuzz_init_encode_options(&opt);
    opt.color_type = PNG_COLOR_TRUECOLOR_ALPHA;
    opt.bit_depth = 8u;
    opt.input_format = PNG_OUTPUT_RGBA8;
    opt.max_apng_frames = PNG_FUZZ_MAX_APNG_FRAMES;
    opt.interlace_method = 0u;

    err = png_encode_apng_memory_ex_zlib(frames,
                                         frame_count,
                                         canvas_width,
                                         canvas_height,
                                         fuzz_range(&cur, 0u, 8u),
                                         &opt,
                                         &out_png,
                                         &out_png_size);
    for (i = 0u; i < 4u; ++i)
        png_mem89_release(frame_pixels[i]);
    if (err != PNG_DEC_OK)
        return 0;

    fuzz_init_decode_options(&dopt, &cur);
    dopt.max_apng_frames = PNG_FUZZ_MAX_APNG_FRAMES;
    if (png_decode_apng_memory_ex_zlib(out_png, out_png_size, &dopt, &apng) == PNG_DEC_OK)
        png_free_apng(&apng);
    png_free_file(out_png);
    return 0;
}
