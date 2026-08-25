/* SPDX-License-Identifier: CC0-1.0 */
#include "tifx.h"

#include <stdio.h>
#include <string.h>

#ifndef MAX_FILE_BYTES
#define MAX_FILE_BYTES (16UL * 1024UL * 1024UL)
#endif

#ifndef MAX_DECODE_BYTES
#define MAX_DECODE_BYTES (16UL * 1024UL * 1024UL)
#endif

static unsigned char file_buffer[MAX_FILE_BYTES];
static unsigned char decode_buffer[MAX_DECODE_BYTES];
static unsigned char workspace[65536UL];

static long file_length(FILE *fp)
{
    long pos;
    if (fseek(fp, 0L, SEEK_END) != 0) {
        return -1L;
    }
    pos = ftell(fp);
    if (pos < 0L) {
        return -1L;
    }
    if (fseek(fp, 0L, SEEK_SET) != 0) {
        return -1L;
    }
    return pos;
}

static int save_pnm(const char *path,
                    const tifx_image_info *info,
                    const unsigned char *pixels,
                    unsigned long stride)
{
    FILE *fp;
    unsigned long y;
    fp = fopen(path, "wb");
    if (fp == 0) {
        return 0;
    }
    if (info->pixel_format == TIFX_PIXEL_GRAY8) {
        fprintf(fp, "P5\n%lu %lu\n255\n", info->width, info->height);
        for (y = 0UL; y < info->height; ++y) {
            fwrite(pixels + y * stride, 1U, (size_t)info->width, fp);
        }
    } else if (info->pixel_format == TIFX_PIXEL_RGBA32) {
        fprintf(fp,
                "P7\nWIDTH %lu\nHEIGHT %lu\nDEPTH 4\nMAXVAL 255\nTUPLTYPE RGB_ALPHA\nENDHDR\n",
                info->width,
                info->height);
        for (y = 0UL; y < info->height; ++y) {
            fwrite(pixels + y * stride, 1U, (size_t)(info->width * 4UL), fp);
        }
    } else {
        fprintf(fp, "P6\n%lu %lu\n255\n", info->width, info->height);
        for (y = 0UL; y < info->height; ++y) {
            fwrite(pixels + y * stride, 1U, (size_t)(info->width * 3UL), fp);
        }
    }
    fclose(fp);
    return 1;
}

static int parse_ulong_arg(const char *text, unsigned long *out_value)
{
    unsigned long value;
    const unsigned char *p;

    if (text == 0 || out_value == 0 || text[0] == '\0') {
        return 0;
    }

    value = 0UL;
    p = (const unsigned char *)text;
    while (*p != '\0') {
        if (*p < (unsigned char)'0' || *p > (unsigned char)'9') {
            return 0;
        }
        value = value * 10UL + (unsigned long)(*p - (unsigned char)'0');
        ++p;
    }

    *out_value = value;
    return 1;
}

static int parse_path_arg(const char *text,
                          unsigned long *path_values,
                          unsigned long *out_length)
{
    unsigned long count;
    const char *p;

    if (text == 0 || path_values == 0 || out_length == 0) {
        return 0;
    }
    if (strncmp(text, "path=", 5U) != 0) {
        return 0;
    }

    p = text + 5;
    count = 0UL;
    if (*p == '\0') {
        *out_length = 0UL;
        return 1;
    }

    while (*p != '\0') {
        const char *start;
        unsigned long value;

        if (count >= TIFX_MAX_SUBIFD_DEPTH) {
            return 0;
        }
        start = p;
        while (*p != '\0' && *p != ',') {
            ++p;
        }
        value = 0UL;
        while (start != p) {
            if (*start < '0' || *start > '9') {
                return 0;
            }
            value = value * 10UL + (unsigned long)(*start - '0');
            ++start;
        }
        path_values[count++] = value;
        if (*p == ',') {
            ++p;
            if (*p == '\0') {
                return 0;
            }
        }
    }

    *out_length = count;
    return 1;
}

