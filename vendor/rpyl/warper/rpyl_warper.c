#include "rpyl_warper.h"
#include "rpyl_common.h"

rpyl_fx rpyl_warper_clamp01(rpyl_fx t) {
    if (t < 0) return 0;
    if (t > (rpyl_fx)RPYL_FX_ONE) return (rpyl_fx)RPYL_FX_ONE;
    return t;
}

rpyl_fx rpyl_warper_lerp(rpyl_fx a, rpyl_fx b, rpyl_fx t) {
    rpyl_fx delta;
    t = rpyl_warper_clamp01(t);
    delta = b - a;
    return a + rpyl_fx_mul(delta, t);
}

rpyl_fx rpyl_warper_linear(rpyl_fx a, rpyl_fx b, rpyl_fx t) {
    return rpyl_warper_lerp(a, b, t);
}

rpyl_fx rpyl_warper_ease_in_quad(rpyl_fx t) {
    t = rpyl_warper_clamp01(t);
    return rpyl_fx_mul(t, t);
}

rpyl_fx rpyl_warper_ease_out_quad(rpyl_fx t) {
    rpyl_fx one;
    rpyl_fx inv;
    one = (rpyl_fx)RPYL_FX_ONE;
    t = rpyl_warper_clamp01(t);
    inv = one - t;
    return one - rpyl_fx_mul(inv, inv);
}

rpyl_fx rpyl_warper_ease_in_out_quad(rpyl_fx t) {
    rpyl_fx two;
    rpyl_fx one;
    rpyl_fx inv;
    t = rpyl_warper_clamp01(t);
    one = (rpyl_fx)RPYL_FX_ONE;
    two = (rpyl_fx)(2L * RPYL_FX_ONE);
    if (t < (rpyl_fx)(RPYL_FX_ONE / 2L)) return rpyl_fx_mul(two, rpyl_fx_mul(t, t));
    inv = one - t;
    return one - rpyl_fx_mul(two, rpyl_fx_mul(inv, inv));
}

rpyl_fx rpyl_warper_smoothstep(rpyl_fx t) {
    rpyl_fx three;
    rpyl_fx two_t;
    rpyl_fx inner;
    t = rpyl_warper_clamp01(t);
    three = (rpyl_fx)(3L * RPYL_FX_ONE);
    two_t = (rpyl_fx)(2L * t);
    inner = three - two_t;
    return rpyl_fx_mul(rpyl_fx_mul(t, t), inner);
}

rpyl_fx rpyl_warper_apply(rpyl_fx a, rpyl_fx b, rpyl_fx t, const char* curve) {
    rpyl_fx u;
    if (!curve || curve[0] == '\0' || rpyl_common_streq(curve, "linear")) return rpyl_warper_lerp(a, b, t);
    if (rpyl_common_streq(curve, "ease_in") || rpyl_common_streq(curve, "ease_in_quad")) u = rpyl_warper_ease_in_quad(t);
    else if (rpyl_common_streq(curve, "ease_out") || rpyl_common_streq(curve, "ease_out_quad")) u = rpyl_warper_ease_out_quad(t);
    else if (rpyl_common_streq(curve, "ease_in_out") || rpyl_common_streq(curve, "ease_in_out_quad")) u = rpyl_warper_ease_in_out_quad(t);
    else if (rpyl_common_streq(curve, "smoothstep")) u = rpyl_warper_smoothstep(t);
    else u = t;
    return rpyl_warper_lerp(a, b, u);
}
