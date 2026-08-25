#include "gweaponprofileio89.h"
#include "gweaponmodules89.h"
#include "gweaponio89.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#define B3D_INI_LINE_CAP 512
#define B3D_INI_PATH_CAP 512
#define B3D_INI_MAX_WEAPONS 16
#define B3D_Q16_ONE 65536L

#define GWPROFILE89_TEXT_CAP 32768

typedef struct GWProfile89TextReaderTag {
    const char *cursor;
} GWProfile89TextReader;

static int gwprofile89_next_line(GWProfile89TextReader *reader,
                                 char *line, size_t capacity)
{
    const char *p;
    size_t n;
    if (!reader || !reader->cursor || !*reader->cursor ||
        !line || capacity == 0U) return 0;
    p = reader->cursor;
    n = 0U;
    while (*p && *p != '\n' && *p != '\r') {
        if (n + 1U < capacity) line[n++] = *p;
        ++p;
    }
    line[n] = '\0';
    while (*p == '\n' || *p == '\r') ++p;
    reader->cursor = p;
    return 1;
}

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

static int b3d_spin_direction(const char *text, int fallback)
{
    if (!text || !*text) return fallback;
    if (b3d_eq(text, "forward") || b3d_eq(text, "cw") ||
        b3d_eq(text, "clockwise") || b3d_eq(text, "+") ||
        b3d_eq(text, "1")) return 1;
    if (b3d_eq(text, "reverse") || b3d_eq(text, "ccw") ||
        b3d_eq(text, "counterclockwise") || b3d_eq(text, "-") ||
        b3d_eq(text, "-1")) return -1;
    return fallback;
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
    if (b3d_eq(value, "spinup")) return GWM89_TRIGGER_SPINUP;
    if (b3d_eq(value, "charge_release") || b3d_eq(value, "charge-release"))
        return GWM89_TRIGGER_CHARGE_RELEASE;
    if (b3d_eq(value, "press_charge_release") ||
        b3d_eq(value, "press-charge-release") ||
        b3d_eq(value, "buster"))
        return GWM89_TRIGGER_PRESS_CHARGE_RELEASE;
    return GWM89_TRIGGER_STANDARD;
}

static int b3d_physics(const char *value)
{
    if (b3d_eq(value, "gravity")) return GWM89_PHYSICS_GRAVITY;
    if (b3d_eq(value, "bolt3d")) return GWM89_PHYSICS_BOLT3D;
    return GWM89_PHYSICS_LINEAR;
}

static int b3d_trail(const char *value)
{
    if (b3d_eq(value, "none")) return GWM89_TRAIL_NONE;
    if (b3d_eq(value, "tracer")) return GWM89_TRAIL_TRACER;
    if (b3d_eq(value, "heavy")) return GWM89_TRAIL_HEAVY;
    if (b3d_eq(value, "arc")) return GWM89_TRAIL_ARC;
    return GWM89_TRAIL_BULLET;
}

static int b3d_optic(const char *value)
{
    if (b3d_eq(value, "sniper")) return GWM89_OPTIC_SNIPER;
    if (b3d_eq(value, "generic")) return GWM89_OPTIC_GENERIC;
    return GWM89_OPTIC_NONE;
}

static int b3d_audio_profile(const char *v)
{
    if (b3d_eq(v, "service_pistol") || b3d_eq(v, "pistol")) return GWM89_AUDIO_PROFILE_PISTOL;
    if (b3d_eq(v, "smg") || b3d_eq(v, "machine_gun")) return GWM89_AUDIO_PROFILE_SMG;
    if (b3d_eq(v, "shotgun")) return GWM89_AUDIO_PROFILE_SHOTGUN;
    if (b3d_eq(v, "magnum")) return GWM89_AUDIO_PROFILE_MAGNUM;
    if (b3d_eq(v, "sniper")) return GWM89_AUDIO_PROFILE_SNIPER;
    if (b3d_eq(v, "launcher")) return GWM89_AUDIO_PROFILE_LAUNCHER;
    if (b3d_eq(v, "heavy") || b3d_eq(v, "gatling")) return GWM89_AUDIO_PROFILE_HEAVY;
    return GWM89_AUDIO_PROFILE_NONE;
}

static int b3d_audio_action(const char *v)
{
    if (b3d_eq(v, "pistol")) return GWM89_AUDIO_ACTION_PISTOL;
    if (b3d_eq(v, "machine") || b3d_eq(v, "automatic")) return GWM89_AUDIO_ACTION_MACHINE;
    if (b3d_eq(v, "pump") || b3d_eq(v, "pump_shotgun")) return GWM89_AUDIO_ACTION_PUMP;
    if (b3d_eq(v, "revolver")) return GWM89_AUDIO_ACTION_REVOLVER;
    if (b3d_eq(v, "rifle") || b3d_eq(v, "bolt")) return GWM89_AUDIO_ACTION_RIFLE;
    return GWM89_AUDIO_ACTION_NONE;
}

static int b3d_audio_magazine(const char *v)
{
    if (b3d_eq(v, "pistol_polymer")) return GWM89_AUDIO_MAG_PISTOL_POLYMER;
    if (b3d_eq(v, "pistol_metal")) return GWM89_AUDIO_MAG_PISTOL_METAL;
    if (b3d_eq(v, "smg_steel")) return GWM89_AUDIO_MAG_SMG_STEEL;
    if (b3d_eq(v, "rifle_polymer")) return GWM89_AUDIO_MAG_RIFLE_POLYMER;
    if (b3d_eq(v, "rifle_aluminum")) return GWM89_AUDIO_MAG_RIFLE_ALUMINUM;
    if (b3d_eq(v, "sniper_box")) return GWM89_AUDIO_MAG_SNIPER_BOX;
    if (b3d_eq(v, "drum_heavy")) return GWM89_AUDIO_MAG_DRUM_HEAVY;
    return GWM89_AUDIO_MAG_NONE;
}

