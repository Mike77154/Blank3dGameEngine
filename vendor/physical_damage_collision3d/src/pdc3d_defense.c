#include "pdc3d_defense.h"

static long pdc3d_def_to_gfix(hb3_fx v)
{
#if HB3_FX_SHIFT < 10
    return (long)v * (long)(1L << (10 - HB3_FX_SHIFT));
#elif HB3_FX_SHIFT > 10
    return (long)v / (long)(1L << (HB3_FX_SHIFT - 10));
#else
    return (long)v;
#endif
}

static GBLK_Vec3 pdc3d_to_gblk_v3(hb3_v3 v)
{
    GBLK_Vec3 o;
    o.x = pdc3d_def_to_gfix(v.x);
    o.y = pdc3d_def_to_gfix(v.y);
    o.z = pdc3d_def_to_gfix(v.z);
    return o;
}

static GGRD_Vec3 pdc3d_to_ggrd_v3(hb3_v3 v)
{
    GGRD_Vec3 o;
    o.x = pdc3d_def_to_gfix(v.x);
    o.y = pdc3d_def_to_gfix(v.y);
    o.z = pdc3d_def_to_gfix(v.z);
    return o;
}

static GPRY_Vec3 pdc3d_to_gpry_v3(hb3_v3 v)
{
    GPRY_Vec3 o;
    o.x = pdc3d_def_to_gfix(v.x);
    o.y = pdc3d_def_to_gfix(v.y);
    o.z = pdc3d_def_to_gfix(v.z);
    return o;
}

static GSHL_Vec3 pdc3d_to_gshl_v3(hb3_v3 v)
{
    GSHL_Vec3 o;
    o.x = pdc3d_def_to_gfix(v.x);
    o.y = pdc3d_def_to_gfix(v.y);
    o.z = pdc3d_def_to_gfix(v.z);
    return o;
}

static GCTR_Vec3 pdc3d_to_gctr_v3(hb3_v3 v)
{
    GCTR_Vec3 o;
    o.x = pdc3d_def_to_gfix(v.x);
    o.y = pdc3d_def_to_gfix(v.y);
    o.z = pdc3d_def_to_gfix(v.z);
    return o;
}

static pdc3d_defense_actor *pdc3d_def_find(pdc3d_defense_world *d,
                                           int actor_id)
{
    int i;
    if (d == 0) {
        return 0;
    }
    for (i = 0; i < PDC3D_MAX_DEFENSE_ACTORS; ++i) {
        if (d->actors[i].used != 0 && d->actors[i].actor_id == actor_id) {
            return &d->actors[i];
        }
    }
    return 0;
}

static const pdc3d_defense_actor *pdc3d_def_find_const(const pdc3d_defense_world *d,
                                                       int actor_id)
{
    int i;
    if (d == 0) {
        return 0;
    }
    for (i = 0; i < PDC3D_MAX_DEFENSE_ACTORS; ++i) {
        if (d->actors[i].used != 0 && d->actors[i].actor_id == actor_id) {
            return &d->actors[i];
        }
    }
    return 0;
}

static pdc3d_defense_actor *pdc3d_def_alloc(pdc3d_defense_world *d,
                                            int actor_id)
{
    int i;
    pdc3d_defense_actor *a;
    a = pdc3d_def_find(d, actor_id);
    if (a != 0) {
        return a;
    }
    if (d == 0) {
        return 0;
    }
    for (i = 0; i < PDC3D_MAX_DEFENSE_ACTORS; ++i) {
        if (d->actors[i].used == 0) {
            d->actors[i].used = 1;
            d->actors[i].actor_id = actor_id;
            d->actors[i].enabled_flags = 0u;
            d->actors[i].guard_hold_input = 0;
            return &d->actors[i];
        }
    }
    return 0;
}

static int pdc3d_def_attack_to_block_flags(const pdc3d_damage_packet *p)
{
    int flags;
    flags = 0;
    if ((p->attack_flags & PDC3D_ATK_GUARDABLE) == 0) {
        flags |= GBLK_FLAG_UNBLOCKABLE;
    }
    if ((p->attack_flags & (PDC3D_ATK_EXPLOSIVE | PDC3D_ATK_KNOCKDOWN)) != 0) {
        flags |= GBLK_FLAG_GUARD_BREAKER;
    }
    return flags;
}

