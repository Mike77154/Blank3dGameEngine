#include <stdio.h>
#include <string.h>

#include "qoi89.h"

static size_t min_size(size_t a, size_t b) {
    return a < b ? a : b;
}

int main(void) {
    static const unsigned char pixels[] = {
        1u, 2u, 3u, 255u,
        1u, 2u, 3u, 255u,
        9u, 8u, 7u, 255u,
        9u, 8u, 7u, 255u
    };
    qoi89_desc desc;
    qoi89_desc decoded_desc;
    qoi89_encode_pipe enc;
    qoi89_decode_pipe dec;
    unsigned char enc_queue[32];
    unsigned char dec_queue[32];
    unsigned char qoi_data[128];
    unsigned char decoded[sizeof(pixels)];
    size_t in_pos;
    size_t qoi_pos;
    size_t dec_src_pos;
    size_t dec_out_pos;
    qoi89_status st;

    desc.width = 2ul;
    desc.height = 2ul;
    desc.channels = 4u;
    desc.colorspace = QOI89_SRGB;

    st = qoi89_encode_pipe_init(&enc, &desc, enc_queue, sizeof(enc_queue));
    if (st != QOI89_OK) {
        printf("encode pipe init failed: %s\n", qoi89_status_string(st));
        return 1;
    }

    in_pos = 0u;
    qoi_pos = 0u;
    while (!qoi89_encode_pipe_finished(&enc)) {
        if (in_pos < sizeof(pixels)) {
            size_t used;
            size_t chunk;

            chunk = min_size(sizeof(pixels) - in_pos, 3u);
            used = 0u;
            st = qoi89_encode_pipe_push(&enc, pixels + in_pos, chunk, &used);
            if (st != QOI89_STREAM_NEED_INPUT && st != QOI89_STREAM_NEED_OUTPUT && st != QOI89_STREAM_FINISHED) {
                printf("encode pipe push failed: %s\n", qoi89_status_string(st));
                return 1;
            }
            in_pos += used;
        }
        else {
            st = qoi89_encode_pipe_end(&enc);
            if (st != QOI89_STREAM_NEED_INPUT && st != QOI89_STREAM_NEED_OUTPUT && st != QOI89_STREAM_FINISHED) {
                printf("encode pipe end failed: %s\n", qoi89_status_string(st));
                return 1;
            }
        }

        if (qoi_pos == sizeof(qoi_data) && !qoi89_encode_pipe_finished(&enc)) {
            puts("qoi output buffer too small");
            return 1;
        }

        {
            size_t written;
            size_t chunk;

            chunk = min_size(sizeof(qoi_data) - qoi_pos, 5u);
            written = 0u;
            st = qoi89_encode_pipe_pull(&enc, qoi_data + qoi_pos, chunk, &written);
            if (st != QOI89_STREAM_NEED_INPUT && st != QOI89_STREAM_NEED_OUTPUT && st != QOI89_STREAM_FINISHED) {
                printf("encode pipe pull failed: %s\n", qoi89_status_string(st));
                return 1;
            }
            qoi_pos += written;
        }
    }

    st = qoi89_decode_pipe_init(&dec, 0u, dec_queue, sizeof(dec_queue));
    if (st != QOI89_OK) {
        printf("decode pipe init failed: %s\n", qoi89_status_string(st));
        return 1;
    }

    decoded_desc.width = 0ul;
    decoded_desc.height = 0ul;
    decoded_desc.channels = 0u;
    decoded_desc.colorspace = 0u;
    dec_src_pos = 0u;
    dec_out_pos = 0u;
    while (!qoi89_decode_pipe_finished(&dec)) {
        if (dec_src_pos < qoi_pos) {
            size_t used;
            size_t chunk;

            chunk = min_size(qoi_pos - dec_src_pos, 4u);
            used = 0u;
            st = qoi89_decode_pipe_push(&dec, qoi_data + dec_src_pos, chunk, &used);
            if (st != QOI89_STREAM_NEED_INPUT && st != QOI89_STREAM_NEED_OUTPUT && st != QOI89_STREAM_FINISHED) {
                printf("decode pipe push failed: %s\n", qoi89_status_string(st));
                return 1;
            }
            dec_src_pos += used;
        }
        else {
            st = qoi89_decode_pipe_end(&dec);
            if (st != QOI89_STREAM_NEED_INPUT && st != QOI89_STREAM_NEED_OUTPUT && st != QOI89_STREAM_FINISHED) {
                printf("decode pipe end failed: %s\n", qoi89_status_string(st));
                return 1;
            }
        }

        if (qoi89_decode_pipe_header_ready(&dec) && decoded_desc.width == 0ul) {
            st = qoi89_decode_pipe_get_desc(&dec, &decoded_desc);
            if (st != QOI89_OK) {
                printf("decode pipe get_desc failed: %s\n", qoi89_status_string(st));
                return 1;
            }
        }

        if (qoi89_decode_pipe_output_pending(&dec) > 0u) {
            size_t written;
            size_t chunk;

            chunk = min_size(sizeof(decoded) - dec_out_pos, 3u);
            written = 0u;
            st = qoi89_decode_pipe_pull(&dec, decoded + dec_out_pos, chunk, &written);
            if (st != QOI89_STREAM_NEED_INPUT && st != QOI89_STREAM_NEED_OUTPUT && st != QOI89_STREAM_FINISHED) {
                printf("decode pipe pull failed: %s\n", qoi89_status_string(st));
                return 1;
            }
            dec_out_pos += written;
        }
    }

    if (dec_out_pos != sizeof(pixels) || memcmp(decoded, pixels, sizeof(pixels)) != 0) {
        puts("pipeline roundtrip mismatch");
        return 1;
    }

    printf("pipeline ok, qoi bytes=%lu, decoded=%lux%lu channels=%u\n",
        (unsigned long)qoi_pos,
        decoded_desc.width,
        decoded_desc.height,
        (unsigned int)decoded_desc.channels);
    return 0;
}
