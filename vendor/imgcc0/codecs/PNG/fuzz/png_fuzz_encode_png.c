#include "fuzz_common.h"

static void fuzz_choose_encode_target(png_u8 public_format,
                                      png_u8* out_color_type,
                                      png_u8* out_bit_depth)
{
    if (!out_color_type || !out_bit_depth)
        return;
    switch (public_format)
    {
        case PNG_OUTPUT_G8:
            *out_color_type = PNG_COLOR_GRAYSCALE;
            *out_bit_depth = 8u;
            break;
        case PNG_OUTPUT_G16:
            *out_color_type = PNG_COLOR_GRAYSCALE;
            *out_bit_depth = 16u;
            break;
        case PNG_OUTPUT_GA8:
            *out_color_type = PNG_COLOR_GRAYSCALE_ALPHA;
            *out_bit_depth = 8u;
            break;
        case PNG_OUTPUT_GA16:
            *out_color_type = PNG_COLOR_GRAYSCALE_ALPHA;
            *out_bit_depth = 16u;
            break;
        case PNG_OUTPUT_RGB16:
            *out_color_type = PNG_COLOR_TRUECOLOR;
            *out_bit_depth = 16u;
            break;
        case PNG_OUTPUT_RGBA16:
            *out_color_type = PNG_COLOR_TRUECOLOR_ALPHA;
            *out_bit_depth = 16u;
            break;
        default:
            if (png_output_format_channels(public_format) == 3u)
            {
                *out_color_type = PNG_COLOR_TRUECOLOR;
                *out_bit_depth = (png_output_format_sample_depth(public_format) > 8u) ? 16u : 8u;
            }
            else
            {
                *out_color_type = PNG_COLOR_TRUECOLOR_ALPHA;
                *out_bit_depth = (png_output_format_sample_depth(public_format) > 8u) ? 16u : 8u;
            }
            break;
    }
}

int LLVMFuzzerTestOneInput(const unsigned char* data, png_u32 size)
{
    fuzz_cursor cur;
    png_encode_options opt;
    png_decode_options dopt;
    png_image img;
    png_u8* pixels;
    png_u32 rowbytes;
    png_u32 width;
    png_u32 height;
    png_u8 input_format;
    png_u8* out_png;
    png_u32 out_png_size;
    int err;

    png_mem89_reset();

    if (!data || size > PNG_FUZZ_MAX_INPUT_BYTES)
        return 0;

    fuzz_cursor_init(&cur, data, size);
    pixels = 0;
    out_png = 0;
    out_png_size = 0u;
    memset(&img, 0, sizeof(img));

    width = fuzz_range(&cur, 1u, 64u);
    height = fuzz_range(&cur, 1u, 64u);
    input_format = fuzz_pick_public_input_format(fuzz_take_u8(&cur));
    if (!fuzz_alloc_public_pixels(&cur, input_format, width, height, &pixels, &rowbytes))
        return 0;

    fuzz_init_encode_options(&opt);
    opt.input_format = input_format;
    opt.stride_bytes = rowbytes;
    fuzz_choose_encode_target(input_format, &opt.color_type, &opt.bit_depth);
    opt.interlace_method = (png_u8)(fuzz_bool(&cur) ? 1u : 0u);
    opt.write_gAMA = fuzz_bool(&cur);
    opt.image_gamma = png_fixed89_from_ratio(45455, 100000);
    opt.write_pHYs = fuzz_bool(&cur);
    opt.pHYs_ppu_x = fuzz_range(&cur, 1u, 10000u);
    opt.pHYs_ppu_y = fuzz_range(&cur, 1u, 10000u);
    opt.pHYs_unit = (png_u8)(fuzz_take_u8(&cur) % 2u);

    err = png_encode_memory_ex_zlib(pixels, width, height, &opt, &out_png, &out_png_size);
    png_mem89_release(pixels);
    if (err != PNG_DEC_OK)
        return 0;

    fuzz_init_decode_options(&dopt, &cur);
    if (png_decode_memory_ex_zlib(out_png, out_png_size, &dopt, &img) == PNG_DEC_OK)
        png_free_image(&img);
    png_free_file(out_png);
    return 0;
}
