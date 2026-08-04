#ifndef RPYL_VALUE_H
#define RPYL_VALUE_H

#include "rpyl_fixed.h"
#include "rpyl_bytecode.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum RpylValueKind {
    RPYL_VALUE_NONE = 0,
    RPYL_VALUE_INT,
    RPYL_VALUE_FIXED,
    RPYL_VALUE_BOOL,
    RPYL_VALUE_STRING
} RpylValueKind;

typedef struct RpylValue {
    RpylValueKind kind;
    union {
        long i;
        rpyl_fx fx;
        int b;
        rpyl_u32 sid;
    } as;
} RpylValue;

RpylValue rpyl_value_none(void);
RpylValue rpyl_value_int(long v);
RpylValue rpyl_value_fixed(rpyl_fx v);
RpylValue rpyl_value_bool(int v);
RpylValue rpyl_value_string(rpyl_u32 sid);

#ifdef __cplusplus
}
#endif

#endif
