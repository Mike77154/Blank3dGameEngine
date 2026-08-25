#include "bmp_parser.h"

#define BMP_MATH32_ENABLE_ADD 1
#define BMP_MATH32_ENABLE_MUL 1
#include "bmp_math32.h"

#include <string.h>

static const bmp_limits *bmp_resolve_limits(const bmp_limits *limits);
static int bmp_check_input_limits(bmp_u32 size, const bmp_limits *limits);
static int bmp_check_parsed_limits(const bmp_image *img, const bmp_limits *limits);

void bmp_diagnostics_default(bmp_diagnostics *diag)
{
    if (!diag) return;
    memset(diag, 0, sizeof(*diag));
    diag->payload_signature_ok = -1;
}

int bmp_warning_mask_has(bmp_u32 mask, bmp_u32 flag)
{
    return ((mask & flag) != 0U) ? 1 : 0;
}


static const bmp_limits *bmp_resolve_limits(const bmp_limits *limits)
{
    static bmp_limits snapshot;

    if (limits) return limits;
    bmp_get_global_limits(&snapshot);
    return &snapshot;
}


static int bmp_check_input_limits(bmp_u32 size, const bmp_limits *limits)
{
    const bmp_limits *effective = bmp_resolve_limits(limits);
    if (effective->max_input_bytes != 0U && size > effective->max_input_bytes) {
        return BMP_ERR_LIMITS;
    }
    return BMP_OK;
}

static int bmp_check_parsed_limits(const bmp_image *img, const bmp_limits *limits)
{
    const bmp_limits *effective = bmp_resolve_limits(limits);
    const bmp_metadata *m;
    bmp_u32 width;
    bmp_u32 height;

    if (!img) return BMP_ERR_ARGUMENT;

    m = &img->meta;
    width = (bmp_u32)m->width;
    height = (bmp_u32)m->height;

    if (effective->max_width != 0U && width > effective->max_width) {
        return BMP_ERR_LIMITS;
    }
    if (effective->max_height != 0U && height > effective->max_height) {
        return BMP_ERR_LIMITS;
    }

    if (effective->max_pixels != 0U &&
        height != 0U && width > effective->max_pixels / height) {
        return BMP_ERR_LIMITS;
    }

    if (effective->max_palette_entries != 0U &&
        m->palette_entries > effective->max_palette_entries) {
        return BMP_ERR_LIMITS;
    }

    if (bmp_image_has_embedded_payload(img) &&
        effective->max_embedded_payload_bytes != 0U &&
        img->pixel_data_size > effective->max_embedded_payload_bytes) {
        return BMP_ERR_LIMITS;
    }

    if (effective->max_icc_profile_bytes != 0U &&
        m->profile_size > effective->max_icc_profile_bytes) {
        return BMP_ERR_LIMITS;
    }

    return BMP_OK;
}

const char *bmp_warning_string(bmp_u32 flag)
{
    switch (flag) {
    case BMP_WARN_FILE_SIZE_HEADER_ZERO:
        return "BMP_WARN_FILE_SIZE_HEADER_ZERO";
    case BMP_WARN_FILE_SIZE_HEADER_SMALLER_ACTUAL:
        return "BMP_WARN_FILE_SIZE_HEADER_SMALLER_ACTUAL";
    case BMP_WARN_IMAGE_SIZE_ZERO_COMPRESSED:
        return "BMP_WARN_IMAGE_SIZE_ZERO_COMPRESSED";
    case BMP_WARN_TRUECOLOR_COLORS_USED_NONZERO:
        return "BMP_WARN_TRUECOLOR_COLORS_USED_NONZERO";
    case BMP_WARN_COLORS_IMPORTANT_EXCEEDS_PALETTE:
        return "BMP_WARN_COLORS_IMPORTANT_EXCEEDS_PALETTE";
    case BMP_WARN_PROFILE_SIZE_WITHOUT_PROFILE_CST:
        return "BMP_WARN_PROFILE_SIZE_WITHOUT_PROFILE_CST";
    case BMP_WARN_TOP_DOWN:
        return "BMP_WARN_TOP_DOWN";
    case BMP_WARN_EMBEDDED_PAYLOAD:
        return "BMP_WARN_EMBEDDED_PAYLOAD";
    default:
        return "BMP_WARN_UNKNOWN";
    }
}

