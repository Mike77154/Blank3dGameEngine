/* pcx_decoder.c - decodificador PCX robusto */

#include "pcx_decoder.h"
#include <string.h>
#include "pcx_config89.h"


static pcx_u8 g_pcx_decode_row_store[PCX89_STATIC_SCANLINE_BYTES];
static pcx_u8 g_pcx_decode_row_index_store[PCX89_STATIC_ROW_INDEX_BYTES];
static pcx_u8 g_pcx_decode_index_store[PCX89_STATIC_INDEX_BYTES];
#define PCX_INTERNAL_MODE_INDEXED_LOW  1
#define PCX_INTERNAL_MODE_INDEXED_8    2
#define PCX_INTERNAL_MODE_RGB24        3

static PCXDecodeLimits g_pcx_global_limits = {
    PCX_DEFAULT_MAX_INPUT_BYTES,
    PCX_DEFAULT_MAX_WIDTH,
    PCX_DEFAULT_MAX_HEIGHT,
    PCX_DEFAULT_MAX_PIXELS,
    PCX_DEFAULT_MAX_DECODED_BYTES,
    PCX_DEFAULT_MAX_SCANLINE_BYTES
};

static const PCXDecodeLimits *pcx_resolve_limits(const PCXDecodeLimits *limits);
static void pcx_init_file_info(PCXFileInfo *info);
static int pcx_check_stream_input_limit(FILE *f,
                                        const PCXDecodeLimits *limits);
static int pcx_check_parsed_limits(const PCXHeader *hdr,
                                   int width,
                                   int height,
                                   const PCXDecodeLimits *limits);
static int pcx_check_decoded_limits(int width,
                                    int height,
                                    int channels,
                                    const PCXDecodeLimits *limits);
static void pcx_fill_file_info(const PCXHeader *hdr,
                               int width,
                               int height,
                               PCXFileInfo *outInfo);
static int pcx_load_fp_with_info_flags(FILE *f,
                                       PCXImage *outImage,
                                       PCXFileInfo *outInfo,
                                       unsigned parseFlags,
                                       const PCXDecodeLimits *limits);
static int pcx_load_fp_indexed_with_info_flags(FILE *f,
                                               PCXIndexedImage *outImage,
                                               PCXFileInfo *outInfo,
                                               unsigned parseFlags,
                                               const PCXDecodeLimits *limits);


void pcx_decode_limits_default(PCXDecodeLimits *limits)
{
    if (limits == NULL)
    {
        return;
    }

    limits->maxInputBytes = PCX_DEFAULT_MAX_INPUT_BYTES;
    limits->maxWidth = PCX_DEFAULT_MAX_WIDTH;
    limits->maxHeight = PCX_DEFAULT_MAX_HEIGHT;
    limits->maxPixels = PCX_DEFAULT_MAX_PIXELS;
    limits->maxDecodedBytes = PCX_DEFAULT_MAX_DECODED_BYTES;
    limits->maxScanlineBytes = PCX_DEFAULT_MAX_SCANLINE_BYTES;
}

void pcx_set_global_decode_limits(const PCXDecodeLimits *limits)
{
    if (limits == NULL)
    {
        pcx_decode_limits_default(&g_pcx_global_limits);
        return;
    }

    g_pcx_global_limits = *limits;
}

void pcx_get_global_decode_limits(PCXDecodeLimits *outLimits)
{
    if (outLimits == NULL)
    {
        return;
    }

    *outLimits = g_pcx_global_limits;
}

static const PCXDecodeLimits *pcx_resolve_limits(const PCXDecodeLimits *limits)
{
    if (limits != NULL)
    {
        return limits;
    }

    return &g_pcx_global_limits;
}

static int pcx_check_stream_input_limit(FILE *f,
                                        const PCXDecodeLimits *limits)
{
    const PCXDecodeLimits *effective;
    pcx_off saved;
    pcx_off end;

    if (f == NULL)
    {
        return PCX_ERR_NULL_POINTER;
    }

    effective = pcx_resolve_limits(limits);
    if (effective->maxInputBytes == 0U)
    {
        return PCX_OK;
    }

    saved = ftell(f);
    if (saved < 0L)
    {
        return PCX_OK;
    }

    if (fseek(f, 0L, SEEK_END) != 0)
    {
        (void)fseek(f, saved, SEEK_SET);
        return PCX_OK;
    }

    end = ftell(f);
    if (end < 0L)
    {
        (void)fseek(f, saved, SEEK_SET);
        return PCX_OK;
    }

    if (fseek(f, saved, SEEK_SET) != 0)
    {
        return PCX_ERR_IO;
    }

    if ((pcx_u32)end > (pcx_u32)effective->maxInputBytes)
    {
        return PCX_ERR_LIMITS;
    }

    return PCX_OK;
}

static int pcx_check_parsed_limits(const PCXHeader *hdr,
                                   int width,
                                   int height,
                                   const PCXDecodeLimits *limits)
{
    const PCXDecodeLimits *effective;
    pcx_size pixels;
    pcx_size scanlineBytes;

    if (hdr == NULL)
    {
        return PCX_ERR_NULL_POINTER;
    }

    effective = pcx_resolve_limits(limits);

    if (effective->maxWidth > 0 &&
        width > effective->maxWidth)
    {
        return PCX_ERR_LIMITS;
    }

    if (effective->maxHeight > 0 &&
        height > effective->maxHeight)
    {
        return PCX_ERR_LIMITS;
    }

    if (!pcx_mul_pcx_size_safe((pcx_size)width, (pcx_size)height, &pixels))
    {
        return PCX_ERR_OVERFLOW;
    }

    if (effective->maxPixels > 0U &&
        pixels > effective->maxPixels)
    {
        return PCX_ERR_LIMITS;
    }

    if (!pcx_mul_pcx_size_safe((pcx_size)hdr->bytesPerLine,
                             (pcx_size)hdr->nPlanes,
                             &scanlineBytes))
    {
        return PCX_ERR_OVERFLOW;
    }

    if (effective->maxScanlineBytes > 0U &&
        scanlineBytes > effective->maxScanlineBytes)
    {
        return PCX_ERR_LIMITS;
    }

    return PCX_OK;
}

