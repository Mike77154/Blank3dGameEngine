#include "pdc3d_framedata.h"
#include <string.h>

FD3D_Fx pdc3d_fd_from_pdc_fx(hb3_fx v)
{
#if FD3D_FX_SHIFT >= HB3_FX_SHIFT
    return (FD3D_Fx)v * (FD3D_Fx)(1L << (FD3D_FX_SHIFT - HB3_FX_SHIFT));
#else
    return (FD3D_Fx)v / (FD3D_Fx)(1L << (HB3_FX_SHIFT - FD3D_FX_SHIFT));
#endif
}

hb3_fx pdc3d_fd_to_pdc_fx(FD3D_Fx v)
{
#if FD3D_FX_SHIFT >= HB3_FX_SHIFT
    return (hb3_fx)(v / (FD3D_Fx)(1L << (FD3D_FX_SHIFT - HB3_FX_SHIFT)));
#else
    return (hb3_fx)(v * (FD3D_Fx)(1L << (HB3_FX_SHIFT - FD3D_FX_SHIFT)));
#endif
}

FD3D_Volume pdc3d_fd_sphere_from_pdc(hb3_v3 center, hb3_fx radius,
                                      unsigned int mask,
                                      unsigned int group,
                                      unsigned int lane_mask)
{
    FD3D_Volume v;
    v.type = FD3D_VOL_SPHERE;
    v.x1 = pdc3d_fd_from_pdc_fx(center.x);
    v.y1 = pdc3d_fd_from_pdc_fx(center.y);
    v.z1 = pdc3d_fd_from_pdc_fx(center.z);
    v.x2 = v.x1;
    v.y2 = v.y1;
    v.z2 = v.z1;
    v.radius = pdc3d_fd_from_pdc_fx(radius);
    v.mask = mask;
    v.group = group;
    v.lane_mask = lane_mask;
    return v;
}

FD3D_Volume pdc3d_fd_aabb_from_ints(int x1, int y1, int z1,
                                    int x2, int y2, int z2,
                                    unsigned int mask,
                                    unsigned int group,
                                    unsigned int lane_mask)
{
    FD3D_Volume v;
    FD3D_VolumeMakeAabb(&v, x1, y1, z1, x2, y2, z2, mask, group, lane_mask);
    return v;
}

unsigned int pdc3d_fd_attack_flags_to_attr(int attack_flags)
{
    unsigned int attr;
    attr = 0u;
    if ((attack_flags & PDC3D_ATK_GRAB) != 0) {
        attr |= FD3D_ATTR_THROW;
    }
    if ((attack_flags & (PDC3D_ATK_SLASH | PDC3D_ATK_BLUNT |
                         PDC3D_ATK_BITE | PDC3D_ATK_PIERCE |
                         PDC3D_ATK_FIRE | PDC3D_ATK_ACID |
                         PDC3D_ATK_EXPLOSIVE | PDC3D_ATK_KNOCKDOWN)) != 0) {
        attr |= FD3D_ATTR_STRIKE;
    }
    if (attr == 0u) {
        attr = FD3D_ATTR_STRIKE;
    }
    return attr;
}

void pdc3d_framedata_result_init(pdc3d_framedata_result *r)
{
    if (r == 0) {
        return;
    }
    memset(r, 0, sizeof(*r));
    r->can_hit = 1;
    r->result_flags = PDC3D_FDRES_CAN_HIT;
}

void pdc3d_framedata_world_init(pdc3d_framedata_world *fw)
{
    if (fw == 0) {
        return;
    }
    memset(fw, 0, sizeof(*fw));
}

FD3D_Entity *pdc3d_framedata_actor_find(pdc3d_framedata_world *fw,
                                        int actor_id)
{
    int i;
    if (fw == 0) {
        return 0;
    }
    for (i = 0; i < PDC3D_MAX_FRAMEDATA_ACTORS; ++i) {
        if (fw->actors[i].used != 0 && fw->actors[i].actor_id == actor_id) {
            return &fw->actors[i].fd;
        }
    }
    return 0;
}

