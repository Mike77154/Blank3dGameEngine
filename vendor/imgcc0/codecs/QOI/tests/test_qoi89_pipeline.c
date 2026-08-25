#include <stdio.h>
#include <string.h>

#include "test_common.h"

static size_t min_size(size_t a, size_t b) {
    return a < b ? a : b;
}

static qoi89_status pipe_encode_all(
    const unsigned char *pixels,
    size_t pixels_len,
    const qoi89_desc *desc,
    unsigned int input_step,
    unsigned int output_step,
    unsigned char *dst,
    size_t dst_cap,
    size_t *dst_len
) {
    qoi89_encode_pipe pipe;
    unsigned char queue_storage[64];
    qoi89_status st;
    size_t in_pos;
    size_t out_pos;
    int ended;
    unsigned long guard;

    st = qoi89_encode_pipe_init(&pipe, desc, queue_storage, sizeof(queue_storage));
    if (st != QOI89_OK) {
        return st;
    }

    in_pos = 0u;
    out_pos = 0u;
    ended = 0;
    guard = 0ul;

    while (!qoi89_encode_pipe_finished(&pipe)) {
        if (++guard > 300000ul) {
            return QOI89_ERR_STATE;
        }

        if (in_pos < pixels_len) {
            size_t chunk;
            size_t used;

            chunk = min_size(pixels_len - in_pos, (size_t)input_step);
            used = 0u;
            st = qoi89_encode_pipe_push(&pipe, pixels + in_pos, chunk, &used);
            in_pos += used;
            if (st != QOI89_STREAM_NEED_INPUT &&
                st != QOI89_STREAM_NEED_OUTPUT &&
                st != QOI89_STREAM_FINISHED) {
                return st;
            }
        }
        else if (!ended) {
            st = qoi89_encode_pipe_end(&pipe);
            ended = 1;
            if (st != QOI89_STREAM_NEED_INPUT &&
                st != QOI89_STREAM_NEED_OUTPUT &&
                st != QOI89_STREAM_FINISHED) {
                return st;
            }
        }

        if (qoi89_encode_pipe_output_pending(&pipe) > 0u || ended) {
            size_t chunk;
            size_t written;

            if (out_pos == dst_cap && !qoi89_encode_pipe_finished(&pipe)) {
                return QOI89_ERR_OUTPUT_TOO_SMALL;
            }

            chunk = min_size(dst_cap - out_pos, (size_t)output_step);
            written = 0u;
            st = qoi89_encode_pipe_pull(&pipe, dst + out_pos, chunk, &written);
            out_pos += written;
            if (st != QOI89_STREAM_NEED_INPUT &&
                st != QOI89_STREAM_NEED_OUTPUT &&
                st != QOI89_STREAM_FINISHED) {
                return st;
            }
        }
    }

    *dst_len = out_pos;
    return QOI89_OK;
}