static int b3d_audio_ammo(const char *v)
{
    if (b3d_eq(v, "box_mag")) return GWM89_AUDIO_AMMO_BOX_MAG;
    if (b3d_eq(v, "tube")) return GWM89_AUDIO_AMMO_TUBE;
    if (b3d_eq(v, "loose_shells")) return GWM89_AUDIO_AMMO_LOOSE_SHELLS;
    if (b3d_eq(v, "belt_box")) return GWM89_AUDIO_AMMO_BELT_BOX;
    return GWM89_AUDIO_AMMO_NONE;
}

static int b3d_audio_muzzle(const char *v)
{
    if (b3d_eq(v, "brake")) return GWM89_AUDIO_MUZZLE_BRAKE;
    if (b3d_eq(v, "compensator")) return GWM89_AUDIO_MUZZLE_COMPENSATOR;
    if (b3d_eq(v, "ported")) return GWM89_AUDIO_MUZZLE_PORTED;
    return GWM89_AUDIO_MUZZLE_BARE;
}

static int b3d_audio_shell(const char *v)
{
    if (b3d_eq(v, "pistol_brass")) return GWM89_AUDIO_SHELL_PISTOL_BRASS;
    if (b3d_eq(v, "steel_case")) return GWM89_AUDIO_SHELL_STEEL_CASE;
    if (b3d_eq(v, "shotgun_plastic")) return GWM89_AUDIO_SHELL_SHOTGUN_PLASTIC;
    if (b3d_eq(v, "magnum_brass")) return GWM89_AUDIO_SHELL_MAGNUM_BRASS;
    if (b3d_eq(v, "rifle_brass")) return GWM89_AUDIO_SHELL_RIFLE_BRASS;
    if (b3d_eq(v, "shotgun_brass")) return GWM89_AUDIO_SHELL_SHOTGUN_BRASS;
    if (b3d_eq(v, "rimfire_brass")) return GWM89_AUDIO_SHELL_RIMFIRE_BRASS;
    return GWM89_AUDIO_SHELL_NONE;
}

static int b3d_audio_projectile(const char *v)
{
    if (b3d_eq(v, "none")) return GWM89_AUDIO_PROJECTILE_NONE;
    if (b3d_eq(v, "near_miss") || b3d_eq(v, "snap")) return GWM89_AUDIO_PROJECTILE_NEAR_MISS;
    if (b3d_eq(v, "supersonic") || b3d_eq(v, "nwave")) return GWM89_AUDIO_PROJECTILE_SUPERSONIC;
    if (b3d_eq(v, "pellet_swarm")) return GWM89_AUDIO_PROJECTILE_PELLET_SWARM;
    if (b3d_eq(v, "tracer") || b3d_eq(v, "flyby")) return GWM89_AUDIO_PROJECTILE_TRACER;
    return GWM89_AUDIO_PROJECTILE_AUTO;
}

static int b3d_audio_explosion(const char *v)
{
    if (b3d_eq(v, "grenade")) return GWM89_AUDIO_EXPLOSION_GRENADE;
    if (b3d_eq(v, "rocket")) return GWM89_AUDIO_EXPLOSION_ROCKET;
    return GWM89_AUDIO_EXPLOSION_NONE;
}

static int b3d_projectile_visual(const char *v)
{
    if (b3d_eq(v, "shango_press") || b3d_eq(v, "shango-press") ||
        b3d_eq(v, "orbital_press") || b3d_eq(v, "orbital-press"))
        return GWM89_PROJECTILE_VISUAL_SHANGO_PRESS;
    if (b3d_eq(v, "expandible_fire") || b3d_eq(v, "expandible-fire") ||
        b3d_eq(v, "expanding_fire") || b3d_eq(v, "expanding-fire") ||
        b3d_eq(v, "flame_puff") || b3d_eq(v, "flame-puff"))
        return GWM89_PROJECTILE_VISUAL_EXPANDIBLE_FIRE;
    return GWM89_PROJECTILE_VISUAL_DEFAULT;
}

static int b3d_projectile_sprite_mode(const char *v)
{
    if (b3d_eq(v, "static") || b3d_eq(v, "staticsprite") ||
        b3d_eq(v, "staticsprite89")) return GWM89_PROJECTILE_SPRITE_STATIC;
    if (b3d_eq(v, "sequence") || b3d_eq(v, "imagesequence") ||
        b3d_eq(v, "imagesequencer") || b3d_eq(v, "imagesequencer89"))
        return GWM89_PROJECTILE_SPRITE_SEQUENCE;
    if (b3d_eq(v, "renlist") || b3d_eq(v, "renlist89") ||
        b3d_eq(v, "renpy") || b3d_eq(v, "atl"))
        return GWM89_PROJECTILE_SPRITE_RENLIST;
    if (b3d_eq(v, "tilecell") || b3d_eq(v, "tilecell89") ||
        b3d_eq(v, "atlas") || b3d_eq(v, "grid") ||
        b3d_eq(v, "tilemap")) return GWM89_PROJECTILE_SPRITE_TILECELL;
    if (b3d_eq(v, "gmstrip") || b3d_eq(v, "gmspritestrip") ||
        b3d_eq(v, "gmspritestrip89") || b3d_eq(v, "gamemaker_strip") ||
        b3d_eq(v, "gamemaker")) return GWM89_PROJECTILE_SPRITE_GMSTRIP;
    return GWM89_PROJECTILE_SPRITE_NONE;
}

static int b3d_projectile_sprite_loop(const char *v, int fallback)
{
    if (b3d_eq(v, "none") || b3d_eq(v, "once"))
        return GWM89_PROJECTILE_SPRITE_LOOP_NONE;
    if (b3d_eq(v, "loop") || b3d_eq(v, "forward"))
        return GWM89_PROJECTILE_SPRITE_LOOP_FORWARD;
    if (b3d_eq(v, "reverse"))
        return GWM89_PROJECTILE_SPRITE_LOOP_REVERSE;
    if (b3d_eq(v, "pingpong") || b3d_eq(v, "ping-pong"))
        return GWM89_PROJECTILE_SPRITE_LOOP_PINGPONG;
    if (b3d_eq(v, "hold") || b3d_eq(v, "hold_last"))
        return GWM89_PROJECTILE_SPRITE_LOOP_HOLD;
    return fallback;
}