const FD3D_Entity *pdc3d_framedata_actor_find_const(const pdc3d_framedata_world *fw,
                                                    int actor_id)
{
    int i;
    if (fw == 0) {
        return 0;
    }
    for (i = 0; i < PDC3D_MAX_FRAMEDATA_ACTORS; ++i) {
        if (fw->actors[i].used != 0 && fw->actors[i].actor_id == actor_id) {
            return &fw->actors[i].fd;
        }
    }
    return 0;
}

int pdc3d_framedata_actor_create(pdc3d_framedata_world *fw,
                                 int actor_id,
                                 int team,
                                 int hp)
{
    int i;
    FD3D_Entity *existing;
    if (fw == 0) {
        return 0;
    }
    existing = pdc3d_framedata_actor_find(fw, actor_id);
    if (existing != 0) {
        existing->team = team;
        existing->hp = hp;
        return 1;
    }
    for (i = 0; i < PDC3D_MAX_FRAMEDATA_ACTORS; ++i) {
        if (fw->actors[i].used == 0) {
            fw->actors[i].used = 1;
            fw->actors[i].actor_id = actor_id;
            FD3D_EntityInit(&fw->actors[i].fd, (unsigned int)actor_id, team, hp);
            return 1;
        }
    }
    return 0;
}

int pdc3d_framedata_actor_remove(pdc3d_framedata_world *fw,
                                 int actor_id)
{
    int i;
    if (fw == 0) {
        return 0;
    }
    for (i = 0; i < PDC3D_MAX_FRAMEDATA_ACTORS; ++i) {
        if (fw->actors[i].used != 0 && fw->actors[i].actor_id == actor_id) {
            memset(&fw->actors[i], 0, sizeof(fw->actors[i]));
            return 1;
        }
    }
    return 0;
}

int pdc3d_framedata_actor_set_action(pdc3d_framedata_world *fw,
                                     int actor_id,
                                     const pdc3d_fd_action *action)
{
    FD3D_Entity *e;
    e = pdc3d_framedata_actor_find(fw, actor_id);
    if (e == 0 || action == 0) {
        return 0;
    }
    return FD3D_SetAction(e, action) == FD3D_OK ? 1 : 0;
}

int pdc3d_framedata_actor_set_pose(pdc3d_framedata_world *fw,
                                   int actor_id,
                                   hb3_v3 pos,
                                   int facing,
                                   unsigned int lane_mask)
{
    FD3D_Entity *e;
    e = pdc3d_framedata_actor_find(fw, actor_id);
    if (e == 0) {
        return 0;
    }
    e->pos_x = pdc3d_fd_from_pdc_fx(pos.x);
    e->pos_y = pdc3d_fd_from_pdc_fx(pos.y);
    e->pos_z = pdc3d_fd_from_pdc_fx(pos.z);
    e->facing = facing;
    e->lane_mask = lane_mask;
    return 1;
}

int pdc3d_framedata_actor_set_nothitby(pdc3d_framedata_world *fw,
                                       int actor_id,
                                       unsigned int attr_mask,
                                       int frames)
{
    FD3D_Entity *e;
    e = pdc3d_framedata_actor_find(fw, actor_id);
    if (e == 0) {
        return 0;
    }
    FD3D_SetNotHitBy(e, attr_mask, frames);
    return 1;
}

int pdc3d_framedata_actor_set_hitby(pdc3d_framedata_world *fw,
                                    int actor_id,
                                    unsigned int attr_mask,
                                    int frames)
{
    FD3D_Entity *e;
    e = pdc3d_framedata_actor_find(fw, actor_id);
    if (e == 0) {
        return 0;
    }
    FD3D_SetHitBy(e, attr_mask, frames);
    return 1;
}

int pdc3d_framedata_actor_tick(pdc3d_framedata_world *fw,
                               int actor_id,
                               const pdc3d_fd_callbacks *callbacks,
                               void *user)
{
    FD3D_Entity *e;
    e = pdc3d_framedata_actor_find(fw, actor_id);
    if (e == 0) {
        return 0;
    }
    return FD3D_Tick(e, callbacks, user) == FD3D_OK ? 1 : 0;
}

