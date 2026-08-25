/* pcx_render.c - implementación de paletas y conversiones RGB */

#include "pcx_render.h"
#include <string.h>

static int pcx_try_read_extended_palette(FILE *f,
                                         PCXPalette *pal,
                                         pcx_off fileSize,
                                         pcx_off minimumPaletteOffset)
{
    pcx_off palettePos;
    pcx_u8 marker;
    int i;

    if (f == NULL || pal == NULL)
    {
        return PCX_ERR_NULL_POINTER;
    }

    if (fileSize < 769)
    {
        return PCX_ERR_FORMAT;
    }

    palettePos = fileSize - 769;
    if (palettePos < minimumPaletteOffset)
    {
        return PCX_ERR_FORMAT;
    }

    if (fseek(f, palettePos, SEEK_SET) != 0)
    {
        return PCX_ERR_IO;
    }

    if (!pcx_read_u8(f, &marker))
    {
        return feof(f) ? PCX_ERR_EOF : PCX_ERR_IO;
    }

    if (marker != 0x0C)
    {
        return PCX_ERR_FORMAT;
    }

    for (i = 0; i < 256; ++i)
    {
        if (!pcx_read_u8(f, &pal->colors[i][0]) ||
            !pcx_read_u8(f, &pal->colors[i][1]) ||
            !pcx_read_u8(f, &pal->colors[i][2]))
        {
            return feof(f) ? PCX_ERR_EOF : PCX_ERR_IO;
        }
    }

    pal->isValid = 1;
    return PCX_OK;
}

static void pcx_fix_monochrome_palette_if_needed(PCXPalette *pal)
{
    if (pal == NULL)
    {
        return;
    }

    if (pal->colors[0][0] == pal->colors[1][0] &&
        pal->colors[0][1] == pal->colors[1][1] &&
        pal->colors[0][2] == pal->colors[1][2])
    {
        pal->colors[0][0] = 0;
        pal->colors[0][1] = 0;
        pal->colors[0][2] = 0;
        pal->colors[1][0] = 255;
        pal->colors[1][1] = 255;
        pal->colors[1][2] = 255;
    }
}

int pcx_palette_build_from_header16(const PCXHeader *hdr, PCXPalette *pal)
{
    int i;
    int c;

    if (hdr == NULL || pal == NULL)
    {
        return PCX_ERR_NULL_POINTER;
    }

    pcx_palette_init(pal);

    for (i = 0; i < 16; ++i)
    {
        pal->colors[i][0] = hdr->colorMap[i * 3 + 0];
        pal->colors[i][1] = hdr->colorMap[i * 3 + 1];
        pal->colors[i][2] = hdr->colorMap[i * 3 + 2];
    }

    for (i = 16; i < 256; ++i)
    {
        c = i & 15;
        pal->colors[i][0] = pal->colors[c][0];
        pal->colors[i][1] = pal->colors[c][1];
        pal->colors[i][2] = pal->colors[c][2];
    }

    pcx_fix_monochrome_palette_if_needed(pal);
    pal->isValid = 1;
    return PCX_OK;
}

int pcx_palette_build_grayscale256(PCXPalette *pal)
{
    int i;

    if (pal == NULL)
    {
        return PCX_ERR_NULL_POINTER;
    }

    pcx_palette_init(pal);

    for (i = 0; i < 256; ++i)
    {
        pal->colors[i][0] = (pcx_u8)i;
        pal->colors[i][1] = (pcx_u8)i;
        pal->colors[i][2] = (pcx_u8)i;
    }

    pal->isValid = 1;
    return PCX_OK;
}