static unsigned int pdc3d_def_attack_to_guard_flags(const pdc3d_damage_packet *p)
{
    unsigned int flags;
    flags = 0u;
    if ((p->attack_flags & PDC3D_ATK_GUARDABLE) == 0) {
        flags |= GGRD_FLAG_UNGUARDABLE;
    }
    if ((p->attack_flags & (PDC3D_ATK_EXPLOSIVE | PDC3D_ATK_KNOCKDOWN)) != 0) {
        flags |= GGRD_FLAG_HEAVY;
    }
    return flags;
}

static unsigned int pdc3d_def_attack_to_parry_flags(const pdc3d_damage_packet *p)
{
    unsigned int flags;
    flags = 0u;
    if ((p->attack_flags & PDC3D_ATK_PARRYABLE) == 0) {
        flags |= GPRY_FLAG_UNPARRYABLE;
    }
    if ((p->attack_flags & PDC3D_ATK_PIERCE) != 0) {
        flags |= GPRY_FLAG_THRUST;
    }
    return flags;
}

static unsigned int pdc3d_def_attack_to_shell_type(const pdc3d_damage_packet *p)
{
    unsigned int type;
    type = 0u;
    if ((p->attack_flags & PDC3D_ATK_BLUNT) != 0) {
        type |= GSHL_DMG_BLUNT;
    }
    if ((p->attack_flags & PDC3D_ATK_SLASH) != 0) {
        type |= GSHL_DMG_SLASH;
    }
    if ((p->attack_flags & PDC3D_ATK_PIERCE) != 0) {
        type |= GSHL_DMG_PIERCE;
    }
    if ((p->attack_flags & (PDC3D_ATK_FIRE | PDC3D_ATK_ACID | PDC3D_ATK_EXPLOSIVE)) != 0) {
        type |= GSHL_DMG_FIRE;
    }
    if (type == 0u) {
        type = GSHL_DMG_BLUNT;
    }
    return type;
}

static void pdc3d_def_result_init(pdc3d_defense_result *r,
                                  const pdc3d_damage_packet *p)
{
    if (r == 0) {
        return;
    }
    r->accepted = 0;
    r->final_damage = p != 0 ? p->final_damage : 0;
    r->chip_damage = 0;
    r->absorbed_damage = 0;
    r->stamina_damage = 0;
    r->posture_damage = 0;
    r->guard_damage = 0;
    r->guard_broken = 0;
    r->block_broken = 0;
    r->shell_broken = 0;
    r->parry_result = GPRY_RESULT_NONE;
    r->counter_token_created = 0;
    r->counter_used = 0;
    r->counter_kind = 0;
    r->counter_script_code = 0;
    r->attacker_stagger_frames = 0;
    r->defender_stun_frames = p != 0 ? p->stun_frames : 0;
    r->blockstun_frames = 0;
    r->recoil_frames = 0;
    r->dot = 0;
    r->defense_flags = PDC3D_DEFRES_NONE;
}

static void pdc3d_def_feed_counter(pdc3d_defense_world *d,
                                   const pdc3d_damage_packet *p,
                                   hb3_v3 source_dir,
                                   int current_frame,
                                   unsigned int source_flags,
                                   unsigned int result_flags,
                                   pdc3d_defense_result *out_result)
{
    GCTR_DefenseEvent ev;
    int fed;
    if (d == 0 || p == 0) {
        return;
    }
    ev.defender_id = p->defender_id;
    ev.attacker_id = p->attacker_id;
    ev.event_frame = current_frame;
    ev.source_flags = source_flags;
    ev.defense_result_flags = result_flags;
    ev.attacker_rel_pos = pdc3d_to_gctr_v3(source_dir);
    fed = gctr_feed_defense_event(&d->counter, &ev);
    if (fed != 0 && out_result != 0) {
        out_result->counter_token_created = 1;
        out_result->defense_flags |= PDC3D_DEFRES_COUNTER_TOKEN;
    }
}

void pdc3d_defense_profile_defaults(pdc3d_defense_profile *p)
{
    if (p == 0) {
        return;
    }
    p->enabled_flags = PDC3D_DEF_ALL;
    gblk_default_profile(&p->block);
    ggrd_default_profile(&p->guard);
    gpry_default_profile(&p->parry);
    gctr_default_profile(&p->counter);
}