static int pcx_check_decoded_limits(int width,
                                    int height,
                                    int channels,
                                    const PCXDecodeLimits *limits)
{
    const PCXDecodeLimits *effective;
    pcx_size decodedBytes;

    if (channels <= 0)
    {
        return PCX_OK;
    }

    if (!pcx_image_calc_buffer_size(width, height, channels, &decodedBytes))
    {
        return PCX_ERR_OVERFLOW;
    }

    effective = pcx_resolve_limits(limits);
    if (effective->maxDecodedBytes > 0U &&
        decodedBytes > effective->maxDecodedBytes)
    {
        return PCX_ERR_LIMITS;
    }

    return PCX_OK;
}

static int pcx_open_memory_stream(const void *data,
                                  pcx_size size,
                                  const PCXDecodeLimits *limits,
                                  FILE **outFile)
{
    const PCXDecodeLimits *effective;
    FILE *f;

    if (outFile == NULL)
    {
        return PCX_ERR_NULL_POINTER;
    }

    *outFile = NULL;

    if (data == NULL)
    {
        return PCX_ERR_NULL_POINTER;
    }

    effective = pcx_resolve_limits(limits);
    if (effective->maxInputBytes > 0U &&
        size > effective->maxInputBytes)
    {
        return PCX_ERR_LIMITS;
    }

    f = tmpfile();
    if (f == NULL)
    {
        return PCX_ERR_IO;
    }

    if (size > 0U)
    {
        if (fwrite(data, 1U, size, f) != size)
        {
            fclose(f);
            return PCX_ERR_IO;
        }
    }

    if (fflush(f) != 0)
    {
        fclose(f);
        return PCX_ERR_IO;
    }

    if (fseek(f, 0L, SEEK_SET) != 0)
    {
        fclose(f);
        return PCX_ERR_IO;
    }

    *outFile = f;
    return PCX_OK;
}

static int pcx_inspect_stream(FILE *f,
                              unsigned flags,
                              const PCXDecodeLimits *limits,
                              PCXHeader *outHeader,
                              int *outWidth,
                              int *outHeight)
{
    PCXHeader hdr;
    int width;
    int height;
    int result;

    if (f == NULL || outHeader == NULL || outWidth == NULL || outHeight == NULL)
    {
        return PCX_ERR_NULL_POINTER;
    }

    result = pcx_check_stream_input_limit(f, limits);
    if (result != PCX_OK)
    {
        return result;
    }

    result = pcx_parse_header_ex(f, &hdr, &width, &height, flags);
    if (result != PCX_OK)
    {
        return result;
    }

    result = pcx_check_parsed_limits(&hdr, width, height, limits);
    if (result != PCX_OK)
    {
        return result;
    }

    *outHeader = hdr;
    *outWidth = width;
    *outHeight = height;
    return PCX_OK;
}

static int pcx_inspect_stream_ex(FILE *f,
                                 const PCXDecodeLimits *limits,
                                 PCXFileInfo *outInfo)
{
    PCXHeader hdr;
    int width;
    int height;
    int result;

    if (f == NULL || outInfo == NULL)
    {
        return PCX_ERR_NULL_POINTER;
    }

    pcx_init_file_info(outInfo);

    result = pcx_check_stream_input_limit(f, limits);
    if (result != PCX_OK)
    {
        return result;
    }

    result = pcx_parse_header_ex(f, &hdr, &width, &height, PCX_PARSE_FLAG_NONE);
    if (result != PCX_OK)
    {
        return result;
    }

    result = pcx_check_parsed_limits(&hdr, width, height, limits);
    if (result != PCX_OK)
    {
        return result;
    }

    pcx_fill_file_info(&hdr, width, height, outInfo);
    return PCX_OK;
}

static void pcx_init_file_info(PCXFileInfo *info)
{
    if (info == NULL)
    {
        return;
    }

    memset(info, 0, sizeof(*info));
    pcx_diagnostics_init(&info->diagnostics);
}

static void pcx_fill_file_info(const PCXHeader *hdr,
                               int width,
                               int height,
                               PCXFileInfo *outInfo)
{
    int totalBits;

    if (outInfo == NULL || hdr == NULL)
    {
        return;
    }

    pcx_init_file_info(outInfo);
    outInfo->header = *hdr;
    outInfo->width = width;
    outInfo->height = height;
    outInfo->usesExtendedPaletteHint = pcx_header_uses_extended_palette(hdr) ? 1 : 0;
    outInfo->strictHeaderPasses =
        (pcx_validate_header_strict(hdr, width, height) == PCX_OK) ? 1 : 0;
    (void)pcx_collect_header_diagnostics(hdr,
                                         width,
                                         height,
                                         &outInfo->diagnostics);

    if (pcx_header_get_total_bpp(hdr, &totalBits))
    {
        outInfo->totalBitsPerPixel = totalBits;
    }

    if (pcx_header_is_truecolor24(hdr))
    {
        outInfo->decodedFormat = PCX_DECODED_FORMAT_RGB;
        outInfo->channelsAfterDecode = 3;
    }
    else if (outInfo->totalBitsPerPixel > 0 && outInfo->totalBitsPerPixel <= 8)
    {
        outInfo->decodedFormat = PCX_DECODED_FORMAT_INDEXED;
        outInfo->channelsAfterDecode = 3;
    }
    else
    {
        outInfo->decodedFormat = PCX_DECODED_FORMAT_UNKNOWN;
        outInfo->channelsAfterDecode = 0;
    }
}

