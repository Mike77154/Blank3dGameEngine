#ifndef BLANK3D_BOLT_H
#define BLANK3D_BOLT_H

#include "blank3d_collision.h"
#include "blank3d_weapon_modules.h"
#include "bolt3d/bolt3d.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BLANK3D_BOLT_MAX_PROJECTILES 32
#define BLANK3D_BOLT_MAX_EVENTS 128
#define BLANK3D_BOLT_MAX_COLLIDERS 1

typedef struct Blank3DBoltTag {
    B3D_World world;
    B3D_Projectile projectiles[BLANK3D_BOLT_MAX_PROJECTILES];
    B3D_Event events[BLANK3D_BOLT_MAX_EVENTS];
    B3D_Collider colliders[BLANK3D_BOLT_MAX_COLLIDERS];
    Blank3DCollision *collision;
    B3D_Fixed drag_q16[BLANK3D_BOLT_MAX_PROJECTILES];
} Blank3DBolt;

void blank3d_bolt_init(Blank3DBolt *bolt, Blank3DCollision *collision);
int blank3d_bolt_spawn(Blank3DBolt *bolt,
                       const Blank3DWeaponModules *modules,
                       int owner_id,
                       const GWP89_Vec3 *origin_q12,
                       const GWP89_Vec3 *direction_q12,
                       gwp89_fx speed_q12,
                       gwp89_fx radius_q12,
                       gwp89_fx damage_q12,
                       unsigned short life_ms,
                       void *user_ptr);
void blank3d_bolt_update(Blank3DBolt *bolt, gwp89_fx dt_q12);
const B3D_Projectile *blank3d_bolt_get(const Blank3DBolt *bolt,
                                      int projectile_id);
int blank3d_bolt_poll_event(Blank3DBolt *bolt, B3D_Event *event_out);
void blank3d_bolt_destroy(Blank3DBolt *bolt, int projectile_id);

#ifdef __cplusplus
}
#endif

#endif
