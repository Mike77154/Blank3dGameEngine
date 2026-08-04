#include "blank3d_weapon_ini.h"
#include "blank3d_weapon_modules.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define B3D_INI_LINE_CAP 512
#define B3D_INI_PATH_CAP 512
#define B3D_INI_MAX_WEAPONS 16
#define B3D_Q16_ONE 65536L

static void b3d_status(char *status, size_t capacity, const char *text)
{
    if (!status || capacity == 0U) return;
    if (!text) text = "";
    strncpy(status, text, capacity - 1U);
    status[capacity - 1U] = '\0';
}

static char *b3d_trim(char *text)
{
    char *end;
    while (*text && isspace((unsigned char)*text)) ++text;
    end = text + strlen(text);
    while (end > text && isspace((unsigned char)end[-1])) --end;
    *end = '\0';
    return text;
}

static int b3d_eq(const char *a, const char *b)
{
    unsigned char ca;
    unsigned char cb;
    if (!a || !b) return 0;
    while (*a && *b) {
        ca = (unsigned char)tolower((unsigned char)*a++);
        cb = (unsigned char)tolower((unsigned char)*b++);
        if (ca != cb) return 0;
    }
    return *a == '\0' && *b == '\0';
}

static int b3d_bool(const char *text, int fallback)
{
    if (!text) return fallback;
    if (b3d_eq(text, "1") || b3d_eq(text, "true") ||
        b3d_eq(text, "yes") || b3d_eq(text, "on")) return 1;
    if (b3d_eq(text, "0") || b3d_eq(text, "false") ||
        b3d_eq(text, "no") || b3d_eq(text, "off")) return 0;
    return fallback;
}

static int b3d_int(const char *text, int fallback)
{
    char *end;
    long value;
    if (!text || !*text) return fallback;
    value = strtol(text, &end, 10);
    if (*b3d_trim(end) != '\0') return fallback;
    if (value < -2147483647L) value = -2147483647L;
    if (value > 2147483647L) value = 2147483647L;
    return (int)value;
}

static long b3d_q16(const char *text, long fallback)
{
    gwp89_fx q12;
    if (!text || !*text) return fallback;
    q12 = gwp89_fx_from_text(text);
    return (long)q12 * 16L;
}

static unsigned short b3d_u16(const char *text, unsigned short fallback)
{
    int value;
    value = b3d_int(text, (int)fallback);
    if (value < 0) value = 0;
    if (value > 65535) value = 65535;
    return (unsigned short)value;
}

/* Converts a positive fixed-point rate to an integer shot interval.  The
   weapon manager stores cadence as milliseconds, while INI authors may use
   the more readable rounds/second or rounds/minute forms. */
static unsigned short b3d_rate_interval_ms(const char *text,
                                            long milliseconds_per_unit,
                                            unsigned short fallback)
{
    gwp89_fx rate_fx;
    long numerator;
    long interval;
    if (!text || !*text || milliseconds_per_unit <= 0L) return fallback;
    rate_fx = gwp89_fx_from_text(text);
    if (rate_fx <= 0L) return fallback;
    numerator = milliseconds_per_unit * 4096L;
    interval = (numerator + ((long)rate_fx / 2L)) / (long)rate_fx;
    if (interval < 1L) interval = 1L;
    if (interval > 65535L) interval = 65535L;
    return (unsigned short)interval;
}

static void b3d_copy(char *dst, size_t capacity, const char *src)
{
    size_t i;
    if (!dst || capacity == 0U) return;
    if (!src) src = "";
    i = 0U;
    while (i + 1U < capacity && src[i] != '\0') {
        dst[i] = src[i];
        ++i;
    }
    dst[i] = '\0';
}

static void b3d_manifest_dir(const char *path, char *out, size_t capacity)
{
    const char *slash;
    const char *backslash;
    const char *last;
    size_t length;
    if (!out || capacity == 0U) return;
    out[0] = '\0';
    if (!path) return;
    slash = strrchr(path, '/');
    backslash = strrchr(path, '\\');
    last = slash;
    if (!last || (backslash && backslash > last)) last = backslash;
    if (!last) return;
    length = (size_t)(last - path);
    if (length >= capacity) length = capacity - 1U;
    memcpy(out, path, length);
    out[length] = '\0';
}

