#ifndef DDSL_VALUE_H
#define DDSL_VALUE_H

#include "common/strview.h"
#include "types/fixed.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum ddsl_value_kind {
    DDSL_VAL_NULL = 0,
    DDSL_VAL_NUM,
    DDSL_VAL_BOOL,
    DDSL_VAL_STR
} ddsl_value_kind;

typedef struct ddsl_value {
    ddsl_value_kind kind;
    ddsl_fixed num;
    int boolean;
    ddsl_strview str; /* cuando kind=STR */
} ddsl_value;

/* Crea valores */
ddsl_value ddsl_v_null(void);
ddsl_value ddsl_v_num(ddsl_fixed x);
ddsl_value ddsl_v_bool(int b);
ddsl_value ddsl_v_str(ddsl_strview sv);

/* Verdad "humana": números != 0, bool, strings tipo true/false/yes/no/etc */
int ddsl_value_truthy(ddsl_value v);

/* Intenta convertir a número (si posible). ok=1 si pudo. */
ddsl_fixed ddsl_value_to_num(ddsl_value v, int *ok);

/* Convierte valor a C-string en un buffer. Devuelve longitud (sin NUL). */
int ddsl_value_to_cstr(ddsl_value v, char *dst, int dst_cap);

#ifdef __cplusplus
}
#endif

#endif /* DDSL_VALUE_H */
