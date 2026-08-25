#include "blank3d_katana_melee.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define B3D_KATANA_RIG_TAG 9271
#define B3D_KATANA_PLAYER_ID 1
#define B3D_KATANA_PLAYER_TEAM 1
#define B3D_KATANA_ENEMY_TEAM 2
#define B3D_KATANA_MESH_ID 0

static void b3d_katana_trim(char *text)
{
    char *start;
    char *end;
    if (!text) return;
    start = text;
    while (*start && isspace((unsigned char)*start)) ++start;
    if (start != text) memmove(text, start, strlen(start) + 1U);
    end = text + strlen(text);
    while (end > text && isspace((unsigned char)end[-1])) --end;
    *end = '\0';
}

static int b3d_katana_bool(const char *text, int fallback)
{
    if (!text) return fallback;
    if (strcmp(text, "1") == 0 || strcmp(text, "true") == 0 ||
        strcmp(text, "yes") == 0 || strcmp(text, "on") == 0)
        return 1;
    if (strcmp(text, "0") == 0 || strcmp(text, "false") == 0 ||
        strcmp(text, "no") == 0 || strcmp(text, "off") == 0)
        return 0;
    return fallback;
}

void blank3d_katana_config_defaults(Blank3DKatanaConfig *config)
{
    if (!config) return;
    memset(config, 0, sizeof(*config));
    config->enabled = 1;
    config->debug_draw = 1;
    config->preset_index = 0U;
    config->mesh_scale_thickness_percent = 220;
    config->mesh_scale_width_percent = 180;
    config->mesh_scale_length_percent = 180;
    config->mount_lateral_cm = 0;
    config->mount_height_cm = 105;
    config->mount_forward_cm = 190;
    config->tick_ms = 16U;
    config->duration_frames = 28U;
    config->active_start_frame = 7U;
    config->active_end_frame = 14U;
    config->active_start_ms = 0U;
    config->active_end_ms = 0U;
    config->damage = 34;
    config->stun_frames = 18;
    config->hitstop_frames = 4;
    config->one_hit_per_enemy = 1;
    config->slash_start_yaw_deg = -78;
    config->slash_contact_yaw_deg = 20;
    config->slash_end_yaw_deg = 112;
    config->slash_pitch_deg = -18;
    config->slash_roll_deg = -8;
    config->blade_base_cm = 4;
    config->blade_tip_cm = 68;
    config->damage_radius_cm = 9;
    config->damage_box_half_width_cm = 8;
    config->damage_box_half_thickness_cm = 6;
}