static void b3d_join_path(const char *dir, const char *file,
                          char *out, size_t capacity)
{
    if (!dir || !*dir) {
        b3d_copy(out, capacity, file);
        return;
    }
    if (!file) file = "";
    if (file[0] == '/' || file[0] == '\\' ||
        (isalpha((unsigned char)file[0]) && file[1] == ':')) {
        b3d_copy(out, capacity, file);
        return;
    }
    {
        size_t used;
        b3d_copy(out, capacity, dir);
        used = strlen(out);
        if (used + 1U < capacity) {
            out[used++] = '/';
            out[used] = '\0';
        }
        if (used < capacity - 1U)
            b3d_copy(out + used, capacity - used, file);
    }
}

static int b3d_fire_mode(const char *value)
{
    if (b3d_eq(value, "auto")) return GWP89_FIRE_AUTO;
    if (b3d_eq(value, "hold_once") || b3d_eq(value, "hold-once"))
        return GWP89_FIRE_HOLD_ONCE;
    if (b3d_eq(value, "burst")) return GWP89_FIRE_BURST;
    return GWP89_FIRE_SEMI;
}

static int b3d_trigger(const char *value)
{
    if (b3d_eq(value, "spinup")) return B3D_TRIGGER_SPINUP;
    if (b3d_eq(value, "charge_release") || b3d_eq(value, "charge-release"))
        return B3D_TRIGGER_CHARGE_RELEASE;
    return B3D_TRIGGER_STANDARD;
}

static int b3d_physics(const char *value)
{
    if (b3d_eq(value, "gravity")) return B3D_PHYSICS_GRAVITY;
    if (b3d_eq(value, "bolt3d")) return B3D_PHYSICS_BOLT3D;
    return B3D_PHYSICS_LINEAR;
}

static int b3d_trail(const char *value)
{
    if (b3d_eq(value, "none")) return B3D_TRAIL_NONE;
    if (b3d_eq(value, "tracer")) return B3D_TRAIL_TRACER;
    if (b3d_eq(value, "heavy")) return B3D_TRAIL_HEAVY;
    if (b3d_eq(value, "arc")) return B3D_TRAIL_ARC;
    return B3D_TRAIL_BULLET;
}

static int b3d_optic(const char *value)
{
    if (b3d_eq(value, "sniper")) return B3D_OPTIC_SNIPER;
    if (b3d_eq(value, "generic")) return B3D_OPTIC_GENERIC;
    return B3D_OPTIC_NONE;
}

static int b3d_audio_profile(const char *v)
{
    if (b3d_eq(v, "service_pistol") || b3d_eq(v, "pistol")) return B3D_AUDIO_PROFILE_PISTOL;
    if (b3d_eq(v, "smg") || b3d_eq(v, "machine_gun")) return B3D_AUDIO_PROFILE_SMG;
    if (b3d_eq(v, "shotgun")) return B3D_AUDIO_PROFILE_SHOTGUN;
    if (b3d_eq(v, "magnum")) return B3D_AUDIO_PROFILE_MAGNUM;
    if (b3d_eq(v, "sniper")) return B3D_AUDIO_PROFILE_SNIPER;
    if (b3d_eq(v, "launcher")) return B3D_AUDIO_PROFILE_LAUNCHER;
    if (b3d_eq(v, "heavy") || b3d_eq(v, "gatling")) return B3D_AUDIO_PROFILE_HEAVY;
    return B3D_AUDIO_PROFILE_NONE;
}

static int b3d_audio_action(const char *v)
{
    if (b3d_eq(v, "pistol")) return B3D_AUDIO_ACTION_PISTOL;
    if (b3d_eq(v, "machine") || b3d_eq(v, "automatic")) return B3D_AUDIO_ACTION_MACHINE;
    if (b3d_eq(v, "pump") || b3d_eq(v, "pump_shotgun")) return B3D_AUDIO_ACTION_PUMP;
    if (b3d_eq(v, "revolver")) return B3D_AUDIO_ACTION_REVOLVER;
    if (b3d_eq(v, "rifle") || b3d_eq(v, "bolt")) return B3D_AUDIO_ACTION_RIFLE;
    return B3D_AUDIO_ACTION_NONE;
}