void pdc3d_defense_world_init(pdc3d_defense_world *d)
{
    int i;
    if (d == 0) {
        return;
    }
    gblk_init(&d->block);
    ggrd_init(&d->guard);
    gpry_init(&d->parry);
    gshl_init(&d->shell);
    gctr_init(&d->counter);
    for (i = 0; i < PDC3D_MAX_DEFENSE_ACTORS; ++i) {
        d->actors[i].used = 0;
        d->actors[i].actor_id = 0;
        d->actors[i].enabled_flags = 0u;
        d->actors[i].guard_hold_input = 0;
    }
}

int pdc3d_defense_actor_create(pdc3d_defense_world *d,
                               int actor_id,
                               unsigned int enabled_flags,
                               const pdc3d_defense_profile *profile)
{
    pdc3d_defense_actor *a;
    pdc3d_defense_profile defp;
    unsigned int flags;
    if (d == 0) {
        return 0;
    }
    pdc3d_defense_profile_defaults(&defp);
    if (profile != 0) {
        defp = *profile;
    }
    flags = enabled_flags;
    if (flags == 0u) {
        flags = defp.enabled_flags;
    }
    a = pdc3d_def_alloc(d, actor_id);
    if (a == 0) {
        return 0;
    }
    a->enabled_flags = flags;
    a->guard_hold_input = 0;
    if ((flags & PDC3D_DEF_BLOCK) != 0u) {
        gblk_create_actor(&d->block, actor_id, &defp.block);
    }
    if ((flags & PDC3D_DEF_GUARD) != 0u) {
        ggrd_create_actor(&d->guard, actor_id, &defp.guard);
    }
    if ((flags & PDC3D_DEF_PARRY) != 0u) {
        gpry_create_actor(&d->parry, actor_id, &defp.parry);
    }
    if ((flags & PDC3D_DEF_SHELL) != 0u) {
        gshl_create_actor(&d->shell, actor_id);
    }
    if ((flags & PDC3D_DEF_COUNTER) != 0u) {
        gctr_create_actor(&d->counter, actor_id, &defp.counter);
    }
    return 1;
}

int pdc3d_defense_actor_remove(pdc3d_defense_world *d, int actor_id)
{
    pdc3d_defense_actor *a;
    if (d == 0) {
        return 0;
    }
    a = pdc3d_def_find(d, actor_id);
    if (a != 0) {
        a->used = 0;
    }
    gblk_remove_actor(&d->block, actor_id);
    ggrd_remove_actor(&d->guard, actor_id);
    gpry_remove_actor(&d->parry, actor_id);
    gshl_remove_actor(&d->shell, actor_id);
    gctr_remove_actor(&d->counter, actor_id);
    return 1;
}

int pdc3d_defense_actor_set_enabled(pdc3d_defense_world *d,
                                    int actor_id,
                                    unsigned int enabled_flags)
{
    pdc3d_defense_actor *a;
    if (d == 0) {
        return 0;
    }
    a = pdc3d_def_alloc(d, actor_id);
    if (a == 0) {
        return 0;
    }
    a->enabled_flags = enabled_flags;
    return 1;
}

unsigned int pdc3d_defense_actor_get_enabled(const pdc3d_defense_world *d,
                                             int actor_id)
{
    const pdc3d_defense_actor *a;
    a = pdc3d_def_find_const(d, actor_id);
    if (a == 0) {
        return 0u;
    }
    return a->enabled_flags;
}

int pdc3d_defense_set_facing(pdc3d_defense_world *d,
                             int actor_id,
                             hb3_v3 facing_dir)
{
    GBLK_Vec3 b;
    GGRD_Vec3 g;
    GPRY_Vec3 p;
    GSHL_Vec3 s;
    if (d == 0) {
        return 0;
    }
    b = pdc3d_to_gblk_v3(facing_dir);
    g = pdc3d_to_ggrd_v3(facing_dir);
    p = pdc3d_to_gpry_v3(facing_dir);
    s = pdc3d_to_gshl_v3(facing_dir);
    gblk_set_facing(&d->block, actor_id, b.x, b.y, b.z);
    ggrd_set_facing(&d->guard, actor_id, g.x, g.y, g.z);
    gpry_set_facing(&d->parry, actor_id, p.x, p.y, p.z);
    gshl_set_facing(&d->shell, actor_id, s.x, s.y, s.z);
    return 1;
}

int pdc3d_defense_set_guard_input(pdc3d_defense_world *d,
                                  int actor_id,
                                  int hold_input)
{
    pdc3d_defense_actor *a;
    a = pdc3d_def_find(d, actor_id);
    if (a == 0) {
        return 0;
    }
    a->guard_hold_input = hold_input != 0 ? 1 : 0;
    return 1;
}

