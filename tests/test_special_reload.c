#include <stdio.h>
#include <string.h>

#include "blank3d_systems.h"
#include "blank3d_weapon_modules.h"
#include "gtrigger89.h"

static GWP89_Vec3 make_v3(int x, int y, int z)
{
    return gwp89_v3(gwp89_fx_from_int(x),
                    gwp89_fx_from_int(y),
                    gwp89_fx_from_int(z));
}

static int player_latch(const Blank3DSystems *systems)
{
    int slot;
    if (!systems) return -1;
    slot = gwp89_find_user_slot(&systems->weapons, B3D_PLAYER_ACTOR_ID);
    if (slot < 0) return -1;
    return systems->weapons.users[slot].trigger_latched;
}

static void runner_step(Blank3DSystems *systems,
                        gtrigger89_state *trigger,
                        int weapon_id,
                        int raw_down,
                        unsigned short dt_ms)
{
    gtrigger89_config config;
    gtrigger89_output output;
    const Blank3DWeaponModules *modules;
    GWP89_Vec3 origin;
    GWP89_Vec3 forward;
    GWP89_Vec3 right;
    GWP89_Vec3 up;

    memset(&config, 0, sizeof(config));
    config.model = GTRIGGER89_MODEL_NORMAL;
    config.spinup_ms = 360U;
    config.charge_max_ms = 900U;
    modules = blank3d_weapon_modules_get(weapon_id);
    if (modules && modules->trigger_model == B3D_TRIGGER_CHARGE_RELEASE)
        config.model = GTRIGGER89_MODEL_CHARGE_RELEASE;
    else if (modules && modules->trigger_model == B3D_TRIGGER_SPINUP)
        config.model = GTRIGGER89_MODEL_SPINUP;

    gtrigger89_update(trigger, &config, raw_down, (unsigned int)dt_ms,
                      &output);
    origin = make_v3(0, 1, 0);
    forward = make_v3(0, 0, -1);
    right = make_v3(1, 0, 0);
    up = make_v3(0, 1, 0);
    blank3d_systems_update(systems, dt_ms,
                           output.trigger_down,
                           output.trigger_pressed,
                           output.trigger_released,
                           GWP89_VIEW_FPS, GWP89_FIX_ONE,
                           &origin, &forward, &right, &up,
                           &origin, &forward);
}

static int fire_once(Blank3DSystems *systems,
                     gtrigger89_state *trigger,
                     int weapon_id)
{
    int i;
    if (weapon_id == 12) {
        for (i = 0; i < 60; ++i)
            runner_step(systems, trigger, weapon_id, 1, 16U);
        runner_step(systems, trigger, weapon_id, 0, 16U);
    } else {
        runner_step(systems, trigger, weapon_id, 1, 16U);
        runner_step(systems, trigger, weapon_id, 0, 16U);
    }
    return blank3d_systems_clip(systems);
}

static int test_weapon_reload(int weapon_id, int expected_reserve)
{
    Blank3DSystems systems;
    gtrigger89_state trigger;
    int i;
    int result;

    blank3d_systems_init(&systems);
    gtrigger89_init(&trigger);
    if (blank3d_systems_equip_id(&systems, weapon_id) != GWP89_OK)
        return 10 + weapon_id;
    if (blank3d_systems_clip(&systems) != 1)
        return 30 + weapon_id;
    if (blank3d_systems_reserve(&systems) != expected_reserve)
        return 50 + weapon_id;

    if (fire_once(&systems, &trigger, weapon_id) != 0)
        return 70 + weapon_id;
    if (weapon_id == 12 && player_latch(&systems) != 0)
        return 90 + weapon_id;

    result = blank3d_systems_reload(&systems);
    if (result != GWP89_OK)
        return 110 + weapon_id;
    for (i = 0; i < 200; ++i)
        runner_step(&systems, &trigger, weapon_id, 0, 16U);
    if (blank3d_systems_clip(&systems) != 1)
        return 130 + weapon_id;
    if (blank3d_systems_reserve(&systems) != expected_reserve - 1)
        return 150 + weapon_id;

    if (fire_once(&systems, &trigger, weapon_id) != 0)
        return 170 + weapon_id;
    return 0;
}

int main(void)
{
    int result;
    result = test_weapon_reload(12, 6);
    if (result != 0) {
        printf("special reload: Shango failed (%d)\n", result);
        return result;
    }
    result = test_weapon_reload(13, 6);
    if (result != 0) {
        printf("special reload: Homing failed (%d)\n", result);
        return result;
    }
    puts("Shango + Homing reload runner path: PASS");
    return 0;
}
