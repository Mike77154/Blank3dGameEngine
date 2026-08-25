#include "psd89/psd89.h"

#include <string.h>

static int psd89_memio_read_fn(void *user, void *dst, psd89_u32 size)
{
    psd89_memio *m;
    if (user == 0 || dst == 0) {
        return 0;
    }
    m = (psd89_memio *)user;
    if (m->pos + size > m->size) {
        return 0;
    }
    memcpy(dst, m->data + m->pos, (size_t)size);
    m->pos += size;
    return 1;
}

static int psd89_memio_write_fn(void *user, const void *src, psd89_u32 size)
{
    psd89_memio *m;
    if (user == 0 || src == 0) {
        return 0;
    }
    m = (psd89_memio *)user;
    if (!m->writable || m->pos + size > m->capacity) {
        return 0;
    }
    memcpy(m->data + m->pos, src, (size_t)size);
    m->pos += size;
    if (m->pos > m->size) {
        m->size = m->pos;
    }
    return 1;
}

static int psd89_memio_seek_fn(void *user, psd89_u32 offset)
{
    psd89_memio *m;
    if (user == 0) {
        return 0;
    }
    m = (psd89_memio *)user;
    if (offset == 0xFFFFFFFFU) {
        m->pos = m->writable ? m->size : m->size;
        return 1;
    }
    if (offset > (m->writable ? m->capacity : m->size)) {
        return 0;
    }
    m->pos = offset;
    if (m->writable && m->pos > m->size) {
        m->size = m->pos;
    }
    return 1;
}

static psd89_u32 psd89_memio_tell_fn(void *user)
{
    psd89_memio *m;
    if (user == 0) {
        return 0U;
    }
    m = (psd89_memio *)user;
    return m->pos;
}

void psd89_doc_init(psd89_doc *doc)
{
    if (doc == 0) {
        return;
    }
    memset(doc, 0, sizeof(*doc));
    doc->depth = 8U;
    doc->composite_write_compression = PSD89_COMP_RAW;
}

const char *psd89_error_string(int code)
{
    switch (code) {
    case PSD89_OK: return "ok";
    case PSD89_E_IO: return "io error";
    case PSD89_E_BAD_SIGNATURE: return "bad signature";
    case PSD89_E_BAD_VERSION: return "bad version";
    case PSD89_E_BAD_RESERVED: return "bad reserved bytes";
    case PSD89_E_UNSUPPORTED_DEPTH: return "unsupported depth";
    case PSD89_E_UNSUPPORTED_MODE: return "unsupported mode";
    case PSD89_E_UNSUPPORTED_CHANNELS: return "unsupported channels";
    case PSD89_E_UNSUPPORTED_COMPRESSION: return "unsupported compression";
    case PSD89_E_TRUNCATED: return "truncated";
    case PSD89_E_LIMIT: return "limit exceeded";
    case PSD89_E_OVERFLOW: return "overflow";
    case PSD89_E_BAD_RESOURCE: return "bad resource block";
    case PSD89_E_BAD_LAYER: return "bad layer block";
    case PSD89_E_BAD_ARGUMENT: return "bad argument";
    case PSD89_E_ROW_TOO_LARGE: return "row too large";
    case PSD89_E_BAD_STATE: return "bad state";
    case PSD89_E_RLE: return "rle error";
    default: return "unknown error";
    }
}

psd89_fx16 psd89_fx16_from_int(int v)
{
    return (psd89_fx16)((psd89_s32)v << 16);
}

int psd89_fx16_to_int(psd89_fx16 v)
{
    if (v >= 0) {
        return (int)((v + 0x8000) >> 16);
    }
    return (int)(-(((-v) + 0x8000) >> 16));
}

typedef struct psd89_pair32 {
    psd89_u32 hi;
    psd89_u32 lo;
} psd89_pair32;

static psd89_u32 psd89_abs_s32_u32(psd89_s32 v)
{
    psd89_u32 u;
    u = (psd89_u32)v;
    if (v < 0) {
        u = (psd89_u32)(0U - u);
    }
    return u;
}

static psd89_pair32 psd89_mul_u32_pair(psd89_u32 a, psd89_u32 b)
{
    psd89_u32 a0;
    psd89_u32 a1;
    psd89_u32 b0;
    psd89_u32 b1;
    psd89_u32 p0;
    psd89_u32 p1;
    psd89_u32 p2;
    psd89_u32 p3;
    psd89_u32 middle;
    psd89_pair32 r;

    a0 = a & 0xFFFFU;
    a1 = a >> 16;
    b0 = b & 0xFFFFU;
    b1 = b >> 16;
    p0 = a0 * b0;
    p1 = a0 * b1;
    p2 = a1 * b0;
    p3 = a1 * b1;
    middle = (p0 >> 16) + (p1 & 0xFFFFU) + (p2 & 0xFFFFU);
    r.lo = (p0 & 0xFFFFU) | (middle << 16);
    r.hi = p3 + (p1 >> 16) + (p2 >> 16) + (middle >> 16);
    return r;
}

static void psd89_pair_add_u32(psd89_pair32 *v, psd89_u32 add)
{
    psd89_u32 old;
    old = v->lo;
    v->lo += add;
    if (v->lo < old) {
        v->hi += 1U;
    }
}

static psd89_u32 psd89_pair_shr_low32(psd89_pair32 v, unsigned int shift)
{
    if (shift == 0U) {
        return v.lo;
    }
    if (shift < 32U) {
        return (v.hi << (32U - shift)) | (v.lo >> shift);
    }
    if (shift < 64U) {
        return v.hi >> (shift - 32U);
    }
    return 0U;
}

