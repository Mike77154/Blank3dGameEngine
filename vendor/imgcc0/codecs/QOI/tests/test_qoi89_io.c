#include <stdio.h>
#include <string.h>

#include "test_common.h"

struct mem_reader {
    const unsigned char *src;
    size_t len;
    size_t pos;
    unsigned int step;
};

struct mem_writer {
    unsigned char *dst;
    size_t cap;
    size_t pos;
    unsigned int step;
};

static size_t min_size(size_t a, size_t b) {
    return a < b ? a : b;
}

static qoi89_status mem_read_cb(
    void *user,
    unsigned char *dst,
    size_t dst_cap,
    size_t *dst_read
) {
    struct mem_reader *r;
    size_t avail;
    size_t n;

    r = (struct mem_reader *)user;
    avail = r->len - r->pos;
    n = min_size(avail, dst_cap);
    if (r->step > 0u) {
        n = min_size(n, (size_t)r->step);
    }

    if (n > 0u) {
        memcpy(dst, r->src + r->pos, n);
        r->pos += n;
    }

    *dst_read = n;
    return QOI89_OK;
}

static qoi89_status mem_write_cb(
    void *user,
    const unsigned char *src,
    size_t src_len,
    size_t *src_used
) {
    struct mem_writer *w;
    size_t avail;
    size_t n;

    w = (struct mem_writer *)user;
    avail = w->cap - w->pos;
    n = min_size(avail, src_len);
    if (w->step > 0u) {
        n = min_size(n, (size_t)w->step);
    }
    else {
        n = 0u;
    }

    if (n > 0u) {
        memcpy(w->dst + w->pos, src, n);
        w->pos += n;
    }

    *src_used = n;
    return QOI89_OK;
}

static void test_io_vectors(void) {
    size_t i;

    for (i = 0u; i < test_vectors_count; ++i) {
        const struct vector_case *v;
        struct mem_reader read_pixels;
        struct mem_reader read_qoi;
        struct mem_writer write_qoi;
        struct mem_writer write_pixels;
        qoi89_io_buffers buffers;
        unsigned char input_buf[5];
        unsigned char output_buf[7];
        unsigned char encoded[128];
        unsigned char decoded[64];
        qoi89_desc decoded_desc;
        qoi89_status st;

        v = &test_vectors[i];

        read_pixels.src = v->pixels;
        read_pixels.len = v->pixels_len;
        read_pixels.pos = 0u;
        read_pixels.step = 2u;
        write_qoi.dst = encoded;
        write_qoi.cap = sizeof(encoded);
        write_qoi.pos = 0u;
        write_qoi.step = 3u;
        buffers.input = input_buf;
        buffers.input_cap = sizeof(input_buf);
        buffers.output = output_buf;
        buffers.output_cap = sizeof(output_buf);

        st = qoi89_encode_io(&v->desc, mem_read_cb, &read_pixels, mem_write_cb, &write_qoi, &buffers);
        test_expect_status(v->name, st, QOI89_OK);
        if (write_qoi.pos != v->qoi_len) {
            test_fail(v->name, "io encoded length mismatch");
        }
        test_expect_mem(v->name, encoded, v->qoi, write_qoi.pos);

        read_qoi.src = v->qoi;
        read_qoi.len = v->qoi_len;
        read_qoi.pos = 0u;
        read_qoi.step = 3u;
        write_pixels.dst = decoded;
        write_pixels.cap = sizeof(decoded);
        write_pixels.pos = 0u;
        write_pixels.step = 2u;
        decoded_desc.width = 0ul;
        decoded_desc.height = 0ul;
        decoded_desc.channels = 0u;
        decoded_desc.colorspace = 0u;

        st = qoi89_decode_io(0u, mem_read_cb, &read_qoi, mem_write_cb, &write_pixels, &buffers, &decoded_desc);
        test_expect_status(v->name, st, QOI89_OK);
        if (write_pixels.pos != v->pixels_len) {
            test_fail(v->name, "io decoded length mismatch");
        }
        test_expect_mem(v->name, decoded, v->pixels, write_pixels.pos);
        if (decoded_desc.width != v->desc.width || decoded_desc.height != v->desc.height ||
            decoded_desc.channels != v->desc.channels || decoded_desc.colorspace != v->desc.colorspace) {
            test_fail(v->name, "io decoded descriptor mismatch");
        }
    }
}

