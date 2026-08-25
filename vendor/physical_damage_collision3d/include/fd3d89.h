#ifndef FD3D89_H
#define FD3D89_H

/*
   fd3d89 - deterministic 3D fighting frame-data volume kernel
   C89, fixed-point, user arenas, no malloc/free/realloc, no float/double.
*/

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef long FD3D_Fx;

#define FD3D_FX_SHIFT 12
#define FD3D_FX_ONE   (1L << FD3D_FX_SHIFT)
#define FD3D_FX_FROM_INT(v) ((FD3D_Fx)(v) * (FD3D_Fx)FD3D_FX_ONE)
#define FD3D_FX_TO_INT(v)   ((int)((v) >> FD3D_FX_SHIFT))

#define FD3D_TRUE 1
#define FD3D_FALSE 0

#define FD3D_FACE_Z_POS 0
#define FD3D_FACE_X_POS 1
#define FD3D_FACE_Z_NEG 2
#define FD3D_FACE_X_NEG 3

#define FD3D_VOL_AABB   0u
#define FD3D_VOL_SPHERE 1u

#define FD3D_ATTR_STRIKE      0x0001u
#define FD3D_ATTR_PROJECTILE  0x0002u
#define FD3D_ATTR_THROW       0x0004u
#define FD3D_ATTR_HIGH        0x0008u
#define FD3D_ATTR_MID         0x0010u
#define FD3D_ATTR_LOW         0x0020u
#define FD3D_ATTR_AIR         0x0040u
#define FD3D_ATTR_GROUND      0x0080u
#define FD3D_ATTR_UNBLOCKABLE 0x0100u
#define FD3D_ATTR_HOMING      0x0200u
#define FD3D_ATTR_TRACK_LEFT  0x0400u
#define FD3D_ATTR_TRACK_RIGHT 0x0800u

#define FD3D_FLAG_INVINCIBLE        0x00000001ul
#define FD3D_FLAG_STRIKE_INVULN     0x00000002ul
#define FD3D_FLAG_PROJECTILE_INVULN 0x00000004ul
#define FD3D_FLAG_THROW_INVULN      0x00000008ul
#define FD3D_FLAG_ARMOR             0x00000010ul
#define FD3D_FLAG_COUNTER_WINDOW    0x00000020ul
#define FD3D_FLAG_AIRBORNE          0x00000040ul
#define FD3D_FLAG_CROUCHING         0x00000080ul
#define FD3D_FLAG_SIDE_STEP         0x00000100ul
#define FD3D_FLAG_CAN_CANCEL        0x00000200ul
#define FD3D_FLAG_BACK_TURNED       0x00000400ul
#define FD3D_FLAG_SUPER_ARMOR       0x00000800ul

#define FD3D_LANE_CENTER 0x0001u
#define FD3D_LANE_LEFT   0x0002u
#define FD3D_LANE_RIGHT  0x0004u
#define FD3D_LANE_ALL    0xffffu

#define FD3D_OK 0
#define FD3D_ERR_NULL       -1
#define FD3D_ERR_OOM        -2
#define FD3D_ERR_BAD_INDEX  -3
#define FD3D_ERR_BAD_ACTION -4

typedef struct FD3D_Arena FD3D_Arena;
typedef struct FD3D_Volume FD3D_Volume;
typedef struct FD3D_HitDef FD3D_HitDef;
typedef struct FD3D_Frame FD3D_Frame;
typedef struct FD3D_Action FD3D_Action;
typedef struct FD3D_Entity FD3D_Entity;
typedef struct FD3D_HitResult FD3D_HitResult;
typedef struct FD3D_Callbacks FD3D_Callbacks;

struct FD3D_Arena {
    unsigned char *mem;
    size_t cap;
    size_t used;
};

struct FD3D_Volume {
    unsigned int type;
    FD3D_Fx x1;
    FD3D_Fx y1;
    FD3D_Fx z1;
    FD3D_Fx x2;
    FD3D_Fx y2;
    FD3D_Fx z2;
    FD3D_Fx radius;
    unsigned int mask;
    unsigned int group;
    unsigned int lane_mask;
};

struct FD3D_HitDef {
    unsigned int attr_mask;
    unsigned int lane_mask;
    int damage;
    int chip_damage;
    int hitstun_frames;
    int blockstun_frames;
    int hitstop_frames;
    int blockstop_frames;
    FD3D_Fx push_x;
    FD3D_Fx push_y;
    FD3D_Fx push_z;
    unsigned int hit_id;
    unsigned int cancel_on_hit_mask;
    unsigned int cancel_on_block_mask;
};