static void b3d_katana_set_config_value(Blank3DKatanaConfig *config,
                                         const char *key,
                                         const char *value)
{
    int number;
    if (!config || !key || !value) return;
    number = atoi(value);
    if (strcmp(key, "enabled") == 0)
        config->enabled = b3d_katana_bool(value, config->enabled);
    else if (strcmp(key, "debug_draw") == 0)
        config->debug_draw = b3d_katana_bool(value, config->debug_draw);
    else if (strcmp(key, "preset") == 0)
        config->preset_index = (unsigned short)(number < 0 ? 0 : number);
    else if (strcmp(key, "mesh_scale_thickness_percent") == 0)
        config->mesh_scale_thickness_percent = number < 1 ? 1 : number;
    else if (strcmp(key, "mesh_scale_width_percent") == 0)
        config->mesh_scale_width_percent = number < 1 ? 1 : number;
    else if (strcmp(key, "mesh_scale_length_percent") == 0)
        config->mesh_scale_length_percent = number < 1 ? 1 : number;
    else if (strcmp(key, "mount_lateral_cm") == 0)
        config->mount_lateral_cm = number;
    else if (strcmp(key, "mount_height_cm") == 0)
        config->mount_height_cm = number;
    else if (strcmp(key, "mount_forward_cm") == 0)
        config->mount_forward_cm = number < 1 ? 1 : number;
    else if (strcmp(key, "tick_ms") == 0)
        config->tick_ms = (unsigned short)(number < 1 ? 1 : number);
    else if (strcmp(key, "duration_frames") == 0)
        config->duration_frames = (unsigned short)(number < 2 ? 2 : number);
    else if (strcmp(key, "active_start_frame") == 0)
        config->active_start_frame = (unsigned short)(number < 0 ? 0 : number);
    else if (strcmp(key, "active_end_frame") == 0)
        config->active_end_frame = (unsigned short)(number < 0 ? 0 : number);
    else if (strcmp(key, "active_start_ms") == 0)
        config->active_start_ms = (unsigned short)(number < 0 ? 0 : number);
    else if (strcmp(key, "active_end_ms") == 0)
        config->active_end_ms = (unsigned short)(number < 0 ? 0 : number);
    else if (strcmp(key, "damage") == 0)
        config->damage = number;
    else if (strcmp(key, "stun_frames") == 0)
        config->stun_frames = number;
    else if (strcmp(key, "hitstop_frames") == 0)
        config->hitstop_frames = number;
    else if (strcmp(key, "one_hit_per_enemy") == 0)
        config->one_hit_per_enemy = b3d_katana_bool(value,
                                      config->one_hit_per_enemy);
    else if (strcmp(key, "slash_start_yaw_deg") == 0)
        config->slash_start_yaw_deg = number;
    else if (strcmp(key, "slash_contact_yaw_deg") == 0)
        config->slash_contact_yaw_deg = number;
    else if (strcmp(key, "slash_end_yaw_deg") == 0)
        config->slash_end_yaw_deg = number;
    else if (strcmp(key, "slash_pitch_deg") == 0)
        config->slash_pitch_deg = number;
    else if (strcmp(key, "slash_roll_deg") == 0)
        config->slash_roll_deg = number;
    else if (strcmp(key, "blade_base_cm") == 0)
        config->blade_base_cm = number;
    else if (strcmp(key, "blade_tip_cm") == 0)
        config->blade_tip_cm = number;
    else if (strcmp(key, "damage_radius_cm") == 0) {
        /* Backward-compatible alias from the capsule prototype. */
        config->damage_radius_cm = number;
        config->damage_box_half_width_cm = number;
        config->damage_box_half_thickness_cm = number;
    } else if (strcmp(key, "damage_box_half_width_cm") == 0)
        config->damage_box_half_width_cm = number;
    else if (strcmp(key, "damage_box_half_thickness_cm") == 0)
        config->damage_box_half_thickness_cm = number;
}

int blank3d_katana_config_load(Blank3DKatanaConfig *config,
                               const char *path)
{
    FILE *file;
    char line[256];
    char *equals;
    char *comment;
    if (!config || !path) return 0;
    file = fopen(path, "rb");
    if (!file) return 0;
    while (fgets(line, sizeof(line), file)) {
        comment = strchr(line, ';');
        if (comment) *comment = '\0';
        comment = strchr(line, '#');
        if (comment) *comment = '\0';
        b3d_katana_trim(line);
        if (!line[0] || line[0] == '[') continue;
        equals = strchr(line, '=');
        if (!equals) continue;
        *equals = '\0';
        b3d_katana_trim(line);
        b3d_katana_trim(equals + 1);
        b3d_katana_set_config_value(config, line, equals + 1);
    }
    fclose(file);
    return 1;
}

static nm89_fx b3d_katana_meters_q16_from_cm(int centimetres)
{
    return nm89_fx_from_ratio((nm89_i32)centimetres, 100);
}

static hb3_fx b3d_katana_q12_to_hitfx(int value)
{
    return (hb3_fx)(value / 256);
}

static int b3d_katana_hitfx_to_q12(hb3_fx value)
{
    return value * 256;
}

static int b3d_katana_sample_world(void *user,
                                   const nm89_rig *rig,
                                   nm89_i16 part_id,
                                   nm89_i16 source_id,
                                   nm89_matrix *out_world)
{
    Blank3DKatanaMelee *melee;
    (void)rig;
    (void)part_id;
    (void)source_id;
    melee = (Blank3DKatanaMelee *)user;
    if (!melee || !out_world || !melee->root_valid)
        return NM89_ERR_PROVIDER;
    *out_world = melee->root_world;
    return NM89_OK;
}

