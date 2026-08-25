#include "fuzz_common.h"

int LLVMFuzzerTestOneInput(const unsigned char* data, png_u32 size)
{
    fuzz_cursor cur;
    png_encode_options eopt;
    png_decode_options dopt;
    png_conversion_limits limits;
    png_image img;
    png_u8* pixels;
    png_u32 rowbytes;
    png_u32 width;
    png_u32 height;
    png_u8 input_format;
    png_u8* png_data;
    png_u32 png_size;
    int i;

    png_mem89_reset();

    if (!data || size > PNG_FUZZ_MAX_INPUT_BYTES)
        return 0;

    fuzz_cursor_init(&cur, data, size);
    memset(&img, 0, sizeof(img));
    pixels = 0;
    png_data = 0;
    png_size = 0u;

    width = fuzz_range(&cur, 1u, 48u);
    height = fuzz_range(&cur, 1u, 48u);
    input_format = fuzz_pick_public_input_format(fuzz_take_u8(&cur));
    if (!fuzz_alloc_public_pixels(&cur, input_format, width, height, &pixels, &rowbytes))
        return 0;

    fuzz_init_encode_options(&eopt);
    eopt.input_format = input_format;
    eopt.stride_bytes = rowbytes;
    eopt.color_type = (png_u8)(png_output_format_channels(input_format) >= 3u ? PNG_COLOR_TRUECOLOR_ALPHA : PNG_COLOR_GRAYSCALE_ALPHA);
    eopt.bit_depth = (png_u8)(png_output_format_sample_depth(input_format) > 8u ? 16u : 8u);
    if (png_output_format_channels(input_format) == 1u)
        eopt.color_type = PNG_COLOR_GRAYSCALE;
    if (png_output_format_channels(input_format) == 3u)
        eopt.color_type = PNG_COLOR_TRUECOLOR;
    if (png_output_format_channels(input_format) == 2u)
        eopt.color_type = PNG_COLOR_GRAYSCALE_ALPHA;
    eopt.interlace_method = 0u;
    if (png_encode_memory_ex_zlib(pixels, width, height, &eopt, &png_data, &png_size) != PNG_DEC_OK)
    {
        png_mem89_release(pixels);
        return 0;
    }
    png_mem89_release(pixels);

    fuzz_init_decode_options(&dopt, &cur);
    dopt.output_format = fuzz_pick_output_format(fuzz_take_u8(&cur));
    if (png_decode_memory_ex_zlib(png_data, png_size, &dopt, &img) == PNG_DEC_OK)
    {
        for (i = 0; i < 4; ++i)
        {
            png_u8 fmt;
            fmt = fuzz_pick_output_format(fuzz_take_u8(&cur));
            png_conversion_limits_init(&limits);
            limits.max_output_bytes = PNG_FUZZ_MAX_IMAGE_BYTES;
            limits.max_expansion = (png_u32)(1u + (fuzz_take_u8(&cur) % 8u));
            limits.max_output_sample_depth = (png_u8)(fuzz_bool(&cur) ? 16u : 8u);
            (void)png_image_convert_format_ex(&img, fmt, &limits);
        }
        png_free_image(&img);
    }
    png_free_file(png_data);
    return 0;
}
