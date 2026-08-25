#include "psd89/psd89.h"

#include <string.h>

typedef struct psd89_vpt {
    psd89_fx16 x;
    psd89_fx16 y;
} psd89_vpt;

static psd89_fx16 fx16_abs(psd89_fx16 v)
{
    return v < 0 ? (psd89_fx16)(-v) : v;
}

static psd89_fx16 fx16_mul_local(psd89_fx16 a, psd89_fx16 b)
{
    return psd89_fx16_mul(a, b);
}

static psd89_fx16 fx16_half(psd89_fx16 a, psd89_fx16 b)
{
    return (psd89_fx16)((a + b) >> 1);
}

static psd89_fx16 fx16_from_px_center(psd89_s32 px)
{
    return (psd89_fx16)(((psd89_s32)px << 16) + 32768);
}

static psd89_fx16 fx24_rel_to_px_q16(psd89_fx24 rel, psd89_u32 dim)
{
    psd89_fx24 mag;
    psd89_s32 sign;
    psd89_s32 int_part;
    psd89_u32 frac_part;
    psd89_s32 whole;
    psd89_s32 frac;

    if (rel == 0) {
        return 0;
    }
    sign = rel < 0 ? -1 : 1;
    mag = rel < 0 ? (psd89_fx24)(-rel) : rel;
    int_part = (psd89_s32)(mag >> 24);
    frac_part = (psd89_u32)(mag & 0x00FFFFFF);

    whole = (psd89_s32)((psd89_s32)int_part * (psd89_s32)dim * 65536);
    frac = (psd89_s32)(((psd89_u32)frac_part * (psd89_u32)dim + 128U) >> 8);

    if (sign < 0) {
        return (psd89_fx16)(-(whole + frac));
    }
    return (psd89_fx16)(whole + frac);
}

static psd89_vpt knot_anchor_px(const psd89_vector_knot *k, psd89_u32 doc_width, psd89_u32 doc_height)
{
    psd89_vpt p;
    p.x = fx24_rel_to_px_q16(k->anchor_h, doc_width);
    p.y = fx24_rel_to_px_q16(k->anchor_v, doc_height);
    return p;
}

static psd89_vpt knot_prev_px(const psd89_vector_knot *k, psd89_u32 doc_width, psd89_u32 doc_height)
{
    psd89_vpt p;
    p.x = fx24_rel_to_px_q16(k->preceding_h, doc_width);
    p.y = fx24_rel_to_px_q16(k->preceding_v, doc_height);
    return p;
}

static psd89_vpt knot_next_px(const psd89_vector_knot *k, psd89_u32 doc_width, psd89_u32 doc_height)
{
    psd89_vpt p;
    p.x = fx24_rel_to_px_q16(k->leaving_h, doc_width);
    p.y = fx24_rel_to_px_q16(k->leaving_v, doc_height);
    return p;
}

static int emit_segment(psd89_vector_flatten_result *out, psd89_vpt a, psd89_vpt b)
{
    if (out == 0 || out->segment_count >= PSD89_MAX_VECTOR_SEGMENTS) {
        return 0;
    }
    out->segments[out->segment_count].x0 = a.x;
    out->segments[out->segment_count].y0 = a.y;
    out->segments[out->segment_count].x1 = b.x;
    out->segments[out->segment_count].y1 = b.y;
    out->segment_count = (psd89_u16)(out->segment_count + 1U);
    return 1;
}

static psd89_fx16 point_line_deviation(psd89_vpt a, psd89_vpt b, psd89_vpt p)
{
    psd89_fx16 abx;
    psd89_fx16 aby;
    psd89_fx16 apx;
    psd89_fx16 apy;
    psd89_fx16 cross;

    abx = (psd89_fx16)(b.x - a.x);
    aby = (psd89_fx16)(b.y - a.y);
    apx = (psd89_fx16)(p.x - a.x);
    apy = (psd89_fx16)(p.y - a.y);

    cross = (psd89_fx16)(fx16_mul_local(abx, apy) - fx16_mul_local(aby, apx));
    return fx16_abs(cross);
}