static void b3d_katana_apply_geometry(void *user,
                                      const nm89_rig *rig,
                                      const nm89_geometry_packet *packet)
{
    Blank3DKatanaMelee *melee;
    (void)rig;
    melee = (Blank3DKatanaMelee *)user;
    if (!melee || !packet) return;
    melee->packet = *packet;
    melee->packet_valid = packet->visible ? 1 : 0;
}

static void b3d_katana_emit_event(void *user,
                                  const nm89_rig *rig,
                                  const nm89_clip_event *event_value)
{
    Blank3DKatanaMelee *melee;
    (void)rig;
    melee = (Blank3DKatanaMelee *)user;
    if (!melee || !event_value) return;
    if (event_value->code == B3D_KATANA_EVENT_ACTIVE_BEGIN)
        melee->collision_window = 1;
    else if (event_value->code == B3D_KATANA_EVENT_ACTIVE_END)
        melee->collision_window = 0;
}

static int b3d_katana_add_track3(nm89_rig *rig,
                                 nm89_i16 part,
                                 nm89_u8 property,
                                 nm89_u16 tick0,
                                 nm89_fx value0,
                                 nm89_u16 tick1,
                                 nm89_fx value1,
                                 nm89_u16 tick2,
                                 nm89_fx value2)
{
    nm89_i16 track;
    int result;
    result = nm89_clip_add_track(rig, part, property,
                                 NM89_INTERP_SMOOTH, &track);
    if (result != NM89_OK) return result;
    if (nm89_track_add_key(rig, track, tick0, value0) != NM89_OK ||
        nm89_track_add_key(rig, track, tick1, value1) != NM89_OK ||
        nm89_track_add_key(rig, track, tick2, value2) != NM89_OK)
        return NM89_ERR_CAPACITY;
    return NM89_OK;
}

static int b3d_katana_build_clip(Blank3DKatanaMelee *melee)
{
    nm89_i16 track;
    nm89_clip_event event_value;
    nm89_u16 duration;
    nm89_u16 start;
    nm89_u16 end;
    nm89_u16 contact;
    int result;

    duration = melee->config.duration_frames;
    start = melee->config.active_start_frame;
    end = melee->config.active_end_frame;
    if (start >= duration) start = (nm89_u16)(duration / 3U);
    if (end <= start) end = (nm89_u16)(start + 1U);
    if (end >= duration) end = (nm89_u16)(duration - 1U);
    contact = (nm89_u16)(start + (end - start) / 2U);

    result = nm89_clip_begin(&melee->rig, B3D_KATANA_ACTION_SLASH,
                             duration, 0U, &melee->clip_id);
    if (result != NM89_OK) return result;

    result = nm89_clip_add_track(&melee->rig, melee->blade_part,
                                 NM89_PROP_ROTATE_Y,
                                 NM89_INTERP_SMOOTH, &track);
    if (result != NM89_OK) return result;
    if (nm89_track_add_key(&melee->rig, track, 0U,
            NM89_FX_FROM_INT(melee->config.slash_start_yaw_deg)) != NM89_OK ||
        nm89_track_add_key(&melee->rig, track, start,
            NM89_FX_FROM_INT(melee->config.slash_start_yaw_deg + 12)) != NM89_OK ||
        nm89_track_add_key(&melee->rig, track, contact,
            NM89_FX_FROM_INT(melee->config.slash_contact_yaw_deg)) != NM89_OK ||
        nm89_track_add_key(&melee->rig, track, end,
            NM89_FX_FROM_INT(melee->config.slash_end_yaw_deg)) != NM89_OK ||
        nm89_track_add_key(&melee->rig, track, duration,
            NM89_FX_FROM_INT(melee->config.slash_start_yaw_deg)) != NM89_OK)
        return NM89_ERR_CAPACITY;

    result = b3d_katana_add_track3(&melee->rig, melee->blade_part,
        NM89_PROP_ROTATE_X,
        0U, NM89_FX_FROM_INT(melee->config.slash_pitch_deg),
        contact, NM89_FX_FROM_INT(melee->config.slash_pitch_deg + 28),
        duration, NM89_FX_FROM_INT(melee->config.slash_pitch_deg));
    if (result != NM89_OK) return result;
    result = b3d_katana_add_track3(&melee->rig, melee->blade_part,
        NM89_PROP_ROTATE_Z,
        0U, NM89_FX_FROM_INT(melee->config.slash_roll_deg),
        contact, NM89_FX_FROM_INT(melee->config.slash_roll_deg + 18),
        duration, NM89_FX_FROM_INT(melee->config.slash_roll_deg));
    if (result != NM89_OK) return result;

    memset(&event_value, 0, sizeof(event_value));
    event_value.tick = start;
    event_value.part_id = melee->blade_part;
    event_value.code = B3D_KATANA_EVENT_ACTIVE_BEGIN;
    event_value.type = NM89_EVENT_MARKER;
    result = nm89_clip_add_event(&melee->rig, &event_value);
    if (result != NM89_OK) return result;
    event_value.tick = (nm89_u16)(end + 1U <= duration ? end + 1U : end);
    event_value.code = B3D_KATANA_EVENT_ACTIVE_END;
    result = nm89_clip_add_event(&melee->rig, &event_value);
    if (result != NM89_OK) return result;

    result = nm89_clip_end(&melee->rig);
    if (result != NM89_OK) return result;
    return nm89_action_bind(&melee->rig, B3D_KATANA_ACTION_SLASH,
                            melee->clip_id, NM89_ACTION_RESTART);
}

