#include "ecg_png_writer.h"
#include "ecg_surface.h"

#include <stdio.h>

static ECG_U32 ecg_png_crc_table[256];
static int ecg_png_crc_table_ready = 0;

static void ecg_png_make_crc_table(void)
{
    ECG_U32 c;
    unsigned int n;
    unsigned int k;

    for (n = 0u; n < 256u; n++) {
        c = (ECG_U32)n;
        for (k = 0u; k < 8u; k++) {
            if ((c & 1UL) != 0UL) {
                c = 0xedb88320UL ^ (c >> 1);
            } else {
                c = c >> 1;
            }
        }
        ecg_png_crc_table[n] = c;
    }

    ecg_png_crc_table_ready = 1;
}

static ECG_U32 ecg_png_crc_update_byte(ECG_U32 crc, ECG_U8 value)
{
    if (ecg_png_crc_table_ready == 0) {
        ecg_png_make_crc_table();
    }

    return ecg_png_crc_table[(crc ^ (ECG_U32)value) & 0xffUL] ^ (crc >> 8);
}

static int ecg_png_write_byte(FILE *file, ECG_U8 value)
{
    return fwrite(&value, 1u, 1u, file) == 1u;
}

static int ecg_png_write_be32(FILE *file, ECG_U32 value)
{
    ECG_U8 bytes[4];

    bytes[0] = (ECG_U8)((value >> 24) & 255UL);
    bytes[1] = (ECG_U8)((value >> 16) & 255UL);
    bytes[2] = (ECG_U8)((value >> 8) & 255UL);
    bytes[3] = (ECG_U8)(value & 255UL);

    return fwrite(bytes, 1u, 4u, file) == 4u;
}

static int ecg_png_write_chunk_data_byte(FILE *file, ECG_U32 *crc, ECG_U8 value)
{
    if (!ecg_png_write_byte(file, value)) {
        return 0;
    }
    *crc = ecg_png_crc_update_byte(*crc, value);
    return 1;
}

static int ecg_png_write_chunk_begin(FILE *file, const ECG_U8 type[4], ECG_U32 len,
                                     ECG_U32 *crc)
{
    unsigned int i;

    if (!ecg_png_write_be32(file, len)) {
        return 0;
    }

    *crc = 0xffffffffUL;
    for (i = 0u; i < 4u; i++) {
        if (!ecg_png_write_chunk_data_byte(file, crc, type[i])) {
            return 0;
        }
    }

    return 1;
}

static int ecg_png_write_chunk_end(FILE *file, ECG_U32 crc)
{
    ECG_U32 final_crc;

    final_crc = crc ^ 0xffffffffUL;
    return ecg_png_write_be32(file, final_crc);
}

static int ecg_png_write_chunk(FILE *file, const ECG_U8 type[4],
                               const ECG_U8 *data, ECG_U32 len)
{
    ECG_U32 crc;
    ECG_U32 i;

    if (!ecg_png_write_chunk_begin(file, type, len, &crc)) {
        return 0;
    }

    for (i = 0UL; i < len; i++) {
        if (!ecg_png_write_chunk_data_byte(file, &crc, data[i])) {
            return 0;
        }
    }

    return ecg_png_write_chunk_end(file, crc);
}

static ECG_U8 ecg_png_surface_raw_byte(const ECG_Surface *surface, ECG_U32 index)
{
    ECG_U32 row_bytes;
    ECG_U32 row;
    ECG_U32 in_row;
    ECG_U32 pixel_byte;
    unsigned int x;
    unsigned int channel;
    const ECG_Color *pixel;

    row_bytes = ((ECG_U32)surface->width * 3UL) + 1UL;
    row = index / row_bytes;
    in_row = index - (row * row_bytes);

    if (in_row == 0UL) {
        return 0u;
    }

    pixel_byte = in_row - 1UL;
    x = (unsigned int)(pixel_byte / 3UL);
    channel = (unsigned int)(pixel_byte - ((ECG_U32)x * 3UL));
    pixel = surface->pixels + ((unsigned long)(unsigned int)row * (unsigned long)surface->stride) + x;

    if (channel == 0u) {
        return pixel->r;
    }
    if (channel == 1u) {
        return pixel->g;
    }
    return pixel->b;
}

static void ecg_png_adler_update(ECG_U32 *a, ECG_U32 *b, ECG_U8 value)
{
    *a = (*a + (ECG_U32)value) % 65521UL;
    *b = (*b + *a) % 65521UL;
}

