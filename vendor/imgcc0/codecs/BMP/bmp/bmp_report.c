#include "bmp_report.h"
#include <limits.h>
#include <stdio.h>

#define BMP_REPORT_WARNING_COUNT 8

typedef struct bmp_report_buffer_s {
    char *dst;
    bmp_u32 cap;
    bmp_u32 len;
    int overflow;
} bmp_report_buffer;

static bmp_u32 bmp_strlen32(const char *s)
{
    bmp_u32 n;
    if (!s) return 0U;
    n = 0U;
    while (s[n] != '\0') {
        if (n == 0xFFFFFFFFU) return n;
        ++n;
    }
    return n;
}

static void bmp_report_buffer_init(bmp_report_buffer *b, char *dst, bmp_u32 cap)
{
    if (!b) return;
    b->dst = dst;
    b->cap = cap;
    b->len = 0U;
    b->overflow = 0;
    if (dst && cap > 0U) dst[0] = '\0';
}

static void bmp_report_buffer_append_mem(bmp_report_buffer *b,
                                         const char *src,
                                         bmp_u32 src_len)
{
    bmp_u32 copy_len;
    bmp_u32 i;
    bmp_u32 new_len;

    if (!b || !src || b->overflow) return;
    copy_len = 0U;
    if (b->dst && b->cap > 0U && b->len < b->cap - 1U) {
        copy_len = b->cap - 1U - b->len;
        if (copy_len > src_len) copy_len = src_len;
        for (i = 0U; i < copy_len; ++i) b->dst[b->len + i] = src[i];
        b->dst[b->len + copy_len] = '\0';
    }
    if (b->len > 0xFFFFFFFFU - src_len) {
        b->overflow = 1;
        return;
    }
    new_len = b->len + src_len;
    b->len = new_len;
}

static void bmp_report_buffer_append_str(bmp_report_buffer *b, const char *src)
{
    bmp_report_buffer_append_mem(b, src, bmp_strlen32(src));
}

static void bmp_report_buffer_append_char(bmp_report_buffer *b, char c)
{
    bmp_report_buffer_append_mem(b, &c, 1U);
}

static void bmp_report_buffer_append_u32(bmp_report_buffer *b, bmp_u32 value)
{
    char tmp[16];
    bmp_u32 n;
    bmp_u32 i;

    if (value == 0U) {
        bmp_report_buffer_append_char(b, '0');
        return;
    }
    n = 0U;
    while (value != 0U && n < 15U) {
        tmp[n++] = (char)('0' + (char)(value % 10U));
        value /= 10U;
    }
    for (i = 0U; i < n; ++i) {
        bmp_report_buffer_append_char(b, tmp[n - 1U - i]);
    }
}

static void bmp_report_buffer_append_int(bmp_report_buffer *b, int value)
{
    bmp_u32 mag;
    if (value < 0) {
        bmp_report_buffer_append_char(b, '-');
        mag = (bmp_u32)(-(value + 1));
        ++mag;
    } else {
        mag = (bmp_u32)value;
    }
    bmp_report_buffer_append_u32(b, mag);
}

static void bmp_report_buffer_append_bool(bmp_report_buffer *b, int value)
{
    bmp_report_buffer_append_str(b, value ? "true" : "false");
}

static void bmp_report_buffer_append_json_string(bmp_report_buffer *b,
                                                 const char *value)
{
    if (!value) value = "";
    bmp_report_buffer_append_char(b, '"');
    while (*value) {
        switch (*value) {
        case '\\': bmp_report_buffer_append_str(b, "\\\\"); break;
        case '"':  bmp_report_buffer_append_str(b, "\\\""); break;
        case '\n': bmp_report_buffer_append_str(b, "\\n"); break;
        case '\r': bmp_report_buffer_append_str(b, "\\r"); break;
        case '\t': bmp_report_buffer_append_str(b, "\\t"); break;
        default: bmp_report_buffer_append_char(b, *value); break;
        }
        ++value;
    }
    bmp_report_buffer_append_char(b, '"');
}

static int bmp_report_finish(bmp_report_buffer *b)
{
    if (!b) return BMP_ERR_ARGUMENT;
    if (b->overflow || b->len > (bmp_u32)INT_MAX) return BMP_ERR_OVERFLOW;
    if (b->dst && b->cap > 0U) {
        if (b->len < b->cap) b->dst[b->len] = '\0';
        else b->dst[b->cap - 1U] = '\0';
    }
    return (int)b->len;
}

