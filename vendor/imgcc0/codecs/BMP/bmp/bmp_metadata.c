#include "bmp_metadata.h"
#define BMP_MATH32_ENABLE_ADD 1
#define BMP_MATH32_ENABLE_MUL 1
#include "bmp_math32.h"

#include <string.h>

static bmp_limits g_bmp_global_limits = {
    BMP_DEFAULT_MAX_INPUT_BYTES,
    BMP_DEFAULT_MAX_WIDTH,
    BMP_DEFAULT_MAX_HEIGHT,
    BMP_DEFAULT_MAX_PIXELS,
    BMP_DEFAULT_MAX_DECODED_BYTES,
    BMP_DEFAULT_MAX_PALETTE_ENTRIES,
    BMP_DEFAULT_MAX_EMBEDDED_PAYLOAD_BYTES,
    BMP_DEFAULT_MAX_ICC_PROFILE_BYTES
};

void bmp_metadata_default(bmp_metadata *m)
{
    if (!m) return;
    memset(m, 0, sizeof(*m));
    m->dib_type = BMP_DIB_NONE;
}

void bmp_limits_default(bmp_limits *limits)
{
    if (!limits) return;
    limits->max_input_bytes = BMP_DEFAULT_MAX_INPUT_BYTES;
    limits->max_width = BMP_DEFAULT_MAX_WIDTH;
    limits->max_height = BMP_DEFAULT_MAX_HEIGHT;
    limits->max_pixels = BMP_DEFAULT_MAX_PIXELS;
    limits->max_decoded_bytes = BMP_DEFAULT_MAX_DECODED_BYTES;
    limits->max_palette_entries = BMP_DEFAULT_MAX_PALETTE_ENTRIES;
    limits->max_embedded_payload_bytes = BMP_DEFAULT_MAX_EMBEDDED_PAYLOAD_BYTES;
    limits->max_icc_profile_bytes = BMP_DEFAULT_MAX_ICC_PROFILE_BYTES;
}

void bmp_set_global_limits(const bmp_limits *limits)
{
    if (!limits) {
        bmp_limits_default(&g_bmp_global_limits);
        return;
    }
    g_bmp_global_limits = *limits;
}

void bmp_get_global_limits(bmp_limits *out_limits)
{
    if (!out_limits) return;
    *out_limits = g_bmp_global_limits;
}

int bmp_calc_row_stride(bmp_u32 width, bmp_u16 bpp, bmp_u32 *out_stride)
{
    bmp_u32 whole_bytes;
    bmp_u32 rem_bits;
    bmp_u32 bytes;
    bmp_u32 padded;
    bmp_u32 extra;

    if (!out_stride || width == 0U || bpp == 0U) return BMP_ERR_ARGUMENT;

    /* width*bpp/8 without forming a potentially overflowing product. */
    whole_bytes = width / 8U;
    rem_bits = width % 8U;
    if (!bmp_u32_mul_checked(whole_bytes, (bmp_u32)bpp, &bytes)) return BMP_ERR_OVERFLOW;
    if (!bmp_u32_mul_checked(rem_bits, (bmp_u32)bpp, &extra)) return BMP_ERR_OVERFLOW;
    extra = (extra + 7U) / 8U;
    if (!bmp_u32_add_checked(bytes, extra, &bytes)) return BMP_ERR_OVERFLOW;
    if (!bmp_u32_add_checked(bytes, 3U, &padded)) return BMP_ERR_OVERFLOW;
    padded &= ~3U;

    *out_stride = padded;
    return BMP_OK;
}

int bmp_calc_image_size(bmp_u32 width, bmp_u32 height, bmp_u16 bpp, bmp_u32 *out_size)
{
    bmp_u32 stride;
    int rc;

    if (!out_size || height == 0U) return BMP_ERR_ARGUMENT;
    rc = bmp_calc_row_stride(width, bpp, &stride);
    if (rc != BMP_OK) return rc;
    if (!bmp_u32_mul_checked(stride, height, out_size)) return BMP_ERR_OVERFLOW;
    return BMP_OK;
}

const char *bmp_error_string(int code)
{
    switch (code) {
    case BMP_OK:                   return "BMP_OK";
    case BMP_ERR_STREAM:           return "BMP_ERR_STREAM";
    case BMP_ERR_FORMAT:           return "BMP_ERR_FORMAT";
    case BMP_ERR_UNSUPPORTED:      return "BMP_ERR_UNSUPPORTED";
    case BMP_ERR_DIMENSIONS:       return "BMP_ERR_DIMENSIONS";
    case BMP_ERR_ARGUMENT:         return "BMP_ERR_ARGUMENT";
    case BMP_ERR_OVERFLOW:         return "BMP_ERR_OVERFLOW";
    case BMP_ERR_PALETTE:          return "BMP_ERR_PALETTE";
    case BMP_ERR_NOMEM:            return "BMP_ERR_NOMEM";
    case BMP_ERR_BUFFER_TOO_SMALL: return "BMP_ERR_BUFFER_TOO_SMALL";
    case BMP_ERR_MASKS:            return "BMP_ERR_MASKS";
    case BMP_ERR_LIMITS:           return "BMP_ERR_LIMITS";
    default:                       return "BMP_ERR_UNKNOWN";
    }
}

const char *bmp_error_description(int code)
{
    switch (code) {
    case BMP_OK:                   return "success";
    case BMP_ERR_STREAM:           return "the BMP stream ended early or is internally inconsistent";
    case BMP_ERR_FORMAT:           return "the BMP stream is malformed";
    case BMP_ERR_UNSUPPORTED:      return "the BMP variant is valid but not supported by this build";
    case BMP_ERR_DIMENSIONS:       return "the BMP dimensions are invalid";
    case BMP_ERR_ARGUMENT:         return "a required BMP argument was invalid";
    case BMP_ERR_OVERFLOW:         return "an integer or size computation overflowed";
    case BMP_ERR_PALETTE:          return "the BMP palette metadata is invalid";
    case BMP_ERR_NOMEM:            return "caller workspace is unavailable";
    case BMP_ERR_BUFFER_TOO_SMALL: return "the caller-provided output buffer is too small";
    case BMP_ERR_MASKS:            return "the BMP bitfield masks are invalid";
    case BMP_ERR_LIMITS:           return "the BMP object exceeds the configured security limits";
    default:                       return "unknown BMP error";
    }
}
