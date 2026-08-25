#include "psd89/psd89.h"

#include <stdio.h>
#include <string.h>

static int fail(const char *msg)
{
    fprintf(stderr, "FAIL: %s\n", msg);
    return 1;
}

static psd89_fx24 fx24_frac(psd89_s32 num, psd89_s32 den)
{
    if (den == 0) {
        return 0;
    }
    return (psd89_fx24)((num / den) * PSD89_FX24_ONE +
                        ((num % den) << 24) / den);
}

static void set_knot(psd89_vector_knot *knot,
                     psd89_fx24 prev_x,
                     psd89_fx24 prev_y,
                     psd89_fx24 anchor_x,
                     psd89_fx24 anchor_y,
                     psd89_fx24 next_x,
                     psd89_fx24 next_y)
{
    memset(knot, 0, sizeof(*knot));
    knot->linked = 0U;
    knot->preceding_h = prev_x;
    knot->preceding_v = prev_y;
    knot->anchor_h = anchor_x;
    knot->anchor_v = anchor_y;
    knot->leaving_h = next_x;
    knot->leaving_v = next_y;
}

static int test_rectangle_mask(void)
{
    psd89_vector_mask vm;
    psd89_vector_flatten_options opt;
    psd89_vector_flatten_result flat;
    int rc;

    memset(&vm, 0, sizeof(vm));
    vm.present = 1U;
    vm.version = 3U;
    vm.path_fill_rule_present = 1U;
    vm.initial_fill_rule_present = 1U;
    vm.initial_fill_rule = 0U;
    vm.subpath_count = 1U;
    vm.knot_count = 4U;
    vm.subpaths[0].closed = 1U;
    vm.subpaths[0].first_knot = 0U;
    vm.subpaths[0].knot_count = 4U;

    set_knot(&vm.knots[0], fx24_frac(1, 4), fx24_frac(1, 4), fx24_frac(1, 4), fx24_frac(1, 4), fx24_frac(1, 4), fx24_frac(1, 4));
    set_knot(&vm.knots[1], fx24_frac(3, 4), fx24_frac(1, 4), fx24_frac(3, 4), fx24_frac(1, 4), fx24_frac(3, 4), fx24_frac(1, 4));
    set_knot(&vm.knots[2], fx24_frac(3, 4), fx24_frac(3, 4), fx24_frac(3, 4), fx24_frac(3, 4), fx24_frac(3, 4), fx24_frac(3, 4));
    set_knot(&vm.knots[3], fx24_frac(1, 4), fx24_frac(3, 4), fx24_frac(1, 4), fx24_frac(3, 4), fx24_frac(1, 4), fx24_frac(3, 4));

    psd89_vector_flatten_options_init(&opt);
    opt.flatness = (psd89_fx16)(1 << 8);
    psd89_vector_flatten_result_init(&flat);

    rc = psd89_vector_flatten_mask(8U, 8U, &vm, &opt, &flat);
    if (rc != PSD89_OK) {
        return fail("rectangle flatten");
    }
    if (flat.segment_count != 4U) {
        return fail("rectangle segment count");
    }
    if (!psd89_vector_contains_point_px(&vm, &flat, 3, 3)) {
        return fail("rectangle inside");
    }
    if (psd89_vector_contains_point_px(&vm, &flat, 0, 0)) {
        return fail("rectangle outside");
    }
    return 0;
}

static int test_curved_mask(void)
{
    psd89_vector_mask vm;
    psd89_vector_flatten_options opt;
    psd89_vector_flatten_result flat;
    int rc;

    memset(&vm, 0, sizeof(vm));
    vm.present = 1U;
    vm.version = 3U;
    vm.initial_fill_rule_present = 1U;
    vm.subpath_count = 1U;
    vm.knot_count = 2U;
    vm.subpaths[0].closed = 0U;
    vm.subpaths[0].first_knot = 0U;
    vm.subpaths[0].knot_count = 2U;

    /* A single cubic arch from (1,4) to (7,4) with high handles upward. */
    set_knot(&vm.knots[0],
             fx24_frac(1, 8), fx24_frac(4, 8),
             fx24_frac(1, 8), fx24_frac(4, 8),
             fx24_frac(3, 8), fx24_frac(0, 8));
    set_knot(&vm.knots[1],
             fx24_frac(5, 8), fx24_frac(0, 8),
             fx24_frac(7, 8), fx24_frac(4, 8),
             fx24_frac(7, 8), fx24_frac(4, 8));

    psd89_vector_flatten_options_init(&opt);
    opt.flatness = (psd89_fx16)(1 << 10);
    psd89_vector_flatten_result_init(&flat);

    rc = psd89_vector_flatten_mask(8U, 8U, &vm, &opt, &flat);
    if (rc != PSD89_OK) {
        return fail("curved flatten");
    }
    if (flat.segment_count <= 1U) {
        return fail("curved produced too few segments");
    }
    return 0;
}

static int test_mask_global_tags(void)
{
    psd89_u8 raw[4];
    psd89_u8 v;
    int rc;

    psd89_mask_global_bool_write(raw, 1U);
    if (raw[0] != 1U || raw[1] != 0U || raw[2] != 0U || raw[3] != 0U) {
        return fail("mask global write");
    }
    rc = psd89_mask_global_bool_parse(raw, &v);
    if (rc != PSD89_OK || v != 1U) {
        return fail("mask global parse");
    }
    return 0;
}

static int test_zip_diag(void)
{
    psd89_zip_diag diag;
    int rc;

    rc = psd89_zip_diag_check_planar(PSD89_COMP_ZIP, 4U, 5U, 3U, 11U, 60U, 0, &diag);
    if (rc != PSD89_ZIP_DIAG_OK) {
        return fail("zip diag ok");
    }
    rc = psd89_zip_diag_check_planar(PSD89_COMP_ZIP_PRED, 4U, 5U, 3U, 11U, 59U, 0, &diag);
    if (rc != PSD89_ZIP_DIAG_BAD_PREDICTED_SIZE) {
        return fail("zip diag predicted mismatch");
    }
    rc = psd89_zip_diag_check_planar(PSD89_COMP_ZIP, 4U, 5U, 3U, 11U, 60U, -5, &diag);
    if (rc != PSD89_ZIP_DIAG_ZLIB_STATUS) {
        return fail("zip diag zlib status");
    }
    return 0;
}

int main(void)
{
    if (test_rectangle_mask() != 0) {
        return 1;
    }
    if (test_curved_mask() != 0) {
        return 1;
    }
    if (test_mask_global_tags() != 0) {
        return 1;
    }
    if (test_zip_diag() != 0) {
        return 1;
    }
    printf("test_v16: ok\n");
    return 0;
}