int pdc3d_defense_press_parry(pdc3d_defense_world *d, int actor_id)
{
    if (d == 0) {
        return 0;
    }
    return gpry_press(&d->parry, actor_id);
}

int pdc3d_defense_set_block_resources(pdc3d_defense_world *d,
                                      int actor_id,
                                      int stamina,
                                      int posture)
{
    if (d == 0) {
        return 0;
    }
    return gblk_set_resources(&d->block, actor_id, stamina, posture);
}

int pdc3d_defense_set_guard_value(pdc3d_defense_world *d,
                                  int actor_id,
                                  int guard_value)
{
    if (d == 0) {
        return 0;
    }
    return ggrd_set_guard_value(&d->guard, actor_id, guard_value);
}

int pdc3d_defense_set_block_active(pdc3d_defense_world *d,
                                   int actor_id,
                                   int active)
{
    if (d == 0) {
        return 0;
    }
    return gblk_set_active(&d->block, actor_id, active);
}

int pdc3d_defense_set_shell_layer(pdc3d_defense_world *d,
                                  int actor_id,
                                  int layer_index,
                                  const GSHL_LayerProfile *profile,
                                  int start_full)
{
    if (d == 0) {
        return 0;
    }
    return gshl_set_layer(&d->shell, actor_id, layer_index, profile, start_full);
}

int pdc3d_defense_clear_shell_layer(pdc3d_defense_world *d,
                                    int actor_id,
                                    int layer_index)
{
    if (d == 0) {
        return 0;
    }
    return gshl_clear_layer(&d->shell, actor_id, layer_index);
}

void pdc3d_defense_update(pdc3d_defense_world *d,
                          int frames,
                          int current_frame)
{
    int i;
    if (d == 0 || frames <= 0) {
        return;
    }
    gblk_update(&d->block, frames);
    gpry_update(&d->parry, frames);
    gshl_update(&d->shell, frames);
    gctr_update(&d->counter, frames, current_frame);
    for (i = 0; i < PDC3D_MAX_DEFENSE_ACTORS; ++i) {
        if (d->actors[i].used != 0) {
            ggrd_update(&d->guard, frames, d->actors[i].actor_id,
                        d->actors[i].guard_hold_input);
        }
    }
}

