/* pcx_parser.c - implementación del parser de cabecera PCX */

#include "pcx_parser.h"
#include <string.h>

static pcx_u16 pcx_u16_from_le_bytes(const pcx_u8 *p)
{
    return (pcx_u16)(p[0] | ((pcx_u16)p[1] << 8));
}

static int pcx_version_is_known(pcx_u8 version)
{
    return (version == 0U || version == 2U || version == 3U ||
            version == 4U || version == 5U) ? 1 : 0;
}

static int pcx_validate_header_common(const PCXHeader *hdr,
                                      int *outWidth,
                                      int *outHeight)
{
    int width;
    int height;
    int minBytes;

    if (hdr == NULL || outWidth == NULL || outHeight == NULL)
    {
        return PCX_ERR_NULL_POINTER;
    }

    if (hdr->manufacturer != 0x0A)
    {
        return PCX_ERR_FORMAT;
    }

    if (hdr->encoding != 0 && hdr->encoding != 1)
    {
        return PCX_ERR_UNSUPPORTED;
    }

    if (!(hdr->bitsPerPixel == 1 || hdr->bitsPerPixel == 2 ||
          hdr->bitsPerPixel == 4 || hdr->bitsPerPixel == 8))
    {
        return PCX_ERR_UNSUPPORTED;
    }

    if (hdr->nPlanes < 1 || hdr->nPlanes > 4)
    {
        return PCX_ERR_FORMAT;
    }

    if (!pcx_header_get_dimensions(hdr, &width, &height))
    {
        return PCX_ERR_FORMAT;
    }

    if (!pcx_header_get_min_bytes_per_line(hdr, width, &minBytes))
    {
        return PCX_ERR_OVERFLOW;
    }

    if ((int)hdr->bytesPerLine < minBytes || hdr->bytesPerLine == 0U)
    {
        return PCX_ERR_FORMAT;
    }

    *outWidth = width;
    *outHeight = height;
    return PCX_OK;
}

int pcx_validate_header_strict(const PCXHeader *hdr,
                               int width,
                               int height)
{
    int totalBits;

    if (hdr == NULL)
    {
        return PCX_ERR_NULL_POINTER;
    }

    if (width <= 0 || height <= 0)
    {
        return PCX_ERR_DIMENSIONS;
    }

    if (!pcx_version_is_known(hdr->version))
    {
        return PCX_ERR_UNSUPPORTED;
    }

    if (hdr->encoding != 1U)
    {
        return PCX_ERR_UNSUPPORTED;
    }

    if (hdr->reserved != 0U)
    {
        return PCX_ERR_FORMAT;
    }

    if ((hdr->bytesPerLine & 1U) != 0U)
    {
        return PCX_ERR_FORMAT;
    }

    if (pcx_header_is_truecolor24(hdr))
    {
        if (hdr->version != 5U)
        {
            return PCX_ERR_UNSUPPORTED;
        }
        return PCX_OK;
    }

    if (pcx_header_is_indexed8(hdr) && hdr->version != 5U)
    {
        return PCX_ERR_UNSUPPORTED;
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
        return PCX_OK;
    }

    return PCX_ERR_UNSUPPORTED;
}