static int b3d_katana_target_registered(const Blank3DKatanaMelee *melee,
                                         int actor_id)
{
    int i;
    for (i = 0; i < melee->registered_target_count; ++i) {
        if (melee->registered_targets[i] == actor_id) return 1;
    }
    return 0;
}

static void b3d_katana_push_hit(Blank3DKatanaMelee *melee,
                                const ml3_event *event_value)
{
    Blank3DKatanaHit *hit;
    if (!melee || !event_value) return;
    if (melee->hit_count >= B3D_KATANA_MAX_HITS) return;
    hit = &melee->hits[melee->hit_tail];
    memset(hit, 0, sizeof(*hit));
    hit->attacker_id = event_value->attacker_id;
    hit->defender_id = event_value->defender_id;
    hit->damage = event_value->damage;
    hit->stun_frames = event_value->stun_frames;
    hit->hitstop_frames = event_value->hitstop_frames;
    hit->point_x_q12 = b3d_katana_hitfx_to_q12(event_value->point.x);
    hit->point_y_q12 = b3d_katana_hitfx_to_q12(event_value->point.y);
    hit->point_z_q12 = b3d_katana_hitfx_to_q12(event_value->point.z);
    melee->hit_tail += 1;
    if (melee->hit_tail >= B3D_KATANA_MAX_HITS) melee->hit_tail = 0;
    melee->hit_count += 1;
}

static void b3d_katana_drain_events(Blank3DKatanaMelee *melee)
{
    ml3_event event_value;
    while (ml3_poll_event(&melee->collision, &event_value))
        b3d_katana_push_hit(melee, &event_value);
}

