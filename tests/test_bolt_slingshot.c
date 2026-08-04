#include <stdio.h>

#include "blank3d_bolt.h"

static Blank3DCollision collision;
static Blank3DBolt bolt;

int main(void)
{
    const Blank3DWeaponModules *modules;
    GWP89_Vec3 origin;
    GWP89_Vec3 direction;
    const B3D_Projectile *projectile;
    int projectile_id;
    int i;

    blank3d_collision_init(&collision);
    blank3d_bolt_init(&bolt, &collision);
    modules = blank3d_weapon_modules_get(B3D_WEAPON_ID_SLINGSHOT);
    if (!modules) return 1;
    origin = gwp89_v3(0, gwp89_fx_from_int(5), 0);
    direction = gwp89_v3(0, 0, GWP89_FIX_ONE);
    projectile_id = blank3d_bolt_spawn(&bolt, modules, 1,
        &origin, &direction, gwp89_fx_from_int(20),
        gwp89_fx_from_text("0.22"), gwp89_fx_from_int(16),
        3000U, 0);
    if (projectile_id < 0) return 2;
    for (i = 0; i < 10; ++i)
        blank3d_bolt_update(&bolt, gwp89_fx_from_text("0.05"));
    projectile = blank3d_bolt_get(&bolt, projectile_id);
    if (!projectile) return 3;
    if (projectile->position.z <= 0) return 4;
    if (projectile->position.y >= b3d_fixed_from_int(5)) return 5;
    if (projectile->velocity.z <= 0) return 6;
    puts("Blank3D Bolt3D slingshot physics test: OK");
    return 0;
}