static int cubic_flat_enough(psd89_vpt p0, psd89_vpt p1, psd89_vpt p2, psd89_vpt p3, psd89_fx16 flatness)
{
    psd89_fx16 d1;
    psd89_fx16 d2;

    d1 = point_line_deviation(p0, p3, p1);
    d2 = point_line_deviation(p0, p3, p2);
    return d1 <= flatness && d2 <= flatness;
}

static int flatten_cubic_recursive(psd89_vpt p0,
                                   psd89_vpt p1,
                                   psd89_vpt p2,
                                   psd89_vpt p3,
                                   const psd89_vector_flatten_options *opt,
                                   psd89_u16 depth,
                                   psd89_vector_flatten_result *out)
{
    psd89_vpt q0;
    psd89_vpt q1;
    psd89_vpt q2;
    psd89_vpt r0;
    psd89_vpt r1;
    psd89_vpt s;

    if (out == 0 || opt == 0) {
        return 0;
    }
    if (depth >= opt->max_depth || cubic_flat_enough(p0, p1, p2, p3, opt->flatness)) {
        return emit_segment(out, p0, p3);
    }

    q0.x = fx16_half(p0.x, p1.x);
    q0.y = fx16_half(p0.y, p1.y);
    q1.x = fx16_half(p1.x, p2.x);
    q1.y = fx16_half(p1.y, p2.y);
    q2.x = fx16_half(p2.x, p3.x);
    q2.y = fx16_half(p2.y, p3.y);

    r0.x = fx16_half(q0.x, q1.x);
    r0.y = fx16_half(q0.y, q1.y);
    r1.x = fx16_half(q1.x, q2.x);
    r1.y = fx16_half(q1.y, q2.y);

    s.x = fx16_half(r0.x, r1.x);
    s.y = fx16_half(r0.y, r1.y);

    if (!flatten_cubic_recursive(p0, q0, r0, s, opt, (psd89_u16)(depth + 1U), out)) {
        return 0;
    }
    return flatten_cubic_recursive(s, r1, q2, p3, opt, (psd89_u16)(depth + 1U), out);
}

void psd89_vector_flatten_options_init(psd89_vector_flatten_options *opt)
{
    if (opt == 0) {
        return;
    }
    opt->flatness = (psd89_fx16)(1 << 16);
    opt->max_depth = 8U;
    opt->honor_handles = 1;
}

void psd89_vector_flatten_result_init(psd89_vector_flatten_result *out)
{
    if (out == 0) {
        return;
    }
    memset(out, 0, sizeof(*out));
}

static int flatten_subpath(psd89_u32 doc_width,
                           psd89_u32 doc_height,
                           const psd89_vector_mask *mask,
                           const psd89_vector_subpath *sp,
                           const psd89_vector_flatten_options *opt,
                           psd89_vector_flatten_result *out)
{
    psd89_u16 i;
    psd89_u16 count;

    if (mask == 0 || sp == 0 || opt == 0 || out == 0) {
        return 0;
    }
    count = sp->knot_count;
    if (count == 0U) {
        return 1;
    }

    for (i = 0U; i + 1U < count; ++i) {
        const psd89_vector_knot *k0;
        const psd89_vector_knot *k1;
        psd89_vpt p0, p1, p2, p3;

        k0 = &mask->knots[sp->first_knot + i];
        k1 = &mask->knots[sp->first_knot + i + 1U];
        p0 = knot_anchor_px(k0, doc_width, doc_height);
        p1 = opt->honor_handles ? knot_next_px(k0, doc_width, doc_height) : p0;
        p2 = opt->honor_handles ? knot_prev_px(k1, doc_width, doc_height) : knot_anchor_px(k1, doc_width, doc_height);
        p3 = knot_anchor_px(k1, doc_width, doc_height);
        if (!flatten_cubic_recursive(p0, p1, p2, p3, opt, 0U, out)) {
            return 0;
        }
    }

    if (sp->closed && count > 1U) {
        const psd89_vector_knot *k0;
        const psd89_vector_knot *k1;
        psd89_vpt p0, p1, p2, p3;

        k0 = &mask->knots[sp->first_knot + count - 1U];
        k1 = &mask->knots[sp->first_knot];
        p0 = knot_anchor_px(k0, doc_width, doc_height);
        p1 = opt->honor_handles ? knot_next_px(k0, doc_width, doc_height) : p0;
        p2 = opt->honor_handles ? knot_prev_px(k1, doc_width, doc_height) : knot_anchor_px(k1, doc_width, doc_height);
        p3 = knot_anchor_px(k1, doc_width, doc_height);
        if (!flatten_cubic_recursive(p0, p1, p2, p3, opt, 0U, out)) {
            return 0;
        }
    }

    return 1;
}

