#ifndef HITBOX3D_H
#define HITBOX3D_H

/*
    hitbox3d - offensive 3D volumes + swept checks against hurtbox3d

    Depends on hurtbox3d for fixed math, matrices, shapes and AABBs.
    C89, no malloc/realloc/free, no internal heap, no float/double.
*/

#include "hurtbox3d.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef HIT3_MAX_HITBOXES_PER_ATTACK
#define HIT3_MAX_HITBOXES_PER_ATTACK 8
#endif

#define HIT3_OK                 0
#define HIT3_ERR_FULL          -1
#define HIT3_ERR_NOT_FOUND     -2
#define HIT3_ERR_BAD_ARG       -3

#define HIT3_HIT_SLASH          0x0001
#define HIT3_HIT_BLUNT          0x0002
#define HIT3_HIT_BITE           0x0004
#define HIT3_HIT_GRAB           0x0008
#define HIT3_HIT_PIERCE         0x0010
#define HIT3_HIT_INFECT         0x0020
#define HIT3_HIT_KNOCKDOWN      0x0040
#define HIT3_HIT_GUARDABLE      0x0080
#define HIT3_HIT_PARRYABLE      0x0100

#define HIT3_DEBUG_HITBOXES     0x0001
#define HIT3_DEBUG_COLOR_HIT    2

typedef struct hit3_box_s {
    int active;
    hb3_shape curr;
    hb3_shape prev;
    int has_prev;
    int user_tag;
} hit3_box;

typedef struct hit3_attack_s {
    int active;
    int owner_id;
    int group_mask;
    int hit_mask;
    hit3_box hitboxes[HIT3_MAX_HITBOXES_PER_ATTACK];
    int hitbox_count;
} hit3_attack;

void hit3_attack_init(hit3_attack *a, int owner_id);
void hit3_attack_clear(hit3_attack *a);
void hit3_attack_set_masks(hit3_attack *a, int group_mask, int hit_mask);

int hit3_attack_set_sphere(hit3_attack *a, int slot, int hit_id,
                           hb3_v3 center, hb3_fx radius,
                           int hit_flags, int user_tag);
int hit3_attack_set_capsule(hit3_attack *a, int slot, int hit_id,
                            hb3_v3 p0, hb3_v3 p1, hb3_fx radius,
                            int hit_flags, int user_tag);
int hit3_attack_set_obb(hit3_attack *a, int slot, int hit_id,
                        hb3_v3 center, hb3_v3 half_extents,
                        hb3_mat3 basis, int hit_flags, int user_tag);

int hit3_hitbox_get_query_aabb(const hit3_box *hb, hb3_aabb *out_aabb);
int hit3_hitbox_intersects_hurtbox(const hit3_box *hb, const hb3_hurtbox *hurt);
int hit3_attack_debug_draw(const hit3_attack *a, hb3_debug_line_fn line_fn,
                           void *user);

#ifdef __cplusplus
}
#endif

#endif