static qoi89_status pipe_decode_all(
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
    qoi89_decode_pipe pipe;
    unsigned char queue_storage[64];
    qoi89_status st;
    size_t src_pos;
    size_t out_pos;
    int ended;
    unsigned long guard;

    st = qoi89_decode_pipe_init(&pipe, out_channels, queue_storage, sizeof(queue_storage));
    if (st != QOI89_OK) {
        return st;
    }

    desc_out->width = 0ul;
    desc_out->height = 0ul;
    desc_out->channels = 0u;
    desc_out->colorspace = 0u;
    src_pos = 0u;
    out_pos = 0u;
    ended = 0;
    guard = 0ul;

    while (!qoi89_decode_pipe_finished(&pipe)) {
        if (++guard > 300000ul) {
            return QOI89_ERR_STATE;
        }

        if (src_pos < src_len) {
            size_t chunk;
            size_t used;

            chunk = min_size(src_len - src_pos, (size_t)input_step);
            used = 0u;
            st = qoi89_decode_pipe_push(&pipe, src + src_pos, chunk, &used);
            src_pos += used;
            if (st != QOI89_STREAM_NEED_INPUT &&
                st != QOI89_STREAM_NEED_OUTPUT &&
                st != QOI89_STREAM_FINISHED) {
                return st;
            }
        }
        else if (!ended) {
            st = qoi89_decode_pipe_end(&pipe);
            ended = 1;
            if (st != QOI89_STREAM_NEED_INPUT &&
                st != QOI89_STREAM_NEED_OUTPUT &&
                st != QOI89_STREAM_FINISHED) {
                return st;
            }
        }

        if (qoi89_decode_pipe_header_ready(&pipe) && desc_out->width == 0ul) {
            st = qoi89_decode_pipe_get_desc(&pipe, desc_out);
            if (st != QOI89_OK) {
                return st;
            }
        }

        if (qoi89_decode_pipe_output_pending(&pipe) > 0u) {
            size_t chunk;
            size_t written;

            if (out_pos == pixels_cap && !qoi89_decode_pipe_finished(&pipe)) {
                return QOI89_ERR_OUTPUT_TOO_SMALL;
            }

            chunk = min_size(pixels_cap - out_pos, (size_t)output_step);
            written = 0u;
            st = qoi89_decode_pipe_pull(&pipe, pixels + out_pos, chunk, &written);
            out_pos += written;
            if (st != QOI89_STREAM_NEED_INPUT &&
                st != QOI89_STREAM_NEED_OUTPUT &&
                st != QOI89_STREAM_FINISHED) {
                return st;
            }
        }
    }

    *pixels_len = out_pos;
    return QOI89_OK;
}

static void test_pipeline_vectors(void) {
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

        st = pipe_encode_all(v->pixels, v->pixels_len, &v->desc, 2u, 3u, encoded, sizeof(encoded), &encoded_len);
        test_expect_status(v->name, st, QOI89_OK);
        if (encoded_len != v->qoi_len) {
            test_fail(v->name, "pipeline encoded length mismatch");
        }
        test_expect_mem(v->name, encoded, v->qoi, encoded_len);

        st = pipe_decode_all(v->qoi, v->qoi_len, 0u, 3u, 2u, decoded, sizeof(decoded), &decoded_desc, &decoded_len);
        test_expect_status(v->name, st, QOI89_OK);
        if (decoded_len != v->pixels_len) {
            test_fail(v->name, "pipeline decoded length mismatch");
        }
        test_expect_mem(v->name, decoded, v->pixels, decoded_len);
        if (decoded_desc.width != v->desc.width || decoded_desc.height != v->desc.height ||
            decoded_desc.channels != v->desc.channels || decoded_desc.colorspace != v->desc.colorspace) {
            test_fail(v->name, "pipeline decoded descriptor mismatch");
        }
    }
}

static void test_pipeline_roundtrip(void) {
    unsigned long case_id;

    for (case_id = 0ul; case_id < 260ul; ++case_id) {
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

        desc.width = 1ul + (case_id % 19ul);
        desc.height = 1ul + ((case_id / 19ul) % 11ul);
        desc.channels = (unsigned char)((case_id & 1ul) ? 4u : 3u);
        desc.colorspace = (unsigned char)((case_id >> 1) & 1ul);

        st = qoi89_decoded_size(&desc, 0u, &pixels_len);
        test_expect_status("pipeline:decoded_size", st, QOI89_OK);
        st = qoi89_max_encoded_size(&desc, &qoi_cap);
        test_expect_status("pipeline:max_encoded_size", st, QOI89_OK);
        if (pixels_len > sizeof(pixels) || qoi_cap > sizeof(encoded)) {
            test_fail("pipeline", "fixed test buffers too small");
        }

        test_fill_pattern(pixels, pixels_len, (unsigned int)desc.channels, case_id + 9000ul);

        st = pipe_encode_all(
            pixels,
            pixels_len,
            &desc,
            (unsigned int)(1u + (case_id % 5ul)),
            (unsigned int)(1u + ((case_id / 5ul) % 7ul)),
            encoded,
            sizeof(encoded),
            &encoded_len
        );
        test_expect_status("pipeline:encode", st, QOI89_OK);

        decoded_desc.width = 0ul;
        decoded_desc.height = 0ul;
        decoded_desc.channels = 0u;
        decoded_desc.colorspace = 0u;
        st = pipe_decode_all(
            encoded,
            encoded_len,
            0u,
            (unsigned int)(1u + ((case_id / 3ul) % 6ul)),
            (unsigned int)(1u + ((case_id / 7ul) % 5ul)),
            decoded,
            sizeof(decoded),
            &decoded_desc,
            &decoded_len
        );
        test_expect_status("pipeline:decode", st, QOI89_OK);
        if (decoded_len != pixels_len) {
            test_fail("pipeline:decode", "roundtrip pixel length mismatch");
        }
        test_expect_mem("pipeline:roundtrip", decoded, pixels, pixels_len);
        if (decoded_desc.width != desc.width || decoded_desc.height != desc.height ||
            decoded_desc.channels != desc.channels || decoded_desc.colorspace != desc.colorspace) {
            test_fail("pipeline:roundtrip", "roundtrip descriptor mismatch");
        }
    }
}

