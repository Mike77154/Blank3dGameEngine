#include "fd3d89.h"

static int fd3d_is_power_of_two_size(size_t value)
{
    return (value != 0u && (value & (value - 1u)) == 0u) ? FD3D_TRUE : FD3D_FALSE;
}

static size_t fd3d_align_up(size_t value, size_t align)
{
    size_t mask;
    mask = align - 1u;
    return (value + mask) & ~mask;
}

static FD3D_Fx fd3d_min_fx(FD3D_Fx a, FD3D_Fx b)
{
    return (a < b) ? a : b;
}

static FD3D_Fx fd3d_max_fx(FD3D_Fx a, FD3D_Fx b)
{
    return (a > b) ? a : b;
}

static void fd3d_point_to_world(FD3D_Fx lx, FD3D_Fx ly, FD3D_Fx lz, FD3D_Fx px, FD3D_Fx py, FD3D_Fx pz, int facing, FD3D_Fx *ox, FD3D_Fx *oy, FD3D_Fx *oz)
{
    FD3D_Fx wx;
    FD3D_Fx wz;
    if (facing == FD3D_FACE_X_POS) {
        wx = px + lz;
        wz = pz - lx;
    } else if (facing == FD3D_FACE_Z_NEG) {
        wx = px - lx;
        wz = pz - lz;
    } else if (facing == FD3D_FACE_X_NEG) {
        wx = px - lz;
        wz = pz + lx;
    } else {
        wx = px + lx;
        wz = pz + lz;
    }
    *ox = wx;
    *oy = py + ly;
    *oz = wz;
}

void FD3D_ArenaInit(FD3D_Arena *arena, void *memory, size_t capacity)
{
    if (arena == 0) {
        return;
    }
    arena->mem = (unsigned char *)memory;
    arena->cap = capacity;
    arena->used = 0u;
}

void *FD3D_ArenaAlloc(FD3D_Arena *arena, size_t size, size_t align)
{
    size_t at;
    size_t next;
    if (arena == 0 || arena->mem == 0 || size == 0u) {
        return 0;
    }
    if (align == 0u) {
        align = sizeof(long);
    }
    if (fd3d_is_power_of_two_size(align) == FD3D_FALSE) {
        return 0;
    }
    at = fd3d_align_up(arena->used, align);
    next = at + size;
    if (next < at || next > arena->cap) {
        return 0;
    }
    arena->used = next;
    return (void *)(arena->mem + at);
}

void FD3D_ArenaReset(FD3D_Arena *arena)
{
    if (arena != 0) {
        arena->used = 0u;
    }
}

int FD3D_VolumeMakeAabb(FD3D_Volume *volume, int x1, int y1, int z1, int x2, int y2, int z2, unsigned int mask, unsigned int group, unsigned int lane_mask)
{
    int minx;
    int maxx;
    int miny;
    int maxy;
    int minz;
    int maxz;
    if (volume == 0) {
        return FD3D_ERR_NULL;
    }
    minx = (x1 < x2) ? x1 : x2;
    maxx = (x1 < x2) ? x2 : x1;
    miny = (y1 < y2) ? y1 : y2;
    maxy = (y1 < y2) ? y2 : y1;
    minz = (z1 < z2) ? z1 : z2;
    maxz = (z1 < z2) ? z2 : z1;
    volume->type = FD3D_VOL_AABB;
    volume->x1 = FD3D_FX_FROM_INT(minx);
    volume->y1 = FD3D_FX_FROM_INT(miny);
    volume->z1 = FD3D_FX_FROM_INT(minz);
    volume->x2 = FD3D_FX_FROM_INT(maxx);
    volume->y2 = FD3D_FX_FROM_INT(maxy);
    volume->z2 = FD3D_FX_FROM_INT(maxz);
    volume->radius = 0;
    volume->mask = mask;
    volume->group = group;
    volume->lane_mask = lane_mask;
    return FD3D_OK;
}

int FD3D_VolumeMakeSphere(FD3D_Volume *volume, int x, int y, int z, int radius, unsigned int mask, unsigned int group, unsigned int lane_mask)
{
    if (volume == 0) {
        return FD3D_ERR_NULL;
    }
    if (radius < 0) {
        radius = -radius;
    }
    volume->type = FD3D_VOL_SPHERE;
    volume->x1 = FD3D_FX_FROM_INT(x);
    volume->y1 = FD3D_FX_FROM_INT(y);
    volume->z1 = FD3D_FX_FROM_INT(z);
    volume->x2 = volume->x1;
    volume->y2 = volume->y1;
    volume->z2 = volume->z1;
    volume->radius = FD3D_FX_FROM_INT(radius);
    volume->mask = mask;
    volume->group = group;
    volume->lane_mask = lane_mask;
    return FD3D_OK;
}

