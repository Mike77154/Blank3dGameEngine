#ifndef FPI_VALUE_H
#define FPI_VALUE_H

#include "fpi_error.h"

typedef enum FPI_ValueKind {
    FPI_VALUE_NONE = 0,
    FPI_VALUE_TEXT = 1,
    FPI_VALUE_FIXED = 2
} FPI_ValueKind;

typedef struct FPI_ValueRef {
    FPI_U32 text_offset;
    FPI_U16 text_length;
    FPI_U8 has;
    FPI_U8 kind;
    FPI_Fixed fixed;
} FPI_ValueRef;

typedef struct FPI_Value {
    int has;
    int kind;
    FPI_Fixed fixed;
    int is_int;
    int i;
    const char* s;
    int len;
} FPI_Value;

int fpi_fixed_parse(const char* text, FPI_Fixed* out_value);
int fpi_fixed_format(FPI_Fixed value, char* out, int cap, int decimals);
FPI_Fixed fpi_fixed_from_int(long value);
long fpi_fixed_to_int(FPI_Fixed value);
FPI_Fixed fpi_fixed_add_sat(FPI_Fixed a, FPI_Fixed b);
FPI_Fixed fpi_fixed_sub_sat(FPI_Fixed a, FPI_Fixed b);
FPI_Fixed fpi_fixed_mul_sat(FPI_Fixed a, FPI_Fixed b);
FPI_Fixed fpi_fixed_div_sat(FPI_Fixed a, FPI_Fixed b, int* ok);
FPI_Fixed fpi_fixed_mod(FPI_Fixed a, FPI_Fixed b, int* ok);
FPI_Fixed fpi_fixed_sin_deg(FPI_Fixed degrees);
FPI_Fixed fpi_fixed_cos_deg(FPI_Fixed degrees);
void fpi_value_clear(FPI_Value* value);

#endif /* FPI_VALUE_H */
