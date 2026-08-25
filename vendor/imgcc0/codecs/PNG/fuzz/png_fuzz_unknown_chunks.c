#include "fuzz_common.h"

static void fuzz_fill_chunk_type(png_u8 out_type[5], png_u8 selector)
{
    static const char* types[] = {
        "vpAg", "faKe", "stEg", "prIv", "raRe", "miNt"
    };
    const char* src;
    src = types[(unsigned int)selector % (sizeof(types) / sizeof(types[0]))];
    out_type[0] = (png_u8)src[0];
    out_type[1] = (png_u8)src[1];
    out_type[2] = (png_u8)src[2];
    out_type[3] = (png_u8)src[3];
    out_type[4] = 0u;
}

int LLVMFuzzerTestOneInput(const unsigned char* data, png_u32 size)
{
    fuzz_cursor cur;
    png_encode_options opt;
    png_decode_options dopt;
    png_image img;
    png_u8* pixels;
    png_u32 rowbytes;
    png_unknown_chunk chunks[4];
    png_u8* chunk_data[4];
    png_u32 width;
    png_u32 height;
    png_u32 i;
    png_u8* out_png;
    png_u32 out_png_size;
    int err;

    png_mem89_reset();

    if (!data || size > PNG_FUZZ_MAX_INPUT_BYTES)
        return 0;

    fuzz_cursor_init(&cur, data, size);
    memset(&img, 0, sizeof(img));
    memset(chunks, 0, sizeof(chunks));
    memset(chunk_data, 0, sizeof(chunk_data));
    out_png = 0;
    out_png_size = 0u;

    width = fuzz_range(&cur, 1u, 32u);
    height = fuzz_range(&cur, 1u, 32u);
    pixels = 0;
    rowbytes = 0u;
    if (!fuzz_alloc_public_pixels(&cur, PNG_OUTPUT_RGBA8, width, height, &pixels, &rowbytes))
        return 0;

    for (i = 0u; i < 4u; ++i)
    {
        png_u32 sz;
        sz = fuzz_range(&cur, 0u, 64u);
        chunk_data[i] = (png_u8*)png_mem89_alloc(sz == 0u ? 1u : (png_u32)sz);
        if (!chunk_data[i])
            break;
        fuzz_fill_bytes(&cur, chunk_data[i], sz);
        fuzz_fill_chunk_type(chunks[i].type, fuzz_take_u8(&cur));
        chunks[i].data = chunk_data[i];
        chunks[i].size = sz;
        chunks[i].location = (png_u8)((i & 1u) ? PNG_CHUNK_POS_BEFORE_IEND : PNG_CHUNK_POS_AFTER_IHDR);
        chunks[i].safe_to_copy = (png_u8)(fuzz_bool(&cur) ? 1u : 0u);
    }

    fuzz_init_encode_options(&opt);
    opt.input_format = PNG_OUTPUT_RGBA8;
    opt.stride_bytes = rowbytes;
    opt.color_type = PNG_COLOR_TRUECOLOR_ALPHA;
    opt.bit_depth = 8u;
    opt.unknown_chunks = chunks;
    opt.unknown_chunk_count = 4u;
    err = png_encode_memory_ex_zlib(pixels, width, height, &opt, &out_png, &out_png_size);
    png_mem89_release(pixels);
    for (i = 0u; i < 4u; ++i)
        png_mem89_release(chunk_data[i]);
    if (err != PNG_DEC_OK)
        return 0;

    fuzz_init_decode_options(&dopt, &cur);
    dopt.keep_unknown_chunks = PNG_DEC_KEEP_UNKNOWN_ALL;
    dopt.output_format = PNG_OUTPUT_RGBA8;
    if (png_decode_memory_ex_zlib(out_png, out_png_size, &dopt, &img) == PNG_DEC_OK)
    {
        png_u8* roundtrip_png;
        png_u32 roundtrip_png_size;
        png_encode_options ropt;
        roundtrip_png = 0;
        roundtrip_png_size = 0u;
        fuzz_init_encode_options(&ropt);
        ropt.input_format = PNG_OUTPUT_RGBA8;
        ropt.stride_bytes = img.pixel_rowbytes;
        ropt.color_type = PNG_COLOR_TRUECOLOR_ALPHA;
        ropt.bit_depth = 8u;
        ropt.unknown_chunks = img.unknown_chunks;
        ropt.unknown_chunk_count = img.unknown_chunk_count;
        if (png_encode_memory_ex_zlib(img.pixels, img.width, img.height, &ropt, &roundtrip_png, &roundtrip_png_size) == PNG_DEC_OK)
            png_free_file(roundtrip_png);
        png_free_image(&img);
    }
    png_free_file(out_png);
    return 0;
}
