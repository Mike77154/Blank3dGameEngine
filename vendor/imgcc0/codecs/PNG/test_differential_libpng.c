#include "png_mem89.h"
#include <stdio.h>
#include <string.h>

#define png_image libpng_simple_image
#define png_unknown_chunk libpng_unknown_chunk
#include <png.h>
#undef png_unknown_chunk
#undef png_image

#include "png_decoder.h"

#ifndef PNG_LIBPNG_VER_STRING
#error "libpng headers are required for test_differential_libpng.c"
#endif

#define PNG_DIFF_MAX_FILE_BYTES (16u * 1024u * 1024u)

typedef struct file_buffer_s {
    png_u8* data;
    png_u32 size;
} file_buffer;

typedef struct ref_png_image_s {
    png_u32 width;
    png_u32 height;
    int bit_depth;
    int color_type;
    int interlace_method;
    int has_gamma;
    png_fixed89 gamma;
    int has_srgb;
    int srgb_intent;
    int has_phys;
    png_u32 phys_x;
    png_u32 phys_y;
    int phys_unit;
    int has_iccp;
    png_u32 iccp_profile_size;
    int text_count;
    int has_hist;
    png_u32 hist_count;
    int splt_count;
    int unknown_count;
    png_u8* rgba;
    png_u32 rowbytes;
} ref_png_image;

typedef struct ref_mem_reader_s {
    const png_u8* data;
    png_u32 size;
    png_u32 offset;
} ref_mem_reader;

static void file_buffer_init(file_buffer* fb)
{
    fb->data = (png_u8*)0;
    fb->size = 0u;
}

static void file_buffer_free(file_buffer* fb)
{
    if (fb->data)
        png_mem89_release(fb->data);
    fb->data = (png_u8*)0;
    fb->size = 0u;
}

static void ref_png_image_init(ref_png_image* img)
{
    memset(img, 0, sizeof(*img));
}

static void ref_png_image_free(ref_png_image* img)
{
    if (img->rgba)
        png_mem89_release(img->rgba);
    img->rgba = (png_u8*)0;
    img->rowbytes = 0u;
}

static int read_file_data(const char* path, file_buffer* out)
{
    FILE* fp;
    png_u8* data;
    png_u32 used;
    int ch;

    file_buffer_init(out);
    fp = fopen(path, "rb");
    if (!fp)
        return 0;

    data = (png_u8*)png_mem89_alloc(PNG_DIFF_MAX_FILE_BYTES);
    if (!data)
    {
        fclose(fp);
        return 0;
    }

    used = 0u;
    while ((ch = fgetc(fp)) != EOF)
    {
        if (used >= PNG_DIFF_MAX_FILE_BYTES)
        {
            fclose(fp);
            png_mem89_release(data);
            return 0;
        }
        data[used++] = (png_u8)ch;
    }
    fclose(fp);

    out->data = data;
    out->size = used;
    return 1;
}

static png_voidp libpng_mem89_alloc_fn(png_structp png_ptr, png_alloc_size_t bytes)
{
    (void)png_ptr;
    if (bytes == 0u)
        bytes = 1u;
    if (bytes > (png_alloc_size_t)0xFFFFFFFFu)
        return (png_voidp)0;
    return (png_voidp)png_mem89_alloc((png_u32)bytes);
}

static void libpng_mem89_free_fn(png_structp png_ptr, png_voidp ptr)
{
    (void)png_ptr;
    png_mem89_release(ptr);
}

static void libpng_error_fn(png_structp png_ptr, png_const_charp msg)
{
    (void)msg;
    longjmp(png_jmpbuf(png_ptr), 1);
}

static void libpng_warning_fn(png_structp png_ptr, png_const_charp msg)
{
    (void)png_ptr;
    (void)msg;
}