int pcx_load_palette_ex(FILE *f,
                        const PCXHeader *hdr,
                        PCXPalette *pal,
                        pcx_off minimumPaletteOffset,
                        PCXDiagnostics *outDiag)
{
    pcx_off fileSize;
    pcx_off restorePos;
    int result;

    if (f == NULL || hdr == NULL || pal == NULL)
    {
        return PCX_ERR_NULL_POINTER;
    }

    restorePos = ftell(f);
    if (restorePos < 0)
    {
        return PCX_ERR_IO;
    }

    if (minimumPaletteOffset < 0)
    {
        minimumPaletteOffset = restorePos;
    }

    if (pcx_header_uses_extended_palette(hdr))
    {
        if (fseek(f, 0, SEEK_END) != 0)
        {
            return PCX_ERR_IO;
        }

        fileSize = ftell(f);
        if (fileSize < 0)
        {
            return PCX_ERR_IO;
        }

        result = pcx_try_read_extended_palette(f,
                                               pal,
                                               fileSize,
                                               minimumPaletteOffset);
        if (result == PCX_OK)
        {
            if (outDiag != NULL)
            {
                outDiag->paletteSource = PCX_PALETTE_SOURCE_EXTENDED_VGA;
            }
            if (fseek(f, restorePos, SEEK_SET) != 0)
            {
                return PCX_ERR_IO;
            }
            return PCX_OK;
        }

        if (outDiag != NULL)
        {
            outDiag->warningMask |= PCX_WARN_EXTENDED_PALETTE_MISSING;
        }

        if (pcx_header_is_grayscale_hint(hdr))
        {
            result = pcx_palette_build_grayscale256(pal);
            if (outDiag != NULL && result == PCX_OK)
            {
                outDiag->warningMask |= PCX_WARN_GRAYSCALE_FALLBACK;
                outDiag->paletteSource = PCX_PALETTE_SOURCE_GRAYSCALE_FALLBACK;
            }
        }
        else
        {
            result = pcx_palette_build_from_header16(hdr, pal);
            if (outDiag != NULL && result == PCX_OK)
            {
                outDiag->warningMask |= PCX_WARN_HEADER16_PALETTE_FALLBACK;
                outDiag->paletteSource = PCX_PALETTE_SOURCE_HEADER16_FALLBACK;
            }
        }

        if (fseek(f, restorePos, SEEK_SET) != 0)
        {
            return PCX_ERR_IO;
        }
        return result;
    }

    result = pcx_palette_build_from_header16(hdr, pal);
    if (outDiag != NULL && result == PCX_OK)
    {
        outDiag->paletteSource = PCX_PALETTE_SOURCE_HEADER16;
    }
    if (fseek(f, restorePos, SEEK_SET) != 0)
    {
        return PCX_ERR_IO;
    }

    return result;
}

int pcx_load_palette(FILE *f,
                     const PCXHeader *hdr,
                     PCXPalette *pal,
                     pcx_off minimumPaletteOffset)
{
    return pcx_load_palette_ex(f,
                               hdr,
                               pal,
                               minimumPaletteOffset,
                               NULL);
}

int pcx_indexed_row_to_rgb(const pcx_u8 *indices,
                           const PCXPalette *pal,
                           int width,
                           pcx_u8 *outRGB)
{
    int x;
    const pcx_u8 *src;
    pcx_u8 *dst;

    if (indices == NULL || pal == NULL || outRGB == NULL)
    {
        return PCX_ERR_NULL_POINTER;
    }

    if (!pcx_palette_is_valid(pal) || width <= 0)
    {
        return PCX_ERR_FORMAT;
    }

    src = indices;
    dst = outRGB;

    for (x = 0; x < width; ++x)
    {
        int idx = (int)src[x];
        dst[0] = pal->colors[idx][0];
        dst[1] = pal->colors[idx][1];
        dst[2] = pal->colors[idx][2];
        dst += 3;
    }

    return PCX_OK;
}

int pcx_indexed_to_rgb(const pcx_u8 *indices,
                       const PCXPalette *pal,
                       int width,
                       int height,
                       pcx_u8 *outRGB)
{
    int y;
    pcx_size rowIndices;
    pcx_size rowRgb;
    int result;

    if (indices == NULL || pal == NULL || outRGB == NULL)
    {
        return PCX_ERR_NULL_POINTER;
    }

    if (width <= 0 || height <= 0)
    {
        return PCX_ERR_FORMAT;
    }

    rowIndices = (pcx_size)width;
    rowRgb = (pcx_size)width * 3U;

    for (y = 0; y < height; ++y)
    {
        result = pcx_indexed_row_to_rgb(indices + (pcx_size)y * rowIndices,
                                        pal,
                                        width,
                                        outRGB + (pcx_size)y * rowRgb);
        if (result != PCX_OK)
        {
            return result;
        }
    }

    return PCX_OK;
}

int pcx_planar24_to_rgb(const pcx_u8 *planar,
                        int width,
                        int height,
                        int bytesPerLine,
                        pcx_u8 *outRGB)
{
    int y;
    int x;
    int rowStrideRgb;
    const pcx_u8 *row;
    const pcx_u8 *planeR;
    const pcx_u8 *planeG;
    const pcx_u8 *planeB;
    pcx_u8 *dst;

    if (planar == NULL || outRGB == NULL)
    {
        return PCX_ERR_NULL_POINTER;
    }

    if (width <= 0 || height <= 0 || bytesPerLine < width)
    {
        return PCX_ERR_FORMAT;
    }

    rowStrideRgb = width * 3;

    for (y = 0; y < height; ++y)
    {
        row = planar + (pcx_size)y * (pcx_size)bytesPerLine * 3U;
        planeR = row;
        planeG = row + bytesPerLine;
        planeB = row + bytesPerLine * 2;
        dst = outRGB + (pcx_size)y * (pcx_size)rowStrideRgb;

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
