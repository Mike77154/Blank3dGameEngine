#include "qoi89_internal.h"

qoi89_status qoi89_decoded_size(
    const qoi89_desc *desc,
    unsigned int out_channels,
    size_t *out_size
) {
    qoi89_status st;
    unsigned long pixel_count;
    size_t count_size;
    size_t channels_size;
    unsigned int actual_channels;

    if (desc == NULL || out_size == NULL) {
        return QOI89_ERR_NULL;
    }

    st = qoi89_validate_desc(desc);
    if (st != QOI89_OK) {
        return st;
    }

    actual_channels = out_channels == 0u ? (unsigned int)desc->channels : out_channels;
    if (!qoi89_valid_channels(actual_channels)) {
        return QOI89_ERR_BAD_CHANNELS;
    }

    st = qoi89_pixel_count_from_desc(desc, &pixel_count);
    if (st != QOI89_OK) {
        return st;
    }

    count_size = (size_t)pixel_count;
    channels_size = (size_t)actual_channels;
    if (qoi89_mul_size_t(count_size, channels_size, out_size)) {
        return QOI89_ERR_OVERFLOW;
    }

    return QOI89_OK;
}

qoi89_status qoi89_max_encoded_size(
    const qoi89_desc *desc,
    size_t *out_size
) {
    qoi89_status st;
    unsigned long pixel_count;
    size_t pixels_size;
    size_t per_pixel_max;
    size_t body_size;
    size_t total_size;

    if (desc == NULL || out_size == NULL) {
        return QOI89_ERR_NULL;
    }

    st = qoi89_validate_desc(desc);
    if (st != QOI89_OK) {
        return st;
    }

    st = qoi89_pixel_count_from_desc(desc, &pixel_count);
    if (st != QOI89_OK) {
        return st;
    }

    pixels_size = (size_t)pixel_count;
    per_pixel_max = (size_t)desc->channels + (size_t)1u;
    if (qoi89_mul_size_t(pixels_size, per_pixel_max, &body_size)) {
        return QOI89_ERR_OVERFLOW;
    }
    if (qoi89_add_size_t((size_t)QOI89_HEADER_SIZE, body_size, &total_size)) {
        return QOI89_ERR_OVERFLOW;
    }
    if (qoi89_add_size_t(total_size, (size_t)QOI89_PADDING_SIZE, out_size)) {
        return QOI89_ERR_OVERFLOW;
    }

    return QOI89_OK;
}
