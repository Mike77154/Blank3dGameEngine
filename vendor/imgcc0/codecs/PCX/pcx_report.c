#include "pcx_report.h"

#include <limits.h>
#include <string.h>
#include <stdio.h>
#include "pcx_config89.h"

#define PCX_REPORT_WARNING_COUNT 9

typedef struct PCXReportBufferTag
{
    char   *dst;
    pcx_size  cap;
    pcx_size  len;
} PCXReportBuffer;

static void pcx_report_buffer_init(PCXReportBuffer *b, char *dst, pcx_size cap)
{
    if (b == NULL)
    {
        return;
    }
    b->dst = dst;
    b->cap = cap;
    b->len = 0U;
    if (dst != NULL && cap > 0U)
    {
        dst[0] = '\0';
    }
}

static void pcx_report_buffer_append_mem(PCXReportBuffer *b,
                                         const char *src,
                                         pcx_size srcLen)
{
    pcx_size copyLen = 0U;
    if (b == NULL || src == NULL)
    {
        return;
    }

    if (b->dst != NULL && b->cap > 0U && b->len < b->cap - 1U)
    {
        copyLen = b->cap - 1U - b->len;
        if (copyLen > srcLen)
        {
            copyLen = srcLen;
        }
        if (copyLen > 0U)
        {
            memcpy(b->dst + b->len, src, copyLen);
            b->dst[b->len + copyLen] = '\0';
        }
    }

    b->len += srcLen;
}

static void pcx_report_buffer_append_str(PCXReportBuffer *b, const char *src)
{
    if (src == NULL)
    {
        return;
    }
    pcx_report_buffer_append_mem(b, src, strlen(src));
}

static void pcx_report_buffer_append_char(PCXReportBuffer *b, char c)
{
    pcx_report_buffer_append_mem(b, &c, 1U);
}

static void pcx_report_buffer_append_int(PCXReportBuffer *b, int value)
{
    char tmp[64];
    sprintf(tmp, "%d", value);
    pcx_report_buffer_append_str(b, tmp);
}

static void pcx_report_buffer_append_ulong(PCXReportBuffer *b,
                                           pcx_u32 value)
{
    char tmp[64];
    sprintf(tmp, "%u", value);
    pcx_report_buffer_append_str(b, tmp);
}

static void pcx_report_buffer_append_bool(PCXReportBuffer *b, int value)
{
    pcx_report_buffer_append_str(b, value ? "true" : "false");
}

static void pcx_report_buffer_append_json_string(PCXReportBuffer *b,
                                                 const char *value)
{
    if (value == NULL)
    {
        value = "";
    }

    pcx_report_buffer_append_char(b, '"');
    while (*value != '\0')
    {
        switch (*value)
        {
        case '\\':
            pcx_report_buffer_append_str(b, "\\\\");
            break;
        case '"':
            pcx_report_buffer_append_str(b, "\\\"");
            break;
        case '\n':
            pcx_report_buffer_append_str(b, "\\n");
            break;
        case '\r':
            pcx_report_buffer_append_str(b, "\\r");
            break;
        case '\t':
            pcx_report_buffer_append_str(b, "\\t");
            break;
        default:
            pcx_report_buffer_append_char(b, *value);
            break;
        }
        ++value;
    }
    pcx_report_buffer_append_char(b, '"');
}

static int pcx_report_finish(PCXReportBuffer *b)
{
    if (b == NULL)
    {
        return PCX_ERR_NULL_POINTER;
    }
    if (b->len > (pcx_size)INT_MAX)
    {
        return PCX_ERR_OVERFLOW;
    }
    if (b->dst != NULL && b->cap > 0U)
    {
        if (b->len < b->cap)
        {
            b->dst[b->len] = '\0';
        }
        else
        {
            b->dst[b->cap - 1U] = '\0';
        }
    }
    return (int)b->len;
}

static const pcx_u32 g_pcx_warning_flags[PCX_REPORT_WARNING_COUNT] = {
    PCX_WARN_UNKNOWN_VERSION,
    PCX_WARN_RAW_ENCODING,
    PCX_WARN_RESERVED_NONZERO,
    PCX_WARN_ODD_BYTES_PER_LINE,
    PCX_WARN_STRICT_LAYOUT_UNSUPPORTED,
    PCX_WARN_EXTENDED_PALETTE_MISSING,
    PCX_WARN_GRAYSCALE_FALLBACK,
    PCX_WARN_HEADER16_PALETTE_FALLBACK,
    PCX_WARN_SCANLINE_PADDING
};

const char *pcx_version_string(void)
{
    return PCX_VERSION_STRING;
}

pcx_u32 pcx_version_number(void)
{
    return PCX_VERSION_NUMBER;
}

const char *pcx_decoded_format_to_string(PCXDecodedFormat fmt)
{
    switch (fmt)
    {
    case PCX_DECODED_FORMAT_INDEXED:
        return "PCX_DECODED_FORMAT_INDEXED";
    case PCX_DECODED_FORMAT_RGB:
        return "PCX_DECODED_FORMAT_RGB";
    case PCX_DECODED_FORMAT_UNKNOWN:
    default:
        return "PCX_DECODED_FORMAT_UNKNOWN";
    }
}

