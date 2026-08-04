/*
    cameranaku89_target_group.h
    CC0 1.0 Universal.
    Multi-target framing helper with static storage.
*/
#ifndef CAMERANAKU89_TARGET_GROUP_H
#define CAMERANAKU89_TARGET_GROUP_H

#include "cameranaku89.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct cnk_target_group_item_s {
    cnk_target target;
    cnk_fx weight;
    cnk_fx radius;
    int id;
    int active;
} cnk_target_group_item;

typedef struct cnk_target_group_s {
    cnk_target_group_item items[CNK_MAX_TARGET_GROUP_ITEMS];
    int count;
    cnk_vec3 center;
    cnk_vec3 minv;
    cnk_vec3 maxv;
    cnk_fx radius;
    int valid;
} cnk_target_group;

CNK_API void cnk_target_group_clear(cnk_target_group *group);
CNK_API int cnk_target_group_add(cnk_target_group *group, int id, cnk_target target, cnk_fx weight, cnk_fx radius);
CNK_API int cnk_target_group_set(cnk_target_group *group, int slot, cnk_target target, cnk_fx weight, cnk_fx radius);
CNK_API int cnk_target_group_compute(cnk_target_group *group);
CNK_API int cnk_target_group_as_target(cnk_target_group *group, cnk_target *out_target);

#ifdef __cplusplus
}
#endif

#endif