static int pcx_classify_mode(const PCXHeader *hdr)
{
    int totalBits;

    if (hdr == NULL)
    {
        return PCX_ERR_NULL_POINTER;
    }

    if (pcx_header_is_truecolor24(hdr))
    {
        return PCX_INTERNAL_MODE_RGB24;
    }

    if (!pcx_header_get_total_bpp(hdr, &totalBits))
    {
        return PCX_ERR_OVERFLOW;
    }

    if (totalBits <= 0)
    {
        return PCX_ERR_FORMAT;
    }

    if (totalBits <= 8)
    {
        if (totalBits == 8)
        {
            return PCX_INTERNAL_MODE_INDEXED_8;
        }
        return PCX_INTERNAL_MODE_INDEXED_LOW;
    }

    return PCX_ERR_UNSUPPORTED;
}

static int pcx_decode_rle_bytes(FILE *f, pcx_u8 *dst, int expectedSize)
{
    int written;

    if (f == NULL || dst == NULL)
    {
        return PCX_ERR_NULL_POINTER;
    }

    if (expectedSize < 0)
    {
        return PCX_ERR_FORMAT;
    }

    written = 0;
    while (written < expectedSize)
    {
        pcx_u8 b;

        if (!pcx_read_u8(f, &b))
        {
            return feof(f) ? PCX_ERR_EOF : PCX_ERR_IO;
        }

        if ((b & 0xC0) == 0xC0)
        {
            int runLength;
            pcx_u8 value;

            runLength = (int)(b & 0x3F);
            if (runLength == 0)
            {
                return PCX_ERR_FORMAT;
            }

            if (!pcx_read_u8(f, &value))
            {
                return feof(f) ? PCX_ERR_EOF : PCX_ERR_IO;
            }

            if (written + runLength > expectedSize)
            {
                return PCX_ERR_FORMAT;
            }

            memset(dst + written, (int)value, (pcx_size)runLength);
            written += runLength;
        }
        else
        {
            dst[written++] = b;
        }
    }

    return PCX_OK;
}

static int pcx_decode_plane_bytes(FILE *f,
                                  const PCXHeader *hdr,
                                  pcx_u8 *dst,
                                  int expectedSize)
{
    if (f == NULL || hdr == NULL || dst == NULL)
    {
        return PCX_ERR_NULL_POINTER;
    }

    if (expectedSize < 0)
    {
        return PCX_ERR_FORMAT;
    }

    if (hdr->encoding == 0)
    {
        if (!pcx_read_bytes(f, dst, (pcx_size)expectedSize))
        {
            return feof(f) ? PCX_ERR_EOF : PCX_ERR_IO;
        }
        return PCX_OK;
    }

    if (hdr->encoding == 1)
    {
        return pcx_decode_rle_bytes(f, dst, expectedSize);
    }

    return PCX_ERR_UNSUPPORTED;
}

static int pcx_decode_row_planes(FILE *f,
                                 const PCXHeader *hdr,
                                 pcx_u8 *dstRow)
{
    int plane;
    int bytesPerLine;

    if (f == NULL || hdr == NULL || dstRow == NULL)
    {
        return PCX_ERR_NULL_POINTER;
    }

    bytesPerLine = (int)hdr->bytesPerLine;
    if (bytesPerLine <= 0)
    {
        return PCX_ERR_FORMAT;
    }

    for (plane = 0; plane < (int)hdr->nPlanes; ++plane)
    {
        int result;

        result = pcx_decode_plane_bytes(f,
                                        hdr,
                                        dstRow + (pcx_size)plane * (pcx_size)bytesPerLine,
                                        bytesPerLine);
        if (result != PCX_OK)
        {
            return result;
        }
    }

    return PCX_OK;
}

static pcx_u8 pcx_unpack_packed_sample(const pcx_u8 *plane,
                                       int bitsPerPixel,
                                       int x)
{
    int samplesPerByte;
    int byteIndex;
    int sampleIndex;
    int shift;
    int mask;

    samplesPerByte = 8 / bitsPerPixel;
    byteIndex = x / samplesPerByte;
    sampleIndex = x % samplesPerByte;
    shift = 8 - bitsPerPixel * (sampleIndex + 1);
    mask = (1 << bitsPerPixel) - 1;

    return (pcx_u8)((plane[byteIndex] >> shift) & mask);
}

static int pcx_unpack_index_row(const PCXHeader *hdr,
                                int width,
                                const pcx_u8 *rowPlanes,
                                pcx_u8 *outIndices)
{
    int x;
    int plane;
    int bitsPerPixel;
    int bytesPerLine;

    if (hdr == NULL || rowPlanes == NULL || outIndices == NULL)
    {
        return PCX_ERR_NULL_POINTER;
    }

    bitsPerPixel = (int)hdr->bitsPerPixel;
    bytesPerLine = (int)hdr->bytesPerLine;

    if (!(bitsPerPixel == 1 || bitsPerPixel == 2 || bitsPerPixel == 4 || bitsPerPixel == 8))
    {
        return PCX_ERR_UNSUPPORTED;
    }

    for (x = 0; x < width; ++x)
    {
        unsigned int index;

        index = 0U;
        for (plane = 0; plane < (int)hdr->nPlanes; ++plane)
        {
            const pcx_u8 *planePtr;
            unsigned int sample;

            planePtr = rowPlanes + (pcx_size)plane * (pcx_size)bytesPerLine;
            sample = (unsigned int)pcx_unpack_packed_sample(planePtr, bitsPerPixel, x);
            index |= (sample << (plane * bitsPerPixel));
        }

        outIndices[x] = (pcx_u8)index;
    }

    return PCX_OK;
}

