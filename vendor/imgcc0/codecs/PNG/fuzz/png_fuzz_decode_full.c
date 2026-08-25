#include "fuzz_common.h"

int LLVMFuzzerTestOneInput(const unsigned char* data, png_u32 size)
{
    fuzz_cursor cur;
    png_decode_options opt;
    png_conversion_limits limits;
    png_image img;
    int err;

    png_mem89_reset();

    if (!data || size > PNG_FUZZ_MAX_INPUT_BYTES)
        return 0;

    fuzz_cursor_init(&cur, data, size);
    fuzz_init_decode_options(&opt, &cur);
    memset(&img, 0, sizeof(img));

    err = png_decode_memory_ex_zlib(data, (png_u32)size, &opt, &img);
    if (err == PNG_DEC_OK)
    {
        png_u8 fmt;
        fmt = fuzz_pick_output_format(fuzz_take_u8(&cur));
        png_conversion_limits_init(&limits);
        limits.max_output_bytes = PNG_FUZZ_MAX_IMAGE_BYTES;
        limits.max_expansion = 4u;
        limits.max_output_sample_depth = (png_u8)(fuzz_bool(&cur) ? 16u : 8u);
        (void)png_image_convert_format_ex(&img, fmt, &limits);
        png_free_image(&img);
    }

    return 0;
}