static void test_io_roundtrip(void) {
    unsigned long case_id;

    for (case_id = 0ul; case_id < 220ul; ++case_id) {
        qoi89_desc desc;
        qoi89_desc decoded_desc;
        size_t pixels_len;
        size_t qoi_cap;
        unsigned char pixels[2048];
        unsigned char encoded[8192];
        unsigned char decoded[2048];
        struct mem_reader read_pixels;
        struct mem_reader read_qoi;
        struct mem_writer write_qoi;
        struct mem_writer write_pixels;
        qoi89_io_buffers buffers;
        unsigned char input_buf[11];
        unsigned char output_buf[13];
        qoi89_status st;

        desc.width = 1ul + (case_id % 17ul);
        desc.height = 1ul + ((case_id / 17ul) % 9ul);
        desc.channels = (unsigned char)((case_id & 1ul) ? 4u : 3u);
        desc.colorspace = (unsigned char)((case_id >> 1) & 1ul);

        st = qoi89_decoded_size(&desc, 0u, &pixels_len);
        test_expect_status("io:decoded_size", st, QOI89_OK);
        st = qoi89_max_encoded_size(&desc, &qoi_cap);
        test_expect_status("io:max_encoded_size", st, QOI89_OK);
        if (pixels_len > sizeof(pixels) || qoi_cap > sizeof(encoded)) {
            test_fail("io", "fixed test buffers too small");
        }

        test_fill_pattern(pixels, pixels_len, (unsigned int)desc.channels, case_id + 15000ul);

        read_pixels.src = pixels;
        read_pixels.len = pixels_len;
        read_pixels.pos = 0u;
        read_pixels.step = (unsigned int)(1u + (case_id % 7ul));
        write_qoi.dst = encoded;
        write_qoi.cap = sizeof(encoded);
        write_qoi.pos = 0u;
        write_qoi.step = (unsigned int)(1u + ((case_id / 3ul) % 5ul));
        buffers.input = input_buf;
        buffers.input_cap = sizeof(input_buf);
        buffers.output = output_buf;
        buffers.output_cap = sizeof(output_buf);

        st = qoi89_encode_io(&desc, mem_read_cb, &read_pixels, mem_write_cb, &write_qoi, &buffers);
        test_expect_status("io:encode", st, QOI89_OK);

        read_qoi.src = encoded;
        read_qoi.len = write_qoi.pos;
        read_qoi.pos = 0u;
        read_qoi.step = (unsigned int)(1u + ((case_id / 5ul) % 7ul));
        write_pixels.dst = decoded;
        write_pixels.cap = sizeof(decoded);
        write_pixels.pos = 0u;
        write_pixels.step = (unsigned int)(1u + ((case_id / 7ul) % 4ul));
        decoded_desc.width = 0ul;
        decoded_desc.height = 0ul;
        decoded_desc.channels = 0u;
        decoded_desc.colorspace = 0u;

        st = qoi89_decode_io(0u, mem_read_cb, &read_qoi, mem_write_cb, &write_pixels, &buffers, &decoded_desc);
        test_expect_status("io:decode", st, QOI89_OK);
        if (write_pixels.pos != pixels_len) {
            test_fail("io:decode", "roundtrip pixel length mismatch");
        }
        test_expect_mem("io:roundtrip", decoded, pixels, pixels_len);
        if (decoded_desc.width != desc.width || decoded_desc.height != desc.height ||
            decoded_desc.channels != desc.channels || decoded_desc.colorspace != desc.colorspace) {
            test_fail("io:roundtrip", "roundtrip descriptor mismatch");
        }
    }
}

static void test_io_errors(void) {
    struct mem_reader read_pixels;
    struct mem_reader read_qoi;
    struct mem_writer stalled_writer;
    qoi89_io_buffers buffers;
    unsigned char input_buf[4];
    unsigned char output_buf[6];
    unsigned char sink[128];
    unsigned char qoi_extra[64];
    qoi89_desc desc_out;
    qoi89_status st;

    buffers.input = input_buf;
    buffers.input_cap = sizeof(input_buf);
    buffers.output = output_buf;
    buffers.output_cap = sizeof(output_buf);

    read_pixels.src = test_vectors[0].pixels;
    read_pixels.len = test_vectors[0].pixels_len;
    read_pixels.pos = 0u;
    read_pixels.step = 1u;
    stalled_writer.dst = sink;
    stalled_writer.cap = sizeof(sink);
    stalled_writer.pos = 0u;
    stalled_writer.step = 0u;

    st = qoi89_encode_io(&test_vectors[0].desc, mem_read_cb, &read_pixels, mem_write_cb, &stalled_writer, &buffers);
    test_expect_status("io:error:stall", st, QOI89_ERR_IO_STALL);

    memcpy(qoi_extra, test_vectors[1].qoi, test_vectors[1].qoi_len);
    qoi_extra[test_vectors[1].qoi_len + 0u] = 0x12u;
    qoi_extra[test_vectors[1].qoi_len + 1u] = 0x34u;

    read_qoi.src = qoi_extra;
    read_qoi.len = test_vectors[1].qoi_len + 2u;
    read_qoi.pos = 0u;
    read_qoi.step = 5u;
    stalled_writer.dst = sink;
    stalled_writer.cap = sizeof(sink);
    stalled_writer.pos = 0u;
    stalled_writer.step = 4u;

    desc_out.width = 0ul;
    desc_out.height = 0ul;
    desc_out.channels = 0u;
    desc_out.colorspace = 0u;
    st = qoi89_decode_io(0u, mem_read_cb, &read_qoi, mem_write_cb, &stalled_writer, &buffers, &desc_out);
    test_expect_status("io:error:trailing", st, QOI89_ERR_TRAILING_DATA);
}

int main(void) {
    test_io_vectors();
    test_io_roundtrip();
    test_io_errors();
    puts("All qoi89 callback-io tests passed.");
    return 0;
}