static psd89_pair32 psd89_pair_from_u32_shift16(psd89_u32 v)
{
    psd89_pair32 r;
    r.hi = v >> 16;
    r.lo = v << 16;
    return r;
}

static psd89_pair32 psd89_pair_div_u32(psd89_pair32 n, psd89_u32 d)
{
    psd89_pair32 q;
    psd89_u32 rem;
    unsigned int i;
    psd89_u32 bit;
    psd89_u32 carry;
    int qbit;

    q.hi = 0U;
    q.lo = 0U;
    if (d == 0U) {
        return q;
    }
    rem = 0U;
    for (i = 0U; i < 64U; ++i) {
        if (i < 32U) {
            bit = (n.hi >> (31U - i)) & 1U;
        } else {
            bit = (n.lo >> (63U - i)) & 1U;
        }
        qbit = 0;
        if (rem >= (d >> 1) + (d & 1U)) {
            rem = (psd89_u32)(rem - ((d >> 1) + (d & 1U)));
            rem = (psd89_u32)((rem << 1) + bit);
            rem = (psd89_u32)(rem + (d & 1U));
            qbit = 1;
        } else {
            rem = (psd89_u32)((rem << 1) | bit);
            if (rem >= d) {
                rem -= d;
                qbit = 1;
            }
        }
        carry = q.lo >> 31;
        q.lo <<= 1;
        q.hi = (q.hi << 1) | carry;
        if (qbit) {
            q.lo |= 1U;
        }
    }
    return q;
}

psd89_fx16 psd89_fx16_mul(psd89_fx16 a, psd89_fx16 b)
{
    psd89_pair32 p;
    psd89_u32 mag;
    int negative;

    negative = (a < 0) != (b < 0);
    p = psd89_mul_u32_pair(psd89_abs_s32_u32(a), psd89_abs_s32_u32(b));
    psd89_pair_add_u32(&p, 0x00008000U);
    mag = psd89_pair_shr_low32(p, 16U);
    if ((p.hi >> 16) != 0U || (!negative && mag > 0x7FFFFFFFU) ||
        (negative && mag > 0x80000000U)) {
        return (psd89_fx16)(-2147483647 - 1);
    }
    if (negative) {
        return (psd89_fx16)(psd89_s32)(0U - mag);
    }
    return (psd89_fx16)(psd89_s32)mag;
}

psd89_fx16 psd89_fx16_div(psd89_fx16 a, psd89_fx16 b)
{
    psd89_pair32 n;
    psd89_u32 den;
    psd89_u32 mag;
    psd89_pair32 q;
    int negative;

    if (b == 0) {
        return 0;
    }
    negative = (a < 0) != (b < 0);
    den = psd89_abs_s32_u32(b);
    n = psd89_pair_from_u32_shift16(psd89_abs_s32_u32(a));
    q = psd89_pair_div_u32(n, den);
    mag = q.lo;
    if (q.hi != 0U || (!negative && mag > 0x7FFFFFFFU) ||
        (negative && mag > 0x80000000U)) {
        return (psd89_fx16)(-2147483647 - 1);
    }
    if (negative) {
        return (psd89_fx16)(psd89_s32)(0U - mag);
    }
    return (psd89_fx16)(psd89_s32)mag;
}

psd89_fx24 psd89_fx24_from_int(int v)
{
    return (psd89_fx24)((psd89_s32)v << 24);
}

int psd89_fx24_to_int(psd89_fx24 v)
{
    if (v >= 0) {
        return (int)((v + 0x800000) >> 24);
    }
    return (int)(-(((-v) + 0x800000) >> 24));
}

psd89_fx24 psd89_fx24_mul(psd89_fx24 a, psd89_fx24 b)
{
    psd89_pair32 p;
    psd89_u32 mag;
    int negative;

    negative = (a < 0) != (b < 0);
    p = psd89_mul_u32_pair(psd89_abs_s32_u32(a), psd89_abs_s32_u32(b));
    mag = psd89_pair_shr_low32(p, 24U);
    if ((p.hi >> 24) != 0U || (!negative && mag > 0x7FFFFFFFU) ||
        (negative && mag > 0x80000000U)) {
        return (psd89_fx24)(-2147483647 - 1);
    }
    if (negative) {
        return (psd89_fx24)(psd89_s32)(0U - mag);
    }
    return (psd89_fx24)(psd89_s32)mag;
}

void psd89_memio_init_read(psd89_memio *m, const void *data, psd89_u32 size)
{
    if (m == 0) {
        return;
    }
    m->data = (psd89_u8 *)data;
    m->size = size;
    m->capacity = size;
    m->pos = 0U;
    m->writable = 0;
}

void psd89_memio_init_write(psd89_memio *m, void *data, psd89_u32 capacity)
{
    if (m == 0) {
        return;
    }
    m->data = (psd89_u8 *)data;
    m->size = 0U;
    m->capacity = capacity;
    m->pos = 0U;
    m->writable = 1;
}

void psd89_memio_make_io(psd89_memio *m, psd89_io *io)
{
    if (io == 0) {
        return;
    }
    io->read = psd89_memio_read_fn;
    io->write = psd89_memio_write_fn;
    io->seek = psd89_memio_seek_fn;
    io->tell = psd89_memio_tell_fn;
    io->user = m;
}
