#include "pcx_encoder.h"

#include <string.h>
#include "pcx_config89.h"

static pcx_u8 g_pcx_encode_store[PCX89_STATIC_ENCODE_BYTES];
static pcx_u8 g_pcx_encode_row_store[65536U];

typedef struct PCXEncodeBufferTag
{
    pcx_u8 *data;
    pcx_size  size;
    pcx_size  capacity;
} PCXEncodeBuffer;

static void pcx_write_u16_le_to_buf(pcx_u8 *dst, pcx_u16 value)
{
    dst[0] = (pcx_u8)(value & 0xFFU);
    dst[1] = (pcx_u8)((value >> 8) & 0xFFU);
}

static int pcx_buffer_reserve(PCXEncodeBuffer *buf, pcx_size extra)
{
    pcx_size needed;

    if (buf == NULL)
    {
        return PCX_ERR_NULL_POINTER;
    }
    if (!pcx_add_pcx_size_safe(buf->size, extra, &needed))
    {
        return PCX_ERR_OVERFLOW;
    }
    if (needed > PCX89_STATIC_ENCODE_BYTES)
    {
        return PCX_ERR_LIMITS;
    }
    if (buf->data == NULL)
    {
        buf->data = g_pcx_encode_store;
        buf->capacity = PCX89_STATIC_ENCODE_BYTES;
    }
    return PCX_OK;
}

static int pcx_buffer_append_byte(PCXEncodeBuffer *buf, pcx_u8 value)
{
    int rc;

    rc = pcx_buffer_reserve(buf, 1U);
    if (rc != PCX_OK)
    {
        return rc;
    }

    buf->data[buf->size++] = value;
    return PCX_OK;
}

static int pcx_buffer_append_bytes(PCXEncodeBuffer *buf,
                                   const pcx_u8 *src,
                                   pcx_size count)
{
    int rc;

    if (buf == NULL)
    {
        return PCX_ERR_NULL_POINTER;
    }

    if (count == 0U)
    {
        return PCX_OK;
    }

    if (src == NULL)
    {
        return PCX_ERR_NULL_POINTER;
    }

    rc = pcx_buffer_reserve(buf, count);
    if (rc != PCX_OK)
    {
        return rc;
    }

    memcpy(buf->data + buf->size, src, count);
    buf->size += count;
    return PCX_OK;
}

static void pcx_buffer_reset(PCXEncodeBuffer *buf)
{
    if (buf == NULL)
    {
        return;
    }
    buf->data = NULL;
    buf->size = 0U;
    buf->capacity = 0U;
}

static int pcx_version_is_supported(pcx_u8 version)
{
    return (version == 0U || version == 2U || version == 3U ||
            version == 4U || version == 5U) ? 1 : 0;
}

static int pcx_choose_version(const PCXEncodeOptions *opt,
                              int requiresVersion5,
                              pcx_u8 *outVersion)
{
    pcx_u8 version;

    if (outVersion == NULL)
    {
        return PCX_ERR_NULL_POINTER;
    }

    version = (opt != NULL) ? opt->version : 0U;
    if (version == 0U)
    {
        version = 5U;
    }

    if (!pcx_version_is_supported(version))
    {
        return PCX_ERR_UNSUPPORTED;
    }

    if (requiresVersion5 && version != 5U)
    {
        return PCX_ERR_UNSUPPORTED;
    }

    *outVersion = version;
    return PCX_OK;
}

static int pcx_validate_encoding_value(const PCXEncodeOptions *opt,
                                       pcx_u8 *outEncoding)
{
    pcx_u8 encoding;

    if (outEncoding == NULL)
    {
        return PCX_ERR_NULL_POINTER;
    }

    encoding = (opt != NULL) ? opt->encoding : 1U;
    if (!(encoding == 0U || encoding == 1U))
    {
        return PCX_ERR_UNSUPPORTED;
    }

    *outEncoding = encoding;
    return PCX_OK;
}

static int pcx_compute_even_bytes_per_line(int width,
                                           int bitsPerPixel,
                                           pcx_u16 *outBytesPerLine)
{
    int bits;
    int bitsPlus7;
    int bytes;
    int evenBytes;

    if (outBytesPerLine == NULL)
    {
        return PCX_ERR_NULL_POINTER;
    }

    if (width <= 0 || !(bitsPerPixel == 1 || bitsPerPixel == 2 ||
                        bitsPerPixel == 4 || bitsPerPixel == 8))
    {
        return PCX_ERR_DIMENSIONS;
    }

    if (!pcx_mul_int_safe(width, bitsPerPixel, &bits) ||
        !pcx_add_int_safe(bits, 7, &bitsPlus7))
    {
        return PCX_ERR_OVERFLOW;
    }

    bytes = bitsPlus7 / 8;
    if (bytes <= 0)
    {
        return PCX_ERR_DIMENSIONS;
    }

    evenBytes = bytes + (bytes & 1);
    if (evenBytes > 65535)
    {
        return PCX_ERR_OVERFLOW;
    }

    *outBytesPerLine = (pcx_u16)evenBytes;
    return PCX_OK;
}