static int b3d_audio_magazine(const char *v)
{
    if (b3d_eq(v, "pistol_polymer")) return B3D_AUDIO_MAG_PISTOL_POLYMER;
    if (b3d_eq(v, "pistol_metal")) return B3D_AUDIO_MAG_PISTOL_METAL;
    if (b3d_eq(v, "smg_steel")) return B3D_AUDIO_MAG_SMG_STEEL;
    if (b3d_eq(v, "rifle_polymer")) return B3D_AUDIO_MAG_RIFLE_POLYMER;
    if (b3d_eq(v, "rifle_aluminum")) return B3D_AUDIO_MAG_RIFLE_ALUMINUM;
    if (b3d_eq(v, "sniper_box")) return B3D_AUDIO_MAG_SNIPER_BOX;
    if (b3d_eq(v, "drum_heavy")) return B3D_AUDIO_MAG_DRUM_HEAVY;
    return B3D_AUDIO_MAG_NONE;
}

static int b3d_audio_ammo(const char *v)
{
    if (b3d_eq(v, "box_mag")) return B3D_AUDIO_AMMO_BOX_MAG;
    if (b3d_eq(v, "tube")) return B3D_AUDIO_AMMO_TUBE;
    if (b3d_eq(v, "loose_shells")) return B3D_AUDIO_AMMO_LOOSE_SHELLS;
    if (b3d_eq(v, "belt_box")) return B3D_AUDIO_AMMO_BELT_BOX;
    return B3D_AUDIO_AMMO_NONE;
}

static int b3d_audio_muzzle(const char *v)
{
    if (b3d_eq(v, "brake")) return B3D_AUDIO_MUZZLE_BRAKE;
    if (b3d_eq(v, "compensator")) return B3D_AUDIO_MUZZLE_COMPENSATOR;
    if (b3d_eq(v, "ported")) return B3D_AUDIO_MUZZLE_PORTED;
    return B3D_AUDIO_MUZZLE_BARE;
}

static int b3d_audio_shell(const char *v)
{
    if (b3d_eq(v, "pistol_brass")) return B3D_AUDIO_SHELL_PISTOL_BRASS;
    if (b3d_eq(v, "steel_case")) return B3D_AUDIO_SHELL_STEEL_CASE;
    if (b3d_eq(v, "shotgun_plastic")) return B3D_AUDIO_SHELL_SHOTGUN_PLASTIC;
    if (b3d_eq(v, "magnum_brass")) return B3D_AUDIO_SHELL_MAGNUM_BRASS;
    if (b3d_eq(v, "rifle_brass")) return B3D_AUDIO_SHELL_RIFLE_BRASS;
    if (b3d_eq(v, "shotgun_brass")) return B3D_AUDIO_SHELL_SHOTGUN_BRASS;
    if (b3d_eq(v, "rimfire_brass")) return B3D_AUDIO_SHELL_RIMFIRE_BRASS;
    return B3D_AUDIO_SHELL_NONE;
}

static int b3d_audio_projectile(const char *v)
{
    if (b3d_eq(v, "none")) return B3D_AUDIO_PROJECTILE_NONE;
    if (b3d_eq(v, "near_miss") || b3d_eq(v, "snap")) return B3D_AUDIO_PROJECTILE_NEAR_MISS;
    if (b3d_eq(v, "supersonic") || b3d_eq(v, "nwave")) return B3D_AUDIO_PROJECTILE_SUPERSONIC;
    if (b3d_eq(v, "pellet_swarm")) return B3D_AUDIO_PROJECTILE_PELLET_SWARM;
    if (b3d_eq(v, "tracer") || b3d_eq(v, "flyby")) return B3D_AUDIO_PROJECTILE_TRACER;
    return B3D_AUDIO_PROJECTILE_AUTO;
}

static int b3d_audio_explosion(const char *v)
{
    if (b3d_eq(v, "grenade")) return B3D_AUDIO_EXPLOSION_GRENADE;
    if (b3d_eq(v, "rocket")) return B3D_AUDIO_EXPLOSION_ROCKET;
    return B3D_AUDIO_EXPLOSION_NONE;
}