int FD3D_EntityInit(FD3D_Entity *entity, unsigned int id, int team, int hp)
{
    if (entity == 0) {
        return FD3D_ERR_NULL;
    }
    entity->entity_id = id;
    entity->team = team;
    entity->pos_x = 0;
    entity->pos_y = 0;
    entity->pos_z = 0;
    entity->facing = FD3D_FACE_Z_POS;
    entity->hp = hp;
    entity->lane_mask = FD3D_LANE_CENTER;
    entity->action = 0;
    entity->frame_index = 0u;
    entity->frame_tick = 0u;
    entity->total_tick = 0u;
    entity->hitstop_timer = 0;
    entity->hitstun_timer = 0;
    entity->blockstun_timer = 0;
    entity->hitby_mask = 0u;
    entity->hitby_timer = 0;
    entity->nothitby_mask = 0u;
    entity->nothitby_timer = 0;
    entity->last_taken_hit_id = 0u;
    entity->user_flags = 0ul;
    entity->user = 0;
    return FD3D_OK;
}

int FD3D_SetAction(FD3D_Entity *entity, const FD3D_Action *action)
{
    if (entity == 0 || action == 0 || action->frames == 0 || action->frame_count == 0u) {
        return FD3D_ERR_NULL;
    }
    entity->action = action;
    entity->frame_index = 0u;
    entity->frame_tick = 0u;
    entity->total_tick = 0u;
    entity->last_taken_hit_id = 0u;
    return FD3D_OK;
}

const FD3D_Frame *FD3D_CurrentFrame(const FD3D_Entity *entity)
{
    if (entity == 0 || entity->action == 0 || entity->action->frames == 0) {
        return 0;
    }
    if (entity->frame_index >= entity->action->frame_count) {
        return 0;
    }
    return &entity->action->frames[entity->frame_index];
}

int FD3D_TickTimers(FD3D_Entity *entity)
{
    if (entity == 0) {
        return FD3D_ERR_NULL;
    }
    if (entity->hitstop_timer > 0) {
        entity->hitstop_timer -= 1;
    }
    if (entity->hitstun_timer > 0) {
        entity->hitstun_timer -= 1;
    }
    if (entity->blockstun_timer > 0) {
        entity->blockstun_timer -= 1;
    }
    if (entity->hitby_timer > 0) {
        entity->hitby_timer -= 1;
        if (entity->hitby_timer == 0) {
            entity->hitby_mask = 0u;
        }
    }
    if (entity->nothitby_timer > 0) {
        entity->nothitby_timer -= 1;
        if (entity->nothitby_timer == 0) {
            entity->nothitby_mask = 0u;
        }
    }
    return FD3D_OK;
}

int FD3D_Tick(FD3D_Entity *entity, const FD3D_Callbacks *callbacks, void *user)
{
    const FD3D_Frame *frame;
    if (entity == 0 || entity->action == 0) {
        return FD3D_ERR_NULL;
    }
    FD3D_TickTimers(entity);
    if (entity->hitstop_timer > 0) {
        return FD3D_OK;
    }
    frame = FD3D_CurrentFrame(entity);
    if (frame == 0) {
        return FD3D_ERR_BAD_ACTION;
    }
    entity->frame_tick = (unsigned short)(entity->frame_tick + 1u);
    entity->total_tick = (unsigned short)(entity->total_tick + 1u);
    if (frame->duration == 0u || entity->frame_tick >= frame->duration) {
        entity->frame_tick = 0u;
        if ((unsigned int)entity->frame_index + 1u >= entity->action->frame_count) {
            if (entity->action->loop != 0u) {
                entity->frame_index = 0u;
                entity->total_tick = 0u;
            } else {
                if (callbacks != 0 && callbacks->on_action_end != 0) {
                    callbacks->on_action_end(entity, entity->action, user);
                }
                return FD3D_OK;
            }
        } else {
            entity->frame_index = (unsigned short)(entity->frame_index + 1u);
        }
        frame = FD3D_CurrentFrame(entity);
        if (callbacks != 0 && callbacks->on_frame_enter != 0 && frame != 0) {
            callbacks->on_frame_enter(entity, frame, user);
        }
    }
    return FD3D_OK;
}

void FD3D_SetHitBy(FD3D_Entity *entity, unsigned int mask, int frames)
{
    if (entity == 0) {
        return;
    }
    entity->hitby_mask = mask;
    entity->hitby_timer = frames;
}