static int ecg_png_write_idat_uncompressed(FILE *file, const ECG_Surface *surface)
{
    static const ECG_U8 type[4] = {'I','D','A','T'};
    ECG_U32 crc;
    ECG_U32 row_bytes;
    ECG_U32 raw_len;
    ECG_U32 max_blocks;
    ECG_U32 idat_len;
    ECG_U32 raw_pos;
    ECG_U32 remaining;
    ECG_U32 adler_a;
    ECG_U32 adler_b;
    ECG_U32 adler;
    unsigned int block_len;
    unsigned int block_nlen;
    unsigned int i;
    int final_block;
    ECG_U8 value;

    row_bytes = ((ECG_U32)surface->width * 3UL) + 1UL;
    raw_len = row_bytes * (ECG_U32)surface->height;
    max_blocks = (raw_len + 65534UL) / 65535UL;
    idat_len = 2UL + raw_len + (max_blocks * 5UL) + 4UL;

    if (!ecg_png_write_chunk_begin(file, type, idat_len, &crc)) {
        return 0;
    }

    if (!ecg_png_write_chunk_data_byte(file, &crc, (ECG_U8)0x78u)) {
        return 0;
    }
    if (!ecg_png_write_chunk_data_byte(file, &crc, (ECG_U8)0x01u)) {
        return 0;
    }

    adler_a = 1UL;
    adler_b = 0UL;
    raw_pos = 0UL;
    remaining = raw_len;

    while (remaining > 0UL) {
        if (remaining > 65535UL) {
            block_len = 65535u;
            final_block = 0;
        } else {
            block_len = (unsigned int)remaining;
            final_block = 1;
        }

        block_nlen = 0xffffu - block_len;

        if (!ecg_png_write_chunk_data_byte(file, &crc, final_block != 0 ? (ECG_U8)0x01u : (ECG_U8)0x00u)) {
            return 0;
        }
        if (!ecg_png_write_chunk_data_byte(file, &crc, (ECG_U8)(block_len & 255u))) {
            return 0;
        }
        if (!ecg_png_write_chunk_data_byte(file, &crc, (ECG_U8)((block_len >> 8) & 255u))) {
            return 0;
        }
        if (!ecg_png_write_chunk_data_byte(file, &crc, (ECG_U8)(block_nlen & 255u))) {
            return 0;
        }
        if (!ecg_png_write_chunk_data_byte(file, &crc, (ECG_U8)((block_nlen >> 8) & 255u))) {
            return 0;
        }

        for (i = 0u; i < block_len; i++) {
            value = ecg_png_surface_raw_byte(surface, raw_pos);
            ecg_png_adler_update(&adler_a, &adler_b, value);
            if (!ecg_png_write_chunk_data_byte(file, &crc, value)) {
                return 0;
            }
            raw_pos++;
        }

        remaining -= (ECG_U32)block_len;
    }

    adler = (adler_b << 16) | adler_a;
    if (!ecg_png_write_chunk_data_byte(file, &crc, (ECG_U8)((adler >> 24) & 255UL))) {
        return 0;
    }
    if (!ecg_png_write_chunk_data_byte(file, &crc, (ECG_U8)((adler >> 16) & 255UL))) {
        return 0;
    }
    if (!ecg_png_write_chunk_data_byte(file, &crc, (ECG_U8)((adler >> 8) & 255UL))) {
        return 0;
    }
    if (!ecg_png_write_chunk_data_byte(file, &crc, (ECG_U8)(adler & 255UL))) {
        return 0;
    }

    return ecg_png_write_chunk_end(file, crc);
}

ECG_Status ecg_png_write_rgb24(const char *filename, const ECG_Surface *surface)
{
    static const ECG_U8 png_signature[8] = {137u,80u,78u,71u,13u,10u,26u,10u};
    static const ECG_U8 ihdr_type[4] = {'I','H','D','R'};
    static const ECG_U8 iend_type[4] = {'I','E','N','D'};
    static const ECG_U8 empty_data[1] = {0u};
    FILE *file;
    ECG_U8 ihdr[13];
    ECG_Status status;

    if (filename == (const char *)0 || !ecg_surface_is_valid(surface)) {
        return ECG_STATUS_NULL;
    }

    file = fopen(filename, "wb");
    if (file == (FILE *)0) {
        return ECG_STATUS_IO;
    }

    status = ECG_STATUS_OK;
    if (fwrite(png_signature, 1u, 8u, file) != 8u) {
        status = ECG_STATUS_IO;
    }

    ihdr[0] = (ECG_U8)(((ECG_U32)surface->width >> 24) & 255UL);
    ihdr[1] = (ECG_U8)(((ECG_U32)surface->width >> 16) & 255UL);
    ihdr[2] = (ECG_U8)(((ECG_U32)surface->width >> 8) & 255UL);
    ihdr[3] = (ECG_U8)((ECG_U32)surface->width & 255UL);
    ihdr[4] = (ECG_U8)(((ECG_U32)surface->height >> 24) & 255UL);
    ihdr[5] = (ECG_U8)(((ECG_U32)surface->height >> 16) & 255UL);
    ihdr[6] = (ECG_U8)(((ECG_U32)surface->height >> 8) & 255UL);
    ihdr[7] = (ECG_U8)((ECG_U32)surface->height & 255UL);
    ihdr[8] = 8u;
    ihdr[9] = 2u;
    ihdr[10] = 0u;
    ihdr[11] = 0u;
    ihdr[12] = 0u;

    if (status == ECG_STATUS_OK && !ecg_png_write_chunk(file, ihdr_type, ihdr, 13UL)) {
        status = ECG_STATUS_IO;
    }
    if (status == ECG_STATUS_OK && !ecg_png_write_idat_uncompressed(file, surface)) {
        status = ECG_STATUS_IO;
    }
    if (status == ECG_STATUS_OK && !ecg_png_write_chunk(file, iend_type, empty_data, 0UL)) {
        status = ECG_STATUS_IO;
    }

    if (fclose(file) != 0 && status == ECG_STATUS_OK) {
        status = ECG_STATUS_IO;
    }

    return status;
}
