#include <stdio.h>
#include <string.h>

#include "test_common.h"

static void test_vectors_core(void) {
    size_t i;

    for (i = 0u; i < test_vectors_count; ++i) {
        const struct vector_case *v;
        unsigned char encoded[128];
        unsigned char decoded[64];
        qoi89_desc parsed;
        qoi89_desc decoded_desc;
        size_t encoded_len;
        size_t decoded_len;
        size_t max_len;
        qoi89_status st;

        v = &test_vectors[i];
        st = qoi89_max_encoded_size(&v->desc, &max_len);
        test_expect_status(v->name, st, QOI89_OK);
        if (max_len > sizeof(encoded)) {
            test_fail(v->name, "scratch buffer too small for test");
        }

        st = qoi89_encode(v->pixels, v->pixels_len, &v->desc, encoded, sizeof(encoded), &encoded_len);
        test_expect_status(v->name, st, QOI89_OK);
        if (encoded_len != v->qoi_len) {
            test_fail(v->name, "encoded length mismatch");
        }
        test_expect_mem(v->name, encoded, v->qoi, encoded_len);

        st = qoi89_decode_header(v->qoi, v->qoi_len, &parsed);
        test_expect_status(v->name, st, QOI89_OK);
        if (parsed.width != v->desc.width || parsed.height != v->desc.height ||
            parsed.channels != v->desc.channels || parsed.colorspace != v->desc.colorspace) {
            test_fail(v->name, "header mismatch");
        }

        st = qoi89_decode(v->qoi, v->qoi_len, 0u, decoded, sizeof(decoded), &decoded_desc, &decoded_len);
        test_expect_status(v->name, st, QOI89_OK);
        if (decoded_len != v->pixels_len) {
            test_fail(v->name, "decoded length mismatch");
        }
        test_expect_mem(v->name, decoded, v->pixels, decoded_len);

        st = qoi89_decode(v->qoi, v->qoi_len, 3u, decoded, sizeof(decoded), &decoded_desc, &decoded_len);
        test_expect_status(v->name, st, QOI89_OK);
        if (decoded_len != (size_t)(v->desc.width * v->desc.height * 3ul)) {
            test_fail(v->name, "decoded length mismatch for forced RGB");
        }
    }
}

static void test_errors(void) {
    unsigned char bad_magic[27];
    unsigned char bad_padding[27];
    unsigned char truncated[] = {
        0x71u,0x6fu,0x69u,0x66u,0x00u,0x00u,0x00u,0x02u,0x00u,0x00u,0x00u,0x01u,0x04u,0x00u,
        0xfeu,0x32u,0x32u,0x32u,0xaau,
        0x00u,0x00u,0x00u,0x00u,0x00u,0x00u,0x00u,0x01u
    };
    unsigned char repeated_index[] = {
        0x71u,0x6fu,0x69u,0x66u,0x00u,0x00u,0x00u,0x04u,0x00u,0x00u,0x00u,0x01u,0x04u,0x00u,
        0xa2u,0x79u,0xa3u,0x88u,0x17u,0x17u,
        0x00u,0x00u,0x00u,0x00u,0x00u,0x00u,0x00u,0x01u
    };
    unsigned char trailing_data[] = {
        0x71u,0x6fu,0x69u,0x66u,0x00u,0x00u,0x00u,0x01u,0x00u,0x00u,0x00u,0x01u,0x04u,0x00u,
        0xffu,0x01u,0x02u,0x03u,0x04u,
        0x00u,
        0x00u,0x00u,0x00u,0x00u,0x00u,0x00u,0x00u,0x01u
    };
    unsigned char out[64];
    qoi89_desc desc;
    size_t out_len;
    qoi89_status st;

    memcpy(bad_magic, test_vectors[1].qoi, 27u);
    bad_magic[0] = 'x';
    st = qoi89_decode(bad_magic, sizeof(bad_magic), 0u, out, sizeof(out), &desc, &out_len);
    test_expect_status("bad_magic", st, QOI89_ERR_BAD_MAGIC);

    memcpy(bad_padding, test_vectors[1].qoi, 27u);
    bad_padding[sizeof(bad_padding) - 1u] = 0x02u;
    st = qoi89_decode(bad_padding, sizeof(bad_padding), 0u, out, sizeof(out), &desc, &out_len);
    test_expect_status("bad_padding", st, QOI89_ERR_BAD_PADDING);

    st = qoi89_decode(truncated, sizeof(truncated), 0u, out, sizeof(out), &desc, &out_len);
    test_expect_status("truncated", st, QOI89_ERR_TRUNCATED);

    st = qoi89_decode(repeated_index, sizeof(repeated_index), 0u, out, sizeof(out), &desc, &out_len);
    test_expect_status("repeated_index", st, QOI89_ERR_REPEATED_INDEX);

    st = qoi89_decode(trailing_data, sizeof(trailing_data), 0u, out, sizeof(out), &desc, &out_len);
    test_expect_status("trailing_data", st, QOI89_ERR_TRAILING_DATA);
}

static void test_roundtrip(void) {
    unsigned long case_id;

    for (case_id = 0ul; case_id < 250ul; ++case_id) {
        qoi89_desc desc;
        size_t pixels_len;
        size_t max_qoi_len;
        unsigned char pixels[1024];
        unsigned char encoded[4096];
        unsigned char decoded[1024];
        qoi89_desc decoded_desc;
        size_t encoded_len;
        size_t decoded_len;
        qoi89_status st;

        desc.width = 1ul + (case_id % 17ul);
        desc.height = 1ul + ((case_id / 17ul) % 11ul);
        desc.channels = (unsigned char)((case_id & 1ul) ? 4u : 3u);
        desc.colorspace = (unsigned char)(case_id & 1ul);

        st = qoi89_decoded_size(&desc, 0u, &pixels_len);
        test_expect_status("roundtrip:size", st, QOI89_OK);
        st = qoi89_max_encoded_size(&desc, &max_qoi_len);
        test_expect_status("roundtrip:max", st, QOI89_OK);

        if (pixels_len > sizeof(pixels) || max_qoi_len > sizeof(encoded)) {
            test_fail("roundtrip", "fixed test buffers too small");
        }

        test_fill_pattern(pixels, pixels_len, (unsigned int)desc.channels, case_id);

        st = qoi89_encode(pixels, pixels_len, &desc, encoded, max_qoi_len, &encoded_len);
        test_expect_status("roundtrip:encode", st, QOI89_OK);

        st = qoi89_decode(encoded, encoded_len, 0u, decoded, sizeof(decoded), &decoded_desc, &decoded_len);
        test_expect_status("roundtrip:decode", st, QOI89_OK);

        if (decoded_len != pixels_len) {
            test_fail("roundtrip", "decoded length mismatch");
        }
        if (decoded_desc.width != desc.width || decoded_desc.height != desc.height ||
            decoded_desc.channels != desc.channels || decoded_desc.colorspace != desc.colorspace) {
            test_fail("roundtrip", "decoded descriptor mismatch");
        }
        test_expect_mem("roundtrip", pixels, decoded, pixels_len);
    }
}

int main(void) {
    test_vectors_core();
    test_errors();
    test_roundtrip();
    printf("All qoi89 core tests passed.\n");
    return 0;
}
