#include <stdio.h>
#include <string.h>

#include "test_common.h"

#define QOI_NO_STDIO

#define TEST_REF_ARENA_SIZE 65536u

static unsigned char test_ref_arena[TEST_REF_ARENA_SIZE];
static size_t test_ref_arena_pos;

static void *test_ref_alloc(size_t sz) {
    void *ptr;
    size_t aligned;

    aligned = (sz + 7u) & ~(size_t)7u;
    if (test_ref_arena_pos > TEST_REF_ARENA_SIZE || aligned > TEST_REF_ARENA_SIZE - test_ref_arena_pos) {
        return NULL;
    }
    ptr = test_ref_arena + test_ref_arena_pos;
    test_ref_arena_pos += aligned;
    return ptr;
}

static void test_ref_reset(void) {
    test_ref_arena_pos = 0u;
}

#define QOI_MALLOC(sz) test_ref_alloc((size_t)(sz))
#define QOI_FREE(p) ((void)(p))
#define QOI_IMPLEMENTATION
#include "qoi_ref.h"


static size_t test_ref_min_size(size_t a, size_t b) {
    return a < b ? a : b;
}

static qoi89_status test_ref_stream_encode(
    const unsigned char *pixels,
    size_t pixels_len,
    const qoi89_desc *desc,
    unsigned char *out,
    size_t out_cap,
    size_t *out_len
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
        size_t used;
        size_t written;
        size_t src_chunk;
        size_t dst_chunk;

        if (++guard > 200000ul) {
            return QOI89_ERR_STATE;
        }

        src_chunk = test_ref_min_size(pixels_len - in_pos, 3u);
        dst_chunk = test_ref_min_size(out_cap - out_pos, 2u);
        st = qoi89_encode_stream_process(
            &stream,
            pixels + in_pos,
            src_chunk,
            &used,
            out + out_pos,
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
        size_t written;
        size_t dst_chunk;

        if (++guard > 300000ul) {
            return QOI89_ERR_STATE;
        }

        dst_chunk = test_ref_min_size(out_cap - out_pos, 2u);
        st = qoi89_encode_stream_finish(&stream, out + out_pos, dst_chunk, &written);
        out_pos += written;
        if (st == QOI89_STREAM_NEED_OUTPUT) {
            continue;
        }
        if (st == QOI89_STREAM_FINISHED) {
            *out_len = out_pos;
            return QOI89_OK;
        }
        return st;
    }
}

static void test_ref_vectors(void) {
    size_t i;

    for (i = 0u; i < test_vectors_count; ++i) {
        const struct vector_case *v;
        int ref_len;
        void *ref_bytes;
        qoi_desc ref_desc;

        v = &test_vectors[i];
        ref_desc.width = (unsigned int)v->desc.width;
        ref_desc.height = (unsigned int)v->desc.height;
        ref_desc.channels = v->desc.channels;
        ref_desc.colorspace = v->desc.colorspace;

        test_ref_reset();
        ref_len = 0;
        ref_bytes = qoi_encode(v->pixels, &ref_desc, &ref_len);
        if (ref_bytes == NULL) {
            test_fail(v->name, "reference encode failed");
        }
        if ((size_t)ref_len != v->qoi_len) {
            test_fail(v->name, "reference encoded length mismatch");
        }
        test_expect_mem(v->name, ref_bytes, v->qoi, v->qoi_len);
    }
}