int psd89_vector_flatten_mask(psd89_u32 doc_width,
                              psd89_u32 doc_height,
                              const psd89_vector_mask *mask,
                              const psd89_vector_flatten_options *opt,
                              psd89_vector_flatten_result *out)
{
    psd89_vector_flatten_options local_opt;
    psd89_u16 i;

    if (out == 0 || mask == 0 || !mask->present) {
        return PSD89_E_BAD_ARGUMENT;
    }
    if (doc_width == 0U || doc_height == 0U) {
        return PSD89_E_BAD_ARGUMENT;
    }
    psd89_vector_flatten_result_init(out);
    if (opt == 0) {
        psd89_vector_flatten_options_init(&local_opt);
        opt = &local_opt;
    }
    for (i = 0U; i < mask->subpath_count; ++i) {
        if (!flatten_subpath(doc_width, doc_height, mask, &mask->subpaths[i], opt, out)) {
            return PSD89_E_LIMIT;
        }
    }
    return PSD89_OK;
}

static int segment_crosses_ray(psd89_vector_segment seg, psd89_fx16 px, psd89_fx16 py)
{
    psd89_fx16 y0, y1;
    psd89_fx16 x0, x1;
    psd89_fx16 xhit;
    psd89_fx16 dy;
    psd89_fx16 num;
    psd89_fx16 t;

    x0 = seg.x0;
    y0 = seg.y0;
    x1 = seg.x1;
    y1 = seg.y1;

    if (y0 == y1) {
        return 0;
    }
    if (y0 > y1) {
        psd89_fx16 tx;
        tx = x0; x0 = x1; x1 = tx;
        tx = y0; y0 = y1; y1 = tx;
    }
    if (py < y0 || py >= y1) {
        return 0;
    }

    dy = (psd89_fx16)(y1 - y0);
    num = (psd89_fx16)(py - y0);
    t = dy == 0 ? 0 : psd89_fx16_div(num, dy);
    xhit = (psd89_fx16)(x0 + fx16_mul_local((psd89_fx16)(x1 - x0), t));
    return xhit > px;
}

int psd89_vector_contains_point_px(const psd89_vector_mask *mask,
                                   const psd89_vector_flatten_result *flat,
                                   psd89_s32 px,
                                   psd89_s32 py)
{
    unsigned int i;
    int inside;
    psd89_fx16 qx;
    psd89_fx16 qy;

    if (mask == 0 || flat == 0 || !mask->present || mask->disabled) {
        return 0;
    }

    qx = fx16_from_px_center(px);
    qy = fx16_from_px_center(py);

    inside = (mask->initial_fill_rule_present && mask->initial_fill_rule) ? 1 : 0;
    for (i = 0U; i < flat->segment_count; ++i) {
        if (segment_crosses_ray(flat->segments[i], qx, qy)) {
            inside ^= 1;
        }
    }
    if (mask->invert) {
        inside ^= 1;
    }
    return inside;
}

psd89_u8 psd89_vector_coverage_u8(const psd89_vector_mask *mask,
                                  const psd89_vector_flatten_result *flat,
                                  psd89_s32 px,
                                  psd89_s32 py)
{
    return psd89_vector_contains_point_px(mask, flat, px, py) ? 255U : 0U;
}
