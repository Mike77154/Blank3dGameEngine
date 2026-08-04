#include "blank3d_config.h"

#include <stdio.h>
#include <string.h>

static void b3d_config_status(Blank3DConfig *config, const char *text)
{
    if (!config) return;
    if (!text) text = "";
    strncpy(config->status, text, sizeof(config->status) - 1U);
    config->status[sizeof(config->status) - 1U] = '\0';
}

void blank3d_config_defaults(Blank3DConfig *config)
{
    if (!config) return;
    memset(config, 0, sizeof(*config));
    config->audio_enabled = 1;
    config->start_first_person = 0;
    config->lock_mouse = 1;
    config->initial_weapon = 1;
    config->gatling_spinup_ms = 360;
    config->slingshot_charge_ms = 900;
    config->mouse_sensitivity_q16 = CONF_FIXED_ONE * 18L / 100L;
    config->pitch_min_q16 = -85L * CONF_FIXED_ONE;
    config->pitch_max_q16 = 85L * CONF_FIXED_ONE;
    config->camera_distance_q16 = 8L * CONF_FIXED_ONE;
    config->camera_height_q16 = 4L * CONF_FIXED_ONE;
    config->camera_shoulder_q16 = CONF_FIXED_ONE;
    config->eye_height_q16 = 17L * CONF_FIXED_ONE / 10L;
    config->zoom_fov_q16 = 45L * CONF_FIXED_ONE;
    config->sniper_zoom_fov_q16 = 18L * CONF_FIXED_ONE;
    config->zoom_speed_q16 = 120L * CONF_FIXED_ONE;
    config->ammo_9mm = 180;
    config->ammo_shells = 40;
    config->ammo_magnum = 24;
    config->ammo_sniper = 30;
    config->ammo_grenades = 18;
    config->ammo_rockets = 8;
    config->ammo_gatling = 900;
    config->ammo_stones = 60;
    config->damage_multiplier_q16 = CONF_FIXED_ONE;
    config->speed_multiplier_q16 = CONF_FIXED_ONE;
    config->recoil_multiplier_q16 = CONF_FIXED_ONE;
    config->gravity_enabled = 1;
    config->gravity_fall_speed_q16 = 9L * CONF_FIXED_ONE;
    config->jump_gravity_q16 = 22L * CONF_FIXED_ONE;
    config->flag_weapon_enabled = 1;
    config->flag_can_fire = 1;
    config->flag_can_reload = 1;
    config->flag_active_reload = 1;
    b3d_config_status(config, "defaults");
}