void pdc3d_framedata_tick_all(pdc3d_framedata_world *fw,
                              const pdc3d_fd_callbacks *callbacks,
                              void *user)
{
    int i;
    if (fw == 0) {
        return;
    }
    for (i = 0; i < PDC3D_MAX_FRAMEDATA_ACTORS; ++i) {
        if (fw->actors[i].used != 0) {
            FD3D_Tick(&fw->actors[i].fd, callbacks, user);
        }
    }
}

int pdc3d_framedata_actor_is_done(const pdc3d_framedata_world *fw,
                                  int actor_id)
{
    const FD3D_Entity *e;
    e = pdc3d_framedata_actor_find_const(fw, actor_id);
    if (e == 0) {
        return 1;
    }
    return FD3D_IsActionDone(e);
}

int pdc3d_framedata_actor_frame_info(const pdc3d_framedata_world *fw,
                                     int actor_id,
                                     unsigned int *out_action_id,
                                     unsigned short *out_frame_index,
                                     unsigned short *out_frame_tick,
                                     unsigned short *out_total_tick,
                                     unsigned long *out_frame_flags)
{
    const FD3D_Entity *e;
    const FD3D_Frame *f;
    e = pdc3d_framedata_actor_find_const(fw, actor_id);
    if (e == 0) {
        return 0;
    }
    f = FD3D_CurrentFrame(e);
    if (out_action_id != 0) {
        *out_action_id = e->action != 0 ? e->action->id : 0u;
    }
    if (out_frame_index != 0) {
        *out_frame_index = e->frame_index;
    }
    if (out_frame_tick != 0) {
        *out_frame_tick = e->frame_tick;
    }
    if (out_total_tick != 0) {
        *out_total_tick = e->total_tick;
    }
    if (out_frame_flags != 0) {
        *out_frame_flags = f != 0 ? f->flags : 0ul;
    }
    return 1;
}

static void pdc3d_framedata_mark_defender_flags(const FD3D_Entity *defender,
                                                unsigned int attr,
                                                pdc3d_framedata_result *r)
{
    const FD3D_Frame *f;
    unsigned long flags;
    if (defender == 0 || r == 0) {
        return;
    }
    f = FD3D_CurrentFrame(defender);
    flags = f != 0 ? f->flags : 0ul;
    if ((flags & FD3D_FLAG_COUNTER_WINDOW) != 0ul) {
        r->counter_window = 1;
        r->result_flags |= PDC3D_FDRES_COUNTER_WINDOW;
    }
    if ((flags & FD3D_FLAG_ARMOR) != 0ul) {
        r->armor = 1;
        r->result_flags |= PDC3D_FDRES_ARMOR;
    }
    if ((flags & FD3D_FLAG_SUPER_ARMOR) != 0ul) {
        r->super_armor = 1;
        r->result_flags |= PDC3D_FDRES_SUPER_ARMOR;
    }
    if ((flags & FD3D_FLAG_INVINCIBLE) != 0ul) {
        r->can_hit = 0;
        r->result_flags &= ~PDC3D_FDRES_CAN_HIT;
        r->result_flags |= PDC3D_FDRES_INVINCIBLE;
    }
    if ((flags & FD3D_FLAG_STRIKE_INVULN) != 0ul && (attr & FD3D_ATTR_STRIKE) != 0u) {
        r->can_hit = 0;
        r->result_flags &= ~PDC3D_FDRES_CAN_HIT;
        r->result_flags |= PDC3D_FDRES_STRIKE_INVULN;
    }
    if ((flags & FD3D_FLAG_PROJECTILE_INVULN) != 0ul && (attr & FD3D_ATTR_PROJECTILE) != 0u) {
        r->can_hit = 0;
        r->result_flags &= ~PDC3D_FDRES_CAN_HIT;
        r->result_flags |= PDC3D_FDRES_PROJECTILE_INVULN;
    }
    if ((flags & FD3D_FLAG_THROW_INVULN) != 0ul && (attr & FD3D_ATTR_THROW) != 0u) {
        r->can_hit = 0;
        r->result_flags &= ~PDC3D_FDRES_CAN_HIT;
        r->result_flags |= PDC3D_FDRES_THROW_INVULN;
    }
    if ((flags & FD3D_FLAG_SIDE_STEP) != 0ul && (attr & FD3D_ATTR_HOMING) == 0u) {
        r->can_hit = 0;
        r->side_step_evaded = 1;
        r->result_flags &= ~PDC3D_FDRES_CAN_HIT;
        r->result_flags |= PDC3D_FDRES_SIDE_STEP_EVADE;
    }
    if ((flags & FD3D_FLAG_CAN_CANCEL) != 0ul) {
        r->result_flags |= PDC3D_FDRES_CAN_CANCEL;
    }
}