int pcx_warning_count(pcx_u32 mask)
{
    int count = 0;
    int i;
    for (i = 0; i < PCX_REPORT_WARNING_COUNT; ++i)
    {
        if (pcx_warning_mask_has(mask, g_pcx_warning_flags[i]))
        {
            ++count;
        }
    }
    return count;
}

static void pcx_append_warning_list_text(PCXReportBuffer *b,
                                         pcx_u32 mask)
{
    int i;
    int first = 1;
    for (i = 0; i < PCX_REPORT_WARNING_COUNT; ++i)
    {
        pcx_u32 flag = g_pcx_warning_flags[i];
        if (!pcx_warning_mask_has(mask, flag))
        {
            continue;
        }
        if (!first)
        {
            pcx_report_buffer_append_str(b, ", ");
        }
        first = 0;
        pcx_report_buffer_append_str(b, pcx_warning_flag_to_string(flag));
    }
    if (first)
    {
        pcx_report_buffer_append_str(b, "none");
    }
}

static void pcx_append_warning_list_json(PCXReportBuffer *b,
                                         pcx_u32 mask)
{
    int i;
    int first = 1;
    pcx_report_buffer_append_char(b, '[');
    for (i = 0; i < PCX_REPORT_WARNING_COUNT; ++i)
    {
        pcx_u32 flag = g_pcx_warning_flags[i];
        if (!pcx_warning_mask_has(mask, flag))
        {
            continue;
        }
        if (!first)
        {
            pcx_report_buffer_append_char(b, ',');
        }
        first = 0;
        pcx_report_buffer_append_json_string(b, pcx_warning_flag_to_string(flag));
    }
    pcx_report_buffer_append_char(b, ']');
}

int pcx_format_diagnostics_text(const PCXFileInfo *info,
                                char *buffer,
                                pcx_size bufferSize)
{
    PCXReportBuffer b;
    if (info == NULL || (buffer == NULL && bufferSize != 0U))
    {
        return PCX_ERR_NULL_POINTER;
    }

    pcx_report_buffer_init(&b, buffer, bufferSize);
    pcx_report_buffer_append_str(&b, "format=pcx\n");
    pcx_report_buffer_append_str(&b, "libraryVersion=");
    pcx_report_buffer_append_str(&b, pcx_version_string());
    pcx_report_buffer_append_str(&b, "\nwidth=");
    pcx_report_buffer_append_int(&b, info->width);
    pcx_report_buffer_append_str(&b, "\nheight=");
    pcx_report_buffer_append_int(&b, info->height);
    pcx_report_buffer_append_str(&b, "\ntotalBitsPerPixel=");
    pcx_report_buffer_append_int(&b, info->totalBitsPerPixel);
    pcx_report_buffer_append_str(&b, "\nchannelsAfterDecode=");
    pcx_report_buffer_append_int(&b, info->channelsAfterDecode);
    pcx_report_buffer_append_str(&b, "\nusesExtendedPaletteHint=");
    pcx_report_buffer_append_bool(&b, info->usesExtendedPaletteHint);
    pcx_report_buffer_append_str(&b, "\nstrictHeaderPasses=");
    pcx_report_buffer_append_bool(&b, info->strictHeaderPasses);
    pcx_report_buffer_append_str(&b, "\ndecodedFormat=");
    pcx_report_buffer_append_str(&b,
                                 pcx_decoded_format_to_string(info->decodedFormat));
    pcx_report_buffer_append_str(&b, "\npaletteSource=");
    pcx_report_buffer_append_str(&b,
                                 pcx_palette_source_to_string(info->diagnostics.paletteSource));
    pcx_report_buffer_append_str(&b, "\nstrictWouldFail=");
    pcx_report_buffer_append_bool(&b, info->diagnostics.strictWouldFail);
    pcx_report_buffer_append_str(&b, "\nwarningMask=");
    pcx_report_buffer_append_ulong(&b, info->diagnostics.warningMask);
    pcx_report_buffer_append_str(&b, "\nwarningCount=");
    pcx_report_buffer_append_int(&b,
                                 pcx_warning_count(info->diagnostics.warningMask));
    pcx_report_buffer_append_str(&b, "\nwarnings=");
    pcx_append_warning_list_text(&b, info->diagnostics.warningMask);
    pcx_report_buffer_append_char(&b, '\n');
    return pcx_report_finish(&b);
}