int pdc3d_defense_resolve_damage(pdc3d_defense_world *d,
                                 pdc3d_damage_packet *io_packet,
                                 hb3_v3 source_dir,
                                 int current_frame,
                                 pdc3d_defense_result *out_result)
{
    const pdc3d_defense_actor *a;
    GPRY_Attack patk;
    GPRY_Result pry;
    GGRD_Attack gatk;
    GGRD_Result grd;
    GBLK_Attack batk;
    GBLK_Result blk;
    GSHL_Damage shd;
    GSHL_Result shl;
    int current_damage;
    unsigned int counter_result;
    if (d == 0 || io_packet == 0) {
        return 0;
    }
    pdc3d_def_result_init(out_result, io_packet);
    a = pdc3d_def_find_const(d, io_packet->defender_id);
    current_damage = io_packet->final_damage;
    if (a == 0 || a->enabled_flags == 0u) {
        if (out_result != 0) {
            out_result->defense_flags |= PDC3D_DEFRES_PASSED;
        }
        return 0;
    }

    if ((a->enabled_flags & PDC3D_DEF_PARRY) != 0u) {
        patk.attack_id = io_packet->attack_id;
        patk.attacker_id = io_packet->attacker_id;
        patk.damage = current_damage;
        patk.posture_damage = current_damage;
        patk.flags = pdc3d_def_attack_to_parry_flags(io_packet);
        patk.source_dir = pdc3d_to_gpry_v3(source_dir);
        if (gpry_try_parry(&d->parry, io_packet->defender_id, &patk, &pry) != 0) {
            current_damage = pry.damage_to_apply;
            io_packet->final_damage = current_damage;
            io_packet->result_flags |= PDC3D_DMG_DEFENDED;
            if (out_result != 0) {
                out_result->accepted = 1;
                out_result->final_damage = current_damage;
                out_result->parry_result = pry.result_code;
                out_result->attacker_stagger_frames = pry.attacker_stagger_frames;
                out_result->defender_stun_frames = pry.recovery_frames;
                out_result->dot = pry.dot;
                out_result->defense_flags |= PDC3D_DEFRES_ACCEPTED;
                if (pry.result_code == GPRY_RESULT_PERFECT) {
                    out_result->defense_flags |= PDC3D_DEFRES_PARRY_PERFECT;
                    io_packet->result_flags |= PDC3D_DMG_PARRY;
                } else if (pry.result_code == GPRY_RESULT_NORMAL) {
                    out_result->defense_flags |= PDC3D_DEFRES_PARRY_NORMAL;
                    io_packet->result_flags |= PDC3D_DMG_PARRY;
                } else {
                    out_result->defense_flags |= PDC3D_DEFRES_PARRY_LATE;
                    io_packet->result_flags |= PDC3D_DMG_PARRY_LATE;
                }
            }
            if (pry.result_code == GPRY_RESULT_PERFECT || pry.result_code == GPRY_RESULT_NORMAL) {
                counter_result = GCTR_DEF_OK;
                if (pry.result_code == GPRY_RESULT_PERFECT) {
                    counter_result |= GCTR_DEF_PERFECT;
                }
                pdc3d_def_feed_counter(d, io_packet, source_dir, current_frame,
                                       GCTR_SRC_PARRY, counter_result,
                                       out_result);
                return 1;
            }
        }
    }

    if ((a->enabled_flags & PDC3D_DEF_GUARD) != 0u && current_damage > 0) {
        gatk.attack_id = io_packet->attack_id;
        gatk.attacker_id = io_packet->attacker_id;
        gatk.damage = current_damage;
        gatk.guard_damage = current_damage;
        gatk.blockstun_frames = io_packet->stun_frames;
        gatk.flags = pdc3d_def_attack_to_guard_flags(io_packet);
        gatk.source_dir = pdc3d_to_ggrd_v3(source_dir);
        if (ggrd_try_guard(&d->guard, io_packet->defender_id, &gatk, &grd) != 0) {
            current_damage = grd.damage_to_apply;
            io_packet->final_damage = current_damage;
            io_packet->result_flags |= PDC3D_DMG_DEFENDED;
            if (out_result != 0) {
                out_result->accepted = 1;
                out_result->final_damage = current_damage;
                out_result->guard_damage = grd.guard_damage_taken;
                out_result->guard_broken = grd.guard_broken;
                out_result->blockstun_frames = grd.blockstun_frames;
                out_result->dot = grd.dot;
                out_result->defense_flags |= PDC3D_DEFRES_ACCEPTED;
                if (grd.guard_broken != 0) {
                    out_result->defense_flags |= PDC3D_DEFRES_GUARD_BROKEN;
                    io_packet->result_flags |= PDC3D_DMG_GUARD_BROKEN;
                } else {
                    out_result->chip_damage = current_damage;
                    out_result->defense_flags |= PDC3D_DEFRES_GUARDED;
                    io_packet->result_flags |= PDC3D_DMG_GUARDED;
                    pdc3d_def_feed_counter(d, io_packet, source_dir,
                                           current_frame, GCTR_SRC_GUARD,
                                           GCTR_DEF_OK, out_result);
                }
            }
        }
    }

    if ((a->enabled_flags & PDC3D_DEF_BLOCK) != 0u && current_damage > 0) {
        batk.attack_id = io_packet->attack_id;
        batk.attacker_id = io_packet->attacker_id;
        batk.damage = current_damage;
        batk.stamina_damage = current_damage;
        batk.posture_damage = current_damage;
        batk.blockstun_frames = io_packet->stun_frames;
        batk.breaker_power = current_damage / 2;
        batk.flags = (unsigned int)pdc3d_def_attack_to_block_flags(io_packet);
        batk.source_dir = pdc3d_to_gblk_v3(source_dir);
        if (gblk_try_block(&d->block, io_packet->defender_id, &batk, &blk) != 0) {
            current_damage = blk.damage_to_apply;
            io_packet->final_damage = current_damage;
            io_packet->result_flags |= PDC3D_DMG_DEFENDED;
            if (out_result != 0) {
                out_result->accepted = 1;
                out_result->final_damage = current_damage;
                out_result->stamina_damage = blk.stamina_cost;
                out_result->posture_damage = blk.posture_cost;
                out_result->block_broken = blk.broken;
                out_result->blockstun_frames = blk.blockstun_frames;
                out_result->recoil_frames = blk.recoil_frames;
                out_result->dot = blk.dot;
                out_result->defense_flags |= PDC3D_DEFRES_ACCEPTED;
                if (blk.broken != 0) {
                    out_result->defense_flags |= PDC3D_DEFRES_BLOCK_BROKEN;
                    io_packet->result_flags |= PDC3D_DMG_BLOCK_BROKEN;
                } else {
                    out_result->chip_damage = current_damage;
                    out_result->defense_flags |= PDC3D_DEFRES_BLOCKED;
                    io_packet->result_flags |= PDC3D_DMG_BLOCKED;
                    pdc3d_def_feed_counter(d, io_packet, source_dir,
                                           current_frame, GCTR_SRC_BLOCK,
                                           GCTR_DEF_OK, out_result);
                }
            }
        }
    }

    if ((a->enabled_flags & PDC3D_DEF_SHELL) != 0u && current_damage > 0) {
        shd.attack_id = io_packet->attack_id;
        shd.attacker_id = io_packet->attacker_id;
        shd.damage = current_damage;
        shd.damage_type = pdc3d_def_attack_to_shell_type(io_packet);
        shd.flags = 0u;
        if ((io_packet->attack_flags & PDC3D_ATK_PIERCE) != 0 &&
            (io_packet->material_flags & PDC3D_MAT_WEAKSPOT) != 0) {
            shd.flags |= GSHL_FLAG_BYPASS_SHELL;
        }
        shd.source_dir = pdc3d_to_gshl_v3(source_dir);
        if (gshl_apply_damage(&d->shell, io_packet->defender_id, &shd, &shl) != 0) {
            current_damage = shl.leaked_damage;
            io_packet->final_damage = current_damage;
            io_packet->result_flags |= PDC3D_DMG_DEFENDED | PDC3D_DMG_SHELL;
            if (out_result != 0) {
                out_result->accepted = 1;
                out_result->final_damage = current_damage;
                out_result->absorbed_damage += shl.absorbed;
                out_result->dot = shl.dot;
                out_result->defense_flags |= PDC3D_DEFRES_ACCEPTED;
                if (shl.absorbed > 0) {
                    out_result->defense_flags |= PDC3D_DEFRES_SHELL_ABSORB;
                    pdc3d_def_feed_counter(d, io_packet, source_dir,
                                           current_frame, GCTR_SRC_SHELL,
                                           GCTR_DEF_OK, out_result);
                }
                if (shl.broken_layer_index >= 0) {
                    out_result->shell_broken = 1;
                    out_result->defense_flags |= PDC3D_DEFRES_SHELL_BROKEN;
                    io_packet->result_flags |= PDC3D_DMG_SHELL_BROKEN;
                }
            }
        }
    }

    if (out_result != 0) {
        out_result->final_damage = current_damage;
        if (out_result->accepted == 0) {
            out_result->defense_flags |= PDC3D_DEFRES_PASSED;
        }
    }
    io_packet->final_damage = current_damage;
    return out_result != 0 ? out_result->accepted : 0;
}