static void b3d_profile_defaults(GWP89_WeaponProfile *p,
                                 Blank3DWeaponModules *m)
{
    memset(p, 0, sizeof(*p));
    memset(m, 0, sizeof(*m));
    p->active = 1;
    p->fire_mode = GWP89_FIRE_SEMI;
    p->clip_size = 1;
    p->ammo_per_shot = 1;
    p->pellet_count = 1;
    p->allow_dry_fire_event = 1;
    p->active_reload_enabled = 1;
    p->cooldown_ms = 150U;
    p->reload_ms = 1000U;
    p->projectile_life_ms = 2500U;
    p->damage_fx = gwp89_fx_from_text("10");
    p->speed_fx = gwp89_fx_from_text("40");
    p->range_fx = gwp89_fx_from_text("100");
    p->spread_fx = 0;
    p->projectile_radius_fx = gwp89_fx_from_text("0.1");
    p->projectile_mesh_scale_fx = gwp89_fx_from_text("1");
    p->shell_mesh_scale_fx = gwp89_fx_from_text("1");
    p->recoil_fx = gwp89_fx_from_text("0.2");

    m->trigger_model = B3D_TRIGGER_STANDARD;
    m->physics_backend = B3D_PHYSICS_LINEAR;
    m->trail_profile = B3D_TRAIL_BULLET;
    m->optic_profile = B3D_OPTIC_NONE;
    m->emit_muzzle = 1;
    m->emit_casing = 1;
    m->emit_trail = 1;
    m->mass_q16 = B3D_Q16_ONE;
    m->audio_enabled = 1;
    m->audio_profile = B3D_AUDIO_PROFILE_PISTOL;
    m->audio_action = B3D_AUDIO_ACTION_PISTOL;
    m->audio_magazine = B3D_AUDIO_MAG_PISTOL_POLYMER;
    m->audio_ammo = B3D_AUDIO_AMMO_BOX_MAG;
    m->audio_muzzle = B3D_AUDIO_MUZZLE_BARE;
    m->audio_shell = B3D_AUDIO_SHELL_PISTOL_BRASS;
    m->audio_detachable_magazine = 1;
    m->audio_emits_casing = 1;
    m->audio_projectile = B3D_AUDIO_PROJECTILE_AUTO;
    m->audio_fire_gain_q15 = 29200;
    m->audio_pressure_energy_q15 = 0;
    m->audio_ammo_motion_q15 = 11800;
    m->audio_magazine_velocity_q15 = 23500;
    m->audio_magazine_gain_q15 = 24800;
    m->audio_reload_remove_motion_q15 = 27000;
    m->audio_reload_insert_motion_q15 = 30500;
    m->audio_dry_receiver_impulse = 4300;
    m->audio_action_speed_q16 = 65536UL;
    m->audio_casing_velocity = 150;
    m->audio_casing_angular_velocity = 180;
    m->audio_casing_gain_q15 = 24500;
    m->audio_projectile_gain_q15 = 21000;
    m->audio_projectile_proximity_q15 = 23000;
    m->audio_projectile_instance_limit = 24;
    m->audio_explosion_gain_q15 = 29200;
    m->audio_rocket_whistle_gain_q15 = 10500;
    m->audio_rocket_spin_gain_q15 = 5900;
}

