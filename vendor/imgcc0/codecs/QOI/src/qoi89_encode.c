#include "qoi89_internal.h"

qoi89_status qoi89_encode(
    const unsigned char *pixels,
    size_t pixels_len,
    const qoi89_desc *desc,
    unsigned char *dst,
    size_t dst_cap,
    size_t *dst_len
) {
    qoi89_status st;
    unsigned long pixel_count;
    size_t expected_pixels_len;
    size_t pos;
    size_t pixel_index;
    size_t src_pos;
    qoi89_rgba index_table[QOI89_INDEX_SIZE];
    qoi89_rgba px;
    qoi89_rgba prev;
    unsigned int run;
    unsigned int channels;

    if (dst_len != NULL) {
        *dst_len = 0u;
    }

    if (pixels == NULL || desc == NULL || dst == NULL || dst_len == NULL) {
        return QOI89_ERR_NULL;
    }

    st = qoi89_validate_desc(desc);
    if (st != QOI89_OK) {
        return st;
    }

    channels = (unsigned int)desc->channels;
    st = qoi89_pixel_count_from_desc(desc, &pixel_count);
    if (st != QOI89_OK) {
        return st;
    }

    if (qoi89_mul_size_t((size_t)pixel_count, (size_t)channels, &expected_pixels_len)) {
        return QOI89_ERR_OVERFLOW;
    }

    if (pixels_len != expected_pixels_len) {
        return QOI89_ERR_BAD_ARGUMENT;
    }

    pos = 0u;
    st = qoi89_write_header(desc, dst, dst_cap, &pos);
    if (st != QOI89_OK) {
        return st;
    }

    qoi89_zero_index(index_table);

    prev.r = 0u;
    prev.g = 0u;
    prev.b = 0u;
    prev.a = 255u;
    px = prev;
    run = 0u;
    src_pos = 0u;

    for (pixel_index = 0u; pixel_index < (size_t)pixel_count; ++pixel_index) {
        unsigned int hash_index;

        px.r = pixels[src_pos + 0u];
        px.g = pixels[src_pos + 1u];
        px.b = pixels[src_pos + 2u];
        if (channels == 4u) {
            px.a = pixels[src_pos + 3u];
        }
        else {
            px.a = 255u;
        }
        src_pos += (size_t)channels;

        if (qoi89_rgba_equal(&px, &prev)) {
            ++run;
            if (run == 62u || (pixel_index + 1u) == (size_t)pixel_count) {
                if (qoi89_need_space(pos, dst_cap, 1u)) {
                    return QOI89_ERR_OUTPUT_TOO_SMALL;
                }
                dst[pos++] = (unsigned char)(QOI89_OP_RUN | (run - 1u));
                run = 0u;
            }
        }
        else {
            if (run > 0u) {
                if (qoi89_need_space(pos, dst_cap, 1u)) {
                    return QOI89_ERR_OUTPUT_TOO_SMALL;
                }
                dst[pos++] = (unsigned char)(QOI89_OP_RUN | (run - 1u));
                run = 0u;
            }

            hash_index = qoi89_hash_rgba(&px);
            if (qoi89_rgba_equal(&index_table[hash_index], &px)) {
                if (qoi89_need_space(pos, dst_cap, 1u)) {
                    return QOI89_ERR_OUTPUT_TOO_SMALL;
                }
                dst[pos++] = (unsigned char)(QOI89_OP_INDEX | hash_index);
            }
            else {
                int vr;
                int vg;
                int vb;
                int vg_r;
                int vg_b;

                index_table[hash_index] = px;

                if (px.a == prev.a) {
                    vr = qoi89_wrap_diff_u8((unsigned int)px.r, (unsigned int)prev.r);
                    vg = qoi89_wrap_diff_u8((unsigned int)px.g, (unsigned int)prev.g);
                    vb = qoi89_wrap_diff_u8((unsigned int)px.b, (unsigned int)prev.b);
                    vg_r = vr - vg;
                    vg_b = vb - vg;

                    if (vr > -3 && vr < 2 && vg > -3 && vg < 2 && vb > -3 && vb < 2) {
                        if (qoi89_need_space(pos, dst_cap, 1u)) {
                            return QOI89_ERR_OUTPUT_TOO_SMALL;
                        }
                        dst[pos++] = (unsigned char)(
                            QOI89_OP_DIFF |
                            ((unsigned int)(vr + 2) << 4) |
                            ((unsigned int)(vg + 2) << 2) |
                            (unsigned int)(vb + 2)
                        );
                    }
                    else if (vg_r > -9 && vg_r < 8 && vg > -33 && vg < 32 && vg_b > -9 && vg_b < 8) {
                        if (qoi89_need_space(pos, dst_cap, 2u)) {
                            return QOI89_ERR_OUTPUT_TOO_SMALL;
                        }
                        dst[pos++] = (unsigned char)(QOI89_OP_LUMA | (unsigned int)(vg + 32));
                        dst[pos++] = (unsigned char)(
                            ((unsigned int)(vg_r + 8) << 4) |
                            (unsigned int)(vg_b + 8)
                        );
                    }
                    else {
                        if (qoi89_need_space(pos, dst_cap, 4u)) {
                            return QOI89_ERR_OUTPUT_TOO_SMALL;
                        }
                        dst[pos++] = (unsigned char)QOI89_OP_RGB;
                        dst[pos++] = px.r;
                        dst[pos++] = px.g;
                        dst[pos++] = px.b;
                    }
                }
                else {
                    if (qoi89_need_space(pos, dst_cap, 5u)) {
                        return QOI89_ERR_OUTPUT_TOO_SMALL;
                    }
                    dst[pos++] = (unsigned char)QOI89_OP_RGBA;
                    dst[pos++] = px.r;
                    dst[pos++] = px.g;
                    dst[pos++] = px.b;
                    dst[pos++] = px.a;
                }
            }
        }

        prev = px;
    }

    if (qoi89_need_space(pos, dst_cap, (size_t)QOI89_PADDING_SIZE)) {
        return QOI89_ERR_OUTPUT_TOO_SMALL;
    }

    for (pixel_index = 0u; pixel_index < (size_t)QOI89_PADDING_SIZE; ++pixel_index) {
        dst[pos++] = qoi89_padding[pixel_index];
    }

    *dst_len = pos;
    return QOI89_OK;
}