static void b3d_profile_defaults(GWP89_WeaponProfile *p,
                                 GWeaponModules89 *m)
{
    memset(p, 0, sizeof(*p));
    memset(m, 0, sizeof(*m));
    p->active = 1;
    p->fire_mode = GWP89_FIRE_SEMI;
    p->clip_size = 1;
    p->ammo_per_shot = 1;
    p->infinite_ammo = 0;
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

    m->trigger_model = GWM89_TRIGGER_STANDARD;
    m->physics_backend = GWM89_PHYSICS_LINEAR;
    m->trail_profile = GWM89_TRAIL_BULLET;
    m->optic_profile = GWM89_OPTIC_NONE;
    m->emit_muzzle = 1;
    m->muzzle_image_width_q16 = b3d_q16("0.70", B3D_Q16_ONE);
    m->muzzle_image_height_q16 = b3d_q16("0.70", B3D_Q16_ONE);
    m->muzzle_image_life_ms = 70U;
    m->muzzle_image_billboard = GWM89_MUZZLE_IMAGE_BILLBOARD_CAMERA;
    m->muzzle_image_blend = GWM89_MUZZLE_IMAGE_BLEND_ADDITIVE;
    m->muzzle_image_glow = 0;
    m->muzzle_image_glow_scale_q16 = b3d_q16("1.35", B3D_Q16_ONE);
    m->muzzle_image_glow_alpha = 96U;
    m->muzzle_light = 0;
    m->muzzle_light_life_ms = 70U;
    m->muzzle_light_intensity_q16 = b3d_q16("2.0", B3D_Q16_ONE);
    m->muzzle_light_radius_q16 = b3d_q16("7.0", B3D_Q16_ONE);
    m->muzzle_light_r = 255U;
    m->muzzle_light_g = 205U;
    m->muzzle_light_b = 120U;
    m->emit_casing = 1;
    m->emit_trail = 1;
    m->satellaborner_enabled = 0;
    m->telesearcher_enabled = 0;
    m->telesearcher_reacquire = 1;
    m->telesearcher_gain_q16 = B3D_Q16_ONE;
    m->expandible_fire_enabled = 0;
    m->expandible_fire_scale_start_q16 = B3D_Q16_ONE;
    m->expandible_fire_scale_end_q16 = B3D_Q16_ONE;
    m->expandible_fire_growth_distance_q16 = B3D_Q16_ONE;
    m->expandible_fire_kill_distance_q16 = 0L;
    m->expandible_fire_collision_growth = 1;
    m->bullet_circle_enabled = 0;
    m->bullet_circle_count = 1;
    m->bullet_circle_radius_q16 = 0L;
    m->bullet_circle_phase_turn_q16 = 0L;
    m->bullet_spin_enabled = 0;
    m->bullet_spin_start_slot = 0;
    m->bullet_spin_step = 1;
    m->bullet_spin_direction = 1;
    m->bullet_inline_enabled = 0;
    m->bullet_inline_distance_q16 = 0L;
    m->projectile_visual = GWM89_PROJECTILE_VISUAL_DEFAULT;
    m->projectile_visual_radial_start_q16 = B3D_Q16_ONE;
    m->projectile_visual_radial_peak_q16 = B3D_Q16_ONE;
    m->projectile_visual_radial_end_q16 = B3D_Q16_ONE;
    m->projectile_visual_axial_start_q16 = B3D_Q16_ONE;
    m->projectile_visual_axial_end_q16 = B3D_Q16_ONE;
    m->projectile_sprite_mode = GWM89_PROJECTILE_SPRITE_NONE;
    m->projectile_visual_billboard = 1;
    m->projectile_sprite_loop = GWM89_PROJECTILE_SPRITE_LOOP_FORWARD;
    b3d_copy(m->projectile_visual_clip, sizeof(m->projectile_visual_clip),
             "default");
    m->projectile_visual_frame_width = 0U;
    m->projectile_visual_frame_height = 0U;
    m->projectile_visual_columns = 1U;
    m->projectile_visual_rows = 1U;
    m->projectile_visual_margin_x = 0U;
    m->projectile_visual_margin_y = 0U;
    m->projectile_visual_spacing_x = 0U;
    m->projectile_visual_spacing_y = 0U;
    m->projectile_visual_start_x = 0U;
    m->projectile_visual_start_y = 0U;
    m->projectile_visual_step_x = 1;
    m->projectile_visual_step_y = 0;
    m->projectile_visual_frame_count = 1U;
    m->projectile_visual_frame_ms = 33U;
    m->projectile_visual_phase_step = 0U;
    m->projectile_visual_width_scale_q16 = B3D_Q16_ONE;
    m->projectile_visual_height_scale_q16 = B3D_Q16_ONE;
    m->projectile_visual_glow = 0;
    m->projectile_visual_glow_scale_q16 = b3d_q16("1.30", B3D_Q16_ONE);
    m->projectile_visual_glow_alpha = 96U;
    m->projectile_visual_core = 0;
    m->projectile_visual_core_scale_q16 = b3d_q16("0.72", B3D_Q16_ONE);
    m->projectile_visual_core_alpha = 160U;
    m->projectile_visual_light = 0;
    m->projectile_visual_light_intensity_q16 = b3d_q16("2.0", B3D_Q16_ONE);
    m->projectile_visual_light_radius_q16 = b3d_q16("8.0", B3D_Q16_ONE);
    m->projectile_visual_light_r = 255U;
    m->projectile_visual_light_g = 128U;
    m->projectile_visual_light_b = 48U;
    b3d_copy(m->crosshair_preset_name, sizeof(m->crosshair_preset_name),
             "default");
    m->mass_q16 = B3D_Q16_ONE;
    m->audio_enabled = 1;
    m->audio_profile = GWM89_AUDIO_PROFILE_PISTOL;
    m->audio_action = GWM89_AUDIO_ACTION_PISTOL;
    m->audio_magazine = GWM89_AUDIO_MAG_PISTOL_POLYMER;
    m->audio_ammo = GWM89_AUDIO_AMMO_BOX_MAG;
    m->audio_muzzle = GWM89_AUDIO_MUZZLE_BARE;
    m->audio_shell = GWM89_AUDIO_SHELL_PISTOL_BRASS;
    m->audio_detachable_magazine = 1;
    m->audio_emits_casing = 1;
    m->audio_projectile = GWM89_AUDIO_PROJECTILE_AUTO;
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
    else if (b3d_eq(key, "infinite_ammo") || b3d_eq(key, "ammo_infinite") || b3d_eq(key, "infinite")) p->infinite_ammo = b3d_bool(value, p->infinite_ammo);
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

static void b3d_apply_modules(GWeaponModules89 *m,
                              const char *key, const char *value)
{
    if (b3d_eq(key, "trigger")) m->trigger_model = b3d_trigger(value);
    else if (b3d_eq(key, "physics")) m->physics_backend = b3d_physics(value);
    else if (b3d_eq(key, "trail")) m->trail_profile = b3d_trail(value);
    else if (b3d_eq(key, "optic")) m->optic_profile = b3d_optic(value);
    else if (b3d_eq(key, "sway")) m->sway_enabled = b3d_bool(value, m->sway_enabled);
    else if (b3d_eq(key, "aim_query")) m->aim_query_enabled = b3d_bool(value, m->aim_query_enabled);
    else if (b3d_eq(key, "emit_muzzle")) m->emit_muzzle = b3d_bool(value, m->emit_muzzle);
    else if (b3d_eq(key, "muzzle_image") || b3d_eq(key, "muzzle_image_path"))
        b3d_copy(m->muzzle_image_path, sizeof(m->muzzle_image_path), value);
    else if (b3d_eq(key, "muzzle_image_width"))
        m->muzzle_image_width_q16 = b3d_q16(value, m->muzzle_image_width_q16);
    else if (b3d_eq(key, "muzzle_image_height"))
        m->muzzle_image_height_q16 = b3d_q16(value, m->muzzle_image_height_q16);
    else if (b3d_eq(key, "muzzle_image_ms") || b3d_eq(key, "muzzle_image_life_ms"))
        m->muzzle_image_life_ms = b3d_u16(value, m->muzzle_image_life_ms);
    else if (b3d_eq(key, "muzzle_image_blend")) {
        if (b3d_eq(value, "alpha")) m->muzzle_image_blend = GWM89_MUZZLE_IMAGE_BLEND_ALPHA;
        else m->muzzle_image_blend = GWM89_MUZZLE_IMAGE_BLEND_ADDITIVE;
    }
    else if (b3d_eq(key, "muzzle_image_billboard")) {
        if (b3d_eq(value, "fixed")) m->muzzle_image_billboard = GWM89_MUZZLE_IMAGE_BILLBOARD_FIXED;
        else if (b3d_eq(value, "view")) m->muzzle_image_billboard = GWM89_MUZZLE_IMAGE_BILLBOARD_VIEW;
        else m->muzzle_image_billboard = GWM89_MUZZLE_IMAGE_BILLBOARD_CAMERA;
    }
    else if (b3d_eq(key, "muzzle_image_glow"))
        m->muzzle_image_glow = b3d_bool(value, m->muzzle_image_glow);
    else if (b3d_eq(key, "muzzle_image_glow_scale"))
        m->muzzle_image_glow_scale_q16 = b3d_q16(value, m->muzzle_image_glow_scale_q16);
    else if (b3d_eq(key, "muzzle_image_glow_alpha")) {
        int glow_alpha;
        glow_alpha = b3d_int(value, (int)m->muzzle_image_glow_alpha);
        if (glow_alpha < 0) glow_alpha = 0;
        if (glow_alpha > 255) glow_alpha = 255;
        m->muzzle_image_glow_alpha = (unsigned short)glow_alpha;
    }
    else if (b3d_eq(key, "muzzle_light"))
        m->muzzle_light = b3d_bool(value, m->muzzle_light);
    else if (b3d_eq(key, "muzzle_light_ms") || b3d_eq(key, "muzzle_light_life_ms"))
        m->muzzle_light_life_ms = b3d_u16(value, m->muzzle_light_life_ms);
    else if (b3d_eq(key, "muzzle_light_intensity"))
        m->muzzle_light_intensity_q16 = b3d_q16(value, m->muzzle_light_intensity_q16);
    else if (b3d_eq(key, "muzzle_light_radius"))
        m->muzzle_light_radius_q16 = b3d_q16(value, m->muzzle_light_radius_q16);
    else if (b3d_eq(key, "muzzle_light_r") ||
             b3d_eq(key, "muzzle_light_g") ||
             b3d_eq(key, "muzzle_light_b")) {
        int channel;
        channel = b3d_int(value, 255);
        if (channel < 0) channel = 0;
        if (channel > 255) channel = 255;
        if (b3d_eq(key, "muzzle_light_r")) m->muzzle_light_r = (unsigned short)channel;
        else if (b3d_eq(key, "muzzle_light_g")) m->muzzle_light_g = (unsigned short)channel;
        else m->muzzle_light_b = (unsigned short)channel;
    }
    else if (b3d_eq(key, "emit_casing")) m->emit_casing = b3d_bool(value, m->emit_casing);
    else if (b3d_eq(key, "emit_trail")) m->emit_trail = b3d_bool(value, m->emit_trail);
    else if (b3d_eq(key, "projectile_mesh_id")) m->projectile_mesh_id = b3d_int(value, m->projectile_mesh_id);
    else if (b3d_eq(key, "casing_mesh_id")) m->casing_mesh_id = b3d_int(value, m->casing_mesh_id);
    else if (b3d_eq(key, "satellaborner") || b3d_eq(key, "spawn_at_target"))
        m->satellaborner_enabled = b3d_bool(value, m->satellaborner_enabled);
    else if (b3d_eq(key, "satellaborner_offset_x") || b3d_eq(key, "spawn_target_offset_x"))
        m->satellaborner_offset_x_q16 = b3d_q16(value, m->satellaborner_offset_x_q16);
    else if (b3d_eq(key, "satellaborner_offset_y") || b3d_eq(key, "spawn_target_offset_y"))
        m->satellaborner_offset_y_q16 = b3d_q16(value, m->satellaborner_offset_y_q16);
    else if (b3d_eq(key, "satellaborner_offset_z") || b3d_eq(key, "spawn_target_offset_z"))
        m->satellaborner_offset_z_q16 = b3d_q16(value, m->satellaborner_offset_z_q16);
    else if (b3d_eq(key, "telesearcher") || b3d_eq(key, "projectile_homing"))
        m->telesearcher_enabled = b3d_bool(value, m->telesearcher_enabled);
    else if (b3d_eq(key, "telesearcher_reacquire") || b3d_eq(key, "homing_reacquire"))
        m->telesearcher_reacquire = b3d_bool(value, m->telesearcher_reacquire);
    else if (b3d_eq(key, "telesearcher_gain") || b3d_eq(key, "homing_gain"))
        m->telesearcher_gain_q16 = b3d_q16(value, m->telesearcher_gain_q16);
    else if (b3d_eq(key, "telesearcher_offset_x") || b3d_eq(key, "homing_offset_x"))
        m->telesearcher_offset_x_q16 = b3d_q16(value, m->telesearcher_offset_x_q16);
    else if (b3d_eq(key, "telesearcher_offset_y") || b3d_eq(key, "homing_offset_y"))
        m->telesearcher_offset_y_q16 = b3d_q16(value, m->telesearcher_offset_y_q16);
    else if (b3d_eq(key, "telesearcher_offset_z") || b3d_eq(key, "homing_offset_z"))
        m->telesearcher_offset_z_q16 = b3d_q16(value, m->telesearcher_offset_z_q16);
    else if (b3d_eq(key, "expandible_fire") || b3d_eq(key, "expanding_projectile"))
        m->expandible_fire_enabled = b3d_bool(value, m->expandible_fire_enabled);
    else if (b3d_eq(key, "expandible_fire_scale_start") ||
             b3d_eq(key, "projectile_growth_scale_start"))
        m->expandible_fire_scale_start_q16 =
            b3d_q16(value, m->expandible_fire_scale_start_q16);
    else if (b3d_eq(key, "expandible_fire_scale_end") ||
             b3d_eq(key, "projectile_growth_scale_end"))
        m->expandible_fire_scale_end_q16 =
            b3d_q16(value, m->expandible_fire_scale_end_q16);
    else if (b3d_eq(key, "expandible_fire_growth_distance") ||
             b3d_eq(key, "projectile_growth_distance"))
        m->expandible_fire_growth_distance_q16 =
            b3d_q16(value, m->expandible_fire_growth_distance_q16);
    else if (b3d_eq(key, "expandible_fire_kill_distance") ||
             b3d_eq(key, "projectile_kill_distance"))
        m->expandible_fire_kill_distance_q16 =
            b3d_q16(value, m->expandible_fire_kill_distance_q16);
    else if (b3d_eq(key, "expandible_fire_collision_growth") ||
             b3d_eq(key, "projectile_growth_collision"))
        m->expandible_fire_collision_growth =
            b3d_bool(value, m->expandible_fire_collision_growth);
    else if (b3d_eq(key, "bullet_circle") || b3d_eq(key, "bulletcircle"))
        m->bullet_circle_enabled = b3d_bool(value, m->bullet_circle_enabled);
    else if (b3d_eq(key, "bullet_circle_count") || b3d_eq(key, "bulletcircle_count") ||
             b3d_eq(key, "barrel_count"))
        m->bullet_circle_count = b3d_int(value, m->bullet_circle_count);
    else if (b3d_eq(key, "bullet_circle_radius") || b3d_eq(key, "bulletcircle_radius") ||
             b3d_eq(key, "barrel_ring_radius"))
        m->bullet_circle_radius_q16 = b3d_q16(value, m->bullet_circle_radius_q16);
    else if (b3d_eq(key, "bullet_circle_phase") || b3d_eq(key, "bulletcircle_phase") ||
             b3d_eq(key, "barrel_ring_phase"))
        m->bullet_circle_phase_turn_q16 = b3d_q16(value, m->bullet_circle_phase_turn_q16);
    else if (b3d_eq(key, "bullet_spin") || b3d_eq(key, "bulletspin"))
        m->bullet_spin_enabled = b3d_bool(value, m->bullet_spin_enabled);
    else if (b3d_eq(key, "bullet_spin_start") || b3d_eq(key, "bulletspin_start"))
        m->bullet_spin_start_slot = b3d_int(value, m->bullet_spin_start_slot);
    else if (b3d_eq(key, "bullet_spin_step") || b3d_eq(key, "bulletspin_step"))
        m->bullet_spin_step = b3d_int(value, m->bullet_spin_step);
    else if (b3d_eq(key, "bullet_spin_direction") || b3d_eq(key, "bulletspin_direction"))
        m->bullet_spin_direction = b3d_spin_direction(value, m->bullet_spin_direction);
    else if (b3d_eq(key, "bullet_inline") || b3d_eq(key, "bulletinline") ||
             b3d_eq(key, "projectile_converge"))
        m->bullet_inline_enabled = b3d_bool(value, m->bullet_inline_enabled);
    else if (b3d_eq(key, "bullet_inline_distance") || b3d_eq(key, "bulletinline_distance") ||
             b3d_eq(key, "projectile_converge_distance"))
        m->bullet_inline_distance_q16 = b3d_q16(value, m->bullet_inline_distance_q16);
    else if (b3d_eq(key, "player_physical_projectile") ||
             b3d_eq(key, "physical_projectile") ||
             b3d_eq(key, "player_projectile_physical"))
        m->player_physical_projectile_enabled =
            b3d_bool(value, m->player_physical_projectile_enabled);
    else if (b3d_eq(key, "more_than_one") || b3d_eq(key, "morethanone"))
        m->more_than_one.enabled = b3d_bool(value, m->more_than_one.enabled);
    else if (b3d_eq(key, "projectile_visual") || b3d_eq(key, "projectile_fx"))
        m->projectile_visual = b3d_projectile_visual(value);
    else if (b3d_eq(key, "projectile_visual_expand_ms") || b3d_eq(key, "projectile_fx_expand_ms"))
        m->projectile_visual_expand_ms = b3d_u16(value, m->projectile_visual_expand_ms);
    else if (b3d_eq(key, "projectile_visual_crush_ms") || b3d_eq(key, "projectile_fx_crush_ms"))
        m->projectile_visual_crush_ms = b3d_u16(value, m->projectile_visual_crush_ms);
    else if (b3d_eq(key, "projectile_visual_radial_start") || b3d_eq(key, "projectile_fx_radial_start"))
        m->projectile_visual_radial_start_q16 = b3d_q16(value, m->projectile_visual_radial_start_q16);
    else if (b3d_eq(key, "projectile_visual_radial_peak") || b3d_eq(key, "projectile_fx_radial_peak"))
        m->projectile_visual_radial_peak_q16 = b3d_q16(value, m->projectile_visual_radial_peak_q16);
    else if (b3d_eq(key, "projectile_visual_radial_end") || b3d_eq(key, "projectile_fx_radial_end"))
        m->projectile_visual_radial_end_q16 = b3d_q16(value, m->projectile_visual_radial_end_q16);
    else if (b3d_eq(key, "projectile_visual_axial_start") || b3d_eq(key, "projectile_fx_axial_start"))
        m->projectile_visual_axial_start_q16 = b3d_q16(value, m->projectile_visual_axial_start_q16);
    else if (b3d_eq(key, "projectile_visual_axial_end") || b3d_eq(key, "projectile_fx_axial_end"))
        m->projectile_visual_axial_end_q16 = b3d_q16(value, m->projectile_visual_axial_end_q16);
    else if (b3d_eq(key, "projectile_visual_unlit") || b3d_eq(key, "projectile_fx_unlit"))
        m->projectile_visual_unlit = b3d_bool(value, m->projectile_visual_unlit);
    else if (b3d_eq(key, "projectile_visual_additive") || b3d_eq(key, "projectile_fx_additive"))
        m->projectile_visual_additive = b3d_bool(value, m->projectile_visual_additive);
    else if (b3d_eq(key, "projectile_sprite_mode") ||
             b3d_eq(key, "projectile_visual_mode") ||
             b3d_eq(key, "projectile_sprite_source"))
        m->projectile_sprite_mode = b3d_projectile_sprite_mode(value);
    else if (b3d_eq(key, "projectile_visual_billboard") ||
             b3d_eq(key, "projectile_billboard"))
        m->projectile_visual_billboard = b3d_bool(value, m->projectile_visual_billboard);
    else if (b3d_eq(key, "projectile_visual_loop") ||
             b3d_eq(key, "projectile_sprite_loop"))
        m->projectile_sprite_loop = b3d_projectile_sprite_loop(value, m->projectile_sprite_loop);
    else if (b3d_eq(key, "projectile_visual_recipe") ||
             b3d_eq(key, "projectile_sprite_recipe"))
        b3d_copy(m->projectile_visual_recipe, sizeof(m->projectile_visual_recipe), value);
    else if (b3d_eq(key, "projectile_visual_animation") ||
             b3d_eq(key, "projectile_sprite_animation"))
        b3d_copy(m->projectile_visual_animation, sizeof(m->projectile_visual_animation), value);
    else if (b3d_eq(key, "projectile_visual_clip") ||
             b3d_eq(key, "projectile_sprite_clip"))
        b3d_copy(m->projectile_visual_clip, sizeof(m->projectile_visual_clip), value);
    else if (b3d_eq(key, "projectile_visual_image") ||
             b3d_eq(key, "projectile_visual_asset") ||
             b3d_eq(key, "projectile_fx_image"))
        b3d_copy(m->projectile_visual_image, sizeof(m->projectile_visual_image), value);
    else if (b3d_eq(key, "projectile_visual_frame_width"))
        m->projectile_visual_frame_width = b3d_u16(value, m->projectile_visual_frame_width);
    else if (b3d_eq(key, "projectile_visual_frame_height"))
        m->projectile_visual_frame_height = b3d_u16(value, m->projectile_visual_frame_height);
    else if (b3d_eq(key, "projectile_visual_columns"))
        m->projectile_visual_columns = b3d_u16(value, m->projectile_visual_columns);
    else if (b3d_eq(key, "projectile_visual_rows"))
        m->projectile_visual_rows = b3d_u16(value, m->projectile_visual_rows);
    else if (b3d_eq(key, "projectile_visual_margin_x"))
        m->projectile_visual_margin_x = b3d_u16(value, m->projectile_visual_margin_x);
    else if (b3d_eq(key, "projectile_visual_margin_y"))
        m->projectile_visual_margin_y = b3d_u16(value, m->projectile_visual_margin_y);
    else if (b3d_eq(key, "projectile_visual_spacing_x"))
        m->projectile_visual_spacing_x = b3d_u16(value, m->projectile_visual_spacing_x);
    else if (b3d_eq(key, "projectile_visual_spacing_y"))
        m->projectile_visual_spacing_y = b3d_u16(value, m->projectile_visual_spacing_y);
    else if (b3d_eq(key, "projectile_visual_start_x"))
        m->projectile_visual_start_x = b3d_u16(value, m->projectile_visual_start_x);
    else if (b3d_eq(key, "projectile_visual_start_y"))
        m->projectile_visual_start_y = b3d_u16(value, m->projectile_visual_start_y);
    else if (b3d_eq(key, "projectile_visual_step_x"))
        m->projectile_visual_step_x = (short)b3d_int(value, (int)m->projectile_visual_step_x);
    else if (b3d_eq(key, "projectile_visual_step_y"))
        m->projectile_visual_step_y = (short)b3d_int(value, (int)m->projectile_visual_step_y);
    else if (b3d_eq(key, "projectile_visual_frame_count"))
        m->projectile_visual_frame_count = b3d_u16(value, m->projectile_visual_frame_count);
    else if (b3d_eq(key, "projectile_visual_frame_ms"))
        m->projectile_visual_frame_ms = b3d_u16(value, m->projectile_visual_frame_ms);
    else if (b3d_eq(key, "projectile_visual_phase_step"))
        m->projectile_visual_phase_step = b3d_u16(value, m->projectile_visual_phase_step);
    else if (b3d_eq(key, "projectile_visual_width_scale"))
        m->projectile_visual_width_scale_q16 = b3d_q16(value, m->projectile_visual_width_scale_q16);
    else if (b3d_eq(key, "projectile_visual_height_scale"))
        m->projectile_visual_height_scale_q16 = b3d_q16(value, m->projectile_visual_height_scale_q16);
    else if (b3d_eq(key, "projectile_visual_glow"))
        m->projectile_visual_glow = b3d_bool(value, m->projectile_visual_glow);
    else if (b3d_eq(key, "projectile_visual_glow_scale"))
        m->projectile_visual_glow_scale_q16 = b3d_q16(value, m->projectile_visual_glow_scale_q16);
    else if (b3d_eq(key, "projectile_visual_glow_alpha")) {
        int alpha;
        alpha = b3d_int(value, (int)m->projectile_visual_glow_alpha);
        if (alpha < 0) alpha = 0;
        if (alpha > 255) alpha = 255;
        m->projectile_visual_glow_alpha = (unsigned short)alpha;
    }
    else if (b3d_eq(key, "projectile_visual_core"))
        m->projectile_visual_core = b3d_bool(value, m->projectile_visual_core);
    else if (b3d_eq(key, "projectile_visual_core_scale"))
        m->projectile_visual_core_scale_q16 = b3d_q16(value, m->projectile_visual_core_scale_q16);
    else if (b3d_eq(key, "projectile_visual_core_alpha")) {
        int alpha;
        alpha = b3d_int(value, (int)m->projectile_visual_core_alpha);
        if (alpha < 0) alpha = 0;
        if (alpha > 255) alpha = 255;
        m->projectile_visual_core_alpha = (unsigned short)alpha;
    }
    else if (b3d_eq(key, "projectile_visual_light"))
        m->projectile_visual_light = b3d_bool(value, m->projectile_visual_light);
    else if (b3d_eq(key, "projectile_visual_light_intensity"))
        m->projectile_visual_light_intensity_q16 = b3d_q16(value, m->projectile_visual_light_intensity_q16);
    else if (b3d_eq(key, "projectile_visual_light_radius"))
        m->projectile_visual_light_radius_q16 = b3d_q16(value, m->projectile_visual_light_radius_q16);
    else if (b3d_eq(key, "projectile_visual_light_r") ||
             b3d_eq(key, "projectile_visual_light_g") ||
             b3d_eq(key, "projectile_visual_light_b")) {
        int channel;
        channel = b3d_int(value, 255);
        if (channel < 0) channel = 0;
        if (channel > 255) channel = 255;
        if (b3d_eq(key, "projectile_visual_light_r"))
            m->projectile_visual_light_r = (unsigned short)channel;
        else if (b3d_eq(key, "projectile_visual_light_g"))
            m->projectile_visual_light_g = (unsigned short)channel;
        else m->projectile_visual_light_b = (unsigned short)channel;
    }
    else if (b3d_eq(key, "scope_recipe")) b3d_copy(m->scope_recipe_path, sizeof(m->scope_recipe_path), value);
    else if (b3d_eq(key, "scope_preset")) b3d_copy(m->scope_preset_name, sizeof(m->scope_preset_name), value);
    else if (b3d_eq(key, "crosshair_preset")) b3d_copy(m->crosshair_preset_name, sizeof(m->crosshair_preset_name), value);
    else if (b3d_eq(key, "crosshair_preset_first_person") ||
             b3d_eq(key, "crosshair_preset_fp"))
        b3d_copy(m->crosshair_preset_first_person,
                 sizeof(m->crosshair_preset_first_person), value);
    else if (b3d_eq(key, "crosshair_preset_third_person") ||
             b3d_eq(key, "crosshair_preset_tp"))
        b3d_copy(m->crosshair_preset_third_person,
                 sizeof(m->crosshair_preset_third_person), value);
    else if (b3d_eq(key, "charge_time_ms")) m->charge_time_ms = b3d_u16(value, m->charge_time_ms);
    else if (b3d_eq(key, "charge_min_speed")) m->charge_min_speed_q16 = b3d_q16(value, m->charge_min_speed_q16);
    else if (b3d_eq(key, "charge_max_speed")) m->charge_max_speed_q16 = b3d_q16(value, m->charge_max_speed_q16);
    else if (b3d_eq(key, "gravity")) m->gravity_q16 = b3d_q16(value, m->gravity_q16);
    else if (b3d_eq(key, "bounce")) m->bounce_q16 = b3d_q16(value, m->bounce_q16);
    else if (b3d_eq(key, "drag")) m->drag_q16 = b3d_q16(value, m->drag_q16);
    else if (b3d_eq(key, "mass")) m->mass_q16 = b3d_q16(value, m->mass_q16);
}

static void b3d_apply_audio(GWeaponModules89 *m,
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

static int b3d_more_than_one_stage_index(const char *section)
{
    const char *p;
    int value;
    if (!section) return -1;
    if (strncmp(section, "more_than_one_", 14U) == 0) p = section + 14;
    else if (strncmp(section, "morethanone_", 12U) == 0) p = section + 12;
    else return -1;
    if (!*p) return -1;
    value = 0;
    while (*p) {
        if (*p < '0' || *p > '9') return -1;
        value = value * 10 + (*p - '0');
        ++p;
    }
    return value >= 0 && value < MTO89_MAX_STAGES ? value : -1;
}

static void b3d_apply_more_than_one(GWeaponModules89 *m,
                                    const char *key, const char *value)
{
    if (!m) return;
    if (b3d_eq(key, "enabled"))
        m->more_than_one.enabled = b3d_bool(value, m->more_than_one.enabled);
    else if (b3d_eq(key, "activation_ms"))
        m->more_than_one.activation_ms = (unsigned int)b3d_int(value, (int)m->more_than_one.activation_ms);
    else if (b3d_eq(key, "stages") || b3d_eq(key, "stage_count")) {
        int n = b3d_int(value, m->more_than_one.stage_count);
        if (n < 0) n = 0;
        if (n > MTO89_MAX_STAGES) n = MTO89_MAX_STAGES;
        m->more_than_one.stage_count = n;
    }
}

static void b3d_apply_more_than_one_stage(GWeaponModules89 *m, int index,
                                          const char *key, const char *value)
{
    mto89_stage *s;
    if (!m || index < 0 || index >= MTO89_MAX_STAGES) return;
    s = &m->more_than_one.stages[index];
    if (m->more_than_one.stage_count <= index)
        m->more_than_one.stage_count = index + 1;
    if (b3d_eq(key, "min_ms")) s->min_ms = (unsigned int)b3d_int(value, (int)s->min_ms);
    else if (b3d_eq(key, "projectile_id")) {
        s->projectile_id = b3d_int(value, s->projectile_id);
        s->override_mask |= MTO89_OVERRIDE_PROJECTILE_ID;
    } else if (b3d_eq(key, "projectile_mesh_id")) {
        s->projectile_mesh_id = b3d_int(value, s->projectile_mesh_id);
        s->override_mask |= MTO89_OVERRIDE_PROJECTILE_MESH_ID;
    } else if (b3d_eq(key, "damage")) {
        s->damage_q16 = b3d_q16(value, s->damage_q16);
        s->override_mask |= MTO89_OVERRIDE_DAMAGE_Q16;
    } else if (b3d_eq(key, "speed") || b3d_eq(key, "projectile_speed")) {
        s->speed_q16 = b3d_q16(value, s->speed_q16);
        s->override_mask |= MTO89_OVERRIDE_SPEED_Q16;
    } else if (b3d_eq(key, "radius") || b3d_eq(key, "projectile_radius")) {
        s->radius_q16 = b3d_q16(value, s->radius_q16);
        s->override_mask |= MTO89_OVERRIDE_RADIUS_Q16;
    } else if (b3d_eq(key, "mesh_scale") || b3d_eq(key, "projectile_mesh_scale")) {
        s->mesh_scale_q16 = b3d_q16(value, s->mesh_scale_q16);
        s->override_mask |= MTO89_OVERRIDE_MESH_SCALE_Q16;
    } else if (b3d_eq(key, "life_ms") || b3d_eq(key, "projectile_life_ms")) {
        s->life_ms = b3d_u16(value, s->life_ms);
        s->override_mask |= MTO89_OVERRIDE_LIFE_MS;
    }
}

static int b3d_more_than_one_valid(const mto89_profile *profile)
{
    int i;
    if (!profile || !profile->enabled) return 1;
    if (profile->stage_count <= 0 || profile->stage_count > MTO89_MAX_STAGES) return 0;
    if (profile->stages[0].min_ms < profile->activation_ms) return 0;
    for (i = 1; i < profile->stage_count; ++i)
        if (profile->stages[i].min_ms <= profile->stages[i - 1].min_ms) return 0;
    return 1;
}

static int b3d_load_weapon_file(GWP89_Manager *manager,
                                const char *path,
                                GWP89_WeaponProfile *profile,
                                GWeaponModules89 *modules,
                                char *status, size_t status_capacity)
{
    char file_text[GWPROFILE89_TEXT_CAP];
    char line[B3D_INI_LINE_CAP];
    char section[32];
    char *text;
    char *equals;
    char *key;
    char *value;
    GWProfile89TextReader reader;
    int length;
    b3d_profile_defaults(profile, modules);
    section[0] = '\0';
    if (!gweaponio89_read_text(manager, path, file_text,
                               (int)sizeof(file_text), &length)) {
        b3d_status(status, status_capacity,
                   "weapon INI could not be read by host IO provider");
        return 0;
    }
    (void)length;
    reader.cursor = file_text;
    while (gwprofile89_next_line(&reader, line, sizeof(line))) {
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
        else if (b3d_eq(section, "more_than_one") || b3d_eq(section, "morethanone"))
            b3d_apply_more_than_one(modules, key, value);
        else {
            int stage_index;
            stage_index = b3d_more_than_one_stage_index(section);
            if (stage_index >= 0)
                b3d_apply_more_than_one_stage(modules, stage_index, key, value);
            else if (b3d_eq(section, "audio")) b3d_apply_audio(modules, key, value);
        }
    }
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
    if (modules->more_than_one.enabled && !b3d_more_than_one_valid(&modules->more_than_one)) {
        b3d_status(status, status_capacity, "weapon INI invalid more_than_one stages");
        return 0;
    }
    return 1;
}

int gweaponprofileio89_load_manifest(GWP89_Manager *manager,
                                     const char *manifest_path,
                                     char *status,
                                     size_t status_capacity)
{
    char manifest_text[GWPROFILE89_TEXT_CAP];
    char line[B3D_INI_LINE_CAP];
    char directory[B3D_INI_PATH_CAP];
    char path[B3D_INI_PATH_CAP];
    char section[32];
    char *text;
    char *equals;
    char *value;
    GWP89_WeaponProfile profile;
    GWeaponModules89 modules;
    GWProfile89TextReader reader;
    int count;
    int length;
    if (!manager || !manifest_path) return 0;
    if (!gweaponio89_read_text(manager, manifest_path, manifest_text,
                               (int)sizeof(manifest_text), &length)) {
        b3d_status(status, status_capacity,
                   "weapon manifest unavailable; built-in fallback used");
        return 0;
    }
    (void)length;
    b3d_manifest_dir(manifest_path, directory, sizeof(directory));
    gweaponmodules89_reset();
    section[0] = '\0';
    count = 0;
    reader.cursor = manifest_text;
    while (gwprofile89_next_line(&reader, line, sizeof(line))) {
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
        if (!b3d_eq(section, "weapons")) continue;
        equals = strchr(text, '=');
        if (!equals) continue;
        value = b3d_trim(equals + 1);
        if (!*value) continue;
        b3d_join_path(directory, value, path, sizeof(path));
        if (!b3d_load_weapon_file(manager, path, &profile, &modules,
                                  status, status_capacity)) {
            gweaponmodules89_load_defaults();
            return 0;
        }
        if (gwp89_add_weapon(manager, &profile) < 0 ||
            !gweaponmodules89_register(&modules)) {
            b3d_status(status, status_capacity,
                       "weapon registry capacity exceeded");
            gweaponmodules89_load_defaults();
            return 0;
        }
        ++count;
        if (count >= B3D_INI_MAX_WEAPONS) break;
    }
    if (count <= 0) {
        gweaponmodules89_load_defaults();
        b3d_status(status, status_capacity,
                   "weapon manifest contained no files");
        return 0;
    }
    b3d_status(status, status_capacity, "weapon INI registry loaded");
    return count;
}