static const bmp_u32 g_bmp_warning_flags[BMP_REPORT_WARNING_COUNT] = {
    BMP_WARN_FILE_SIZE_HEADER_ZERO,
    BMP_WARN_FILE_SIZE_HEADER_SMALLER_ACTUAL,
    BMP_WARN_IMAGE_SIZE_ZERO_COMPRESSED,
    BMP_WARN_TRUECOLOR_COLORS_USED_NONZERO,
    BMP_WARN_COLORS_IMPORTANT_EXCEEDS_PALETTE,
    BMP_WARN_PROFILE_SIZE_WITHOUT_PROFILE_CST,
    BMP_WARN_TOP_DOWN,
    BMP_WARN_EMBEDDED_PAYLOAD
};

const char *bmp_version_string(void)
{
    return BMP_VERSION_STRING;
}

bmp_u32 bmp_version_number(void)
{
    return BMP_VERSION_NUMBER;
}

const char *bmp_dib_type_string(enum bmp_dib_type type)
{
    switch (type) {
    case BMP_DIB_CORE: return "BMP_DIB_CORE";
    case BMP_DIB_INFO: return "BMP_DIB_INFO";
    case BMP_DIB_V2: return "BMP_DIB_V2";
    case BMP_DIB_V3: return "BMP_DIB_V3";
    case BMP_DIB_V4: return "BMP_DIB_V4";
    case BMP_DIB_V5: return "BMP_DIB_V5";
    case BMP_DIB_OS2V2: return "BMP_DIB_OS2V2";
    case BMP_DIB_NONE:
    default: return "BMP_DIB_NONE";
    }
}

const char *bmp_compression_string(bmp_u32 compression)
{
    switch (compression) {
    case BMP_COMP_RGB: return "BMP_COMP_RGB";
    case BMP_COMP_RLE8: return "BMP_COMP_RLE8";
    case BMP_COMP_RLE4: return "BMP_COMP_RLE4";
    case BMP_COMP_BITFIELDS: return "BMP_COMP_BITFIELDS";
    case BMP_COMP_JPEG: return "BMP_COMP_JPEG";
    case BMP_COMP_PNG: return "BMP_COMP_PNG";
    case BMP_COMP_ALPHABITFIELDS: return "BMP_COMP_ALPHABITFIELDS";
    default: return "BMP_COMP_UNKNOWN";
    }
}

int bmp_warning_count(bmp_u32 mask)
{
    int i;
    int count;
    count = 0;
    for (i = 0; i < BMP_REPORT_WARNING_COUNT; ++i) {
        if (bmp_warning_mask_has(mask, g_bmp_warning_flags[i])) ++count;
    }
    return count;
}

static void bmp_append_warning_list_text(bmp_report_buffer *b, bmp_u32 mask)
{
    int i;
    int first;
    first = 1;
    for (i = 0; i < BMP_REPORT_WARNING_COUNT; ++i) {
        bmp_u32 flag;
        flag = g_bmp_warning_flags[i];
        if (!bmp_warning_mask_has(mask, flag)) continue;
        if (!first) bmp_report_buffer_append_str(b, ", ");
        first = 0;
        bmp_report_buffer_append_str(b, bmp_warning_string(flag));
    }
    if (first) bmp_report_buffer_append_str(b, "none");
}

static void bmp_append_warning_list_json(bmp_report_buffer *b, bmp_u32 mask)
{
    int i;
    int first;
    first = 1;
    bmp_report_buffer_append_char(b, '[');
    for (i = 0; i < BMP_REPORT_WARNING_COUNT; ++i) {
        bmp_u32 flag;
        flag = g_bmp_warning_flags[i];
        if (!bmp_warning_mask_has(mask, flag)) continue;
        if (!first) bmp_report_buffer_append_char(b, ',');
        first = 0;
        bmp_report_buffer_append_json_string(b, bmp_warning_string(flag));
    }
    bmp_report_buffer_append_char(b, ']');
}

static void bmp_append_payload_signature_json(bmp_report_buffer *b, int value)
{
    if (value < 0) bmp_report_buffer_append_str(b, "null");
    else bmp_report_buffer_append_bool(b, value != 0);
}