int blank3d_config_load(Blank3DConfig *config, const char *path)
{
    FILE *file;
    long size;
    unsigned int read_count;
    conf_err_t error;
    if (!config || !path) return 0;
    blank3d_config_defaults(config);
    file = fopen(path, "rb");
    if (!file) {
        b3d_config_status(config, "config missing; defaults active");
        return 0;
    }
    if (fseek(file, 0L, SEEK_END) != 0) {
        fclose(file);
        b3d_config_status(config, "config seek failed");
        return 0;
    }
    size = ftell(file);
    if (size < 0L || size >= B3D_CONFIG_TEXT_CAPACITY) {
        fclose(file);
        b3d_config_status(config, "config too large");
        return 0;
    }
    rewind(file);
    read_count = (unsigned int)fread(config->text, 1U, (size_t)size, file);
    fclose(file);
    config->text[read_count] = '\0';

    conf_ctx_init(&config->parser, config->arena, B3D_CONFIG_ARENA_CAPACITY);
    conf_ctx_set_load_flags(&config->parser,
                            CONF_LOAD_OVERRIDE | CONF_LOAD_COPY_SLICES);
    error = conf_load_auto(&config->parser, config->text, read_count);
    if (error != CONF_OK) {
        b3d_config_status(config, "config parse failed; defaults active");
        return 0;
    }
    config->audio_enabled = conf_get_bool(&config->parser, "audio.enabled", config->audio_enabled);
    config->start_first_person = conf_get_bool(&config->parser, "camera.first_person", config->start_first_person);
    config->lock_mouse = conf_get_bool(&config->parser, "camera.lock_mouse", config->lock_mouse);
    config->initial_weapon = (int)conf_get_int(&config->parser, "weapon.initial", config->initial_weapon);
    config->gatling_spinup_ms = (int)conf_get_int(&config->parser, "weapon.gatling_spinup_ms", config->gatling_spinup_ms);
    config->slingshot_charge_ms = (int)conf_get_int(&config->parser, "weapon.slingshot_charge_ms", config->slingshot_charge_ms);
    config->mouse_sensitivity_q16 = conf_get_fixed(&config->parser, "camera.mouse_sensitivity", config->mouse_sensitivity_q16);
    config->pitch_min_q16 = conf_get_fixed(&config->parser, "camera.pitch_min", config->pitch_min_q16);
    config->pitch_max_q16 = conf_get_fixed(&config->parser, "camera.pitch_max", config->pitch_max_q16);
    config->camera_distance_q16 = conf_get_fixed(&config->parser, "camera.distance", config->camera_distance_q16);
    config->camera_height_q16 = conf_get_fixed(&config->parser, "camera.height", config->camera_height_q16);
    config->camera_shoulder_q16 = conf_get_fixed(&config->parser, "camera.shoulder", config->camera_shoulder_q16);
    config->eye_height_q16 = conf_get_fixed(&config->parser, "camera.eye_height", config->eye_height_q16);
    config->zoom_fov_q16 = conf_get_fixed(&config->parser, "camera.zoom_fov", config->zoom_fov_q16);
    config->sniper_zoom_fov_q16 = conf_get_fixed(&config->parser, "camera.sniper_zoom_fov", config->sniper_zoom_fov_q16);
    config->zoom_speed_q16 = conf_get_fixed(&config->parser, "camera.zoom_speed", config->zoom_speed_q16);
    config->ammo_9mm = (int)conf_get_int(&config->parser, "inventory.ammo_9mm", config->ammo_9mm);
    config->ammo_shells = (int)conf_get_int(&config->parser, "inventory.ammo_shells", config->ammo_shells);
    config->ammo_magnum = (int)conf_get_int(&config->parser, "inventory.ammo_magnum", config->ammo_magnum);
    config->ammo_sniper = (int)conf_get_int(&config->parser, "inventory.ammo_sniper", config->ammo_sniper);
    config->ammo_grenades = (int)conf_get_int(&config->parser, "inventory.ammo_grenades", config->ammo_grenades);
    config->ammo_rockets = (int)conf_get_int(&config->parser, "inventory.ammo_rockets", config->ammo_rockets);
    config->ammo_gatling = (int)conf_get_int(&config->parser, "inventory.ammo_gatling", config->ammo_gatling);
    config->ammo_stones = (int)conf_get_int(&config->parser, "inventory.ammo_stones", config->ammo_stones);
    config->damage_multiplier_q16 = conf_get_fixed(&config->parser, "numeric.damage_multiplier", config->damage_multiplier_q16);
    config->speed_multiplier_q16 = conf_get_fixed(&config->parser, "numeric.speed_multiplier", config->speed_multiplier_q16);
    config->recoil_multiplier_q16 = conf_get_fixed(&config->parser, "numeric.recoil_multiplier", config->recoil_multiplier_q16);
    config->gravity_enabled = conf_get_bool(&config->parser, "gravity.enabled", config->gravity_enabled);
    config->gravity_fall_speed_q16 = conf_get_fixed(&config->parser, "gravity.fall_speed", config->gravity_fall_speed_q16);
    config->jump_gravity_q16 = conf_get_fixed(&config->parser, "gravity.jump_acceleration", config->jump_gravity_q16);
    config->flag_weapon_enabled = conf_get_bool(&config->parser, "flags.weapon_enabled", config->flag_weapon_enabled);
    config->flag_can_fire = conf_get_bool(&config->parser, "flags.can_fire", config->flag_can_fire);
    config->flag_can_reload = conf_get_bool(&config->parser, "flags.can_reload", config->flag_can_reload);
    config->flag_active_reload = conf_get_bool(&config->parser, "flags.active_reload", config->flag_active_reload);
    config->loaded = 1;
    b3d_config_status(config, "config/blank3d.toml loaded");
    return 1;
}

const char *blank3d_config_status(const Blank3DConfig *config)
{
    return config ? config->status : "config unavailable";
}
