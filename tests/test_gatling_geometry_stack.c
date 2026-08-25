#include "blank3d_weapon_ini.h"
#include "blank3d_weapon_modules.h"
#include "blank3d_weapon_host_io.h"
#include "bulletspin89.h"
#include "bulletcircle89.h"
#include "bulletinline89.h"
#include "gweapon89.h"

#include <stdio.h>
#include <string.h>

static long abs_l(long v) { return v < 0L ? -v : v; }

int main(void)
{
    GWP89_Manager manager;
    const Blank3DWeaponModules *modules;
    bs89_config spin_config;
    bs89_state spin_state;
    bc89_request circle_request;
    bc89_result circle_result;
    bi89_request inline_request;
    bi89_result inline_result;
    bc89_vec3 first_origin;
    bi89_vec3 first_direction;
    char status[160];
    int loaded;
    int shot;
    int slot;
    int unique_count;
    bc89_vec3 prior[6];

    gwp89_init(&manager);
    (void)blank3d_weapon_host_io_bind(&manager);
    loaded = blank3d_weapon_ini_load_manifest(
        &manager, "config/weapons/weapons.ini", status, sizeof(status));
    if (loaded != 15) return 1;
    modules = blank3d_weapon_modules_get(B3D_WEAPON_ID_GATLING);
    if (!modules || !modules->bullet_circle_enabled ||
        !modules->bullet_spin_enabled || !modules->bullet_inline_enabled)
        return 2;

    memset(&spin_config, 0, sizeof(spin_config));
    spin_config.slot_count = modules->bullet_circle_count;
    spin_config.start_slot = modules->bullet_spin_start_slot;
    spin_config.step = modules->bullet_spin_step;
    spin_config.direction = modules->bullet_spin_direction;
    bulletspin89_init(&spin_state, &spin_config);

    memset(&circle_request, 0, sizeof(circle_request));
    circle_request.center.x = 0L;
    circle_request.center.y = 0L;
    circle_request.center.z = 0L;
    circle_request.right.x = BC89_ONE;
    circle_request.up.y = BC89_ONE;
    circle_request.radius_fx = (bc89_fx)(modules->bullet_circle_radius_q16 / 16L);
    circle_request.slot_count = modules->bullet_circle_count;
    circle_request.phase_turn_q16 = modules->bullet_circle_phase_turn_q16;

    unique_count = 0;
    memset(prior, 0, sizeof(prior));
    for (shot = 0; shot < 7; ++shot) {
        int i;
        int duplicate;
        if (!bulletspin89_next(&spin_state, &slot)) return 3;
        circle_request.slot_index = slot;
        if (!bulletcircle89_resolve(&circle_request, &circle_result) ||
            !circle_result.valid) return 4;

        memset(&inline_request, 0, sizeof(inline_request));
        inline_request.origin.x = circle_result.origin.x;
        inline_request.origin.y = circle_result.origin.y;
        inline_request.origin.z = circle_result.origin.z;
        inline_request.target.x = 0L;
        inline_request.target.y = 0L;
        inline_request.target.z = 100L * BI89_ONE;
        inline_request.fallback_direction.z = BI89_ONE;
        inline_request.target_valid = 1;
        if (!bulletinline89_resolve(&inline_request, &inline_result) ||
            !inline_result.valid || !inline_result.used_target) return 5;

        if (shot < 6) {
            duplicate = 0;
            for (i = 0; i < unique_count; ++i) {
                if (abs_l(prior[i].x - circle_result.origin.x) <= 2L &&
                    abs_l(prior[i].y - circle_result.origin.y) <= 2L &&
                    abs_l(prior[i].z - circle_result.origin.z) <= 2L)
                    duplicate = 1;
            }
            if (duplicate) return 6;
            prior[unique_count++] = circle_result.origin;
            if (circle_result.origin.x * inline_result.direction.x > 0L)
                return 7;
            if (circle_result.origin.y * inline_result.direction.y > 0L)
                return 8;
        }
        if (shot == 0) {
            first_origin = circle_result.origin;
            first_direction = inline_result.direction;
        }
        if (shot == 6) {
            if (abs_l(first_origin.x - circle_result.origin.x) > 2L ||
                abs_l(first_origin.y - circle_result.origin.y) > 2L ||
                abs_l(first_origin.z - circle_result.origin.z) > 2L)
                return 9;
            if (abs_l(first_direction.x - inline_result.direction.x) > 2L ||
                abs_l(first_direction.y - inline_result.direction.y) > 2L ||
                abs_l(first_direction.z - inline_result.direction.z) > 2L)
                return 10;
        }
    }

    if (unique_count != 6) return 11;
    printf("Gatling bulletspin89 + bulletcircle89 + bulletinline89: OK\n");
    return 0;
}