static void libpng_mem_read_fn(png_structp png_ptr, png_bytep out_bytes, png_size_t byte_count)
{
    ref_mem_reader* reader;
    png_u32 count32;
    reader = (ref_mem_reader*)png_get_io_ptr(png_ptr);
    if (byte_count > (png_size_t)0xFFFFFFFFu)
        png_error(png_ptr, "read request exceeds 32-bit protocol");
    count32 = (png_u32)byte_count;
    if (!reader || count32 > reader->size - reader->offset)
        png_error(png_ptr, "read beyond end of memory buffer");
    memcpy(out_bytes, reader->data + reader->offset, count32);
    reader->offset += count32;
}

static int ref_collect_text_count(png_structp png_ptr, png_infop info_ptr)
{
    png_textp text_ptr = (png_textp)0;
    int num_text = 0;
    if (!info_ptr)
        return 0;
    (void)png_get_text(png_ptr, info_ptr, &text_ptr, &num_text);
    return num_text;
}

static int ref_decode_with_libpng_ex(const png_u8* data,
                                     png_u32 size,
                                     const png_u8* keep_chunks,
                                     int keep_chunk_count,
                                     ref_png_image* out)
{
    ref_mem_reader reader;
    png_structp png_ptr = (png_structp)0;
    png_infop info_ptr = (png_infop)0;
    png_infop end_info = (png_infop)0;
    png_uint_32 width = 0u, height = 0u;
    int bit_depth = 0, color_type = 0, interlace_method = 0;
    int compression_method = 0, filter_method = 0;
    png_uint_32 phys_x = 0u, phys_y = 0u;
    int phys_unit = 0;
    png_fixed_point gamma_fixed = 0;
    int srgb_intent = 0;
    png_charp iccp_name = (png_charp)0;
    int iccp_comp = 0;
    png_bytep iccp_profile = (png_bytep)0;
    png_uint_32 iccp_size = 0u;
    png_uint_16p hist = (png_uint_16p)0;
    png_colorp palette = (png_colorp)0;
    int palette_entries = 0;
    png_sPLT_tp splt_entries = (png_sPLT_tp)0;
    png_unknown_chunkp unknown_entries = (png_unknown_chunkp)0;
    png_bytep* rows = (png_bytep*)0;
    png_u8* rgba = (png_u8*)0;
    png_u32 rowbytes = 0u;
    png_u32 i;
    int has_trns = 0;

    ref_png_image_init(out);

    reader.data = data;
    reader.size = size;
    reader.offset = 0u;

    png_ptr = png_create_read_struct_2(PNG_LIBPNG_VER_STRING, (png_voidp)0,
                                       libpng_error_fn, libpng_warning_fn,
                                       (png_voidp)0, libpng_mem89_alloc_fn,
                                       libpng_mem89_free_fn);
    if (!png_ptr)
        return 0;
    info_ptr = png_create_info_struct(png_ptr);
    end_info = png_create_info_struct(png_ptr);
    if (!info_ptr || !end_info)
    {
        png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
        return 0;
    }

    if (setjmp(png_jmpbuf(png_ptr)) != 0)
    {
        if (rows)
            png_mem89_release(rows);
        if (rgba)
            png_mem89_release(rgba);
        png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
        ref_png_image_init(out);
        return 0;
    }

    png_set_read_fn(png_ptr, &reader, libpng_mem_read_fn);
    if (keep_chunk_count > 0)
        png_set_keep_unknown_chunks(png_ptr, PNG_HANDLE_CHUNK_ALWAYS,
                                    (png_const_bytep)keep_chunks, keep_chunk_count);
    png_read_info(png_ptr, info_ptr);

    png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type,
                 &interlace_method, &compression_method, &filter_method);
    (void)compression_method;
    (void)filter_method;

    out->width = (png_u32)width;
    out->height = (png_u32)height;
    out->bit_depth = bit_depth;
    out->color_type = color_type;
    out->interlace_method = interlace_method;

    if (png_get_gAMA_fixed(png_ptr, info_ptr, &gamma_fixed) != 0)
    {
        out->has_gamma = 1;
        out->gamma = png_fixed89_from_ratio((signed int)gamma_fixed, 100000);
    }

    if (png_get_sRGB(png_ptr, info_ptr, &srgb_intent) != 0)
    {
        out->has_srgb = 1;
        out->srgb_intent = srgb_intent;
    }

    if (png_get_pHYs(png_ptr, info_ptr, &phys_x, &phys_y, &phys_unit) != 0)
    {
        out->has_phys = 1;
        out->phys_x = (png_u32)phys_x;
        out->phys_y = (png_u32)phys_y;
        out->phys_unit = phys_unit;
    }

    if (png_get_iCCP(png_ptr, info_ptr, &iccp_name, &iccp_comp, &iccp_profile, &iccp_size) != 0)
    {
        (void)iccp_name;
        (void)iccp_comp;
        (void)iccp_profile;
        out->has_iccp = 1;
        out->iccp_profile_size = (png_u32)iccp_size;
    }

    out->text_count = ref_collect_text_count(png_ptr, info_ptr);

    if (png_get_hIST(png_ptr, info_ptr, &hist) != 0)
    {
        out->has_hist = 1;
        if (png_get_PLTE(png_ptr, info_ptr, &palette, &palette_entries) != 0 && palette_entries > 0)
            out->hist_count = (png_u32)palette_entries;
        else
            out->hist_count = 0u;
        (void)hist;
    }

    out->splt_count = png_get_sPLT(png_ptr, info_ptr, &splt_entries);
    if (out->splt_count < 0)
        out->splt_count = 0;

    has_trns = (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS) != 0);

    png_set_expand(png_ptr);
    if (bit_depth == 16)
        png_set_strip_16(png_ptr);
    if (color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
        png_set_gray_to_rgb(png_ptr);
    if ((color_type & PNG_COLOR_MASK_ALPHA) == 0 && !has_trns)
        png_set_filler(png_ptr, 0xFF, PNG_FILLER_AFTER);
    (void)png_set_interlace_handling(png_ptr);
    png_read_update_info(png_ptr, info_ptr);

    rowbytes = (png_u32)png_get_rowbytes(png_ptr, info_ptr);
    rgba = (png_u8*)png_mem89_alloc(rowbytes * height);
    rows = (png_bytep*)png_mem89_alloc(sizeof(png_bytep) * height);
    if ((height != 0u && !rgba) || (height != 0u && !rows))
        png_error(png_ptr, "out of memory allocating reference image");

    for (i = 0u; i < height; ++i)
        rows[i] = rgba + i * rowbytes;

    if (height != 0u)
        png_read_image(png_ptr, rows);
    png_read_end(png_ptr, end_info);

    out->text_count += ref_collect_text_count(png_ptr, end_info);
    if (keep_chunk_count > 0)
    {
        out->unknown_count += png_get_unknown_chunks(png_ptr, info_ptr, &unknown_entries);
        unknown_entries = (png_unknown_chunkp)0;
        out->unknown_count += png_get_unknown_chunks(png_ptr, end_info, &unknown_entries);
    }
    out->rgba = rgba;
    out->rowbytes = rowbytes;

    png_mem89_release(rows);
    png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
    return 1;
}

