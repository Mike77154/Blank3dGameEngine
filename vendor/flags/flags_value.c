/* flags_value.c - typed values - C89 */
#include "flags_value.h"

FlagsValue flags_value_none(void)
{
    FlagsValue v;
    v.type = FLAGS_VAL_NONE;
    v.as.i = 0;
    return v;
}

FlagsValue flags_value_bool(int b)
{
    FlagsValue v;
    v.type = FLAGS_VAL_BOOL;
    v.as.i = (b != 0) ? 1 : 0;
    return v;
}

FlagsValue flags_value_int(long i)
{
    FlagsValue v;
    v.type = FLAGS_VAL_INT;
    v.as.i = i;
    return v;
}

FlagsValue flags_value_fx(flags_fx_t fx)
{
    FlagsValue v;
    v.type = FLAGS_VAL_FX;
    v.as.fx = fx;
    return v;
}

FlagsValue flags_value_str(const char *s)
{
    FlagsValue v;
    v.type = FLAGS_VAL_STR;
    v.as.s = s ? s : "";
    return v;
}

int flags_value_to_fx(const FlagsValue *v, flags_fx_t *out_fx)
{
    if (!v || !out_fx) return 0;
    if (v->type == FLAGS_VAL_FX) {
        *out_fx = v->as.fx;
        return 1;
    }
    if (v->type == FLAGS_VAL_INT || v->type == FLAGS_VAL_BOOL) {
        *out_fx = FLAGS_FX_FROM_INT(v->as.i);
        return 1;
    }
    return 0;
}