static void b3d_apply_weapon(GWP89_WeaponProfile *p,
                             const char *key, const char *value)
{
    if (b3d_eq(key, "id")) p->weapon_id = b3d_int(value, p->weapon_id);
    else if (b3d_eq(key, "name")) gwp89_copy_id(p->name, GWP89_NAME_MAX, value);
    else if (b3d_eq(key, "gun_id")) p->gun_id = b3d_int(value, p->gun_id);
    else if (b3d_eq(key, "ammo_id")) p->ammo_id = b3d_int(value, p->ammo_id);
    else if (b3d_eq(key, "projectile_id")) p->projectile_id = b3d_int(value, p->projectile_id);
    else if (b3d_eq(key, "shell_id")) p->shell_id = b3d_int(value, p->shell_id);
    else if (b3d_eq(key, "muzzle_id")) p->muzzle_id = b3d_int(value, p->muzzle_id);
    else if (b3d_eq(key, "casing_id")) p->casing_id = b3d_int(value, p->casing_id);
    else if (b3d_eq(key, "trail_id")) p->trail_id = b3d_int(value, p->trail_id);
    else if (b3d_eq(key, "projectile_mesh_id")) p->projectile_mesh_id = b3d_int(value, p->projectile_mesh_id);
    else if (b3d_eq(key, "shell_mesh_id")) p->shell_mesh_id = b3d_int(value, p->shell_mesh_id);
    else if (b3d_eq(key, "gun_name")) gwp89_copy_id(p->gun_name, GWP89_NAME_MAX, value);
    else if (b3d_eq(key, "ammo_name")) gwp89_copy_id(p->ammo_name, GWP89_NAME_MAX, value);
    else if (b3d_eq(key, "projectile_name")) gwp89_copy_id(p->projectile_name, GWP89_NAME_MAX, value);
    else if (b3d_eq(key, "shell_name")) gwp89_copy_id(p->shell_name, GWP89_NAME_MAX, value);
    else if (b3d_eq(key, "muzzle_name")) gwp89_copy_id(p->muzzle_name, GWP89_NAME_MAX, value);
    else if (b3d_eq(key, "casing_name")) gwp89_copy_id(p->casing_name, GWP89_NAME_MAX, value);
    else if (b3d_eq(key, "trail_name")) gwp89_copy_id(p->trail_name, GWP89_NAME_MAX, value);
    else if (b3d_eq(key, "projectile_mesh_name")) gwp89_copy_id(p->projectile_mesh_name, GWP89_NAME_MAX, value);
    else if (b3d_eq(key, "shell_mesh_name")) gwp89_copy_id(p->shell_mesh_name, GWP89_NAME_MAX, value);
    else if (b3d_eq(key, "fire_mode")) p->fire_mode = b3d_fire_mode(value);
    else if (b3d_eq(key, "clip_size")) p->clip_size = b3d_int(value, p->clip_size);
    else if (b3d_eq(key, "ammo_per_shot")) p->ammo_per_shot = b3d_int(value, p->ammo_per_shot);
    else if (b3d_eq(key, "pellet_count")) p->pellet_count = b3d_int(value, p->pellet_count);
    else if (b3d_eq(key, "burst_count")) p->burst_count = b3d_int(value, p->burst_count);
    else if (b3d_eq(key, "allow_dry_fire")) p->allow_dry_fire_event = b3d_bool(value, p->allow_dry_fire_event);
    else if (b3d_eq(key, "active_reload")) p->active_reload_enabled = b3d_bool(value, p->active_reload_enabled);
    else if (b3d_eq(key, "cooldown_ms") ||
             b3d_eq(key, "shot_interval_ms") ||
             b3d_eq(key, "fire_interval_ms"))
        p->cooldown_ms = b3d_u16(value, p->cooldown_ms);
    else if (b3d_eq(key, "fire_rate_bps") ||
             b3d_eq(key, "shots_per_second") ||
             b3d_eq(key, "rounds_per_second") ||
             b3d_eq(key, "cadence_bps"))
        p->cooldown_ms = b3d_rate_interval_ms(value, 1000L,
                                               p->cooldown_ms);
    else if (b3d_eq(key, "fire_rate_rpm") ||
             b3d_eq(key, "rounds_per_minute") ||
             b3d_eq(key, "rpm"))
        p->cooldown_ms = b3d_rate_interval_ms(value, 60000L,
                                               p->cooldown_ms);
    else if (b3d_eq(key, "reload_ms")) p->reload_ms = b3d_u16(value, p->reload_ms);
    else if (b3d_eq(key, "projectile_life_ms")) p->projectile_life_ms = b3d_u16(value, p->projectile_life_ms);
    else if (b3d_eq(key, "active_reload_window_start_ms")) p->active_reload_window_start_ms = b3d_u16(value, p->active_reload_window_start_ms);
    else if (b3d_eq(key, "active_reload_window_end_ms")) p->active_reload_window_end_ms = b3d_u16(value, p->active_reload_window_end_ms);
    else if (b3d_eq(key, "active_reload_bonus_ms")) p->active_reload_bonus_ms = b3d_u16(value, p->active_reload_bonus_ms);
    else if (b3d_eq(key, "active_reload_penalty_ms")) p->active_reload_penalty_ms = b3d_u16(value, p->active_reload_penalty_ms);
    else if (b3d_eq(key, "damage")) p->damage_fx = gwp89_fx_from_text(value);
    else if (b3d_eq(key, "projectile_speed") ||
             b3d_eq(key, "bullet_speed") ||
             b3d_eq(key, "muzzle_velocity") ||
             b3d_eq(key, "speed"))
        p->speed_fx = gwp89_fx_from_text(value);
    else if (b3d_eq(key, "range")) p->range_fx = gwp89_fx_from_text(value);
    else if (b3d_eq(key, "spread")) p->spread_fx = gwp89_fx_from_text(value);
    else if (b3d_eq(key, "projectile_radius")) p->projectile_radius_fx = gwp89_fx_from_text(value);
    else if (b3d_eq(key, "projectile_mesh_scale")) p->projectile_mesh_scale_fx = gwp89_fx_from_text(value);
    else if (b3d_eq(key, "shell_mesh_scale")) p->shell_mesh_scale_fx = gwp89_fx_from_text(value);
    else if (b3d_eq(key, "recoil")) p->recoil_fx = gwp89_fx_from_text(value);
}