static void bmp_detect_dib_type(bmp_metadata *meta)
{
    bmp_u32 sz = meta->dib_header_size;
    if (sz == 12U)       meta->dib_type = BMP_DIB_CORE;
    else if (sz == 40U)  meta->dib_type = BMP_DIB_INFO;
    else if (sz == 52U)  meta->dib_type = BMP_DIB_V2;
    else if (sz == 56U)  meta->dib_type = BMP_DIB_V3;
    else if (sz == 64U)  meta->dib_type = BMP_DIB_OS2V2;
    else if (sz == 108U) meta->dib_type = BMP_DIB_V4;
    else if (sz == 124U) meta->dib_type = BMP_DIB_V5;
    else                 meta->dib_type = BMP_DIB_NONE;
}

static bmp_u32 bmp_effective_file_size(const bmp_metadata *m, bmp_u32 actual_size)
{
    if (m->file.file_size != 0U && m->file.file_size <= actual_size) {
        return m->file.file_size;
    }
    return actual_size;
}

static int bmp_mask_is_contiguous(bmp_u32 mask)
{
    bmp_u32 m = mask;
    if (m == 0U) return 1;
    while ((m & 1U) == 0U) m >>= 1;
    while ((m & 1U) == 1U) m >>= 1;
    return (m == 0U);
}

static int bmp_masks_valid(bmp_u32 red,
                           bmp_u32 green,
                           bmp_u32 blue,
                           bmp_u32 alpha,
                           bmp_u16 bpp,
                           int require_alpha)
{
    bmp_u32 limit;

    if (bpp == 32U) {
        limit = 0xFFFFFFFFU;
    } else if (bpp < 32U) {
        limit = (1U << bpp) - 1U;
    } else {
        return BMP_ERR_MASKS;
    }

    if ((red | green | blue) == 0U) return BMP_ERR_MASKS;
    if (require_alpha && alpha == 0U) return BMP_ERR_MASKS;

    if ((red & ~limit) != 0U ||
        (green & ~limit) != 0U ||
        (blue & ~limit) != 0U ||
        (alpha & ~limit) != 0U) {
        return BMP_ERR_MASKS;
    }

    if ((red & green) != 0U ||
        (red & blue) != 0U ||
        (red & alpha) != 0U ||
        (green & blue) != 0U ||
        (green & alpha) != 0U ||
        (blue & alpha) != 0U) {
        return BMP_ERR_MASKS;
    }

    if (!bmp_mask_is_contiguous(red) ||
        !bmp_mask_is_contiguous(green) ||
        !bmp_mask_is_contiguous(blue) ||
        (alpha != 0U && !bmp_mask_is_contiguous(alpha))) {
        return BMP_ERR_MASKS;
    }

    return BMP_OK;
}

static int bmp_validate_file_header(const bmp_metadata *m)
{
    if (m->file.magic != 0x4D42U) return BMP_ERR_FORMAT;
    if (m->file.reserved1 != 0U || m->file.reserved2 != 0U) return BMP_ERR_FORMAT;
    return BMP_OK;
}

static int bmp_validate_dimensions_and_orientation(bmp_metadata *m)
{
    if (m->height < 0) {
        if (m->compression != BMP_COMP_RGB &&
            m->compression != BMP_COMP_BITFIELDS) {
            return BMP_ERR_FORMAT;
        }
        if (m->height == (-2147483647 - 1)) {
            return BMP_ERR_DIMENSIONS;
        }
        m->is_top_down = 1;
        m->height = -m->height;
    } else {
        m->is_top_down = 0;
    }

    if (m->width <= 0 || m->height <= 0) {
        return BMP_ERR_DIMENSIONS;
    }

    return BMP_OK;
}

static int bmp_validate_core_fields(const bmp_metadata *m)
{
    if (m->planes != 1U) return BMP_ERR_FORMAT;
    switch (m->bpp) {
    case 1:
    case 4:
    case 8:
    case 24:
        return BMP_OK;
    default:
        return BMP_ERR_UNSUPPORTED;
    }
}