int pdc3d_framedata_check_damage(const pdc3d_framedata_world *fw,
                                 int defender_id,
                                 int attack_flags,
                                 pdc3d_framedata_result *out_result)
{
    const FD3D_Entity *defender;
    unsigned int attr;
    pdc3d_framedata_result local;
    pdc3d_framedata_result *r;
    r = out_result != 0 ? out_result : &local;
    pdc3d_framedata_result_init(r);
    if (fw == 0) {
        return 0;
    }
    defender = pdc3d_framedata_actor_find_const(fw, defender_id);
    if (defender == 0) {
        return 0;
    }
    r->defender_registered = 1;
    r->result_flags |= PDC3D_FDRES_HAS_DEFENDER;
    attr = pdc3d_fd_attack_flags_to_attr(attack_flags);
    r->attr_mask = attr;
    pdc3d_framedata_mark_defender_flags(defender, attr, r);
    if (FD3D_CanBeHit(defender, attr) == FD3D_FALSE) {
        r->can_hit = 0;
        r->result_flags &= ~PDC3D_FDRES_CAN_HIT;
    }
    return 1;
}

static void pdc3d_framedata_copy_hitdef_to_packet(const FD3D_HitDef *hd,
                                                  pdc3d_damage_packet *p,
                                                  pdc3d_framedata_result *r)
{
    if (hd == 0 || p == 0) {
        return;
    }
    p->base_damage = hd->damage;
    p->final_damage = hd->damage;
    p->stun_frames = hd->hitstun_frames;
    p->hitstop_frames = hd->hitstop_frames;
    if (r != 0) {
        r->hitstop_frames = hd->hitstop_frames;
        r->hitstun_frames = hd->hitstun_frames;
        r->blockstop_frames = hd->blockstop_frames;
        r->blockstun_frames = hd->blockstun_frames;
        r->cancel_on_hit_mask = hd->cancel_on_hit_mask;
        r->cancel_on_block_mask = hd->cancel_on_block_mask;
        r->attack_hit_id = hd->hit_id;
        if (hd->hitstop_frames > 0 || hd->blockstop_frames > 0) {
            r->result_flags |= PDC3D_FDRES_HITSTOP;
        }
    }
}