int pcx_format_diagnostics_json(const PCXFileInfo *info,
                                char *buffer,
                                pcx_size bufferSize)
{
    PCXReportBuffer b;
    if (info == NULL || (buffer == NULL && bufferSize != 0U))
    {
        return PCX_ERR_NULL_POINTER;
    }

    pcx_report_buffer_init(&b, buffer, bufferSize);
    pcx_report_buffer_append_char(&b, '{');
    pcx_report_buffer_append_json_string(&b, "format");
    pcx_report_buffer_append_str(&b, ":");
    pcx_report_buffer_append_json_string(&b, "pcx");
    pcx_report_buffer_append_str(&b, ",");
    pcx_report_buffer_append_json_string(&b, "libraryVersion");
    pcx_report_buffer_append_str(&b, ":");
    pcx_report_buffer_append_json_string(&b, pcx_version_string());
    pcx_report_buffer_append_str(&b, ",");
    pcx_report_buffer_append_json_string(&b, "width");
    pcx_report_buffer_append_str(&b, ":");
    pcx_report_buffer_append_int(&b, info->width);
    pcx_report_buffer_append_str(&b, ",");
    pcx_report_buffer_append_json_string(&b, "height");
    pcx_report_buffer_append_str(&b, ":");
    pcx_report_buffer_append_int(&b, info->height);
    pcx_report_buffer_append_str(&b, ",");
    pcx_report_buffer_append_json_string(&b, "totalBitsPerPixel");
    pcx_report_buffer_append_str(&b, ":");
    pcx_report_buffer_append_int(&b, info->totalBitsPerPixel);
    pcx_report_buffer_append_str(&b, ",");
    pcx_report_buffer_append_json_string(&b, "channelsAfterDecode");
    pcx_report_buffer_append_str(&b, ":");
    pcx_report_buffer_append_int(&b, info->channelsAfterDecode);
    pcx_report_buffer_append_str(&b, ",");
    pcx_report_buffer_append_json_string(&b, "usesExtendedPaletteHint");
    pcx_report_buffer_append_str(&b, ":");
    pcx_report_buffer_append_bool(&b, info->usesExtendedPaletteHint);
    pcx_report_buffer_append_str(&b, ",");
    pcx_report_buffer_append_json_string(&b, "strictHeaderPasses");
    pcx_report_buffer_append_str(&b, ":");
    pcx_report_buffer_append_bool(&b, info->strictHeaderPasses);
    pcx_report_buffer_append_str(&b, ",");
    pcx_report_buffer_append_json_string(&b, "decodedFormat");
    pcx_report_buffer_append_str(&b, ":");
    pcx_report_buffer_append_json_string(&b,
        pcx_decoded_format_to_string(info->decodedFormat));
    pcx_report_buffer_append_str(&b, ",");
    pcx_report_buffer_append_json_string(&b, "paletteSource");
    pcx_report_buffer_append_str(&b, ":");
    pcx_report_buffer_append_json_string(&b,
        pcx_palette_source_to_string(info->diagnostics.paletteSource));
    pcx_report_buffer_append_str(&b, ",");
    pcx_report_buffer_append_json_string(&b, "strictWouldFail");
    pcx_report_buffer_append_str(&b, ":");
    pcx_report_buffer_append_bool(&b, info->diagnostics.strictWouldFail);
    pcx_report_buffer_append_str(&b, ",");
    pcx_report_buffer_append_json_string(&b, "warningMask");
    pcx_report_buffer_append_str(&b, ":");
    pcx_report_buffer_append_ulong(&b, info->diagnostics.warningMask);
    pcx_report_buffer_append_str(&b, ",");
    pcx_report_buffer_append_json_string(&b, "warningCount");
    pcx_report_buffer_append_str(&b, ":");
    pcx_report_buffer_append_int(&b, pcx_warning_count(info->diagnostics.warningMask));
    pcx_report_buffer_append_str(&b, ",");
    pcx_report_buffer_append_json_string(&b, "warnings");
    pcx_report_buffer_append_str(&b, ":");
    pcx_append_warning_list_json(&b, info->diagnostics.warningMask);
    pcx_report_buffer_append_char(&b, '}');
    return pcx_report_finish(&b);
}

int pcx_write_diagnostics_text(FILE *f, const PCXFileInfo *info)
{
    int rc;
    static char reportbuf[PCX89_STATIC_REPORT_BYTES];

    if (f == NULL || info == NULL)
    {
        return PCX_ERR_NULL_POINTER;
    }

    rc = pcx_format_diagnostics_text(info, reportbuf, PCX89_STATIC_REPORT_BYTES);
    if (rc < 0)
    {
        return rc;
    }
    if ((pcx_size)rc >= PCX89_STATIC_REPORT_BYTES)
    {
        return PCX_ERR_LIMITS;
    }
    return (fputs(reportbuf, f) == EOF) ? PCX_ERR_IO : PCX_OK;
}

int pcx_write_diagnostics_json(FILE *f, const PCXFileInfo *info)
{
    int rc;
    static char reportbuf[PCX89_STATIC_REPORT_BYTES];

    if (f == NULL || info == NULL)
    {
        return PCX_ERR_NULL_POINTER;
    }

    rc = pcx_format_diagnostics_json(info, reportbuf, PCX89_STATIC_REPORT_BYTES);
    if (rc < 0)
    {
        return rc;
    }
    if ((pcx_size)rc >= PCX89_STATIC_REPORT_BYTES)
    {
        return PCX_ERR_LIMITS;
    }
    if (fputs(reportbuf, f) == EOF || fputc('\n', f) == EOF)
    {
        return PCX_ERR_IO;
    }
    return PCX_OK;
}