int pdc3d_defense_try_counter(pdc3d_defense_world *d,
                              const pdc3d_counter_request *request,
                              int current_frame,
                              pdc3d_defense_result *out_result)
{
    GCTR_Request req;
    GCTR_Result cres;
    int ok;
    if (d == 0 || request == 0) {
        return 0;
    }
    pdc3d_def_result_init(out_result, 0);
    req.actor_id = request->actor_id;
    req.target_id = request->target_id;
    req.current_frame = current_frame;
    req.stamina_available = request->stamina_available;
    req.posture_available = request->posture_available;
    req.target_rel_pos = pdc3d_to_gctr_v3(request->target_rel_pos);
    ok = gctr_try_counter(&d->counter, &req, &cres);
    if (ok != 0 && out_result != 0) {
        out_result->accepted = 1;
        out_result->counter_used = 1;
        out_result->counter_kind = cres.counter_kind;
        out_result->counter_script_code = cres.script_code;
        out_result->defense_flags |= PDC3D_DEFRES_ACCEPTED |
                                     PDC3D_DEFRES_COUNTER_USED;
    }
    return ok;
}

int pdc3d_defense_count_counter_tokens(const pdc3d_defense_world *d,
                                       int actor_id)
{
    if (d == 0) {
        return 0;
    }
    return gctr_count_tokens(&d->counter, actor_id);
}