static void bmp_append_payload_signature_text(bmp_report_buffer *b, int value)
{
    if (value < 0) bmp_report_buffer_append_str(b, "unknown");
    else bmp_report_buffer_append_str(b, value ? "true" : "false");
}

int bmp_format_diagnostics_text(const bmp_image *img,
                                char *buffer,
                                bmp_u32 buffer_size)
{
    bmp_report_buffer b;
    if (!img || (!buffer && buffer_size != 0U)) return BMP_ERR_ARGUMENT;

    bmp_report_buffer_init(&b, buffer, buffer_size);
    bmp_report_buffer_append_str(&b, "format=bmp\n");
    bmp_report_buffer_append_str(&b, "libraryVersion=");
    bmp_report_buffer_append_str(&b, bmp_version_string());
    bmp_report_buffer_append_str(&b, "\ndibType=");
    bmp_report_buffer_append_str(&b, bmp_dib_type_string(img->meta.dib_type));
    bmp_report_buffer_append_str(&b, "\nwidth=");
    bmp_report_buffer_append_int(&b, (int)img->meta.width);
    bmp_report_buffer_append_str(&b, "\nheight=");
    bmp_report_buffer_append_int(&b, (int)img->meta.height);
    bmp_report_buffer_append_str(&b, "\nbpp=");
    bmp_report_buffer_append_u32(&b, img->meta.bpp);
    bmp_report_buffer_append_str(&b, "\ncompression=");
    bmp_report_buffer_append_str(&b, bmp_compression_string(img->meta.compression));
    bmp_report_buffer_append_str(&b, "\nisTopDown=");
    bmp_report_buffer_append_bool(&b, img->meta.is_top_down);
    bmp_report_buffer_append_str(&b, "\npaletteEntries=");
    bmp_report_buffer_append_u32(&b, img->meta.palette_entries);
    bmp_report_buffer_append_str(&b, "\npixelDataSize=");
    bmp_report_buffer_append_u32(&b, img->pixel_data_size);
    bmp_report_buffer_append_str(&b, "\nembeddedPayload=");
    bmp_report_buffer_append_bool(&b, bmp_image_has_embedded_payload(img));
    bmp_report_buffer_append_str(&b, "\npayloadSignatureOk=");
    bmp_append_payload_signature_text(&b, img->diagnostics.payload_signature_ok);
    bmp_report_buffer_append_str(&b, "\nwarningMask=");
    bmp_report_buffer_append_u32(&b, img->diagnostics.warning_mask);
    bmp_report_buffer_append_str(&b, "\nwarningCount=");
    bmp_report_buffer_append_int(&b, bmp_warning_count(img->diagnostics.warning_mask));
    bmp_report_buffer_append_str(&b, "\nwarnings=");
    bmp_append_warning_list_text(&b, img->diagnostics.warning_mask);
    bmp_report_buffer_append_char(&b, '\n');
    return bmp_report_finish(&b);
}