static void b3d_apply_modules(Blank3DWeaponModules *m,
                              const char *key, const char *value)
{
    if (b3d_eq(key, "trigger")) m->trigger_model = b3d_trigger(value);
    else if (b3d_eq(key, "physics")) m->physics_backend = b3d_physics(value);
    else if (b3d_eq(key, "trail")) m->trail_profile = b3d_trail(value);
    else if (b3d_eq(key, "optic")) m->optic_profile = b3d_optic(value);
    else if (b3d_eq(key, "sway")) m->sway_enabled = b3d_bool(value, m->sway_enabled);
    else if (b3d_eq(key, "aim_query")) m->aim_query_enabled = b3d_bool(value, m->aim_query_enabled);
    else if (b3d_eq(key, "emit_muzzle")) m->emit_muzzle = b3d_bool(value, m->emit_muzzle);
    else if (b3d_eq(key, "emit_casing")) m->emit_casing = b3d_bool(value, m->emit_casing);
    else if (b3d_eq(key, "emit_trail")) m->emit_trail = b3d_bool(value, m->emit_trail);
    else if (b3d_eq(key, "projectile_mesh_id")) m->projectile_mesh_id = b3d_int(value, m->projectile_mesh_id);
    else if (b3d_eq(key, "casing_mesh_id")) m->casing_mesh_id = b3d_int(value, m->casing_mesh_id);
    else if (b3d_eq(key, "scope_preset")) b3d_copy(m->scope_preset_name, sizeof(m->scope_preset_name), value);
    else if (b3d_eq(key, "charge_time_ms")) m->charge_time_ms = b3d_u16(value, m->charge_time_ms);
    else if (b3d_eq(key, "charge_min_speed")) m->charge_min_speed_q16 = b3d_q16(value, m->charge_min_speed_q16);
    else if (b3d_eq(key, "charge_max_speed")) m->charge_max_speed_q16 = b3d_q16(value, m->charge_max_speed_q16);
    else if (b3d_eq(key, "gravity")) m->gravity_q16 = b3d_q16(value, m->gravity_q16);
    else if (b3d_eq(key, "bounce")) m->bounce_q16 = b3d_q16(value, m->bounce_q16);
    else if (b3d_eq(key, "drag")) m->drag_q16 = b3d_q16(value, m->drag_q16);
    else if (b3d_eq(key, "mass")) m->mass_q16 = b3d_q16(value, m->mass_q16);
}