static int bmp_validate_info_fields(const bmp_metadata *m)
{
    if (m->planes != 1U) return BMP_ERR_FORMAT;

    switch (m->compression) {
    case BMP_COMP_RGB:
        switch (m->bpp) {
        case 1:
        case 2:
        case 4:
        case 8:
        case 16:
        case 24:
        case 32:
            return BMP_OK;
        default:
            return BMP_ERR_UNSUPPORTED;
        }
    case BMP_COMP_RLE8:
        return (m->bpp == 8U) ? BMP_OK : BMP_ERR_UNSUPPORTED;
    case BMP_COMP_RLE4:
        return (m->bpp == 4U) ? BMP_OK : BMP_ERR_UNSUPPORTED;
    case BMP_COMP_BITFIELDS:
    case BMP_COMP_ALPHABITFIELDS:
        return (m->bpp == 16U || m->bpp == 32U) ? BMP_OK : BMP_ERR_UNSUPPORTED;
    case BMP_COMP_JPEG:
    case BMP_COMP_PNG:
        switch (m->bpp) {
        case 0:
        case 1:
        case 2:
        case 4:
        case 8:
        case 16:
        case 24:
        case 32:
            return BMP_OK;
        default:
            return BMP_ERR_UNSUPPORTED;
        }
    default:
        return BMP_ERR_UNSUPPORTED;
    }
}

static int bmp_validate_colors_used(const bmp_metadata *m)
{
    bmp_u32 max_colors;

    if (m->compression == BMP_COMP_JPEG ||
        m->compression == BMP_COMP_PNG) {
        return BMP_OK;
    }

    if (m->bpp > 8U) {
        return BMP_OK;
    }

    max_colors = (bmp_u32)(1U << m->bpp);
    if (m->colors_used > max_colors) {
        return BMP_ERR_PALETTE;
    }

    return BMP_OK;
}

static int bmp_validate_masks_for_metadata(const bmp_metadata *m)
{
    if (m->compression == BMP_COMP_BITFIELDS) {
        return bmp_masks_valid(m->red_mask,
                               m->green_mask,
                               m->blue_mask,
                               m->alpha_mask,
                               m->bpp,
                               0);
    }

    if (m->compression == BMP_COMP_ALPHABITFIELDS) {
        return bmp_masks_valid(m->red_mask,
                               m->green_mask,
                               m->blue_mask,
                               m->alpha_mask,
                               m->bpp,
                               1);
    }

    return BMP_OK;
}

static int bmp_palette_entries_default(const bmp_metadata *m, bmp_u32 *out_entries)
{
    bmp_u32 max_colors = 0;

    if (!out_entries) return BMP_ERR_ARGUMENT;

    if (m->compression == BMP_COMP_JPEG ||
        m->compression == BMP_COMP_PNG) {
        *out_entries = 0U;
        return BMP_OK;
    }

    if (m->bpp <= 8U) {
        max_colors = (bmp_u32)(1U << m->bpp);
        if (m->colors_used != 0U && m->colors_used < max_colors) {
            max_colors = m->colors_used;
        }
        *out_entries = max_colors;
    } else {
        *out_entries = 0U;
    }

    return BMP_OK;
}


static int bmp_validate_embedded_payload_signature_raw(bmp_u32 compression,
                                                       const bmp_u8 *data,
                                                       bmp_u32 size)
{
    static const bmp_u8 png_sig[8] = { 0x89U, 'P', 'N', 'G', 0x0DU, 0x0AU, 0x1AU, 0x0AU };

    if (!data) return BMP_ERR_ARGUMENT;

    switch (compression) {
    case BMP_COMP_PNG:
        if (size < 8U) return BMP_ERR_STREAM;
        return (memcmp(data, png_sig, 8U) == 0) ? BMP_OK : BMP_ERR_FORMAT;
    case BMP_COMP_JPEG:
        if (size < 3U) return BMP_ERR_STREAM;
        return (data[0] == 0xFFU && data[1] == 0xD8U && data[2] == 0xFFU) ? BMP_OK : BMP_ERR_FORMAT;
    default:
        return BMP_ERR_UNSUPPORTED;
    }
}