static void test_ref_differential(void) {
    unsigned long case_id;

    for (case_id = 0ul; case_id < 1000ul; ++case_id) {
        qoi89_desc desc;
        qoi_desc ref_desc;
        size_t pixels_len;
        size_t max_qoi_len;
        unsigned char pixels[4096];
        unsigned char ours[16384];
        unsigned char ref_copy[16384];
        qoi89_desc ours_dec_desc;
        qoi_desc ref_dec_desc;
        size_t ours_len;
        int ref_len;
        void *ref_bytes;
        unsigned char *ref_decoded;
        unsigned char ours_decoded[4096];
        size_t ours_decoded_len;
        qoi89_status st;

        desc.width = 1ul + (case_id % 31ul);
        desc.height = 1ul + ((case_id / 31ul) % 17ul);
        desc.channels = (unsigned char)((case_id & 1ul) ? 4u : 3u);
        desc.colorspace = (unsigned char)((case_id >> 1) & 1ul);

        st = qoi89_decoded_size(&desc, 0u, &pixels_len);
        test_expect_status("ref:size", st, QOI89_OK);
        st = qoi89_max_encoded_size(&desc, &max_qoi_len);
        test_expect_status("ref:max", st, QOI89_OK);

        if (pixels_len > sizeof(pixels) || max_qoi_len > sizeof(ours)) {
            test_fail("ref", "fixed differential buffers too small");
        }

        test_fill_pattern(pixels, pixels_len, (unsigned int)desc.channels, case_id + 1000ul);

        st = qoi89_encode(pixels, pixels_len, &desc, ours, max_qoi_len, &ours_len);
        test_expect_status("ref:encode", st, QOI89_OK);

        ref_desc.width = (unsigned int)desc.width;
        ref_desc.height = (unsigned int)desc.height;
        ref_desc.channels = desc.channels;
        ref_desc.colorspace = desc.colorspace;

        test_ref_reset();
        ref_len = 0;
        ref_bytes = qoi_encode(pixels, &ref_desc, &ref_len);
        if (ref_bytes == NULL) {
            test_fail("ref", "reference encode failed");
        }
        if ((size_t)ref_len != ours_len) {
            test_fail("ref", "encoded length mismatch vs reference");
        }
        test_expect_mem("ref:bytes", ours, ref_bytes, ours_len);
        if ((size_t)ref_len > sizeof(ref_copy)) {
            test_fail("ref", "ref copy buffer too small");
        }
        memcpy(ref_copy, ref_bytes, (size_t)ref_len);

        test_ref_reset();
        ref_decoded = (unsigned char *)qoi_decode(ours, (int)ours_len, &ref_dec_desc, 0);
        if (ref_decoded == NULL) {
            test_fail("ref", "reference decode failed");
        }

        st = qoi89_decode(ref_copy, (size_t)ref_len, 0u, ours_decoded, sizeof(ours_decoded), &ours_dec_desc, &ours_decoded_len);
        test_expect_status("ref:decode", st, QOI89_OK);

        if (ours_decoded_len != pixels_len) {
            test_fail("ref", "decoded length mismatch");
        }
        if (ours_dec_desc.width != desc.width || ours_dec_desc.height != desc.height ||
            ours_dec_desc.channels != desc.channels || ours_dec_desc.colorspace != desc.colorspace) {
            test_fail("ref", "decoded descriptor mismatch");
        }
        if (ref_dec_desc.width != (unsigned int)desc.width || ref_dec_desc.height != (unsigned int)desc.height ||
            ref_dec_desc.channels != desc.channels || ref_dec_desc.colorspace != desc.colorspace) {
            test_fail("ref", "reference descriptor mismatch");
        }

        test_expect_mem("ref:decoded-ours", ours_decoded, pixels, pixels_len);
        test_expect_mem("ref:decoded-ref", ref_decoded, pixels, pixels_len);
    }
}

static void test_ref_stream_differential(void) {
    unsigned long case_id;

    for (case_id = 0ul; case_id < 500ul; ++case_id) {
        qoi89_desc desc;
        qoi_desc ref_desc;
        size_t pixels_len;
        size_t max_qoi_len;
        unsigned char pixels[4096];
        unsigned char ours[16384];
        size_t ours_len;
        int ref_len;
        void *ref_bytes;
        qoi89_status st;

        ours_len = 0u;

        ours_len = 0u;

        desc.width = 1ul + (case_id % 31ul);
        desc.height = 1ul + ((case_id / 31ul) % 17ul);
        desc.channels = (unsigned char)((case_id & 1ul) ? 4u : 3u);
        desc.colorspace = (unsigned char)((case_id >> 1) & 1ul);

        st = qoi89_decoded_size(&desc, 0u, &pixels_len);
        test_expect_status("ref:stream-size", st, QOI89_OK);
        st = qoi89_max_encoded_size(&desc, &max_qoi_len);
        test_expect_status("ref:stream-max", st, QOI89_OK);

        if (pixels_len > sizeof(pixels) || max_qoi_len > sizeof(ours)) {
            test_fail("ref:stream", "fixed differential buffers too small");
        }

        test_fill_pattern(pixels, pixels_len, (unsigned int)desc.channels, case_id + 9000ul);

        st = test_ref_stream_encode(pixels, pixels_len, &desc, ours, sizeof(ours), &ours_len);
        test_expect_status("ref:stream-encode", st, QOI89_OK);

        ref_desc.width = (unsigned int)desc.width;
        ref_desc.height = (unsigned int)desc.height;
        ref_desc.channels = desc.channels;
        ref_desc.colorspace = desc.colorspace;

        test_ref_reset();
        ref_len = 0;
        ref_bytes = qoi_encode(pixels, &ref_desc, &ref_len);
        if (ref_bytes == NULL) {
            test_fail("ref:stream", "reference encode failed");
        }
        if ((size_t)ref_len != ours_len) {
            test_fail("ref:stream", "encoded length mismatch vs reference");
        }
        test_expect_mem("ref:stream-bytes", ours, ref_bytes, ours_len);
    }
}

int main(void) {
    test_ref_vectors();
    test_ref_differential();
    test_ref_stream_differential();
    printf("All qoi89 reference-compat tests passed.\n");
    return 0;
}
