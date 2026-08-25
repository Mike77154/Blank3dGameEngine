/* pcx_chunk.c - utilidades básicas y helpers compartidos */

#include "pcx_chunk.h"
#include <string.h>
#include <limits.h>
#include "pcx_config89.h"

/* -------------------------------------------------------------------------
 * E/S básica
 * ------------------------------------------------------------------------- */

int pcx_read_u8(FILE *f, pcx_u8 *value)
{
    int c;

    if (f == NULL || value == NULL)
    {
        return 0;
    }

    c = fgetc(f);
    if (c == EOF)
    {
        return 0;
    }

    *value = (pcx_u8)c;
    return 1;
}

int pcx_read_u16_le(FILE *f, pcx_u16 *value)
{
    pcx_u8 lo;
    pcx_u8 hi;

    if (f == NULL || value == NULL)
    {
        return 0;
    }

    if (!pcx_read_u8(f, &lo) || !pcx_read_u8(f, &hi))
    {
        return 0;
    }

    *value = (pcx_u16)(lo | ((pcx_u16)hi << 8));
    return 1;
}

int pcx_read_bytes(FILE *f, void *buffer, pcx_size count)
{
    if (f == NULL || buffer == NULL)
    {
        return 0;
    }

    if (count == 0)
    {
        return 1;
    }

    if (fread(buffer, 1, count, f) != count)
    {
        return 0;
    }

    return 1;
}

int pcx_skip_bytes(FILE *f, pcx_off count)
{
    if (f == NULL || count < 0)
    {
        return 0;
    }

    return (fseek(f, count, SEEK_CUR) == 0) ? 1 : 0;
}

/* -------------------------------------------------------------------------
 * Imagen / paleta: almacenamiento estático, sin asignación dinámica.
 * ------------------------------------------------------------------------- */

static pcx_u8 g_pcx_image_store[PCX89_STATIC_IMAGE_BYTES];
static pcx_u8 g_pcx_index_store[PCX89_STATIC_INDEX_BYTES];

void pcx_image_init(PCXImage *img)
{
    if (img == NULL)
    {
        return;
    }
    img->width = 0;
    img->height = 0;
    img->channels = 0;
    img->pixels = NULL;
}

void pcx_image_release(PCXImage *img)
{
    pcx_image_init(img);
}

PCXResult pcx_image_use_static(PCXImage *img, int width, int height, int channels)
{
    pcx_size size;
    if (img == NULL)
    {
        return PCX_ERR_NULL_POINTER;
    }
    if (width <= 0 || height <= 0 || channels <= 0)
    {
        pcx_image_release(img);
        return PCX_ERR_DIMENSIONS;
    }
    if (!pcx_image_calc_buffer_size(width, height, channels, &size))
    {
        pcx_image_release(img);
        return PCX_ERR_OVERFLOW;
    }
    if (size > PCX89_STATIC_IMAGE_BYTES)
    {
        pcx_image_release(img);
        return PCX_ERR_LIMITS;
    }
    img->width = width;
    img->height = height;
    img->channels = channels;
    img->pixels = g_pcx_image_store;
    return PCX_OK;
}

PCXResult pcx_image_use_buffer(PCXImage *img,
                               int width,
                               int height,
                               int channels,
                               pcx_u8 *pixels)
{
    pcx_size size;
    if (img == NULL || pixels == NULL)
    {
        return PCX_ERR_NULL_POINTER;
    }
    if (width <= 0 || height <= 0 || channels <= 0)
    {
        return PCX_ERR_DIMENSIONS;
    }
    if (!pcx_image_calc_buffer_size(width, height, channels, &size))
    {
        return PCX_ERR_OVERFLOW;
    }
    (void)size;
    img->width = width;
    img->height = height;
    img->channels = channels;
    img->pixels = pixels;
    return PCX_OK;
}

void pcx_indexed_image_init(PCXIndexedImage *img)
{
    if (img == NULL)
    {
        return;
    }
    img->width = 0;
    img->height = 0;
    img->totalBitsPerPixel = 0;
    img->indices = NULL;
    pcx_palette_init(&img->palette);
}

void pcx_indexed_image_release(PCXIndexedImage *img)
{
    pcx_indexed_image_init(img);
}