void FD3D_SetNotHitBy(FD3D_Entity *entity, unsigned int mask, int frames)
{
    if (entity == 0) {
        return;
    }
    entity->nothitby_mask = mask;
    entity->nothitby_timer = frames;
}

int FD3D_CanBeHit(const FD3D_Entity *defender, unsigned int attr_mask)
{
    const FD3D_Frame *frame;
    unsigned long flags;
    if (defender == 0) {
        return FD3D_FALSE;
    }
    frame = FD3D_CurrentFrame(defender);
    flags = 0ul;
    if (frame != 0) {
        flags = frame->flags;
    }
    if ((flags & FD3D_FLAG_INVINCIBLE) != 0ul) {
        return FD3D_FALSE;
    }
    if ((flags & FD3D_FLAG_STRIKE_INVULN) != 0ul && (attr_mask & FD3D_ATTR_STRIKE) != 0u) {
        return FD3D_FALSE;
    }
    if ((flags & FD3D_FLAG_PROJECTILE_INVULN) != 0ul && (attr_mask & FD3D_ATTR_PROJECTILE) != 0u) {
        return FD3D_FALSE;
    }
    if ((flags & FD3D_FLAG_THROW_INVULN) != 0ul && (attr_mask & FD3D_ATTR_THROW) != 0u) {
        return FD3D_FALSE;
    }
    if ((flags & FD3D_FLAG_SIDE_STEP) != 0ul && (attr_mask & FD3D_ATTR_HOMING) == 0u) {
        return FD3D_FALSE;
    }
    if (defender->hitby_timer > 0 && (attr_mask & defender->hitby_mask) == 0u) {
        return FD3D_FALSE;
    }
    if (defender->nothitby_timer > 0 && (attr_mask & defender->nothitby_mask) != 0u) {
        return FD3D_FALSE;
    }
    return FD3D_TRUE;
}

FD3D_Volume FD3D_WorldVolume(const FD3D_Volume *local_volume, FD3D_Fx pos_x, FD3D_Fx pos_y, FD3D_Fx pos_z, int facing)
{
    FD3D_Volume out;
    FD3D_Fx xs[2];
    FD3D_Fx ys[2];
    FD3D_Fx zs[2];
    FD3D_Fx wx;
    FD3D_Fx wy;
    FD3D_Fx wz;
    int ix;
    int iy;
    int iz;
    if (local_volume == 0) {
        out.type = FD3D_VOL_AABB;
        out.x1 = 0;
        out.y1 = 0;
        out.z1 = 0;
        out.x2 = 0;
        out.y2 = 0;
        out.z2 = 0;
        out.radius = 0;
        out.mask = 0u;
        out.group = 0u;
        out.lane_mask = 0u;
        return out;
    }
    out = *local_volume;
    if (local_volume->type == FD3D_VOL_SPHERE) {
        fd3d_point_to_world(local_volume->x1, local_volume->y1, local_volume->z1, pos_x, pos_y, pos_z, facing, &wx, &wy, &wz);
        out.x1 = wx - local_volume->radius;
        out.y1 = wy - local_volume->radius;
        out.z1 = wz - local_volume->radius;
        out.x2 = wx + local_volume->radius;
        out.y2 = wy + local_volume->radius;
        out.z2 = wz + local_volume->radius;
        out.type = FD3D_VOL_AABB;
        return out;
    }
    xs[0] = local_volume->x1;
    xs[1] = local_volume->x2;
    ys[0] = local_volume->y1;
    ys[1] = local_volume->y2;
    zs[0] = local_volume->z1;
    zs[1] = local_volume->z2;
    fd3d_point_to_world(xs[0], ys[0], zs[0], pos_x, pos_y, pos_z, facing, &wx, &wy, &wz);
    out.x1 = wx;
    out.x2 = wx;
    out.y1 = wy;
    out.y2 = wy;
    out.z1 = wz;
    out.z2 = wz;
    ix = 0;
    while (ix < 2) {
        iy = 0;
        while (iy < 2) {
            iz = 0;
            while (iz < 2) {
                fd3d_point_to_world(xs[ix], ys[iy], zs[iz], pos_x, pos_y, pos_z, facing, &wx, &wy, &wz);
                out.x1 = fd3d_min_fx(out.x1, wx);
                out.x2 = fd3d_max_fx(out.x2, wx);
                out.y1 = fd3d_min_fx(out.y1, wy);
                out.y2 = fd3d_max_fx(out.y2, wy);
                out.z1 = fd3d_min_fx(out.z1, wz);
                out.z2 = fd3d_max_fx(out.z2, wz);
                iz += 1;
            }
            iy += 1;
        }
        ix += 1;
    }
    out.type = FD3D_VOL_AABB;
    return out;
}