static int pcx_decode_index_rows(FILE *f,
                                 const PCXHeader *hdr,
                                 int width,
                                 int height,
                                 pcx_u8 *outIndices,
                                 pcx_off *outDataEndOffset)
{
    pcx_u8 *rowPlanes;
    pcx_u8 *rowIndices;
    pcx_size rowPlanesSize;
    int y;
    int result;

    if (f == NULL || hdr == NULL || outIndices == NULL)
    {
        return PCX_ERR_NULL_POINTER;
    }

    if (!pcx_mul_pcx_size_safe((pcx_size)hdr->bytesPerLine,
                             (pcx_size)hdr->nPlanes,
                             &rowPlanesSize))
    {
        return PCX_ERR_OVERFLOW;
    }

    if (rowPlanesSize > PCX89_STATIC_SCANLINE_BYTES ||
        (pcx_size)width > PCX89_STATIC_ROW_INDEX_BYTES)
    {
        return PCX_ERR_LIMITS;
    }
    rowPlanes = g_pcx_decode_row_store;
    rowIndices = g_pcx_decode_row_index_store;

    for (y = 0; y < height; ++y)
    {
        result = pcx_decode_row_planes(f, hdr, rowPlanes);
        if (result != PCX_OK)
        {
            return result;
        }

        result = pcx_unpack_index_row(hdr, width, rowPlanes, rowIndices);
        if (result != PCX_OK)
        {
            return result;
        }

        memcpy(outIndices + (pcx_size)y * (pcx_size)width, rowIndices, (pcx_size)width);
    }

    if (outDataEndOffset != NULL)
    {
        pcx_off dataEndOffset;

        dataEndOffset = ftell(f);
        if (dataEndOffset < 0)
        {
            return PCX_ERR_IO;
        }
        *outDataEndOffset = dataEndOffset;
    }

    return PCX_OK;
}

static int pcx_decode_indexed_to_rgb(FILE *f,
                                     const PCXHeader *hdr,
                                     int width,
                                     int height,
                                     PCXImage *outImage,
                                     PCXFileInfo *outInfo)
{
    pcx_u8 *allIndices;
    pcx_size totalIndicesSize;
    PCXPalette palette;
    pcx_off dataEndOffset;
    int result;

    if (f == NULL || hdr == NULL || outImage == NULL)
    {
        return PCX_ERR_NULL_POINTER;
    }

    if (!pcx_mul_pcx_size_safe((pcx_size)width, (pcx_size)height, &totalIndicesSize))
    {
        return PCX_ERR_OVERFLOW;
    }

    if (totalIndicesSize > PCX89_STATIC_INDEX_BYTES)
    {
        return PCX_ERR_LIMITS;
    }
    allIndices = g_pcx_decode_index_store;

    dataEndOffset = -1;
    result = pcx_decode_index_rows(f, hdr, width, height, allIndices, &dataEndOffset);
    if (result != PCX_OK)
    {
        return result;
    }

    result = pcx_load_palette_ex(f,
                                 hdr,
                                 &palette,
                                 dataEndOffset,
                                 (outInfo != NULL) ? &outInfo->diagnostics : NULL);
    if (result != PCX_OK)
    {
        return result;
    }

    result = pcx_indexed_to_rgb(allIndices, &palette, width, height, outImage->pixels);
    return result;
}

static int pcx_decode_truecolor24(FILE *f,
                                  const PCXHeader *hdr,
                                  int width,
                                  int height,
                                  PCXImage *outImage)
{
    pcx_u8 *rowPlanes;
    int y;
    int x;
    int bytesPerLine;
    pcx_size rowPlanesSize;
    pcx_size rowRgbSize;
    int result;

    if (f == NULL || hdr == NULL || outImage == NULL)
    {
        return PCX_ERR_NULL_POINTER;
    }

    bytesPerLine = (int)hdr->bytesPerLine;
    if (bytesPerLine < width || hdr->nPlanes != 3 || hdr->bitsPerPixel != 8)
    {
        return PCX_ERR_FORMAT;
    }

    if (!pcx_mul_pcx_size_safe((pcx_size)bytesPerLine, 3U, &rowPlanesSize))
    {
        return PCX_ERR_OVERFLOW;
    }

    if (!pcx_mul_pcx_size_safe((pcx_size)width, 3U, &rowRgbSize))
    {
        return PCX_ERR_OVERFLOW;
    }

    if (rowPlanesSize > PCX89_STATIC_SCANLINE_BYTES)
    {
        return PCX_ERR_LIMITS;
    }
    rowPlanes = g_pcx_decode_row_store;

    for (y = 0; y < height; ++y)
    {
        pcx_u8 *dst;
        const pcx_u8 *planeR;
        const pcx_u8 *planeG;
        const pcx_u8 *planeB;

        result = pcx_decode_row_planes(f, hdr, rowPlanes);
        if (result != PCX_OK)
        {
            return result;
        }

        dst = outImage->pixels + (pcx_size)y * rowRgbSize;
        planeR = rowPlanes;
        planeG = rowPlanes + bytesPerLine;
        planeB = rowPlanes + bytesPerLine * 2;

        for (x = 0; x < width; ++x)
        {
            dst[0] = planeR[x];
            dst[1] = planeG[x];
            dst[2] = planeB[x];
            dst += 3;
        }
    }

    return PCX_OK;
}

