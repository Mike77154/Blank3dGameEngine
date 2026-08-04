#include "ecg_fixed.h"

ECG_FixedQ8 ecg_fixed_from_int(int value)
{
    return ((ECG_FixedQ8)value) * ECG_FIXED_ONE;
}

ECG_FixedQ8 ecg_fixed_from_ratio(int numerator, int denominator)
{
    ECG_FixedQ8 scaled;

    if (denominator == 0) {
        return (ECG_FixedQ8)0;
    }

    scaled = ((ECG_FixedQ8)numerator) * ECG_FIXED_ONE;
    return scaled / (ECG_FixedQ8)denominator;
}

ECG_FixedQ8 ecg_fixed_mul_q8(ECG_FixedQ8 left_q8,
                              ECG_FixedQ8 right_q8)
{
    ECG_FixedQ8 whole;
    ECG_FixedQ8 fraction;

    whole = (left_q8 / ECG_FIXED_ONE) * right_q8;
    fraction = ((left_q8 % ECG_FIXED_ONE) * right_q8) / ECG_FIXED_ONE;
    return whole + fraction;
}

int ecg_fixed_to_int_floor(ECG_FixedQ8 value)
{
    ECG_FixedQ8 mag;

    if (value >= (ECG_FixedQ8)0) {
        return (int)(value / ECG_FIXED_ONE);
    }

    mag = (ECG_FixedQ8)(-value);
    return -(int)((mag + ECG_FIXED_ONE - (ECG_FixedQ8)1) / ECG_FIXED_ONE);
}

int ecg_fixed_to_int_ceil(ECG_FixedQ8 value)
{
    ECG_FixedQ8 mag;

    if (value >= (ECG_FixedQ8)0) {
        return (int)((value + ECG_FIXED_ONE - (ECG_FixedQ8)1) / ECG_FIXED_ONE);
    }

    mag = (ECG_FixedQ8)(-value);
    return -(int)(mag / ECG_FIXED_ONE);
}

int ecg_fixed_mul_int_to_int_floor(int value, ECG_FixedQ8 scale_q8)
{
    ECG_FixedQ8 scaled;

    scaled = ((ECG_FixedQ8)value) * scale_q8;
    return ecg_fixed_to_int_floor(scaled);
}

int ecg_fixed_mul_i8_to_int_floor(ECG_I8 value, ECG_FixedQ8 scale_q8)
{
    return ecg_fixed_mul_int_to_int_floor((int)value, scale_q8);
}

int ecg_fixed_mul_uint_to_int_floor(unsigned int value, ECG_FixedQ8 scale_q8)
{
    ECG_FixedQ8 scaled;

    scaled = ((ECG_FixedQ8)value) * scale_q8;
    return ecg_fixed_to_int_floor(scaled);
}

int ecg_fixed_mul_uint_to_int_ceil(unsigned int value, ECG_FixedQ8 scale_q8)
{
    ECG_FixedQ8 scaled;

    scaled = ((ECG_FixedQ8)value) * scale_q8;
    return ecg_fixed_to_int_ceil(scaled);
}