int blank3d_katana_melee_init(Blank3DKatanaMelee *melee,
                              const Blank3DKatanaConfig *config)
{
    Blank3DKatanaConfig fallback;
    nm89_transform root_home;
    nm89_transform blade_home;
    int result;

    if (!melee) return 0;
    if (!config) {
        blank3d_katana_config_defaults(&fallback);
        config = &fallback;
    }
    memset(melee, 0, sizeof(*melee));
    melee->config = *config;
    if (melee->config.tick_ms == 0U) melee->config.tick_ms = 16U;
    if (melee->config.active_start_ms > 0U)
        melee->config.active_start_frame = (unsigned short)(
            (melee->config.active_start_ms + melee->config.tick_ms - 1U) /
            melee->config.tick_ms);
    if (melee->config.active_end_ms > 0U)
        melee->config.active_end_frame = (unsigned short)(
            (melee->config.active_end_ms + melee->config.tick_ms - 1U) /
            melee->config.tick_ms);
    if (melee->config.active_end_frame <= melee->config.active_start_frame)
        melee->config.active_end_frame =
            (unsigned short)(melee->config.active_start_frame + 1U);
    if (melee->config.duration_frames <= melee->config.active_end_frame)
        melee->config.duration_frames =
            (unsigned short)(melee->config.active_end_frame + 4U);

    melee->enabled = melee->config.enabled;
    melee->provider_id = NM89_INVALID_ID;
    melee->root_part = NM89_INVALID_ID;
    melee->blade_part = NM89_INVALID_ID;
    melee->clip_id = NM89_INVALID_ID;
    melee->attack_handle = -1;
    nm89_matrix_identity(&melee->root_world);
    nm89_rig_init(&melee->rig, B3D_KATANA_RIG_TAG);
    memset(&melee->provider, 0, sizeof(melee->provider));
    melee->provider.user = melee;
    melee->provider.sample_world = b3d_katana_sample_world;
    melee->provider.apply_geometry = b3d_katana_apply_geometry;
    melee->provider.emit_event = b3d_katana_emit_event;
    result = nm89_provider_add(&melee->rig, &melee->provider,
                               &melee->provider_id);
    if (result != NM89_OK) return 0;

    nm89_transform_identity(&root_home);
    nm89_transform_identity(&blade_home);
    /* Native katana89 origin is near the guard.  Raise it slightly so the
       right-hand carrier grips the tsuka instead of the tsuba. */
    blade_home.move.x = b3d_katana_meters_q16_from_cm(7);
    blade_home.move.y = b3d_katana_meters_q16_from_cm(-4);
    blade_home.move.z = b3d_katana_meters_q16_from_cm(15);
    blade_home.rotate.x = NM89_FX_FROM_INT(melee->config.slash_pitch_deg);
    blade_home.rotate.y = NM89_FX_FROM_INT(melee->config.slash_start_yaw_deg);
    blade_home.rotate.z = NM89_FX_FROM_INT(melee->config.slash_roll_deg);

    result = nm89_part_add(&melee->rig, "katana_root", NM89_ROOT_PART,
        1, &root_home, NM89_INVALID_ID, NM89_INVALID_ID, 1U,
        &melee->root_part);
    if (result != NM89_OK) return 0;
    result = nm89_part_add(&melee->rig, "katana_blade", melee->root_part,
        2, &blade_home, NM89_INVALID_ID, NM89_INVALID_ID, 1U,
        &melee->blade_part);
    if (result != NM89_OK) return 0;
    result = nm89_part_bind_world_provider(&melee->rig, melee->root_part,
        melee->provider_id, 0, NM89_WORLD_REPLACE);
    if (result != NM89_OK) return 0;
    result = nm89_binding_add(&melee->rig, melee->blade_part,
        B3D_KATANA_MESH_ID, NM89_SELECTOR_ENGINE_MESH,
        B3D_KATANA_MESH_ID, B3D_KATANA_MESH_ID,
        NM89_INVALID_ID, NM89_INVALID_ID, 0, 0);
    if (result != NM89_OK) return 0;
    result = b3d_katana_build_clip(melee);
    if (result != NM89_OK) return 0;

    ml3_world_init(&melee->collision);
    (void)ml3_actor_add(&melee->collision,
                        B3D_KATANA_PLAYER_ID, B3D_KATANA_PLAYER_TEAM);
    melee->root_valid = 1;
    melee->packet_valid = 0;
    if (nm89_step(&melee->rig, 0U, 1U) != NM89_OK) return 0;
    melee->initialized = 1;
    return 1;
}