static int bmp_validate_embedded_payload_view(const bmp_image *img)
{
    if (!img) return BMP_ERR_ARGUMENT;
    if (img->meta.compression != BMP_COMP_JPEG &&
        img->meta.compression != BMP_COMP_PNG) return BMP_ERR_UNSUPPORTED;
    if (!img->pixel_data || img->pixel_data_size == 0U) return BMP_ERR_STREAM;
    return bmp_validate_embedded_payload_signature_raw(img->meta.compression,
                                                       img->pixel_data,
                                                       img->pixel_data_size);
}

int bmp_parse_memory_with_limits(const bmp_u8 *data,
                                 bmp_u32 size,
                                 const bmp_limits *limits,
                                 bmp_image *out_img)
{
    bmp_stream s;
    bmp_metadata *m;
    bmp_u32 effective_file_size;
    bmp_u32 palette_entries;
    bmp_u32 palette_bytes = 0;
    bmp_u32 masks_after_infoheader = 0;
    bmp_u32 expected_pixel_data_size = 0;
    bmp_u32 minimum_pixel_offset;
    bmp_u32 abs_profile_offset;
    bmp_u32 abs_profile_end;
    bmp_u32 temp_offset;
    int rc;

    if (!data || !out_img) return BMP_ERR_ARGUMENT;
    if (size < 26U) return BMP_ERR_STREAM;

    rc = bmp_check_input_limits(size, limits);
    if (rc != BMP_OK) return rc;

    memset(out_img, 0, sizeof(*out_img));
    bmp_diagnostics_default(&out_img->diagnostics);
    bmp_stream_init(&s, data, size);

    m = &out_img->meta;
    bmp_metadata_default(m);

    m->file.magic        = bmp_read_u16(&s);
    m->file.file_size    = bmp_read_u32(&s);
    m->file.reserved1    = bmp_read_u16(&s);
    m->file.reserved2    = bmp_read_u16(&s);
    m->file.pixel_offset = bmp_read_u32(&s);

    if (bmp_stream_failed(&s)) return BMP_ERR_STREAM;

    rc = bmp_validate_file_header(m);
    if (rc != BMP_OK) return rc;

    if (m->file.file_size != 0U && m->file.file_size > size) {
        return BMP_ERR_STREAM;
    }

    if (m->file.file_size == 0U) {
        out_img->diagnostics.warning_mask |= BMP_WARN_FILE_SIZE_HEADER_ZERO;
    } else if (m->file.file_size < size) {
        out_img->diagnostics.warning_mask |= BMP_WARN_FILE_SIZE_HEADER_SMALLER_ACTUAL;
    }

    effective_file_size = bmp_effective_file_size(m, size);
    if (m->file.pixel_offset >= effective_file_size) {
        return BMP_ERR_STREAM;
    }

    m->dib_header_size = bmp_read_u32(&s);
    if (bmp_stream_failed(&s)) return BMP_ERR_STREAM;

    bmp_detect_dib_type(m);
    if (m->dib_type == BMP_DIB_NONE) {
        return BMP_ERR_UNSUPPORTED;
    }

    if (m->dib_type == BMP_DIB_CORE) {
        bmp_u16 w;
        bmp_u16 h;

        w = bmp_read_u16(&s);
        h = bmp_read_u16(&s);
        m->planes = bmp_read_u16(&s);
        m->bpp    = bmp_read_u16(&s);

        m->width = (bmp_s32)w;
        m->height = (bmp_s32)h;
        m->compression = BMP_COMP_RGB;
        m->image_size = 0U;
        m->ppm_x = 0;
        m->ppm_y = 0;
        m->colors_used = 0U;
        m->colors_important = 0U;

        if (bmp_stream_failed(&s)) return BMP_ERR_STREAM;
        rc = bmp_validate_core_fields(m);
        if (rc != BMP_OK) return rc;
    } else {
        m->width            = bmp_read_s32(&s);
        m->height           = bmp_read_s32(&s);
        m->planes           = bmp_read_u16(&s);
        m->bpp              = bmp_read_u16(&s);
        m->compression      = bmp_read_u32(&s);
        m->image_size       = bmp_read_u32(&s);
        m->ppm_x            = bmp_read_s32(&s);
        m->ppm_y            = bmp_read_s32(&s);
        m->colors_used      = bmp_read_u32(&s);
        m->colors_important = bmp_read_u32(&s);

        if (bmp_stream_failed(&s)) return BMP_ERR_STREAM;

        if (m->dib_header_size >= 52U) {
            m->red_mask   = bmp_read_u32(&s);
            m->green_mask = bmp_read_u32(&s);
            m->blue_mask  = bmp_read_u32(&s);
        }
        if (m->dib_header_size >= 56U) {
            m->alpha_mask = bmp_read_u32(&s);
        }
        if (m->dib_header_size == 64U) {
            if (bmp_stream_remaining(&s) < 8U) return BMP_ERR_STREAM;
            s.pos += 8U;
        }
        if (m->dib_header_size >= 108U) {
            bmp_u32 i;
            m->color_space_type = bmp_read_u32(&s);
            for (i = 0; i < 9U; ++i) {
                m->cie_endpoints[i] = bmp_read_s32(&s);
            }
            m->gamma_red   = bmp_read_u32(&s);
            m->gamma_green = bmp_read_u32(&s);
            m->gamma_blue  = bmp_read_u32(&s);
        }
        if (m->dib_header_size >= 124U) {
            m->rendering_intent    = bmp_read_u32(&s);
            m->profile_data_offset = bmp_read_u32(&s);
            m->profile_size        = bmp_read_u32(&s);
            m->reserved_v5         = bmp_read_u32(&s);
        }
        if (bmp_stream_failed(&s)) return BMP_ERR_STREAM;

        if (m->dib_header_size == 40U || m->dib_header_size == 64U) {
            if (m->compression == BMP_COMP_BITFIELDS) {
                masks_after_infoheader = 12U;
                if (bmp_stream_remaining(&s) < masks_after_infoheader) return BMP_ERR_STREAM;
                m->red_mask   = bmp_read_u32(&s);
                m->green_mask = bmp_read_u32(&s);
                m->blue_mask  = bmp_read_u32(&s);
            } else if (m->compression == BMP_COMP_ALPHABITFIELDS) {
                masks_after_infoheader = 16U;
                if (bmp_stream_remaining(&s) < masks_after_infoheader) return BMP_ERR_STREAM;
                m->red_mask   = bmp_read_u32(&s);
                m->green_mask = bmp_read_u32(&s);
                m->blue_mask  = bmp_read_u32(&s);
                m->alpha_mask = bmp_read_u32(&s);
            }
        }
        if (bmp_stream_failed(&s)) return BMP_ERR_STREAM;

        rc = bmp_validate_info_fields(m);
        if (rc != BMP_OK) return rc;
        rc = bmp_validate_colors_used(m);
        if (rc != BMP_OK) return rc;
        rc = bmp_validate_masks_for_metadata(m);
        if (rc != BMP_OK) return rc;

        if (m->dib_type == BMP_DIB_V5 && m->reserved_v5 != 0U) {
            return BMP_ERR_FORMAT;
        }
    }

    rc = bmp_validate_dimensions_and_orientation(m);
    if (rc != BMP_OK) return rc;
    if (m->is_top_down) {
        out_img->diagnostics.warning_mask |= BMP_WARN_TOP_DOWN;
    }

    rc = bmp_palette_entries_default(m, &palette_entries);
    if (rc != BMP_OK) return rc;

    if (m->dib_type == BMP_DIB_CORE) {
        m->palette_entry_size = (palette_entries != 0U) ? 3U : 0U;
    } else {
        m->palette_entry_size = (palette_entries != 0U) ? 4U : 0U;
    }

    if (m->palette_entry_size != 0U) {
        if (!bmp_u32_mul_checked(palette_entries, m->palette_entry_size, &palette_bytes)) {
            return BMP_ERR_OVERFLOW;
        }
    }

    if (!bmp_u32_add_checked(14U, m->dib_header_size, &minimum_pixel_offset) ||
        !bmp_u32_add_checked(minimum_pixel_offset, masks_after_infoheader, &minimum_pixel_offset) ||
        !bmp_u32_add_checked(minimum_pixel_offset, palette_bytes, &minimum_pixel_offset)) {
        return BMP_ERR_OVERFLOW;
    }
    if (m->file.pixel_offset < minimum_pixel_offset) {
        return BMP_ERR_FORMAT;
    }

    m->palette_entries = palette_entries;
    m->palette_offset = 0U;

    if (palette_bytes != 0U) {
        m->palette_offset = (bmp_u32)(14U + m->dib_header_size + masks_after_infoheader);
    }

    m->pixel_array_offset = m->file.pixel_offset;
    out_img->palette = (m->palette_offset != 0U) ? (data + m->palette_offset) : 0;
    out_img->palette_entry_size = m->palette_entry_size;
    out_img->pixel_data = data + m->pixel_array_offset;
    out_img->icc_profile_data = 0;

    if (m->bpp > 8U && m->colors_used != 0U) {
        out_img->diagnostics.warning_mask |= BMP_WARN_TRUECOLOR_COLORS_USED_NONZERO;
    }
    if (palette_entries != 0U && m->colors_important > palette_entries) {
        out_img->diagnostics.warning_mask |= BMP_WARN_COLORS_IMPORTANT_EXCEEDS_PALETTE;
    }

    switch (m->compression) {
    case BMP_COMP_RGB:
    case BMP_COMP_BITFIELDS:
    case BMP_COMP_ALPHABITFIELDS:
        rc = bmp_calc_image_size((bmp_u32)m->width,
                                 (bmp_u32)m->height,
                                 m->bpp,
                                 &expected_pixel_data_size);
        if (rc != BMP_OK) return rc;
        if (m->image_size != 0U && m->image_size < expected_pixel_data_size) {
            return BMP_ERR_STREAM;
        }
        if (!bmp_u32_add_checked(m->pixel_array_offset, expected_pixel_data_size, &temp_offset) ||
            temp_offset > effective_file_size) {
            return BMP_ERR_STREAM;
        }
        out_img->pixel_data_size = expected_pixel_data_size;
        break;
    case BMP_COMP_RLE8:
    case BMP_COMP_RLE4:
    case BMP_COMP_JPEG:
    case BMP_COMP_PNG:
        if (m->image_size != 0U) {
            if (!bmp_u32_add_checked(m->pixel_array_offset, m->image_size, &temp_offset) ||
                temp_offset > effective_file_size) {
                return BMP_ERR_STREAM;
            }
            out_img->pixel_data_size = m->image_size;
        } else {
            out_img->pixel_data_size = effective_file_size - m->pixel_array_offset;
            out_img->diagnostics.warning_mask |= BMP_WARN_IMAGE_SIZE_ZERO_COMPRESSED;
        }
        break;
    default:
        return BMP_ERR_UNSUPPORTED;
    }

    if (bmp_image_has_embedded_payload(out_img)) {
        out_img->diagnostics.warning_mask |= BMP_WARN_EMBEDDED_PAYLOAD;
        rc = bmp_validate_embedded_payload_view(out_img);
        if (rc != BMP_OK) return rc;
        out_img->diagnostics.payload_signature_ok = 1;
    }

    rc = bmp_check_parsed_limits(out_img, limits);
    if (rc != BMP_OK) return rc;

    if (m->dib_type == BMP_DIB_V5 && m->profile_size != 0U) {
        if (m->color_space_type != BMP_CSTYPE_PROFILE_EMBEDDED &&
            m->color_space_type != BMP_CSTYPE_PROFILE_LINKED) {
            out_img->diagnostics.warning_mask |= BMP_WARN_PROFILE_SIZE_WITHOUT_PROFILE_CST;
        }
        if (!bmp_u32_add_checked(14U, m->profile_data_offset, &abs_profile_offset) ||
            !bmp_u32_add_checked(abs_profile_offset, m->profile_size, &abs_profile_end)) {
            return BMP_ERR_STREAM;
        }
        if (abs_profile_offset >= effective_file_size || abs_profile_end > effective_file_size) {
            return BMP_ERR_STREAM;
        }
        if (!bmp_u32_add_checked(m->pixel_array_offset, out_img->pixel_data_size, &temp_offset)) {
            return BMP_ERR_STREAM;
        }
        if ((m->color_space_type == BMP_CSTYPE_PROFILE_EMBEDDED ||
             m->color_space_type == BMP_CSTYPE_PROFILE_LINKED) &&
            abs_profile_offset < temp_offset &&
            m->image_size != 0U) {
            return BMP_ERR_FORMAT;
        }
        if (m->color_space_type == BMP_CSTYPE_PROFILE_EMBEDDED ||
            m->color_space_type == BMP_CSTYPE_PROFILE_LINKED) {
            out_img->icc_profile_data = data + (bmp_u32)abs_profile_offset;
        }
    }

    return BMP_OK;
}