static int pcx_fill_header_bytes(pcx_u8 *header,
                                 int width,
                                 int height,
                                 pcx_u8 version,
                                 pcx_u8 encoding,
                                 pcx_u8 bitsPerPixel,
                                 pcx_u8 nPlanes,
                                 pcx_u16 bytesPerLine,
                                 const PCXPalette *palette,
                                 const PCXEncodeOptions *opt)
{
    pcx_u16 dpiX;
    pcx_u16 dpiY;
    pcx_u16 paletteInfo;
    int i;

    if (header == NULL)
    {
        return PCX_ERR_NULL_POINTER;
    }

    if (width <= 0 || height <= 0 || width > 65536 || height > 65536)
    {
        return PCX_ERR_DIMENSIONS;
    }

    memset(header, 0, 128U);

    dpiX = (opt != NULL && opt->hDPI != 0U) ? opt->hDPI : 72U;
    dpiY = (opt != NULL && opt->vDPI != 0U) ? opt->vDPI : 72U;
    paletteInfo = (opt != NULL && opt->paletteInfo != 0U) ? opt->paletteInfo : 1U;

    header[0] = 0x0AU;
    header[1] = version;
    header[2] = encoding;
    header[3] = bitsPerPixel;
    pcx_write_u16_le_to_buf(header + 4, 0U);
    pcx_write_u16_le_to_buf(header + 6, 0U);
    pcx_write_u16_le_to_buf(header + 8, (pcx_u16)(width - 1));
    pcx_write_u16_le_to_buf(header + 10, (pcx_u16)(height - 1));
    pcx_write_u16_le_to_buf(header + 12, dpiX);
    pcx_write_u16_le_to_buf(header + 14, dpiY);

    if (palette != NULL && pcx_palette_is_valid(palette))
    {
        for (i = 0; i < 16; ++i)
        {
            header[16 + i * 3 + 0] = palette->colors[i][0];
            header[16 + i * 3 + 1] = palette->colors[i][1];
            header[16 + i * 3 + 2] = palette->colors[i][2];
        }
    }

    header[64] = 0U;
    header[65] = nPlanes;
    pcx_write_u16_le_to_buf(header + 66, bytesPerLine);
    pcx_write_u16_le_to_buf(header + 68, paletteInfo);
    pcx_write_u16_le_to_buf(header + 70,
                            (opt != NULL) ? opt->hScreenSize : 0U);
    pcx_write_u16_le_to_buf(header + 72,
                            (opt != NULL) ? opt->vScreenSize : 0U);

    return PCX_OK;
}

static int pcx_append_encoded_plane(PCXEncodeBuffer *buf,
                                    const pcx_u8 *src,
                                    pcx_size count,
                                    pcx_u8 encoding)
{
    pcx_size i;

    if (buf == NULL || src == NULL)
    {
        return PCX_ERR_NULL_POINTER;
    }

    if (encoding == 0U)
    {
        return pcx_buffer_append_bytes(buf, src, count);
    }

    if (encoding != 1U)
    {
        return PCX_ERR_UNSUPPORTED;
    }

    i = 0U;
    while (i < count)
    {
        pcx_u8 value;
        pcx_size run;
        int rc;

        value = src[i];
        run = 1U;
        while (i + run < count && run < 63U && src[i + run] == value)
        {
            ++run;
        }

        if (run > 1U || value >= 0xC0U)
        {
            rc = pcx_buffer_append_byte(buf, (pcx_u8)(0xC0U | (pcx_u8)run));
            if (rc != PCX_OK)
            {
                return rc;
            }
            rc = pcx_buffer_append_byte(buf, value);
            if (rc != PCX_OK)
            {
                return rc;
            }
        }
        else
        {
            rc = pcx_buffer_append_byte(buf, value);
            if (rc != PCX_OK)
            {
                return rc;
            }
        }

        i += run;
    }

    return PCX_OK;
}