int pcx_collect_header_diagnostics(const PCXHeader *hdr,
                                   int width,
                                   int height,
                                   PCXDiagnostics *outDiag)
{
    int totalBits;
    int minBytes;

    if (hdr == NULL || outDiag == NULL)
    {
        return PCX_ERR_NULL_POINTER;
    }

    pcx_diagnostics_init(outDiag);

    if (width <= 0 || height <= 0)
    {
        return PCX_ERR_DIMENSIONS;
    }

    if (!pcx_version_is_known(hdr->version))
    {
        outDiag->warningMask |= PCX_WARN_UNKNOWN_VERSION;
    }

    if (hdr->encoding == 0U)
    {
        outDiag->warningMask |= PCX_WARN_RAW_ENCODING;
    }

    if (hdr->reserved != 0U)
    {
        outDiag->warningMask |= PCX_WARN_RESERVED_NONZERO;
    }

    if ((hdr->bytesPerLine & 1U) != 0U)
    {
        outDiag->warningMask |= PCX_WARN_ODD_BYTES_PER_LINE;
    }

    if (pcx_header_get_min_bytes_per_line(hdr, width, &minBytes) &&
        (int)hdr->bytesPerLine > minBytes)
    {
        outDiag->warningMask |= PCX_WARN_SCANLINE_PADDING;
    }

    if ((pcx_header_is_truecolor24(hdr) || pcx_header_is_indexed8(hdr)) &&
        hdr->version != 5U)
    {
        outDiag->warningMask |= PCX_WARN_STRICT_LAYOUT_UNSUPPORTED;
    }

    if (pcx_header_is_truecolor24(hdr))
    {
        outDiag->strictWouldFail =
            (pcx_validate_header_strict(hdr, width, height) == PCX_OK) ? 0 : 1;
        return PCX_OK;
    }

    if (!pcx_header_get_total_bpp(hdr, &totalBits))
    {
        return PCX_ERR_OVERFLOW;
    }

    if (totalBits > 8)
    {
        outDiag->warningMask |= PCX_WARN_STRICT_LAYOUT_UNSUPPORTED;
    }

    outDiag->strictWouldFail =
        (pcx_validate_header_strict(hdr, width, height) == PCX_OK) ? 0 : 1;

    return PCX_OK;
}

int pcx_parse_header_ex(FILE *f,
                        PCXHeader *hdr,
                        int *outWidth,
                        int *outHeight,
                        unsigned flags)
{
    pcx_u8 raw[128];
    pcx_size got;
    int result;
    int width;
    int height;

    if (f == NULL || hdr == NULL || outWidth == NULL || outHeight == NULL)
    {
        return PCX_ERR_NULL_POINTER;
    }

    memset(hdr, 0, sizeof(*hdr));

    got = fread(raw, 1, sizeof(raw), f);
    if (got != sizeof(raw))
    {
        if (feof(f))
        {
            return PCX_ERR_EOF;
        }
        return PCX_ERR_IO;
    }

    hdr->manufacturer = raw[0];
    hdr->version      = raw[1];
    hdr->encoding     = raw[2];
    hdr->bitsPerPixel = raw[3];
    hdr->xMin         = pcx_u16_from_le_bytes(raw + 4);
    hdr->yMin         = pcx_u16_from_le_bytes(raw + 6);
    hdr->xMax         = pcx_u16_from_le_bytes(raw + 8);
    hdr->yMax         = pcx_u16_from_le_bytes(raw + 10);
    hdr->hDPI         = pcx_u16_from_le_bytes(raw + 12);
    hdr->vDPI         = pcx_u16_from_le_bytes(raw + 14);
    memcpy(hdr->colorMap, raw + 16, 48);
    hdr->reserved     = raw[64];
    hdr->nPlanes      = raw[65];
    hdr->bytesPerLine = pcx_u16_from_le_bytes(raw + 66);
    hdr->paletteInfo  = pcx_u16_from_le_bytes(raw + 68);
    hdr->hScreenSize  = pcx_u16_from_le_bytes(raw + 70);
    hdr->vScreenSize  = pcx_u16_from_le_bytes(raw + 72);
    memcpy(hdr->filler, raw + 74, 54);

    result = pcx_validate_header_common(hdr, &width, &height);
    if (result != PCX_OK)
    {
        return result;
    }

    if ((flags & (unsigned)PCX_PARSE_FLAG_STRICT) != 0U)
    {
        result = pcx_validate_header_strict(hdr, width, height);
        if (result != PCX_OK)
        {
            return result;
        }
    }

    *outWidth = width;
    *outHeight = height;
    return PCX_OK;
}

int pcx_parse_header(FILE *f, PCXHeader *hdr, int *outWidth, int *outHeight)
{
    return pcx_parse_header_ex(f, hdr, outWidth, outHeight, PCX_PARSE_FLAG_NONE);
}