static int ref_decode_with_libpng(const png_u8* data, png_u32 size, ref_png_image* out)
{
    return ref_decode_with_libpng_ex(data, size, (const png_u8*)0, 0, out);
}

static int our_decode_full(const png_u8* data, png_u32 size, png_image* out)
{
    png_decode_options opt;
    png_decode_options_init(&opt);
    opt.output_format = PNG_OUTPUT_RGBA8;
    opt.keep_text = 1;
    opt.keep_unknown_chunks = PNG_DEC_KEEP_UNKNOWN_ALL;
    opt.max_chunk_bytes = 64u * 1024u * 1024u;
    opt.max_text_bytes = 32u * 1024u * 1024u;
    memset(out, 0, sizeof(*out));
    return png_decode_memory_ex_zlib(data, size, &opt, out);
}

static int our_decode_progressive(const png_u8* data, png_u32 size, png_image* out)
{
    png_decoder* dec = (png_decoder*)0;
    png_decode_options dopt;
    png_progressive_poll_state poll;
    png_u32 offset = 0u;
    int err;
    int guard = 0;

    memset(out, 0, sizeof(*out));
    err = png_decoder_init(&dec, png_zlib_decompress);
    if (err != PNG_DEC_OK)
        return err;

    png_decode_options_init(&dopt);
    dopt.output_format = PNG_OUTPUT_RGBA8;
    dopt.keep_text = 1;
    dopt.keep_unknown_chunks = PNG_DEC_KEEP_UNKNOWN_ALL;
    dopt.max_chunk_bytes = 64u * 1024u * 1024u;
    dopt.max_text_bytes = 32u * 1024u * 1024u;
    err = png_decoder_set_options(dec, &dopt);
    if (err != PNG_DEC_OK)
    {
        png_decoder_free(dec);
        return err;
    }

    while (offset < size && guard++ < 8192)
    {
        png_u32 consumed = 0u;
        png_u32 take = size - offset;
        if (take > 17u)
            take = 17u;
        err = png_decoder_feed_ex(dec, data + offset, take, &consumed);
        offset += consumed;
        if (err == PNG_DEC_OK)
            continue;
        if (err == PNG_DEC_DONE)
            break;
        png_decoder_free(dec);
        return err;
    }

    while (guard++ < 8192)
    {
        png_u32 consumed = 0u;
        err = png_decoder_poll(dec, &poll);
        if (err != PNG_DEC_OK)
        {
            png_decoder_free(dec);
            return err;
        }
        if ((poll.events & PNG_PROGRESSIVE_POLL_DONE) != 0u)
            break;
        err = png_decoder_feed_ex(dec, (const png_u8*)0, 0u, &consumed);
        if (err == PNG_DEC_OK)
            continue;
        if (err == PNG_DEC_DONE)
            break;
        png_decoder_free(dec);
        return err;
    }

    if (guard >= 8192)
    {
        png_decoder_free(dec);
        return PNG_DEC_ERR_WORK_BUDGET;
    }

    err = png_decoder_take_image(dec, out);
    png_decoder_free(dec);
    return err;
}