void blank3d_katana_melee_set_root_pose(Blank3DKatanaMelee *melee,
                                        const soq3d_pose *pose)
{
    if (!melee || !pose) return;
    nm89_matrix_identity(&melee->root_world);
    melee->root_world.m[0][0] = pose->basis.m00;
    melee->root_world.m[0][1] = pose->basis.m01;
    melee->root_world.m[0][2] = pose->basis.m02;
    melee->root_world.m[0][3] = pose->position.x;
    melee->root_world.m[1][0] = pose->basis.m10;
    melee->root_world.m[1][1] = pose->basis.m11;
    melee->root_world.m[1][2] = pose->basis.m12;
    melee->root_world.m[1][3] = pose->position.y;
    melee->root_world.m[2][0] = pose->basis.m20;
    melee->root_world.m[2][1] = pose->basis.m21;
    melee->root_world.m[2][2] = pose->basis.m22;
    melee->root_world.m[2][3] = pose->position.z;
    melee->root_valid = 1;
}

void blank3d_katana_melee_clear_targets(Blank3DKatanaMelee *melee)
{
    int i;
    if (!melee || !melee->initialized) return;
    for (i = 0; i < melee->registered_target_count; ++i)
        (void)ml3_actor_clear_hurtboxes(&melee->collision,
                                          melee->registered_targets[i]);
}

int blank3d_katana_melee_set_target_q12(Blank3DKatanaMelee *melee,
                                        int actor_id,
                                        int team,
                                        int alive,
                                        int x_q12,
                                        int y_q12,
                                        int z_q12,
                                        int height_q12,
                                        int radius_q12)
{
    hb3_v3 body_center;
    hb3_v3 body_half;
    hb3_v3 head_center;
    hb3_v3 head_half;
    hb3_mat3 basis;
    hb3_fx height;
    hb3_fx radius;
    hb3_fx body_half_y;
    hb3_fx head_half_y;
    hb3_fx head_radius;
    int result;
    if (!melee || !melee->initialized || actor_id <= 0) return 0;
    if (!b3d_katana_target_registered(melee, actor_id)) {
        if (melee->registered_target_count >= B3D_KATANA_MAX_TARGETS)
            return 0;
        if (ml3_actor_add(&melee->collision, actor_id, team) != ML3_OK)
            return 0;
        melee->registered_targets[melee->registered_target_count++] = actor_id;
    }
    (void)ml3_actor_clear_hurtboxes(&melee->collision, actor_id);
    if (!alive) return 1;
    height = b3d_katana_q12_to_hitfx(height_q12);
    radius = b3d_katana_q12_to_hitfx(radius_q12);
    if (height < 4) height = 4;
    if (radius < 1) radius = 1;

    /* Enemy vulnerability is represented by real box primitives so F3
       displays green boxes instead of capsule wireframes.  The torso and
       head are separate OBBs, both axis-aligned for this prototype. */
    basis = hb3_mat3_identity();
    body_half_y = (hb3_fx)((height * 35) / 100);
    if (body_half_y < 1) body_half_y = 1;
    body_center = hb3_v3_make(
        b3d_katana_q12_to_hitfx(x_q12),
        (hb3_fx)(b3d_katana_q12_to_hitfx(y_q12) +
                 (height * 42) / 100),
        b3d_katana_q12_to_hitfx(z_q12));
    body_half = hb3_v3_make(radius, body_half_y,
                            (hb3_fx)((radius * 4) / 5));
    if (body_half.z < 1) body_half.z = 1;
    result = ml3_actor_add_hurt_obb(&melee->collision, actor_id, 1,
        body_center, body_half, basis, HB3_HURT_FLESH, 0);
    if (result != ML3_OK) return 0;

    head_radius = (hb3_fx)((radius * 3) / 5);
    if (head_radius < 1) head_radius = 1;
    head_half_y = (hb3_fx)((height * 11) / 100);
    if (head_half_y < 1) head_half_y = 1;
    head_center = hb3_v3_make(
        b3d_katana_q12_to_hitfx(x_q12),
        (hb3_fx)(b3d_katana_q12_to_hitfx(y_q12) +
                 (height * 86) / 100),
        b3d_katana_q12_to_hitfx(z_q12));
    head_half = hb3_v3_make(head_radius, head_half_y, head_radius);
    return ml3_actor_add_hurt_obb(&melee->collision, actor_id, 2,
        head_center, head_half, basis, HB3_HURT_FLESH, 1) == ML3_OK;
}