static int pcx_validate_index_range(const pcx_u8 *indices,
                                    int width,
                                    int height,
                                    int stride,
                                    int totalBitsPerPixel)
{
    int limit;
    int y;
    int x;

    if (indices == NULL)
    {
        return PCX_ERR_NULL_POINTER;
    }

    if (totalBitsPerPixel == 8)
    {
        return PCX_OK;
    }

    limit = 1 << totalBitsPerPixel;
    for (y = 0; y < height; ++y)
    {
        const pcx_u8 *row;

        row = indices + (pcx_size)y * (pcx_size)stride;
        for (x = 0; x < width; ++x)
        {
            if ((int)row[x] >= limit)
            {
                return PCX_ERR_FORMAT;
            }
        }
    }

    return PCX_OK;
}

static int pcx_pack_index_row(const pcx_u8 *src,
                              int width,
                              int totalBitsPerPixel,
                              pcx_u8 *dst,
                              int bytesPerLine)
{
    int x;

    if (src == NULL || dst == NULL)
    {
        return PCX_ERR_NULL_POINTER;
    }

    memset(dst, 0, (pcx_size)bytesPerLine);

    if (totalBitsPerPixel == 8)
    {
        memcpy(dst, src, (pcx_size)width);
        return PCX_OK;
    }

    for (x = 0; x < width; ++x)
    {
        int perByte;
        int byteIndex;
        int sampleIndex;
        int shift;
        pcx_u8 mask;

        perByte = 8 / totalBitsPerPixel;
        byteIndex = x / perByte;
        sampleIndex = x % perByte;
        shift = 8 - totalBitsPerPixel * (sampleIndex + 1);
        mask = (pcx_u8)((1 << totalBitsPerPixel) - 1);
        dst[byteIndex] |= (pcx_u8)((src[x] & mask) << shift);
    }

    return PCX_OK;
}

static int pcx_encode_indexed_rows(const pcx_u8 *indices,
                                   int width,
                                   int height,
                                   int stride,
                                   int totalBitsPerPixel,
                                   pcx_u16 bytesPerLine,
                                   pcx_u8 encoding,
                                   PCXEncodeBuffer *buf)
{
    pcx_u8 *rowPacked;
    int y;
    int rc;

    rowPacked = g_pcx_encode_row_store;

    for (y = 0; y < height; ++y)
    {
        const pcx_u8 *row;

        row = indices + (pcx_size)y * (pcx_size)stride;
        rc = pcx_pack_index_row(row,
                                width,
                                totalBitsPerPixel,
                                rowPacked,
                                (int)bytesPerLine);
        if (rc != PCX_OK)
        {
            return rc;
        }

        rc = pcx_append_encoded_plane(buf,
                                      rowPacked,
                                      (pcx_size)bytesPerLine,
                                      encoding);
        if (rc != PCX_OK)
        {
            return rc;
        }
    }

    return PCX_OK;
}

static int pcx_encode_rgb24_rows(const pcx_u8 *rgb,
                                 int width,
                                 int height,
                                 int stride,
                                 pcx_u16 bytesPerLine,
                                 pcx_u8 encoding,
                                 PCXEncodeBuffer *buf)
{
    pcx_u8 *planeRow;
    int y;
    int rc;

    planeRow = g_pcx_encode_row_store;

    for (y = 0; y < height; ++y)
    {
        const pcx_u8 *srcRow;
        int plane;
        int x;

        srcRow = rgb + (pcx_size)y * (pcx_size)stride;
        for (plane = 0; plane < 3; ++plane)
        {
            memset(planeRow, 0, (pcx_size)bytesPerLine);
            for (x = 0; x < width; ++x)
            {
                planeRow[x] = srcRow[(pcx_size)x * 3U + (pcx_size)plane];
            }

            rc = pcx_append_encoded_plane(buf,
                                          planeRow,
                                          (pcx_size)bytesPerLine,
                                          encoding);
            if (rc != PCX_OK)
            {
                return rc;
            }
        }
    }

    return PCX_OK;
}

static int pcx_append_vga_palette(PCXEncodeBuffer *buf,
                                  const PCXPalette *palette)
{
    int i;
    int rc;

    if (buf == NULL || palette == NULL)
    {
        return PCX_ERR_NULL_POINTER;
    }

    if (!pcx_palette_is_valid(palette))
    {
        return PCX_ERR_FORMAT;
    }

    rc = pcx_buffer_append_byte(buf, 0x0CU);
    if (rc != PCX_OK)
    {
        return rc;
    }

    for (i = 0; i < 256; ++i)
    {
        rc = pcx_buffer_append_byte(buf, palette->colors[i][0]);
        if (rc != PCX_OK) return rc;
        rc = pcx_buffer_append_byte(buf, palette->colors[i][1]);
        if (rc != PCX_OK) return rc;
        rc = pcx_buffer_append_byte(buf, palette->colors[i][2]);
        if (rc != PCX_OK) return rc;
    }

    return PCX_OK;
}