static int compare_pixels(const png_u8* a, const png_u8* b, png_u32 bytes)
{
    if (bytes == 0u)
        return 1;
    return memcmp(a, b, bytes) == 0;
}

static int compare_ref_and_ours(const char* path, const ref_png_image* ref, const png_image* ours)
{
    if (ref->width != ours->width || ref->height != ours->height)
    {
        printf("MISMATCH %s dimensions ref=%u x %u ours=%u x %u\n",
               path,
               (unsigned int)ref->width, (unsigned int)ref->height,
               (unsigned int)ours->width, (unsigned int)ours->height);
        return 0;
    }
    if ((int)ours->source_bit_depth != ref->bit_depth ||
        (int)ours->source_color_type != ref->color_type ||
        (int)ours->interlace_method != ref->interlace_method)
    {
        printf("MISMATCH %s header ref(bit=%d color=%d interlace=%d) ours(bit=%d color=%d interlace=%d)\n",
               path,
               ref->bit_depth, ref->color_type, ref->interlace_method,
               (int)ours->source_bit_depth,
               (int)ours->source_color_type,
               (int)ours->interlace_method);
        return 0;
    }
    if (ours->output_format != PNG_OUTPUT_RGBA8 || ours->pixel_rowbytes != ref->rowbytes)
    {
        printf("MISMATCH %s output format/rowbytes ours_format=%u ours_rowbytes=%u ref_rowbytes=%u\n",
               path,
               (unsigned)ours->output_format,
               (unsigned int)ours->pixel_rowbytes,
               (unsigned int)ref->rowbytes);
        return 0;
    }
    if (!compare_pixels(ref->rgba, ours->pixels, ref->rowbytes * ref->height))
    {
        printf("MISMATCH %s rgba pixels differ\n", path);
        return 0;
    }
    if ((ref->has_srgb != 0) != (ours->is_srgb != 0))
    {
        printf("MISMATCH %s sRGB flag ref=%d ours=%d\n", path, ref->has_srgb, ours->is_srgb ? 1 : 0);
        return 0;
    }
    if (ref->has_gamma && !ref->has_srgb)
    {
        png_fixed89 gamma_diff;
        gamma_diff = ours->image_gamma - ref->gamma;
        if (gamma_diff < 0) gamma_diff = -gamma_diff;
        if (ours->image_gamma <= 0 || gamma_diff > png_fixed89_from_ratio(5, 10000))
        {
            printf("MISMATCH %s gamma_q16 ref=%d ours=%d\n",
                   path, (int)ref->gamma, (int)ours->image_gamma);
            return 0;
        }
    }
    if ((ref->has_phys != 0) != (ours->has_pHYs != 0))
    {
        printf("MISMATCH %s pHYs presence ref=%d ours=%d\n", path, ref->has_phys, ours->has_pHYs ? 1 : 0);
        return 0;
    }
    if (ref->has_phys)
    {
        if (ours->pHYs_ppu_x != ref->phys_x || ours->pHYs_ppu_y != ref->phys_y ||
            (int)ours->pHYs_unit != ref->phys_unit)
        {
            printf("MISMATCH %s pHYs values ref=%u,%u,%d ours=%u,%u,%u\n",
                   path,
                   (unsigned int)ref->phys_x, (unsigned int)ref->phys_y, ref->phys_unit,
                   (unsigned int)ours->pHYs_ppu_x, (unsigned int)ours->pHYs_ppu_y,
                   (unsigned)ours->pHYs_unit);
            return 0;
        }
    }
    if (ref->has_iccp && !ours->has_iCCP)
    {
        printf("MISMATCH %s iCCP presence ref=%d ours=%d\n", path, ref->has_iccp, ours->has_iCCP ? 1 : 0);
        return 0;
    }
    if (ref->has_iccp && ours->iccp_profile_size != ref->iccp_profile_size)
    {
        printf("MISMATCH %s iCCP size ref=%u ours=%u\n",
               path,
               (unsigned int)ref->iccp_profile_size,
               (unsigned int)ours->iccp_profile_size);
        return 0;
    }
    if (ours->text_count != (png_u32)ref->text_count)
    {
        printf("MISMATCH %s text_count ref=%d ours=%u\n",
               path,
               ref->text_count,
               (unsigned int)ours->text_count);
        return 0;
    }
    if ((ref->has_hist != 0) != (ours->hist_count != 0u))
    {
        printf("MISMATCH %s hIST presence ref=%d ours=%d\n",
               path,
               ref->has_hist,
               ours->hist_count != 0u ? 1 : 0);
        return 0;
    }
    if (ref->has_hist && ours->hist_count != ref->hist_count)
    {
        printf("MISMATCH %s hIST count ref=%u ours=%u\n",
               path,
               (unsigned int)ref->hist_count,
               (unsigned int)ours->hist_count);
        return 0;
    }
    if (ours->splt_palette_count != (png_u32)ref->splt_count)
    {
        printf("MISMATCH %s sPLT count ref=%d ours=%u\n",
               path,
               ref->splt_count,
               (unsigned int)ours->splt_palette_count);
        return 0;
    }
    return 1;
}