static void b3d_apply_audio(Blank3DWeaponModules *m,
                            const char *key, const char *value)
{
    if (b3d_eq(key, "enabled")) m->audio_enabled = b3d_bool(value, m->audio_enabled);
    else if (b3d_eq(key, "profile")) m->audio_profile = b3d_audio_profile(value);
    else if (b3d_eq(key, "action")) m->audio_action = b3d_audio_action(value);
    else if (b3d_eq(key, "magazine")) m->audio_magazine = b3d_audio_magazine(value);
    else if (b3d_eq(key, "ammo")) m->audio_ammo = b3d_audio_ammo(value);
    else if (b3d_eq(key, "muzzle")) m->audio_muzzle = b3d_audio_muzzle(value);
    else if (b3d_eq(key, "shell")) m->audio_shell = b3d_audio_shell(value);
    else if (b3d_eq(key, "detachable_magazine")) m->audio_detachable_magazine = b3d_bool(value, m->audio_detachable_magazine);
    else if (b3d_eq(key, "emits_casing")) m->audio_emits_casing = b3d_bool(value, m->audio_emits_casing);
    else if (b3d_eq(key, "projectile")) m->audio_projectile = b3d_audio_projectile(value);
    else if (b3d_eq(key, "explosion")) m->audio_explosion = b3d_audio_explosion(value);
    else if (b3d_eq(key, "continuous_rocket")) m->audio_continuous_rocket = b3d_bool(value, m->audio_continuous_rocket);
    else if (b3d_eq(key, "fire_gain_q15")) m->audio_fire_gain_q15 = b3d_int(value, m->audio_fire_gain_q15);
    else if (b3d_eq(key, "pressure_energy_q15")) m->audio_pressure_energy_q15 = b3d_int(value, m->audio_pressure_energy_q15);
    else if (b3d_eq(key, "ammo_motion_q15")) m->audio_ammo_motion_q15 = b3d_int(value, m->audio_ammo_motion_q15);
    else if (b3d_eq(key, "magazine_velocity_q15")) m->audio_magazine_velocity_q15 = b3d_int(value, m->audio_magazine_velocity_q15);
    else if (b3d_eq(key, "magazine_gain_q15")) m->audio_magazine_gain_q15 = b3d_int(value, m->audio_magazine_gain_q15);
    else if (b3d_eq(key, "reload_remove_motion_q15")) m->audio_reload_remove_motion_q15 = b3d_int(value, m->audio_reload_remove_motion_q15);
    else if (b3d_eq(key, "reload_insert_motion_q15")) m->audio_reload_insert_motion_q15 = b3d_int(value, m->audio_reload_insert_motion_q15);
    else if (b3d_eq(key, "dry_receiver_impulse")) m->audio_dry_receiver_impulse = b3d_int(value, m->audio_dry_receiver_impulse);
    else if (b3d_eq(key, "action_speed")) m->audio_action_speed_q16 = (unsigned long)b3d_q16(value, (long)m->audio_action_speed_q16);
    else if (b3d_eq(key, "casing_velocity")) m->audio_casing_velocity = b3d_int(value, m->audio_casing_velocity);
    else if (b3d_eq(key, "casing_angular_velocity")) m->audio_casing_angular_velocity = b3d_int(value, m->audio_casing_angular_velocity);
    else if (b3d_eq(key, "casing_gain_q15")) m->audio_casing_gain_q15 = b3d_int(value, m->audio_casing_gain_q15);
    else if (b3d_eq(key, "projectile_gain_q15")) m->audio_projectile_gain_q15 = b3d_int(value, m->audio_projectile_gain_q15);
    else if (b3d_eq(key, "projectile_proximity_q15")) m->audio_projectile_proximity_q15 = b3d_int(value, m->audio_projectile_proximity_q15);
    else if (b3d_eq(key, "projectile_instance_limit")) m->audio_projectile_instance_limit = b3d_int(value, m->audio_projectile_instance_limit);
    else if (b3d_eq(key, "explosion_gain_q15")) m->audio_explosion_gain_q15 = b3d_int(value, m->audio_explosion_gain_q15);
    else if (b3d_eq(key, "rocket_whistle_gain_q15")) m->audio_rocket_whistle_gain_q15 = b3d_int(value, m->audio_rocket_whistle_gain_q15);
    else if (b3d_eq(key, "rocket_spin_gain_q15")) m->audio_rocket_spin_gain_q15 = b3d_int(value, m->audio_rocket_spin_gain_q15);
}