int main(int argc, char **argv)
{
    FILE *fp;
    long length;
    unsigned long decode_stride;
    unsigned long decode_size;
    unsigned long work_size;
    unsigned long page_index;
    unsigned long subifd_path[TIFX_MAX_SUBIFD_DEPTH];
    unsigned long subifd_path_length;
    const char *output_path;
    int page_index_set;
    int i;
    tifx_image_info info;
    int rc;

    if (argc < 2) {
        printf("usage: inspect_tiff <input.tif> [page_index] [path=0,1,...] [output.pnm|output.pam]\n");
        return 1;
    }

    page_index = 0UL;
    subifd_path_length = 0UL;
    output_path = 0;
    page_index_set = 0;

    for (i = 2; i < argc; ++i) {
        unsigned long value;
        if (parse_ulong_arg(argv[i], &value) && !page_index_set) {
            page_index = value;
            page_index_set = 1;
        } else if (parse_path_arg(argv[i], subifd_path, &subifd_path_length)) {
            ;
        } else {
            output_path = argv[i];
        }
    }

    fp = fopen(argv[1], "rb");
    if (fp == 0) {
        printf("could not open %s\n", argv[1]);
        return 1;
    }

    length = file_length(fp);
    if (length < 0L || (unsigned long)length > MAX_FILE_BYTES) {
        fclose(fp);
        printf("file too large for this fixed-buffer example\n");
        return 1;
    }
    if (fread(file_buffer, 1U, (size_t)length, fp) != (size_t)length) {
        fclose(fp);
        printf("could not read file\n");
        return 1;
    }
    fclose(fp);

    rc = tifx_parse_memory(&info, file_buffer, (unsigned long)length);
    if (rc != TIFX_OK) {
        printf("parse failed: %s\n", tifx_strerror(rc));
        return 1;
    }

    if (page_index != 0UL || subifd_path_length != 0UL) {
        if (info.container_format == TIFX_CONTAINER_BIGTIFF) {
            rc = tifx_parse_bigtiff_node_memory(&info,
                                                file_buffer,
                                                (unsigned long)length,
                                                page_index,
                                                subifd_path,
                                                subifd_path_length);
        } else {
            rc = tifx_parse_classic_node_memory(&info,
                                                file_buffer,
                                                (unsigned long)length,
                                                page_index,
                                                subifd_path,
                                                subifd_path_length);
        }
        if (rc != TIFX_OK) {
            printf("page parse failed: %s\n", tifx_strerror(rc));
            return 1;
        }
    }

    printf("container=%s\n",
           info.container_format == TIFX_CONTAINER_BIGTIFF ? "bigtiff" : "classic");
    printf("first_ifd_offset=%lu\n", info.first_ifd_offset);
    printf("current_ifd_offset=%lu\n", info.current_ifd_offset);
    printf("next_ifd_offset=%lu\n", info.next_ifd_offset);
    printf("parent_ifd_offset=%lu\n", info.parent_ifd_offset);
    printf("page_index=%lu\n", info.page_index);
    printf("page_count=%lu\n", info.page_count);
    printf("subifd_depth=%lu\n", info.subifd_depth);
    printf("subifd_index=%lu\n", info.subifd_index);
    printf("subifd_count=%lu\n", info.subifd_count);
    printf("new_subfile_type=%lu\n", info.new_subfile_type);
    printf("page_number=%u/%u\n",
           (unsigned int)info.page_number[0],
           (unsigned int)info.page_number[1]);
    printf("width=%lu\n", info.width);
    printf("height=%lu\n", info.height);
    printf("compression=%u\n", (unsigned int)info.compression);
    printf("photometric=%u\n", (unsigned int)info.photometric);
    printf("fill_order=%u\n", (unsigned int)info.fill_order);
    printf("planar_config=%u\n", (unsigned int)info.planar_config);
    printf("storage_layout=%s\n",
           info.storage_layout == TIFX_LAYOUT_TILES ? "tiles" : "strips");
    printf("rows_per_strip=%lu\n", info.rows_per_strip);
    printf("strip_count=%lu\n", info.strip_count);
    printf("tile_width=%lu\n", info.tile_width);
    printf("tile_length=%lu\n", info.tile_length);
    printf("tile_count=%lu\n", info.tile_count);
    printf("samples_per_pixel=%u\n", (unsigned int)info.samples_per_pixel);
    printf("bits_per_sample[0]=%u\n", (unsigned int)info.bits_per_sample[0]);
    printf("alpha_mode=%u\n", (unsigned int)info.alpha_mode);
    printf("predictor=%u\n", (unsigned int)info.predictor);
    printf("extra_samples_count=%u\n", (unsigned int)info.extra_samples_count);
    if (info.compression == 3U) {
        printf("t4_options=%lu\n", info.t4_options);
    }
    if (info.compression == 4U) {
        printf("t6_options=%lu\n", info.t6_options);
    }
    printf("pixel_format=%s\n",
           info.pixel_format == TIFX_PIXEL_GRAY8 ? "gray8" :
           info.pixel_format == TIFX_PIXEL_RGB24 ? "rgb24" :
           info.pixel_format == TIFX_PIXEL_RGBA32 ? "rgba32" : "unknown");

    decode_size = tifx_decode_buffer_size(&info, &decode_stride);
    work_size = tifx_decode_workspace_size(&info);
    printf("decode_stride=%lu\n", decode_stride);
    printf("workspace_size=%lu\n", work_size);
    if (decode_size == 0UL || decode_size > MAX_DECODE_BYTES || work_size > sizeof(workspace)) {
        printf("image is valid but too large for this fixed-buffer example\n");
        return 0;
    }

    rc = tifx_decode_memory(&info,
                            file_buffer,
                            (unsigned long)length,
                            decode_buffer,
                            sizeof(decode_buffer),
                            decode_stride,
                            workspace,
                            sizeof(workspace));
    if (rc != TIFX_OK) {
        printf("decode failed: %s\n", tifx_strerror(rc));
        return 1;
    }

    if (output_path != 0) {
        if (!save_pnm(output_path, &info, decode_buffer, decode_stride)) {
            printf("could not write %s\n", output_path);
            return 1;
        }
        printf("wrote %s\n", output_path);
    }

    return 0;
}
