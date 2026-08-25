#include <stdio.h>
#include <string.h>

#include "test_common.h"

static size_t min_size(size_t a, size_t b) {
    return a < b ? a : b;
}

static qoi89_status stream_encode_all(
    const unsigned char *pixels,
    size_t pixels_len,
    const qoi89_desc *desc,
    unsigned int input_step,
    unsigned int output_step,
    unsigned char *dst,
    size_t dst_cap,
    size_t *dst_len
) {
    qoi89_encode_stream stream;
    qoi89_status st;
    size_t in_pos;
    size_t out_pos;
    unsigned long guard;

    st = qoi89_encode_stream_init(&stream, desc);
    if (st != QOI89_OK) {
        return st;
    }

    in_pos = 0u;
    out_pos = 0u;
    guard = 0ul;

    while (1) {
        size_t src_chunk;
        size_t dst_chunk;
        size_t used;
        size_t written;

        if (++guard > 100000ul) {
            return QOI89_ERR_STATE;
        }

        src_chunk = min_size(pixels_len - in_pos, (size_t)input_step);
        dst_chunk = min_size(dst_cap - out_pos, (size_t)output_step);

        st = qoi89_encode_stream_process(
            &stream,
            pixels + in_pos,
            src_chunk,
            &used,
            dst + out_pos,
            dst_chunk,
            &written
        );
        in_pos += used;
        out_pos += written;

        if (st == QOI89_STREAM_NEED_OUTPUT) {
            continue;
        }
        if (st == QOI89_STREAM_NEED_INPUT) {
            if (in_pos == pixels_len) {
                break;
            }
            continue;
        }
        return st;
    }

    while (1) {
        size_t dst_chunk;
        size_t written;

        if (++guard > 200000ul) {
            return QOI89_ERR_STATE;
        }

        dst_chunk = min_size(dst_cap - out_pos, (size_t)output_step);
        st = qoi89_encode_stream_finish(&stream, dst + out_pos, dst_chunk, &written);
        out_pos += written;

        if (st == QOI89_STREAM_NEED_OUTPUT) {
            continue;
        }
        if (st == QOI89_STREAM_FINISHED) {
            *dst_len = out_pos;
            if (!qoi89_encode_stream_finished(&stream)) {
                return QOI89_ERR_STATE;
            }
            return QOI89_OK;
        }
        return st;
    }
}

static qoi89_status stream_decode_all(
    const unsigned char *src,
    size_t src_len,
    unsigned int out_channels,
    unsigned int input_step,
    unsigned int output_step,
    unsigned char *pixels,
    size_t pixels_cap,
    qoi89_desc *desc_out,
    size_t *pixels_len
) {
    qoi89_decode_stream stream;
    qoi89_status st;
    size_t src_pos;
    size_t out_pos;
    unsigned long guard;

    st = qoi89_decode_stream_init(&stream, out_channels);
    if (st != QOI89_OK) {
        return st;
    }

    src_pos = 0u;
    out_pos = 0u;
    guard = 0ul;

    while (1) {
        size_t src_chunk;
        size_t dst_chunk;
        size_t used;
        size_t written;

        if (++guard > 200000ul) {
            return QOI89_ERR_STATE;
        }

        src_chunk = min_size(src_len - src_pos, (size_t)input_step);
        dst_chunk = min_size(pixels_cap - out_pos, (size_t)output_step);

        st = qoi89_decode_stream_process(
            &stream,
            src + src_pos,
            src_chunk,
            &used,
            pixels + out_pos,
            dst_chunk,
            &written
        );
        src_pos += used;
        out_pos += written;

        if (qoi89_decode_stream_header_ready(&stream) && desc_out->width == 0ul) {
            qoi89_status st_desc;
            st_desc = qoi89_decode_stream_get_desc(&stream, desc_out);
            if (st_desc != QOI89_OK) {
                return st_desc;
            }
        }

        if (st == QOI89_STREAM_NEED_OUTPUT) {
            continue;
        }
        if (st == QOI89_STREAM_NEED_INPUT) {
            if (src_pos == src_len) {
                st = qoi89_decode_stream_finish(&stream);
                if (st != QOI89_STREAM_FINISHED) {
                    return st;
                }
                *pixels_len = out_pos;
                return QOI89_OK;
            }
            continue;
        }
        if (st == QOI89_STREAM_FINISHED) {
            *pixels_len = out_pos;
            if (!qoi89_decode_stream_finished(&stream)) {
                return QOI89_ERR_STATE;
            }
            return QOI89_OK;
        }
        return st;
    }
}

static void test_stream_vectors(void) {
    size_t i;

    for (i = 0u; i < test_vectors_count; ++i) {
        const struct vector_case *v;
        unsigned char encoded[128];
        unsigned char decoded[64];
        qoi89_desc decoded_desc;
        size_t encoded_len;
        size_t decoded_len;
        qoi89_status st;

        v = &test_vectors[i];
        decoded_desc.width = 0ul;
        decoded_desc.height = 0ul;
        decoded_desc.channels = 0u;
        decoded_desc.colorspace = 0u;

        st = stream_encode_all(v->pixels, v->pixels_len, &v->desc, 1u, 2u, encoded, sizeof(encoded), &encoded_len);
        test_expect_status(v->name, st, QOI89_OK);
        if (encoded_len != v->qoi_len) {
            test_fail(v->name, "stream encoded length mismatch");
        }
        test_expect_mem(v->name, encoded, v->qoi, encoded_len);

        st = stream_decode_all(v->qoi, v->qoi_len, 0u, 2u, 3u, decoded, sizeof(decoded), &decoded_desc, &decoded_len);
        test_expect_status(v->name, st, QOI89_OK);
        if (decoded_len != v->pixels_len) {
            test_fail(v->name, "stream decoded length mismatch");
        }
        test_expect_mem(v->name, decoded, v->pixels, decoded_len);
        if (decoded_desc.width != v->desc.width || decoded_desc.height != v->desc.height ||
            decoded_desc.channels != v->desc.channels || decoded_desc.colorspace != v->desc.colorspace) {
            test_fail(v->name, "stream decoded descriptor mismatch");
        }
    }
}