static int pcx_load_indexed_payload(FILE *f,
                                    const PCXHeader *hdr,
                                    int width,
                                    int height,
                                    PCXIndexedImage *outImage,
                                    PCXFileInfo *outInfo)
{
    int totalBits;
    int result;
    pcx_off dataEndOffset;

    if (f == NULL || hdr == NULL || outImage == NULL)
    {
        return PCX_ERR_NULL_POINTER;
    }

    if (!pcx_header_get_total_bpp(hdr, &totalBits))
    {
        return PCX_ERR_OVERFLOW;
    }

    if (totalBits <= 0 || totalBits > 8)
    {
        return PCX_ERR_UNSUPPORTED;
    }

    result = pcx_indexed_image_use_static(outImage, width, height, totalBits);
    if (result != PCX_OK)
    {
        return result;
    }

    dataEndOffset = -1;
    result = pcx_decode_index_rows(f,
                                   hdr,
                                   width,
                                   height,
                                   outImage->indices,
                                   &dataEndOffset);
    if (result != PCX_OK)
    {
        pcx_indexed_image_release(outImage);
        return result;
    }

    result = pcx_load_palette_ex(f,
                                hdr,
                                &outImage->palette,
                                dataEndOffset,
                                (outInfo != NULL) ? &outInfo->diagnostics : NULL);
    if (result != PCX_OK)
    {
        pcx_indexed_image_release(outImage);
        return result;
    }

    outImage->totalBitsPerPixel = totalBits;
    return PCX_OK;
}

static int pcx_load_fp_with_info_flags(FILE *f,
                                       PCXImage *outImage,
                                       PCXFileInfo *outInfo,
                                       unsigned parseFlags,
                                       const PCXDecodeLimits *limits)
{
    PCXHeader hdr;
    int width;
    int height;
    int mode;
    int result;

    if (outImage == NULL)
    {
        return PCX_ERR_NULL_POINTER;
    }

    pcx_image_init(outImage);
    pcx_init_file_info(outInfo);

    if (f == NULL)
    {
        return PCX_ERR_NULL_POINTER;
    }

    result = pcx_check_stream_input_limit(f, limits);
    if (result != PCX_OK)
    {
        return result;
    }

    result = pcx_parse_header_ex(f, &hdr, &width, &height, parseFlags);
    if (result != PCX_OK)
    {
        return result;
    }

    result = pcx_check_parsed_limits(&hdr, width, height, limits);
    if (result != PCX_OK)
    {
        return result;
    }

    pcx_fill_file_info(&hdr, width, height, outInfo);

    mode = pcx_classify_mode(&hdr);
    if (mode < 0)
    {
        return mode;
    }

    result = pcx_check_decoded_limits(width, height, 3, limits);
    if (result != PCX_OK)
    {
        return result;
    }

    result = pcx_image_use_static(outImage, width, height, 3);
    if (result != PCX_OK)
    {
        return result;
    }

    switch (mode)
    {
    case PCX_INTERNAL_MODE_INDEXED_LOW:
    case PCX_INTERNAL_MODE_INDEXED_8:
        result = pcx_decode_indexed_to_rgb(f, &hdr, width, height, outImage, outInfo);
        break;
    case PCX_INTERNAL_MODE_RGB24:
        result = pcx_decode_truecolor24(f, &hdr, width, height, outImage);
        break;
    default:
        result = PCX_ERR_UNSUPPORTED;
        break;
    }

    if (result != PCX_OK)
    {
        pcx_image_release(outImage);
        return result;
    }

    return PCX_OK;
}

int pcx_load_fp_ex(FILE *f,
                   PCXImage *outImage,
                   PCXFileInfo *outInfo,
                   unsigned parseFlags,
                   const PCXDecodeLimits *limits)
{
    return pcx_load_fp_with_info_flags(f, outImage, outInfo, parseFlags, limits);
}

int pcx_load_file_ex(const char *filename,
                     PCXImage *outImage,
                     PCXFileInfo *outInfo,
                     unsigned parseFlags,
                     const PCXDecodeLimits *limits)
{
    FILE *f;
    int result;

    if (filename == NULL || outImage == NULL)
    {
        return PCX_ERR_NULL_POINTER;
    }

    f = fopen(filename, "rb");
    if (f == NULL)
    {
        return PCX_ERR_IO;
    }

    result = pcx_load_fp_ex(f, outImage, outInfo, parseFlags, limits);
    fclose(f);
    return result;
}

int pcx_load_memory_ex(const void *data,
                       pcx_size size,
                       PCXImage *outImage,
                       PCXFileInfo *outInfo,
                       unsigned parseFlags,
                       const PCXDecodeLimits *limits)
{
    FILE *f;
    int result;

    if (outImage == NULL)
    {
        return PCX_ERR_NULL_POINTER;
    }

    result = pcx_open_memory_stream(data, size, limits, &f);
    if (result != PCX_OK)
    {
        return result;
    }

    result = pcx_load_fp_ex(f, outImage, outInfo, parseFlags, limits);
    fclose(f);
    return result;
}

int pcx_load_fp_with_info(FILE *f,
                          PCXImage *outImage,
                          PCXFileInfo *outInfo)
{
    return pcx_load_fp_with_info_flags(f, outImage, outInfo, PCX_PARSE_FLAG_NONE, NULL);
}