int bmp_collect_diagnostics(const bmp_image *img, bmp_diagnostics *out_diag)
{
    if (!img || !out_diag) return BMP_ERR_ARGUMENT;
    *out_diag = img->diagnostics;
    if (bmp_image_has_embedded_payload(img)) {
        out_diag->warning_mask |= BMP_WARN_EMBEDDED_PAYLOAD;
        out_diag->payload_signature_ok =
            (bmp_validate_embedded_payload_view(img) == BMP_OK) ? 1 : 0;
    }
    return BMP_OK;
}

int bmp_parse_memory(const bmp_u8 *data, bmp_u32 size, bmp_image *out_img)
{
    return bmp_parse_memory_with_limits(data, size, 0, out_img);
}

int bmp_image_has_embedded_payload(const bmp_image *img)
{
    if (!img) return 0;
    return (img->meta.compression == BMP_COMP_JPEG ||
            img->meta.compression == BMP_COMP_PNG) ? 1 : 0;
}

int bmp_validate_embedded_payload_signature(const bmp_image *img)
{
    return bmp_validate_embedded_payload_view(img);
}

int bmp_embedded_payload_signature_matches(const bmp_image *img)
{
    return (bmp_validate_embedded_payload_view(img) == BMP_OK) ? 1 : 0;
}

