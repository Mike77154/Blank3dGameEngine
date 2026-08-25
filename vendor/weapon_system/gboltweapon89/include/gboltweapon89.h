#ifndef GBOLTWEAPON89_H
#define GBOLTWEAPON89_H
#include "gweapon89.h"
#include "gweaponmodules89.h"
#include "bolt3d/bolt3d.h"
#ifdef __cplusplus
extern "C" {
#endif
#define GBW89_MAX_PROJECTILES 32
#define GBW89_MAX_EVENTS 128
#define GBW89_MAX_COLLIDERS 1

typedef struct GBoltSweepHit89Tag {
    int hit;
    int target_id;
    int target_layer;
    int material_id;
    gwp89_fx fraction_fx;
    GWP89_Vec3 point;
    GWP89_Vec3 normal;
} GBoltSweepHit89;
typedef int (*GBoltSweep89Fn)(void *user,const GWP89_Vec3 *start,const GWP89_Vec3 *delta,gwp89_fx radius_fx,GBoltSweepHit89 *out_hit);

typedef struct GBoltWeapon89Tag {
    B3D_World world;
    B3D_Projectile projectiles[GBW89_MAX_PROJECTILES];
    B3D_Event events[GBW89_MAX_EVENTS];
    B3D_Collider colliders[GBW89_MAX_COLLIDERS];
    GBoltSweep89Fn sweep_fn;
    void *sweep_user;
    B3D_Fixed drag_q16[GBW89_MAX_PROJECTILES];
} GBoltWeapon89;

void gboltweapon89_init(GBoltWeapon89 *bolt,GBoltSweep89Fn sweep_fn,void *sweep_user);
int gboltweapon89_spawn(GBoltWeapon89 *bolt,const GWeaponModules89 *modules,int owner_id,const GWP89_Vec3 *origin_q12,const GWP89_Vec3 *direction_q12,gwp89_fx speed_q12,gwp89_fx radius_q12,gwp89_fx damage_q12,unsigned short life_ms,void *user_ptr);
void gboltweapon89_update(GBoltWeapon89 *bolt,gwp89_fx dt_q12);
const B3D_Projectile *gboltweapon89_get(const GBoltWeapon89 *bolt,int projectile_id);
int gboltweapon89_poll_event(GBoltWeapon89 *bolt,B3D_Event *event_out);
void gboltweapon89_destroy(GBoltWeapon89 *bolt,int projectile_id);
#ifdef __cplusplus
}
#endif
#endif