int pcx_load_fp_strict_with_info(FILE *f,
                                 PCXImage *outImage,
                                 PCXFileInfo *outInfo)
{
    return pcx_load_fp_with_info_flags(f,
                                       outImage,
                                       outInfo,
                                       PCX_PARSE_FLAG_STRICT,
                                       NULL);
}

int pcx_load_fp(FILE *f, PCXImage *outImage)
{
    return pcx_load_fp_with_info(f, outImage, NULL);
}

int pcx_load_with_info(const char *filename,
                       PCXImage *outImage,
                       PCXFileInfo *outInfo)
{
    FILE *f;
    int result;

    if (filename == NULL || outImage == NULL)
    {
        return PCX_ERR_NULL_POINTER;
    }

    f = fopen(filename, "rb");
    if (f == NULL)
    {
        return PCX_ERR_IO;
    }

    result = pcx_load_fp_with_info(f, outImage, outInfo);
    fclose(f);
    return result;
}

int pcx_load_strict_with_info(const char *filename,
                              PCXImage *outImage,
                              PCXFileInfo *outInfo)
{
    FILE *f;
    int result;

    if (filename == NULL || outImage == NULL)
    {
        return PCX_ERR_NULL_POINTER;
    }

    f = fopen(filename, "rb");
    if (f == NULL)
    {
        return PCX_ERR_IO;
    }

    result = pcx_load_fp_strict_with_info(f, outImage, outInfo);
    fclose(f);
    return result;
}

int pcx_load_memory_with_info(const void *data,
                              pcx_size size,
                              PCXImage *outImage,
                              PCXFileInfo *outInfo)
{
    FILE *f;
    int result;

    if (outImage == NULL)
    {
        return PCX_ERR_NULL_POINTER;
    }

    result = pcx_open_memory_stream(data, size, NULL, &f);
    if (result != PCX_OK)
    {
        return result;
    }

    result = pcx_load_fp_with_info(f, outImage, outInfo);
    fclose(f);
    return result;
}

int pcx_load_memory_strict_with_info(const void *data,
                                     pcx_size size,
                                     PCXImage *outImage,
                                     PCXFileInfo *outInfo)
{
    FILE *f;
    int result;

    if (outImage == NULL)
    {
        return PCX_ERR_NULL_POINTER;
    }

    result = pcx_open_memory_stream(data, size, NULL, &f);
    if (result != PCX_OK)
    {
        return result;
    }

    result = pcx_load_fp_strict_with_info(f, outImage, outInfo);
    fclose(f);
    return result;
}

int pcx_load(const char *filename, PCXImage *outImage)
{
    return pcx_load_with_info(filename, outImage, NULL);
}

int pcx_load_strict(const char *filename, PCXImage *outImage)
{
    return pcx_load_strict_with_info(filename, outImage, NULL);
}

int pcx_load_memory(const void *data, pcx_size size, PCXImage *outImage)
{
    return pcx_load_memory_with_info(data, size, outImage, NULL);
}

int pcx_load_memory_strict(const void *data, pcx_size size, PCXImage *outImage)
{
    return pcx_load_memory_strict_with_info(data, size, outImage, NULL);
}

int pcx_load_fp_strict(FILE *f, PCXImage *outImage)
{
    return pcx_load_fp_strict_with_info(f, outImage, NULL);
}

static int pcx_load_fp_indexed_with_info_flags(FILE *f,
                                               PCXIndexedImage *outImage,
                                               PCXFileInfo *outInfo,
                                               unsigned parseFlags,
                                               const PCXDecodeLimits *limits)
{
    PCXHeader hdr;
    int width;
    int height;
    int mode;
    int result;

    if (outImage == NULL)
    {
        return PCX_ERR_NULL_POINTER;
    }

    pcx_indexed_image_init(outImage);
    pcx_init_file_info(outInfo);

    if (f == NULL)
    {
        return PCX_ERR_NULL_POINTER;
    }

    result = pcx_check_stream_input_limit(f, limits);
    if (result != PCX_OK)
    {
        return result;
    }

    result = pcx_parse_header_ex(f, &hdr, &width, &height, parseFlags);
    if (result != PCX_OK)
    {
        return result;
    }

    result = pcx_check_parsed_limits(&hdr, width, height, limits);
    if (result != PCX_OK)
    {
        return result;
    }

    pcx_fill_file_info(&hdr, width, height, outInfo);

    mode = pcx_classify_mode(&hdr);
    if (mode < 0)
    {
        return mode;
    }

    if (mode == PCX_INTERNAL_MODE_RGB24)
    {
        return PCX_ERR_UNSUPPORTED;
    }

    result = pcx_check_decoded_limits(width, height, 1, limits);
    if (result != PCX_OK)
    {
        return result;
    }

    return pcx_load_indexed_payload(f, &hdr, width, height, outImage, outInfo);
}

int pcx_load_fp_indexed_ex(FILE *f,
                           PCXIndexedImage *outImage,
                           PCXFileInfo *outInfo,
                           unsigned parseFlags,
                           const PCXDecodeLimits *limits)
{
    return pcx_load_fp_indexed_with_info_flags(f,
                                               outImage,
                                               outInfo,
                                               parseFlags,
                                               limits);
}

