#include "qoi89_internal.h"

const char *qoi89_status_string(qoi89_status status) {
    switch (status) {
        case QOI89_OK: return "QOI89_OK";
        case QOI89_ERR_NULL: return "QOI89_ERR_NULL";
        case QOI89_ERR_BAD_ARGUMENT: return "QOI89_ERR_BAD_ARGUMENT";
        case QOI89_ERR_BAD_MAGIC: return "QOI89_ERR_BAD_MAGIC";
        case QOI89_ERR_BAD_DIMENSIONS: return "QOI89_ERR_BAD_DIMENSIONS";
        case QOI89_ERR_BAD_CHANNELS: return "QOI89_ERR_BAD_CHANNELS";
        case QOI89_ERR_BAD_COLORSPACE: return "QOI89_ERR_BAD_COLORSPACE";
        case QOI89_ERR_OVERFLOW: return "QOI89_ERR_OVERFLOW";
        case QOI89_ERR_INPUT_TOO_SMALL: return "QOI89_ERR_INPUT_TOO_SMALL";
        case QOI89_ERR_OUTPUT_TOO_SMALL: return "QOI89_ERR_OUTPUT_TOO_SMALL";
        case QOI89_ERR_TRUNCATED: return "QOI89_ERR_TRUNCATED";
        case QOI89_ERR_BAD_PADDING: return "QOI89_ERR_BAD_PADDING";
        case QOI89_ERR_TRAILING_DATA: return "QOI89_ERR_TRAILING_DATA";
        case QOI89_ERR_REPEATED_INDEX: return "QOI89_ERR_REPEATED_INDEX";
        case QOI89_ERR_STATE: return "QOI89_ERR_STATE";
        case QOI89_ERR_CALLBACK: return "QOI89_ERR_CALLBACK";
        case QOI89_ERR_IO_STALL: return "QOI89_ERR_IO_STALL";
        case QOI89_STREAM_NEED_INPUT: return "QOI89_STREAM_NEED_INPUT";
        case QOI89_STREAM_NEED_OUTPUT: return "QOI89_STREAM_NEED_OUTPUT";
        case QOI89_STREAM_FINISHED: return "QOI89_STREAM_FINISHED";
        default: return "QOI89_ERR_UNKNOWN";
    }
}

qoi89_status qoi89_pixel_count_from_desc(
    const qoi89_desc *desc,
    unsigned long *pixel_count_out
) {
    if (desc == NULL || pixel_count_out == NULL) {
        return QOI89_ERR_NULL;
    }

    if (desc->width == 0ul || desc->height == 0ul) {
        return QOI89_ERR_BAD_DIMENSIONS;
    }

    if (desc->width > QOI89_U32_MAX || desc->height > QOI89_U32_MAX) {
        return QOI89_ERR_BAD_DIMENSIONS;
    }

    if (desc->height >= (QOI89_PIXELS_LIMIT / desc->width)) {
        return QOI89_ERR_BAD_DIMENSIONS;
    }

    *pixel_count_out = desc->width * desc->height;
    return QOI89_OK;
}

qoi89_status qoi89_validate_desc(const qoi89_desc *desc) {
    qoi89_status st;
    unsigned long pixel_count;

    if (desc == NULL) {
        return QOI89_ERR_NULL;
    }

    if (!qoi89_valid_channels((unsigned int)desc->channels)) {
        return QOI89_ERR_BAD_CHANNELS;
    }

    if ((unsigned int)desc->colorspace > 1u) {
        return QOI89_ERR_BAD_COLORSPACE;
    }

    st = qoi89_pixel_count_from_desc(desc, &pixel_count);
    if (st != QOI89_OK) {
        return st;
    }

    (void)pixel_count;
    return QOI89_OK;
}
