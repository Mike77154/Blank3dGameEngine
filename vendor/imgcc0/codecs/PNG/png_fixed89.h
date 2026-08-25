#ifndef PNG_FIXED89_H
#define PNG_FIXED89_H

typedef signed int png_fixed89;

#define PNG_FIXED89_ONE 65536
#define PNG_FIXED89_HALF 32768

png_fixed89 png_fixed89_from_ratio(signed int num, signed int den);
png_fixed89 png_fixed89_mul(png_fixed89 a, png_fixed89 b);
png_fixed89 png_fixed89_div(png_fixed89 a, png_fixed89 b);
int png_fixed89_parse_decimal(const char* s, png_fixed89* out_value);
png_fixed89 png_fixed89_pow_unit(png_fixed89 x, png_fixed89 exponent);
png_fixed89 png_fixed89_pow_positive(png_fixed89 base, png_fixed89 exponent);
png_fixed89 png_fixed89_exp(png_fixed89 x);
png_fixed89 png_fixed89_sinh(png_fixed89 x);
unsigned int png_fixed89_unit_to_u16(png_fixed89 x);
unsigned int png_fixed89_unit_to_u8(png_fixed89 x);

#endif
