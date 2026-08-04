#ifndef ECG_FIXED_H
#define ECG_FIXED_H

#include "ecg_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ECG_FIXED_SHIFT 8
#define ECG_FIXED_ONE ((ECG_FixedQ8)256L)

ECG_FixedQ8 ecg_fixed_from_int(int value);
ECG_FixedQ8 ecg_fixed_from_ratio(int numerator, int denominator);
ECG_FixedQ8 ecg_fixed_mul_q8(ECG_FixedQ8 left_q8,
                              ECG_FixedQ8 right_q8);
int ecg_fixed_to_int_floor(ECG_FixedQ8 value);
int ecg_fixed_to_int_ceil(ECG_FixedQ8 value);
int ecg_fixed_mul_int_to_int_floor(int value, ECG_FixedQ8 scale_q8);
int ecg_fixed_mul_i8_to_int_floor(ECG_I8 value, ECG_FixedQ8 scale_q8);
int ecg_fixed_mul_uint_to_int_floor(unsigned int value, ECG_FixedQ8 scale_q8);
int ecg_fixed_mul_uint_to_int_ceil(unsigned int value, ECG_FixedQ8 scale_q8);

#ifdef __cplusplus
}
#endif

#endif
