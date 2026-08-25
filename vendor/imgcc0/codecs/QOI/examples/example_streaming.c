#include <stdio.h>

#include "qoi89.h"

static size_t min_size(size_t a, size_t b) {
    return a < b ? a : b;
}

int main(void) {
    qoi89_desc desc;
    unsigned char pixels[12];
    unsigned char encoded[128];
    unsigned char decoded[12];
    qoi89_encode_stream enc;
    qoi89_decode_stream dec;
    size_t in_pos;
    size_t qoi_len;
    size_t pix_len;
    qoi89_status st;

    desc.width = 3ul;
    desc.height = 1ul;
    desc.channels = 4u;
    desc.colorspace = QOI89_SRGB;

    pixels[0] = 255u; pixels[1] = 0u;   pixels[2] = 0u;   pixels[3] = 255u;
    pixels[4] = 255u; pixels[5] = 0u;   pixels[6] = 0u;   pixels[7] = 255u;
    pixels[8] = 0u;   pixels[9] = 255u; pixels[10] = 0u;  pixels[11] = 255u;

    st = qoi89_encode_stream_init(&enc, &desc);
    if (st != QOI89_OK) {
        printf("encode init failed: %s\n", qoi89_status_string(st));
        return 1;
    }

    in_pos = 0u;
    qoi_len = 0u;
    while (in_pos < sizeof(pixels)) {
        size_t used;
        size_t written;
        size_t src_chunk;
        size_t dst_chunk;

        src_chunk = min_size(sizeof(pixels) - in_pos, 3u);
        dst_chunk = min_size(sizeof(encoded) - qoi_len, 4u);
        st = qoi89_encode_stream_process(
            &enc,
            pixels + in_pos,
            src_chunk,
            &used,
            encoded + qoi_len,
            dst_chunk,
            &written
        );
        in_pos += used;
        qoi_len += written;

        if (st != QOI89_STREAM_NEED_INPUT && st != QOI89_STREAM_NEED_OUTPUT) {
            printf("encode process failed: %s\n", qoi89_status_string(st));
            return 1;
        }
    }

    while (1) {
        size_t written;
        size_t dst_chunk;

        dst_chunk = min_size(sizeof(encoded) - qoi_len, 4u);
        st = qoi89_encode_stream_finish(&enc, encoded + qoi_len, dst_chunk, &written);
        qoi_len += written;
        if (st == QOI89_STREAM_NEED_OUTPUT) {
            continue;
        }
        if (st != QOI89_STREAM_FINISHED) {
            printf("encode finish failed: %s\n", qoi89_status_string(st));
            return 1;
        }
        break;
    }

    st = qoi89_decode_stream_init(&dec, 0u);
    if (st != QOI89_OK) {
        printf("decode init failed: %s\n", qoi89_status_string(st));
        return 1;
    }

    in_pos = 0u;
    pix_len = 0u;
    while (1) {
        size_t used;
        size_t written;
        size_t src_chunk;
        size_t dst_chunk;

        src_chunk = min_size(qoi_len - in_pos, 5u);
        dst_chunk = min_size(sizeof(decoded) - pix_len, 4u);
        st = qoi89_decode_stream_process(
            &dec,
            encoded + in_pos,
            src_chunk,
            &used,
            decoded + pix_len,
            dst_chunk,
            &written
        );
        in_pos += used;
        pix_len += written;

        if (st == QOI89_STREAM_NEED_INPUT || st == QOI89_STREAM_NEED_OUTPUT) {
            continue;
        }
        if (st != QOI89_STREAM_FINISHED) {
            printf("decode process failed: %s\n", qoi89_status_string(st));
            return 1;
        }
        break;
    }

    printf("streamed qoi_len=%lu decoded_len=%lu\n",
        (unsigned long)qoi_len,
        (unsigned long)pix_len);

    return 0;
}
