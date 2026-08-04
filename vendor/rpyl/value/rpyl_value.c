#include "rpyl_value.h"

RpylValue rpyl_value_none(void) {
    RpylValue v;
    v.kind = RPYL_VALUE_NONE;
    v.as.i = 0L;
    return v;
}

RpylValue rpyl_value_int(long x) {
    RpylValue v;
    v.kind = RPYL_VALUE_INT;
    v.as.i = x;
    return v;
}

RpylValue rpyl_value_fixed(rpyl_fx x) {
    RpylValue v;
    v.kind = RPYL_VALUE_FIXED;
    v.as.fx = x;
    return v;
}

RpylValue rpyl_value_bool(int x) {
    RpylValue v;
    v.kind = RPYL_VALUE_BOOL;
    v.as.b = x ? 1 : 0;
    return v;
}

RpylValue rpyl_value_string(rpyl_u32 sid) {
    RpylValue v;
    v.kind = RPYL_VALUE_STRING;
    v.as.sid = sid;
    return v;
}
