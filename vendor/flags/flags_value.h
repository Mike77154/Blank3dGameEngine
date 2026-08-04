/* flags_value.h - typed values for flags (no float) - C89 */
#ifndef FLAGS_VALUE_H
#define FLAGS_VALUE_H

#include "flags_fx.h"

typedef enum FlagsValType {
    FLAGS_VAL_NONE = 0,
    FLAGS_VAL_BOOL = 1,
    FLAGS_VAL_INT  = 2,
    FLAGS_VAL_FX   = 3,
    FLAGS_VAL_STR  = 4
} FlagsValType;

typedef struct FlagsValue {
    FlagsValType type;
    union {
        long i;          /* bool/int */
        flags_fx_t fx;   /* fixed */
        const char *s;   /* string (NUL-terminated) */
    } as;
} FlagsValue;

/* Constructors */
FlagsValue flags_value_none(void);
FlagsValue flags_value_bool(int b);
FlagsValue flags_value_int(long i);
FlagsValue flags_value_fx(flags_fx_t fx);
FlagsValue flags_value_str(const char *s);

/* Convert to fixed-point when possible (BOOL/INT/FX). Returns 1 if converted. */
int flags_value_to_fx(const FlagsValue *v, flags_fx_t *out_fx);

#endif /* FLAGS_VALUE_H */