struct FD3D_Frame {
    unsigned short duration;
    unsigned short anim_index;
    unsigned long flags;
    unsigned int cancel_mask;
    unsigned int lane_mask;
    const FD3D_Volume *attack_volumes;
    unsigned short attack_count;
    const FD3D_Volume *vulnerable_volumes;
    unsigned short vulnerable_count;
    const FD3D_Volume *body_volumes;
    unsigned short body_count;
    const FD3D_Volume *proximity_volumes;
    unsigned short proximity_count;
    const FD3D_HitDef *hit_def;
};

struct FD3D_Action {
    unsigned int id;
    const char *name;
    const FD3D_Frame *frames;
    unsigned short frame_count;
    unsigned char loop;
    unsigned char interruptible_on_end;
};

struct FD3D_Entity {
    unsigned int entity_id;
    int team;
    FD3D_Fx pos_x;
    FD3D_Fx pos_y;
    FD3D_Fx pos_z;
    int facing;
    int hp;
    unsigned int lane_mask;
    const FD3D_Action *action;
    unsigned short frame_index;
    unsigned short frame_tick;
    unsigned short total_tick;
    int hitstop_timer;
    int hitstun_timer;
    int blockstun_timer;
    unsigned int hitby_mask;
    int hitby_timer;
    unsigned int nothitby_mask;
    int nothitby_timer;
    unsigned int last_taken_hit_id;
    unsigned long user_flags;
    void *user;
};

struct FD3D_HitResult {
    int did_hit;
    int was_blocked;
    int was_counter;
    int was_side_step_evaded;
    unsigned short attacker_frame;
    unsigned short defender_frame;
    const FD3D_Volume *attack_volume;
    const FD3D_Volume *vulnerable_volume;
    const FD3D_HitDef *hit_def;
    FD3D_Volume world_attack_volume;
    FD3D_Volume world_vulnerable_volume;
};

struct FD3D_Callbacks {
    void (*on_frame_enter)(FD3D_Entity *entity, const FD3D_Frame *frame, void *user);
    void (*on_action_end)(FD3D_Entity *entity, const FD3D_Action *action, void *user);
    void (*on_hit)(FD3D_Entity *attacker, FD3D_Entity *defender, const FD3D_HitResult *hit, void *user);
    void (*on_block)(FD3D_Entity *attacker, FD3D_Entity *defender, const FD3D_HitResult *hit, void *user);
};

void FD3D_ArenaInit(FD3D_Arena *arena, void *memory, size_t capacity);
void *FD3D_ArenaAlloc(FD3D_Arena *arena, size_t size, size_t align);
void FD3D_ArenaReset(FD3D_Arena *arena);

int FD3D_VolumeMakeAabb(FD3D_Volume *volume, int x1, int y1, int z1, int x2, int y2, int z2, unsigned int mask, unsigned int group, unsigned int lane_mask);
int FD3D_VolumeMakeSphere(FD3D_Volume *volume, int x, int y, int z, int radius, unsigned int mask, unsigned int group, unsigned int lane_mask);
int FD3D_EntityInit(FD3D_Entity *entity, unsigned int id, int team, int hp);
int FD3D_SetAction(FD3D_Entity *entity, const FD3D_Action *action);
const FD3D_Frame *FD3D_CurrentFrame(const FD3D_Entity *entity);
int FD3D_Tick(FD3D_Entity *entity, const FD3D_Callbacks *callbacks, void *user);
int FD3D_TickTimers(FD3D_Entity *entity);
void FD3D_SetHitBy(FD3D_Entity *entity, unsigned int mask, int frames);
void FD3D_SetNotHitBy(FD3D_Entity *entity, unsigned int mask, int frames);
int FD3D_CanBeHit(const FD3D_Entity *defender, unsigned int attr_mask);
FD3D_Volume FD3D_WorldVolume(const FD3D_Volume *local_volume, FD3D_Fx pos_x, FD3D_Fx pos_y, FD3D_Fx pos_z, int facing);
int FD3D_VolumeOverlap(const FD3D_Volume *a, const FD3D_Volume *b);
int FD3D_CheckHit(const FD3D_Entity *attacker, const FD3D_Entity *defender, FD3D_HitResult *out_hit);
int FD3D_ApplyHit(FD3D_Entity *attacker, FD3D_Entity *defender, const FD3D_HitResult *hit, const FD3D_Callbacks *callbacks, void *user);
int FD3D_CheckAndApply(FD3D_Entity *attacker, FD3D_Entity *defender, const FD3D_Callbacks *callbacks, void *user, FD3D_HitResult *out_hit);
int FD3D_ActionTotalFrames(const FD3D_Action *action);
int FD3D_FrameStartTick(const FD3D_Action *action, unsigned short frame_index);
int FD3D_FrameAdvantageAfterContact(const FD3D_Action *action, unsigned short frame_index, unsigned short tick_in_frame, int defender_stun_frames);
int FD3D_IsActionDone(const FD3D_Entity *entity);

#ifdef __cplusplus
}
#endif

#endif