const char *bmp_embedded_payload_extension(const bmp_image *img)
{
    if (!img) return 0;
    switch (img->meta.compression) {
    case BMP_COMP_JPEG:
        return "jpg";
    case BMP_COMP_PNG:
        return "png";
    default:
        return 0;
    }
}

int bmp_get_embedded_payload(const bmp_image *img,
                             bmp_u32 *out_compression,
                             const bmp_u8 **out_data,
                             bmp_u32 *out_size)
{
    if (!img || !out_compression || !out_data || !out_size) return BMP_ERR_ARGUMENT;
    if (!bmp_image_has_embedded_payload(img)) return BMP_ERR_UNSUPPORTED;
    if (!img->pixel_data || img->pixel_data_size == 0U) return BMP_ERR_STREAM;

    *out_compression = img->meta.compression;
    *out_data = img->pixel_data;
    *out_size = img->pixel_data_size;
    return BMP_OK;
}

int bmp_copy_embedded_payload_into(const bmp_image *img,
                                   bmp_u32 *out_compression,
                                   bmp_u8 *out_data,
                                   bmp_u32 out_capacity,
                                   bmp_u32 *out_size)
{
    const bmp_u8 *src;
    bmp_u32 size;
    bmp_u32 compression;
    int rc;

    if (!img || !out_compression || !out_data || !out_size) return BMP_ERR_ARGUMENT;
    rc = bmp_get_embedded_payload(img, &compression, &src, &size);
    if (rc != BMP_OK) return rc;
    *out_size = size;
    if (out_capacity < size) return BMP_ERR_BUFFER_TOO_SMALL;
    memcpy(out_data, src, size);
    *out_compression = compression;
    return BMP_OK;
}