static int compare_our_images(const char* path, const png_image* a, const png_image* b)
{
    if (a->width != b->width || a->height != b->height ||
        a->pixel_rowbytes != b->pixel_rowbytes ||
        a->output_format != b->output_format ||
        a->source_color_type != b->source_color_type ||
        a->source_bit_depth != b->source_bit_depth ||
        a->interlace_method != b->interlace_method)
    {
        printf("MISMATCH %s full/progressive header mismatch\n", path);
        return 0;
    }
    if (!compare_pixels(a->pixels, b->pixels, a->pixel_rowbytes * a->height))
    {
        printf("MISMATCH %s full/progressive pixels differ\n", path);
        return 0;
    }
    if (a->text_count != b->text_count ||
        a->unknown_chunk_count != b->unknown_chunk_count ||
        a->splt_palette_count != b->splt_palette_count ||
        a->hist_count != b->hist_count ||
        a->iccp_profile_size != b->iccp_profile_size ||
        a->has_pHYs != b->has_pHYs)
    {
        printf("MISMATCH %s full/progressive metadata differs\n", path);
        return 0;
    }
    return 1;
}

static int check_png_file_against_libpng(const char* path)
{
    file_buffer fb;
    ref_png_image ref;
    png_image ours_full;
    png_image ours_prog;
    int ref_ok;
    int full_err;
    int prog_err;
    int ok = 1;

    if (!read_file_data(path, &fb))
    {
        printf("FAIL %s could not read input\n", path);
        return 0;
    }

    ref_png_image_init(&ref);
    memset(&ours_full, 0, sizeof(ours_full));
    memset(&ours_prog, 0, sizeof(ours_prog));

    ref_ok = ref_decode_with_libpng(fb.data, fb.size, &ref);
    full_err = our_decode_full(fb.data, fb.size, &ours_full);

    if (!ref_ok && full_err != PNG_DEC_OK)
    {
        printf("MATCH-REJECT %s ref=reject ours=%d\n", path, full_err);
        file_buffer_free(&fb);
        return 1;
    }

    if (!ref_ok && full_err == PNG_DEC_OK)
    {
        printf("MISMATCH %s ref rejected but ours accepted\n", path);
        ok = 0;
        goto done;
    }

    if (ref_ok && full_err != PNG_DEC_OK)
    {
        printf("MISMATCH %s ref accepted but ours failed=%d\n", path, full_err);
        ok = 0;
        goto done;
    }

    if (!compare_ref_and_ours(path, &ref, &ours_full))
    {
        ok = 0;
        goto done;
    }

    prog_err = our_decode_progressive(fb.data, fb.size, &ours_prog);
    if (prog_err != PNG_DEC_OK)
    {
        printf("MISMATCH %s progressive decode failed=%d\n", path, prog_err);
        ok = 0;
        goto done;
    }
    if (!compare_our_images(path, &ours_full, &ours_prog) ||
        !compare_ref_and_ours(path, &ref, &ours_prog))
    {
        ok = 0;
        goto done;
    }

    printf("MATCH %s\n", path);

done:
    ref_png_image_free(&ref);
    png_free_image(&ours_full);
    png_free_image(&ours_prog);
    file_buffer_free(&fb);
    return ok;
}