static int pcx_write_file_bytes(const char *filename,
                                const pcx_u8 *data,
                                pcx_size size)
{
    FILE *f;

    if (filename == NULL || data == NULL)
    {
        return PCX_ERR_NULL_POINTER;
    }

    f = fopen(filename, "wb");
    if (f == NULL)
    {
        return PCX_ERR_IO;
    }

    if (size > 0U && fwrite(data, 1U, size, f) != size)
    {
        fclose(f);
        return PCX_ERR_IO;
    }

    if (fclose(f) != 0)
    {
        return PCX_ERR_IO;
    }

    return PCX_OK;
}

void pcx_encode_options_default(PCXEncodeOptions *opt)
{
    if (opt == NULL)
    {
        return;
    }

    memset(opt, 0, sizeof(*opt));
    opt->encoding = 1U;
    opt->hDPI = 72U;
    opt->vDPI = 72U;
    opt->paletteInfo = 1U;
}

int pcx_encode_rgb24(const pcx_u8 *rgb,
                     int width,
                     int height,
                     int stride,
                     const PCXEncodeOptions *opt,
                     pcx_u8 **outData,
                     pcx_size *outSize)
{
    PCXEncodeBuffer buf;
    pcx_u8 header[128];
    pcx_u16 bytesPerLine;
    pcx_u8 version;
    pcx_u8 encoding;
    int rc;

    if (rgb == NULL || outData == NULL || outSize == NULL)
    {
        return PCX_ERR_NULL_POINTER;
    }

    {
        int requiredStride;
        if (width <= 0 || height <= 0)
        {
            return PCX_ERR_DIMENSIONS;
        }
        if (!pcx_mul_int_safe(width, 3, &requiredStride))
        {
            return PCX_ERR_OVERFLOW;
        }
        if (stride < requiredStride)
        {
            return PCX_ERR_DIMENSIONS;
        }
    }

    rc = pcx_choose_version(opt, 1, &version);
    if (rc != PCX_OK)
    {
        return rc;
    }

    rc = pcx_validate_encoding_value(opt, &encoding);
    if (rc != PCX_OK)
    {
        return rc;
    }

    rc = pcx_compute_even_bytes_per_line(width, 8, &bytesPerLine);
    if (rc != PCX_OK)
    {
        return rc;
    }

    rc = pcx_fill_header_bytes(header,
                               width,
                               height,
                               version,
                               encoding,
                               8U,
                               3U,
                               bytesPerLine,
                               NULL,
                               opt);
    if (rc != PCX_OK)
    {
        return rc;
    }

    memset(&buf, 0, sizeof(buf));
    rc = pcx_buffer_append_bytes(&buf, header, sizeof(header));
    if (rc != PCX_OK)
    {
        pcx_buffer_reset(&buf);
        return rc;
    }

    rc = pcx_encode_rgb24_rows(rgb,
                               width,
                               height,
                               stride,
                               bytesPerLine,
                               encoding,
                               &buf);
    if (rc != PCX_OK)
    {
        pcx_buffer_reset(&buf);
        return rc;
    }

    *outData = buf.data;
    *outSize = buf.size;
    return PCX_OK;
}

