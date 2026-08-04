#include <stdio.h>
#include "blank3d_config.h"

static Blank3DConfig config;

int main(void)
{
    if (!blank3d_config_load(&config, "config/blank3d.toml")) return 1;
    if (!config.loaded) return 2;
    if (!config.audio_enabled) return 3;
    if (config.mouse_sensitivity_q16 <= 0L) return 4;
    if (config.initial_weapon < 1 || config.initial_weapon > 9) return 5;
    if (config.gatling_spinup_ms != 360) return 12;
    if (config.slingshot_charge_ms != 900) return 13;
    if (config.ammo_9mm != 180) return 6;
    if (config.damage_multiplier_q16 != CONF_FIXED_ONE) return 7;
    if (!config.flag_weapon_enabled || !config.flag_can_fire) return 8;
    if (config.ammo_grenades != 18 || config.ammo_rockets != 8) return 9;
    if (config.ammo_gatling != 900) return 11;
    if (config.ammo_stones != 60) return 14;
    if (config.zoom_fov_q16 <= 0L || config.sniper_zoom_fov_q16 <= 0L) return 10;
    if (!config.gravity_enabled || config.gravity_fall_speed_q16 <= 0L ||
        config.jump_gravity_q16 <= 0L) return 15;
    puts("Blank3D conf_total integration test: OK");
    return 0;
}