int bmp_format_diagnostics_json(const bmp_image *img,
                                char *buffer,
                                bmp_u32 buffer_size)
{
    bmp_report_buffer b;
    if (!img || (!buffer && buffer_size != 0U)) return BMP_ERR_ARGUMENT;

    bmp_report_buffer_init(&b, buffer, buffer_size);
    bmp_report_buffer_append_char(&b, '{');
    bmp_report_buffer_append_json_string(&b, "format");
    bmp_report_buffer_append_str(&b, ":");
    bmp_report_buffer_append_json_string(&b, "bmp");
    bmp_report_buffer_append_str(&b, ",");
    bmp_report_buffer_append_json_string(&b, "libraryVersion");
    bmp_report_buffer_append_str(&b, ":");
    bmp_report_buffer_append_json_string(&b, bmp_version_string());
    bmp_report_buffer_append_str(&b, ",");
    bmp_report_buffer_append_json_string(&b, "dibType");
    bmp_report_buffer_append_str(&b, ":");
    bmp_report_buffer_append_json_string(&b, bmp_dib_type_string(img->meta.dib_type));
    bmp_report_buffer_append_str(&b, ",");
    bmp_report_buffer_append_json_string(&b, "width");
    bmp_report_buffer_append_str(&b, ":");
    bmp_report_buffer_append_int(&b, (int)img->meta.width);
    bmp_report_buffer_append_str(&b, ",");
    bmp_report_buffer_append_json_string(&b, "height");
    bmp_report_buffer_append_str(&b, ":");
    bmp_report_buffer_append_int(&b, (int)img->meta.height);
    bmp_report_buffer_append_str(&b, ",");
    bmp_report_buffer_append_json_string(&b, "bpp");
    bmp_report_buffer_append_str(&b, ":");
    bmp_report_buffer_append_u32(&b, img->meta.bpp);
    bmp_report_buffer_append_str(&b, ",");
    bmp_report_buffer_append_json_string(&b, "compression");
    bmp_report_buffer_append_str(&b, ":");
    bmp_report_buffer_append_json_string(&b, bmp_compression_string(img->meta.compression));
    bmp_report_buffer_append_str(&b, ",");
    bmp_report_buffer_append_json_string(&b, "isTopDown");
    bmp_report_buffer_append_str(&b, ":");
    bmp_report_buffer_append_bool(&b, img->meta.is_top_down);
    bmp_report_buffer_append_str(&b, ",");
    bmp_report_buffer_append_json_string(&b, "paletteEntries");
    bmp_report_buffer_append_str(&b, ":");
    bmp_report_buffer_append_u32(&b, img->meta.palette_entries);
    bmp_report_buffer_append_str(&b, ",");
    bmp_report_buffer_append_json_string(&b, "pixelDataSize");
    bmp_report_buffer_append_str(&b, ":");
    bmp_report_buffer_append_u32(&b, img->pixel_data_size);
    bmp_report_buffer_append_str(&b, ",");
    bmp_report_buffer_append_json_string(&b, "embeddedPayload");
    bmp_report_buffer_append_str(&b, ":");
    bmp_report_buffer_append_bool(&b, bmp_image_has_embedded_payload(img));
    bmp_report_buffer_append_str(&b, ",");
    bmp_report_buffer_append_json_string(&b, "payloadSignatureOk");
    bmp_report_buffer_append_str(&b, ":");
    bmp_append_payload_signature_json(&b, img->diagnostics.payload_signature_ok);
    bmp_report_buffer_append_str(&b, ",");
    bmp_report_buffer_append_json_string(&b, "warningMask");
    bmp_report_buffer_append_str(&b, ":");
    bmp_report_buffer_append_u32(&b, img->diagnostics.warning_mask);
    bmp_report_buffer_append_str(&b, ",");
    bmp_report_buffer_append_json_string(&b, "warningCount");
    bmp_report_buffer_append_str(&b, ":");
    bmp_report_buffer_append_int(&b, bmp_warning_count(img->diagnostics.warning_mask));
    bmp_report_buffer_append_str(&b, ",");
    bmp_report_buffer_append_json_string(&b, "warnings");
    bmp_report_buffer_append_str(&b, ":");
    bmp_append_warning_list_json(&b, img->diagnostics.warning_mask);
    bmp_report_buffer_append_char(&b, '}');
    return bmp_report_finish(&b);
}

int bmp_write_diagnostics_text(FILE *f, const bmp_image *img)
{
    int need;
    char stackbuf[BMP_REPORT_STACK_CAP];
    if (!f || !img) return BMP_ERR_ARGUMENT;
    need = bmp_format_diagnostics_text(img, stackbuf, BMP_REPORT_STACK_CAP);
    if (need < 0) return need;
    if ((bmp_u32)need >= BMP_REPORT_STACK_CAP) return BMP_ERR_BUFFER_TOO_SMALL;
    return (fputs(stackbuf, f) == EOF) ? BMP_ERR_STREAM : BMP_OK;
}

int bmp_write_diagnostics_json(FILE *f, const bmp_image *img)
{
    int need;
    char stackbuf[BMP_REPORT_STACK_CAP];
    if (!f || !img) return BMP_ERR_ARGUMENT;
    need = bmp_format_diagnostics_json(img, stackbuf, BMP_REPORT_STACK_CAP);
    if (need < 0) return need;
    if ((bmp_u32)need >= BMP_REPORT_STACK_CAP) return BMP_ERR_BUFFER_TOO_SMALL;
    if (fputs(stackbuf, f) == EOF || fputc('\n', f) == EOF) return BMP_ERR_STREAM;
    return BMP_OK;
}
