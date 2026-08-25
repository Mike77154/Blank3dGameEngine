#include "qoi89_internal.h"

qoi89_status qoi89_decode_header(
    const unsigned char *src,
    size_t src_len,
    qoi89_desc *desc_out
) {
    size_t pos;
    unsigned long magic;
    qoi89_status st;

    if (src == NULL || desc_out == NULL) {
        return QOI89_ERR_NULL;
    }

    if (src_len < QOI89_HEADER_SIZE) {
        return QOI89_ERR_INPUT_TOO_SMALL;
    }

    pos = 0u;
    magic = qoi89_read_u32be(src, &pos);
    if (magic != 0x716f6966ul) {
        return QOI89_ERR_BAD_MAGIC;
    }

    desc_out->width = qoi89_read_u32be(src, &pos);
    desc_out->height = qoi89_read_u32be(src, &pos);
    desc_out->channels = src[pos++];
    desc_out->colorspace = src[pos++];

    st = qoi89_validate_desc(desc_out);
    return st;
}

qoi89_status qoi89_write_header(
    const qoi89_desc *desc,
    unsigned char *dst,
    size_t dst_cap,
    size_t *pos
) {
    if (desc == NULL || dst == NULL || pos == NULL) {
        return QOI89_ERR_NULL;
    }

    if (qoi89_need_space(*pos, dst_cap, (size_t)QOI89_HEADER_SIZE)) {
        return QOI89_ERR_OUTPUT_TOO_SMALL;
    }

    dst[(*pos)++] = 'q';
    dst[(*pos)++] = 'o';
    dst[(*pos)++] = 'i';
    dst[(*pos)++] = 'f';
    qoi89_write_u32be(dst, pos, desc->width);
    qoi89_write_u32be(dst, pos, desc->height);
    dst[(*pos)++] = desc->channels;
    dst[(*pos)++] = desc->colorspace;

    return QOI89_OK;
}
