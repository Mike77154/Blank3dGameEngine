#include "pdc3d_damage.h"

void pdc3d_damage_table_init(pdc3d_damage_table *t)
{
    int i;
    if (t == 0) {
        return;
    }
    t->rule_count = 0;
    for (i = 0; i < PDC3D_MAX_MATERIAL_RULES; ++i) {
        t->rules[i].active = 0;
        t->rules[i].material_mask = 0;
        t->rules[i].attack_mask = 0;
        t->rules[i].multiplier_num = 1;
        t->rules[i].multiplier_den = 1;
        t->rules[i].flat_add = 0;
        t->rules[i].result_flags = 0;
    }
}

int pdc3d_damage_table_add_rule(pdc3d_damage_table *t,
                                int material_mask,
                                int attack_mask,
                                int multiplier_num,
                                int multiplier_den,
                                int flat_add,
                                int result_flags)
{
    pdc3d_material_rule *r;
    if (t == 0 || multiplier_den == 0) {
        return -3;
    }
    if (t->rule_count >= PDC3D_MAX_MATERIAL_RULES) {
        return -1;
    }
    r = &t->rules[t->rule_count];
    r->active = 1;
    r->material_mask = material_mask;
    r->attack_mask = attack_mask;
    r->multiplier_num = multiplier_num;
    r->multiplier_den = multiplier_den;
    r->flat_add = flat_add;
    r->result_flags = result_flags;
    t->rule_count += 1;
    return 0;
}

void pdc3d_damage_table_add_defaults(pdc3d_damage_table *t)
{
    if (t == 0) {
        return;
    }
    /* No-damage areas always zero out first. */
    pdc3d_damage_table_add_rule(t, PDC3D_MAT_NO_DAMAGE, 0x7fffffff,
                                0, 1, 0, PDC3D_DMG_NO_DAMAGE);
    /* Head/head-like parts are naturally dangerous for pierce/slash/blunt. */
    pdc3d_damage_table_add_rule(t, PDC3D_MAT_HEAD,
                                PDC3D_ATK_PIERCE | PDC3D_ATK_SLASH | PDC3D_ATK_BLUNT,
                                3, 2, 0, PDC3D_DMG_STAGGER);
    /* Weakspot flag gives a light bonus; explicit weakspot volumes can do more. */
    pdc3d_damage_table_add_rule(t, PDC3D_MAT_WEAKSPOT, 0x7fffffff,
                                2, 1, 0, PDC3D_DMG_WEAKSPOT);
    /* Armor resists low-energy slash/bite and blunt. */
    pdc3d_damage_table_add_rule(t, PDC3D_MAT_ARMOR,
                                PDC3D_ATK_SLASH | PDC3D_ATK_BITE | PDC3D_ATK_BLUNT,
                                1, 2, 0, PDC3D_DMG_ARMOR);
    /* Shield is stronger than armor unless attack is explosive/acid. */
    pdc3d_damage_table_add_rule(t, PDC3D_MAT_SHIELD,
                                PDC3D_ATK_SLASH | PDC3D_ATK_BITE | PDC3D_ATK_BLUNT | PDC3D_ATK_PIERCE,
                                1, 4, 0, PDC3D_DMG_BLOCKED);
    /* Parasites hate fire/acid. */
    pdc3d_damage_table_add_rule(t, PDC3D_MAT_PARASITE,
                                PDC3D_ATK_FIRE | PDC3D_ATK_ACID,
                                2, 1, 0, PDC3D_DMG_WEAKSPOT);
    /* Bone/limbs can stagger/fracture with blunt damage. */
    pdc3d_damage_table_add_rule(t, PDC3D_MAT_BONE | PDC3D_MAT_LIMB,
                                PDC3D_ATK_BLUNT,
                                5, 4, 0, PDC3D_DMG_FRACTURE);
}

int pdc3d_damage_apply_rules(const pdc3d_damage_table *t,
                             int base_damage,
                             int material_flags,
                             int attack_flags,
                             int *out_flags)
{
    int i;
    int dmg;
    int flags;
    if (t == 0) {
        if (out_flags != 0) {
            *out_flags = 0;
        }
        return base_damage;
    }
    dmg = base_damage;
    flags = 0;
    for (i = 0; i < t->rule_count; ++i) {
        const pdc3d_material_rule *r;
        int material_match;
        int attack_match;
        r = &t->rules[i];
        if (!r->active) {
            continue;
        }
        material_match = (r->material_mask == 0) || ((material_flags & r->material_mask) != 0);
        attack_match = (r->attack_mask == 0) || ((attack_flags & r->attack_mask) != 0);
        if (material_match && attack_match) {
            dmg = (dmg * r->multiplier_num) / r->multiplier_den;
            dmg += r->flat_add;
            flags |= r->result_flags;
            if ((r->result_flags & PDC3D_DMG_NO_DAMAGE) != 0) {
                dmg = 0;
            }
        }
    }
    if (dmg < 0) {
        dmg = 0;
    }
    if (out_flags != 0) {
        *out_flags = flags;
    }
    return dmg;
}
