#ifndef QOI89_INTERNAL_H
#define QOI89_INTERNAL_H

#include "qoi89.h"

#define QOI89_OP_INDEX 0x00u
#define QOI89_OP_DIFF  0x40u
#define QOI89_OP_LUMA  0x80u
#define QOI89_OP_RUN   0xc0u
#define QOI89_OP_RGB   0xfeu
#define QOI89_OP_RGBA  0xffu
#define QOI89_MASK_2   0xc0u

#define QOI89_ENC_STATE_ACTIVE    0u
#define QOI89_ENC_STATE_FINISHING 1u
#define QOI89_ENC_STATE_DONE      2u

#define QOI89_DEC_STATE_HEADER    0u
#define QOI89_DEC_STATE_BODY      1u
#define QOI89_DEC_STATE_PADDING   2u
#define QOI89_DEC_STATE_DONE      3u

#define QOI89_DEC_PENDING_NONE    0u
#define QOI89_DEC_PENDING_LUMA    1u
#define QOI89_DEC_PENDING_RGB     2u
#define QOI89_DEC_PENDING_RGBA    3u

extern const unsigned char qoi89_padding[QOI89_PADDING_SIZE];

unsigned int qoi89_hash_rgba(const qoi89_rgba *px);
int qoi89_rgba_equal(const qoi89_rgba *a, const qoi89_rgba *b);
int qoi89_wrap_diff_u8(unsigned int cur, unsigned int prev);
void qoi89_zero_index(qoi89_rgba *index_table);
void qoi89_rgba_from_bytes(qoi89_rgba *px, const unsigned char *src, unsigned int channels);
void qoi89_rgba_to_bytes(const qoi89_rgba *px, unsigned char *dst, unsigned int channels);

void qoi89_write_u32be(unsigned char *dst, size_t *pos, unsigned long value);
unsigned long qoi89_read_u32be(const unsigned char *src, size_t *pos);

int qoi89_valid_channels(unsigned int channels);
qoi89_status qoi89_pixel_count_from_desc(
    const qoi89_desc *desc,
    unsigned long *pixel_count_out
);

int qoi89_mul_size_t(size_t a, size_t b, size_t *out);
int qoi89_add_size_t(size_t a, size_t b, size_t *out);
int qoi89_need_space(size_t pos, size_t cap, size_t need);

qoi89_status qoi89_write_header(
    const qoi89_desc *desc,
    unsigned char *dst,
    size_t dst_cap,
    size_t *pos
);

#endif /* QOI89_INTERNAL_H */