static void fill_rgba_pattern_local(png_u8* rgba, png_u32 width, png_u32 height)
{
    png_u32 x, y;
    for (y = 0u; y < height; ++y)
    {
        for (x = 0u; x < width; ++x)
        {
            png_u8* px = rgba + (y * width + x) * 4u;
            px[0] = (png_u8)((x * 31u + y * 17u + 3u) & 0xFFu);
            px[1] = (png_u8)((x * 7u + y * 29u + 11u) & 0xFFu);
            px[2] = (png_u8)((x * 19u + y * 13u + 23u) & 0xFFu);
            px[3] = (png_u8)(255u - ((x * 9u + y * 21u) & 0x7Fu));
        }
    }
}

static int encode_and_compare_rgba(const png_u8* rgba,
                                   png_u32 width,
                                   png_u32 height,
                                   int interlace_method,
                                   const char* label)
{
    png_encode_options opt;
    png_u8* encoded = (png_u8*)0;
    png_u32 encoded_size = 0u;
    ref_png_image ref;
    int err;

    ref_png_image_init(&ref);
    png_encode_options_init(&opt);
    opt.color_type = PNG_COLOR_TRUECOLOR_ALPHA;
    opt.bit_depth = 8u;
    opt.interlace_method = (png_u8)interlace_method;
    err = png_encode_memory_ex_zlib(rgba, width, height, &opt, &encoded, &encoded_size);
    if (err != PNG_DEC_OK)
    {
        printf("FAIL %s encode err=%d\n", label, err);
        return 0;
    }
    if (!ref_decode_with_libpng(encoded, encoded_size, &ref))
    {
        printf("FAIL %s libpng could not decode our output\n", label);
        png_free_file(encoded);
        return 0;
    }
    if (ref.width != width || ref.height != height || ref.interlace_method != interlace_method ||
        ref.rowbytes != width * 4u || !compare_pixels(ref.rgba, rgba, width * height * 4u))
    {
        printf("FAIL %s round-trip mismatch\n", label);
        ref_png_image_free(&ref);
        png_free_file(encoded);
        return 0;
    }
    ref_png_image_free(&ref);
    png_free_file(encoded);
    return 1;
}