static void test_pipeline_header_ready(void) {
    qoi89_decode_pipe pipe;
    unsigned char queue_storage[16];
    size_t used;
    qoi89_desc desc;
    qoi89_status st;

    st = qoi89_decode_pipe_init(&pipe, 0u, queue_storage, sizeof(queue_storage));
    test_expect_status("pipeline:header:init", st, QOI89_OK);

    used = 0u;
    st = qoi89_decode_pipe_push(&pipe, test_vectors[0].qoi, (size_t)QOI89_HEADER_SIZE, &used);
    if (st != QOI89_STREAM_NEED_INPUT && st != QOI89_STREAM_NEED_OUTPUT) {
        test_expect_status("pipeline:header:push", st, QOI89_STREAM_NEED_INPUT);
    }
    if (used != (size_t)QOI89_HEADER_SIZE) {
        test_fail("pipeline:header", "header bytes not fully consumed");
    }
    if (!qoi89_decode_pipe_header_ready(&pipe)) {
        test_fail("pipeline:header", "header not marked ready");
    }

    st = qoi89_decode_pipe_get_desc(&pipe, &desc);
    test_expect_status("pipeline:header:get_desc", st, QOI89_OK);
    if (desc.width != test_vectors[0].desc.width ||
        desc.height != test_vectors[0].desc.height ||
        desc.channels != test_vectors[0].desc.channels ||
        desc.colorspace != test_vectors[0].desc.colorspace) {
        test_fail("pipeline:header", "descriptor mismatch after header push");
    }
}

static void test_pipeline_end_errors(void) {
    qoi89_encode_pipe enc;
    qoi89_decode_pipe dec;
    unsigned char queue_a[16];
    unsigned char queue_b[16];
    size_t used;
    qoi89_status st;

    st = qoi89_encode_pipe_init(&enc, &test_vectors[4].desc, queue_a, sizeof(queue_a));
    test_expect_status("pipeline:end:enc:init", st, QOI89_OK);
    used = 0u;
    st = qoi89_encode_pipe_push(&enc, test_vectors[4].pixels, 3u, &used);
    if (st != QOI89_STREAM_NEED_INPUT && st != QOI89_STREAM_NEED_OUTPUT) {
        test_fail("pipeline:end:enc", "unexpected status while feeding partial pixel data");
    }
    st = qoi89_encode_pipe_end(&enc);
    test_expect_status("pipeline:end:enc", st, QOI89_ERR_BAD_ARGUMENT);

    st = qoi89_decode_pipe_init(&dec, 0u, queue_b, sizeof(queue_b));
    test_expect_status("pipeline:end:dec:init", st, QOI89_OK);
    st = qoi89_decode_pipe_end(&dec);
    test_expect_status("pipeline:end:dec", st, QOI89_ERR_TRUNCATED);
}

int main(void) {
    test_pipeline_vectors();
    test_pipeline_roundtrip();
    test_pipeline_header_ready();
    test_pipeline_end_errors();
    puts("All qoi89 pipeline tests passed.");
    return 0;
}