int pdc3d_framedata_resolve_damage(pdc3d_framedata_world *fw,
                                   int attacker_id,
                                   int defender_id,
                                   int was_blocked,
                                   pdc3d_damage_packet *io_packet,
                                   pdc3d_framedata_result *out_result)
{
    FD3D_Entity *attacker;
    FD3D_Entity *defender;
    const FD3D_Frame *af;
    const FD3D_Frame *df;
    const FD3D_HitDef *hd;
    unsigned int attr;
    int adv;
    pdc3d_framedata_result local;
    pdc3d_framedata_result *r;
    int saved_base_damage;
    int saved_final_damage;
    int saved_stun_frames;
    int saved_hitstop_frames;
    r = out_result != 0 ? out_result : &local;
    pdc3d_framedata_result_init(r);
    if (fw == 0 || io_packet == 0) {
        return 0;
    }
    saved_base_damage = io_packet->base_damage;
    saved_final_damage = io_packet->final_damage;
    saved_stun_frames = io_packet->stun_frames;
    saved_hitstop_frames = io_packet->hitstop_frames;
    attacker = pdc3d_framedata_actor_find(fw, attacker_id);
    defender = pdc3d_framedata_actor_find(fw, defender_id);
    if (attacker != 0) {
        r->attacker_registered = 1;
        r->result_flags |= PDC3D_FDRES_HAS_ATTACKER;
    }
    if (defender != 0) {
        r->defender_registered = 1;
        r->result_flags |= PDC3D_FDRES_HAS_DEFENDER;
    }
    attr = pdc3d_fd_attack_flags_to_attr(io_packet->attack_flags);
    r->attr_mask = attr;

    if (attacker != 0) {
        af = FD3D_CurrentFrame(attacker);
        hd = af != 0 ? af->hit_def : 0;
        if (hd == 0) {
            r->can_hit = 0;
            r->attack_active = 0;
            r->result_flags &= ~PDC3D_FDRES_CAN_HIT;
            r->result_flags |= PDC3D_FDRES_ATTACK_INACTIVE;
            io_packet->final_damage = 0;
            io_packet->result_flags |= PDC3D_DMG_NO_DAMAGE;
            return 1;
        }
        r->attack_active = 1;
        r->result_flags |= PDC3D_FDRES_ATTACK_ACTIVE;
        attr = hd->attr_mask != 0u ? hd->attr_mask : attr;
        r->attr_mask = attr;
        pdc3d_framedata_copy_hitdef_to_packet(hd, io_packet, r);
        if (was_blocked >= 0) {
            io_packet->base_damage = saved_base_damage;
            io_packet->final_damage = saved_final_damage;
            io_packet->stun_frames = saved_stun_frames;
            io_packet->hitstop_frames = saved_hitstop_frames;
        }
    } else {
        hd = 0;
    }

    if (defender != 0) {
        pdc3d_framedata_mark_defender_flags(defender, attr, r);
        if (FD3D_CanBeHit(defender, attr) == FD3D_FALSE) {
            r->can_hit = 0;
            r->result_flags &= ~PDC3D_FDRES_CAN_HIT;
            io_packet->final_damage = 0;
            io_packet->result_flags |= PDC3D_DMG_NO_DAMAGE;
            return 1;
        }
    }

    if (was_blocked >= 0 && attacker != 0 && defender != 0 && hd != 0) {
        df = FD3D_CurrentFrame(defender);
        if (was_blocked != 0) {
            defender->blockstun_timer = hd->blockstun_frames;
            defender->hitstop_timer = hd->blockstop_frames;
            attacker->hitstop_timer = hd->blockstop_frames;
            r->blockstun_frames = hd->blockstun_frames;
            r->blockstop_frames = hd->blockstop_frames;
            r->result_flags |= PDC3D_FDRES_BLOCKSTUN | PDC3D_FDRES_CONTACT_APPLIED;
            adv = FD3D_FrameAdvantageAfterContact(attacker->action,
                                                  attacker->frame_index,
                                                  attacker->frame_tick,
                                                  hd->blockstun_frames);
            r->frame_advantage_block = adv;
        } else {
            if (df == 0 || ((df->flags & (FD3D_FLAG_ARMOR | FD3D_FLAG_SUPER_ARMOR)) == 0ul)) {
                defender->hitstun_timer = hd->hitstun_frames;
                r->result_flags |= PDC3D_FDRES_HITSTUN;
            }
            defender->hitstop_timer = hd->hitstop_frames;
            attacker->hitstop_timer = hd->hitstop_frames;
            defender->last_taken_hit_id = hd->hit_id;
            r->result_flags |= PDC3D_FDRES_CONTACT_APPLIED;
            adv = FD3D_FrameAdvantageAfterContact(attacker->action,
                                                  attacker->frame_index,
                                                  attacker->frame_tick,
                                                  hd->hitstun_frames);
            r->frame_advantage_hit = adv;
        }
    }
    return 1;
}

int pdc3d_framedata_frame_advantage(const pdc3d_framedata_world *fw,
                                    int attacker_id,
                                    int use_blockstun)
{
    const FD3D_Entity *attacker;
    const FD3D_Frame *frame;
    const FD3D_HitDef *hd;
    int stun;
    attacker = pdc3d_framedata_actor_find_const(fw, attacker_id);
    if (attacker == 0) {
        return 0;
    }
    frame = FD3D_CurrentFrame(attacker);
    if (frame == 0 || frame->hit_def == 0) {
        return 0;
    }
    hd = frame->hit_def;
    stun = use_blockstun != 0 ? hd->blockstun_frames : hd->hitstun_frames;
    return FD3D_FrameAdvantageAfterContact(attacker->action,
                                           attacker->frame_index,
                                           attacker->frame_tick,
                                           stun);
}