PCXResult pcx_indexed_image_use_static(PCXIndexedImage *img,
                                       int width,
                                       int height,
                                       int totalBitsPerPixel)
{
    pcx_size size;
    if (img == NULL)
    {
        return PCX_ERR_NULL_POINTER;
    }
    if (width <= 0 || height <= 0 || totalBitsPerPixel <= 0 || totalBitsPerPixel > 8)
    {
        pcx_indexed_image_release(img);
        return PCX_ERR_DIMENSIONS;
    }
    if (!pcx_image_calc_buffer_size(width, height, 1, &size))
    {
        pcx_indexed_image_release(img);
        return PCX_ERR_OVERFLOW;
    }
    if (size > PCX89_STATIC_INDEX_BYTES)
    {
        pcx_indexed_image_release(img);
        return PCX_ERR_LIMITS;
    }
    img->width = width;
    img->height = height;
    img->totalBitsPerPixel = totalBitsPerPixel;
    img->indices = g_pcx_index_store;
    return PCX_OK;
}

PCXResult pcx_indexed_image_use_buffer(PCXIndexedImage *img,
                                       int width,
                                       int height,
                                       int totalBitsPerPixel,
                                       pcx_u8 *indices)
{
    pcx_size size;
    if (img == NULL || indices == NULL)
    {
        return PCX_ERR_NULL_POINTER;
    }
    if (width <= 0 || height <= 0 || totalBitsPerPixel <= 0 || totalBitsPerPixel > 8)
    {
        return PCX_ERR_DIMENSIONS;
    }
    if (!pcx_image_calc_buffer_size(width, height, 1, &size))
    {
        return PCX_ERR_OVERFLOW;
    }
    (void)size;
    img->width = width;
    img->height = height;
    img->totalBitsPerPixel = totalBitsPerPixel;
    img->indices = indices;
    return PCX_OK;
}

void pcx_palette_init(PCXPalette *pal)
{
    if (pal == NULL)
    {
        return;
    }

    memset(pal->colors, 0, sizeof(pal->colors));
    pal->isValid = 0;
}

PCXBool pcx_palette_is_valid(const PCXPalette *pal)
{
    if (pal == NULL)
    {
        return PCX_FALSE;
    }

    return (pal->isValid != 0) ? PCX_TRUE : PCX_FALSE;
}

/* -------------------------------------------------------------------------
 * Aritmética segura
 * ------------------------------------------------------------------------- */

PCXBool pcx_mul_int_safe(int a, int b, int *out)
{

    if (out == NULL)
    {
        return PCX_FALSE;
    }

    if (a < 0 || b < 0)
    {
        return PCX_FALSE;
    }

    if (a != 0 && b > INT_MAX / a)
    {
        return PCX_FALSE;
    }

    *out = a * b;
    return PCX_TRUE;
}

PCXBool pcx_add_int_safe(int a, int b, int *out)
{

    if (out == NULL)
    {
        return PCX_FALSE;
    }

    if ((b > 0 && a > INT_MAX - b) ||
        (b < 0 && a < INT_MIN - b))
    {
        return PCX_FALSE;
    }

    *out = a + b;
    return PCX_TRUE;
}

PCXBool pcx_mul_pcx_size_safe(pcx_size a, pcx_size b, pcx_size *out)
{
    pcx_size maxv;

    if (out == NULL)
    {
        return PCX_FALSE;
    }

    maxv = (pcx_size)~(pcx_size)0;

    if (a != 0 && b > maxv / a)
    {
        return PCX_FALSE;
    }

    *out = a * b;
    return PCX_TRUE;
}

PCXBool pcx_add_pcx_size_safe(pcx_size a, pcx_size b, pcx_size *out)
{
    pcx_size maxv;

    if (out == NULL)
    {
        return PCX_FALSE;
    }

    maxv = (pcx_size)~(pcx_size)0;
    if (b > maxv - a)
    {
        return PCX_FALSE;
    }

    *out = a + b;
    return PCX_TRUE;
}

PCXBool pcx_image_calc_buffer_size(int width,
                                   int height,
                                   int channels,
                                   pcx_size *outSize)
{
    pcx_size size;
    pcx_size tmp;

    if (outSize == NULL)
    {
        return PCX_FALSE;
    }

    if (width <= 0 || height <= 0 || channels <= 0)
    {
        return PCX_FALSE;
    }

    if (!pcx_mul_pcx_size_safe((pcx_size)width, (pcx_size)height, &tmp))
    {
        return PCX_FALSE;
    }

    if (!pcx_mul_pcx_size_safe(tmp, (pcx_size)channels, &size))
    {
        return PCX_FALSE;
    }

    *outSize = size;
    return PCX_TRUE;
}

/* -------------------------------------------------------------------------
 * Helpers de cabecera
 * ------------------------------------------------------------------------- */