static int test_basic_encoder_compatibility(void)
{
    png_u8 rgba[7u * 5u * 4u];
    fill_rgba_pattern_local(rgba, 7u, 5u);
    if (!encode_and_compare_rgba(rgba, 7u, 5u, 0, "encode-basic-rgba"))
        return 0;
    if (!encode_and_compare_rgba(rgba, 7u, 5u, 1, "encode-basic-adam7"))
        return 0;
    return 1;
}

static int copy_unknown_chunk_names(const png_image* src, png_u8** out_names, int* out_count)
{
    png_u8* names;
    png_u32 i;
    if (src->unknown_chunk_count == 0u)
    {
        *out_names = (png_u8*)0;
        *out_count = 0;
        return 1;
    }
    if (src->unknown_chunk_count > 1000u)
        return 0;
    names = (png_u8*)png_mem89_alloc(src->unknown_chunk_count * 5u);
    if (!names)
        return 0;
    for (i = 0u; i < src->unknown_chunk_count; ++i)
    {
        memcpy(names + i * 5u, src->unknown_chunks[i].type, 5u);
    }
    *out_names = names;
    *out_count = (int)src->unknown_chunk_count;
    return 1;
}

static int reencode_with_metadata_and_compare(const char* path,
                                              int preserve_text_iccp_phys,
                                              int preserve_unknowns)
{
    file_buffer fb;
    png_image src;
    png_encode_options opt;
    png_u8* out_png = (png_u8*)0;
    png_u32 out_png_size = 0u;
    ref_png_image ref;
    png_u8* unknown_names = (png_u8*)0;
    int unknown_name_count = 0;
    int err;
    int ok = 1;

    file_buffer_init(&fb);
    memset(&src, 0, sizeof(src));
    ref_png_image_init(&ref);

    if (!read_file_data(path, &fb))
    {
        printf("FAIL could not read %s\n", path);
        return 0;
    }
    err = our_decode_full(fb.data, fb.size, &src);
    if (err != PNG_DEC_OK)
    {
        printf("FAIL could not decode source %s err=%d\n", path, err);
        file_buffer_free(&fb);
        return 0;
    }

    png_encode_options_init(&opt);
    opt.color_type = PNG_COLOR_TRUECOLOR_ALPHA;
    opt.bit_depth = 8u;
    opt.interlace_method = src.interlace_method;
    if (preserve_text_iccp_phys)
    {
        if (src.has_iCCP)
        {
            opt.write_iCCP = 1;
            opt.iccp_name = src.iccp_name ? src.iccp_name : "icc";
            opt.iccp_profile = src.iccp_profile;
            opt.iccp_profile_size = src.iccp_profile_size;
        }
        opt.text_entries = src.text_entries;
        opt.text_count = src.text_count;
        if (src.has_pHYs)
        {
            opt.write_pHYs = 1;
            opt.pHYs_ppu_x = src.pHYs_ppu_x;
            opt.pHYs_ppu_y = src.pHYs_ppu_y;
            opt.pHYs_unit = src.pHYs_unit;
        }
        if (src.is_srgb)
        {
            opt.write_sRGB = 1;
            opt.srgb_intent = 0u;
        }
        else if (src.image_gamma > 0)
        {
            opt.write_gAMA = 1;
            opt.image_gamma = src.image_gamma;
        }
    }
    if (preserve_unknowns && src.unknown_chunk_count != 0u)
    {
        opt.unknown_chunks = src.unknown_chunks;
        opt.unknown_chunk_count = src.unknown_chunk_count;
    }

    err = png_encode_image_ex_zlib(&src, &opt, &out_png, &out_png_size);
    if (err != PNG_DEC_OK)
    {
        printf("FAIL could not re-encode %s err=%d\n", path, err);
        ok = 0;
        goto done;
    }

    if (preserve_unknowns && src.unknown_chunk_count != 0u)
    {
        if (!copy_unknown_chunk_names(&src, &unknown_names, &unknown_name_count))
        {
            printf("FAIL could not prepare unknown chunk names for %s\n", path);
            ok = 0;
            goto done;
        }
    }

    if (!ref_decode_with_libpng_ex(out_png, out_png_size,
                                   unknown_names, unknown_name_count,
                                   &ref))
    {
        printf("FAIL libpng rejected re-encoded output from %s\n", path);
        ok = 0;
        goto done;
    }

    if (ref.width != src.width || ref.height != src.height || ref.rowbytes != src.pixel_rowbytes ||
        !compare_pixels(ref.rgba, src.pixels, src.pixel_rowbytes * src.height))
    {
        printf("FAIL re-encoded pixels mismatch for %s\n", path);
        ok = 0;
        goto done;
    }

    if (preserve_text_iccp_phys)
    {
        if ((png_u32)ref.text_count != src.text_count)
        {
            printf("FAIL re-encoded text_count mismatch for %s ref=%d src=%u\n",
                   path, ref.text_count, (unsigned int)src.text_count);
            ok = 0;
            goto done;
        }
        if (ref.has_iccp && ref.iccp_profile_size != src.iccp_profile_size)
        {
            printf("FAIL re-encoded iCCP mismatch for %s\n", path);
            ok = 0;
            goto done;
        }
        if ((ref.has_phys != 0) != (src.has_pHYs != 0))
        {
            printf("FAIL re-encoded pHYs presence mismatch for %s\n", path);
            ok = 0;
            goto done;
        }
        if (ref.has_phys && (ref.phys_x != src.pHYs_ppu_x || ref.phys_y != src.pHYs_ppu_y ||
                             ref.phys_unit != (int)src.pHYs_unit))
        {
            printf("FAIL re-encoded pHYs values mismatch for %s\n", path);
            ok = 0;
            goto done;
        }
    }

    if (preserve_unknowns && src.unknown_chunk_count != 0u && ref.unknown_count < (int)src.unknown_chunk_count)
    {
        printf("FAIL re-encoded unknown chunks mismatch for %s ref=%d src=%u\n",
               path, ref.unknown_count, (unsigned int)src.unknown_chunk_count);
        ok = 0;
        goto done;
    }

done:
    if (unknown_names)
        png_mem89_release(unknown_names);
    ref_png_image_free(&ref);
    png_free_file(out_png);
    png_free_image(&src);
    file_buffer_free(&fb);
    return ok;
}

static int run_selected_reencode_tests(void)
{
    if (!reencode_with_metadata_and_compare("fuzz/corpus/png_fuzz_decode_full/valid_text_iccp.png", 1, 0))
        return 0;
    if (!reencode_with_metadata_and_compare("fuzz/corpus/png_fuzz_decode_full/valid_unknown.png", 0, 1))
        return 0;
    return 1;
}

int main(int argc, char** argv)
{
    int i;
    int ok = 1;

    if (!test_basic_encoder_compatibility())
        ok = 0;
    if (!run_selected_reencode_tests())
        ok = 0;

    if (argc < 2)
    {
        if (!ok)
            return 1;
        printf("DIFF_OK (no corpus args supplied)\n");
        return 0;
    }

    for (i = 1; i < argc; ++i)
    {
        if (!check_png_file_against_libpng(argv[i]))
            ok = 0;
    }

    if (!ok)
        return 1;
    printf("DIFF_OK\n");
    return 0;
}