int pcx_encode_indexed(const pcx_u8 *indices,
                       int width,
                       int height,
                       int stride,
                       int totalBitsPerPixel,
                       const PCXPalette *palette,
                       const PCXEncodeOptions *opt,
                       pcx_u8 **outData,
                       pcx_size *outSize)
{
    PCXEncodeBuffer buf;
    pcx_u8 header[128];
    pcx_u16 bytesPerLine;
    pcx_u8 version;
    pcx_u8 encoding;
    int rc;
    int requiresVersion5;

    if (indices == NULL || outData == NULL || outSize == NULL || palette == NULL)
    {
        return PCX_ERR_NULL_POINTER;
    }

    if (!pcx_palette_is_valid(palette))
    {
        return PCX_ERR_FORMAT;
    }

    if (width <= 0 || height <= 0 || stride < width)
    {
        return PCX_ERR_DIMENSIONS;
    }

    if (!(totalBitsPerPixel == 1 || totalBitsPerPixel == 2 ||
          totalBitsPerPixel == 4 || totalBitsPerPixel == 8))
    {
        return PCX_ERR_UNSUPPORTED;
    }

    rc = pcx_validate_index_range(indices,
                                  width,
                                  height,
                                  stride,
                                  totalBitsPerPixel);
    if (rc != PCX_OK)
    {
        return rc;
    }

    requiresVersion5 = (totalBitsPerPixel == 8) ? 1 : 0;
    rc = pcx_choose_version(opt, requiresVersion5, &version);
    if (rc != PCX_OK)
    {
        return rc;
    }

    rc = pcx_validate_encoding_value(opt, &encoding);
    if (rc != PCX_OK)
    {
        return rc;
    }

    rc = pcx_compute_even_bytes_per_line(width,
                                         totalBitsPerPixel,
                                         &bytesPerLine);
    if (rc != PCX_OK)
    {
        return rc;
    }

    rc = pcx_fill_header_bytes(header,
                               width,
                               height,
                               version,
                               encoding,
                               (pcx_u8)totalBitsPerPixel,
                               1U,
                               bytesPerLine,
                               palette,
                               opt);
    if (rc != PCX_OK)
    {
        return rc;
    }

    memset(&buf, 0, sizeof(buf));
    rc = pcx_buffer_append_bytes(&buf, header, sizeof(header));
    if (rc != PCX_OK)
    {
        pcx_buffer_reset(&buf);
        return rc;
    }

    rc = pcx_encode_indexed_rows(indices,
                                 width,
                                 height,
                                 stride,
                                 totalBitsPerPixel,
                                 bytesPerLine,
                                 encoding,
                                 &buf);
    if (rc != PCX_OK)
    {
        pcx_buffer_reset(&buf);
        return rc;
    }

    if (totalBitsPerPixel == 8)
    {
        rc = pcx_append_vga_palette(&buf, palette);
        if (rc != PCX_OK)
        {
            pcx_buffer_reset(&buf);
            return rc;
        }
    }

    *outData = buf.data;
    *outSize = buf.size;
    return PCX_OK;
}

int pcx_encode_image_rgb24(const PCXImage *img,
                           const PCXEncodeOptions *opt,
                           pcx_u8 **outData,
                           pcx_size *outSize)
{
    if (img == NULL)
    {
        return PCX_ERR_NULL_POINTER;
    }

    if (img->channels != 3 || img->pixels == NULL)
    {
        return PCX_ERR_FORMAT;
    }

    {
        int stride;
        if (!pcx_mul_int_safe(img->width, img->channels, &stride))
        {
            return PCX_ERR_OVERFLOW;
        }
        return pcx_encode_rgb24(img->pixels,
                                img->width,
                                img->height,
                                stride,
                                opt,
                                outData,
                                outSize);
    }
}

int pcx_encode_indexed_image(const PCXIndexedImage *img,
                             const PCXEncodeOptions *opt,
                             pcx_u8 **outData,
                             pcx_size *outSize)
{
    if (img == NULL)
    {
        return PCX_ERR_NULL_POINTER;
    }

    if (img->indices == NULL)
    {
        return PCX_ERR_FORMAT;
    }

    return pcx_encode_indexed(img->indices,
                              img->width,
                              img->height,
                              img->width,
                              img->totalBitsPerPixel,
                              &img->palette,
                              opt,
                              outData,
                              outSize);
}

int pcx_write_rgb24_file(const char *filename,
                         const pcx_u8 *rgb,
                         int width,
                         int height,
                         int stride,
                         const PCXEncodeOptions *opt)
{
    pcx_u8 *data;
    pcx_size size;
    int rc;

    data = NULL;
    size = 0U;
    rc = pcx_encode_rgb24(rgb, width, height, stride, opt, &data, &size);
    if (rc != PCX_OK)
    {
        return rc;
    }

    rc = pcx_write_file_bytes(filename, data, size);
    return rc;
}

int pcx_write_indexed_file(const char *filename,
                           const pcx_u8 *indices,
                           int width,
                           int height,
                           int stride,
                           int totalBitsPerPixel,
                           const PCXPalette *palette,
                           const PCXEncodeOptions *opt)
{
    pcx_u8 *data;
    pcx_size size;
    int rc;

    data = NULL;
    size = 0U;
    rc = pcx_encode_indexed(indices,
                            width,
                            height,
                            stride,
                            totalBitsPerPixel,
                            palette,
                            opt,
                            &data,
                            &size);
    if (rc != PCX_OK)
    {
        return rc;
    }

    rc = pcx_write_file_bytes(filename, data, size);
    return rc;
}
