#include <stdio.h>
#include <string.h>

#include "qoi89.h"

struct mem_reader {
    const unsigned char *src;
    size_t len;
    size_t pos;
};

struct mem_writer {
    unsigned char *dst;
    size_t cap;
    size_t pos;
};

static qoi89_status mem_read_cb(
    void *user,
    unsigned char *dst,
    size_t dst_cap,
    size_t *dst_read
) {
    struct mem_reader *r;
    size_t remaining;
    size_t n;

    r = (struct mem_reader *)user;
    remaining = r->len - r->pos;
    n = remaining < dst_cap ? remaining : dst_cap;
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
    size_t remaining;
    size_t n;

    w = (struct mem_writer *)user;
    remaining = w->cap - w->pos;
    n = remaining < src_len ? remaining : src_len;
    if (n > 0u) {
        memcpy(w->dst + w->pos, src, n);
        w->pos += n;
    }
    *src_used = n;
    return QOI89_OK;
}

int main(void) {
    static const unsigned char pixels[] = {
        255u, 0u, 0u, 255u,
        255u, 0u, 0u, 255u,
        0u, 255u, 0u, 255u,
        0u, 0u, 255u, 255u
    };
    qoi89_desc desc;
    qoi89_desc decoded_desc;
    struct mem_reader read_pixels;
    struct mem_reader read_qoi;
    struct mem_writer write_qoi;
    struct mem_writer write_pixels;
    qoi89_io_buffers bufs;
    unsigned char input_buf[8];
    unsigned char output_buf[16];
    unsigned char qoi_data[128];
    unsigned char decoded[sizeof(pixels)];
    qoi89_status st;

    desc.width = 2ul;
    desc.height = 2ul;
    desc.channels = 4u;
    desc.colorspace = QOI89_SRGB;

    bufs.input = input_buf;
    bufs.input_cap = sizeof(input_buf);
    bufs.output = output_buf;
    bufs.output_cap = sizeof(output_buf);

    read_pixels.src = pixels;
    read_pixels.len = sizeof(pixels);
    read_pixels.pos = 0u;
    write_qoi.dst = qoi_data;
    write_qoi.cap = sizeof(qoi_data);
    write_qoi.pos = 0u;

    st = qoi89_encode_io(&desc, mem_read_cb, &read_pixels, mem_write_cb, &write_qoi, &bufs);
    if (st != QOI89_OK) {
        printf("encode_io failed: %s\n", qoi89_status_string(st));
        return 1;
    }

    read_qoi.src = qoi_data;
    read_qoi.len = write_qoi.pos;
    read_qoi.pos = 0u;
    write_pixels.dst = decoded;
    write_pixels.cap = sizeof(decoded);
    write_pixels.pos = 0u;

    st = qoi89_decode_io(0u, mem_read_cb, &read_qoi, mem_write_cb, &write_pixels, &bufs, &decoded_desc);
    if (st != QOI89_OK) {
        printf("decode_io failed: %s\n", qoi89_status_string(st));
        return 1;
    }

    if (write_pixels.pos != sizeof(pixels) || memcmp(decoded, pixels, sizeof(pixels)) != 0) {
        puts("callback IO roundtrip mismatch");
        return 1;
    }

    printf("callback IO ok, qoi bytes=%lu, decoded=%lux%lu channels=%u\n",
        (unsigned long)write_qoi.pos,
        decoded_desc.width,
        decoded_desc.height,
        (unsigned int)decoded_desc.channels);
    return 0;
}