static void test_stream_roundtrip(void) {
    unsigned long case_id;

    for (case_id = 0ul; case_id < 300ul; ++case_id) {
        qoi89_desc desc;
        qoi89_desc decoded_desc;
        size_t pixels_len;
        size_t qoi_cap;
        unsigned char pixels[2048];
        unsigned char encoded[8192];
        unsigned char decoded[2048];
        size_t encoded_len;
        size_t decoded_len;
        qoi89_status st;

        desc.width = 1ul + (case_id % 23ul);
        desc.height = 1ul + ((case_id / 23ul) % 13ul);
        desc.channels = (unsigned char)((case_id & 1ul) ? 4u : 3u);
        desc.colorspace = (unsigned char)((case_id >> 1) & 1ul);

        st = qoi89_decoded_size(&desc, 0u, &pixels_len);
        test_expect_status("stream:decoded_size", st, QOI89_OK);
        st = qoi89_max_encoded_size(&desc, &qoi_cap);
        test_expect_status("stream:max_encoded_size", st, QOI89_OK);
        if (pixels_len > sizeof(pixels) || qoi_cap > sizeof(encoded)) {
            test_fail("stream", "fixed test buffers too small");
        }

        test_fill_pattern(pixels, pixels_len, (unsigned int)desc.channels, case_id + 5000ul);

        st = stream_encode_all(
            pixels,
            pixels_len,
            &desc,
            (unsigned int)(1u + (case_id % 7ul)),
            (unsigned int)(1u + ((case_id / 7ul) % 5ul)),
            encoded,
            sizeof(encoded),
            &encoded_len
        );
        test_expect_status("stream:encode", st, QOI89_OK);

        decoded_desc.width = 0ul;
        decoded_desc.height = 0ul;
        decoded_desc.channels = 0u;
        decoded_desc.colorspace = 0u;
        st = stream_decode_all(
            encoded,
            encoded_len,
            0u,
            (unsigned int)(1u + ((case_id / 11ul) % 9ul)),
            (unsigned int)(1u + ((case_id / 17ul) % 7ul)),
            decoded,
            sizeof(decoded),
            &decoded_desc,
            &decoded_len
        );
        test_expect_status("stream:decode", st, QOI89_OK);

        if (decoded_len != pixels_len) {
            test_fail("stream", "roundtrip decoded length mismatch");
        }
        if (decoded_desc.width != desc.width || decoded_desc.height != desc.height ||
            decoded_desc.channels != desc.channels || decoded_desc.colorspace != desc.colorspace) {
            test_fail("stream", "roundtrip decoded descriptor mismatch");
        }
        test_expect_mem("stream", decoded, pixels, pixels_len);
    }
}

static void test_stream_finish_errors(void) {
    qoi89_desc desc;
    qoi89_encode_stream enc;
    qoi89_decode_stream dec;
    unsigned char scratch[64];
    size_t written;
    qoi89_status st;

    desc.width = 2ul;
    desc.height = 1ul;
    desc.channels = 4u;
    desc.colorspace = 0u;

    st = qoi89_encode_stream_init(&enc, &desc);
    test_expect_status("stream:init-enc", st, QOI89_OK);
    st = qoi89_encode_stream_finish(&enc, scratch, sizeof(scratch), &written);
    test_expect_status("stream:enc-finish-early", st, QOI89_ERR_BAD_ARGUMENT);

    st = qoi89_decode_stream_init(&dec, 0u);
    test_expect_status("stream:init-dec", st, QOI89_OK);
    st = qoi89_decode_stream_finish(&dec);
    test_expect_status("stream:dec-finish-early", st, QOI89_ERR_TRUNCATED);
}

static void test_stream_decode_truncated(void) {
    unsigned char decoded[64];
    qoi89_desc desc;
    size_t decoded_len;
    qoi89_status st;
    qoi89_decode_stream stream;
    size_t used;
    size_t written;

    desc.width = 0ul;
    desc.height = 0ul;
    desc.channels = 0u;
    desc.colorspace = 0u;

    st = qoi89_decode_stream_init(&stream, 0u);
    test_expect_status("stream:dec-init", st, QOI89_OK);

    st = qoi89_decode_stream_process(
        &stream,
        test_vectors[1].qoi,
        test_vectors[1].qoi_len - 1u,
        &used,
        decoded,
        sizeof(decoded),
        &written
    );
    if (st != QOI89_STREAM_NEED_INPUT && st != QOI89_STREAM_NEED_OUTPUT) {
        test_fail("stream:truncated", "unexpected status before finish");
    }

    (void)desc;
    decoded_len = written;
    (void)decoded_len;

    st = qoi89_decode_stream_finish(&stream);
    test_expect_status("stream:dec-finish-truncated", st, QOI89_ERR_TRUNCATED);
}

int main(void) {
    test_stream_vectors();
    test_stream_roundtrip();
    test_stream_finish_errors();
    test_stream_decode_truncated();
    printf("All qoi89 streaming tests passed.\n");
    return 0;
}