int pcx_load_file_indexed_ex(const char *filename,
                             PCXIndexedImage *outImage,
                             PCXFileInfo *outInfo,
                             unsigned parseFlags,
                             const PCXDecodeLimits *limits)
{
    FILE *f;
    int result;

    if (filename == NULL || outImage == NULL)
    {
        return PCX_ERR_NULL_POINTER;
    }

    f = fopen(filename, "rb");
    if (f == NULL)
    {
        return PCX_ERR_IO;
    }

    result = pcx_load_fp_indexed_ex(f, outImage, outInfo, parseFlags, limits);
    fclose(f);
    return result;
}

int pcx_load_memory_indexed_ex(const void *data,
                               pcx_size size,
                               PCXIndexedImage *outImage,
                               PCXFileInfo *outInfo,
                               unsigned parseFlags,
                               const PCXDecodeLimits *limits)
{
    FILE *f;
    int result;

    if (outImage == NULL)
    {
        return PCX_ERR_NULL_POINTER;
    }

    result = pcx_open_memory_stream(data, size, limits, &f);
    if (result != PCX_OK)
    {
        return result;
    }

    result = pcx_load_fp_indexed_ex(f, outImage, outInfo, parseFlags, limits);
    fclose(f);
    return result;
}

int pcx_load_fp_indexed_with_info(FILE *f,
                                  PCXIndexedImage *outImage,
                                  PCXFileInfo *outInfo)
{
    return pcx_load_fp_indexed_with_info_flags(f,
                                               outImage,
                                               outInfo,
                                               PCX_PARSE_FLAG_NONE,
                                               NULL);
}

int pcx_load_fp_indexed_strict_with_info(FILE *f,
                                         PCXIndexedImage *outImage,
                                         PCXFileInfo *outInfo)
{
    return pcx_load_fp_indexed_with_info_flags(f,
                                               outImage,
                                               outInfo,
                                               PCX_PARSE_FLAG_STRICT,
                                               NULL);
}

int pcx_load_fp_indexed(FILE *f, PCXIndexedImage *outImage)
{
    return pcx_load_fp_indexed_with_info(f, outImage, NULL);
}

int pcx_load_indexed_with_info(const char *filename,
                               PCXIndexedImage *outImage,
                               PCXFileInfo *outInfo)
{
    FILE *f;
    int result;

    if (filename == NULL || outImage == NULL)
    {
        return PCX_ERR_NULL_POINTER;
    }

    f = fopen(filename, "rb");
    if (f == NULL)
    {
        return PCX_ERR_IO;
    }

    result = pcx_load_fp_indexed_with_info(f, outImage, outInfo);
    fclose(f);
    return result;
}

int pcx_load_indexed_strict_with_info(const char *filename,
                                      PCXIndexedImage *outImage,
                                      PCXFileInfo *outInfo)
{
    FILE *f;
    int result;

    if (filename == NULL || outImage == NULL)
    {
        return PCX_ERR_NULL_POINTER;
    }

    f = fopen(filename, "rb");
    if (f == NULL)
    {
        return PCX_ERR_IO;
    }

    result = pcx_load_fp_indexed_strict_with_info(f, outImage, outInfo);
    fclose(f);
    return result;
}

int pcx_load_indexed_memory_with_info(const void *data,
                                      pcx_size size,
                                      PCXIndexedImage *outImage,
                                      PCXFileInfo *outInfo)
{
    FILE *f;
    int result;

    if (outImage == NULL)
    {
        return PCX_ERR_NULL_POINTER;
    }

    result = pcx_open_memory_stream(data, size, NULL, &f);
    if (result != PCX_OK)
    {
        return result;
    }

    result = pcx_load_fp_indexed_with_info(f, outImage, outInfo);
    fclose(f);
    return result;
}

int pcx_load_indexed_memory_strict_with_info(const void *data,
                                             pcx_size size,
                                             PCXIndexedImage *outImage,
                                             PCXFileInfo *outInfo)
{
    FILE *f;
    int result;

    if (outImage == NULL)
    {
        return PCX_ERR_NULL_POINTER;
    }

    result = pcx_open_memory_stream(data, size, NULL, &f);
    if (result != PCX_OK)
    {
        return result;
    }

    result = pcx_load_fp_indexed_strict_with_info(f, outImage, outInfo);
    fclose(f);
    return result;
}

int pcx_load_indexed(const char *filename, PCXIndexedImage *outImage)
{
    return pcx_load_indexed_with_info(filename, outImage, NULL);
}

int pcx_load_indexed_strict(const char *filename, PCXIndexedImage *outImage)
{
    return pcx_load_indexed_strict_with_info(filename, outImage, NULL);
}

int pcx_load_indexed_memory(const void *data,
                            pcx_size size,
                            PCXIndexedImage *outImage)
{
    return pcx_load_indexed_memory_with_info(data, size, outImage, NULL);
}

int pcx_load_indexed_memory_strict(const void *data,
                                   pcx_size size,
                                   PCXIndexedImage *outImage)
{
    return pcx_load_indexed_memory_strict_with_info(data, size, outImage, NULL);
}

int pcx_load_fp_indexed_strict(FILE *f, PCXIndexedImage *outImage)
{
    return pcx_load_fp_indexed_strict_with_info(f, outImage, NULL);
}

int pcx_inspect_fp_with_limits(FILE *f,
                               unsigned parseFlags,
                               const PCXDecodeLimits *limits,
                               PCXHeader *outHeader,
                               int *outWidth,
                               int *outHeight)
{
    return pcx_inspect_stream(f,
                              parseFlags,
                              limits,
                              outHeader,
                              outWidth,
                              outHeight);
}

