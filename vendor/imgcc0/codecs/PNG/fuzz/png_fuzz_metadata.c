#include "fuzz_common.h"

int LLVMFuzzerTestOneInput(const unsigned char* data, png_u32 size)
{
    fuzz_cursor cur;
    png_encode_options opt;
    png_decode_options dopt;
    png_image img;
    png_text_entry texts[3];
    png_splt_palette splt;
    png_splt_entry splt_entries[4];
    png_u8 palette[16u * 3u];
    png_u8 trns[16u];
    png_u16 hist[16u];
    png_u8* pixels;
    png_u32 stride;
    png_u32 width;
    png_u32 height;
    png_u32 palette_entries;
    png_u8 iccp_profile[64];
    png_u8 exif_data[8];
    char keyword0[] = "Title";
    char keyword1[] = "Comment";
    char keyword2[] = "Caption";
    char text0[] = "fuzz-title";
    char text1[] = "fuzz-ztext";
    char text2[] = "fuzz-itxt";
    char lang2[] = "en-US";
    char trkey2[] = "Caption";
    char splt_name[] = "fuzz-splt";
    char scal_w[] = "1.25";
    char scal_h[] = "2.5";
    char iccp_name[] = "fuzz-icc";
    png_u8* out_png;
    png_u32 out_png_size;
    png_u32 i;
    int err;

    png_mem89_reset();

    if (!data || size > PNG_FUZZ_MAX_INPUT_BYTES)
        return 0;

    fuzz_cursor_init(&cur, data, size);
    memset(&img, 0, sizeof(img));
    memset(texts, 0, sizeof(texts));
    memset(&splt, 0, sizeof(splt));
    pixels = 0;
    out_png = 0;
    out_png_size = 0u;

    width = fuzz_range(&cur, 1u, 48u);
    height = fuzz_range(&cur, 1u, 48u);
    palette_entries = fuzz_range(&cur, 2u, 16u);
    if (!fuzz_alloc_indexed_pixels(&cur, width, height, palette_entries, &pixels, &stride))
        return 0;

    fuzz_fill_palette(&cur, palette, palette_entries);
    for (i = 0u; i < palette_entries; ++i)
    {
        trns[i] = (png_u8)(255u - (i * 11u));
        hist[i] = (png_u16)(1u + (fuzz_cycle_byte(&cur, cur.pos + i) % 1024u));
    }

    splt.name = splt_name;
    splt.sample_depth = 8u;
    splt.entries = splt_entries;
    splt.entry_count = 4u;
    for (i = 0u; i < 4u; ++i)
    {
        splt_entries[i].red = (png_u16)(i * 50u + 10u);
        splt_entries[i].green = (png_u16)(i * 70u + 20u);
        splt_entries[i].blue = (png_u16)(i * 90u + 30u);
        splt_entries[i].alpha = (png_u16)(255u - i * 30u);
        splt_entries[i].frequency = (png_u16)(1u + i * 3u);
    }

    memset(iccp_profile, 0, sizeof(iccp_profile));
    fuzz_fill_bytes(&cur, iccp_profile, sizeof(iccp_profile));
    exif_data[0] = 0x49u;
    exif_data[1] = 0x49u;
    exif_data[2] = 0x2Au;
    exif_data[3] = 0x00u;
    exif_data[4] = 0x08u;
    exif_data[5] = 0x00u;
    exif_data[6] = 0x00u;
    exif_data[7] = 0x00u;

    texts[0].keyword = keyword0;
    texts[0].text = text0;
    texts[0].compression = 0u;
    texts[1].keyword = keyword1;
    texts[1].text = text1;
    texts[1].compression = 1u;
    texts[2].keyword = keyword2;
    texts[2].text = text2;
    texts[2].language_tag = lang2;
    texts[2].translated_keyword = trkey2;
    texts[2].compression = 2u;

    fuzz_init_encode_options(&opt);
    opt.color_type = PNG_COLOR_INDEXED;
    opt.bit_depth = 8u;
    opt.palette = palette;
    opt.palette_entries = palette_entries;
    opt.trns_data = trns;
    opt.trns_size = palette_entries;
    opt.hist_entries = hist;
    opt.hist_count = palette_entries;
    opt.splt_palettes = &splt;
    opt.splt_palette_count = 1u;
    opt.text_entries = texts;
    opt.text_count = 3u;
    opt.write_iCCP = 1;
    opt.iccp_name = iccp_name;
    opt.iccp_profile = iccp_profile;
    opt.iccp_profile_size = sizeof(iccp_profile);
    opt.write_eXIf = 1;
    opt.exif_profile = exif_data;
    opt.exif_profile_size = sizeof(exif_data);
    opt.write_pHYs = 1;
    opt.pHYs_ppu_x = 3780u;
    opt.pHYs_ppu_y = 3780u;
    opt.pHYs_unit = 1u;
    opt.write_oFFs = 1;
    opt.offset_x = 3;
    opt.offset_y = -2;
    opt.offset_unit = 0u;
    opt.write_sCAL = 1;
    opt.scal_unit = 1u;
    opt.scal_pixel_width = scal_w;
    opt.scal_pixel_height = scal_h;
    opt.write_sTER = 1;
    opt.ster_mode = (png_u8)(fuzz_take_u8(&cur) % 2u);
    opt.write_tIME = 1;
    opt.time_year = 2026u;
    opt.time_month = 3u;
    opt.time_day = 9u;
    opt.time_hour = 12u;
    opt.time_minute = 34u;
    opt.time_second = 56u;

    err = png_encode_memory_ex_zlib(pixels, width, height, &opt, &out_png, &out_png_size);
    png_mem89_release(pixels);
    if (err != PNG_DEC_OK)
        return 0;

    fuzz_init_decode_options(&dopt, &cur);
    dopt.output_format = PNG_OUTPUT_RGBA8;
    dopt.keep_text = 1;
    dopt.keep_unknown_chunks = PNG_DEC_KEEP_UNKNOWN_ALL;
    if (png_decode_memory_ex_zlib(out_png, out_png_size, &dopt, &img) == PNG_DEC_OK)
        png_free_image(&img);
    png_free_file(out_png);
    return 0;
}
