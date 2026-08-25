#ifndef PDC3D_DAMAGE_H
#define PDC3D_DAMAGE_H

#include "pdc3d_config.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Material/body flags. These intentionally mirror common hurtbox tags. */
#define PDC3D_MAT_FLESH       0x0001
#define PDC3D_MAT_ARMOR       0x0002
#define PDC3D_MAT_HEAD        0x0004
#define PDC3D_MAT_LIMB        0x0008
#define PDC3D_MAT_WEAKSPOT    0x0010
#define PDC3D_MAT_SHIELD      0x0020
#define PDC3D_MAT_NO_DAMAGE   0x0040
#define PDC3D_MAT_BONE        0x0080
#define PDC3D_MAT_METAL       0x0100
#define PDC3D_MAT_WOOD        0x0200
#define PDC3D_MAT_GLASS       0x0400
#define PDC3D_MAT_PARASITE    0x0800
#define PDC3D_MAT_VEHICLE     0x1000

/* Attack flags. These mirror hit3 plus extra physical damage tags. */
#define PDC3D_ATK_SLASH       0x0001
#define PDC3D_ATK_BLUNT       0x0002
#define PDC3D_ATK_BITE        0x0004
#define PDC3D_ATK_GRAB        0x0008
#define PDC3D_ATK_PIERCE      0x0010
#define PDC3D_ATK_INFECT      0x0020
#define PDC3D_ATK_KNOCKDOWN   0x0040
#define PDC3D_ATK_GUARDABLE   0x0080
#define PDC3D_ATK_PARRYABLE   0x0100
#define PDC3D_ATK_FIRE        0x0200
#define PDC3D_ATK_ACID        0x0400
#define PDC3D_ATK_EXPLOSIVE   0x0800

#define PDC3D_DMG_NONE        0x0000
#define PDC3D_DMG_BLOCKED     0x0001
#define PDC3D_DMG_WEAKSPOT    0x0002
#define PDC3D_DMG_ARMOR       0x0004
#define PDC3D_DMG_NO_DAMAGE   0x0008
#define PDC3D_DMG_BLEED       0x0010
#define PDC3D_DMG_FRACTURE    0x0020
#define PDC3D_DMG_STAGGER     0x0040
#define PDC3D_DMG_DEFENDED    0x0080
#define PDC3D_DMG_GUARDED     0x0100
#define PDC3D_DMG_PARRY       0x0200
#define PDC3D_DMG_PARRY_LATE  0x0400
#define PDC3D_DMG_GUARD_BROKEN 0x0800
#define PDC3D_DMG_BLOCK_BROKEN 0x1000
#define PDC3D_DMG_SHELL       0x2000
#define PDC3D_DMG_SHELL_BROKEN 0x4000

typedef struct pdc3d_material_rule_s {
    int active;
    int material_mask;
    int attack_mask;
    int multiplier_num;
    int multiplier_den;
    int flat_add;
    int result_flags;
} pdc3d_material_rule;

typedef struct pdc3d_damage_table_s {
    pdc3d_material_rule rules[PDC3D_MAX_MATERIAL_RULES];
    int rule_count;
} pdc3d_damage_table;

typedef struct pdc3d_damage_packet_s {
    int attacker_id;
    int defender_id;
    int attack_id;
    int hurtbox_index;
    int hitbox_index;
    int base_damage;
    int final_damage;
    int stun_frames;
    int hitstop_frames;
    int attack_flags;
    int material_flags;
    int result_flags;
    int weakspot_id;
    int hit_user_tag;
    int hurt_user_tag;
} pdc3d_damage_packet;

void pdc3d_damage_table_init(pdc3d_damage_table *t);
int pdc3d_damage_table_add_rule(pdc3d_damage_table *t,
                                int material_mask,
                                int attack_mask,
                                int multiplier_num,
                                int multiplier_den,
                                int flat_add,
                                int result_flags);
void pdc3d_damage_table_add_defaults(pdc3d_damage_table *t);
int pdc3d_damage_apply_rules(const pdc3d_damage_table *t,
                             int base_damage,
                             int material_flags,
                             int attack_flags,
                             int *out_flags);

#ifdef __cplusplus
}
#endif

#endif
