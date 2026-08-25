#include "qoi89_internal.h"

qoi89_status qoi89_decode(
    const unsigned char *src,
    size_t src_len,
    unsigned int out_channels,
    unsigned char *pixels,
    size_t pixels_cap,
    qoi89_desc *desc_out,
    size_t *pixels_written
) {
    qoi89_status st;
    qoi89_desc desc;
    size_t expected_pixels_len;
    unsigned long pixel_count_ul;
    size_t pixel_count;
    size_t pos;
    size_t chunks_len;
    size_t out_pos;
    size_t pixel_index;
    unsigned int actual_channels;
    qoi89_rgba index_table[QOI89_INDEX_SIZE];
    qoi89_rgba px;
    unsigned int run;
    int last_chunk_was_index;
    unsigned int last_index;
    size_t i;

    if (pixels_written != NULL) {
        *pixels_written = 0u;
    }

    if (src == NULL || pixels == NULL || desc_out == NULL || pixels_written == NULL) {
        return QOI89_ERR_NULL;
    }

    if (src_len < (size_t)QOI89_HEADER_SIZE + (size_t)QOI89_PADDING_SIZE) {
        return QOI89_ERR_INPUT_TOO_SMALL;
    }

    st = qoi89_decode_header(src, src_len, &desc);
    if (st != QOI89_OK) {
        return st;
    }

    actual_channels = out_channels == 0u ? (unsigned int)desc.channels : out_channels;
    if (!qoi89_valid_channels(actual_channels)) {
        return QOI89_ERR_BAD_CHANNELS;
    }

    st = qoi89_decoded_size(&desc, actual_channels, &expected_pixels_len);
    if (st != QOI89_OK) {
        return st;
    }

    if (pixels_cap < expected_pixels_len) {
        return QOI89_ERR_OUTPUT_TOO_SMALL;
    }

    st = qoi89_pixel_count_from_desc(&desc, &pixel_count_ul);
    if (st != QOI89_OK) {
        return st;
    }
    pixel_count = (size_t)pixel_count_ul;

    chunks_len = src_len - (size_t)QOI89_PADDING_SIZE;
    for (i = 0u; i < (size_t)QOI89_PADDING_SIZE; ++i) {
        if (src[chunks_len + i] != qoi89_padding[i]) {
            return QOI89_ERR_BAD_PADDING;
        }
    }

    qoi89_zero_index(index_table);
    px.r = 0u;
    px.g = 0u;
    px.b = 0u;
    px.a = 255u;
    run = 0u;
    pos = (size_t)QOI89_HEADER_SIZE;
    out_pos = 0u;
    last_chunk_was_index = 0;
    last_index = 0u;

    for (pixel_index = 0u; pixel_index < pixel_count; ++pixel_index) {
        if (run > 0u) {
            --run;
        }
        else {
            unsigned int b1;

            if (pos >= chunks_len) {
                return QOI89_ERR_TRUNCATED;
            }

            b1 = (unsigned int)src[pos++];

            if (b1 == QOI89_OP_RGB) {
                if (chunks_len - pos < 3u) {
                    return QOI89_ERR_TRUNCATED;
                }
                px.r = src[pos++];
                px.g = src[pos++];
                px.b = src[pos++];
                last_chunk_was_index = 0;
            }
            else if (b1 == QOI89_OP_RGBA) {
                if (chunks_len - pos < 4u) {
                    return QOI89_ERR_TRUNCATED;
                }
                px.r = src[pos++];
                px.g = src[pos++];
                px.b = src[pos++];
                px.a = src[pos++];
                last_chunk_was_index = 0;
            }
            else if ((b1 & QOI89_MASK_2) == QOI89_OP_INDEX) {
                unsigned int idx;

                idx = b1 & 63u;
                if (last_chunk_was_index && idx == last_index) {
                    return QOI89_ERR_REPEATED_INDEX;
                }
                px = index_table[idx];
                last_chunk_was_index = 1;
                last_index = idx;
            }
            else if ((b1 & QOI89_MASK_2) == QOI89_OP_DIFF) {
                px.r = (unsigned char)((unsigned int)px.r + (((b1 >> 4) & 0x03u) - 2u));
                px.g = (unsigned char)((unsigned int)px.g + (((b1 >> 2) & 0x03u) - 2u));
                px.b = (unsigned char)((unsigned int)px.b + ((b1 & 0x03u) - 2u));
                last_chunk_was_index = 0;
            }
            else if ((b1 & QOI89_MASK_2) == QOI89_OP_LUMA) {
                unsigned int b2;
                int vg;

                if (chunks_len - pos < 1u) {
                    return QOI89_ERR_TRUNCATED;
                }
                b2 = (unsigned int)src[pos++];
                vg = (int)(b1 & 0x3fu) - 32;
                px.r = (unsigned char)((int)px.r + vg - 8 + (int)((b2 >> 4) & 0x0fu));
                px.g = (unsigned char)((int)px.g + vg);
                px.b = (unsigned char)((int)px.b + vg - 8 + (int)(b2 & 0x0fu));
                last_chunk_was_index = 0;
            }
            else {
                size_t remaining_pixels;

                run = b1 & 0x3fu;
                remaining_pixels = pixel_count - pixel_index;
                if ((size_t)run >= remaining_pixels) {
                    return QOI89_ERR_TRUNCATED;
                }
                last_chunk_was_index = 0;
            }

            index_table[qoi89_hash_rgba(&px)] = px;
        }

        pixels[out_pos++] = px.r;
        pixels[out_pos++] = px.g;
        pixels[out_pos++] = px.b;
        if (actual_channels == 4u) {
            pixels[out_pos++] = px.a;
        }
    }

    if (pos != chunks_len) {
        return QOI89_ERR_TRAILING_DATA;
    }

    *desc_out = desc;
    *pixels_written = out_pos;
    return QOI89_OK;
}