int blank3d_katana_melee_trigger(Blank3DKatanaMelee *melee)
{
    int active_frames;
    int recovery_frames;
    if (!melee || !melee->initialized || !melee->enabled ||
        !melee->root_valid) return 0;
    if (melee->attacking) return 0;
    /* active_end_frame is exclusive: [start, end). */
    active_frames = (int)melee->config.active_end_frame -
                    (int)melee->config.active_start_frame;
    recovery_frames = (int)melee->config.duration_frames -
                      (int)melee->config.active_end_frame;
    if (recovery_frames < 0) recovery_frames = 0;
    {
        int flags;
        flags = ML3_FLAG_NO_SELF_HIT | ML3_FLAG_NO_TEAM_HIT |
                ML3_FLAG_USE_GROUPS;
        if (melee->config.one_hit_per_enemy)
            flags |= ML3_FLAG_ONE_HIT_ONLY;
        if (ml3_attack_begin(&melee->collision, B3D_KATANA_PLAYER_ID,
                B3D_KATANA_ATTACK_ID,
                melee->config.active_start_frame, active_frames,
                recovery_frames, melee->config.damage,
                melee->config.stun_frames, melee->config.hitstop_frames,
                flags, &melee->attack_handle) != ML3_OK) return 0;
        (void)ml3_attack_set_masks(&melee->collision,
                                   melee->attack_handle,
                                   1, HB3_ALL_MASK);
    }
    melee->collision_window = 0;
    melee->attacking = 1;
    melee->swing_serial += 1U;
    if (nm89_trigger_action(&melee->rig,
                            B3D_KATANA_ACTION_SLASH) != NM89_OK) {
        (void)ml3_attack_end(&melee->collision, melee->attack_handle);
        melee->attack_handle = -1;
        melee->attacking = 0;
        return 0;
    }
    return 1;
}

static hb3_fx b3d_katana_q16_axis_to_hitfx(nm89_fx value)
{
    /* NationalMecanicanimal89 uses Q16; PDC3D uses Q4. */
    return (hb3_fx)(value / 4096);
}

static hb3_fx b3d_katana_cm_to_hitfx_positive(int centimetres)
{
    hb3_fx value;
    if (centimetres < 0) centimetres = -centimetres;
    value = (hb3_fx)((centimetres * HB3_FX_ONE) / 100);
    if (value < 1) value = 1;
    return value;
}

static void b3d_katana_update_hit_box(Blank3DKatanaMelee *melee)
{
    const nm89_matrix *world;
    int blade_length_cm;
    int blade_mid_cm;
    nm89_fx center_x;
    nm89_fx center_y;
    nm89_fx center_z;
    hb3_v3 center;
    hb3_v3 half_extents;
    hb3_mat3 basis;
    if (!melee || melee->attack_handle < 0) return;
    if (!melee->collision_window ||
        !ml3_attack_is_active_window(&melee->collision,
                                     melee->attack_handle)) {
        (void)ml3_attack_clear_hitboxes(&melee->collision,
                                        melee->attack_handle);
        return;
    }
    world = nm89_part_get_world(&melee->rig, melee->blade_part);
    if (!world) return;

    blade_length_cm = melee->config.blade_tip_cm -
                      melee->config.blade_base_cm;
    if (blade_length_cm < 1) blade_length_cm = 1;
    blade_mid_cm = melee->config.blade_base_cm + blade_length_cm / 2;
    nm89_matrix_transform_point(world, 0, 0,
        b3d_katana_meters_q16_from_cm(blade_mid_cm),
        &center_x, &center_y, &center_z);
    center = hb3_v3_make((hb3_fx)(center_x / 4096),
                         (hb3_fx)(center_y / 4096),
                         (hb3_fx)(center_z / 4096));

    /* The katana mesh grows along local +Z.  These are true oriented-box
       half extents, not a capsule radius or a debug-only approximation. */
    half_extents = hb3_v3_make(
        b3d_katana_cm_to_hitfx_positive(
            melee->config.damage_box_half_width_cm),
        b3d_katana_cm_to_hitfx_positive(
            melee->config.damage_box_half_thickness_cm),
        b3d_katana_cm_to_hitfx_positive(blade_length_cm / 2));

    /* Matrix columns are the world-space local axes. */
    basis = hb3_mat3_from_axes(
        hb3_v3_make(b3d_katana_q16_axis_to_hitfx(world->m[0][0]),
                    b3d_katana_q16_axis_to_hitfx(world->m[1][0]),
                    b3d_katana_q16_axis_to_hitfx(world->m[2][0])),
        hb3_v3_make(b3d_katana_q16_axis_to_hitfx(world->m[0][1]),
                    b3d_katana_q16_axis_to_hitfx(world->m[1][1]),
                    b3d_katana_q16_axis_to_hitfx(world->m[2][1])),
        hb3_v3_make(b3d_katana_q16_axis_to_hitfx(world->m[0][2]),
                    b3d_katana_q16_axis_to_hitfx(world->m[1][2]),
                    b3d_katana_q16_axis_to_hitfx(world->m[2][2])));

    (void)ml3_attack_set_hit_obb(&melee->collision,
        melee->attack_handle, 0, 1, center, half_extents, basis,
        HIT3_HIT_SLASH | HIT3_HIT_PARRYABLE, 0);
}

