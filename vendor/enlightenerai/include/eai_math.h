#ifndef EAI_MATH_H
#define EAI_MATH_H

#include "eai_types.h"

EAI_Fixed eai_fx_from_int(EAI_Fixed value);
EAI_Fixed eai_fx_from_ratio(EAI_Fixed numerator, EAI_Fixed denominator);
EAI_Fixed eai_fx_mul(EAI_Fixed a, EAI_Fixed b);
EAI_Fixed eai_fx_div(EAI_Fixed a, EAI_Fixed b);
EAI_Fixed eai_fx_sqrt(EAI_Fixed value);
EAI_Fixed eai_fx_clamp(EAI_Fixed value, EAI_Fixed min_value, EAI_Fixed max_value);
EAI_Fixed eai_fx_add_sat(EAI_Fixed a, EAI_Fixed b);
EAI_Fixed eai_fx_sub_sat(EAI_Fixed a, EAI_Fixed b);
EAI_Fixed eai_fx_madd_sat(EAI_Fixed acc, EAI_Fixed a, EAI_Fixed b);

void  eai_vec3_set(EAI_Vec3* v, EAI_Fixed x, EAI_Fixed y, EAI_Fixed z);
void  eai_vec3_add(EAI_Vec3* out_v, const EAI_Vec3* a, const EAI_Vec3* b);
void  eai_vec3_sub(EAI_Vec3* out_v, const EAI_Vec3* a, const EAI_Vec3* b);
void  eai_vec3_scale(EAI_Vec3* out_v, const EAI_Vec3* a, EAI_Fixed s);
EAI_Fixed eai_vec3_dot(const EAI_Vec3* a, const EAI_Vec3* b);
EAI_Fixed eai_vec3_len_sq(const EAI_Vec3* a);
EAI_Fixed eai_vec3_len(const EAI_Vec3* a);
EAI_Fixed eai_vec3_dist_sq(const EAI_Vec3* a, const EAI_Vec3* b);
EAI_Fixed eai_vec3_dist(const EAI_Vec3* a, const EAI_Vec3* b);
void  eai_vec3_normalize(EAI_Vec3* out_v, const EAI_Vec3* a);
EAI_Fixed eai_clampf(EAI_Fixed value, EAI_Fixed min_value, EAI_Fixed max_value);

#endif