PCXBool pcx_header_get_dimensions(const PCXHeader *hdr,
                                  int *outWidth,
                                  int *outHeight)
{
    int width;
    int height;

    if (hdr == NULL || outWidth == NULL || outHeight == NULL)
    {
        return PCX_FALSE;
    }

    if (hdr->xMax < hdr->xMin || hdr->yMax < hdr->yMin)
    {
        return PCX_FALSE;
    }

    width = (int)(hdr->xMax - hdr->xMin + 1);
    height = (int)(hdr->yMax - hdr->yMin + 1);

    if (width <= 0 || height <= 0)
    {
        return PCX_FALSE;
    }

    *outWidth = width;
    *outHeight = height;
    return PCX_TRUE;
}

PCXBool pcx_header_get_total_bpp(const PCXHeader *hdr, int *outTotalBits)
{
    int total;

    if (hdr == NULL || outTotalBits == NULL)
    {
        return PCX_FALSE;
    }

    if (!pcx_mul_int_safe((int)hdr->bitsPerPixel, (int)hdr->nPlanes, &total))
    {
        return PCX_FALSE;
    }

    if (total <= 0)
    {
        return PCX_FALSE;
    }

    *outTotalBits = total;
    return PCX_TRUE;
}

PCXBool pcx_header_get_min_bytes_per_line(const PCXHeader *hdr,
                                          int width,
                                          int *outMinBytesPerLine)
{
    int bits;
    int bitsPlus7;

    if (hdr == NULL || outMinBytesPerLine == NULL)
    {
        return PCX_FALSE;
    }

    if (width <= 0)
    {
        return PCX_FALSE;
    }

    if (!pcx_mul_int_safe(width, (int)hdr->bitsPerPixel, &bits))
    {
        return PCX_FALSE;
    }

    if (!pcx_add_int_safe(bits, 7, &bitsPlus7))
    {
        return PCX_FALSE;
    }

    *outMinBytesPerLine = bitsPlus7 / 8;
    return PCX_TRUE;
}

PCXBool pcx_header_is_basic_valid(const PCXHeader *hdr)
{
    int width;
    int height;
    int minBytes;

    if (hdr == NULL)
    {
        return PCX_FALSE;
    }

    if (hdr->manufacturer != 0x0A)
    {
        return PCX_FALSE;
    }

    if (hdr->encoding != 0 && hdr->encoding != 1)
    {
        return PCX_FALSE;
    }

    if (!(hdr->bitsPerPixel == 1 || hdr->bitsPerPixel == 2 ||
          hdr->bitsPerPixel == 4 || hdr->bitsPerPixel == 8))
    {
        return PCX_FALSE;
    }

    if (hdr->nPlanes < 1 || hdr->nPlanes > 4)
    {
        return PCX_FALSE;
    }

    if (!pcx_header_get_dimensions(hdr, &width, &height))
    {
        return PCX_FALSE;
    }

    if (!pcx_header_get_min_bytes_per_line(hdr, width, &minBytes))
    {
        return PCX_FALSE;
    }

    if ((int)hdr->bytesPerLine < minBytes)
    {
        return PCX_FALSE;
    }

    return PCX_TRUE;
}

PCXBool pcx_header_is_indexed8(const PCXHeader *hdr)
{
    if (hdr == NULL)
    {
        return PCX_FALSE;
    }

    return (hdr->bitsPerPixel == 8 && hdr->nPlanes == 1) ? PCX_TRUE : PCX_FALSE;
}

PCXBool pcx_header_is_indexed4(const PCXHeader *hdr)
{
    if (hdr == NULL)
    {
        return PCX_FALSE;
    }

    return (hdr->bitsPerPixel == 4 && hdr->nPlanes == 1) ? PCX_TRUE : PCX_FALSE;
}

PCXBool pcx_header_is_truecolor24(const PCXHeader *hdr)
{
    if (hdr == NULL)
    {
        return PCX_FALSE;
    }

    return (hdr->bitsPerPixel == 8 && hdr->nPlanes == 3) ? PCX_TRUE : PCX_FALSE;
}

PCXBool pcx_header_uses_extended_palette(const PCXHeader *hdr)
{
    if (hdr == NULL)
    {
        return PCX_FALSE;
    }

    return pcx_header_is_indexed8(hdr);
}

PCXBool pcx_header_is_grayscale_hint(const PCXHeader *hdr)
{
    if (hdr == NULL)
    {
        return PCX_FALSE;
    }

    return (hdr->paletteInfo == 2) ? PCX_TRUE : PCX_FALSE;
}

/* -------------------------------------------------------------------------
 * Diagnóstico
 * ------------------------------------------------------------------------- */

void pcx_diagnostics_init(PCXDiagnostics *diag)
{
    if (diag == NULL)
    {
        return;
    }

    diag->warningMask = 0U;
    diag->strictWouldFail = 0;
    diag->paletteSource = PCX_PALETTE_SOURCE_UNKNOWN;
}