static void b3d_katana_tick(Blank3DKatanaMelee *melee)
{
    int phase;
    melee->packet_valid = 0;
    (void)nm89_step(&melee->rig, 1U, 1U);
    if (melee->attacking && melee->attack_handle >= 0)
        b3d_katana_update_hit_box(melee);
    ml3_tick(&melee->collision);
    b3d_katana_drain_events(melee);
    if (melee->attacking && melee->attack_handle >= 0) {
        phase = ml3_attack_get_phase(&melee->collision,
                                     melee->attack_handle);
        if (phase == ML3_ATTACK_DONE || !melee->rig.player.playing) {
            (void)ml3_attack_end(&melee->collision,
                                   melee->attack_handle);
            melee->attack_handle = -1;
            melee->attacking = 0;
            melee->collision_window = 0;
        }
    }
}

int blank3d_katana_melee_update(Blank3DKatanaMelee *melee,
                                unsigned short frame_ms)
{
    unsigned int total;
    unsigned int ticks;
    unsigned int i;
    if (!melee || !melee->initialized || !melee->enabled) return 0;
    total = melee->tick_remainder_ms + (unsigned int)frame_ms;
    ticks = total / melee->config.tick_ms;
    melee->tick_remainder_ms = total % melee->config.tick_ms;
    if (ticks > 8U) ticks = 8U;
    if (ticks == 0U) {
        melee->packet_valid = 0;
        (void)nm89_step(&melee->rig, 0U, 1U);
        return 1;
    }
    for (i = 0U; i < ticks; ++i) b3d_katana_tick(melee);
    return 1;
}

int blank3d_katana_melee_poll_hit(Blank3DKatanaMelee *melee,
                                  Blank3DKatanaHit *out_hit)
{
    if (!melee || !out_hit || melee->hit_count <= 0) return 0;
    *out_hit = melee->hits[melee->hit_head];
    melee->hit_head += 1;
    if (melee->hit_head >= B3D_KATANA_MAX_HITS) melee->hit_head = 0;
    melee->hit_count -= 1;
    return 1;
}

const nm89_geometry_packet *blank3d_katana_melee_packet(
    const Blank3DKatanaMelee *melee)
{
    if (!melee || !melee->initialized || !melee->packet_valid) return 0;
    return &melee->packet;
}

int blank3d_katana_melee_is_attacking(const Blank3DKatanaMelee *melee)
{
    return melee && melee->initialized && melee->attacking;
}

void blank3d_katana_melee_debug_draw(const Blank3DKatanaMelee *melee,
                                     hb3_debug_line_fn line_fn,
                                     void *user)
{
    if (!melee || !melee->initialized || !melee->config.debug_draw ||
        !line_fn) return;
    ml3_debug_draw_world_lines(&melee->collision, line_fn, user,
        ML3_DEBUG_HURTBOXES | ML3_DEBUG_HITBOXES);
}