int pcx_inspect_file_with_limits(const char *filename,
                                 unsigned parseFlags,
                                 const PCXDecodeLimits *limits,
                                 PCXHeader *outHeader,
                                 int *outWidth,
                                 int *outHeight)
{
    FILE *f;
    int result;

    if (filename == NULL || outHeader == NULL || outWidth == NULL || outHeight == NULL)
    {
        return PCX_ERR_NULL_POINTER;
    }

    f = fopen(filename, "rb");
    if (f == NULL)
    {
        return PCX_ERR_IO;
    }

    result = pcx_inspect_fp_with_limits(f,
                                        parseFlags,
                                        limits,
                                        outHeader,
                                        outWidth,
                                        outHeight);
    fclose(f);
    return result;
}

int pcx_inspect_memory_with_limits(const void *data,
                                   pcx_size size,
                                   unsigned parseFlags,
                                   const PCXDecodeLimits *limits,
                                   PCXHeader *outHeader,
                                   int *outWidth,
                                   int *outHeight)
{
    FILE *f;
    int result;

    result = pcx_open_memory_stream(data, size, limits, &f);
    if (result != PCX_OK)
    {
        return result;
    }

    result = pcx_inspect_fp_with_limits(f,
                                        parseFlags,
                                        limits,
                                        outHeader,
                                        outWidth,
                                        outHeight);
    fclose(f);
    return result;
}

int pcx_inspect_fp_info_with_limits(FILE *f,
                                    const PCXDecodeLimits *limits,
                                    PCXFileInfo *outInfo)
{
    return pcx_inspect_stream_ex(f, limits, outInfo);
}

int pcx_inspect_file_info_with_limits(const char *filename,
                                      const PCXDecodeLimits *limits,
                                      PCXFileInfo *outInfo)
{
    FILE *f;
    int result;

    if (filename == NULL || outInfo == NULL)
    {
        return PCX_ERR_NULL_POINTER;
    }

    f = fopen(filename, "rb");
    if (f == NULL)
    {
        return PCX_ERR_IO;
    }

    result = pcx_inspect_fp_info_with_limits(f, limits, outInfo);
    fclose(f);
    return result;
}

int pcx_inspect_memory_info_with_limits(const void *data,
                                        pcx_size size,
                                        const PCXDecodeLimits *limits,
                                        PCXFileInfo *outInfo)
{
    FILE *f;
    int result;

    result = pcx_open_memory_stream(data, size, limits, &f);
    if (result != PCX_OK)
    {
        return result;
    }

    result = pcx_inspect_fp_info_with_limits(f, limits, outInfo);
    fclose(f);
    return result;
}

int pcx_inspect_file(const char *filename,
                     PCXHeader *outHeader,
                     int *outWidth,
                     int *outHeight)
{
    FILE *f;
    int result;

    if (filename == NULL || outHeader == NULL || outWidth == NULL || outHeight == NULL)
    {
        return PCX_ERR_NULL_POINTER;
    }

    f = fopen(filename, "rb");
    if (f == NULL)
    {
        return PCX_ERR_IO;
    }

    result = pcx_inspect_stream(f, PCX_PARSE_FLAG_NONE, NULL, outHeader, outWidth, outHeight);
    fclose(f);
    return result;
}

int pcx_inspect_memory(const void *data,
                       pcx_size size,
                       PCXHeader *outHeader,
                       int *outWidth,
                       int *outHeight)
{
    FILE *f;
    int result;

    result = pcx_open_memory_stream(data, size, NULL, &f);
    if (result != PCX_OK)
    {
        return result;
    }

    result = pcx_inspect_stream(f,
                                PCX_PARSE_FLAG_NONE,
                                NULL,
                                outHeader,
                                outWidth,
                                outHeight);
    fclose(f);
    return result;
}

int pcx_inspect_file_strict(const char *filename,
                            PCXHeader *outHeader,
                            int *outWidth,
                            int *outHeight)
{
    FILE *f;
    int result;

    if (filename == NULL || outHeader == NULL || outWidth == NULL || outHeight == NULL)
    {
        return PCX_ERR_NULL_POINTER;
    }

    f = fopen(filename, "rb");
    if (f == NULL)
    {
        return PCX_ERR_IO;
    }

    result = pcx_inspect_stream(f,
                                PCX_PARSE_FLAG_STRICT,
                                NULL,
                                outHeader,
                                outWidth,
                                outHeight);
    fclose(f);
    return result;
}

int pcx_inspect_memory_strict(const void *data,
                              pcx_size size,
                              PCXHeader *outHeader,
                              int *outWidth,
                              int *outHeight)
{
    FILE *f;
    int result;

    result = pcx_open_memory_stream(data, size, NULL, &f);
    if (result != PCX_OK)
    {
        return result;
    }

    result = pcx_inspect_stream(f,
                                PCX_PARSE_FLAG_STRICT,
                                NULL,
                                outHeader,
                                outWidth,
                                outHeight);
    fclose(f);
    return result;
}

int pcx_inspect_file_ex(const char *filename, PCXFileInfo *outInfo)
{
    FILE *f;
    int result;

    if (filename == NULL || outInfo == NULL)
    {
        return PCX_ERR_NULL_POINTER;
    }

    f = fopen(filename, "rb");
    if (f == NULL)
    {
        return PCX_ERR_IO;
    }

    result = pcx_inspect_stream_ex(f, NULL, outInfo);
    fclose(f);
    return result;
}

int pcx_inspect_memory_ex(const void *data, pcx_size size, PCXFileInfo *outInfo)
{
    FILE *f;
    int result;

    result = pcx_open_memory_stream(data, size, NULL, &f);
    if (result != PCX_OK)
    {
        return result;
    }

    result = pcx_inspect_stream_ex(f, NULL, outInfo);
    fclose(f);
    return result;
}