int pcx_warning_mask_has(pcx_u32 mask, pcx_u32 flag)
{
    return ((mask & flag) != 0U) ? 1 : 0;
}

const char *pcx_warning_flag_to_string(pcx_u32 flag)
{
    switch (flag)
    {
    case PCX_WARN_UNKNOWN_VERSION:
        return "PCX_WARN_UNKNOWN_VERSION";
    case PCX_WARN_RAW_ENCODING:
        return "PCX_WARN_RAW_ENCODING";
    case PCX_WARN_RESERVED_NONZERO:
        return "PCX_WARN_RESERVED_NONZERO";
    case PCX_WARN_ODD_BYTES_PER_LINE:
        return "PCX_WARN_ODD_BYTES_PER_LINE";
    case PCX_WARN_STRICT_LAYOUT_UNSUPPORTED:
        return "PCX_WARN_STRICT_LAYOUT_UNSUPPORTED";
    case PCX_WARN_EXTENDED_PALETTE_MISSING:
        return "PCX_WARN_EXTENDED_PALETTE_MISSING";
    case PCX_WARN_GRAYSCALE_FALLBACK:
        return "PCX_WARN_GRAYSCALE_FALLBACK";
    case PCX_WARN_HEADER16_PALETTE_FALLBACK:
        return "PCX_WARN_HEADER16_PALETTE_FALLBACK";
    case PCX_WARN_SCANLINE_PADDING:
        return "PCX_WARN_SCANLINE_PADDING";
    default:
        return "PCX_WARN_UNKNOWN";
    }
}

const char *pcx_palette_source_to_string(PCXPaletteSource source)
{
    switch (source)
    {
    case PCX_PALETTE_SOURCE_UNKNOWN:
        return "PCX_PALETTE_SOURCE_UNKNOWN";
    case PCX_PALETTE_SOURCE_HEADER16:
        return "PCX_PALETTE_SOURCE_HEADER16";
    case PCX_PALETTE_SOURCE_GRAYSCALE_FALLBACK:
        return "PCX_PALETTE_SOURCE_GRAYSCALE_FALLBACK";
    case PCX_PALETTE_SOURCE_EXTENDED_VGA:
        return "PCX_PALETTE_SOURCE_EXTENDED_VGA";
    case PCX_PALETTE_SOURCE_HEADER16_FALLBACK:
        return "PCX_PALETTE_SOURCE_HEADER16_FALLBACK";
    default:
        return "PCX_PALETTE_SOURCE_UNKNOWN";
    }
}

const char *pcx_result_to_string(PCXResult result)
{
    switch (result)
    {
    case PCX_OK:
        return "PCX_OK";
    case PCX_ERR_IO:
        return "PCX_ERR_IO";
    case PCX_ERR_FORMAT:
        return "PCX_ERR_FORMAT";
    case PCX_ERR_UNSUPPORTED:
        return "PCX_ERR_UNSUPPORTED";
    case PCX_ERR_OUT_OF_MEMORY:
        return "PCX_ERR_OUT_OF_MEMORY";
    case PCX_ERR_EOF:
        return "PCX_ERR_EOF";
    case PCX_ERR_OVERFLOW:
        return "PCX_ERR_OVERFLOW";
    case PCX_ERR_INTERNAL:
        return "PCX_ERR_INTERNAL";
    case PCX_ERR_NULL_POINTER:
        return "PCX_ERR_NULL_POINTER";
    case PCX_ERR_DIMENSIONS:
        return "PCX_ERR_DIMENSIONS";
    case PCX_ERR_LIMITS:
        return "PCX_ERR_LIMITS";
    default:
        return "PCX_ERR_UNKNOWN";
    }
}

const char *pcx_result_description(PCXResult result)
{
    switch (result)
    {
    case PCX_OK:
        return "success";
    case PCX_ERR_IO:
        return "I/O failure while reading or writing PCX data";
    case PCX_ERR_FORMAT:
        return "the PCX stream is malformed or inconsistent";
    case PCX_ERR_UNSUPPORTED:
        return "the PCX layout is valid but not supported by this build";
    case PCX_ERR_OUT_OF_MEMORY:
        return "memory allocation failed";
    case PCX_ERR_EOF:
        return "unexpected end of file while decoding PCX data";
    case PCX_ERR_OVERFLOW:
        return "an integer or size computation overflowed";
    case PCX_ERR_INTERNAL:
        return "internal PCX library error";
    case PCX_ERR_NULL_POINTER:
        return "a required pointer argument was NULL";
    case PCX_ERR_DIMENSIONS:
        return "the image dimensions are invalid";
    case PCX_ERR_LIMITS:
        return "the image exceeds the configured PCX security limits";
    default:
        return "unknown PCX error";
    }
}
