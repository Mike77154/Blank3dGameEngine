#include <stdio.h>
#include <string.h>

#include "blank3d_weapon_modules.h"

int main(void)
{
    const Blank3DWeaponModules *sniper;
    const Blank3DWeaponModules *sling;
    const Blank3DWeaponModules *grenade;
    long slow;
    long fast;

    sniper = blank3d_weapon_modules_get(B3D_WEAPON_ID_SNIPER);
    sling = blank3d_weapon_modules_get(B3D_WEAPON_ID_SLINGSHOT);
    grenade = blank3d_weapon_modules_get(B3D_WEAPON_ID_HAND_GRENADE);
    if (!sniper || !sling || !grenade) return 1;
    if (sniper->optic_profile != B3D_OPTIC_SNIPER) return 2;
    if (!sniper->sway_enabled || !sniper->aim_query_enabled) return 3;
    if (!sniper->scope_recipe_path[0] ||
        strcmp(sniper->scope_recipe_path,
               "config/scope/hud/sniper_re5_psg1.ini") != 0)
        return 4;
    if (!sniper->scope_preset_name[0] ||
        strcmp(sniper->scope_preset_name, "re5_psg1_game_scope") != 0)
        return 15;

    if (sling->trigger_model != B3D_TRIGGER_CHARGE_RELEASE) return 5;
    if (sling->physics_backend != B3D_PHYSICS_BOLT3D) return 6;
    if (sling->projectile_mesh_id != 9) return 7;
    if (sling->emit_casing || sling->emit_muzzle) return 8;
    if (!sling->emit_trail || sling->trail_profile != B3D_TRAIL_ARC)
        return 9;
    slow = blank3d_weapon_modules_charge_speed_q16(
        B3D_WEAPON_ID_SLINGSHOT, 0U);
    fast = blank3d_weapon_modules_charge_speed_q16(
        B3D_WEAPON_ID_SLINGSHOT, 900U);
    if (slow >= fast) return 10;
    if (blank3d_weapon_modules_charge_ratio_q16(
            B3D_WEAPON_ID_SLINGSHOT, 450U) < 32000L)
        return 11;

    if (grenade->physics_backend != B3D_PHYSICS_GRAVITY) return 12;
    if (grenade->projectile_mesh_id != 10) return 13;
    if (grenade->emit_casing || grenade->emit_muzzle || !grenade->emit_trail)
        return 14;

    puts("Blank3D universal weapon composition test: OK");
    return 0;
}