int FD3D_VolumeOverlap(const FD3D_Volume *a, const FD3D_Volume *b)
{
    if (a == 0 || b == 0) {
        return FD3D_FALSE;
    }
    if (a->x2 < b->x1) {
        return FD3D_FALSE;
    }
    if (b->x2 < a->x1) {
        return FD3D_FALSE;
    }
    if (a->y2 < b->y1) {
        return FD3D_FALSE;
    }
    if (b->y2 < a->y1) {
        return FD3D_FALSE;
    }
    if (a->z2 < b->z1) {
        return FD3D_FALSE;
    }
    if (b->z2 < a->z1) {
        return FD3D_FALSE;
    }
    return FD3D_TRUE;
}

int FD3D_CheckHit(const FD3D_Entity *attacker, const FD3D_Entity *defender, FD3D_HitResult *out_hit)
{
    const FD3D_Frame *af;
    const FD3D_Frame *df;
    const FD3D_HitDef *hd;
    unsigned short i;
    unsigned short j;
    FD3D_Volume aw;
    FD3D_Volume dw;
    if (out_hit != 0) {
        out_hit->did_hit = FD3D_FALSE;
        out_hit->was_blocked = FD3D_FALSE;
        out_hit->was_counter = FD3D_FALSE;
        out_hit->was_side_step_evaded = FD3D_FALSE;
        out_hit->attacker_frame = 0u;
        out_hit->defender_frame = 0u;
        out_hit->attack_volume = 0;
        out_hit->vulnerable_volume = 0;
        out_hit->hit_def = 0;
    }
    if (attacker == 0 || defender == 0 || out_hit == 0) {
        return FD3D_ERR_NULL;
    }
    af = FD3D_CurrentFrame(attacker);
    df = FD3D_CurrentFrame(defender);
    if (af == 0 || df == 0 || af->hit_def == 0) {
        return FD3D_OK;
    }
    hd = af->hit_def;
    if (attacker->team == defender->team) {
        return FD3D_OK;
    }
    if ((hd->lane_mask & defender->lane_mask) == 0u && (hd->attr_mask & FD3D_ATTR_HOMING) == 0u) {
        return FD3D_OK;
    }
    if (attacker->hitstop_timer > 0 || defender->hitstop_timer > 0) {
        return FD3D_OK;
    }
    if (defender->last_taken_hit_id == hd->hit_id && hd->hit_id != 0u) {
        return FD3D_OK;
    }
    if (FD3D_CanBeHit(defender, hd->attr_mask) == FD3D_FALSE) {
        if ((df->flags & FD3D_FLAG_SIDE_STEP) != 0ul && (hd->attr_mask & FD3D_ATTR_HOMING) == 0u) {
            out_hit->was_side_step_evaded = FD3D_TRUE;
        }
        return FD3D_OK;
    }
    if (af->attack_count == 0u || df->vulnerable_count == 0u) {
        return FD3D_OK;
    }
    i = 0u;
    while (i < af->attack_count) {
        if ((af->attack_volumes[i].lane_mask & defender->lane_mask) != 0u || (hd->attr_mask & FD3D_ATTR_HOMING) != 0u) {
            aw = FD3D_WorldVolume(&af->attack_volumes[i], attacker->pos_x, attacker->pos_y, attacker->pos_z, attacker->facing);
            j = 0u;
            while (j < df->vulnerable_count) {
                if ((af->attack_volumes[i].mask & df->vulnerable_volumes[j].mask) != 0u) {
                    dw = FD3D_WorldVolume(&df->vulnerable_volumes[j], defender->pos_x, defender->pos_y, defender->pos_z, defender->facing);
                    if (FD3D_VolumeOverlap(&aw, &dw) != FD3D_FALSE) {
                        out_hit->did_hit = FD3D_TRUE;
                        out_hit->was_counter = (df->flags & FD3D_FLAG_COUNTER_WINDOW) != 0ul ? FD3D_TRUE : FD3D_FALSE;
                        out_hit->attacker_frame = attacker->frame_index;
                        out_hit->defender_frame = defender->frame_index;
                        out_hit->attack_volume = &af->attack_volumes[i];
                        out_hit->vulnerable_volume = &df->vulnerable_volumes[j];
                        out_hit->hit_def = hd;
                        out_hit->world_attack_volume = aw;
                        out_hit->world_vulnerable_volume = dw;
                        return FD3D_OK;
                    }
                }
                j = (unsigned short)(j + 1u);
            }
        }
        i = (unsigned short)(i + 1u);
    }
    return FD3D_OK;
}