static int b3d_load_weapon_file(const char *path,
                                GWP89_WeaponProfile *profile,
                                Blank3DWeaponModules *modules,
                                char *status, size_t status_capacity)
{
    FILE *file;
    char line[B3D_INI_LINE_CAP];
    char section[32];
    char *text;
    char *equals;
    char *key;
    char *value;
    b3d_profile_defaults(profile, modules);
    section[0] = '\0';
    file = fopen(path, "rb");
    if (!file) {
        b3d_status(status, status_capacity, "weapon INI could not be opened");
        return 0;
    }
    while (fgets(line, sizeof(line), file)) {
        text = b3d_trim(line);
        if (!*text || *text == ';' || *text == '#') continue;
        if (*text == '[') {
            char *close;
            close = strchr(text + 1, ']');
            if (!close) continue;
            *close = '\0';
            b3d_copy(section, sizeof(section), b3d_trim(text + 1));
            continue;
        }
        equals = strchr(text, '=');
        if (!equals) continue;
        *equals = '\0';
        key = b3d_trim(text);
        value = b3d_trim(equals + 1);
        if (b3d_eq(section, "weapon")) b3d_apply_weapon(profile, key, value);
        else if (b3d_eq(section, "modules") || b3d_eq(section, "physics"))
            b3d_apply_modules(modules, key, value);
        else if (b3d_eq(section, "audio")) b3d_apply_audio(modules, key, value);
    }
    fclose(file);
    if (profile->weapon_id <= 0 || !profile->name[0]) {
        b3d_status(status, status_capacity, "weapon INI missing id or name");
        return 0;
    }
    modules->weapon_id = profile->weapon_id;
    b3d_copy(modules->name, sizeof(modules->name), profile->name);
    if (modules->projectile_mesh_id <= 0)
        modules->projectile_mesh_id = profile->projectile_mesh_id;
    if (modules->casing_mesh_id < 0)
        modules->casing_mesh_id = profile->shell_mesh_id;
    return 1;
}

int blank3d_weapon_ini_load_manifest(GWP89_Manager *manager,
                                     const char *manifest_path,
                                     char *status,
                                     size_t status_capacity)
{
    FILE *manifest;
    char line[B3D_INI_LINE_CAP];
    char directory[B3D_INI_PATH_CAP];
    char path[B3D_INI_PATH_CAP];
    char section[32];
    char *text;
    char *equals;
    char *value;
    GWP89_WeaponProfile profile;
    Blank3DWeaponModules modules;
    int count;
    if (!manager || !manifest_path) return 0;
    manifest = fopen(manifest_path, "rb");
    if (!manifest) {
        b3d_status(status, status_capacity, "weapon manifest missing; built-in fallback used");
        return 0;
    }
    b3d_manifest_dir(manifest_path, directory, sizeof(directory));
    blank3d_weapon_modules_reset();
    section[0] = '\0';
    count = 0;
    while (fgets(line, sizeof(line), manifest)) {
        text = b3d_trim(line);
        if (!*text || *text == ';' || *text == '#') continue;
        if (*text == '[') {
            char *close;
            close = strchr(text + 1, ']');
            if (!close) continue;
            *close = '\0';
            b3d_copy(section, sizeof(section), b3d_trim(text + 1));
            continue;
        }
        /* weapons.ini also contains [weapon.N] catalog metadata. Only the
           [weapons] section is the profile-file manifest; treating id/name/
           capacity values as paths leaves the manager partially initialized. */
        if (!b3d_eq(section, "weapons")) continue;
        equals = strchr(text, '=');
        if (!equals) continue;
        value = b3d_trim(equals + 1);
        if (!*value) continue;
        b3d_join_path(directory, value, path, sizeof(path));
        if (!b3d_load_weapon_file(path, &profile, &modules,
                                  status, status_capacity)) {
            fclose(manifest);
            blank3d_weapon_modules_load_defaults();
            return 0;
        }
        if (gwp89_add_weapon(manager, &profile) < 0 ||
            !blank3d_weapon_modules_register(&modules)) {
            fclose(manifest);
            b3d_status(status, status_capacity, "weapon registry capacity exceeded");
            blank3d_weapon_modules_load_defaults();
            return 0;
        }
        ++count;
        if (count >= B3D_INI_MAX_WEAPONS) break;
    }
    fclose(manifest);
    if (count <= 0) {
        blank3d_weapon_modules_load_defaults();
        b3d_status(status, status_capacity, "weapon manifest contained no files");
        return 0;
    }
    {
        char message[128];
        sprintf(message, "weapon INI registry loaded: %d definitions", count);
        b3d_status(status, status_capacity, message);
    }
    return count;
}