int FD3D_ApplyHit(FD3D_Entity *attacker, FD3D_Entity *defender, const FD3D_HitResult *hit, const FD3D_Callbacks *callbacks, void *user)
{
    const FD3D_HitDef *hd;
    if (attacker == 0 || defender == 0 || hit == 0) {
        return FD3D_ERR_NULL;
    }
    if (hit->did_hit == FD3D_FALSE || hit->hit_def == 0) {
        return FD3D_OK;
    }
    hd = hit->hit_def;
    if (hit->was_blocked != FD3D_FALSE) {
        defender->hp -= hd->chip_damage;
        defender->blockstun_timer = hd->blockstun_frames;
        defender->hitstop_timer = hd->blockstop_frames;
        attacker->hitstop_timer = hd->blockstop_frames;
        if (callbacks != 0 && callbacks->on_block != 0) {
            callbacks->on_block(attacker, defender, hit, user);
        }
    } else {
        if ((FD3D_CurrentFrame(defender) != 0) && ((FD3D_CurrentFrame(defender)->flags & FD3D_FLAG_SUPER_ARMOR) != 0ul)) {
            defender->hp -= hd->damage;
        } else if ((FD3D_CurrentFrame(defender) != 0) && ((FD3D_CurrentFrame(defender)->flags & FD3D_FLAG_ARMOR) != 0ul)) {
            defender->hp -= hd->damage;
        } else {
            defender->hp -= hd->damage;
            defender->hitstun_timer = hd->hitstun_frames;
        }
        defender->hitstop_timer = hd->hitstop_frames;
        attacker->hitstop_timer = hd->hitstop_frames;
        defender->last_taken_hit_id = hd->hit_id;
        if (callbacks != 0 && callbacks->on_hit != 0) {
            callbacks->on_hit(attacker, defender, hit, user);
        }
    }
    return FD3D_OK;
}

int FD3D_CheckAndApply(FD3D_Entity *attacker, FD3D_Entity *defender, const FD3D_Callbacks *callbacks, void *user, FD3D_HitResult *out_hit)
{
    int rc;
    FD3D_HitResult local_hit;
    FD3D_HitResult *hit;
    hit = out_hit;
    if (hit == 0) {
        hit = &local_hit;
    }
    rc = FD3D_CheckHit(attacker, defender, hit);
    if (rc != FD3D_OK) {
        return rc;
    }
    return FD3D_ApplyHit(attacker, defender, hit, callbacks, user);
}

int FD3D_ActionTotalFrames(const FD3D_Action *action)
{
    unsigned short i;
    int total;
    if (action == 0 || action->frames == 0) {
        return 0;
    }
    total = 0;
    i = 0u;
    while (i < action->frame_count) {
        total += (int)action->frames[i].duration;
        i = (unsigned short)(i + 1u);
    }
    return total;
}

int FD3D_FrameStartTick(const FD3D_Action *action, unsigned short frame_index)
{
    unsigned short i;
    int total;
    if (action == 0 || action->frames == 0 || frame_index >= action->frame_count) {
        return -1;
    }
    total = 0;
    i = 0u;
    while (i < frame_index) {
        total += (int)action->frames[i].duration;
        i = (unsigned short)(i + 1u);
    }
    return total;
}

int FD3D_FrameAdvantageAfterContact(const FD3D_Action *action, unsigned short frame_index, unsigned short tick_in_frame, int defender_stun_frames)
{
    int start;
    int total;
    int contact;
    int remaining;
    start = FD3D_FrameStartTick(action, frame_index);
    total = FD3D_ActionTotalFrames(action);
    if (start < 0 || total <= 0) {
        return 0;
    }
    contact = start + (int)tick_in_frame;
    remaining = total - contact - 1;
    if (remaining < 0) {
        remaining = 0;
    }
    return defender_stun_frames - remaining;
}

int FD3D_IsActionDone(const FD3D_Entity *entity)
{
    const FD3D_Frame *frame;
    if (entity == 0 || entity->action == 0) {
        return FD3D_TRUE;
    }
    if (entity->action->loop != 0u) {
        return FD3D_FALSE;
    }
    if ((unsigned int)entity->frame_index + 1u < entity->action->frame_count) {
        return FD3D_FALSE;
    }
    frame = FD3D_CurrentFrame(entity);
    if (frame == 0) {
        return FD3D_TRUE;
    }
    if (entity->frame_tick + 1u < frame->duration) {
        return FD3D_FALSE;
    }
    return FD3D_TRUE;
}
