/*
 * Monika Blank3D Runner - Gamlib3D + Soquete3D + Giffany Shapes3D edition.
 *
 * Core simulation:
 *   - C89
 *   - fixed-point only (Gamlib3D Q20.12, Soquete/Shapes Q16.16)
 *   - no malloc/realloc/free
 *   - no math.h
 *
 * OpenGL receives converted values only inside engine_bridge.c.
 */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <gl/gl.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#include "../vendor/gamlib3d/gamlib3d_camera.h"
#include "../vendor/gamlib3d/math_helpers/gamlib3d_matrix.h"
#include "../vendor/soquete3d/soquete3d.h"
#include "../vendor/giffany_shapes3d/g3d_shapes.h"
#include "engine_bridge.h"
#include "blank3d_systems.h"
#include "blank3d_audio.h"
#include "blank3d_config.h"
#include "blank3d_hud.h"
#include "blank3d_numbar.h"
#include "blank3d_collision.h"
#include "blank3d_projectile_mesh.h"
#include "blank3d_weapon_modules.h"
#include "blank3d_ballistics.h"
#include "blank3d_universal_aim.h"
#include "blank3d_bolt.h"
#include "blank3d_trails.h"
#include "blank3d_sniper.h"
#include "blank3d_cameranaku.h"
#include "blank3d_fire_frame_sync.h"
#include "blank3d_shotgun.h"
#include "blank3d_languages.h"
#include "blank3d_objects.h"
#include "blank3d_npc_inventory.h"
#include "blank3d_vertical_axis.h"
#include "blank3d_input.h"
#include "blank3d_list_cycle.h"
#include "blank3d_automotion.h"
#include "blank3d_motion_attack.h"
#include "blank3d_truth_gate.h"
#include "blank3d_perception.h"
#include "blank3d_perception_ini.h"
#include "blank3d_attachment.h"
#include "blank3d_mechanical_weapon.h"
#include "blank3d_weapon_presentation.h"
#include "blank3d_actor_equipment.h"
#include "blank3d_vphysics.h"
#include "blank3d_casing_physics.h"
#include "blank3d_faction.h"

#define MAX_BULLETS 128
#define MAX_ENEMIES 32
#define MAX_CASINGS 128
#define PROJECTILE_MESH_COUNT B3D_PROJECTILE_MESH_COUNT
#define CASING_MESH_COUNT B3D_CASING_MESH_COUNT
#define B3D_TARGET_NONE GFA_INVALID_ID
#define B3D_TARGET_DISTANCE_WEIGHT 1
#define B3D_TARGET_STICKINESS_BONUS 3
#define GATLING_WEAPON_ID 8
#define SLINGSHOT_WEAPON_ID 9
#define GATLING_SPINUP_MS 360U
#define MAX_LINE 256
#define SCRIPT_RPY   "scripts/startup.rpy"
#define SCRIPT_DDSL2 "scripts/player.ddsl2"
#define SCRIPT_FPI   "scripts/enemy.fpi"
#define CONFIG_FILE  "config/blank3d.toml"
#define CAMERA_TPS 0
#define CAMERA_FPS 1

/* The provider demo is diagnostic content, not part of the normal scene.
 * Production and ordinary debug builds keep it disabled.  An explicit
 * physics-demo build may opt in with -DB3D_VPHYSICS_DEMO=1. */
#ifndef B3D_VPHYSICS_DEMO
#define B3D_VPHYSICS_DEMO 0
#endif
#ifndef B3D_VPHYSICS_DEBUG_DRAW
#define B3D_VPHYSICS_DEBUG_DRAW B3D_VPHYSICS_DEMO
#endif

#define B3D_DDSL_ACT_FORWARD      (1UL << 0)
#define B3D_DDSL_ACT_BACK         (1UL << 1)
#define B3D_DDSL_ACT_LEFT         (1UL << 2)
#define B3D_DDSL_ACT_RIGHT        (1UL << 3)
#define B3D_DDSL_ACT_UP           (1UL << 4)
#define B3D_DDSL_ACT_DOWN         (1UL << 5)
#define B3D_DDSL_ACT_JUMP         (1UL << 6)
#define B3D_DDSL_ACT_TURN_LEFT    (1UL << 7)
#define B3D_DDSL_ACT_TURN_RIGHT   (1UL << 8)
#define B3D_DDSL_ACT_SHOOT        (1UL << 9)

#define PLAYER_BOX_VERTICES 32
#define PLAYER_BOX_INDICES  64
#define WEAPON_BOX_VERTICES 32
#define WEAPON_BOX_INDICES  64
#define ENEMY_BODY_VERTICES 192
#define ENEMY_BODY_INDICES  512
#define ENEMY_HEAD_VERTICES 32
#define ENEMY_HEAD_INDICES  64
#define BULLET_VERTICES     B3D_PROJECTILE_VERTEX_CAPACITY
#define BULLET_INDICES      B3D_PROJECTILE_INDEX_CAPACITY
#define CASING_VERTICES      B3D_CASING_VERTEX_CAPACITY
#define CASING_INDICES       B3D_CASING_INDEX_CAPACITY

typedef struct BulletTag {
    int alive;
    int weapon_id;
    int owner_actor_id;
    int owner_team_id;
    int projectile_id;
    int projectile_mesh_id;
    int trail_id;
    int aoi_trail_id;
    int physics_backend;
    int bolt_projectile_id;
    int damage;
    g3d_fix radius;
    g3d_fix mesh_scale;
    g3d_fix gravity;
    Transform transform;
    Vec3 previous_position;
    Vec3 velocity;
    g3d_fix life;
    gv89_u32 audio_primary_key;
    gv89_u32 audio_secondary_key;
} Bullet;

typedef struct CasingTag {
    int alive;
    int weapon_id;
    int casing_mesh_id;
    int physics_active;
    g3d_fix mesh_scale;
    Transform transform; /* fallback-only pose when VPhysics is unavailable */
    Vec3 velocity;       /* fallback-only velocity */
    g3d_fix life;
} Casing;

typedef struct EnemyTag {
    int alive;
    int state;
    char archetype[48];
    Transform transform;
    int hp;
    g3d_fix cooldown;
    int weapon_actor_id;
    int weapon_ready;
    char weapon_loadout[160];
    char requested_weapon[64];
    Blank3DVerticalBody vertical;
    Blank3DAutomotion automotion;
    Blank3DMotionAttack motion_attack;
    Blank3DTruthGate truth_gate;
    g3d_fix truth_range;
    Blank3DPerceptionConfig perception_config;
    char perception_ini[160];
    int perception_index;
    int target_actor_id;
    int last_damage_source_actor_id;
    g3d_fix damage_interest_time;
    int faction_is_ally;
    char faction_name[32];
    char team_name[32];
    char role_name[32];
    char faction_tags[96];
} Enemy;

typedef struct FileStampTag {
    FILETIME time;
    int ok;
} FileStamp;

typedef struct MeshCatalogTag {
    g3d_mesh player_body;
    g3d_mesh weapon;
    g3d_mesh mechanical_weapon[B3D_MECH89_MESH_COUNT];
    g3d_mesh enemy_body;
    g3d_mesh enemy_head;
    g3d_mesh ally_body;
    g3d_mesh ally_head;
    g3d_mesh projectile[PROJECTILE_MESH_COUNT];
    g3d_mesh casing[CASING_MESH_COUNT];

    g3d_vertex player_body_vertices[PLAYER_BOX_VERTICES];
    g3d_index player_body_indices[PLAYER_BOX_INDICES];
    g3d_vertex weapon_vertices[WEAPON_BOX_VERTICES];
    g3d_index weapon_indices[WEAPON_BOX_INDICES];
    g3d_vertex mechanical_weapon_vertices[B3D_MECH89_MESH_COUNT][WEAPON_BOX_VERTICES];
    g3d_index mechanical_weapon_indices[B3D_MECH89_MESH_COUNT][WEAPON_BOX_INDICES];
    g3d_vertex enemy_body_vertices[ENEMY_BODY_VERTICES];
    g3d_index enemy_body_indices[ENEMY_BODY_INDICES];
    g3d_vertex enemy_head_vertices[ENEMY_HEAD_VERTICES];
    g3d_index enemy_head_indices[ENEMY_HEAD_INDICES];
    g3d_vertex ally_body_vertices[ENEMY_BODY_VERTICES];
    g3d_index ally_body_indices[ENEMY_BODY_INDICES];
    g3d_vertex ally_head_vertices[ENEMY_HEAD_VERTICES];
    g3d_index ally_head_indices[ENEMY_HEAD_INDICES];
    g3d_vertex projectile_vertices[PROJECTILE_MESH_COUNT][BULLET_VERTICES];
    g3d_index projectile_indices[PROJECTILE_MESH_COUNT][BULLET_INDICES];
    g3d_vertex casing_vertices[CASING_MESH_COUNT][CASING_VERTICES];
    g3d_index casing_indices[CASING_MESH_COUNT][CASING_INDICES];
} MeshCatalog;

typedef struct EngineTag {
    HINSTANCE instance;
    HWND window;
    HDC device;
    HGLRC gl_context;
    int running;
    int active;
    int width;
    int height;
    int keys[256];
    int key_pressed[256];
    int key_released[256];
    int mouse_left;
    int mouse_right;
    int mouse_left_pressed;
    int mouse_left_released;
    int mouse_wheel_delta;
    Blank3DInput input;
    Blank3DListCycleRegistry list_cycles;
    unsigned long ddsl_action_mask;
    int lock_mouse;
    int mouse_captured;
    POINT last_mouse;
    unsigned short frame_ms;

    g3d_fix dt;
    g3d_fix time;
    Transform player;
    int player_hp;
    int player_down_latched;
    int player_damage_flash_ms;
    g3d_fix move_speed;
    g3d_fix strafe_speed;
    g3d_fix vertical_speed;
    Blank3DVerticalAxis vertical_axis;
    Blank3DVerticalBody player_vertical;
    g3d_fix turn_speed;
    g3d_fix bullet_timer;
    g3d_fix camera_dist;
    g3d_fix camera_height;
    g3d_fix camera_shoulder;
    g3d_fix camera_eye_height;
    g3d_fix camera_yaw;
    g3d_fix camera_pitch;
    g3d_fix camera_pitch_min;
    g3d_fix camera_pitch_max;
    g3d_fix mouse_sensitivity;
    int camera_mode;
    int fire_requested;
    int muzzle_flash_ms;
    unsigned int gatling_spin_ms;
    unsigned int gatling_spinup_ms;
    int gatling_armed;
    unsigned int slingshot_charge_ms;
    unsigned int slingshot_charge_max_ms;
    int slingshot_charging;
    g3d_fix sniper_sway_yaw;
    g3d_fix sniper_sway_pitch;
    g3d_fix fov;
    g3d_fix base_fov;
    g3d_fix zoom_fov;
    g3d_fix sniper_zoom_fov;
    g3d_fix zoom_speed;
    g3d_fix zoom_fx;
    g3d_fix near_z;
    g3d_fix far_z;
    int show_grid;

    Camera camera;
    Bullet bullets[MAX_BULLETS];
    Casing casings[MAX_CASINGS];
    Enemy enemies[MAX_ENEMIES];
    int enemy_count;

    soq3d_context locator;
    soq3d_stamp frame_stamp;
    soq3d_key player_key;
    soq3d_key player_body_socket;
    soq3d_key player_weapon_socket;
    soq3d_key player_muzzle_socket;
    soq3d_key player_ballistic_muzzle_socket;
    soq3d_key camera_socket;
    soq3d_key aim_socket;
    soq3d_key enemy_keys[MAX_ENEMIES];
    soq3d_key enemy_body_sockets[MAX_ENEMIES];
    soq3d_key enemy_head_sockets[MAX_ENEMIES];
    soq3d_key enemy_weapon_sockets[MAX_ENEMIES];
    soq3d_key enemy_muzzle_sockets[MAX_ENEMIES];
    soq3d_key bullet_keys[MAX_BULLETS];

    MeshCatalog meshes;
    Blank3DAttachmentWorld attachments;
    Blank3DWeaponPresentationRegistry weapon_presentations;
    Blank3DActorEquipmentSystem actor_equipment;
    Blank3DSystems systems;
    GWP89_Manager npc_weapons;
    Blank3DNpcInventoryBank npc_inventory;
    Blank3DAudio audio;
    Blank3DConfig config;
    Blank3DHud hud;
    Blank3DNumbarSystem numbars;
    Blank3DCollision collision;
    Blank3DVPhysics physics;
    Blank3DCasingPhysics casing_physics;
    Blank3DFactionSystem factions;
    Blank3DPerceptionWorld perception;
    Blank3DBolt bolt;
    Blank3DTrails trails;
    Blank3DSniper sniper;
    Blank3DCameraNaku cameranaku;
    Blank3DFireFrameSync fire_frame_sync;
    Blank3DObjects objects;
    FileStamp rpy_stamp;
    FileStamp ddsl2_stamp;
    FileStamp fpi_stamp;
    char status[256];
} Engine;

static Engine g;

static void apply_camera_config(void);
static int b3d_weapon_id_from_text(const char *weapon_text);
static int b3d_enemy_ensure_weapon_inventory(Enemy *enemy);
static int b3d_enemy_equip_weapon_text(Enemy *enemy, const char *weapon_text);
static int b3d_enemy_give_weapon_text(Enemy *enemy, const char *weapon_text);
static void sync_collision_world(void);
static int current_clip_capacity(void);
static int b3d_actor_current_weapon_id(GWP89_Manager *manager,
                                        int actor_id);

static g3d_fix fix_ratio(long numerator, long denominator)
{
    if (denominator == 0L) return 0;
    return g3d_fix_div((g3d_fix)(numerator * G3D_FIX_ONE),
                       (g3d_fix)(denominator * G3D_FIX_ONE));
}


static g3d_fix q16_to_q12(long value)
{
    return (g3d_fix)(value / 16L);
}

static long q12_to_q16_long(g3d_fix value)
{
    return (long)value * 16L;
}

static void b3d_damage_player(int amount)
{
    if (amount <= 0) return;
    blank3d_systems_damage_player(&g.systems, amount);
    g.player_hp = blank3d_systems_player_health(&g.systems);
    g.player_damage_flash_ms = 520;
}

static Enemy *b3d_actor_enemy(int actor_id)
{
    int i;
    for (i = 0; i < g.enemy_count; ++i)
        if (g.enemies[i].weapon_actor_id == actor_id) return &g.enemies[i];
    return 0;
}

static const Enemy *b3d_actor_enemy_const(int actor_id)
{
    int i;
    for (i = 0; i < g.enemy_count; ++i)
        if (g.enemies[i].weapon_actor_id == actor_id) return &g.enemies[i];
    return 0;
}

static int b3d_actor_alive(int actor_id)
{
    const Enemy *enemy;
    if (actor_id == B3D_PLAYER_ACTOR_ID) return g.player_hp > 0;
    enemy = b3d_actor_enemy_const(actor_id);
    return enemy && enemy->alive;
}

static int b3d_actor_position(int actor_id, Vec3 *out_position)
{
    const Enemy *enemy;
    if (!out_position) return 0;
    if (actor_id == B3D_PLAYER_ACTOR_ID) {
        *out_position = g.player.position;
        return 1;
    }
    enemy = b3d_actor_enemy_const(actor_id);
    if (!enemy || !enemy->alive) return 0;
    *out_position = enemy->transform.position;
    return 1;
}

static soq3d_key b3d_actor_locator_key(int actor_id)
{
    int i;
    if (actor_id == B3D_PLAYER_ACTOR_ID) return g.player_key;
    for (i = 0; i < g.enemy_count; ++i)
        if (g.enemies[i].weapon_actor_id == actor_id) return g.enemy_keys[i];
    return (soq3d_key)0;
}

static int b3d_q12_to_gfa_q16(g3d_fix value)
{
    if (value > (g3d_fix)134217727L) return 2147483632;
    if (value < (g3d_fix)(-134217728L)) return (-2147483647 - 1);
    return (int)(value * 16L);
}

static int b3d_faction_context_between(int source_actor, int target_actor,
                                       GFA_Context *context)
{
    Vec3 source;
    Vec3 target;
    Vec3 delta;
    g3d_fix distance;
    const GFA_Entity *target_entity;
    if (!context) return 0;
    memset(context, 0, sizeof(*context));
    if (!b3d_actor_position(source_actor, &source) ||
        !b3d_actor_position(target_actor, &target)) return 0;
    gamlib_vec3_sub(&delta, &target, &source);
    distance = gamlib_vec3_length(&delta);
    context->stimulus = GFA_STIM_NONE;
    context->distance_fp = b3d_q12_to_gfa_q16(distance);
    target_entity = gfa_get_entity_const(&g.factions.world, target_actor);
    if (target_entity) {
        context->target_threat = target_entity->threat;
        context->target_morale = target_entity->morale;
    }
    return 1;
}

static int b3d_faction_can_attack_actor(int source_actor, int target_actor)
{
    GFA_Context context;
    if (!g.factions.initialized || source_actor == target_actor ||
        !b3d_actor_alive(target_actor)) return 0;
    if (!b3d_faction_context_between(source_actor, target_actor, &context))
        memset(&context, 0, sizeof(context));
    return blank3d_faction_can_attack(&g.factions, source_actor,
                                      target_actor, &context);
}

static int b3d_enemy_target_alive(const Enemy *enemy)
{
    return enemy && enemy->target_actor_id != B3D_TARGET_NONE &&
           b3d_actor_alive(enemy->target_actor_id);
}

static void b3d_damage_actor(int source_actor, int target_actor, int amount)
{
    Enemy *enemy;
    GFA_Event event_data;
    if (amount <= 0 || !b3d_faction_can_attack_actor(source_actor,
                                                       target_actor)) return;
    if (target_actor == B3D_PLAYER_ACTOR_ID) {
        b3d_damage_player(amount);
    } else {
        enemy = b3d_actor_enemy(target_actor);
        if (!enemy || !enemy->alive) return;
        enemy->last_damage_source_actor_id = source_actor;
        enemy->damage_interest_time = G3D_FIX_FROM_INT(4);
        enemy->hp -= amount;
        if (enemy->hp <= 0) enemy->alive = 0;
    }
    memset(&event_data, 0, sizeof(event_data));
    event_data.event_type = b3d_actor_alive(target_actor)
                          ? GFA_EVENT_DAMAGE : GFA_EVENT_DEATH;
    event_data.source_entity = source_actor;
    event_data.target_entity = target_actor;
    event_data.amount = amount;
    (void)gfa_fire_event(&g.factions.world, &event_data);
}

static int b3d_player_threat_level(void)
{
    int i;
    int threat;
    int local_threat;
    int distance_units;
    Vec3 delta;
    g3d_fix distance;

    threat = 0;
    for (i = 0; i < g.enemy_count; ++i) {
        if (!g.enemies[i].alive ||
            !b3d_faction_can_attack_actor(g.enemies[i].weapon_actor_id,
                                           B3D_PLAYER_ACTOR_ID)) continue;
        gamlib_vec3_sub(&delta, &g.enemies[i].transform.position,
                        &g.player.position);
        delta.y = 0;
        distance = gamlib_vec3_length(&delta);
        if (distance >= G3D_FIX_FROM_INT(25)) continue;
        distance_units = (int)(distance / G3D_FIX_ONE);
        local_threat = 100 - distance_units * 4;
        if (blank3d_motion_attack_mode(&g.enemies[i].motion_attack) !=
            B3D_MOTION_ATTACK_NONE)
            local_threat += 16;
        if (local_threat > threat) threat = local_threat;
    }

    for (i = 0; i < MAX_BULLETS; ++i) {
        if (!g.bullets[i].alive ||
            !b3d_faction_can_attack_actor(g.bullets[i].owner_actor_id,
                                           B3D_PLAYER_ACTOR_ID)) continue;
        gamlib_vec3_sub(&delta, &g.bullets[i].transform.position,
                        &g.player.position);
        distance = gamlib_vec3_length(&delta);
        if (distance >= G3D_FIX_FROM_INT(16)) continue;
        distance_units = (int)(distance / G3D_FIX_ONE);
        local_threat = 100 - distance_units * 6;
        if (local_threat > threat) threat = local_threat;
    }

    if (g.player_damage_flash_ms > 0 && threat < 100) threat = 100;
    if (threat < 0) threat = 0;
    if (threat > 100) threat = 100;
    return threat;
}

static int b3d_enemy_truth_has_line_of_sight(const Enemy *enemy)
{
    Vec3 origin;
    Vec3 target;
    Vec3 delta;
    Vec3 direction;
    g3d_fix range;
    GWP89_Vec3 ray_origin;
    GWP89_Vec3 ray_direction;
    Blank3DCollisionHit hit;

    if (!enemy) return 0;
    origin = enemy->transform.position;
    if (!b3d_actor_position(enemy->target_actor_id, &target)) return 0;
    origin.y = g3d_fix_add_sat(origin.y, G3D_FIX_ONE);
    target.y = g3d_fix_add_sat(target.y, G3D_FIX_ONE);
    gamlib_vec3_sub(&delta, &target, &origin);
    range = gamlib_vec3_length(&delta);
    if (range <= G3D_FIX_EPSILON) return 1;
    gamlib_vec3_normalize(&direction, &delta);

    ray_origin.x = origin.x;
    ray_origin.y = origin.y;
    ray_origin.z = origin.z;
    ray_direction.x = direction.x;
    ray_direction.y = direction.y;
    ray_direction.z = direction.z;

    if (!blank3d_collision_raycast(&g.collision,
            &ray_origin, &ray_direction, range,
            B3D_COLLISION_LAYER_WORLD, &hit))
        return 1;
    return hit.distance_fx >= g3d_fix_sub_sat(range, fix_ratio(1, 10));
}

static int b3d_enemy_index(const Enemy *enemy)
{
    long index;
    if (!enemy) return -1;
    index = (long)(enemy - &g.enemies[0]);
    if (index < 0L || index >= (long)MAX_ENEMIES) return -1;
    return (int)index;
}

static Blank3DPerceptionAgent *b3d_enemy_perception(Enemy *enemy)
{
    int index;
    index = b3d_enemy_index(enemy);
    if (index < 0) return 0;
    return blank3d_perception_agent(&g.perception, index);
}

static const Blank3DPerceptionAgent *b3d_enemy_perception_const(
    const Enemy *enemy)
{
    int index;
    index = b3d_enemy_index(enemy);
    if (index < 0) return 0;
    return blank3d_perception_agent_const(&g.perception, index);
}

static EAI_EntityId b3d_actor_eai_id(int actor_id)
{
    int i;
    const Blank3DPerceptionAgent *agent;
    if (actor_id == B3D_PLAYER_ACTOR_ID)
        return g.perception.player_entity_id;
    for (i = 0; i < g.enemy_count; ++i) {
        if (g.enemies[i].weapon_actor_id != actor_id) continue;
        agent = blank3d_perception_agent_const(&g.perception, i);
        return agent ? agent->eai_self_id : EAI_INVALID_ID;
    }
    return EAI_INVALID_ID;
}

static void b3d_faction_update_targets(void)
{
    int i;
    int j;
    int count;
    int target;
    Blank3DFactionCandidate candidates[MAX_ENEMIES + 1];
    Blank3DPerceptionAgent *agent;

    if (!g.factions.initialized) return;
    (void)blank3d_faction_sync_actor(&g.factions, B3D_PLAYER_ACTOR_ID,
        85, 100, g.player_hp > 0, g.player_hp > 0);
    for (i = 0; i < g.enemy_count; ++i) {
        if (g.enemies[i].damage_interest_time > 0) {
            g.enemies[i].damage_interest_time = g3d_fix_sub_sat(
                g.enemies[i].damage_interest_time, g.dt);
            if (g.enemies[i].damage_interest_time <= 0)
                g.enemies[i].last_damage_source_actor_id =
                    B3D_TARGET_NONE;
        }
        (void)blank3d_faction_sync_actor(&g.factions,
            g.enemies[i].weapon_actor_id,
            g.enemies[i].faction_is_ally ? 70 : 65,
            g.enemies[i].faction_is_ally ? 90 : 55,
            g.enemies[i].alive, g.enemies[i].alive);
        if (!g.enemies[i].alive)
            g.enemies[i].target_actor_id = B3D_TARGET_NONE;
    }
    for (i = 0; i < g.enemy_count; ++i) {
        if (!g.enemies[i].alive) continue;
        count = 0;
        if (g.player_hp > 0) {
            candidates[count].entity_id = B3D_PLAYER_ACTOR_ID;
            (void)b3d_faction_context_between(
                g.enemies[i].weapon_actor_id, B3D_PLAYER_ACTOR_ID,
                &candidates[count].context);
            agent = b3d_enemy_perception(&g.enemies[i]);
            if (g.enemies[i].target_actor_id == B3D_PLAYER_ACTOR_ID &&
                agent) {
                candidates[count].context.visible =
                    agent->target_visible;
                candidates[count].context.heard = agent->target_heard;
            }
            if (g.enemies[i].last_damage_source_actor_id ==
                B3D_PLAYER_ACTOR_ID &&
                g.enemies[i].damage_interest_time > 0)
                candidates[count].context.recent_damage = 1;
            ++count;
        }
        for (j = 0; j < g.enemy_count; ++j) {
            if (j == i || !g.enemies[j].alive) continue;
            candidates[count].entity_id = g.enemies[j].weapon_actor_id;
            (void)b3d_faction_context_between(
                g.enemies[i].weapon_actor_id,
                g.enemies[j].weapon_actor_id,
                &candidates[count].context);
            agent = b3d_enemy_perception(&g.enemies[i]);
            if (g.enemies[i].target_actor_id ==
                g.enemies[j].weapon_actor_id && agent) {
                candidates[count].context.visible =
                    agent->target_visible;
                candidates[count].context.heard = agent->target_heard;
            }
            if (g.enemies[i].last_damage_source_actor_id ==
                g.enemies[j].weapon_actor_id &&
                g.enemies[i].damage_interest_time > 0)
                candidates[count].context.recent_damage = 1;
            ++count;
        }
        target = blank3d_faction_choose_attack_target_stable(&g.factions,
            g.enemies[i].weapon_actor_id, candidates, count,
            g.enemies[i].target_actor_id,
            B3D_TARGET_DISTANCE_WEIGHT,
            B3D_TARGET_STICKINESS_BONUS, 0);
        g.enemies[i].target_actor_id = target;
        agent = b3d_enemy_perception(&g.enemies[i]);
        if (agent) {
            if (target != B3D_TARGET_NONE)
                blank3d_perception_set_target_key(agent,
                    b3d_actor_locator_key(target), b3d_actor_eai_id(target));
            else
                blank3d_perception_set_target_key(
                    agent, (soq3d_key)0, EAI_INVALID_ID);
        }
    }
}

static int b3d_enemy_target_position(const Enemy *enemy,
                                     Vec3 *out_position)
{
    const Blank3DPerceptionAgent *agent;
    if (!enemy || !out_position) return 0;
    agent = b3d_enemy_perception_const(enemy);
    if (agent && agent->target_known) {
        *out_position = agent->best_target_position;
        return 1;
    }
    return b3d_actor_position(enemy->target_actor_id, out_position);
}

static int b3d_perception_raycast(void *user,
                                  const Vec3 *from,
                                  const Vec3 *to,
                                  unsigned long block_mask)
{
    Blank3DCollision *collision;
    Vec3 delta;
    Vec3 direction;
    g3d_fix range;
    GWP89_Vec3 origin;
    GWP89_Vec3 ray_direction;
    Blank3DCollisionHit hit;
    unsigned long layers;
    collision = (Blank3DCollision *)user;
    if (!collision || !from || !to) return 0;
    gamlib_vec3_sub(&delta, to, from);
    range = gamlib_vec3_length(&delta);
    if (range <= G3D_FIX_EPSILON) return 0;
    gamlib_vec3_normalize(&direction, &delta);
    origin.x = from->x;
    origin.y = from->y;
    origin.z = from->z;
    ray_direction.x = direction.x;
    ray_direction.y = direction.y;
    ray_direction.z = direction.z;
    layers = block_mask != 0UL ? B3D_COLLISION_LAYER_WORLD : 0UL;
    if (layers == 0UL) return 0;
    if (!blank3d_collision_raycast(collision, &origin, &ray_direction,
                                   range, layers, &hit)) return 0;
    return hit.distance_fx < g3d_fix_sub_sat(range, fix_ratio(1, 10));
}

static void b3d_perception_reset_agents(void)
{
    int i;
    Blank3DPerceptionAgent *agent;
    if (!g.perception.initialized) return;
    for (i = 0; i < MAX_ENEMIES; ++i) {
        blank3d_perception_agent_disable(&g.perception, i);
        g.enemies[i].perception_index = i;
        if (!g.enemies[i].alive) continue;
        if (!blank3d_perception_agent_init(&g.perception, i,
                g.enemy_keys[i], g.player_key,
                (unsigned char)(g.enemies[i].faction_is_ally ? 1u : 2u),
                1u)) continue;
        agent = blank3d_perception_agent(&g.perception, i);
        if (!agent) continue;
        agent->config = g.enemies[i].perception_config;
    }
}

static void b3d_perception_tick(void)
{
    int i;
    blank3d_perception_sync_player(&g.perception, &g.locator,
                                   g.frame_stamp, g.player_key,
                                   g.player_hp > 0);
    for (i = 0; i < g.enemy_count; ++i) {
        blank3d_perception_sync_agent(&g.perception, i, &g.locator,
                                      g.frame_stamp,
                                      g.enemies[i].alive,
                                      b3d_enemy_target_alive(&g.enemies[i]));
    }
    blank3d_perception_update(&g.perception, g.dt);
}

static void b3d_enemy_truth_facts(const Enemy *enemy,
                                  Blank3DTruthFacts *facts)
{
    const Blank3DPerceptionAgent *agent;
    if (!facts) return;
    memset(facts, 0, sizeof(*facts));
    if (!enemy) return;
    agent = b3d_enemy_perception_const(enemy);
    facts->entity_alive = enemy->alive != 0;
    facts->target_alive = b3d_enemy_target_alive(enemy);
    facts->socketer_has_target = agent ? agent->target_known : 1;
    facts->within_range = agent
        ? (enemy->truth_range <= 0 ||
           agent->target_distance <= enemy->truth_range)
        : 1;
    facts->line_of_sight = agent
        ? (agent->target_visible || agent->target_partial)
        : b3d_enemy_truth_has_line_of_sight(enemy);
    facts->eyes_visible = agent ? agent->target_visible : 0;
    facts->eyes_partial = agent ? agent->target_partial : 0;
    facts->enlightener_interpreted = agent
        ? (agent->target_inferred || agent->target_remembered ||
           agent->target_heard || agent->target_visible ||
           agent->target_partial)
        : 0;
    facts->target_heard = agent ? agent->target_heard : 0;
    facts->target_remembered = agent ? agent->target_remembered : 0;
}

static int blank3d_sniper_raycast(void *user,
                                   const g89_vec3 *from,
                                   const g89_vec3 *dir,
                                   g89_fx max_dist,
                                   gaq89_hit *out_hit)
{
    Blank3DCollision *collision;
    GWP89_Vec3 origin_q12;
    GWP89_Vec3 direction_q12;
    Blank3DCollisionHit hit;
    collision = (Blank3DCollision *)user;
    if (!collision || !from || !dir || !out_hit) return 0;
    memset(out_hit, 0, sizeof(*out_hit));
    origin_q12.x = (gwp89_fx)(from->x / 16L);
    origin_q12.y = (gwp89_fx)(from->y / 16L);
    origin_q12.z = (gwp89_fx)(from->z / 16L);
    direction_q12.x = (gwp89_fx)(dir->x / 16L);
    direction_q12.y = (gwp89_fx)(dir->y / 16L);
    direction_q12.z = (gwp89_fx)(dir->z / 16L);
    if (!blank3d_collision_raycast(collision, &origin_q12, &direction_q12,
                                   (gwp89_fx)(max_dist / 16L),
                                   B3D_COLLISION_LAYER_ENEMY |
                                   B3D_COLLISION_LAYER_WORLD, &hit))
        return 0;
    out_hit->valid = 1;
    out_hit->target_id = (short)(hit.enemy_index >= 0
                               ? 100 + hit.enemy_index : 0);
    out_hit->target_kind = (short)(hit.enemy_index >= 0
                                 ? GAQ89_TARGET_ACTOR : GAQ89_TARGET_WORLD);
    out_hit->material_id = (short)hit.material_id;
    out_hit->distance = q12_to_q16_long(hit.distance_fx);
    out_hit->position.x = q12_to_q16_long(hit.point.x);
    out_hit->position.y = q12_to_q16_long(hit.point.y);
    out_hit->position.z = q12_to_q16_long(hit.point.z);
    out_hit->normal.x = q12_to_q16_long(hit.normal.x);
    out_hit->normal.y = q12_to_q16_long(hit.normal.y);
    out_hit->normal.z = q12_to_q16_long(hit.normal.z);
    return 1;
}

static GWP89_Vec3 vec3_to_gwp(Vec3 value)
{
    GWP89_Vec3 result;
    result.x = (gwp89_fx)value.x;
    result.y = (gwp89_fx)value.y;
    result.z = (gwp89_fx)value.z;
    return result;
}

static Vec3 gwp_to_vec3(GWP89_Vec3 value)
{
    return gamlib_vec3((g3d_fix)value.x, (g3d_fix)value.y, (g3d_fix)value.z);
}

static g3d_fix milliseconds_to_seconds_fix(unsigned short milliseconds)
{
    return (g3d_fix)(((unsigned long)milliseconds *
                      (unsigned long)G3D_FIX_ONE) / 1000UL);
}

static int parse_fixed(const char *text, g3d_fix *out_value)
{
    const char *p;
    int negative;
    unsigned long whole;
    unsigned long fraction;
    unsigned long divisor;
    unsigned int digits;
    g3d_fix value;
    g3d_fix fraction_fixed;

    if (!text || !out_value) return 0;
    p = text;
    negative = 0;
    whole = 0UL;
    fraction = 0UL;
    divisor = 1UL;
    digits = 0U;

    if (*p == '-') {
        negative = 1;
        ++p;
    } else if (*p == '+') {
        ++p;
    }

    if (!isdigit((unsigned char)*p) && *p != '.') return 0;
    while (isdigit((unsigned char)*p)) {
        whole = whole * 10UL + (unsigned long)(*p - '0');
        ++p;
    }
    if (*p == '.') {
        ++p;
        while (isdigit((unsigned char)*p) && digits < 6U) {
            fraction = fraction * 10UL + (unsigned long)(*p - '0');
            divisor *= 10UL;
            ++digits;
            ++p;
        }
        while (isdigit((unsigned char)*p)) ++p;
    }
    if (*p != '\0') return 0;
    if (whole > 524287UL) return 0;

    value = (g3d_fix)(whole * (unsigned long)G3D_FIX_ONE);
    fraction_fixed = 0;
    if (divisor > 1UL) {
        fraction_fixed = (g3d_fix)((fraction * (unsigned long)G3D_FIX_ONE) / divisor);
    }
    value = g3d_fix_add_sat(value, fraction_fixed);
    if (negative) value = g3d_fix_neg_sat(value);
    *out_value = value;
    return 1;
}

static FileStamp get_stamp(const char *path)
{
    WIN32_FILE_ATTRIBUTE_DATA data;
    FileStamp stamp;
    stamp.ok = GetFileAttributesExA(path, GetFileExInfoStandard, &data) ? 1 : 0;
    stamp.time.dwLowDateTime = stamp.ok ? data.ftLastWriteTime.dwLowDateTime : 0;
    stamp.time.dwHighDateTime = stamp.ok ? data.ftLastWriteTime.dwHighDateTime : 0;
    return stamp;
}

static int stamp_changed(FileStamp *old_stamp, const char *path)
{
    FileStamp now;
    int changed;
    now = get_stamp(path);
    changed = 0;
    if (now.ok != old_stamp->ok) changed = 1;
    else if (now.ok && CompareFileTime(&now.time, &old_stamp->time) != 0) changed = 1;
    if (changed) *old_stamp = now;
    return changed;
}

static void set_status(const char *message)
{
    strncpy(g.status, message, sizeof(g.status) - 1U);
    g.status[sizeof(g.status) - 1U] = '\0';
}

static void set_game_yaw(Transform *transform, g3d_fix logical_yaw)
{
    if (!transform) return;
    transform->rotation.y = g3d_fix_wrap_angle_deg(
        g3d_fix_add_sat(logical_yaw, G3D_FIX_FROM_INT(180)));
}

static g3d_fix fixed_atan2_xz(Vec3 direction)
{
    g3d_fix abs_x;
    g3d_fix ratio;
    g3d_fix angle;
    g3d_fix forty_five;
    g3d_fix one_thirty_five;
    g3d_fix denominator;

    if (g3d_fix_abs(direction.x) <= G3D_FIX_EPSILON &&
        g3d_fix_abs(direction.z) <= G3D_FIX_EPSILON) return 0;

    abs_x = g3d_fix_add_sat(g3d_fix_abs(direction.x), G3D_FIX_EPSILON);
    forty_five = G3D_FIX_FROM_INT(45);
    one_thirty_five = G3D_FIX_FROM_INT(135);

    if (direction.z >= 0) {
        denominator = g3d_fix_add_sat(direction.z, abs_x);
        if (g3d_fix_abs(denominator) <= G3D_FIX_EPSILON) ratio = 0;
        else ratio = g3d_fix_div(g3d_fix_sub_sat(direction.z, abs_x), denominator);
        angle = g3d_fix_sub_sat(forty_five, g3d_fix_mul(forty_five, ratio));
    } else {
        denominator = g3d_fix_sub_sat(abs_x, direction.z);
        if (g3d_fix_abs(denominator) <= G3D_FIX_EPSILON) ratio = 0;
        else ratio = g3d_fix_div(g3d_fix_add_sat(direction.z, abs_x), denominator);
        angle = g3d_fix_sub_sat(one_thirty_five, g3d_fix_mul(forty_five, ratio));
    }
    if (direction.x < 0) angle = g3d_fix_neg_sat(angle);
    return angle;
}

static soq3d_pose local_pose(g3d_fix x, g3d_fix y, g3d_fix z)
{
    soq3d_pose pose;
    pose = soq3d_pose_identity();
    pose.position.x = bridge_q20_to_q16(x);
    pose.position.y = bridge_q20_to_q16(y);
    pose.position.z = bridge_q20_to_q16(z);
    return pose;
}

static void initialize_keys(void)
{
    int i;
    char name[64];
    g.player_key = soq3d_key_from_cstr("player");
    g.player_body_socket = soq3d_key_from_cstr("player.body");
    g.player_weapon_socket = soq3d_key_from_cstr("player.weapon");
    g.player_muzzle_socket = soq3d_key_from_cstr("player.muzzle");
    g.player_ballistic_muzzle_socket =
        soq3d_key_from_cstr("player.ballistic_muzzle");
    g.camera_socket = soq3d_key_from_cstr("camera.chase");
    g.aim_socket = soq3d_key_from_cstr("player.aim");
    for (i = 0; i < MAX_ENEMIES; ++i) {
        sprintf(name, "enemy.%d", i);
        g.enemy_keys[i] = soq3d_key_from_cstr(name);
        sprintf(name, "enemy.%d.body", i);
        g.enemy_body_sockets[i] = soq3d_key_from_cstr(name);
        sprintf(name, "enemy.%d.head", i);
        g.enemy_head_sockets[i] = soq3d_key_from_cstr(name);
        sprintf(name, "enemy.%d.weapon", i);
        g.enemy_weapon_sockets[i] = soq3d_key_from_cstr(name);
        sprintf(name, "enemy.%d.muzzle", i);
        g.enemy_muzzle_sockets[i] = soq3d_key_from_cstr(name);
    }
    for (i = 0; i < MAX_BULLETS; ++i) {
        sprintf(name, "bullet.%d", i);
        g.bullet_keys[i] = soq3d_key_from_cstr(name);
    }
}

static void configure_sockets(void)
{
    soq3d_pose pose;
    int i;

    pose = local_pose(0, fix_ratio(11, 20), 0);
    soq3d_define_local_socket(&g.locator, g.player_body_socket, g.player_key, &pose);
    pose = local_pose(0, fix_ratio(3, 4), g3d_fix_neg_sat(G3D_FIX_FROM_INT(1)));
    soq3d_define_local_socket(&g.locator, g.player_weapon_socket, g.player_key, &pose);
    pose = local_pose(0, fix_ratio(3, 4), g3d_fix_neg_sat(fix_ratio(33, 20)));
    soq3d_define_local_socket(&g.locator, g.player_muzzle_socket, g.player_key, &pose);
    /* Ballistics must not inherit the recoil/barrel animation published into
       player.muzzle. This sibling remains local to the current actor root. */
    soq3d_define_local_socket(&g.locator,
                              g.player_ballistic_muzzle_socket,
                              g.player_key, &pose);
    pose = local_pose(0, G3D_FIX_FROM_INT(1), 0);
    soq3d_define_local_socket(&g.locator, g.aim_socket, g.player_key, &pose);
    pose = local_pose(0, g.camera_height, g.camera_dist);
    soq3d_define_local_socket(&g.locator, g.camera_socket, g.player_key, &pose);

    for (i = 0; i < MAX_ENEMIES; ++i) {
        pose = local_pose(0, fix_ratio(9, 10), 0);
        soq3d_define_local_socket(&g.locator, g.enemy_body_sockets[i], g.enemy_keys[i], &pose);
        pose = local_pose(0, fix_ratio(39, 20), 0);
        soq3d_define_local_socket(&g.locator, g.enemy_head_sockets[i], g.enemy_keys[i], &pose);
        pose = local_pose(fix_ratio(7, 20), fix_ratio(6, 5),
                          g3d_fix_neg_sat(fix_ratio(2, 5)));
        soq3d_define_local_socket(&g.locator, g.enemy_weapon_sockets[i],
                                  g.enemy_keys[i], &pose);
        pose = local_pose(fix_ratio(7, 20), fix_ratio(6, 5),
                          g3d_fix_neg_sat(fix_ratio(3, 2)));
        soq3d_define_local_socket(&g.locator, g.enemy_muzzle_sockets[i],
                                  g.enemy_keys[i], &pose);
    }
}

static int initialize_meshes(void)
{
    int result;
    g3d_color blue;
    g3d_color yellow;
    g3d_color metal;
    g3d_color dark_yellow;
    g3d_color dark_metal;
    g3d_color red;
    g3d_color dark_red;
    g3d_color ally_blue;
    g3d_color ally_dark_blue;

    blue = g3d_color_rgba(64U, 158U, 255U, 255U);
    yellow = g3d_color_rgba(235U, 225U, 64U, 255U);
    metal = g3d_color_rgba(174U, 190U, 184U, 255U);
    dark_yellow = g3d_color_rgba(126U, 117U, 38U, 255U);
    dark_metal = g3d_color_rgba(72U, 78U, 82U, 255U);
    red = g3d_color_rgba(255U, 46U, 46U, 255U);
    dark_red = g3d_color_rgba(170U, 24U, 24U, 255U);
    ally_blue = g3d_color_rgba(42U, 112U, 238U, 255U);
    ally_dark_blue = g3d_color_rgba(20U, 58U, 150U, 255U);

    result = g3d_mesh_init(&g.meshes.player_body,
                           g.meshes.player_body_vertices, PLAYER_BOX_VERTICES,
                           g.meshes.player_body_indices, PLAYER_BOX_INDICES);
    if (result != G3D_OK) return 0;
    result = g3d_make_box(&g.meshes.player_body,
                          g3d_fx_from_int(1),
                          g3d_fx_from_ratio(11, 10),
                          g3d_fx_from_ratio(8, 5), blue);
    if (result != G3D_OK) return 0;

    result = g3d_mesh_init(&g.meshes.weapon,
                           g.meshes.weapon_vertices, WEAPON_BOX_VERTICES,
                           g.meshes.weapon_indices, WEAPON_BOX_INDICES);
    if (result != G3D_OK) return 0;
    result = g3d_make_box(&g.meshes.weapon,
                          g3d_fx_from_ratio(1, 4),
                          g3d_fx_from_ratio(1, 4),
                          g3d_fx_from_ratio(11, 10), yellow);
    if (result != G3D_OK) return 0;

    result = g3d_mesh_init(
        &g.meshes.mechanical_weapon[B3D_MECH89_MESH_BODY],
        g.meshes.mechanical_weapon_vertices[B3D_MECH89_MESH_BODY],
        WEAPON_BOX_VERTICES,
        g.meshes.mechanical_weapon_indices[B3D_MECH89_MESH_BODY],
        WEAPON_BOX_INDICES);
    if (result != G3D_OK) return 0;
    result = g3d_make_box(
        &g.meshes.mechanical_weapon[B3D_MECH89_MESH_BODY],
        g3d_fx_from_ratio(1, 4), g3d_fx_from_ratio(1, 4),
        g3d_fx_from_ratio(11, 10), yellow);
    if (result != G3D_OK) return 0;

    result = g3d_mesh_init(
        &g.meshes.mechanical_weapon[B3D_MECH89_MESH_SLIDE],
        g.meshes.mechanical_weapon_vertices[B3D_MECH89_MESH_SLIDE],
        WEAPON_BOX_VERTICES,
        g.meshes.mechanical_weapon_indices[B3D_MECH89_MESH_SLIDE],
        WEAPON_BOX_INDICES);
    if (result != G3D_OK) return 0;
    result = g3d_make_box(
        &g.meshes.mechanical_weapon[B3D_MECH89_MESH_SLIDE],
        g3d_fx_from_ratio(9, 40), g3d_fx_from_ratio(3, 20),
        g3d_fx_from_ratio(7, 10), metal);
    if (result != G3D_OK) return 0;

    result = g3d_mesh_init(
        &g.meshes.mechanical_weapon[B3D_MECH89_MESH_MAGAZINE],
        g.meshes.mechanical_weapon_vertices[B3D_MECH89_MESH_MAGAZINE],
        WEAPON_BOX_VERTICES,
        g.meshes.mechanical_weapon_indices[B3D_MECH89_MESH_MAGAZINE],
        WEAPON_BOX_INDICES);
    if (result != G3D_OK) return 0;
    result = g3d_make_box(
        &g.meshes.mechanical_weapon[B3D_MECH89_MESH_MAGAZINE],
        g3d_fx_from_ratio(3, 20), g3d_fx_from_ratio(2, 5),
        g3d_fx_from_ratio(1, 4), dark_yellow);
    if (result != G3D_OK) return 0;

    result = g3d_mesh_init(
        &g.meshes.mechanical_weapon[B3D_MECH89_MESH_BARREL],
        g.meshes.mechanical_weapon_vertices[B3D_MECH89_MESH_BARREL],
        WEAPON_BOX_VERTICES,
        g.meshes.mechanical_weapon_indices[B3D_MECH89_MESH_BARREL],
        WEAPON_BOX_INDICES);
    if (result != G3D_OK) return 0;
    result = g3d_make_box(
        &g.meshes.mechanical_weapon[B3D_MECH89_MESH_BARREL],
        g3d_fx_from_ratio(1, 10), g3d_fx_from_ratio(1, 10),
        g3d_fx_from_ratio(2, 5), dark_metal);
    if (result != G3D_OK) return 0;

    result = g3d_mesh_init(&g.meshes.enemy_body,
                           g.meshes.enemy_body_vertices, ENEMY_BODY_VERTICES,
                           g.meshes.enemy_body_indices, ENEMY_BODY_INDICES);
    if (result != G3D_OK) return 0;
    result = g3d_make_cylinder(&g.meshes.enemy_body,
                               g3d_fx_from_ratio(13, 20),
                               g3d_fx_from_ratio(9, 5), 18U, red);
    if (result != G3D_OK) return 0;

    result = g3d_mesh_init(&g.meshes.enemy_head,
                           g.meshes.enemy_head_vertices, ENEMY_HEAD_VERTICES,
                           g.meshes.enemy_head_indices, ENEMY_HEAD_INDICES);
    if (result != G3D_OK) return 0;
    result = g3d_make_box(&g.meshes.enemy_head,
                          g3d_fx_from_ratio(9, 10),
                          g3d_fx_from_ratio(7, 20),
                          g3d_fx_from_ratio(9, 10), dark_red);
    if (result != G3D_OK) return 0;

    result = g3d_mesh_init(&g.meshes.ally_body,
                           g.meshes.ally_body_vertices, ENEMY_BODY_VERTICES,
                           g.meshes.ally_body_indices, ENEMY_BODY_INDICES);
    if (result != G3D_OK) return 0;
    result = g3d_make_capsule(&g.meshes.ally_body,
                              g3d_fx_from_ratio(13, 20),
                              g3d_fx_from_ratio(1, 2),
                              12U, 3U, ally_blue);
    if (result != G3D_OK) return 0;

    result = g3d_mesh_init(&g.meshes.ally_head,
                           g.meshes.ally_head_vertices, ENEMY_HEAD_VERTICES,
                           g.meshes.ally_head_indices, ENEMY_HEAD_INDICES);
    if (result != G3D_OK) return 0;
    result = g3d_make_box(&g.meshes.ally_head,
                          g3d_fx_from_ratio(9, 10),
                          g3d_fx_from_ratio(7, 20),
                          g3d_fx_from_ratio(9, 10), ally_dark_blue);
    if (result != G3D_OK) return 0;

    {
        int projectile_mesh_id;
        for (projectile_mesh_id = 1;
             projectile_mesh_id <= PROJECTILE_MESH_COUNT;
             ++projectile_mesh_id) {
            if (!blank3d_projectile_mesh_build(
                    projectile_mesh_id,
                    &g.meshes.projectile[projectile_mesh_id - 1],
                    g.meshes.projectile_vertices[projectile_mesh_id - 1],
                    BULLET_VERTICES,
                    g.meshes.projectile_indices[projectile_mesh_id - 1],
                    BULLET_INDICES)) {
                return 0;
            }
        }
    }

    {
        int casing_mesh_id;
        for (casing_mesh_id = 1;
             casing_mesh_id <= CASING_MESH_COUNT;
             ++casing_mesh_id) {
            if (!blank3d_casing_mesh_build(
                    casing_mesh_id,
                    &g.meshes.casing[casing_mesh_id - 1],
                    g.meshes.casing_vertices[casing_mesh_id - 1],
                    CASING_VERTICES,
                    g.meshes.casing_indices[casing_mesh_id - 1],
                    CASING_INDICES)) {
                return 0;
            }
        }
    }
    return 1;
}

static void reset_transform(Transform *transform, g3d_fix x, g3d_fix y, g3d_fix z)
{
    transform_init(transform);
    transform->position = gamlib_vec3(x, y, z);
    set_game_yaw(transform, 0);
}

static void default_world(void)
{
    int i;
    g.width = 960;
    g.height = 540;
    reset_transform(&g.player, 0, 0, 0);
    g.player_hp = 100;
    g.player_damage_flash_ms = 0;
    g.move_speed = G3D_FIX_FROM_INT(7);
    g.strafe_speed = fix_ratio(11, 2);
    g.vertical_speed = G3D_FIX_FROM_INT(4);
    blank3d_vertical_body_init(&g.player_vertical, &g.player,
                               g.player.position.y, 0);
    g.turn_speed = G3D_FIX_FROM_INT(140);
    g.bullet_timer = 0;
    g.camera_dist = G3D_FIX_FROM_INT(8);
    g.camera_height = G3D_FIX_FROM_INT(4);
    g.camera_shoulder = G3D_FIX_FROM_INT(1);
    g.camera_eye_height = fix_ratio(17, 10);
    g.camera_yaw = g.player.rotation.y;
    g.camera_pitch = 0;
    g.camera_pitch_min = G3D_FIX_FROM_INT(-85);
    g.camera_pitch_max = G3D_FIX_FROM_INT(85);
    g.mouse_sensitivity = fix_ratio(18, 100);
    g.camera_mode = CAMERA_TPS;
    g.fire_requested = 0;
    g.muzzle_flash_ms = 0;
    g.gatling_spin_ms = 0U;
    g.gatling_spinup_ms = GATLING_SPINUP_MS;
    g.gatling_armed = 0;
    g.slingshot_charge_ms = 0U;
    g.slingshot_charge_max_ms = 900U;
    g.slingshot_charging = 0;
    g.sniper_sway_yaw = 0;
    g.sniper_sway_pitch = 0;
    g.fov = G3D_FIX_FROM_INT(70);
    g.base_fov = g.fov;
    g.zoom_fov = G3D_FIX_FROM_INT(45);
    g.sniper_zoom_fov = G3D_FIX_FROM_INT(18);
    g.zoom_speed = G3D_FIX_FROM_INT(120);
    g.zoom_fx = GWP89_FIX_ONE;
    g.near_z = fix_ratio(1, 10);
    g.far_z = G3D_FIX_FROM_INT(400);
    g.show_grid = 1;
    g.enemy_count = 3;

    for (i = 0; i < MAX_BULLETS; ++i) {
        g.bullets[i].alive = 0;
        reset_transform(&g.bullets[i].transform, 0, 0, 0);
        g.bullets[i].weapon_id = 0;
        g.bullets[i].owner_actor_id = 0;
        g.bullets[i].owner_team_id = 0;
        g.bullets[i].projectile_id = 0;
        g.bullets[i].projectile_mesh_id = 1;
        g.bullets[i].trail_id = 0;
        g.bullets[i].aoi_trail_id = -1;
        g.bullets[i].physics_backend = B3D_PHYSICS_LINEAR;
        g.bullets[i].bolt_projectile_id = -1;
        g.bullets[i].damage = 0;
        g.bullets[i].radius = fix_ratio(1, 10);
        g.bullets[i].mesh_scale = G3D_FIX_ONE;
        g.bullets[i].previous_position = gamlib_vec3(0, 0, 0);
        g.bullets[i].velocity = gamlib_vec3(0, 0, 0);
        g.bullets[i].life = 0;
        g.bullets[i].audio_primary_key = 0U;
        g.bullets[i].audio_secondary_key = 0U;
    }
    for (i = 0; i < MAX_CASINGS; ++i) {
        g.casings[i].alive = 0;
        g.casings[i].weapon_id = 0;
        g.casings[i].casing_mesh_id = 1;
        g.casings[i].physics_active = 0;
        g.casings[i].mesh_scale = G3D_FIX_ONE;
        reset_transform(&g.casings[i].transform, 0, 0, 0);
        g.casings[i].velocity = gamlib_vec3(0, 0, 0);
        g.casings[i].life = 0;
    }
    for (i = 0; i < MAX_ENEMIES; ++i) {
        g.enemies[i].alive = 0;
        g.enemies[i].state = 0;
        g.enemies[i].archetype[0] = '\0';
        g.enemies[i].hp = 30;
        g.enemies[i].cooldown = 0;
        reset_transform(&g.enemies[i].transform, 0, 0, 0);
        blank3d_vertical_body_init(&g.enemies[i].vertical,
                                   &g.enemies[i].transform, 0, 0);
        blank3d_automotion_init(&g.enemies[i].automotion);
        blank3d_motion_attack_init(&g.enemies[i].motion_attack);
        blank3d_truth_gate_init(&g.enemies[i].truth_gate,
                                B3D_TRUTH_PROFILE_RETRO);
        g.enemies[i].truth_range = G3D_FIX_FROM_INT(28);
        blank3d_perception_config_defaults(&g.enemies[i].perception_config);
        g.enemies[i].perception_ini[0] = '\0';
        g.enemies[i].perception_index = i;
        g.enemies[i].target_actor_id = B3D_TARGET_NONE;
        g.enemies[i].last_damage_source_actor_id = B3D_TARGET_NONE;
        g.enemies[i].damage_interest_time = 0;
        g.enemies[i].faction_is_ally = 0;
        strcpy(g.enemies[i].faction_name, "hostile");
        strcpy(g.enemies[i].team_name, "monsters");
        strcpy(g.enemies[i].role_name, "attacker");
        strcpy(g.enemies[i].faction_tags,
               "organic,enemy,targetable");
    }
    g.enemies[0].alive = 1;
    reset_transform(&g.enemies[0].transform, 0, 0, G3D_FIX_FROM_INT(18));
    blank3d_vertical_body_init(&g.enemies[0].vertical,
                               &g.enemies[0].transform,
                               g.enemies[0].transform.position.y, 0);
    blank3d_automotion_init_archetype(&g.enemies[0].automotion, "zombie");
    g.enemies[1].alive = 1;
    reset_transform(&g.enemies[1].transform, G3D_FIX_FROM_INT(-8), 0, G3D_FIX_FROM_INT(24));
    blank3d_vertical_body_init(&g.enemies[1].vertical,
                               &g.enemies[1].transform,
                               g.enemies[1].transform.position.y, 0);
    blank3d_automotion_init_archetype(&g.enemies[1].automotion, "zombie");
    g.enemies[2].alive = 1;
    reset_transform(&g.enemies[2].transform, G3D_FIX_FROM_INT(8), 0, G3D_FIX_FROM_INT(24));
    blank3d_vertical_body_init(&g.enemies[2].vertical,
                               &g.enemies[2].transform,
                               g.enemies[2].transform.position.y, 0);
    blank3d_automotion_init_archetype(&g.enemies[2].automotion, "zombie");

    camera_init_perspective(&g.camera, g.fov,
                            fix_ratio(g.width, g.height),
                            g.near_z, g.far_z);
    configure_sockets();
}

static void publish_transform(soq3d_key key, const Transform *transform)
{
    soq3d_pose pose;
    bridge_transform_to_pose(transform, &pose);
    soq3d_publish_thing(&g.locator, key, &pose);
}

/* Projectile library meshes are authored with local +Z pointing toward the
   nose. Build a fixed-point orthonormal basis from actual velocity so pitch,
   yaw and spread all orient the mesh along its ballistic path. */
static void bullet_to_pose(const Bullet *bullet, soq3d_pose *out_pose)
{
    Vec3 forward;
    Vec3 right;
    Vec3 up;
    Vec3 reference_up;
    Vec3 scaled_right;
    Vec3 scaled_up;
    Vec3 scaled_forward;
    g3d_fix right_length;

    if (!bullet || !out_pose) return;
    forward = bullet->velocity;
    if (gamlib_vec3_length(&forward) <= G3D_FIX_EPSILON)
        forward = gamlib_vec3(0, 0, G3D_FIX_ONE);
    else
        gamlib_vec3_normalize(&forward, &forward);

    reference_up = gamlib_vec3(0, G3D_FIX_ONE, 0);
    gamlib_vec3_cross(&right, &reference_up, &forward);
    right_length = gamlib_vec3_length(&right);
    if (right_length <= fix_ratio(1, 100)) {
        reference_up = gamlib_vec3(G3D_FIX_ONE, 0, 0);
        gamlib_vec3_cross(&right, &reference_up, &forward);
    }
    gamlib_vec3_normalize(&right, &right);
    gamlib_vec3_cross(&up, &forward, &right);
    gamlib_vec3_normalize(&up, &up);

    gamlib_vec3_scale(&scaled_right, &right, bullet->mesh_scale);
    gamlib_vec3_scale(&scaled_up, &up, bullet->mesh_scale);
    gamlib_vec3_scale(&scaled_forward, &forward, bullet->mesh_scale);

    out_pose->position.x = bridge_q20_to_q16(bullet->transform.position.x);
    out_pose->position.y = bridge_q20_to_q16(bullet->transform.position.y);
    out_pose->position.z = bridge_q20_to_q16(bullet->transform.position.z);

    out_pose->basis.m00 = bridge_q20_to_q16(scaled_right.x);
    out_pose->basis.m10 = bridge_q20_to_q16(scaled_right.y);
    out_pose->basis.m20 = bridge_q20_to_q16(scaled_right.z);
    out_pose->basis.m01 = bridge_q20_to_q16(scaled_up.x);
    out_pose->basis.m11 = bridge_q20_to_q16(scaled_up.y);
    out_pose->basis.m21 = bridge_q20_to_q16(scaled_up.z);
    out_pose->basis.m02 = bridge_q20_to_q16(scaled_forward.x);
    out_pose->basis.m12 = bridge_q20_to_q16(scaled_forward.y);
    out_pose->basis.m22 = bridge_q20_to_q16(scaled_forward.z);
}

static void publish_bullet(int bullet_index)
{
    soq3d_pose pose;
    if (bullet_index < 0 || bullet_index >= MAX_BULLETS) return;
    if (!g.bullets[bullet_index].alive) return;
    bullet_to_pose(&g.bullets[bullet_index], &pose);
    soq3d_publish_thing(&g.locator, g.bullet_keys[bullet_index], &pose);
}

static void publish_all(void)
{
    int i;
    publish_transform(g.player_key, &g.player);
    for (i = 0; i < g.enemy_count; ++i) {
        if (g.enemies[i].alive) publish_transform(g.enemy_keys[i], &g.enemies[i].transform);
    }
    for (i = 0; i < MAX_BULLETS; ++i) {
        if (g.bullets[i].alive) publish_bullet(i);
    }
}

static void begin_locator_frame(void)
{
    ++g.frame_stamp;
    if (g.frame_stamp == SOQ3D_STAMP_ANY || g.frame_stamp == 0U) g.frame_stamp = 1U;
    soq3d_begin_frame(&g.locator, g.frame_stamp);
    publish_all();
}

static void release_bullet_slot(int bullet_index)
{
    if (bullet_index < 0 || bullet_index >= MAX_BULLETS) return;
    blank3d_audio_projectile_end(
        &g.audio,
        g.bullets[bullet_index].weapon_id,
        g.bullets[bullet_index].audio_primary_key,
        g.bullets[bullet_index].audio_secondary_key);
    g.bullets[bullet_index].audio_primary_key = 0U;
    g.bullets[bullet_index].audio_secondary_key = 0U;
    blank3d_trails_release(&g.trails, bullet_index);
    if (g.bullets[bullet_index].bolt_projectile_id >= 0)
        blank3d_bolt_destroy(&g.bolt,
                             g.bullets[bullet_index].bolt_projectile_id);
    g.bullets[bullet_index].bolt_projectile_id = -1;
    g.bullets[bullet_index].aoi_trail_id = -1;
    g.bullets[bullet_index].alive = 0;
}

static int find_projectile_slot(int weapon_id)
{
    int i;
    int candidate;
    g3d_fix shortest_life;

    for (i = 0; i < MAX_BULLETS; ++i) {
        if (!g.bullets[i].alive) return i;
    }

    /* A sustained Gatling burst must never silently stop rendering merely
       because an older tracer still owns a slot. Recycle the oldest Gatling
       tracer first, then the oldest non-explosive small-arms projectile. */
    if (weapon_id != GATLING_WEAPON_ID) return -1;
    candidate = -1;
    shortest_life = G3D_FIX_MAX;
    for (i = 0; i < MAX_BULLETS; ++i) {
        if (g.bullets[i].weapon_id == GATLING_WEAPON_ID &&
            g.bullets[i].life < shortest_life) {
            candidate = i;
            shortest_life = g.bullets[i].life;
        }
    }
    if (candidate >= 0) {
        release_bullet_slot(candidate);
        return candidate;
    }
    for (i = 0; i < MAX_BULLETS; ++i) {
        if (g.bullets[i].weapon_id != 6 && g.bullets[i].weapon_id != 7 &&
            g.bullets[i].life < shortest_life) {
            candidate = i;
            shortest_life = g.bullets[i].life;
        }
    }
    if (candidate >= 0) release_bullet_slot(candidate);
    return candidate;
}

static void spawn_bullet_event(const GWP89_Event *event)
{
    int i;
    int bolt_id;
    Vec3 direction;
    Vec3 camera_eye;
    Vec3 camera_forward;
    Vec3 camera_right;
    Vec3 camera_up;
    Vec3 center_target;
    Vec3 target_offset;
    GWP89_Vec3 camera_eye_q12;
    GWP89_Vec3 camera_forward_q12;
    GWP89_Vec3 camera_right_q12;
    GWP89_Vec3 camera_up_q12;
    GWP89_Vec3 center_target_q12;
    GWP89_Vec3 direction_q12;
    GWP89_Event aligned_event;
    Blank3DCollisionHit center_hit;
    g3d_fix center_range;
    const GWP89_Event *launch_event;
    int captured_view_valid;
    gwp89_fx gravity_q12;
    const Blank3DWeaponModules *modules;
    if (!event) return;

    launch_event = event;
    if (launch_event->actor_id == B3D_PLAYER_ACTOR_ID &&
        g.cameranaku.initialized) {
        /* Never re-zero an event against a camera that has already moved or
           recoiled. Prefer the immutable fire-time snapshot carried by the
           manager event; legacy/foreign events fall back to the live view. */
        camera_eye_q12 = event->camera_origin;
        camera_forward_q12 = event->camera_forward;
        camera_right_q12 = event->camera_right;
        camera_up_q12 = event->camera_up;
        camera_eye = gwp_to_vec3(camera_eye_q12);
        camera_forward = gwp_to_vec3(camera_forward_q12);
        camera_right = gwp_to_vec3(camera_right_q12);
        camera_up = gwp_to_vec3(camera_up_q12);
        captured_view_valid =
            gamlib_vec3_length(&camera_forward) > G3D_FIX_EPSILON;
        if (!captured_view_valid) {
            blank3d_cameranaku_get_view(&g.cameranaku,
                                        &camera_eye,
                                        &camera_forward,
                                        &camera_right,
                                        &camera_up);
        } else {
            gamlib_vec3_normalize(&camera_forward, &camera_forward);
            if (gamlib_vec3_length(&camera_right) > G3D_FIX_EPSILON)
                gamlib_vec3_normalize(&camera_right, &camera_right);
            if (gamlib_vec3_length(&camera_up) > G3D_FIX_EPSILON)
                gamlib_vec3_normalize(&camera_up, &camera_up);
        }
        camera_eye_q12 = vec3_to_gwp(camera_eye);
        camera_forward_q12 = vec3_to_gwp(camera_forward);
        camera_right_q12 = vec3_to_gwp(camera_right);
        camera_up_q12 = vec3_to_gwp(camera_up);

        if (event->weapon_id == B3D_SHOTGUN_WEAPON_ID) {
            /* The runtime pool receives a fresh center-HUD target and then
               rebuilds every pellet around it. This prevents camera zeroing
               providers from flattening all seven manager events into one
               visible projectile. */
            center_range = event->range_fx > 0
                ? (g3d_fix)event->range_fx
                : G3D_FIX_FROM_INT(28);
            if (event->view_style != GWP89_VIEW_FPS &&
                center_range > G3D_FIX_FROM_INT(32))
                center_range = G3D_FIX_FROM_INT(32);
            if (blank3d_collision_raycast(
                    &g.collision, &camera_eye_q12, &camera_forward_q12,
                    (gwp89_fx)center_range,
                    B3D_COLLISION_LAYER_WORLD | B3D_COLLISION_LAYER_ENEMY,
                    &center_hit)) {
                center_target_q12 = center_hit.point;
            } else {
                gamlib_vec3_scale(&target_offset, &camera_forward,
                                  center_range);
                gamlib_vec3_add(&center_target, &camera_eye, &target_offset);
                center_target_q12 = vec3_to_gwp(center_target);
            }
            if (blank3d_shotgun_prepare_pellet(
                    event, &camera_eye_q12, &camera_forward_q12,
                    &camera_right_q12, &camera_up_q12,
                    &center_target_q12, &aligned_event))
                launch_event = &aligned_event;
        } else if (blank3d_ballistics_align_event_to_view(
                event, &camera_eye_q12, &camera_forward_q12,
                &aligned_event)) {
            launch_event = &aligned_event;
        }

        /* The manager may supply an old or empty snapshot. Once this runtime
           has selected the fire-time/live fallback view, publish that exact
           frame into the aligned event so the final universal gate, trails and
           every weapon backend all compare against the same visible forward. */
        if (launch_event == &aligned_event) {
            aligned_event.camera_origin = camera_eye_q12;
            aligned_event.camera_forward = camera_forward_q12;
            aligned_event.camera_right = camera_right_q12;
            aligned_event.camera_up = camera_up_q12;
        }
    }

    i = find_projectile_slot(launch_event->weapon_id);
    if (i < 0) {
        set_status("projectile provider: pool full; spawn rejected");
        return;
    }
    release_bullet_slot(i);

    modules = blank3d_weapon_modules_get(launch_event->weapon_id);
    direction_q12 = launch_event->direction;
    gravity_q12 = 0;
    (void)blank3d_universal_aim_finalize_event(
        launch_event, modules, &direction_q12, &gravity_q12);
    g.bullets[i].gravity = (g3d_fix)gravity_q12;
    direction = gwp_to_vec3(direction_q12);
    if (gamlib_vec3_length(&direction) <= G3D_FIX_EPSILON)
        direction = gamlib_vec3(0, 0, G3D_FIX_ONE);
    else
        gamlib_vec3_normalize(&direction, &direction);
    direction_q12 = vec3_to_gwp(direction);

    g.bullets[i].alive = 1;
    g.bullets[i].weapon_id = launch_event->weapon_id;
    g.bullets[i].owner_actor_id = launch_event->actor_id;
    g.bullets[i].owner_team_id = launch_event->team_id;
    g.bullets[i].projectile_id = launch_event->projectile_id;
    g.bullets[i].projectile_mesh_id = launch_event->projectile_mesh_id;
    if (modules && modules->projectile_mesh_id > 0)
        g.bullets[i].projectile_mesh_id = modules->projectile_mesh_id;
    if (g.bullets[i].projectile_mesh_id < 1 ||
        g.bullets[i].projectile_mesh_id > PROJECTILE_MESH_COUNT)
        g.bullets[i].projectile_mesh_id = 1;
    g.bullets[i].trail_id = launch_event->trail_id;
    g.bullets[i].physics_backend = modules
        ? modules->physics_backend : B3D_PHYSICS_LINEAR;
    g.bullets[i].damage = gwp89_fx_to_int_round(launch_event->damage_fx);
    if (g.bullets[i].damage < 1) g.bullets[i].damage = 1;
    g.bullets[i].radius = (g3d_fix)launch_event->radius_fx;
    if (g.bullets[i].radius <= 0) g.bullets[i].radius = fix_ratio(1, 10);
    g.bullets[i].mesh_scale = (g3d_fix)launch_event->projectile_mesh_scale_fx;
    if (g.bullets[i].mesh_scale <= 0) g.bullets[i].mesh_scale = G3D_FIX_ONE;
    transform_init(&g.bullets[i].transform);
    g.bullets[i].transform.position = gwp_to_vec3(launch_event->origin);
    g.bullets[i].previous_position = g.bullets[i].transform.position;
    gamlib_vec3_scale(&g.bullets[i].velocity, &direction,
                      (g3d_fix)launch_event->speed_fx);
    g.bullets[i].life = milliseconds_to_seconds_fix(launch_event->life_ms);
    if (g.bullets[i].life <= 0) g.bullets[i].life = G3D_FIX_FROM_INT(2);

    g.bullets[i].aoi_trail_id = -1;
    if (modules && modules->emit_trail &&
        modules->trail_profile != B3D_TRAIL_NONE) {
        g.bullets[i].aoi_trail_id =
            blank3d_trails_attach(&g.trails, i, modules->trail_profile);
        blank3d_trails_emit_q12(&g.trails, i,
            g.bullets[i].transform.position.x,
            g.bullets[i].transform.position.y,
            g.bullets[i].transform.position.z);
    }

    g.bullets[i].bolt_projectile_id = -1;
    if (modules && modules->physics_backend == B3D_PHYSICS_BOLT3D) {
        bolt_id = blank3d_bolt_spawn(&g.bolt, modules,
                    B3D_PLAYER_ACTOR_ID, &launch_event->origin, &direction_q12,
                    launch_event->speed_fx, launch_event->radius_fx, launch_event->damage_fx,
                    launch_event->life_ms, &g.bullets[i]);
        if (bolt_id < 0) {
            release_bullet_slot(i);
            set_status("Bolt3D provider: slingshot spawn rejected");
            return;
        }
        g.bullets[i].bolt_projectile_id = bolt_id;
    }
    g.bullets[i].audio_primary_key = 0U;
    g.bullets[i].audio_secondary_key = 0U;
    publish_bullet(i);
    blank3d_audio_projectile_begin(
        &g.audio, launch_event->weapon_id,
        gwp89_fx_to_int_round(launch_event->speed_fx),
        &g.bullets[i].audio_primary_key,
        &g.bullets[i].audio_secondary_key);
    if (launch_event->weapon_id == GATLING_WEAPON_ID)
        set_status("gatling projectile provider: tracer spawned");
    else if (launch_event->weapon_id == SLINGSHOT_WEAPON_ID)
        set_status("slingshot: Bolt3D stone + Aoi Trail spawned");
}


static void b3d_casing_axes_for_event(const GWP89_Event *event,
                                      Vec3 *right,
                                      Vec3 *up,
                                      Vec3 *forward)
{
    int i;
    Transform actor_transform;
    Vec3 world_up;
    Vec3 event_forward;
    if (!right || !up || !forward) return;
    actor_transform = g.player;
    if (event && event->actor_id == B3D_PLAYER_ACTOR_ID) {
        actor_transform.rotation.y = g.camera_yaw;
        actor_transform.rotation.x = 0;
        transform_get_local_axes(&actor_transform, right, up, forward);
        return;
    }
    if (event) {
        for (i = 0; i < g.enemy_count; ++i) {
            if (!g.enemies[i].alive) continue;
            if (g.enemies[i].weapon_actor_id == event->actor_id) {
                actor_transform = g.enemies[i].transform;
                transform_get_local_axes(&actor_transform,
                                         right, up, forward);
                return;
            }
        }
        event_forward = gwp_to_vec3(event->direction);
        if (gamlib_vec3_length(&event_forward) > G3D_FIX_EPSILON) {
            gamlib_vec3_normalize(forward, &event_forward);
            world_up = gamlib_vec3(0, G3D_FIX_ONE, 0);
            gamlib_vec3_cross(right, &world_up, forward);
            if (gamlib_vec3_length(right) > G3D_FIX_EPSILON)
                gamlib_vec3_normalize(right, right);
            else
                *right = gamlib_vec3(G3D_FIX_ONE, 0, 0);
            gamlib_vec3_cross(up, forward, right);
            gamlib_vec3_normalize(up, up);
            return;
        }
    }
    transform_get_local_axes(&actor_transform, right, up, forward);
}

static void spawn_casing_event(const GWP89_Event *event)
{
    int i;
    Vec3 right;
    Vec3 up;
    Vec3 forward;
    Vec3 impulse;
    Vec3 offset;
    if (!event || !blank3d_weapon_modules_has_casing(event->weapon_id))
        return;
    b3d_casing_axes_for_event(event, &right, &up, &forward);
    for (i = 0; i < MAX_CASINGS; ++i) {
        if (!g.casings[i].alive) {
            g.casings[i].alive = 1;
            g.casings[i].weapon_id = event->weapon_id;
            g.casings[i].casing_mesh_id = event->shell_mesh_id;
            if (g.casings[i].casing_mesh_id < 1 ||
                g.casings[i].casing_mesh_id > CASING_MESH_COUNT)
                g.casings[i].casing_mesh_id =
                    event->weapon_id == GATLING_WEAPON_ID ? 2 : event->weapon_id;
            if (g.casings[i].casing_mesh_id < 1 ||
                g.casings[i].casing_mesh_id > CASING_MESH_COUNT)
                g.casings[i].casing_mesh_id = 1;
            g.casings[i].mesh_scale = (g3d_fix)event->shell_mesh_scale_fx;
            if (g.casings[i].mesh_scale <= 0)
                g.casings[i].mesh_scale = G3D_FIX_ONE;
            transform_init(&g.casings[i].transform);
            g.casings[i].transform.position = gwp_to_vec3(event->origin);
            g.casings[i].transform.scale = gamlib_vec3(
                g.casings[i].mesh_scale,
                g.casings[i].mesh_scale,
                g.casings[i].mesh_scale);
            g.casings[i].physics_active = blank3d_casing_physics_spawn(
                &g.casing_physics, i,
                event->weapon_id, g.casings[i].casing_mesh_id,
                &g.casings[i].transform.position,
                &right, &up, &forward);
            if (!g.casings[i].physics_active) {
                /* Graceful fallback only when the VPhysics pool rejects the
                 * spawn. Unlike the old path, angular movement is damped and
                 * allowed to settle instead of rotating forever. */
                gamlib_vec3_scale(&impulse, &right,
                    event->weapon_id == GATLING_WEAPON_ID ? G3D_FIX_FROM_INT(5) :
                    (event->weapon_id == 2 ? G3D_FIX_FROM_INT(4) :
                     G3D_FIX_FROM_INT(3)));
                gamlib_vec3_scale(&offset, &up,
                    event->weapon_id == 3 ? G3D_FIX_FROM_INT(3) :
                    G3D_FIX_FROM_INT(2));
                gamlib_vec3_add(&g.casings[i].velocity, &impulse, &offset);
            }
            g.casings[i].life = G3D_FIX_FROM_INT(3);
            blank3d_audio_casing(&g.audio, event->weapon_id);
            return;
        }
    }
}

static void apply_ddsl2_action(const char *action)
{
    g3d_fix distance;
    if (strcmp(action, "move_forward") == 0 ||
        strcmp(action, "move_foward") == 0) {
        distance = g3d_fix_mul(g.move_speed, g.dt);
        transform_move_local_flat(&g.player, 0, 0, distance);
    } else if (strcmp(action, "move_back") == 0) {
        distance = g3d_fix_neg_sat(g3d_fix_mul(g.move_speed, g.dt));
        transform_move_local_flat(&g.player, 0, 0, distance);
    } else if (strcmp(action, "strafe_left") == 0) {
        distance = g3d_fix_neg_sat(g3d_fix_mul(g.strafe_speed, g.dt));
        transform_move_local_flat(&g.player, distance, 0, 0);
    } else if (strcmp(action, "strafe_right") == 0) {
        distance = g3d_fix_mul(g.strafe_speed, g.dt);
        transform_move_local_flat(&g.player, distance, 0, 0);
    } else if (strcmp(action, "move_up") == 0 ||
               strcmp(action, "move_y_up") == 0) {
        /* jump89 owns Y while its arc is active.  This keeps simultaneous
           J+F/G input deterministic instead of deforming the jump. */
        if (!blank3d_vertical_axis_jumping(&g.player_vertical))
            (void)blank3d_vertical_axis_fly(&g.vertical_axis,
                                            &g.player_vertical,
                                            g.vertical_speed, g.dt, FLY89_UP);
    } else if (strcmp(action, "move_down") == 0 ||
               strcmp(action, "move_y_down") == 0) {
        if (!blank3d_vertical_axis_jumping(&g.player_vertical))
            (void)blank3d_vertical_axis_fly(&g.vertical_axis,
                                            &g.player_vertical,
                                            g.vertical_speed, g.dt, FLY89_DOWN);
    } else if (strcmp(action, "jump") == 0) {
        (void)blank3d_vertical_axis_jump(&g.vertical_axis,
                                         &g.player_vertical,
                                         G3D_FIX_FROM_INT(9));
    } else if (strcmp(action, "turn_left") == 0) {
        blank3d_cameranaku_add_look(&g.cameranaku,
            g3d_fix_neg_sat(g3d_fix_mul(g.turn_speed, g.dt)), 0);
        g.camera_yaw = g.cameranaku.yaw;
        g.camera_pitch = g.cameranaku.pitch;
        g.player.rotation.y = g.camera_yaw;
    } else if (strcmp(action, "turn_right") == 0) {
        blank3d_cameranaku_add_look(&g.cameranaku,
            g3d_fix_mul(g.turn_speed, g.dt), 0);
        g.camera_yaw = g.cameranaku.yaw;
        g.camera_pitch = g.cameranaku.pitch;
        g.player.rotation.y = g.camera_yaw;
    } else if (strcmp(action, "shoot") == 0) {
        g.fire_requested = 1;
    }
}

static int key_down_name(const char *name)
{
    return blank3d_input_query(&g.input, B3D_INPUT_HOLD, name);
}

static int b3d_language_key_down(void *user, const char *name)
{
    (void)user;
    return key_down_name(name);
}

static int b3d_language_input_query(void *user, const char *state_name,
                                    const char *control_name)
{
    (void)user;
    return blank3d_input_query_name(&g.input, state_name, control_name);
}

static unsigned long b3d_ddsl_action_bit(const char *action)
{
    if (!action) return 0UL;
    if (strcmp(action, "move_forward") == 0 ||
        strcmp(action, "move_foward") == 0) return B3D_DDSL_ACT_FORWARD;
    if (strcmp(action, "move_back") == 0) return B3D_DDSL_ACT_BACK;
    if (strcmp(action, "strafe_left") == 0) return B3D_DDSL_ACT_LEFT;
    if (strcmp(action, "strafe_right") == 0) return B3D_DDSL_ACT_RIGHT;
    if (strcmp(action, "move_up") == 0 ||
        strcmp(action, "move_y_up") == 0) return B3D_DDSL_ACT_UP;
    if (strcmp(action, "move_down") == 0 ||
        strcmp(action, "move_y_down") == 0) return B3D_DDSL_ACT_DOWN;
    if (strcmp(action, "jump") == 0) return B3D_DDSL_ACT_JUMP;
    if (strcmp(action, "turn_left") == 0) return B3D_DDSL_ACT_TURN_LEFT;
    if (strcmp(action, "turn_right") == 0) return B3D_DDSL_ACT_TURN_RIGHT;
    if (strcmp(action, "shoot") == 0) return B3D_DDSL_ACT_SHOOT;
    return 0UL;
}

static void b3d_language_ddsl_action(void *user, const char *action,
                                     long value_fixed,
                                     const char *value_text)
{
    unsigned long bit;
    (void)user;
    if (!action) return;
    if (strcmp(action, "list_cyclenext") == 0 ||
        strcmp(action, "list_cycle_next") == 0 ||
        strcmp(action, "list_next") == 0) {
        if (value_text && value_text[0] != '\0')
            (void)blank3d_list_cycle_next(&g.list_cycles, value_text, 0);
        return;
    }
    if (strcmp(action, "list_cycleprev") == 0 ||
        strcmp(action, "list_cycle_prev") == 0 ||
        strcmp(action, "list_prev") == 0) {
        if (value_text && value_text[0] != '\0')
            (void)blank3d_list_cycle_prev(&g.list_cycles, value_text, 0);
        return;
    }
    if (value_fixed == 0L) return;
    bit = b3d_ddsl_action_bit(action);
    if (bit != 0UL) g.ddsl_action_mask |= bit;
    else apply_ddsl2_action(action);
}

static void b3d_apply_ddsl_action_mask(void)
{
    static const struct {
        unsigned long bit;
        const char *action;
    } actions[] = {
        { B3D_DDSL_ACT_FORWARD, "move_forward" },
        { B3D_DDSL_ACT_BACK, "move_back" },
        { B3D_DDSL_ACT_LEFT, "strafe_left" },
        { B3D_DDSL_ACT_RIGHT, "strafe_right" },
        { B3D_DDSL_ACT_UP, "move_up" },
        { B3D_DDSL_ACT_DOWN, "move_down" },
        { B3D_DDSL_ACT_JUMP, "jump" },
        { B3D_DDSL_ACT_TURN_LEFT, "turn_left" },
        { B3D_DDSL_ACT_TURN_RIGHT, "turn_right" },
        { B3D_DDSL_ACT_SHOOT, "shoot" }
    };
    unsigned int i;
    for (i = 0U; i < sizeof(actions) / sizeof(actions[0]); ++i)
        if ((g.ddsl_action_mask & actions[i].bit) != 0UL)
            apply_ddsl2_action(actions[i].action);
    g.ddsl_action_mask = 0UL;
}

static int b3d_is_ini_path(const char *text)
{
    const char *dot;
    if (!text || !*text) return 0;
    if (strchr(text, '/') || strchr(text, '\\')) return 1;
    dot = strrchr(text, '.');
    return dot && strcmp(dot, ".ini") == 0;
}

static int b3d_enemy_load_perception_ini(Enemy *enemy, const char *path)
{
    char status[128];
    if (!enemy || !path || !*path) return 0;
    if (!blank3d_perception_ini_load(path, &enemy->truth_gate,
            &enemy->truth_range, &enemy->perception_config,
            status, sizeof(status))) return 0;
    strncpy(enemy->perception_ini, path,
            sizeof(enemy->perception_ini) - 1U);
    enemy->perception_ini[sizeof(enemy->perception_ini) - 1U] = '\0';
    return 1;
}

static void b3d_rpyl_define_entity(const char *archetype,
                                   const char **args, int argc)
{
    int index;
    int hp;
    int argi;
    int flying_entity;
    int gravity_immune;
    g3d_fix x;
    g3d_fix y;
    g3d_fix z;
    g3d_fix floor_y;
    size_t length;
    char perception_path[192];
    if (!archetype || !args || argc < 7) return;
    if (strcmp(args[1], "pos") != 0 || strcmp(args[5], "hp") != 0) return;
    if (!parse_fixed(args[2], &x) || !parse_fixed(args[3], &y) ||
        !parse_fixed(args[4], &z)) return;
    index = atoi(args[0]);
    hp = atoi(args[6]);
    if (index < 0 || index >= MAX_ENEMIES) return;
    floor_y = 0;
    flying_entity = (strcmp(archetype, "dive_enemy") == 0 ||
                     strcmp(archetype, "dive_bomber_enemy") == 0);
    gravity_immune = 0;
    g.enemies[index].alive = 1;
    g.enemies[index].transform.position = gamlib_vec3(x, y, z);
    g.enemies[index].hp = hp;
    length = strlen(archetype);
    if (length >= sizeof(g.enemies[index].archetype))
        length = sizeof(g.enemies[index].archetype) - 1U;
    memcpy(g.enemies[index].archetype, archetype, length);
    g.enemies[index].archetype[length] = '\0';
    blank3d_automotion_init_archetype(&g.enemies[index].automotion,
                                       g.enemies[index].archetype);
    blank3d_motion_attack_init_archetype(&g.enemies[index].motion_attack,
                                          g.enemies[index].archetype);
    blank3d_truth_gate_init(&g.enemies[index].truth_gate,
                            B3D_TRUTH_PROFILE_RETRO);
    g.enemies[index].truth_range = G3D_FIX_FROM_INT(28);
    blank3d_perception_config_defaults(&g.enemies[index].perception_config);
    g.enemies[index].perception_ini[0] = '\0';
    sprintf(perception_path, "config/entities/%s.ini", archetype);
    (void)b3d_enemy_load_perception_ini(&g.enemies[index],
                                         perception_path);
    g.enemies[index].perception_index = index;
    g.enemies[index].weapon_loadout[0] = '\0';
    g.enemies[index].requested_weapon[0] = '\0';
    g.enemies[index].target_actor_id = B3D_TARGET_NONE;
    g.enemies[index].last_damage_source_actor_id = B3D_TARGET_NONE;
    g.enemies[index].damage_interest_time = 0;
    if (strcmp(archetype, "armed_ally") == 0) {
        strcpy(g.enemies[index].faction_name, "allies");
        strcpy(g.enemies[index].team_name, "survivors");
        strcpy(g.enemies[index].role_name, "armed_ally");
        strcpy(g.enemies[index].faction_tags,
               "human,organic,armed,ally,targetable");
        g.enemies[index].faction_is_ally = 1;
    } else {
        strcpy(g.enemies[index].faction_name, "hostile");
        strcpy(g.enemies[index].team_name, "monsters");
        strcpy(g.enemies[index].role_name, "attacker");
        strcpy(g.enemies[index].faction_tags,
               "organic,enemy,targetable");
        g.enemies[index].faction_is_ally = 0;
    }
    for (argi = 7; argi + 1 < argc; argi += 2) {
        if (strcmp(args[argi], "loadout") == 0 ||
            strcmp(args[argi], "inventory") == 0) {
            strncpy(g.enemies[index].weapon_loadout, args[argi + 1],
                    sizeof(g.enemies[index].weapon_loadout) - 1U);
            g.enemies[index].weapon_loadout[
                sizeof(g.enemies[index].weapon_loadout) - 1U] = '\0';
        } else if (strcmp(args[argi], "weapon") == 0 ||
                   strcmp(args[argi], "equip") == 0) {
            strncpy(g.enemies[index].requested_weapon, args[argi + 1],
                    sizeof(g.enemies[index].requested_weapon) - 1U);
            g.enemies[index].requested_weapon[
                sizeof(g.enemies[index].requested_weapon) - 1U] = '\0';
        } else if (strcmp(args[argi], "faction") == 0) {
            strncpy(g.enemies[index].faction_name, args[argi + 1],
                    sizeof(g.enemies[index].faction_name) - 1U);
            g.enemies[index].faction_name[
                sizeof(g.enemies[index].faction_name) - 1U] = '\0';
        } else if (strcmp(args[argi], "team") == 0) {
            strncpy(g.enemies[index].team_name, args[argi + 1],
                    sizeof(g.enemies[index].team_name) - 1U);
            g.enemies[index].team_name[
                sizeof(g.enemies[index].team_name) - 1U] = '\0';
        } else if (strcmp(args[argi], "role") == 0) {
            strncpy(g.enemies[index].role_name, args[argi + 1],
                    sizeof(g.enemies[index].role_name) - 1U);
            g.enemies[index].role_name[
                sizeof(g.enemies[index].role_name) - 1U] = '\0';
        } else if (strcmp(args[argi], "tags") == 0) {
            strncpy(g.enemies[index].faction_tags, args[argi + 1],
                    sizeof(g.enemies[index].faction_tags) - 1U);
            g.enemies[index].faction_tags[
                sizeof(g.enemies[index].faction_tags) - 1U] = '\0';
        } else if (strcmp(args[argi], "floor") == 0) {
            (void)parse_fixed(args[argi + 1], &floor_y);
        } else if (strcmp(args[argi], "flyingentity") == 0 ||
                   strcmp(args[argi], "flying") == 0) {
            flying_entity = atoi(args[argi + 1]) != 0;
        } else if (strcmp(args[argi], "gravityimmune") == 0 ||
                   strcmp(args[argi], "nogravity") == 0) {
            gravity_immune = atoi(args[argi + 1]) != 0;
        } else if (strcmp(args[argi], "perceptionini") == 0 ||
                   strcmp(args[argi], "sensorini") == 0 ||
                   strcmp(args[argi], "senses") == 0 ||
                   (strcmp(args[argi], "perception") == 0 &&
                    b3d_is_ini_path(args[argi + 1]))) {
            if (!b3d_enemy_load_perception_ini(&g.enemies[index],
                                                args[argi + 1]))
                set_status("perception INI load failed; defaults retained");
        } else if (strcmp(args[argi], "truthprofile") == 0 ||
                   strcmp(args[argi], "truthgate") == 0 ||
                   strcmp(args[argi], "perception") == 0) {
            (void)blank3d_truth_gate_set_profile_name(
                &g.enemies[index].truth_gate, args[argi + 1]);
        } else if (strcmp(args[argi], "truthrange") == 0) {
            (void)parse_fixed(args[argi + 1],
                              &g.enemies[index].truth_range);
        } else if (strcmp(args[argi], "perceptionrange") == 0 ||
                   strcmp(args[argi], "viewrange") == 0) {
            if (parse_fixed(args[argi + 1],
                            &g.enemies[index].perception_config.view_range))
                g.enemies[index].truth_range =
                    g.enemies[index].perception_config.view_range;
        } else if (strcmp(args[argi], "eyeshape") == 0 ||
                   strcmp(args[argi], "visionshape") == 0) {
            Blank3DPerceptionAgent temporary_agent;
            memset(&temporary_agent, 0, sizeof(temporary_agent));
            temporary_agent.config = g.enemies[index].perception_config;
            if (blank3d_perception_set_eye_shape(&temporary_agent,
                                                  args[argi + 1]))
                g.enemies[index].perception_config.eye_shape =
                    temporary_agent.config.eye_shape;
        } else if (strcmp(args[argi], "viewfov") == 0 ||
                   strcmp(args[argi], "horizontalfov") == 0) {
            g.enemies[index].perception_config.horizontal_fov_degrees =
                atoi(args[argi + 1]);
        } else if (strcmp(args[argi], "verticalfov") == 0) {
            g.enemies[index].perception_config.vertical_fov_degrees =
                atoi(args[argi + 1]);
        } else if (strcmp(args[argi], "hearingrange") == 0) {
            (void)parse_fixed(args[argi + 1],
                &g.enemies[index].perception_config.hearing_range);
        } else if (strcmp(args[argi], "requirelos") == 0 ||
                   strcmp(args[argi], "eyesrequirelos") == 0) {
            g.enemies[index].perception_config.require_line_of_sight =
                atoi(args[argi + 1]) != 0;
        }
    }
    blank3d_vertical_body_init(&g.enemies[index].vertical,
                               &g.enemies[index].transform, y, floor_y);
    blank3d_vertical_body_set_flying(&g.enemies[index].vertical,
                                     flying_entity);
    blank3d_vertical_body_set_gravity_immune(&g.enemies[index].vertical,
                                              gravity_immune);
    if (flying_entity && y > floor_y)
        (void)blank3d_vertical_axis_save_position(&g.vertical_axis,
                                                   &g.enemies[index].vertical);
    (void)blank3d_faction_register_actor(&g.factions,
        g.enemies[index].weapon_actor_id,
        g.enemies[index].faction_name,
        g.enemies[index].team_name,
        g.enemies[index].role_name,
        g.enemies[index].faction_tags,
        g.enemies[index].faction_is_ally ? 70 : 65,
        g.enemies[index].faction_is_ally ? 90 : 55, 1, 1);
    g.enemies[index].faction_is_ally =
        blank3d_faction_are_allies(&g.factions, B3D_PLAYER_ACTOR_ID,
                                   g.enemies[index].weapon_actor_id);
    if (index + 1 > g.enemy_count) g.enemy_count = index + 1;
}

static void b3d_language_rpyl_command(void *user, const char *command,
                                      const char **args, int argc)
{
    g3d_fix a;
    g3d_fix b;
    g3d_fix c;
    (void)user;
    if (!command || !args) return;

    if (strcmp(command, "scene") == 0) return;
    if (strcmp(command, "window") == 0 && argc >= 2) {
        g.width = atoi(args[0]);
        g.height = atoi(args[1]);
        return;
    }
    if (strcmp(command, "show_grid") == 0 && argc >= 1) {
        g.show_grid = atoi(args[0]);
        return;
    }
    if (strcmp(command, "player") == 0 && argc >= 2) {
        if (strcmp(args[0], "pos") == 0 && argc >= 4 &&
            parse_fixed(args[1], &a) && parse_fixed(args[2], &b) &&
            parse_fixed(args[3], &c)) {
            g.player.position = gamlib_vec3(a, b, c);
            blank3d_vertical_body_init(&g.player_vertical, &g.player,
                                       b, g.player_vertical.floor_y);
        } else if (strcmp(args[0], "yaw") == 0 &&
                   parse_fixed(args[1], &a)) {
            set_game_yaw(&g.player, a);
            g.camera_yaw = g.player.rotation.y;
        } else if (strcmp(args[0], "floor") == 0 &&
                   parse_fixed(args[1], &a)) {
            blank3d_vertical_body_set_floor(&g.player_vertical, a);
        } else if (strcmp(args[0], "flyingentity") == 0 ||
                   strcmp(args[0], "flying") == 0) {
            blank3d_vertical_body_set_flying(&g.player_vertical,
                                              atoi(args[1]) != 0);
        } else if (strcmp(args[0], "gravityimmune") == 0 ||
                   strcmp(args[0], "nogravity") == 0) {
            blank3d_vertical_body_set_gravity_immune(
                &g.player_vertical, atoi(args[1]) != 0);
        }
        return;
    }
    if (strcmp(command, "camera") == 0 && argc >= 7 &&
        strcmp(args[1], "dist") == 0 &&
        strcmp(args[3], "height") == 0 &&
        strcmp(args[5], "fov") == 0 &&
        parse_fixed(args[2], &a) && parse_fixed(args[4], &b) &&
        parse_fixed(args[6], &c)) {
        g.camera_mode = strcmp(args[0], "firstperson") == 0
                      ? CAMERA_FPS : CAMERA_TPS;
        g.camera_dist = a;
        g.camera_height = b;
        g.fov = c;
        g.base_fov = c;
        return;
    }
    if (strcmp(command, "movement") == 0 && argc >= 6 &&
        strcmp(args[0], "move") == 0 &&
        strcmp(args[2], "strafe") == 0 &&
        strcmp(args[4], "turn") == 0 &&
        parse_fixed(args[1], &a) && parse_fixed(args[3], &b) &&
        parse_fixed(args[5], &c)) {
        g.move_speed = a;
        g.strafe_speed = b;
        g.turn_speed = c;
        if (argc >= 8 && strcmp(args[6], "vertical") == 0 &&
            parse_fixed(args[7], &a))
            g.vertical_speed = a;
        return;
    }
    if ((strcmp(command, "npc_weapon") == 0 ||
         strcmp(command, "npc_inventory") == 0) && argc >= 2) {
        int npc_index;
        npc_index = atoi(args[0]);
        if (npc_index < 0 || npc_index >= MAX_ENEMIES ||
            !g.enemies[npc_index].alive) return;
        if (strcmp(args[1], "equip") == 0 && argc >= 3)
            (void)b3d_enemy_equip_weapon_text(&g.enemies[npc_index], args[2]);
        else if (strcmp(args[1], "give") == 0 && argc >= 3)
            (void)b3d_enemy_give_weapon_text(&g.enemies[npc_index], args[2]);
        else if (strcmp(args[1], "next") == 0) {
            if (b3d_enemy_ensure_weapon_inventory(&g.enemies[npc_index]))
                (void)blank3d_npc_inventory_cycle_next(
                    &g.npc_inventory, &g.npc_weapons,
                    g.enemies[npc_index].weapon_actor_id);
        } else if (strcmp(args[1], "prev") == 0) {
            if (b3d_enemy_ensure_weapon_inventory(&g.enemies[npc_index]))
                (void)blank3d_npc_inventory_cycle_prev(
                    &g.npc_inventory, &g.npc_weapons,
                    g.enemies[npc_index].weapon_actor_id);
        } else if (strcmp(args[1], "reload") == 0) {
            if (b3d_enemy_ensure_weapon_inventory(&g.enemies[npc_index]))
                (void)gwp89_begin_reload(&g.npc_weapons,
                    g.enemies[npc_index].weapon_actor_id);
        }
        return;
    }
    if (strcmp(command, "zombie") == 0) {
        b3d_rpyl_define_entity("zombie", args, argc);
        return;
    }
    if (strcmp(command, "gunner_enemy") == 0) {
        b3d_rpyl_define_entity("gunner_enemy", args, argc);
        return;
    }
    if (strcmp(command, "armed_ally") == 0 ||
        strcmp(command, "ally") == 0 ||
        strcmp(command, "ally_gunner") == 0) {
        b3d_rpyl_define_entity("armed_ally", args, argc);
        return;
    }
    if (strcmp(command, "hopper_enemy") == 0) {
        b3d_rpyl_define_entity("hopper_enemy", args, argc);
        return;
    }
    if (strcmp(command, "dive_enemy") == 0 ||
        strcmp(command, "dive_bomber_enemy") == 0) {
        b3d_rpyl_define_entity("dive_enemy", args, argc);
        return;
    }
    if (strcmp(command, "air_lunger_enemy") == 0 ||
        strcmp(command, "airlunge_enemy") == 0 ||
        strcmp(command, "midair_lunger_enemy") == 0) {
        b3d_rpyl_define_entity("air_lunger_enemy", args, argc);
        return;
    }
    if (strcmp(command, "ground_lancer_enemy") == 0 ||
        strcmp(command, "groundlance_enemy") == 0 ||
        strcmp(command, "pegasus_enemy") == 0) {
        b3d_rpyl_define_entity("ground_lancer_enemy", args, argc);
        return;
    }
    if (strcmp(command, "enemy") == 0) {
        /* Legacy alias: preserve old scenes as ordinary zombies. */
        b3d_rpyl_define_entity("zombie", args, argc);
        return;
    }
}

static g3d_fix b3d_fpi_q16_to_q12(long value_q16)
{
    return (g3d_fix)(value_q16 / 16L);
}

static int b3d_language_fpil_condition(void *user, void *entity,
                                       const char *condition,
                                       long value_q16,
                                       const char *value_text,
                                       int has_value)
{
    Enemy *enemy;
    Vec3 target_position;
    Vec3 delta;
    Vec3 flat_delta;
    g3d_fix distance;
    g3d_fix flat_distance;
    int number;
    int weapon_id;
    int tier;
    unsigned long source_mask;
    const Blank3DPerceptionAgent *perception;
    (void)user;
    enemy = (Enemy *)entity;
    if (!enemy || !condition) return 0;
    perception = b3d_enemy_perception_const(enemy);
    if (perception && perception->target_known)
        target_position = perception->absolute_target_position;
    else if (!b3d_enemy_target_position(enemy, &target_position))
        target_position = enemy->transform.position;
    gamlib_vec3_sub(&delta, &target_position, &enemy->transform.position);
    distance = perception && perception->target_known
             ? perception->target_distance : gamlib_vec3_length(&delta);
    flat_delta = delta;
    flat_delta.y = 0;
    flat_distance = gamlib_vec3_length(&flat_delta);
    number = has_value ? (int)(value_q16 / 65536L) : 0;
    if (strcmp(condition, "always") == 0) return 1;
    if (strcmp(condition, "truthaccepted") == 0)
        return blank3d_truth_gate_is_accepted(&enemy->truth_gate);
    if (strcmp(condition, "playeracceptedastruth") == 0 ||
        strcmp(condition, "targetacceptedastruth") == 0 ||
        strcmp(condition, "socketertruth") == 0 ||
        strcmp(condition, "targetknown") == 0 ||
        strcmp(condition, "hastarget") == 0)
        return perception ? perception->target_known : 1;
    if (strcmp(condition, "canperceiveplayer") == 0 ||
        strcmp(condition, "targetvisible") == 0 ||
        strcmp(condition, "targetcanbeseen") == 0 ||
        strcmp(condition, "eyessee") == 0)
        return perception && (perception->target_visible ||
                              perception->target_partial);
    if (strcmp(condition, "targetfullyvisible") == 0)
        return perception && perception->target_visible;
    if (strcmp(condition, "targetpartial") == 0 ||
        strcmp(condition, "targetpartiallyvisible") == 0)
        return perception && perception->target_partial;
    if (strcmp(condition, "targetoccluded") == 0)
        return perception && perception->target_occluded;
    if (strcmp(condition, "targetoutofrange") == 0)
        return perception && perception->target_out_of_range;
    if (strcmp(condition, "targetoutofshape") == 0)
        return perception && perception->target_out_of_shape;
    if (strcmp(condition, "targetheard") == 0 ||
        strcmp(condition, "noiseheard") == 0)
        return perception && perception->target_heard;
    if (strcmp(condition, "targetremembered") == 0)
        return perception && perception->target_remembered;
    if (strcmp(condition, "targetinferred") == 0 ||
        strcmp(condition, "targetenlightened") == 0)
        return perception && perception->target_inferred;
    if (strcmp(condition, "truthfallbackactive") == 0 ||
        strcmp(condition, "socketerfallback") == 0)
        return perception && perception->fallback_active;
    if (strcmp(condition, "targetalive") == 0)
        return perception ? perception->target_alive :
               b3d_enemy_target_alive(enemy);
    if (strcmp(condition, "targethostile") == 0 ||
        strcmp(condition, "canattacktarget") == 0)
        return b3d_faction_can_attack_actor(enemy->weapon_actor_id,
                                             enemy->target_actor_id);
    if (strcmp(condition, "targetisplayer") == 0)
        return enemy->target_actor_id == B3D_PLAYER_ACTOR_ID;
    if (strcmp(condition, "selfisally") == 0)
        return enemy->faction_is_ally;
    if (strcmp(condition, "targetlostfor") == 0 && has_value)
        return perception && perception->lost_time >=
               b3d_fpi_q16_to_q12(value_q16);
    if (strcmp(condition, "targetseenfor") == 0 && has_value)
        return perception && perception->visible_time >=
               b3d_fpi_q16_to_q12(value_q16);
    if (strcmp(condition, "alertnessatleast") == 0 && has_value)
        return perception && perception->alertness_percent >= number;
    if (strcmp(condition, "suspicionatleast") == 0 && has_value)
        return perception && perception->suspicion_percent >= number;
    if ((strcmp(condition, "targetconfidenceatleast") == 0 ||
         strcmp(condition, "truthconfidenceatleast") == 0) && has_value)
        return perception && perception->action_confidence >= number * 10;
    if (strcmp(condition, "targetraysclearatleast") == 0 && has_value)
        return perception && perception->eye_result.rays_clear >= number;
    if (strcmp(condition, "truthlevelatleast") == 0 && has_value) {
        tier = blank3d_perception_truth_tier_from_name(value_text);
        if (tier == B3D_TRUTH_TIER_NONE) tier = number;
        return perception && perception->truth_tier >= tier;
    }
    if (strcmp(condition, "truthsourcehas") == 0 && has_value) {
        source_mask = 0UL;
        if (strcmp(value_text, "socketer") == 0 ||
            strcmp(value_text, "socket") == 0)
            source_mask = B3D_TRUTH_SOURCE_SOCKETER;
        else if (strcmp(value_text, "eyes") == 0 ||
                 strcmp(value_text, "visual") == 0)
            source_mask = B3D_TRUTH_SOURCE_EYES;
        else if (strcmp(value_text, "enlightener") == 0 ||
                 strcmp(value_text, "interpreted") == 0)
            source_mask = B3D_TRUTH_SOURCE_ENLIGHTENER;
        else if (strcmp(value_text, "hearing") == 0 ||
                 strcmp(value_text, "sound") == 0)
            source_mask = B3D_TRUTH_SOURCE_HEARING;
        else if (strcmp(value_text, "memory") == 0)
            source_mask = B3D_TRUTH_SOURCE_MEMORY;
        return perception && source_mask != 0UL &&
               blank3d_perception_has_source(perception, source_mask);
    }
    if (strcmp(condition, "truthrejected") == 0)
        return !blank3d_truth_gate_is_accepted(&enemy->truth_gate);
    if ((strcmp(condition, "truthcountatleast") == 0 ||
         strcmp(condition, "truthsatleast") == 0) && has_value)
        return blank3d_truth_gate_truth_count(&enemy->truth_gate) >=
               (unsigned int)(number < 0 ? 0 : number);
    if (strcmp(condition, "truthscoreatleast") == 0 && has_value)
        return blank3d_truth_gate_score_floor(&enemy->truth_gate) >=
               (long)number;
    if (strcmp(condition, "truthreason") == 0 && has_value)
        return blank3d_truth_gate_reason(&enemy->truth_gate) ==
               (long)number;
    if (strcmp(condition, "truthprofileis") == 0 && has_value)
        return strcmp(blank3d_truth_gate_profile_name(
                   enemy->truth_gate.profile), value_text) == 0;
    if (strcmp(condition, "state") == 0) return enemy->state == number;
    if (strcmp(condition, "plrdistwithin") == 0 ||
        strcmp(condition, "targetdistwithin") == 0)
        return distance <= b3d_fpi_q16_to_q12(value_q16);
    if (strcmp(condition, "plrdistfurther") == 0 ||
        strcmp(condition, "targetdistfurther") == 0)
        return distance > b3d_fpi_q16_to_q12(value_q16);
    if ((strcmp(condition, "plrflatdistwithin") == 0 ||
         strcmp(condition, "playerflatdistwithin") == 0 ||
         strcmp(condition, "targetflatdistwithin") == 0) && has_value)
        return flat_distance <= b3d_fpi_q16_to_q12(value_q16);
    if ((strcmp(condition, "plrflatdistfurther") == 0 ||
         strcmp(condition, "playerflatdistfurther") == 0 ||
         strcmp(condition, "targetflatdistfurther") == 0) && has_value)
        return flat_distance > b3d_fpi_q16_to_q12(value_q16);
    if (strcmp(condition, "grounded") == 0 ||
        strcmp(condition, "onground") == 0)
        return blank3d_vertical_axis_grounded(&g.vertical_axis,
                                               &enemy->vertical);
    if (strcmp(condition, "airborne") == 0 ||
        strcmp(condition, "offground") == 0)
        return !blank3d_vertical_axis_grounded(&g.vertical_axis,
                                                &enemy->vertical);
    if (strcmp(condition, "jumping") == 0 ||
        strcmp(condition, "leaping") == 0)
        return blank3d_vertical_axis_jumping(&enemy->vertical);
    if (strcmp(condition, "falling") == 0)
        return blank3d_vertical_axis_falling(&enemy->vertical);
    if (strcmp(condition, "flyingentity") == 0 ||
        strcmp(condition, "isflying") == 0)
        return enemy->vertical.flying_entity != 0;
    if ((strcmp(condition, "heightbelow") == 0 ||
         strcmp(condition, "verticalbelow") == 0) && has_value)
        return blank3d_vertical_axis_height(&g.vertical_axis,
                                             &enemy->vertical) <
               b3d_fpi_q16_to_q12(value_q16);
    if ((strcmp(condition, "heightatleast") == 0 ||
         strcmp(condition, "verticalatleast") == 0) && has_value)
        return blank3d_vertical_axis_height(&g.vertical_axis,
                                             &enemy->vertical) >=
               b3d_fpi_q16_to_q12(value_q16);
    if ((strcmp(condition, "belowplayerheight") == 0 ||
         strcmp(condition, "belowplayeroffset") == 0) && has_value)
        return enemy->transform.position.y <
               g3d_fix_add_sat(target_position.y,
                   b3d_fpi_q16_to_q12(value_q16));
    if ((strcmp(condition, "aboveplayerheight") == 0 ||
         strcmp(condition, "aboveplayeroffset") == 0) && has_value)
        return enemy->transform.position.y >
               g3d_fix_add_sat(target_position.y,
                   b3d_fpi_q16_to_q12(value_q16));
    if ((strcmp(condition, "heightaboveplayeratleast") == 0 ||
         strcmp(condition, "playerbelowby") == 0) && has_value)
        return g3d_fix_sub_sat(enemy->transform.position.y,
                   target_position.y) >=
               b3d_fpi_q16_to_q12(value_q16);
    if ((strcmp(condition, "returnpositionwithin") == 0 ||
         strcmp(condition, "homewithin") == 0) && has_value) {
        Vec3 saved_position;
        if (!blank3d_vertical_axis_saved_position(&enemy->vertical,
                                                   &saved_position)) return 0;
        gamlib_vec3_sub(&delta, &saved_position,
                        &enemy->transform.position);
        return gamlib_vec3_length(&delta) <=
               b3d_fpi_q16_to_q12(value_q16);
    }
    if ((strcmp(condition, "returnpositionfurther") == 0 ||
         strcmp(condition, "homefurther") == 0) && has_value) {
        Vec3 saved_position;
        if (!blank3d_vertical_axis_saved_position(&enemy->vertical,
                                                   &saved_position)) return 0;
        gamlib_vec3_sub(&delta, &saved_position,
                        &enemy->transform.position);
        return gamlib_vec3_length(&delta) >
               b3d_fpi_q16_to_q12(value_q16);
    }
    if (strcmp(condition, "motionattackactive") == 0 ||
        strcmp(condition, "attackmotionactive") == 0)
        return blank3d_motion_attack_is_active(&enemy->motion_attack);
    if (strcmp(condition, "airlungeactive") == 0 ||
        strcmp(condition, "midairlungeactive") == 0)
        return blank3d_motion_attack_mode(&enemy->motion_attack) ==
                   B3D_MOTION_ATTACK_AIR_LUNGE &&
               blank3d_motion_attack_is_active(&enemy->motion_attack);
    if (strcmp(condition, "groundlanceactive") == 0 ||
        strcmp(condition, "groundchargeactive") == 0)
        return blank3d_motion_attack_mode(&enemy->motion_attack) ==
                   B3D_MOTION_ATTACK_GROUND_LANCE &&
               blank3d_motion_attack_is_active(&enemy->motion_attack);
    if (strcmp(condition, "motionattackimpact") == 0 ||
        strcmp(condition, "attackmotionimpact") == 0 ||
        strcmp(condition, "attackimpact") == 0 ||
        strcmp(condition, "attackhit") == 0)
        return blank3d_motion_attack_has_impact(&enemy->motion_attack);
    if (strcmp(condition, "motionattackfinished") == 0 ||
        strcmp(condition, "attackmotionfinished") == 0 ||
        strcmp(condition, "attackfinished") == 0 ||
        strcmp(condition, "attackdone") == 0)
        return blank3d_motion_attack_finished(&enemy->motion_attack);
    if ((strcmp(condition, "weaponis") == 0 ||
         strcmp(condition, "usingweapon") == 0) && has_value) {
        if (!b3d_enemy_ensure_weapon_inventory(enemy)) return 0;
        weapon_id = b3d_weapon_id_from_text(value_text);
        return weapon_id > 0 &&
            blank3d_npc_inventory_weapon_id(&g.npc_inventory,
                enemy->weapon_actor_id) == weapon_id;
    }
    if (strcmp(condition, "hasweapon") == 0 && has_value) {
        if (!b3d_enemy_ensure_weapon_inventory(enemy)) return 0;
        return blank3d_npc_inventory_has_name(&g.npc_inventory,
            &g.npc_weapons, enemy->weapon_actor_id, value_text);
    }
    return 0;
}

static void b3d_reset_npc_weapon_manager(void)
{
    /* Clone immutable profiles/providers from the player manager, then clear
       every actor-local and queued field. The NPC inventory provider supplies
       ownership, reserve ammunition and one persistent clip per weapon. */
    g.npc_weapons = g.systems.weapons;
    memset(g.npc_weapons.users, 0, sizeof(g.npc_weapons.users));
    memset(g.npc_weapons.ammo_bank, 0, sizeof(g.npc_weapons.ammo_bank));
    memset(g.npc_weapons.events, 0, sizeof(g.npc_weapons.events));
    g.npc_weapons.event_head = 0;
    g.npc_weapons.event_tail = 0;
    g.npc_weapons.event_count = 0;
    g.npc_weapons.status[0] = '\0';
    blank3d_npc_inventory_bank_init(&g.npc_inventory);
    (void)gwp89_add_provider(&g.npc_weapons, GWP89_SERVICE_INVENTORY,
        180, "blank3d.npc-inventory", &g.npc_inventory,
        blank3d_npc_inventory_provider);
}

static int b3d_poll_weapon_event(GWP89_Event *event_out)
{
    if (!event_out) return 0;
    if (blank3d_systems_poll_event(&g.systems, event_out)) return 1;
    return gwp89_poll_event(&g.npc_weapons, event_out);
}

static int b3d_weapon_id_from_text(const char *weapon_text)
{
    int slot;
    int weapon_id;
    const GWP89_WeaponProfile *profile;
    if (!weapon_text || !*weapon_text) return 0;
    weapon_id = atoi(weapon_text);
    if (weapon_id > 0 &&
        gwp89_find_weapon_slot_by_id(&g.npc_weapons, weapon_id) >= 0)
        return weapon_id;
    slot = gwp89_find_weapon_slot(&g.npc_weapons, weapon_text);
    if (slot < 0) return 0;
    profile = gwp89_get_weapon(&g.npc_weapons, slot);
    return profile ? profile->weapon_id : 0;
}

static int b3d_enemy_ensure_weapon_inventory(Enemy *enemy)
{
    char path[192];
    char status[160];
    int index;
    int slot;
    int weapon_id;
    const GWP89_WeaponProfile *profile;
    if (!enemy || !enemy->alive) return 0;
    if (enemy->weapon_ready) return 1;
    index = (int)(enemy - g.enemies);
    if (index < 0 || index >= MAX_ENEMIES) return 0;
    enemy->weapon_actor_id = 1000 + index;
    if (gwp89_bind_actor(&g.npc_weapons, enemy->weapon_actor_id,
                         2, 2) < 0)
        return 0;

    if (enemy->weapon_loadout[0] != '\0') {
        strncpy(path, enemy->weapon_loadout, sizeof(path) - 1U);
        path[sizeof(path) - 1U] = '\0';
    } else {
        sprintf(path, "config/npc_loadouts/%s.ini",
                enemy->archetype[0] ? enemy->archetype : "enemy");
    }
    status[0] = '\0';
    if (!blank3d_npc_inventory_load_ini(&g.npc_inventory,
                                        &g.npc_weapons,
                                        enemy->weapon_actor_id,
                                        path, status, sizeof(status))) {
        /* Compatibility fallback for old scenes: own one registered weapon,
           never a hard-coded implementation. Prefer machine_gun only as a
           data-name default, then fall back to profile slot zero. */
        slot = gwp89_find_weapon_slot(&g.npc_weapons, "machine_gun");
        if (slot < 0 && g.npc_weapons.weapon_count > 0) slot = 0;
        profile = slot < 0 ? 0 : gwp89_get_weapon(&g.npc_weapons, slot);
        if (!profile) return 0;
        weapon_id = profile->weapon_id;
        if (blank3d_npc_inventory_give_id(&g.npc_inventory,
                &g.npc_weapons, enemy->weapon_actor_id,
                weapon_id, 1) != GWP89_OK)
            return 0;
        (void)blank3d_npc_inventory_set_ammo(&g.npc_inventory,
                enemy->weapon_actor_id, profile->ammo_id, 9999);
        if (blank3d_npc_inventory_equip_id(&g.npc_inventory,
                &g.npc_weapons, enemy->weapon_actor_id,
                weapon_id) != GWP89_OK)
            return 0;
    }

    /* A spawn-level `weapon NAME` is an explicit assignment. It may add the
       weapon even when the selected loadout did not already contain it. */
    if (enemy->requested_weapon[0] != '\0') {
        if (blank3d_npc_inventory_give_name(&g.npc_inventory,
                &g.npc_weapons, enemy->weapon_actor_id,
                enemy->requested_weapon, 1) == GWP89_OK) {
            weapon_id = b3d_weapon_id_from_text(enemy->requested_weapon);
            profile = weapon_id > 0
                    ? gwp89_get_weapon(&g.npc_weapons,
                        gwp89_find_weapon_slot_by_id(&g.npc_weapons,
                                                     weapon_id))
                    : 0;
            if (profile && blank3d_npc_inventory_ammo(&g.npc_inventory,
                    enemy->weapon_actor_id, profile->ammo_id) <= 0)
                (void)blank3d_npc_inventory_set_ammo(&g.npc_inventory,
                    enemy->weapon_actor_id, profile->ammo_id, 9999);
            (void)blank3d_npc_inventory_equip_name(&g.npc_inventory,
                &g.npc_weapons, enemy->weapon_actor_id,
                enemy->requested_weapon);
        }
    }
    enemy->weapon_ready = 1;
    return 1;
}

static int b3d_enemy_equip_weapon_text(Enemy *enemy, const char *weapon_text)
{
    if (!enemy || !weapon_text || !*weapon_text) return GWP89_BAD_ARG;
    if (!b3d_enemy_ensure_weapon_inventory(enemy)) return GWP89_NOT_FOUND;
    return blank3d_npc_inventory_equip_name(&g.npc_inventory,
        &g.npc_weapons, enemy->weapon_actor_id, weapon_text);
}

static int b3d_enemy_give_weapon_text(Enemy *enemy, const char *weapon_text)
{
    int result;
    int weapon_id;
    int slot;
    const GWP89_WeaponProfile *profile;
    if (!enemy || !weapon_text || !*weapon_text) return GWP89_BAD_ARG;
    if (!b3d_enemy_ensure_weapon_inventory(enemy)) return GWP89_NOT_FOUND;
    result = blank3d_npc_inventory_give_name(&g.npc_inventory,
        &g.npc_weapons, enemy->weapon_actor_id, weapon_text, 1);
    if (result != GWP89_OK) return result;
    weapon_id = b3d_weapon_id_from_text(weapon_text);
    slot = gwp89_find_weapon_slot_by_id(&g.npc_weapons, weapon_id);
    profile = slot < 0 ? 0 : gwp89_get_weapon(&g.npc_weapons, slot);
    if (profile && blank3d_npc_inventory_ammo(&g.npc_inventory,
            enemy->weapon_actor_id, profile->ammo_id) <= 0)
        (void)blank3d_npc_inventory_set_ammo(&g.npc_inventory,
            enemy->weapon_actor_id, profile->ammo_id, 9999);
    return GWP89_OK;
}

static void b3d_enemy_face_position(Enemy *enemy,
                                    const Vec3 *position)
{
    Vec3 delta;
    if (!enemy || !position) return;
    gamlib_vec3_sub(&delta, position, &enemy->transform.position);
    delta.y = 0;
    if (gamlib_vec3_length(&delta) <= G3D_FIX_EPSILON) return;
    set_game_yaw(&enemy->transform, fixed_atan2_xz(delta));
}

static void b3d_enemy_move_toward_position(Enemy *enemy,
                                           const Vec3 *position,
                                           g3d_fix speed)
{
    Vec3 delta;
    Vec3 direction;
    Vec3 movement;
    g3d_fix step;
    if (!enemy || !position || speed <= 0) return;
    gamlib_vec3_sub(&delta, position, &enemy->transform.position);
    delta.y = 0;
    if (gamlib_vec3_length(&delta) <= G3D_FIX_EPSILON) return;
    gamlib_vec3_normalize(&direction, &delta);
    step = g3d_fix_mul(speed, g.dt);
    gamlib_vec3_scale(&movement, &direction, step);
    gamlib_vec3_add(&enemy->transform.position,
                    &enemy->transform.position, &movement);
}

static void b3d_enemy_fire_weapon(Enemy *enemy)
{
    GWP89_FireInput input;
    Vec3 target;
    Vec3 to_player;
    Vec3 direction;
    Vec3 right;
    Vec3 up;
    Vec3 world_up;
    Vec3 muzzle;
    Vec3 muzzle_offset;
    g3d_fix clearance;
    int user_slot;
    int fire_result;
    const GWP89_UserState *user_state;
    const GWP89_WeaponProfile *profile;
    if (!enemy || !enemy->alive) return;
    if (!b3d_faction_can_attack_actor(enemy->weapon_actor_id,
                                       enemy->target_actor_id)) return;
    if (!b3d_enemy_ensure_weapon_inventory(enemy)) return;
    user_slot = gwp89_find_user_slot(&g.npc_weapons,
                                     enemy->weapon_actor_id);
    if (user_slot < 0) return;
    user_state = &g.npc_weapons.users[user_slot];
    profile = gwp89_get_weapon(&g.npc_weapons, user_state->weapon_slot);
    if (!profile) return;

    /* Aim at the best target truth (visible, remembered, heard or Socketer
       fallback), then derive direction from the actual muzzle position. */
    if (!b3d_enemy_target_position(enemy, &target)) return;
    target.y = g3d_fix_add_sat(target.y, fix_ratio(9, 10));
    gamlib_vec3_sub(&to_player, &target, &enemy->transform.position);
    if (gamlib_vec3_length(&to_player) <= G3D_FIX_EPSILON) return;
    gamlib_vec3_normalize(&direction, &to_player);

    {
        const nm89_matrix *equipped_muzzle;
        equipped_muzzle = blank3d_actor_equipment_muzzle_world(
            &g.actor_equipment, enemy->weapon_actor_id);
        if (equipped_muzzle != 0) {
            muzzle.x = (g3d_fix)(equipped_muzzle->m[0][3] / 16L);
            muzzle.y = (g3d_fix)(equipped_muzzle->m[1][3] / 16L);
            muzzle.z = (g3d_fix)(equipped_muzzle->m[2][3] / 16L);
        } else {
            muzzle = enemy->transform.position;
            muzzle_offset = gamlib_vec3(0, fix_ratio(6, 5), 0);
            gamlib_vec3_add(&muzzle, &muzzle, &muzzle_offset);
            clearance = g3d_fix_add_sat(fix_ratio(3, 4),
                                        (g3d_fix)profile->projectile_radius_fx);
            if (clearance < fix_ratio(11, 10))
                clearance = fix_ratio(11, 10);
            gamlib_vec3_scale(&muzzle_offset, &direction, clearance);
            gamlib_vec3_add(&muzzle, &muzzle, &muzzle_offset);
        }
    }

    gamlib_vec3_sub(&to_player, &target, &muzzle);
    if (gamlib_vec3_length(&to_player) <= G3D_FIX_EPSILON) return;
    gamlib_vec3_normalize(&direction, &to_player);

    world_up = gamlib_vec3(0, G3D_FIX_ONE, 0);
    gamlib_vec3_cross(&right, &world_up, &direction);
    if (gamlib_vec3_length(&right) <= G3D_FIX_EPSILON)
        right = gamlib_vec3(G3D_FIX_ONE, 0, 0);
    else
        gamlib_vec3_normalize(&right, &right);
    gamlib_vec3_cross(&up, &direction, &right);
    gamlib_vec3_normalize(&up, &up);

    memset(&input, 0, sizeof(input));
    input.actor_id = enemy->weapon_actor_id;
    input.actor_kind = enemy->faction_is_ally
                     ? B3D_EQUIPMENT_ACTOR_ALLY
                     : B3D_EQUIPMENT_ACTOR_ENEMY;
    {
        const GFA_Entity *faction_entity;
        faction_entity = gfa_get_entity_const(&g.factions.world,
                                               enemy->weapon_actor_id);
        input.team_id = faction_entity ? faction_entity->team_id : 0;
    }
    input.view_style = GWP89_VIEW_FPS;
    if (profile->fire_mode == GWP89_FIRE_AUTO)
        input.trigger_flags = GWP89_TRIGGER_DOWN;
    else if (profile->fire_mode == GWP89_FIRE_BURST)
        input.trigger_flags = GWP89_TRIGGER_DOWN | GWP89_TRIGGER_PRESSED;
    else if (profile->fire_mode == GWP89_FIRE_HOLD_ONCE)
        input.trigger_flags = GWP89_TRIGGER_DOWN | GWP89_TRIGGER_PRESSED;
    else
        input.trigger_flags = GWP89_TRIGGER_PRESSED;
    input.dt_ms = g.frame_ms;
    input.zoom_fx = GWP89_FIX_ONE;
    input.socket_origin = vec3_to_gwp(muzzle);
    input.socket_forward = vec3_to_gwp(direction);
    input.socket_right = vec3_to_gwp(right);
    input.socket_up = vec3_to_gwp(up);
    input.camera_origin = input.socket_origin;
    input.camera_forward = input.socket_forward;

    fire_result = gwp89_try_fire(&g.npc_weapons, &input);
    if (profile->fire_mode == GWP89_FIRE_HOLD_ONCE) {
        input.trigger_flags = GWP89_TRIGGER_RELEASED;
        input.dt_ms = 0U;
        (void)gwp89_update_actor(&g.npc_weapons, &input);
    }
    if (fire_result == GWP89_NO_AMMO)
        (void)gwp89_begin_reload(&g.npc_weapons,
                                 enemy->weapon_actor_id);
}

static void b3d_language_fpil_action(void *user, void *entity,
                                     const char *action,
                                     long value_q16,
                                     const char *value_text,
                                     int has_value)
{
    Enemy *enemy;
    Vec3 to_player;
    Vec3 direction;
    Vec3 movement;
    Vec3 target;
    Vec3 automotion_position;
    Vec3 fallback_direction;
    g3d_fix speed;
    g3d_fix step;
    int number;
    int automotion_reached;
    Blank3DPerceptionAgent *perception;
    (void)user;
    enemy = (Enemy *)entity;
    if (!enemy || !action) return;
    perception = b3d_enemy_perception(enemy);
    number = has_value ? (int)(value_q16 / 65536L) : 0;
    if (strcmp(action, "setstate") == 0 || strcmp(action, "state") == 0) {
        enemy->state = number;
        return;
    }
    if (strcmp(action, "rotatetotarget") == 0 ||
        strcmp(action, "lookattarget") == 0 ||
        strcmp(action, "rotatetoplr") == 0 ||
        strcmp(action, "lookatplayer") == 0) {
        if (b3d_enemy_target_position(enemy, &target))
            b3d_enemy_face_position(enemy, &target);
        return;
    }
    if (strcmp(action, "rotatetolastseen") == 0) {
        if (perception && perception->target_remembered)
            b3d_enemy_face_position(enemy,
                                    &perception->last_seen_position);
        return;
    }
    if (strcmp(action, "rotatetolastsound") == 0 ||
        strcmp(action, "rotatetosound") == 0) {
        if (perception && perception->target_heard)
            b3d_enemy_face_position(enemy,
                                    &perception->last_heard_position);
        return;
    }
    if (strcmp(action, "settarget") == 0 && has_value) {
        if (strcmp(value_text, "player") == 0 ||
            strcmp(value_text, "plr") == 0) {
            if (perception)
                blank3d_perception_set_target_key(
                    perception, g.player_key,
                    g.perception.player_entity_id);
        }
        return;
    }
    if (strcmp(action, "cleartarget") == 0) {
        if (perception) {
            perception->target_key = 0UL;
            perception->target_known = 0;
            perception->truth_sources = 0UL;
            perception->truth_tier = B3D_TRUTH_TIER_NONE;
        }
        return;
    }
    if ((strcmp(action, "seteyeshape") == 0 ||
         strcmp(action, "setvisionshape") == 0) && has_value) {
        if (perception &&
            blank3d_perception_set_eye_shape(perception, value_text))
            enemy->perception_config.eye_shape =
                perception->config.eye_shape;
        return;
    }
    if ((strcmp(action, "setviewrange") == 0 ||
         strcmp(action, "setperceptionrange") == 0) && has_value) {
        speed = b3d_fpi_q16_to_q12(value_q16);
        if (perception) blank3d_perception_set_view_range(perception, speed);
        enemy->perception_config.view_range = speed;
        enemy->truth_range = speed;
        return;
    }
    if (strcmp(action, "setviewfov") == 0 && has_value) {
        if (perception)
            blank3d_perception_set_fov(perception, number,
                perception->config.vertical_fov_degrees);
        enemy->perception_config.horizontal_fov_degrees = number;
        return;
    }
    if (strcmp(action, "setverticalfov") == 0 && has_value) {
        if (perception)
            blank3d_perception_set_fov(perception,
                perception->config.horizontal_fov_degrees, number);
        enemy->perception_config.vertical_fov_degrees = number;
        return;
    }
    if (strcmp(action, "sethearingrange") == 0 && has_value) {
        speed = b3d_fpi_q16_to_q12(value_q16);
        if (perception)
            blank3d_perception_set_hearing_range(perception, speed);
        enemy->perception_config.hearing_range = speed;
        return;
    }
    if (strcmp(action, "setrequirelos") == 0 && has_value) {
        if (perception)
            blank3d_perception_set_line_of_sight(perception, number != 0);
        enemy->perception_config.require_line_of_sight = number != 0;
        return;
    }
    if ((strcmp(action, "equipweapon") == 0 ||
         strcmp(action, "useweapon") == 0 ||
         strcmp(action, "selectweapon") == 0) && has_value) {
        (void)b3d_enemy_equip_weapon_text(enemy, value_text);
        return;
    }
    if (strcmp(action, "giveweapon") == 0 && has_value) {
        (void)b3d_enemy_give_weapon_text(enemy, value_text);
        return;
    }
    if (strcmp(action, "nextweapon") == 0 ||
        strcmp(action, "cyclenextweapon") == 0) {
        if (b3d_enemy_ensure_weapon_inventory(enemy))
            (void)blank3d_npc_inventory_cycle_next(&g.npc_inventory,
                &g.npc_weapons, enemy->weapon_actor_id);
        return;
    }
    if (strcmp(action, "prevweapon") == 0 ||
        strcmp(action, "cycleprevweapon") == 0) {
        if (b3d_enemy_ensure_weapon_inventory(enemy))
            (void)blank3d_npc_inventory_cycle_prev(&g.npc_inventory,
                &g.npc_weapons, enemy->weapon_actor_id);
        return;
    }
    if (strcmp(action, "reloadweapon") == 0 ||
        strcmp(action, "reload") == 0) {
        if (b3d_enemy_ensure_weapon_inventory(enemy))
            (void)gwp89_begin_reload(&g.npc_weapons,
                                     enemy->weapon_actor_id);
        return;
    }
    if (strcmp(action, "firetarget") == 0 ||
        strcmp(action, "shoottarget") == 0 ||
        strcmp(action, "fireplayer") == 0 ||
        strcmp(action, "shootplayer") == 0) {
        b3d_enemy_fire_weapon(enemy);
        return;
    }
    if ((strcmp(action, "motionstyle") == 0 ||
         strcmp(action, "movementstyle") == 0 ||
         strcmp(action, "automotionstyle") == 0) && has_value) {
        (void)blank3d_automotion_set_style(&enemy->automotion,
                                           value_text);
        return;
    }
    if ((strcmp(action, "airlungeintent") == 0 ||
         strcmp(action, "midairlungeintent") == 0) && has_value) {
        (void)blank3d_motion_attack_set_air_intent(
            &enemy->motion_attack, value_text);
        return;
    }
    if ((strcmp(action, "airlungetarget") == 0 ||
         strcmp(action, "airlungetargetpolicy") == 0) && has_value) {
        (void)blank3d_motion_attack_set_air_target_policy(
            &enemy->motion_attack, value_text);
        return;
    }
    if (strcmp(action, "airlungesteering") == 0 && has_value) {
        blank3d_motion_attack_set_air_steering(&enemy->motion_attack,
            b3d_fpi_q16_to_q12(value_q16));
        return;
    }
    if ((strcmp(action, "airlungevelocitykeep") == 0 ||
         strcmp(action, "airlungekeepvelocity") == 0) && has_value) {
        blank3d_motion_attack_set_air_velocity_keep(&enemy->motion_attack,
            b3d_fpi_q16_to_q12(value_q16));
        return;
    }
    if (strcmp(action, "airlungeimpulse") == 0 && has_value) {
        blank3d_motion_attack_set_air_impulse(&enemy->motion_attack,
            b3d_fpi_q16_to_q12(value_q16));
        return;
    }
    if (strcmp(action, "airlungepiercecontacts") == 0 && has_value) {
        blank3d_motion_attack_set_air_pierce_contacts(
            &enemy->motion_attack, number < 0 ? 0U : (unsigned int)number);
        return;
    }
    if ((strcmp(action, "groundlancestyle") == 0 ||
         strcmp(action, "groundchargestyle") == 0) && has_value) {
        (void)blank3d_motion_attack_set_ground_style(
            &enemy->motion_attack, value_text);
        return;
    }
    if ((strcmp(action, "groundlancecontact") == 0 ||
         strcmp(action, "groundchargecontact") == 0) && has_value) {
        (void)blank3d_motion_attack_set_ground_contact(
            &enemy->motion_attack, value_text);
        return;
    }
    if (strcmp(action, "groundlancetrack") == 0 && has_value) {
        blank3d_motion_attack_set_ground_track(&enemy->motion_attack,
                                                number != 0);
        return;
    }
    if (strcmp(action, "groundlanceimpulse") == 0 && has_value) {
        blank3d_motion_attack_set_ground_impulse(&enemy->motion_attack,
            b3d_fpi_q16_to_q12(value_q16));
        return;
    }
    if ((strcmp(action, "groundlancehopheight") == 0 ||
         strcmp(action, "lancehopheight") == 0) && has_value) {
        blank3d_motion_attack_set_ground_hop_height(&enemy->motion_attack,
            b3d_fpi_q16_to_q12(value_q16));
        return;
    }
    if (strcmp(action, "groundlancepiercecontacts") == 0 && has_value) {
        blank3d_motion_attack_set_ground_pierce_contacts(
            &enemy->motion_attack, number < 0 ? 0U : (unsigned int)number);
        return;
    }
    if ((strcmp(action, "motionattackcontactradius") == 0 ||
         strcmp(action, "attackradius") == 0) && has_value) {
        blank3d_motion_attack_set_contact_radius(&enemy->motion_attack,
            b3d_fpi_q16_to_q12(value_q16));
        return;
    }
    if ((strcmp(action, "motionattackpierceextension") == 0 ||
         strcmp(action, "pierceextension") == 0) && has_value) {
        blank3d_motion_attack_set_pierce_extension(&enemy->motion_attack,
            b3d_fpi_q16_to_q12(value_q16));
        return;
    }
    if ((strcmp(action, "airlungeplayer") == 0 ||
         strcmp(action, "midairlungeplayer") == 0 ||
         strcmp(action, "airramplayer") == 0) && has_value) {
        if (!blank3d_vertical_axis_grounded(&g.vertical_axis,
                                             &enemy->vertical)) {
            jump89_state_reset(&enemy->vertical.jump_state);
            (void)blank3d_motion_attack_request_air(&enemy->motion_attack,
                b3d_fpi_q16_to_q12(value_q16));
        }
        return;
    }
    if (strcmp(action, "clawairplayer") == 0 && has_value) {
        (void)blank3d_motion_attack_set_air_intent(&enemy->motion_attack,
                                                   "claw");
        if (!blank3d_vertical_axis_grounded(&g.vertical_axis,
                                             &enemy->vertical)) {
            jump89_state_reset(&enemy->vertical.jump_state);
            (void)blank3d_motion_attack_request_air(&enemy->motion_attack,
                b3d_fpi_q16_to_q12(value_q16));
        }
        return;
    }
    if (strcmp(action, "pierceairplayer") == 0 && has_value) {
        (void)blank3d_motion_attack_set_air_intent(&enemy->motion_attack,
                                                   "pierce");
        if (!blank3d_vertical_axis_grounded(&g.vertical_axis,
                                             &enemy->vertical)) {
            jump89_state_reset(&enemy->vertical.jump_state);
            (void)blank3d_motion_attack_request_air(&enemy->motion_attack,
                b3d_fpi_q16_to_q12(value_q16));
        }
        return;
    }
    if ((strcmp(action, "groundlanceplayer") == 0 ||
         strcmp(action, "groundchargeplayer") == 0) && has_value) {
        int grounded;
        grounded = blank3d_vertical_axis_grounded(&g.vertical_axis,
                                                   &enemy->vertical);
        if (grounded) {
            jump89_state_reset(&enemy->vertical.jump_state);
            (void)blank3d_motion_attack_request_ground(
                &enemy->motion_attack, b3d_fpi_q16_to_q12(value_q16),
                grounded);
        }
        return;
    }
    if ((strcmp(action, "pegasusplayer") == 0 ||
         strcmp(action, "groundramplayer") == 0 ||
         strcmp(action, "skimhopplayer") == 0) && has_value) {
        int grounded;
        const char *ground_style;
        ground_style = strcmp(action, "pegasusplayer") == 0
                     ? "pegasus"
                     : (strcmp(action, "groundramplayer") == 0
                        ? "ram" : "skim_hop");
        (void)blank3d_motion_attack_set_ground_style(
            &enemy->motion_attack, ground_style);
        grounded = blank3d_vertical_axis_grounded(&g.vertical_axis,
                                                   &enemy->vertical);
        if (grounded) {
            jump89_state_reset(&enemy->vertical.jump_state);
            (void)blank3d_motion_attack_request_ground(
                &enemy->motion_attack, b3d_fpi_q16_to_q12(value_q16),
                grounded);
        }
        return;
    }
    if (strcmp(action, "motionattackcancel") == 0 ||
        strcmp(action, "cancelmotionattack") == 0 ||
        strcmp(action, "attackcancel") == 0 ||
        strcmp(action, "airlungecancel") == 0 ||
        strcmp(action, "groundlancecancel") == 0) {
        blank3d_motion_attack_cancel(&enemy->motion_attack);
        return;
    }
    if (strcmp(action, "motionattackclearimpact") == 0 ||
        strcmp(action, "clearmotionattackimpact") == 0 ||
        strcmp(action, "clearattackimpact") == 0 ||
        strcmp(action, "clearimpact") == 0) {
        blank3d_motion_attack_clear_impact(&enemy->motion_attack);
        return;
    }
    if (strcmp(action, "motionattackreset") == 0 ||
        strcmp(action, "resetmotionattack") == 0 ||
        strcmp(action, "attackreset") == 0) {
        blank3d_motion_attack_reset(&enemy->motion_attack);
        return;
    }
    if ((strcmp(action, "motionenabled") == 0 ||
         strcmp(action, "automotionenabled") == 0) && has_value) {
        blank3d_automotion_set_enabled(&enemy->automotion, number != 0);
        return;
    }
    if ((strcmp(action, "motionamplitude") == 0 ||
         strcmp(action, "movementamplitude") == 0) && has_value) {
        blank3d_automotion_set_amplitude(&enemy->automotion,
            b3d_fpi_q16_to_q12(value_q16));
        return;
    }
    if ((strcmp(action, "motionfrequency") == 0 ||
         strcmp(action, "movementfrequency") == 0) && has_value) {
        blank3d_automotion_set_frequency(&enemy->automotion,
            b3d_fpi_q16_to_q12(value_q16));
        return;
    }
    if ((strcmp(action, "motionradius") == 0 ||
         strcmp(action, "movementradius") == 0) && has_value) {
        blank3d_automotion_set_radius(&enemy->automotion,
            b3d_fpi_q16_to_q12(value_q16));
        return;
    }
    if ((strcmp(action, "motionstopdistance") == 0 ||
         strcmp(action, "movementstopdistance") == 0) && has_value) {
        blank3d_automotion_set_stop_distance(&enemy->automotion,
            b3d_fpi_q16_to_q12(value_q16));
        return;
    }
    if ((strcmp(action, "motionsafedistance") == 0 ||
         strcmp(action, "movementsafedistance") == 0) && has_value) {
        blank3d_automotion_set_safe_distance(&enemy->automotion,
            b3d_fpi_q16_to_q12(value_q16));
        return;
    }
    if (strcmp(action, "motionreset") == 0 ||
        strcmp(action, "automotionreset") == 0 ||
        strcmp(action, "movementreset") == 0) {
        blank3d_automotion_reset(&enemy->automotion);
        return;
    }
    if ((strcmp(action, "moveforepattern") == 0 ||
         strcmp(action, "movetoplayerpattern") == 0 ||
         strcmp(action, "automovepattern") == 0) && has_value) {
        speed = b3d_fpi_q16_to_q12(value_q16);
        if (!b3d_enemy_target_position(enemy, &target)) return;
        automotion_reached = 0;
        if (blank3d_automotion_step_pattern(&enemy->automotion,
                &enemy->transform.position, &target,
                speed, g.dt, 0, &automotion_position,
                &automotion_reached))
            enemy->transform.position = automotion_position;
        return;
    }
    if ((strcmp(action, "moveforeflatpattern") == 0 ||
         strcmp(action, "movetoplayerflatpattern") == 0 ||
         strcmp(action, "automoveflatpattern") == 0) && has_value) {
        speed = b3d_fpi_q16_to_q12(value_q16);
        if (!b3d_enemy_target_position(enemy, &target)) return;
        automotion_reached = 0;
        if (blank3d_automotion_step_pattern(&enemy->automotion,
                &enemy->transform.position, &target,
                speed, g.dt, 1, &automotion_position,
                &automotion_reached))
            enemy->transform.position = automotion_position;
        return;
    }
    if ((strcmp(action, "zigzagplayer") == 0 ||
         strcmp(action, "sineplayer") == 0 ||
         strcmp(action, "helixplayer") == 0 ||
         strcmp(action, "orbitplayer") == 0 ||
         strcmp(action, "pingpong") == 0) && has_value) {
        const char *style_name;
        style_name = "sine";
        if (strcmp(action, "zigzagplayer") == 0) style_name = "zigzag";
        else if (strcmp(action, "helixplayer") == 0) style_name = "helix";
        else if (strcmp(action, "orbitplayer") == 0) style_name = "orbit";
        else if (strcmp(action, "pingpong") == 0) style_name = "pingpong";
        (void)blank3d_automotion_set_style(&enemy->automotion, style_name);
        speed = b3d_fpi_q16_to_q12(value_q16);
        if (strcmp(action, "pingpong") == 0)
            blank3d_automotion_set_amplitude(&enemy->automotion, speed);
        else if (strcmp(action, "orbitplayer") == 0)
            blank3d_automotion_set_radius(&enemy->automotion, speed);
        if (!b3d_enemy_target_position(enemy, &target)) return;
        automotion_reached = 0;
        if (blank3d_automotion_step_pattern(&enemy->automotion,
                &enemy->transform.position, &target,
                speed, g.dt,
                strcmp(action, "helixplayer") != 0,
                &automotion_position, &automotion_reached))
            enemy->transform.position = automotion_position;
        return;
    }
    if ((strcmp(action, "zigzagplayerflat") == 0 ||
         strcmp(action, "sineplayerflat") == 0 ||
         strcmp(action, "helixplayerflat") == 0 ||
         strcmp(action, "orbitplayerflat") == 0) && has_value) {
        const char *style_name;
        style_name = "sine";
        if (strcmp(action, "zigzagplayerflat") == 0) style_name = "zigzag";
        else if (strcmp(action, "helixplayerflat") == 0) style_name = "helix";
        else if (strcmp(action, "orbitplayerflat") == 0) style_name = "orbit";
        (void)blank3d_automotion_set_style(&enemy->automotion, style_name);
        speed = b3d_fpi_q16_to_q12(value_q16);
        if (!b3d_enemy_target_position(enemy, &target)) return;
        if (strcmp(action, "orbitplayerflat") == 0)
            blank3d_automotion_set_radius(&enemy->automotion, speed);
        automotion_reached = 0;
        if (blank3d_automotion_step_pattern(&enemy->automotion,
                &enemy->transform.position, &target,
                speed, g.dt, 1, &automotion_position,
                &automotion_reached))
            enemy->transform.position = automotion_position;
        return;
    }
    if ((strcmp(action, "automovetoplayer") == 0 ||
         strcmp(action, "directmovetoplayer") == 0) && has_value) {
        speed = b3d_fpi_q16_to_q12(value_q16);
        if (!b3d_enemy_target_position(enemy, &target)) return;
        automotion_reached = 0;
        if (blank3d_automotion_step_direct(&enemy->automotion,
                &enemy->transform.position, &target,
                speed, g.dt, 0, &automotion_position,
                &automotion_reached))
            enemy->transform.position = automotion_position;
        return;
    }
    if ((strcmp(action, "moveawayplayer") == 0 ||
         strcmp(action, "retreatplayer") == 0) && has_value) {
        speed = b3d_fpi_q16_to_q12(value_q16);
        if (!b3d_enemy_target_position(enemy, &target)) return;
        fallback_direction = gamlib_vec3(0, 0, -G3D_FIX_ONE);
        automotion_reached = 0;
        if (blank3d_automotion_step_away(&enemy->automotion,
                &enemy->transform.position, &target,
                &fallback_direction, speed,
                enemy->automotion.safe_distance, g.dt, 1,
                &automotion_position, &automotion_reached))
            enemy->transform.position = automotion_position;
        return;
    }
    if (strcmp(action, "saveposition") == 0 ||
        strcmp(action, "savereturnposition") == 0 ||
        strcmp(action, "savehome") == 0) {
        (void)blank3d_vertical_axis_save_position(&g.vertical_axis,
                                                   &enemy->vertical);
        return;
    }
    if ((strcmp(action, "diveplayer") == 0 ||
         strcmp(action, "ramplayer") == 0 ||
         strcmp(action, "movetowardplayer") == 0) && has_value) {
        speed = b3d_fpi_q16_to_q12(value_q16);
        if (!b3d_enemy_target_position(enemy, &target)) return;
        target.y = g3d_fix_add_sat(target.y, fix_ratio(9, 10));
        gamlib_vec3_sub(&to_player, &target, &enemy->transform.position);
        if (gamlib_vec3_length(&to_player) > G3D_FIX_EPSILON)
            set_game_yaw(&enemy->transform, fixed_atan2_xz(to_player));
        (void)blank3d_vertical_axis_ram_point(&g.vertical_axis,
                                               &enemy->vertical,
                                               &target, speed, g.dt, 0);
        return;
    }
    if ((strcmp(action, "returntoposition") == 0 ||
         strcmp(action, "movetohome") == 0 ||
         strcmp(action, "returnhome") == 0) && has_value) {
        speed = b3d_fpi_q16_to_q12(value_q16);
        (void)blank3d_vertical_axis_return_saved(&g.vertical_axis,
                                                  &enemy->vertical,
                                                  speed, g.dt, 0);
        return;
    }
    if (strcmp(action, "lookatreturnposition") == 0 ||
        strcmp(action, "lookathome") == 0) {
        Vec3 saved_position;
        if (!blank3d_vertical_axis_saved_position(&enemy->vertical,
                                                   &saved_position)) return;
        gamlib_vec3_sub(&to_player, &saved_position,
                        &enemy->transform.position);
        if (gamlib_vec3_length(&to_player) > G3D_FIX_EPSILON)
            set_game_yaw(&enemy->transform, fixed_atan2_xz(to_player));
        return;
    }
    if ((strcmp(action, "moveup") == 0 ||
         strcmp(action, "move_y_up") == 0) && has_value) {
        speed = b3d_fpi_q16_to_q12(value_q16);
        (void)blank3d_vertical_axis_fly(&g.vertical_axis,
                                        &enemy->vertical,
                                        speed, g.dt, FLY89_UP);
        return;
    }
    if ((strcmp(action, "movedown") == 0 ||
         strcmp(action, "move_y_down") == 0) && has_value) {
        speed = b3d_fpi_q16_to_q12(value_q16);
        (void)blank3d_vertical_axis_fly(&g.vertical_axis,
                                        &enemy->vertical,
                                        speed, g.dt, FLY89_DOWN);
        return;
    }
    if ((strcmp(action, "jump") == 0 ||
         strcmp(action, "jumpstart") == 0 ||
         strcmp(action, "leap") == 0) && has_value) {
        speed = b3d_fpi_q16_to_q12(value_q16);
        (void)blank3d_vertical_axis_jump(&g.vertical_axis,
                                         &enemy->vertical, speed);
        return;
    }
    if ((strcmp(action, "descendtoy") == 0 ||
         strcmp(action, "airdescendtoy") == 0) && has_value) {
        g3d_fix target_y;
        target_y = b3d_fpi_q16_to_q12(value_q16);
        (void)blank3d_vertical_axis_descend_to_y(&g.vertical_axis,
                                                  &enemy->vertical,
                                                  target_y,
                                                  G3D_FIX_FROM_INT(8),
                                                  g.dt, 0);
        return;
    }
    if ((strcmp(action, "moveforeflat") == 0 ||
         strcmp(action, "movetowardplayerflat") == 0) && has_value) {
        speed = b3d_fpi_q16_to_q12(value_q16);
        if (!b3d_enemy_target_position(enemy, &target)) return;
        gamlib_vec3_sub(&to_player, &target,
                        &enemy->transform.position);
        to_player.y = 0;
        if (gamlib_vec3_length(&to_player) <= G3D_FIX_EPSILON) return;
        gamlib_vec3_normalize(&direction, &to_player);
        step = g3d_fix_mul(speed, g.dt);
        gamlib_vec3_scale(&movement, &direction, step);
        gamlib_vec3_add(&enemy->transform.position,
                        &enemy->transform.position, &movement);
        return;
    }
    if (strcmp(action, "movetotarget") == 0 ||
        strcmp(action, "runtotarget") == 0 ||
        strcmp(action, "movefore") == 0) {
        speed = has_value ? b3d_fpi_q16_to_q12(value_q16)
                          : G3D_FIX_FROM_INT(3);
        if (b3d_enemy_target_position(enemy, &target))
            b3d_enemy_move_toward_position(enemy, &target, speed);
        return;
    }
    if (strcmp(action, "movetolastseen") == 0) {
        speed = has_value ? b3d_fpi_q16_to_q12(value_q16)
                          : G3D_FIX_FROM_INT(3);
        if (perception && perception->target_remembered)
            b3d_enemy_move_toward_position(
                enemy, &perception->last_seen_position, speed);
        return;
    }
    if (strcmp(action, "movetolastsound") == 0 ||
        strcmp(action, "movetosound") == 0) {
        speed = has_value ? b3d_fpi_q16_to_q12(value_q16)
                          : G3D_FIX_FROM_INT(3);
        if (perception && perception->target_heard)
            b3d_enemy_move_toward_position(
                enemy, &perception->last_heard_position, speed);
        return;
    }
    if ((strcmp(action, "hurtplayer") == 0 ||
         strcmp(action, "hurttarget") == 0) && enemy->cooldown <= 0) {
        b3d_damage_actor(enemy->weapon_actor_id,
                         enemy->target_actor_id, number);
        enemy->cooldown = fix_ratio(7, 10);
    }
}


static void b3d_object_run_ddsl2(void *user, unsigned long entity_id,
                                 void *native_entity, const char *path)
{
    (void)user;
    (void)entity_id;
    (void)native_entity;
    (void)path;
    g.ddsl_action_mask = 0UL;
    if (!blank3d_languages_tick_ddsl2())
        set_status(blank3d_languages_status());
    b3d_apply_ddsl_action_mask();
}

static void b3d_object_run_fpil(void *user, unsigned long entity_id,
                                void *native_entity, const char *path)
{
    Enemy *enemy;
    Blank3DTruthFacts facts;
    (void)user;
    (void)entity_id;
    enemy = (Enemy *)native_entity;
    if (!enemy || !path || !*path) return;
    b3d_enemy_truth_facts(enemy, &facts);
    if (!blank3d_truth_gate_evaluate(&enemy->truth_gate, &facts)) return;
    (void)blank3d_languages_tick_fpil_path(path, native_entity);
}

static void b3d_object_run_rpyl(void *user, unsigned long entity_id,
                                void *native_entity, const char *path)
{
    (void)user;
    (void)entity_id;
    (void)native_entity;
    if (path && *path) (void)blank3d_languages_run_rpyl(path);
}

static void b3d_object_draw_mesh(void *user, unsigned long entity_id,
                                 void *native_entity,
                                 const Blank3DObjectInit *init)
{
    /* Rendering remains batched by draw_scene. This callback is the GFO
       render authorization point and mesh-provider selector. */
    (void)user;
    (void)entity_id;
    (void)native_entity;
    (void)init;
}

static void b3d_object_draw_script(void *user, unsigned long entity_id,
                                   void *native_entity,
                                   const char *language,
                                   const char *code,
                                   unsigned int code_len)
{
    /* Custom drawing languages are provider hooks, never embedded CPython. */
    (void)user;
    (void)entity_id;
    (void)native_entity;
    (void)language;
    (void)code;
    (void)code_len;
}

static int b3d_numbar_text_eq(const char *a, const char *b)
{
    int ca;
    int cb;
    if (!a || !b) return 0;
    while (*a && *b) {
        ca = (unsigned char)*a++;
        cb = (unsigned char)*b++;
        if (ca >= 'A' && ca <= 'Z') ca += 'a' - 'A';
        if (cb >= 'A' && cb <= 'Z') cb += 'a' - 'A';
        if (ca != cb) return 0;
    }
    return *a == '\0' && *b == '\0';
}

static int b3d_numbar_object_hp_max(unsigned long entity_id, int fallback)
{
    unsigned int i;
    for (i = 0U; i < g.objects.count; ++i) {
        const Blank3DObjectEntity *object;
        object = &g.objects.entities[i];
        if (object->alive && object->entity_id == entity_id)
            return object->init.hp > 0 ? object->init.hp : fallback;
    }
    return fallback;
}

static Enemy *b3d_numbar_nearest_enemy(void)
{
    Enemy *best;
    g3d_fix best_distance;
    int i;
    Vec3 delta;
    g3d_fix distance;
    best = (Enemy *)0;
    best_distance = 0;
    for (i = 0; i < g.enemy_count; ++i) {
        if (!g.enemies[i].alive) continue;
        gamlib_vec3_sub(&delta, &g.enemies[i].transform.position,
                        &g.player.position);
        distance = gamlib_vec3_dot(&delta, &delta);
        if (!best || distance < best_distance) {
            best = &g.enemies[i];
            best_distance = distance;
        }
    }
    return best;
}

static int b3d_numbar_weapon_values(GWP89_Manager *manager,
                                    int actor_id,
                                    int *loaded,
                                    int *capacity,
                                    int *reserve)
{
    int user_slot;
    const GWP89_UserState *user_state;
    const GWP89_WeaponProfile *profile;
    if (!manager) return 0;
    user_slot = gwp89_find_user_slot(manager, actor_id);
    if (user_slot < 0) return 0;
    user_state = &manager->users[user_slot];
    profile = gwp89_get_weapon(manager, user_state->weapon_slot);
    if (!profile) return 0;
    if (loaded)
        *loaded = gwp89_query_clip(manager, actor_id, profile->weapon_id);
    if (capacity)
        *capacity = gwp89_resolve_numeric_int(manager, actor_id, profile,
                                               GWP89_NUM_CLIP_SIZE,
                                               profile->clip_size);
    if (reserve)
        *reserve = gwp89_query_ammo(manager, actor_id, profile->ammo_id,
                                    profile->weapon_id);
    return 1;
}

static long b3d_numbar_resolve(void *user,
                               unsigned long entity_id,
                               void *native_entity,
                               const char *binding,
                               long fallback,
                               int *resolved)
{
    Enemy *self_enemy;
    Enemy *target_enemy;
    GWP89_Manager *weapon_manager;
    int weapon_actor_id;
    int loaded;
    int capacity;
    int reserve;
    int self_is_player;
    int self_alive;
    int self_health;
    int self_health_max;
    (void)user;
    if (resolved) *resolved = 1;
    self_is_player = entity_id == 1UL;
    self_enemy = self_is_player ? (Enemy *)0 : (Enemy *)native_entity;
    self_alive = self_is_player ? (g.player_hp > 0) :
                 (self_enemy && self_enemy->alive);
    self_health = self_is_player ? g.player_hp :
                  (self_enemy ? self_enemy->hp : 0);
    self_health_max = b3d_numbar_object_hp_max(entity_id,
                                               self_is_player ? 100 : self_health);

    if (b3d_numbar_text_eq(binding, "self.id")) return (long)entity_id;
    if (b3d_numbar_text_eq(binding, "self.health")) return self_health;
    if (b3d_numbar_text_eq(binding, "self.health_max")) return self_health_max;
    if (b3d_numbar_text_eq(binding, "self.alive")) return self_alive ? 1L : 0L;
    if (b3d_numbar_text_eq(binding, "self.dead")) return self_alive ? 0L : 1L;

    if (b3d_numbar_text_eq(binding, "player.health")) return g.player_hp;
    if (b3d_numbar_text_eq(binding, "player.health_max")) return 100L;
    if (b3d_numbar_text_eq(binding, "player.alive"))
        return g.player_hp > 0 ? 1L : 0L;
    if (b3d_numbar_text_eq(binding, "player.dead"))
        return g.player_hp <= 0 ? 1L : 0L;
    if (b3d_numbar_text_eq(binding, "gameplay.threat"))
        return (long)b3d_player_threat_level();
    if (b3d_numbar_text_eq(binding, "ecg.bpm"))
        return (long)blank3d_ecg_vitals_bpm(&g.hud.ecg);
    if (b3d_numbar_text_eq(binding, "damage.flash_ms"))
        return (long)g.player_damage_flash_ms;
    if (b3d_numbar_text_eq(binding, "camera.first_person"))
        return g.camera_mode == CAMERA_FPS ? 1L : 0L;
    if (b3d_numbar_text_eq(binding, "weapon.muzzle_flash"))
        return g.muzzle_flash_ms > 0 ? 1L : 0L;

    target_enemy = self_is_player ? b3d_numbar_nearest_enemy() : (Enemy *)0;
    if (b3d_numbar_text_eq(binding, "target.health") ||
        b3d_numbar_text_eq(binding, "enemy.health") ||
        b3d_numbar_text_eq(binding, "boss.health"))
        return self_is_player ? (target_enemy ? target_enemy->hp : 0L) :
               (long)g.player_hp;
    if (b3d_numbar_text_eq(binding, "target.health_max") ||
        b3d_numbar_text_eq(binding, "enemy.health_max") ||
        b3d_numbar_text_eq(binding, "boss.health_max")) {
        if (!self_is_player) return 100L;
        if (!target_enemy) return 1L;
        return b3d_numbar_object_hp_max(
            (unsigned long)(100 + (int)(target_enemy - g.enemies)),
            target_enemy->hp > 0 ? target_enemy->hp : 1);
    }
    if (b3d_numbar_text_eq(binding, "target.alive") ||
        b3d_numbar_text_eq(binding, "enemy.alive") ||
        b3d_numbar_text_eq(binding, "boss.alive"))
        return self_is_player ? (target_enemy ? 1L : 0L) :
               (g.player_hp > 0 ? 1L : 0L);

    weapon_manager = self_is_player ? &g.systems.weapons : &g.npc_weapons;
    weapon_actor_id = self_is_player ? B3D_PLAYER_ACTOR_ID :
                      (self_enemy ? self_enemy->weapon_actor_id : 0);
    loaded = 0;
    capacity = 1;
    reserve = 0;
    if (b3d_numbar_weapon_values(weapon_manager, weapon_actor_id,
                                 &loaded, &capacity, &reserve)) {
        if (b3d_numbar_text_eq(binding, "self.weapon.loaded")) return loaded;
        if (b3d_numbar_text_eq(binding, "self.weapon.capacity")) return capacity;
        if (b3d_numbar_text_eq(binding, "self.weapon.reserve")) return reserve;
    }
    if (self_is_player) {
        if (b3d_numbar_text_eq(binding, "weapon.loaded"))
            return blank3d_systems_clip(&g.systems);
        if (b3d_numbar_text_eq(binding, "weapon.capacity"))
            return current_clip_capacity();
        if (b3d_numbar_text_eq(binding, "weapon.reserve"))
            return blank3d_systems_reserve(&g.systems);
    }
    if (resolved) *resolved = 0;
    return fallback;
}

static int b3d_gfo_argument_text(unsigned long entity_id,
                                 const gfo_value *value,
                                 char *out,
                                 unsigned int out_cap)
{
    unsigned int i;
    unsigned int n;
    gfo_str text;
    if (!value || !out || out_cap < 2U) return 0;
    text.ptr = (const char *)0;
    text.len = 0;
    if (value->t == GFO_VAL_STR || value->t == GFO_VAL_PATH) {
        text = value->v.s;
    } else if (value->t == GFO_VAL_SYM) {
        for (i = 0U; i < g.objects.count; ++i) {
            Blank3DObjectEntity *object;
            object = &g.objects.entities[i];
            if (object->alive && object->entity_id == entity_id) {
                text = gfo_sym_name_by_id(&object->gfo.sym, value->v.sym);
                break;
            }
        }
    }
    if (!text.ptr) return 0;
    n = text.len < out_cap - 1U ? text.len : out_cap - 1U;
    memcpy(out, text.ptr, n);
    out[n] = '\0';
    return n > 0U;
}

static void b3d_object_invoke(void *user, unsigned long entity_id,
                              void *native_entity, const char *name,
                              const gfo_value *argv, unsigned int argc)
{
    char path[B3D_NUMBAR_PATH_CAP];
    (void)user;
    if (name && strcmp(name, "draw_numbar") == 0) {
        if (argc < 1U || !b3d_gfo_argument_text(entity_id, &argv[0],
                                                 path, sizeof(path))) {
            set_status("draw_numbar requires an orchestrator INI path");
            return;
        }
        if (!blank3d_numbar_request(&g.numbars, entity_id, native_entity,
                                    path))
            set_status(blank3d_numbar_status(&g.numbars));
        return;
    }
    if (name && strcmp(name, "reload_logic") == 0)
        set_status("GFO requested logic reload");
}

static void b3d_object_handle(void *user, unsigned long entity_id,
                              void *native_entity, const char *name)
{
    (void)user;
    (void)entity_id;
    (void)native_entity;
    if (name && strcmp(name, "initialize_from_ini") == 0)
        return;
}

static void blank3d_init_object_runtime(void)
{
    Blank3DObjectHost host;
    memset(&host, 0, sizeof(host));
    host.user = &g;
    host.run_ddsl2 = b3d_object_run_ddsl2;
    host.run_fpil = b3d_object_run_fpil;
    host.run_rpyl = b3d_object_run_rpyl;
    host.draw_mesh = b3d_object_draw_mesh;
    host.draw_script = b3d_object_draw_script;
    host.invoke = b3d_object_invoke;
    host.handle = b3d_object_handle;
    blank3d_numbar_init(&g.numbars, b3d_numbar_resolve, &g);
    blank3d_objects_init(&g.objects, &host);
}

static void blank3d_rebuild_gfo_entities(void)
{
    unsigned int slot;
    int i;
    char ini_path[128];
    const char *archetype;
    blank3d_init_object_runtime();
    (void)blank3d_objects_spawn(&g.objects,
        "config/entities/player.ini", 1UL, &g.player, &slot);
    for (i = 0; i < g.enemy_count; ++i) {
        if (!g.enemies[i].alive) continue;
        archetype = g.enemies[i].archetype[0] != '\0'
                  ? g.enemies[i].archetype : "zombie";
        sprintf(ini_path, "config/entities/%s.ini", archetype);
        (void)blank3d_objects_spawn(&g.objects, ini_path,
            (unsigned long)(100 + i), &g.enemies[i], &slot);
    }
}

static void blank3d_init_language_runtime(void)
{
    Blank3DLanguageHost host;
    memset(&host, 0, sizeof(host));
    host.user = &g;
    host.key_down = b3d_language_key_down;
    host.input_query = b3d_language_input_query;
    host.ddsl_action = b3d_language_ddsl_action;
    host.rpyl_command = b3d_language_rpyl_command;
    host.fpil_condition = b3d_language_fpil_condition;
    host.fpil_action = b3d_language_fpil_action;
    blank3d_languages_init(&host);
}

static void b3d_clear_casings(void)
{
    int i;
    for (i = 0; i < MAX_CASINGS; ++i) {
        if (g.casings[i].physics_active && g.casing_physics.physics)
            blank3d_casing_physics_release(&g.casing_physics, i);
        g.casings[i].alive = 0;
        g.casings[i].physics_active = 0;
    }
}

static void load_rpy(void)
{
    int i;
    b3d_clear_casings();
    default_world();
    if (!blank3d_faction_init(&g.factions,
                              "config/factions/gameplay.ini"))
        set_status(blank3d_faction_status(&g.factions));
    (void)blank3d_faction_register_actor(&g.factions,
        B3D_PLAYER_ACTOR_ID, "player", "survivors", "hero",
        "human,organic,armed,player,targetable", 85, 100, 1, 1);
    for (i = 0; i < MAX_CASINGS; ++i) {
        g.casings[i].alive = 0;
        g.casings[i].weapon_id = 0;
        g.casings[i].casing_mesh_id = 1;
        g.casings[i].physics_active = 0;
        g.casings[i].mesh_scale = G3D_FIX_ONE;
        reset_transform(&g.casings[i].transform, 0, 0, 0);
        g.casings[i].velocity = gamlib_vec3(0, 0, 0);
        g.casings[i].life = 0;
    }
    for (i = 0; i < MAX_ENEMIES; ++i) {
        g.enemies[i].alive = 0;
        g.enemies[i].state = 0;
        g.enemies[i].archetype[0] = '\0';
        g.enemies[i].hp = 30;
        g.enemies[i].weapon_actor_id = 1000 + i;
        g.enemies[i].weapon_ready = 0;
        g.enemies[i].weapon_loadout[0] = '\0';
        g.enemies[i].requested_weapon[0] = '\0';
        reset_transform(&g.enemies[i].transform, 0, 0, 0);
        blank3d_vertical_body_init(&g.enemies[i].vertical,
                                   &g.enemies[i].transform, 0, 0);
        blank3d_automotion_init(&g.enemies[i].automotion);
        blank3d_motion_attack_init(&g.enemies[i].motion_attack);
        blank3d_truth_gate_init(&g.enemies[i].truth_gate,
                                B3D_TRUTH_PROFILE_RETRO);
        g.enemies[i].truth_range = G3D_FIX_FROM_INT(28);
        blank3d_perception_config_defaults(&g.enemies[i].perception_config);
        g.enemies[i].perception_ini[0] = '\0';
        g.enemies[i].perception_index = i;
    }
    g.enemy_count = 0;
    b3d_reset_npc_weapon_manager();
    if (!blank3d_languages_run_rpyl(SCRIPT_RPY)) {
        set_status(blank3d_languages_status());
    } else {
        set_status("startup.rpy executed by vendored RPYL VM");
    }
    if (g.enemy_count <= 0) {
        g.enemy_count = 1;
        g.enemies[0].alive = 1;
        reset_transform(&g.enemies[0].transform, 0, 0,
                        G3D_FIX_FROM_INT(18));
        g.enemies[0].hp = 30;
        blank3d_vertical_body_init(&g.enemies[0].vertical,
                                   &g.enemies[0].transform,
                                   g.enemies[0].transform.position.y, 0);
        strcpy(g.enemies[0].archetype, "zombie");
        blank3d_automotion_init_archetype(&g.enemies[0].automotion,
                                           "zombie");
        blank3d_truth_gate_init(&g.enemies[0].truth_gate,
                                B3D_TRUTH_PROFILE_RETRO);
        g.enemies[0].truth_range = G3D_FIX_FROM_INT(28);
        blank3d_perception_config_defaults(&g.enemies[0].perception_config);
        g.enemies[0].perception_ini[0] = '\0';
        g.enemies[0].perception_index = 0;
        g.enemies[0].target_actor_id = B3D_TARGET_NONE;
        g.enemies[0].last_damage_source_actor_id = B3D_TARGET_NONE;
        g.enemies[0].damage_interest_time = 0;
        strcpy(g.enemies[0].faction_name, "hostile");
        strcpy(g.enemies[0].team_name, "monsters");
        strcpy(g.enemies[0].role_name, "attacker");
        strcpy(g.enemies[0].faction_tags,
               "organic,enemy,targetable");
        g.enemies[0].faction_is_ally = 0;
        (void)blank3d_faction_register_actor(&g.factions,
            g.enemies[0].weapon_actor_id, "hostile", "monsters",
            "attacker", "organic,enemy,targetable",
            65, 55, 1, 1);
    }
    g.camera_yaw = g.player.rotation.y;
    g.base_fov = g.fov;
    configure_sockets();
    b3d_perception_reset_agents();
    b3d_faction_update_targets();
    blank3d_rebuild_gfo_entities();
    camera_init_perspective(&g.camera, g.fov,
                            fix_ratio(g.width, g.height ? g.height : 1),
                            g.near_z, g.far_z);
}

static void build_view_vectors(Vec3 *out_eye,
                               Vec3 *out_forward,
                               Vec3 *out_right,
                               Vec3 *out_up)
{
    Transform view_transform;
    Vec3 forward;
    Vec3 right;
    Vec3 local_up;
    Vec3 world_up;
    Vec3 eye;
    Vec3 offset;

    if (blank3d_fire_frame_sync_get_view(&g.fire_frame_sync,
                                         (unsigned int)g.frame_stamp,
                                         out_eye,
                                         out_forward,
                                         out_right,
                                         out_up))
        return;

    if (g.cameranaku.initialized) {
        blank3d_cameranaku_get_view(&g.cameranaku,
                                    out_eye,
                                    out_forward,
                                    out_right,
                                    out_up);
        return;
    }

    view_transform = g.player;
    view_transform.rotation.y = g3d_fix_add_sat(g.camera_yaw,
                                                   g.sniper_sway_yaw);
    view_transform.rotation.x = g3d_fix_add_sat(g.camera_pitch,
                                                   g.sniper_sway_pitch);
    transform_get_local_axes(&view_transform, &right, &local_up, &forward);
    world_up = gamlib_vec3(0, G3D_FIX_ONE, 0);
    eye = g.player.position;

    if (g.camera_mode == CAMERA_FPS) {
        gamlib_vec3_scale(&offset, &world_up, g.camera_eye_height);
        gamlib_vec3_add(&eye, &eye, &offset);
        gamlib_vec3_scale(&offset, &forward, fix_ratio(3, 20));
        gamlib_vec3_add(&eye, &eye, &offset);
    } else {
        gamlib_vec3_scale(&offset, &world_up, g.camera_height);
        gamlib_vec3_add(&eye, &eye, &offset);
        gamlib_vec3_scale(&offset, &forward, g3d_fix_neg_sat(g.camera_dist));
        gamlib_vec3_add(&eye, &eye, &offset);
        gamlib_vec3_scale(&offset, &right, g.camera_shoulder);
        gamlib_vec3_add(&eye, &eye, &offset);
    }

    if (out_eye) *out_eye = eye;
    if (out_forward) *out_forward = forward;
    if (out_right) *out_right = right;
    if (out_up) *out_up = local_up;
}

static void build_weapon_input_vectors(GWP89_Vec3 *socket_origin,
                                       GWP89_Vec3 *socket_forward,
                                       GWP89_Vec3 *socket_right,
                                       GWP89_Vec3 *socket_up,
                                       GWP89_Vec3 *camera_origin,
                                       GWP89_Vec3 *camera_forward)
{
    Vec3 eye;
    Vec3 forward;
    Vec3 right;
    Vec3 up;
    Vec3 muzzle_position;
    Vec3 offset;
    soq3d_pose muzzle_pose;

    build_view_vectors(&eye, &forward, &right, &up);
    muzzle_position = g.player.position;
    publish_transform(g.player_key, &g.player);
    if (g.camera_mode == CAMERA_TPS &&
        soq3d_get_socket(&g.locator, g.player_ballistic_muzzle_socket,
                         g.frame_stamp, &muzzle_pose) == SOQ3D_OK) {
        muzzle_position = bridge_pose_position_q20(&muzzle_pose);
    } else {
        muzzle_position = eye;
        gamlib_vec3_scale(&offset, &forward, fix_ratio(9, 20));
        gamlib_vec3_add(&muzzle_position, &muzzle_position, &offset);
        gamlib_vec3_scale(&offset, &right, fix_ratio(1, 5));
        gamlib_vec3_add(&muzzle_position, &muzzle_position, &offset);
        gamlib_vec3_scale(&offset, &up, g3d_fix_neg_sat(fix_ratio(3, 20)));
        gamlib_vec3_add(&muzzle_position, &muzzle_position, &offset);
    }

    if (socket_origin) *socket_origin = vec3_to_gwp(muzzle_position);
    if (socket_forward) *socket_forward = vec3_to_gwp(forward);
    if (socket_right) *socket_right = vec3_to_gwp(right);
    if (socket_up) *socket_up = vec3_to_gwp(up);
    if (camera_origin) *camera_origin = vec3_to_gwp(eye);
    if (camera_forward) *camera_forward = vec3_to_gwp(forward);
}

static void set_mouse_capture_state(int enabled)
{
    RECT rect;
    POINT upper_left;
    POINT lower_right;
    POINT center;
    enabled = enabled ? 1 : 0;
    if (!g.window) {
        g.mouse_captured = enabled;
        return;
    }
    if (enabled == g.mouse_captured) return;
    g.mouse_captured = enabled;
    if (enabled) {
        if (GetClientRect(g.window, &rect)) {
            upper_left.x = rect.left;
            upper_left.y = rect.top;
            lower_right.x = rect.right;
            lower_right.y = rect.bottom;
            ClientToScreen(g.window, &upper_left);
            ClientToScreen(g.window, &lower_right);
            rect.left = upper_left.x;
            rect.top = upper_left.y;
            rect.right = lower_right.x;
            rect.bottom = lower_right.y;
            ClipCursor(&rect);
            center.x = (rect.left + rect.right) / 2;
            center.y = (rect.top + rect.bottom) / 2;
            SetCursorPos(center.x, center.y);
            g.last_mouse = center;
        }
        ShowCursor(FALSE);
    } else {
        ClipCursor(0);
        ShowCursor(TRUE);
        GetCursorPos(&g.last_mouse);
    }
}

static void update_input_mouse(void)
{
    POINT point;
    POINT center;
    RECT rect;
    int dx;
    int dy;
    g3d_fix horizontal_amount;
    g3d_fix vertical_amount;
    g3d_fix sensitivity;

    if (!g.active || !g.mouse_captured) return;
    if (!GetClientRect(g.window, &rect)) return;
    center.x = (rect.left + rect.right) / 2;
    center.y = (rect.top + rect.bottom) / 2;
    ClientToScreen(g.window, &center);
    GetCursorPos(&point);
    dx = point.x - center.x;
    dy = point.y - center.y;
    if (dx != 0 || dy != 0) {
        sensitivity = g.mouse_sensitivity;
        if (blank3d_sniper_is_scoped(&g.sniper)) {
            sensitivity = g3d_fix_mul(sensitivity,
                fix_ratio(blank3d_sniper_sensitivity_pct(&g.sniper), 100));
        }
        /*
         * Keep Win32 mouse deltas untouched.  Route each semantic direction
         * through its own Cameranaku feeder bridge, where all four signs are
         * deliberately negated for the Gamlib3D/Naku convention boundary.
         */
        horizontal_amount = g3d_fix_mul(
            G3D_FIX_FROM_INT(dx < 0 ? -dx : dx), sensitivity);
        vertical_amount = g3d_fix_mul(
            G3D_FIX_FROM_INT(dy < 0 ? -dy : dy), sensitivity);
        if (dx < 0) {
            blank3d_cameranaku_feed_left(&g.cameranaku, horizontal_amount);
        } else if (dx > 0) {
            blank3d_cameranaku_feed_right(&g.cameranaku, horizontal_amount);
        }
        if (dy < 0) {
            blank3d_cameranaku_feed_up(&g.cameranaku, vertical_amount);
        } else if (dy > 0) {
            blank3d_cameranaku_feed_down(&g.cameranaku, vertical_amount);
        }
        g.camera_yaw = g.cameranaku.yaw;
        g.camera_pitch = g.cameranaku.pitch;
        g.player.rotation.y = g.camera_yaw;
        SetCursorPos(center.x, center.y);
        g.last_mouse = center;
    }
}

static void update_zoom(void)
{
    g3d_fix target;
    g3d_fix step;
    int weapon_id;
    const Blank3DWeaponModules *modules;
    Transform view_transform;
    Vec3 right;
    Vec3 up;
    Vec3 forward;
    Vec3 eye;
    Vec3 offset;
    g89_camera scope_camera;
    short movement_pct;
    short stress_pct;
    short dt_frames;
    short active_sniper;

    weapon_id = blank3d_systems_weapon_id(&g.systems);
    modules = blank3d_weapon_modules_get(weapon_id);
    active_sniper = (short)(modules &&
                     modules->optic_profile == B3D_OPTIC_SNIPER);

    view_transform = g.player;
    view_transform.rotation.y = g.camera_yaw;
    view_transform.rotation.x = g.camera_pitch;
    transform_get_local_axes(&view_transform, &right, &up, &forward);
    eye = g.player.position;
    offset = gamlib_vec3(0, g.camera_eye_height, 0);
    gamlib_vec3_add(&eye, &eye, &offset);
    scope_camera.pos.x = q12_to_q16_long(eye.x);
    scope_camera.pos.y = q12_to_q16_long(eye.y);
    scope_camera.pos.z = q12_to_q16_long(eye.z);
    scope_camera.forward.x = q12_to_q16_long(forward.x);
    scope_camera.forward.y = q12_to_q16_long(forward.y);
    scope_camera.forward.z = q12_to_q16_long(forward.z);
    scope_camera.right.x = q12_to_q16_long(right.x);
    scope_camera.right.y = q12_to_q16_long(right.y);
    scope_camera.right.z = q12_to_q16_long(right.z);
    scope_camera.up.x = q12_to_q16_long(up.x);
    scope_camera.up.y = q12_to_q16_long(up.y);
    scope_camera.up.z = q12_to_q16_long(up.z);
    scope_camera.base_fov_deg_x100 = (short)(
        ((long)g.base_fov * 100L) / (long)G3D_FIX_ONE);
    scope_camera.current_fov_deg_x100 = (short)(
        ((long)g.fov * 100L) / (long)G3D_FIX_ONE);
    movement_pct = (short)((g.keys['W'] || g.keys['A'] ||
                            g.keys['S'] || g.keys['D']) ? 70 : 0);
    stress_pct = (short)(100 - g.player_hp);
    if (stress_pct < 0) stress_pct = 0;
    if (stress_pct > 100) stress_pct = 100;
    dt_frames = (short)(((unsigned int)g.frame_ms + 8U) / 16U);
    if (dt_frames < 1) dt_frames = 1;
    blank3d_sniper_update(&g.sniper, active_sniper,
                          (short)(g.keys[VK_CONTROL] ? 1 : 0),
                          (short)(g.keys[0x10] ? 1 : 0),
                          movement_pct, stress_pct, dt_frames, &scope_camera);

    if (active_sniper) {
        g.fov = (g3d_fix)(((long)blank3d_sniper_fov_deg_x100(&g.sniper) *
                           (long)G3D_FIX_ONE) / 100L);
        g.sniper_sway_yaw = (g3d_fix)(((long)
            blank3d_sniper_sway_yaw_x1000(&g.sniper) *
            (long)G3D_FIX_ONE) / 1000L);
        g.sniper_sway_pitch = (g3d_fix)(((long)
            blank3d_sniper_sway_pitch_x1000(&g.sniper) *
            (long)G3D_FIX_ONE) / 1000L);
    } else {
        g.sniper_sway_yaw = 0;
        g.sniper_sway_pitch = 0;
        target = g.keys[VK_CONTROL] ? g.zoom_fov : g.base_fov;
        step = g3d_fix_mul(g.zoom_speed, g.dt);
        if (g.fov < target) {
            g.fov = g3d_fix_add_sat(g.fov, step);
            if (g.fov > target) g.fov = target;
        } else if (g.fov > target) {
            g.fov = g3d_fix_sub_sat(g.fov, step);
            if (g.fov < target) g.fov = target;
        }
    }
    if (g.fov <= 0) g.fov = G3D_FIX_FROM_INT(1);
    g.zoom_fx = g3d_fix_div(g.base_fov, g.fov);
    if (g.zoom_fx < GWP89_FIX_ONE) g.zoom_fx = GWP89_FIX_ONE;
    blank3d_cameranaku_set_fov(&g.cameranaku, g.fov);
    blank3d_cameranaku_set_zoom(&g.cameranaku, g.zoom_fx);
    blank3d_cameranaku_set_sway(&g.cameranaku,
                                g.sniper_sway_yaw,
                                g.sniper_sway_pitch);
}

static void update_cameranaku(void)
{
    Vec3 velocity;

    /* Recoil generated while draining the previous frame's weapon events is
       committed before this frame gets its immutable camera snapshot.  The
       shot, weapon carrier, crosshair and renderer therefore never consume
       two different camera ages during sustained automatic fire. */
    (void)blank3d_fire_frame_sync_apply_pending(&g.fire_frame_sync,
                                                 &g.cameranaku);
    g.camera_yaw = g.cameranaku.yaw;
    g.camera_pitch = g.cameranaku.pitch;

    velocity = gamlib_vec3(0, 0, 0);
    blank3d_cameranaku_set_target(&g.cameranaku,
                                  &g.player.position,
                                  &velocity);
    blank3d_cameranaku_set_angles(&g.cameranaku,
                                  g.camera_yaw,
                                  g.camera_pitch);
    blank3d_cameranaku_update(&g.cameranaku, g.frame_ms);
    g.camera_yaw = g.cameranaku.yaw;
    g.camera_pitch = g.cameranaku.pitch;
    blank3d_fire_frame_sync_capture(&g.fire_frame_sync,
                                    (unsigned int)g.frame_stamp,
                                    &g.cameranaku);
}


static int current_clip_capacity(void)
{
    int user_slot;
    const GWP89_UserState *user;
    const GWP89_WeaponProfile *profile;
    user_slot = gwp89_find_user_slot(&g.systems.weapons, B3D_PLAYER_ACTOR_ID);
    if (user_slot < 0) return 1;
    user = &g.systems.weapons.users[user_slot];
    profile = gwp89_get_weapon(&g.systems.weapons, user->weapon_slot);
    return profile && profile->clip_size > 0 ? profile->clip_size : 1;
}

static void process_weapon_events(void)
{
    GWP89_Event event;
    GWP89_Event gatling_fire_event;
    GWP89_Event shotgun_fire_event;
    GWP89_Event shotgun_template_event;
    GWP89_Event recovered_event;
    int gatling_fire_seen;
    int gatling_projectile_seen;
    int shotgun_fire_seen;
    int shotgun_template_seen;
    int shotgun_pellet_mask;
    int shotgun_expected;
    int shotgun_index;
    int should_spawn_projectile;
    char title[256];
    memset(&gatling_fire_event, 0, sizeof(gatling_fire_event));
    memset(&shotgun_fire_event, 0, sizeof(shotgun_fire_event));
    memset(&shotgun_template_event, 0, sizeof(shotgun_template_event));
    gatling_fire_seen = 0;
    gatling_projectile_seen = 0;
    shotgun_fire_seen = 0;
    shotgun_template_seen = 0;
    shotgun_pellet_mask = 0;
    shotgun_expected = B3D_SHOTGUN_PELLET_COUNT;
    while (b3d_poll_weapon_event(&event)) {
        if (event.type == GWP89_EVENT_PROJECTILE_REQUEST) {
            should_spawn_projectile = 1;
            if (event.weapon_id == GATLING_WEAPON_ID)
                gatling_projectile_seen = 1;
            if (event.weapon_id == B3D_SHOTGUN_WEAPON_ID) {
                /* This profile is buckshot, never a slug: the runtime contract
                   is exactly seven indices even if a provider accidentally
                   reports pellet_count=1 or repeats index zero. */
                if (event.pellet_index < 0 ||
                    event.pellet_index >= B3D_SHOTGUN_PELLET_COUNT) {
                    should_spawn_projectile = 0;
                } else if (shotgun_pellet_mask &
                           (1 << event.pellet_index)) {
                    should_spawn_projectile = 0;
                } else {
                    shotgun_pellet_mask |= 1 << event.pellet_index;
                    if (!shotgun_template_seen) {
                        shotgun_template_event = event;
                        shotgun_template_seen = 1;
                    }
                }
            }
            if (should_spawn_projectile) spawn_bullet_event(&event);
        } else if (event.type == GWP89_EVENT_FIRE_ACCEPTED) {
            if (event.weapon_id == GATLING_WEAPON_ID) {
                gatling_fire_event = event;
                gatling_fire_seen = 1;
            }
            if (event.weapon_id == B3D_SHOTGUN_WEAPON_ID) {
                shotgun_fire_event = event;
                shotgun_fire_seen = 1;
            }
            if (event.weapon_id == GATLING_WEAPON_ID)
                blank3d_audio_gatling_fire_start(&g.audio);
            /* Fire synthesis remains provider-driven. The slingshot is not a
               firearm, so it currently uses projectile/impact voices only. */
            if (event.weapon_id != SLINGSHOT_WEAPON_ID)
                blank3d_audio_fire_sync(
                    &g.audio, event.weapon_id,
                    event.clip_ammo, current_clip_capacity());
            blank3d_actor_equipment_trigger_fire(
                &g.actor_equipment, event.actor_id, event.weapon_id);
            if (event.actor_id == B3D_PLAYER_ACTOR_ID) {
                (void)blank3d_perception_emit_sound(
                    &g.perception, &g.player.position,
                    G3D_FIX_FROM_INT(36), G3D_FIX_ONE,
                    g.perception.player_entity_id);
                if (blank3d_weapon_modules_has_muzzle(event.weapon_id))
                    g.muzzle_flash_ms =
                        event.weapon_id == GATLING_WEAPON_ID ? 38 : 70;
                else
                    g.muzzle_flash_ms = 0;
                blank3d_fire_frame_sync_queue_recoil(
                    &g.fire_frame_sync,
                    g3d_fix_mul((g3d_fix)event.recoil_fx, fix_ratio(7, 20)),
                    g3d_fix_mul((g3d_fix)event.recoil_fx, fix_ratio(1, 20)));
            }
        } else if (event.type == GWP89_EVENT_DRY_FIRE) {
            if (event.weapon_id == GATLING_WEAPON_ID)
                blank3d_audio_gatling_release(&g.audio);
            blank3d_audio_dry_fire(&g.audio, event.weapon_id);
        } else if (event.type == GWP89_EVENT_RELOAD_BEGIN) {
            if (event.weapon_id == GATLING_WEAPON_ID)
                blank3d_audio_gatling_release(&g.audio);
            blank3d_audio_reload_begin_sync(
                &g.audio, event.weapon_id,
                event.clip_ammo, current_clip_capacity());
        } else if (event.type == GWP89_EVENT_RELOAD_END) {
            blank3d_audio_reload_end_sync(
                &g.audio, event.weapon_id,
                event.clip_ammo, current_clip_capacity());
        } else if (event.type == GWP89_EVENT_MUZZLE_REQUEST) {
            if (blank3d_weapon_modules_has_muzzle(event.weapon_id))
                g.muzzle_flash_ms = 70;
        } else if (event.type == GWP89_EVENT_CASING_REQUEST) {
            spawn_casing_event(&event);
            set_status("weapon event: casing mesh + synth emitted");
        } else if (event.type == GWP89_EVENT_TRAIL_REQUEST) {
            /* Active projectile meshes are the source runner's trail proxy. */
            set_status("weapon event: trail emitted");
        } else if (event.type == GWP89_EVENT_MESH_ASSIGN_REQUEST) {
            set_status("weapon event: projectile mesh assigned");
        } else if (event.type == GWP89_EVENT_VISUAL_MOD_REQUEST) {
            set_status("weapon event: visual modifier applied");
        } else if (event.type == GWP89_EVENT_AMMO_CHANGED &&
                   event.actor_id == B3D_PLAYER_ACTOR_ID) {
            sprintf(title, "Blank3D | %s | clip %d reserve %d | %s",
                    event.weapon_name,
                    event.clip_ammo,
                    event.reserve_ammo,
                    g.camera_mode == CAMERA_FPS ? "FPS" : "TPS");
            SetWindowTextA(g.window, title);
        } else if (event.type == GWP89_EVENT_ACTIVE_RELOAD_WINDOW) {
            if (event.actor_id == B3D_PLAYER_ACTOR_ID)
                set_status("active reload window");
        } else if (event.type == GWP89_EVENT_ACTIVE_RELOAD_SUCCESS) {
            blank3d_audio_reload_end_sync(
                &g.audio, event.weapon_id,
                event.clip_ammo, event.clip_ammo > 0 ? event.clip_ammo : 1);
            if (event.actor_id == B3D_PLAYER_ACTOR_ID)
                set_status("active reload: success");
        } else if (event.type == GWP89_EVENT_ACTIVE_RELOAD_FAIL) {
            blank3d_audio_dry_fire(&g.audio, event.weapon_id);
            if (event.actor_id == B3D_PLAYER_ACTOR_ID)
                set_status("active reload: failed");
        } else if (event.type == GWP89_EVENT_PROVIDER_CANCELLED) {
            set_status("weapon provider cancelled an action");
        } else if (event.type == GWP89_EVENT_WEAPON_CHANGED &&
                   event.actor_id == B3D_PLAYER_ACTOR_ID) {
            g.slingshot_charging = 0;
            g.slingshot_charge_ms = 0U;
            if (event.weapon_id != GATLING_WEAPON_ID) {
                blank3d_audio_gatling_release(&g.audio);
                g.gatling_spin_ms = 0U;
                g.gatling_armed = 0;
            }
            sprintf(title, "Blank3D | %s | clip %d reserve %d | %s",
                    event.weapon_name,
                    event.clip_ammo,
                    event.reserve_ammo,
                    g.camera_mode == CAMERA_FPS ? "FPS" : "TPS");
            SetWindowTextA(g.window, title);
        }
    }
    /* A shotgun discharge is one ammo transaction but seven independent
       physical projectiles. Recover only missing pellet indices so a provider
       or saturated event bus can never degrade the shell into one normal
       bullet. Normal manager output keeps all seven bits set and is untouched. */
    if (shotgun_fire_seen) {
        if (shotgun_expected < 1 ||
            shotgun_expected > B3D_SHOTGUN_PELLET_COUNT)
            shotgun_expected = B3D_SHOTGUN_PELLET_COUNT;
        for (shotgun_index = 0; shotgun_index < shotgun_expected;
             ++shotgun_index) {
            if (shotgun_pellet_mask & (1 << shotgun_index)) continue;
            recovered_event = shotgun_template_seen
                ? shotgun_template_event : shotgun_fire_event;
            recovered_event.type = GWP89_EVENT_PROJECTILE_REQUEST;
            recovered_event.weapon_id = B3D_SHOTGUN_WEAPON_ID;
            recovered_event.projectile_id = 3;
            recovered_event.projectile_mesh_id = 3;
            recovered_event.pellet_index = shotgun_index;
            recovered_event.pellet_count = shotgun_expected;
            recovered_event.flags &=
                ~(GWP89_EVENT_FLAG_FIRST_PELLET |
                  GWP89_EVENT_FLAG_LAST_PELLET);
            if (shotgun_index == 0)
                recovered_event.flags |= GWP89_EVENT_FLAG_FIRST_PELLET;
            if (shotgun_index == shotgun_expected - 1)
                recovered_event.flags |= GWP89_EVENT_FLAG_LAST_PELLET;
            if (!shotgun_template_seen && shotgun_expected > 1)
                recovered_event.damage_fx /= (gwp89_fx)shotgun_expected;
            if (recovered_event.life_ms < B3D_SHOTGUN_MIN_LIFE_MS)
                recovered_event.life_ms = B3D_SHOTGUN_MIN_LIFE_MS;
            spawn_bullet_event(&recovered_event);
        }
        set_status("shotgun runtime: seven physical pellets + persistent life");
    }

    /* Defensive provider contract: a Gatling fire acceptance must always own a
       projectile. If a saturated/custom event bus dropped the visual request,
       synthesize the missing request from the accepted pose using the temporary
       machine-gun projectile assignment. Normal manager output never doubles. */
    if (gatling_fire_seen && !gatling_projectile_seen &&
        blank3d_systems_get_flag(&g.systems, "weapon.emit_projectile", 1)) {
        gatling_fire_event.type = GWP89_EVENT_PROJECTILE_REQUEST;
        gatling_fire_event.projectile_id = 2;
        gatling_fire_event.projectile_mesh_id = 2;
        gatling_fire_event.projectile_mesh_scale_fx =
            gwp89_fx_from_text("0.44");
        spawn_bullet_event(&gatling_fire_event);
        set_status("gatling projectile provider: recovered dropped request");
    }
}

static void update_weapon_system(void)
{
    GWP89_Vec3 socket_origin;
    GWP89_Vec3 socket_forward;
    GWP89_Vec3 socket_right;
    GWP89_Vec3 socket_up;
    GWP89_Vec3 camera_origin;
    GWP89_Vec3 camera_forward;
    int raw_trigger_down;
    int raw_trigger_pressed;
    int raw_trigger_released;
    int trigger_down;
    int trigger_pressed;
    int trigger_released;
    int view_style;
    int weapon_id;
    long charge_speed_q16;
    long charge_ratio_q16;
    unsigned int module_charge_ms;
    char charge_status[128];
    const Blank3DWeaponModules *modules;

    raw_trigger_down = (g.mouse_left || g.keys[VK_SPACE] ||
                        g.fire_requested) ? 1 : 0;
    raw_trigger_pressed = (g.mouse_left_pressed ||
                           g.key_pressed[VK_SPACE]) ? 1 : 0;
    raw_trigger_released = (g.mouse_left_released ||
                            g.key_released[VK_SPACE]) ? 1 : 0;
    if (g.fire_requested && !raw_trigger_pressed &&
        !g.systems.last_trigger_down)
        raw_trigger_pressed = 1;

    trigger_down = raw_trigger_down;
    trigger_pressed = raw_trigger_pressed;
    trigger_released = raw_trigger_released;
    weapon_id = blank3d_systems_weapon_id(&g.systems);
    modules = blank3d_weapon_modules_get(weapon_id);

    if (modules && modules->trigger_model == B3D_TRIGGER_SPINUP) {
        if (raw_trigger_pressed) {
            g.gatling_spin_ms = 0U;
            g.gatling_armed = 0;
            blank3d_audio_gatling_begin(&g.audio);
        }
        if (raw_trigger_down && !g.gatling_armed) {
            if (g.gatling_spin_ms + (unsigned int)g.frame_ms >=
                g.gatling_spinup_ms) {
                g.gatling_spin_ms = g.gatling_spinup_ms;
                g.gatling_armed = 1;
                trigger_down = 1;
                trigger_pressed = 1;
                trigger_released = 0;
            } else {
                g.gatling_spin_ms += (unsigned int)g.frame_ms;
                trigger_down = 0;
                trigger_pressed = 0;
                trigger_released = 0;
            }
        } else if (raw_trigger_down && g.gatling_armed) {
            trigger_down = 1;
            trigger_pressed = 0;
            trigger_released = 0;
        }
        if (!raw_trigger_down &&
            (raw_trigger_released || g.gatling_spin_ms > 0U ||
             g.gatling_armed || g.audio.gatling_active)) {
            trigger_down = 0;
            trigger_pressed = 0;
            trigger_released = g.gatling_armed ? 1 : 0;
            blank3d_audio_gatling_release(&g.audio);
            g.gatling_spin_ms = 0U;
            g.gatling_armed = 0;
        }
    } else {
        if (g.audio.gatling_active || g.gatling_spin_ms > 0U ||
            g.gatling_armed) {
            blank3d_audio_gatling_release(&g.audio);
            g.gatling_spin_ms = 0U;
            g.gatling_armed = 0;
        }
    }

    if (modules && modules->trigger_model == B3D_TRIGGER_CHARGE_RELEASE) {
        trigger_down = 0;
        trigger_pressed = 0;
        trigger_released = 0;
        if (raw_trigger_pressed) {
            g.slingshot_charging = 1;
            g.slingshot_charge_ms = 0U;
            set_status("slingshot: charging elastic provider");
        }
        if (raw_trigger_down && g.slingshot_charging) {
            if (g.slingshot_charge_ms + (unsigned int)g.frame_ms >=
                g.slingshot_charge_max_ms)
                g.slingshot_charge_ms = g.slingshot_charge_max_ms;
            else
                g.slingshot_charge_ms += (unsigned int)g.frame_ms;
        }
        if (raw_trigger_released && g.slingshot_charging) {
            module_charge_ms = g.slingshot_charge_ms;
            if (modules->charge_time_ms > 0U &&
                g.slingshot_charge_max_ms > 0U) {
                module_charge_ms = (unsigned int)(
                    ((unsigned long)g.slingshot_charge_ms *
                     (unsigned long)modules->charge_time_ms) /
                    (unsigned long)g.slingshot_charge_max_ms);
            }
            charge_speed_q16 = blank3d_weapon_modules_charge_speed_q16(
                weapon_id, module_charge_ms);
            charge_ratio_q16 = blank3d_weapon_modules_charge_ratio_q16(
                weapon_id, module_charge_ms);
            blank3d_systems_set_launch_speed_q16(&g.systems,
                                                  charge_speed_q16);
            trigger_down = 1;
            trigger_pressed = 1;
            trigger_released = 0;
            sprintf(charge_status,
                    "slingshot release: charge %ld%% speed %ld",
                    (charge_ratio_q16 * 100L) / 65536L,
                    charge_speed_q16 / 65536L);
            set_status(charge_status);
            g.slingshot_charging = 0;
            g.slingshot_charge_ms = 0U;
        }
    } else if (g.slingshot_charging) {
        g.slingshot_charging = 0;
        g.slingshot_charge_ms = 0U;
    }

    build_weapon_input_vectors(&socket_origin, &socket_forward,
                               &socket_right, &socket_up,
                               &camera_origin, &camera_forward);
    view_style = g.camera_mode == CAMERA_FPS
               ? GWP89_VIEW_FPS : GWP89_VIEW_OVER_SHOULDER;
    blank3d_systems_update(&g.systems, g.frame_ms,
                           trigger_down, trigger_pressed, trigger_released,
                           view_style, (gwp89_fx)q12_to_q16_long(g.zoom_fx),
                           &socket_origin, &socket_forward,
                           &socket_right, &socket_up,
                           &camera_origin, &camera_forward);
    process_weapon_events();
    g.fire_requested = 0;
}


static void clear_input_edges(void)
{
    memset(g.key_pressed, 0, sizeof(g.key_pressed));
    memset(g.key_released, 0, sizeof(g.key_released));
    g.mouse_left_pressed = 0;
    g.mouse_left_released = 0;
    g.mouse_wheel_delta = 0;
}


static void sync_collision_world(void)
{
    int i;
    for (i = 0; i < MAX_ENEMIES; ++i) {
        blank3d_collision_set_enemy(&g.collision, i,
            i < g.enemy_count && g.enemies[i].alive,
            (gwp89_fx)g.enemies[i].transform.position.x,
            (gwp89_fx)g.enemies[i].transform.position.y,
            (gwp89_fx)g.enemies[i].transform.position.z);
    }
    blank3d_collision_step(&g.collision);
}

static void apply_explosion_damage(const Vec3 *center, int weapon_id,
                                   int base_damage, int direct_enemy,
                                   int owner_actor_id)
{
    int i;
    int damage;
    g3d_fix radius;
    g3d_fix distance;
    Vec3 delta;
    if (!center) return;
    radius = weapon_id == 7 ? G3D_FIX_FROM_INT(8) : G3D_FIX_FROM_INT(5);
    for (i = 0; i < g.enemy_count; ++i) {
        if (!g.enemies[i].alive ||
            !b3d_faction_can_attack_actor(owner_actor_id,
                                           g.enemies[i].weapon_actor_id))
            continue;
        gamlib_vec3_sub(&delta, &g.enemies[i].transform.position, center);
        distance = gamlib_vec3_length(&delta);
        if (distance > radius && i != direct_enemy) continue;
        if (i == direct_enemy) damage = base_damage;
        else {
            damage = (int)(((long)base_damage *
                (long)g3d_fix_sub_sat(radius, distance)) / (long)radius);
            if (damage < 1) damage = 1;
        }
        b3d_damage_actor(owner_actor_id,
                         g.enemies[i].weapon_actor_id, damage);
    }
    if (b3d_faction_can_attack_actor(owner_actor_id,
                                      B3D_PLAYER_ACTOR_ID)) {
        gamlib_vec3_sub(&delta, &g.player.position, center);
        distance = gamlib_vec3_length(&delta);
        if (distance <= radius) {
            damage = (int)(((long)base_damage *
                (long)g3d_fix_sub_sat(radius, distance)) / (long)radius);
            if (damage < 1) damage = 1;
            b3d_damage_actor(owner_actor_id,
                             B3D_PLAYER_ACTOR_ID, damage);
        }
    }
    blank3d_audio_explosion(&g.audio, weapon_id,
                            base_damage > 100 ? 100 : base_damage);
}

static int b3d_enemy_bullet_hits_player(const Bullet *bullet,
                                         const Vec3 *start,
                                         const Vec3 *movement)
{
    Vec3 center;
    Vec3 rel;
    Vec3 closest;
    Vec3 delta;
    g3d_fix denom;
    g3d_fix t;
    g3d_fix radius;
    if (!bullet || !start || !movement) return 0;
    if (!b3d_faction_can_attack_actor(bullet->owner_actor_id,
                                       B3D_PLAYER_ACTOR_ID)) return 0;
    center = g.player.position;
    center.y = g3d_fix_add_sat(center.y, fix_ratio(9, 10));
    gamlib_vec3_sub(&rel, &center, start);
    denom = gamlib_vec3_dot(movement, movement);
    if (denom <= G3D_FIX_EPSILON) return 0;
    t = g3d_fix_div(gamlib_vec3_dot(&rel, movement), denom);
    if (t < 0) t = 0;
    if (t > G3D_FIX_ONE) t = G3D_FIX_ONE;
    gamlib_vec3_scale(&closest, movement, t);
    gamlib_vec3_add(&closest, start, &closest);
    gamlib_vec3_sub(&delta, &center, &closest);
    radius = g3d_fix_add_sat(fix_ratio(13, 20), bullet->radius);
    return gamlib_vec3_dot(&delta, &delta) <= g3d_fix_mul(radius, radius);
}

static void resolve_bullet_hit(Bullet *bullet,
                               const Blank3DCollisionHit *hit)
{
    Vec3 point;
    if (!bullet || !hit) return;
    point = gwp_to_vec3(hit->point);
    bullet->transform.position = point;
    if ((bullet->weapon_id == 6 || bullet->weapon_id == 7)) {
        apply_explosion_damage(&point, bullet->weapon_id,
                               bullet->damage, hit->enemy_index,
                               bullet->owner_actor_id);
    } else if (hit->enemy_index >= 0 &&
               hit->enemy_index < g.enemy_count &&
               g.enemies[hit->enemy_index].alive) {
        if (g.enemies[hit->enemy_index].weapon_actor_id ==
            bullet->owner_actor_id) {
            Vec3 forward;
            g3d_fix clearance;
            forward = bullet->velocity;
            if (gamlib_vec3_length(&forward) > G3D_FIX_EPSILON) {
                gamlib_vec3_normalize(&forward, &forward);
                clearance = g3d_fix_add_sat(
                    g3d_fix_mul(bullet->radius, G3D_FIX_FROM_INT(3)),
                    fix_ratio(1, 10));
                gamlib_vec3_scale(&forward, &forward, clearance);
                gamlib_vec3_add(&bullet->transform.position,
                                &point, &forward);
            }
            return;
        }
        if (b3d_faction_can_attack_actor(bullet->owner_actor_id,
                g.enemies[hit->enemy_index].weapon_actor_id)) {
            b3d_damage_actor(bullet->owner_actor_id,
                g.enemies[hit->enemy_index].weapon_actor_id,
                bullet->damage);
            blank3d_audio_impact(&g.audio, 1, bullet->damage);
        }
    } else {
        blank3d_audio_impact(&g.audio, 0, bullet->damage);
    }
    release_bullet_slot((int)(bullet - g.bullets));
}

static void update_projectiles(void)
{
    int i;
    int damage;
    int bullet_index;
    Vec3 movement;
    Vec3 gravity;
    GWP89_Vec3 start;
    GWP89_Vec3 delta;
    Blank3DCollisionHit hit;
    unsigned int collision_layers;
    B3D_Event bolt_event;
    const B3D_Projectile *bolt_projectile;
    Bullet *event_bullet;

    blank3d_bolt_update(&g.bolt, (gwp89_fx)g.dt);
    while (blank3d_bolt_poll_event(&g.bolt, &bolt_event)) {
        event_bullet = (Bullet *)bolt_event.source_user_ptr;
        if (!event_bullet) continue;
        bullet_index = (int)(event_bullet - g.bullets);
        if (bullet_index < 0 || bullet_index >= MAX_BULLETS) continue;
        if (bolt_event.type == B3D_EVENT_DAMAGE &&
            bolt_event.target_id >= 0 &&
            bolt_event.target_id < g.enemy_count &&
            g.enemies[bolt_event.target_id].alive) {
            damage = (int)(bolt_event.damage / 65536L);
            if (damage < 1) damage = event_bullet->damage;
            if (b3d_faction_can_attack_actor(
                    event_bullet->owner_actor_id,
                    g.enemies[bolt_event.target_id].weapon_actor_id)) {
                b3d_damage_actor(event_bullet->owner_actor_id,
                    g.enemies[bolt_event.target_id].weapon_actor_id,
                    damage);
                blank3d_audio_impact(&g.audio, 1, damage);
            }
        } else if (bolt_event.type == B3D_EVENT_BOUNCE ||
                   bolt_event.type == B3D_EVENT_HIT_WORLD) {
            blank3d_audio_impact(&g.audio, 0, event_bullet->damage);
        }
    }

    for (i = 0; i < MAX_BULLETS; ++i) {
        if (!g.bullets[i].alive) continue;
        g.bullets[i].previous_position = g.bullets[i].transform.position;

        if (g.bullets[i].physics_backend == B3D_PHYSICS_BOLT3D) {
            bolt_projectile = blank3d_bolt_get(
                &g.bolt, g.bullets[i].bolt_projectile_id);
            if (!bolt_projectile) {
                release_bullet_slot(i);
                continue;
            }
            g.bullets[i].transform.position.x =
                (g3d_fix)(bolt_projectile->position.x / 16L);
            g.bullets[i].transform.position.y =
                (g3d_fix)(bolt_projectile->position.y / 16L);
            g.bullets[i].transform.position.z =
                (g3d_fix)(bolt_projectile->position.z / 16L);
            g.bullets[i].velocity.x =
                (g3d_fix)(bolt_projectile->velocity.x / 16L);
            g.bullets[i].velocity.y =
                (g3d_fix)(bolt_projectile->velocity.y / 16L);
            g.bullets[i].velocity.z =
                (g3d_fix)(bolt_projectile->velocity.z / 16L);
            g.bullets[i].life = (g3d_fix)(
                (bolt_projectile->lifetime - bolt_projectile->age) / 16L);
            blank3d_trails_emit_q12(&g.trails, i,
                g.bullets[i].transform.position.x,
                g.bullets[i].transform.position.y,
                g.bullets[i].transform.position.z);
            publish_bullet(i);
            continue;
        }

        if (g.bullets[i].physics_backend == B3D_PHYSICS_GRAVITY) {
            gravity = gamlib_vec3(0, g.bullets[i].gravity, 0);
            gamlib_vec3_scale(&gravity, &gravity, g.dt);
            gamlib_vec3_add(&g.bullets[i].velocity,
                            &g.bullets[i].velocity, &gravity);
        }
        if (g.bullets[i].audio_primary_key != 0U ||
            g.bullets[i].audio_secondary_key != 0U) {
            g3d_fix projectile_speed;
            int projectile_speed_int;
            projectile_speed = gamlib_vec3_length(&g.bullets[i].velocity);
            projectile_speed_int = (int)(projectile_speed / G3D_FIX_ONE);
            if (projectile_speed_int < 1) projectile_speed_int = 1;
            blank3d_audio_projectile_motion(
                &g.audio, g.bullets[i].weapon_id, projectile_speed_int,
                g.bullets[i].audio_primary_key,
                g.bullets[i].audio_secondary_key);
        }
        gamlib_vec3_scale(&movement, &g.bullets[i].velocity, g.dt);
        start = vec3_to_gwp(g.bullets[i].transform.position);
        delta = vec3_to_gwp(movement);
        if (b3d_enemy_bullet_hits_player(&g.bullets[i],
                &g.bullets[i].transform.position, &movement)) {
            b3d_damage_actor(g.bullets[i].owner_actor_id,
                             B3D_PLAYER_ACTOR_ID,
                             g.bullets[i].damage);
            blank3d_audio_impact(&g.audio, 1, g.bullets[i].damage);
            release_bullet_slot(i);
        } else {
            /* All projectiles may sweep the shared actor layer. GFaction
               decides whether the contacted actor can receive damage; owner
               contacts are nudged out of the shooter's capsule. */
            collision_layers = B3D_COLLISION_LAYER_WORLD |
                               B3D_COLLISION_LAYER_ENEMY;
            if (blank3d_collision_sweep_bullet_mask(
                    &g.collision, &start, &delta,
                    (gwp89_fx)g.bullets[i].radius,
                    collision_layers, &hit)) {
                resolve_bullet_hit(&g.bullets[i], &hit);
            } else {
                gamlib_vec3_add(&g.bullets[i].transform.position,
                            &g.bullets[i].transform.position, &movement);
            g.bullets[i].life =
                g3d_fix_sub_sat(g.bullets[i].life, g.dt);
            if (g.bullets[i].life <= 0) {
                if (g.bullets[i].weapon_id == 6 ||
                    g.bullets[i].weapon_id == 7) {
                    apply_explosion_damage(&g.bullets[i].transform.position,
                        g.bullets[i].weapon_id, g.bullets[i].damage, -1,
                        g.bullets[i].owner_actor_id);
                }
                release_bullet_slot(i);
            }
            }
        }
        if (g.bullets[i].alive) {
            blank3d_trails_emit_q12(&g.trails, i,
                g.bullets[i].transform.position.x,
                g.bullets[i].transform.position.y,
                g.bullets[i].transform.position.z);
            publish_bullet(i);
        }
    }
    blank3d_trails_tick(&g.trails,
                        (int)(((unsigned int)g.frame_ms + 8U) / 16U));
}


static void update_casings(void)
{
    int i;
    Vec3 movement;
    Vec3 gravity;
    gravity = gamlib_vec3(0, G3D_FIX_FROM_INT(-12), 0);
    gamlib_vec3_scale(&gravity, &gravity, g.dt);
    for (i = 0; i < MAX_CASINGS; ++i) {
        if (!g.casings[i].alive) continue;
        if (!g.casings[i].physics_active) {
            gamlib_vec3_add(&g.casings[i].velocity,
                            &g.casings[i].velocity, &gravity);
            gamlib_vec3_scale(&movement, &g.casings[i].velocity, g.dt);
            gamlib_vec3_add(&g.casings[i].transform.position,
                            &g.casings[i].transform.position, &movement);
            if (g.casings[i].transform.position.y < fix_ratio(1, 20)) {
                g.casings[i].transform.position.y = fix_ratio(1, 20);
                if (g.casings[i].velocity.y < -fix_ratio(1, 4))
                    g.casings[i].velocity.y = g3d_fix_neg_sat(
                        g3d_fix_mul(g.casings[i].velocity.y,
                                    fix_ratio(1, 2)));
                else
                    g.casings[i].velocity.y = 0;
                g.casings[i].velocity.x = g3d_fix_mul(
                    g.casings[i].velocity.x, fix_ratio(3, 5));
                g.casings[i].velocity.z = g3d_fix_mul(
                    g.casings[i].velocity.z, fix_ratio(3, 5));
            }
            if (gamlib_vec3_length(&g.casings[i].velocity) >
                    fix_ratio(1, 20))
                transform_rotate(&g.casings[i].transform,
                    g3d_fix_mul(G3D_FIX_FROM_INT(180), g.dt),
                    g3d_fix_mul(G3D_FIX_FROM_INT(120), g.dt), 0);
        }
        g.casings[i].life = g3d_fix_sub_sat(g.casings[i].life, g.dt);
        if (g.casings[i].life <= 0) {
            if (g.casings[i].physics_active)
                blank3d_casing_physics_release(&g.casing_physics, i);
            g.casings[i].physics_active = 0;
            g.casings[i].alive = 0;
        }
    }
}

static void update_motion_attacks(void)
{
    int i;
    int grounded;
    Vec3 target;
    Vec3 facing;
    for (i = 0; i < g.enemy_count; ++i) {
        if (!g.enemies[i].alive) continue;
        if (blank3d_motion_attack_mode(&g.enemies[i].motion_attack) ==
            B3D_MOTION_ATTACK_NONE) continue;
        if (!b3d_enemy_target_position(&g.enemies[i], &target)) continue;
        target.y = g3d_fix_add_sat(target.y, fix_ratio(9, 10));
        grounded = blank3d_vertical_axis_grounded(&g.vertical_axis,
                                                   &g.enemies[i].vertical);
        if (blank3d_motion_attack_tick(&g.enemies[i].motion_attack,
                &g.enemies[i].transform, &target,
                g.enemies[i].vertical.floor_y, grounded, g.dt, &facing) &&
            gamlib_vec3_length(&facing) > G3D_FIX_EPSILON)
            set_game_yaw(&g.enemies[i].transform, fixed_atan2_xz(facing));
    }
}

static void update_vertical_axis(void)
{
    int i;
    blank3d_vertical_axis_tick(&g.vertical_axis,
                               &g.player_vertical, g.dt);
    for (i = 0; i < g.enemy_count; ++i) {
        if (!g.enemies[i].alive) continue;
        if (!blank3d_motion_attack_controls_vertical(
                &g.enemies[i].motion_attack))
            blank3d_vertical_axis_tick(&g.vertical_axis,
                                       &g.enemies[i].vertical, g.dt);
    }
}

static void b3d_publish_equipment_muzzle(int actor_id,
                                         soq3d_key socket_key)
{
    const nm89_matrix *world;
    soq3d_pose pose;
    world = blank3d_actor_equipment_muzzle_world(
        &g.actor_equipment, actor_id);
    if (world == 0) return;
    pose.position.x = (soq3d_fx)world->m[0][3];
    pose.position.y = (soq3d_fx)world->m[1][3];
    pose.position.z = (soq3d_fx)world->m[2][3];
    pose.basis.m00 = (soq3d_fx)world->m[0][0];
    pose.basis.m01 = (soq3d_fx)world->m[0][1];
    pose.basis.m02 = (soq3d_fx)world->m[0][2];
    pose.basis.m10 = (soq3d_fx)world->m[1][0];
    pose.basis.m11 = (soq3d_fx)world->m[1][1];
    pose.basis.m12 = (soq3d_fx)world->m[1][2];
    pose.basis.m20 = (soq3d_fx)world->m[2][0];
    pose.basis.m21 = (soq3d_fx)world->m[2][1];
    pose.basis.m22 = (soq3d_fx)world->m[2][2];
    (void)soq3d_publish_world_socket(&g.locator, socket_key, &pose);
}

static int b3d_actor_current_weapon_id(GWP89_Manager *manager,
                                        int actor_id)
{
    int user_slot;
    const GWP89_UserState *user;
    const GWP89_WeaponProfile *profile;
    if (manager == 0) return 0;
    user_slot = gwp89_find_user_slot(manager, actor_id);
    if (user_slot < 0) return 0;
    user = &manager->users[user_slot];
    profile = gwp89_get_weapon(manager, user->weapon_slot);
    return profile != 0 ? profile->weapon_id : 0;
}

static int b3d_build_player_aim_carrier(
    const soq3d_pose *raw_carrier,
    soq3d_pose *out_carrier)
{
    Vec3 eye;
    Vec3 forward;
    Vec3 right;
    Vec3 up;
    Vec3 carrier_origin;
    Vec3 hit_target;
    GWP89_Vec3 eye_q12;
    GWP89_Vec3 forward_q12;
    Blank3DCollisionHit hit;
    Blank3DUniversalAimFrame frame;
    int hit_valid;

    if (raw_carrier == 0 || out_carrier == 0) return 0;
    build_view_vectors(&eye, &forward, &right, &up);
    carrier_origin = bridge_pose_position_q20(raw_carrier);
    eye_q12 = vec3_to_gwp(eye);
    forward_q12 = vec3_to_gwp(forward);
    hit_valid = blank3d_collision_raycast(
        &g.collision, &eye_q12, &forward_q12,
        (gwp89_fx)B3D_UNIVERSAL_AIM_ZERO_DISTANCE,
        B3D_COLLISION_LAYER_WORLD | B3D_COLLISION_LAYER_ENEMY,
        &hit);
    if (hit_valid)
        hit_target = gwp_to_vec3(hit.point);
    else
        hit_target = eye;

    if (!blank3d_universal_aim_build(
            &eye, &forward, &right, &up,
            &carrier_origin,
            hit_valid ? &hit_target : 0,
            hit_valid,
            B3D_UNIVERSAL_AIM_ZERO_DISTANCE,
            &frame))
        return 0;
    return blank3d_universal_aim_apply_pose(raw_carrier,
                                             &frame,
                                             out_carrier);
}

static void sync_actor_equipment(void)
{
    soq3d_pose carrier_socket;
    int i;
    int actor_id;
    int weapon_id;

    if (!g.actor_equipment.initialized || !g.attachments.initialized)
        return;

    /* begin_locator_frame() runs before movement. Republish the final actor
       transforms here so equipment and its visual muzzle do not trail walking
       actors by one frame. */
    publish_transform(g.player_key, &g.player);
    for (i = 0; i < g.enemy_count; ++i) {
        if (g.enemies[i].alive)
            publish_transform(g.enemy_keys[i], &g.enemies[i].transform);
    }

    if (soq3d_get_socket(&g.locator, g.player_weapon_socket,
                         g.frame_stamp, &carrier_socket) == SOQ3D_OK) {
        soq3d_pose aimed_carrier;
        /* Every player weapon uses this one camera-to-crosshair carrier basis.
           The actor socket still owns position, but it no longer exports a
           body-only yaw as the weapon's ballistic/presentation direction. */
        if (b3d_build_player_aim_carrier(&carrier_socket, &aimed_carrier))
            carrier_socket = aimed_carrier;
        (void)blank3d_attachment_publish_socket_pose(
            &g.attachments, B3D_PLAYER_ACTOR_ID,
            B3D_ATTACH89_SOCKET_WEAPON_R, &carrier_socket);
    }
    weapon_id = blank3d_systems_weapon_id(&g.systems);
    if (weapon_id > 0)
        (void)blank3d_actor_equipment_equip(&g.actor_equipment,
            B3D_PLAYER_ACTOR_ID, B3D_EQUIPMENT_ACTOR_PLAYER, weapon_id);

    for (i = 0; i < g.enemy_count; ++i) {
        if (!g.enemies[i].alive) continue;
        actor_id = g.enemies[i].weapon_actor_id;
        if (soq3d_get_socket(&g.locator, g.enemy_weapon_sockets[i],
                             g.frame_stamp, &carrier_socket) == SOQ3D_OK) {
            (void)blank3d_attachment_publish_socket_pose(
                &g.attachments, actor_id,
                B3D_ATTACH89_SOCKET_WEAPON_R, &carrier_socket);
        }
        if (!g.enemies[i].weapon_ready &&
            (strcmp(g.enemies[i].archetype, "gunner_enemy") == 0 ||
             strcmp(g.enemies[i].archetype, "armed_ally") == 0))
            (void)b3d_enemy_ensure_weapon_inventory(&g.enemies[i]);
        if (!g.enemies[i].weapon_ready) continue;
        weapon_id = b3d_actor_current_weapon_id(&g.npc_weapons, actor_id);
        if (weapon_id > 0)
            (void)blank3d_actor_equipment_equip(&g.actor_equipment,
                actor_id, g.enemies[i].faction_is_ally
                        ? B3D_EQUIPMENT_ACTOR_ALLY
                        : B3D_EQUIPMENT_ACTOR_ENEMY, weapon_id);
    }

    if (!blank3d_attachment_update(&g.attachments)) return;

    (void)blank3d_actor_equipment_sync_actor(&g.actor_equipment,
        &g.systems.weapons, B3D_PLAYER_ACTOR_ID, B3D_EQUIPMENT_ACTOR_PLAYER,
        g.frame_ms);
    b3d_publish_equipment_muzzle(B3D_PLAYER_ACTOR_ID,
                                 g.player_muzzle_socket);

    for (i = 0; i < g.enemy_count; ++i) {
        if (!g.enemies[i].alive || !g.enemies[i].weapon_ready) continue;
        actor_id = g.enemies[i].weapon_actor_id;
        (void)blank3d_actor_equipment_sync_actor(&g.actor_equipment,
            &g.npc_weapons, actor_id,
            g.enemies[i].faction_is_ally
                ? B3D_EQUIPMENT_ACTOR_ALLY
                : B3D_EQUIPMENT_ACTOR_ENEMY,
            g.frame_ms);
        b3d_publish_equipment_muzzle(actor_id, g.enemy_muzzle_sockets[i]);
    }
}

static void update_world(void)
{
    int i;

    begin_locator_frame();

    if (g.key_pressed[VK_ESCAPE]) g.running = 0;
    if (g.key_pressed[VK_F1]) {
        g.lock_mouse = !g.lock_mouse;
        set_mouse_capture_state(g.lock_mouse && g.active);
        set_status(g.lock_mouse
                 ? "mouse-look captured: F1 releases"
                 : "mouse-look released: F1 captures");
    }
    update_input_mouse();
    blank3d_input_set_mouse_buttons(&g.input,
                                    g.mouse_left, g.mouse_right,
                                    0, 0, 0);
    blank3d_input_update(&g.input);
    sync_collision_world();
    b3d_faction_update_targets();
    b3d_perception_tick();
    blank3d_objects_tick(&g.objects);
    update_motion_attacks();
    update_vertical_axis();

    if (g.key_pressed['V']) {
        g.camera_mode = g.camera_mode == CAMERA_FPS ? CAMERA_TPS : CAMERA_FPS;
        blank3d_cameranaku_set_mode(&g.cameranaku,
            g.camera_mode == CAMERA_FPS
            ? B3D_CNK_CAMERA_FPS : B3D_CNK_CAMERA_TPS);
        set_status(g.camera_mode == CAMERA_FPS
                 ? "CamNaku: first person provider"
                 : "CamNaku: orbit/OTS provider");
    }
    if (g.key_pressed['R']) blank3d_systems_reload(&g.systems);
    if (g.key_pressed['H']) blank3d_systems_active_reload(&g.systems);
    if (g.key_pressed['B']) {
        blank3d_vphysics_set_enabled(&g.physics,
            !blank3d_vphysics_is_enabled(&g.physics));
        set_status(blank3d_vphysics_is_enabled(&g.physics)
                 ? "VPhysics providers enabled"
                 : "VPhysics providers paused");
    }
#if B3D_VPHYSICS_DEMO
    if (g.key_pressed['P']) {
        b3d_clear_casings();
        blank3d_vphysics_spawn_demo(&g.physics);
        set_status("VPhysics demo reset: transform/collision providers active");
    }
    if (g.key_pressed['O']) {
        (void)blank3d_vphysics_add_impulse_q12(&g.physics, 70001UL,
            2L * G3D_FIX_ONE, 5L * G3D_FIX_ONE, -G3D_FIX_ONE);
        set_status("VPhysics impulse sent through provider object 70001");
    }
#endif

    if (g.key_pressed['1']) blank3d_systems_equip_id(&g.systems, 1);
    if (g.key_pressed['2']) blank3d_systems_equip_id(&g.systems, 3);
    if (g.key_pressed['3']) blank3d_systems_equip_id(&g.systems, 2);
    if (g.key_pressed['4']) blank3d_systems_equip_id(&g.systems, 4);
    if (g.key_pressed['5']) blank3d_systems_equip_id(&g.systems, 8);
    if (g.key_pressed['6']) blank3d_systems_equip_id(&g.systems, 5);
    if (g.key_pressed['7']) blank3d_systems_equip_id(&g.systems, 6);
    if (g.key_pressed['8']) blank3d_systems_equip_id(&g.systems, 7);
    if (g.key_pressed['9']) blank3d_systems_equip_id(&g.systems, 9);
    if (g.mouse_wheel_delta > 0)
        (void)blank3d_list_cycle_prev(&g.list_cycles, "active_weapon", 0);
    if (g.mouse_wheel_delta < 0)
        (void)blank3d_list_cycle_next(&g.list_cycles, "active_weapon", 0);

    if (g.key_pressed[VK_F5]) {
        load_rpy();
        (void)blank3d_languages_reload_ddsl2(SCRIPT_DDSL2);
        (void)blank3d_languages_reload_fpil(SCRIPT_FPI);
        apply_camera_config();
        g.ddsl2_stamp = get_stamp(SCRIPT_DDSL2);
        g.fpi_stamp = get_stamp(SCRIPT_FPI);
        set_status("RPYL + DDSL2 + FPIL vendor runtimes reloaded");
    }
    if (stamp_changed(&g.rpy_stamp, SCRIPT_RPY)) {
        load_rpy();
        apply_camera_config();
    }
    if (stamp_changed(&g.ddsl2_stamp, SCRIPT_DDSL2)) {
        if (blank3d_languages_reload_ddsl2(SCRIPT_DDSL2))
            set_status("player.ddsl2 hot reloaded by vendored DDSL2");
        else
            set_status(blank3d_languages_status());
    }
    if (stamp_changed(&g.fpi_stamp, SCRIPT_FPI)) {
        if (blank3d_languages_reload_fpil(SCRIPT_FPI))
            set_status("enemy.fpi hot reloaded by vendored FPIL");
        else
            set_status(blank3d_languages_status());
    }

    for (i = 0; i < g.enemy_count; ++i) {
        if (!g.enemies[i].alive) continue;
        if (g.enemies[i].cooldown > 0)
            g.enemies[i].cooldown =
                g3d_fix_sub_sat(g.enemies[i].cooldown, g.dt);
    }

    update_zoom();
    update_cameranaku();
    sync_collision_world();
#if B3D_VPHYSICS_DEMO
    blank3d_vphysics_update_demo(&g.physics, g.time);
#endif
    blank3d_vphysics_step_q12(&g.physics, g.dt);
    sync_actor_equipment();
    update_weapon_system();
    update_projectiles();
    update_casings();

    if (g.muzzle_flash_ms > 0) {
        if ((int)g.frame_ms >= g.muzzle_flash_ms) g.muzzle_flash_ms = 0;
        else g.muzzle_flash_ms -= (int)g.frame_ms;
    }
    if (g.player_damage_flash_ms > 0) {
        if ((int)g.frame_ms >= g.player_damage_flash_ms)
            g.player_damage_flash_ms = 0;
        else
            g.player_damage_flash_ms -= (int)g.frame_ms;
    }
    g.player_hp = blank3d_systems_player_health(&g.systems);
    if (g.player_hp <= 0) {
        if (!g.player_down_latched) {
            set_status("player down: actor-local weapon gate active");
            g.player_down_latched = 1;
        }
    } else {
        g.player_down_latched = 0;
    }

    blank3d_audio_pump(&g.audio);
    publish_all();
    clear_input_edges();
}

static void set_camera(void)
{
    Vec3 eye;
    Vec3 forward;
    Vec3 right;
    Vec3 local_up;
    Vec3 target;
    Vec3 up;
    Vec3 offset;
    g3d_fix view[16];
    g3d_fix projection[16];
    g3d_fix aspect;

    aspect = fix_ratio(g.width, g.height ? g.height : 1);
    camera_set_aspect(&g.camera, aspect);
    g.camera.fov_y_deg = g.fov;
    g.camera.z_near = g.near_z;
    g.camera.z_far = g.far_z;
    camera_build_proj_matrix(&g.camera, projection);

    build_view_vectors(&eye, &forward, &right, &local_up);
    (void)right;
    (void)local_up;
    target = eye;
    gamlib_vec3_scale(&offset, &forward, G3D_FIX_FROM_INT(12));
    gamlib_vec3_add(&target, &target, &offset);
    up = gamlib_vec3(0, G3D_FIX_ONE, 0);
    gamlib_mat4_lookat(view, &eye, &target, &up);

    glMatrixMode(GL_PROJECTION);
    bridge_gl_load_q20_matrix(projection);
    glMatrixMode(GL_MODELVIEW);
    bridge_gl_load_q20_matrix(view);
}

static void draw_socket_mesh(soq3d_key socket_key, const g3d_mesh *mesh)
{
    soq3d_pose pose;
    if (soq3d_get_socket(&g.locator, socket_key,
                         g.frame_stamp, &pose) != SOQ3D_OK) return;
    glPushMatrix();
    bridge_gl_apply_pose(&pose);
    bridge_gl_draw_mesh(mesh);
    glPopMatrix();
}

static void draw_thing_mesh(soq3d_key thing_key, const g3d_mesh *mesh)
{
    soq3d_pose pose;
    if (soq3d_get_thing(&g.locator, thing_key,
                        g.frame_stamp, &pose) != SOQ3D_OK) return;
    glPushMatrix();
    bridge_gl_apply_pose(&pose);
    bridge_gl_draw_mesh(mesh);
    glPopMatrix();
}


static void draw_transform_mesh(const Transform *transform,
                                const g3d_mesh *mesh)
{
    soq3d_pose pose;
    if (!transform || !mesh) return;
    bridge_transform_to_pose(transform, &pose);
    glPushMatrix();
    bridge_gl_apply_pose(&pose);
    bridge_gl_draw_mesh(mesh);
    glPopMatrix();
}

static const g3d_mesh *casing_mesh_for_casing(const Casing *casing)
{
    int index;
    if (!casing) return &g.meshes.casing[0];
    index = casing->casing_mesh_id - 1;
    if (index < 0 || index >= CASING_MESH_COUNT) index = 0;
    return &g.meshes.casing[index];
}

static const g3d_mesh *projectile_mesh_for_bullet(const Bullet *bullet)
{
    int index;
    if (!bullet) return &g.meshes.projectile[0];
    index = bullet->projectile_mesh_id - 1;
    if (index < 0 || index >= PROJECTILE_MESH_COUNT) index = 0;
    return &g.meshes.projectile[index];
}

static const g3d_mesh *mechanical_weapon_mesh(int model_id, int part_id)
{
    /* model_id is the opaque GWeapon/GAttach resource selector.  The current
       demo has one procedural multipart fallback; a real OBJ/GFO/model
       provider can switch on model_id here without changing equipment or
       animation code. */
    (void)model_id;
    if (part_id < 0 || part_id >= B3D_MECH89_MESH_COUNT) return 0;
    return &g.meshes.mechanical_weapon[part_id];
}

static void draw_actor_weapon(int actor_id)
{
    int i;
    int count;
    const nm89_geometry_packet *packet;
    const g3d_mesh *mesh;

    count = blank3d_actor_equipment_packet_count(
        &g.actor_equipment, actor_id);
    for (i = 0; i < count; ++i) {
        packet = blank3d_actor_equipment_packet(
            &g.actor_equipment, actor_id, i);
        if (!packet || !packet->visible) continue;
        mesh = mechanical_weapon_mesh(packet->resource_id, packet->mesh_id);
        if (!mesh) continue;
        glPushMatrix();
        bridge_gl_apply_q16_matrix(packet->world.m);
        bridge_gl_draw_mesh(mesh);
        glPopMatrix();
    }
}

#if B3D_VPHYSICS_DEBUG_DRAW
static void draw_vphysics_objects(void)
{
    int i;
    int count;
    signed int matrix[4][4];
    const Blank3DVPhysicsObject *object;
    count = blank3d_vphysics_object_count(&g.physics);
    for (i = 0; i < count; ++i) {
        object = blank3d_vphysics_object_at(&g.physics, i);
        if (!object || !object->used || !object->debug_draw) continue;
        if (!blank3d_vphysics_get_draw_matrix_q16(&g.physics,
                object->external_id, matrix)) continue;
        glPushMatrix();
        bridge_gl_apply_q16_matrix((const signed int (*)[4])matrix);
        bridge_gl_draw_mesh(&g.meshes.enemy_head);
        glPopMatrix();
    }
}

#endif

static void draw_scene(void)
{
    int i;
    Vec3 trail_camera;
    bridge_gl_begin_frame(g.width, g.height);
    blank3d_numbar_begin_frame(&g.numbars);
    blank3d_objects_render(&g.objects);
    set_camera();
    if (g.show_grid) bridge_gl_draw_grid(30);

    if (g.camera_mode == CAMERA_TPS) {
        draw_socket_mesh(g.player_body_socket, &g.meshes.player_body);
        draw_actor_weapon(B3D_PLAYER_ACTOR_ID);
    }
    for (i = 0; i < g.enemy_count; ++i) {
        if (!g.enemies[i].alive) continue;
        draw_socket_mesh(g.enemy_body_sockets[i],
            g.enemies[i].faction_is_ally
                ? &g.meshes.ally_body : &g.meshes.enemy_body);
        draw_socket_mesh(g.enemy_head_sockets[i],
            g.enemies[i].faction_is_ally
                ? &g.meshes.ally_head : &g.meshes.enemy_head);
        draw_actor_weapon(g.enemies[i].weapon_actor_id);
    }
    for (i = 0; i < MAX_BULLETS; ++i) {
        if (!g.bullets[i].alive) continue;
        draw_thing_mesh(g.bullet_keys[i],
                        projectile_mesh_for_bullet(&g.bullets[i]));
        if (g.bullets[i].aoi_trail_id < 0 &&
            g.bullets[i].trail_id != 0)
            bridge_gl_draw_segment(&g.bullets[i].previous_position,
                                   &g.bullets[i].transform.position,
                                   255U, 220U, 96U);
    }
    build_view_vectors(&trail_camera, 0, 0, 0);
    blank3d_trails_draw(&g.trails, trail_camera.x,
                        trail_camera.y, trail_camera.z);
    for (i = 0; i < MAX_CASINGS; ++i) {
        signed int casing_matrix[4][4];
        if (!g.casings[i].alive) continue;
        if (g.casings[i].physics_active &&
            blank3d_casing_physics_get_draw_matrix_q16(
                &g.casing_physics, i, g.casings[i].mesh_scale,
                casing_matrix)) {
            glPushMatrix();
            bridge_gl_apply_q16_matrix(
                (const signed int (*)[4])casing_matrix);
            bridge_gl_draw_mesh(casing_mesh_for_casing(&g.casings[i]));
            glPopMatrix();
        } else {
            draw_transform_mesh(&g.casings[i].transform,
                                casing_mesh_for_casing(&g.casings[i]));
        }
    }
#if B3D_VPHYSICS_DEBUG_DRAW
    draw_vphysics_objects();
#endif
    blank3d_hud_draw(&g.hud, &g.numbars, g.width, g.height,
                     g.player_hp,
                     blank3d_systems_clip(&g.systems),
                     current_clip_capacity(),
                     blank3d_systems_reserve(&g.systems),
                     g.camera_mode == CAMERA_FPS,
                     g.muzzle_flash_ms > 0,
                     b3d_player_threat_level(),
                     (unsigned int)g.player_damage_flash_ms,
                     (unsigned int)g.frame_ms,
                     &g.sniper);
    SwapBuffers(g.device);
}

static LRESULT CALLBACK window_proc(HWND window, UINT message, WPARAM wparam, LPARAM lparam)
{
    switch (message) {
    case WM_ACTIVATE:
        g.active = (LOWORD(wparam) != WA_INACTIVE);
        set_mouse_capture_state(g.active && g.lock_mouse);
        return 0;
    case WM_SETFOCUS:
        g.active = 1;
        set_mouse_capture_state(g.lock_mouse);
        return 0;
    case WM_KILLFOCUS:
        g.active = 0;
        set_mouse_capture_state(0);
        return 0;
    case WM_SIZE:
        g.width = LOWORD(lparam);
        g.height = HIWORD(lparam);
        if (g.width > 0 && g.height > 0) {
            blank3d_sniper_set_screen(&g.sniper,
                                      (short)g.width, (short)g.height);
            blank3d_cameranaku_resize(&g.cameranaku,
                                      g.width, g.height);
        }
        return 0;
    case WM_CLOSE:
        set_mouse_capture_state(0);
        g.running = 0;
        PostQuitMessage(0);
        return 0;
    case WM_DESTROY:
        set_mouse_capture_state(0);
        g.running = 0;
        PostQuitMessage(0);
        return 0;
    case WM_KEYDOWN:
        if (wparam < 256U) {
            if (!g.keys[wparam]) g.key_pressed[wparam] = 1;
            g.keys[wparam] = 1;
        }
        return 0;
    case WM_KEYUP:
        if (wparam < 256U) {
            g.keys[wparam] = 0;
            g.key_released[wparam] = 1;
        }
        return 0;
    case WM_LBUTTONDOWN:
        if (!g.mouse_captured) {
            g.lock_mouse = 1;
            set_mouse_capture_state(1);
        }
        if (!g.mouse_left) g.mouse_left_pressed = 1;
        g.mouse_left = 1;
        SetCapture(window);
        return 0;
    case WM_LBUTTONUP:
        g.mouse_left = 0;
        g.mouse_left_released = 1;
        ReleaseCapture();
        return 0;
    case WM_RBUTTONDOWN:
        if (!g.mouse_captured) {
            g.lock_mouse = 1;
            set_mouse_capture_state(1);
        }
        g.mouse_right = 1;
        return 0;
    case WM_RBUTTONUP:
        g.mouse_right = 0;
        return 0;
    case WM_MOUSEWHEEL:
        g.mouse_wheel_delta += GET_WHEEL_DELTA_WPARAM(wparam);
        return 0;
    default:
        break;
    }
    return DefWindowProc(window, message, wparam, lparam);
}

static int create_gl_window(HINSTANCE instance)
{
    WNDCLASSA window_class;
    DWORD style;
    RECT rect;
    PIXELFORMATDESCRIPTOR descriptor;
    int pixel_format;

    memset(&window_class, 0, sizeof(window_class));
    window_class.style = CS_OWNDC;
    window_class.lpfnWndProc = window_proc;
    window_class.hInstance = instance;
    window_class.lpszClassName = "MonikaBlank3D";
    window_class.hCursor = LoadCursor(0, IDC_ARROW);
    if (!RegisterClassA(&window_class)) return 0;

    style = WS_OVERLAPPEDWINDOW | WS_VISIBLE;
    rect.left = 0;
    rect.top = 0;
    rect.right = g.width;
    rect.bottom = g.height;
    AdjustWindowRect(&rect, style, 0);
    g.window = CreateWindowA("MonikaBlank3D",
        "Blank3D - CamNaku89 Provider + Universal Weapon Stack",
        style, CW_USEDEFAULT, CW_USEDEFAULT,
        rect.right - rect.left, rect.bottom - rect.top,
        0, 0, instance, 0);
    if (!g.window) return 0;

    g.device = GetDC(g.window);
    memset(&descriptor, 0, sizeof(descriptor));
    descriptor.nSize = sizeof(descriptor);
    descriptor.nVersion = 1;
    descriptor.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    descriptor.iPixelType = PFD_TYPE_RGBA;
    descriptor.cColorBits = 32;
    descriptor.cDepthBits = 24;
    descriptor.iLayerType = PFD_MAIN_PLANE;
    pixel_format = ChoosePixelFormat(g.device, &descriptor);
    if (!pixel_format) return 0;
    if (!SetPixelFormat(g.device, pixel_format, &descriptor)) return 0;
    g.gl_context = wglCreateContext(g.device);
    if (!g.gl_context) return 0;
    if (!wglMakeCurrent(g.device, g.gl_context)) return 0;
    return 1;
}

static void apply_camera_config(void)
{
    g.camera_mode = g.config.start_first_person ? CAMERA_FPS : CAMERA_TPS;
    g.lock_mouse = g.config.lock_mouse;
    g.mouse_sensitivity = q16_to_q12(g.config.mouse_sensitivity_q16);
    g.camera_pitch_min = q16_to_q12(g.config.pitch_min_q16);
    g.camera_pitch_max = q16_to_q12(g.config.pitch_max_q16);
    g.camera_dist = q16_to_q12(g.config.camera_distance_q16);
    g.camera_height = q16_to_q12(g.config.camera_height_q16);
    g.camera_shoulder = q16_to_q12(g.config.camera_shoulder_q16);
    g.camera_eye_height = q16_to_q12(g.config.eye_height_q16);
    g.base_fov = g.fov;
    g.zoom_fov = q16_to_q12(g.config.zoom_fov_q16);
    g.sniper_zoom_fov = q16_to_q12(g.config.sniper_zoom_fov_q16);
    g.zoom_speed = q16_to_q12(g.config.zoom_speed_q16);
    if (g.zoom_fov <= 0) g.zoom_fov = G3D_FIX_FROM_INT(45);
    if (g.sniper_zoom_fov <= 0) g.sniper_zoom_fov = G3D_FIX_FROM_INT(18);
    if (g.zoom_speed <= 0) g.zoom_speed = G3D_FIX_FROM_INT(120);
    g.camera_yaw = g.player.rotation.y;
    blank3d_cameranaku_configure(&g.cameranaku,
                                 g.camera_eye_height,
                                 g.camera_height,
                                 g.camera_dist,
                                 g.camera_shoulder,
                                 g.camera_pitch_min,
                                 g.camera_pitch_max,
                                 g.base_fov,
                                 g.near_z,
                                 g.far_z);
    blank3d_cameranaku_set_angles(&g.cameranaku,
                                  g.camera_yaw,
                                  g.camera_pitch);
    blank3d_cameranaku_set_mode(&g.cameranaku,
        g.camera_mode == CAMERA_FPS
        ? B3D_CNK_CAMERA_FPS : B3D_CNK_CAMERA_TPS);
    configure_sockets();
}

static void apply_runtime_config(void)
{
    blank3d_config_load(&g.config, CONFIG_FILE);
    apply_camera_config();
    blank3d_vertical_axis_configure_gravity(
        &g.vertical_axis,
        g.config.gravity_enabled,
        q16_to_q12(g.config.gravity_fall_speed_q16),
        q16_to_q12(g.config.jump_gravity_q16));
    if (g.config.gatling_spinup_ms < 0)
        g.config.gatling_spinup_ms = 0;
    if (g.config.gatling_spinup_ms > 5000)
        g.config.gatling_spinup_ms = 5000;
    g.gatling_spinup_ms = (unsigned int)g.config.gatling_spinup_ms;
    if (g.config.slingshot_charge_ms < 100)
        g.config.slingshot_charge_ms = 100;
    if (g.config.slingshot_charge_ms > 5000)
        g.config.slingshot_charge_ms = 5000;
    g.slingshot_charge_max_ms =
        (unsigned int)g.config.slingshot_charge_ms;

    gwp89_set_ammo(&g.systems.weapons, B3D_PLAYER_ACTOR_ID, 1, g.config.ammo_9mm);
    gwp89_set_ammo(&g.systems.weapons, B3D_PLAYER_ACTOR_ID, 2, g.config.ammo_shells);
    gwp89_set_ammo(&g.systems.weapons, B3D_PLAYER_ACTOR_ID, 3, g.config.ammo_magnum);
    gwp89_set_ammo(&g.systems.weapons, B3D_PLAYER_ACTOR_ID, 4, g.config.ammo_sniper);
    gwp89_set_ammo(&g.systems.weapons, B3D_PLAYER_ACTOR_ID, 5, g.config.ammo_grenades);
    gwp89_set_ammo(&g.systems.weapons, B3D_PLAYER_ACTOR_ID, 6, g.config.ammo_rockets);
    gwp89_set_ammo(&g.systems.weapons, B3D_PLAYER_ACTOR_ID, 7, g.config.ammo_gatling);
    gwp89_set_ammo(&g.systems.weapons, B3D_PLAYER_ACTOR_ID, 8, g.config.ammo_stones);
    blank3d_systems_set_multiplier_q16(&g.systems, GWP89_NUM_DAMAGE_FX,
                                       g.config.damage_multiplier_q16);
    blank3d_systems_set_multiplier_q16(&g.systems, GWP89_NUM_SPEED_FX,
                                       g.config.speed_multiplier_q16);
    blank3d_systems_set_multiplier_q16(&g.systems, GWP89_NUM_RECOIL_FX,
                                       g.config.recoil_multiplier_q16);
    blank3d_systems_set_flag(&g.systems, "weapon.enabled", g.config.flag_weapon_enabled);
    blank3d_systems_set_flag(&g.systems, "weapon.can_fire", g.config.flag_can_fire);
    blank3d_systems_set_flag(&g.systems, "weapon.can_reload", g.config.flag_can_reload);
    blank3d_systems_set_flag(&g.systems, "weapon.active_reload", g.config.flag_active_reload);
    blank3d_systems_equip_id(&g.systems, g.config.initial_weapon);
}

int WINAPI WinMain(HINSTANCE instance, HINSTANCE previous, LPSTR command_line, int show)
{
    MSG message;
    DWORD last_tick;
    DWORD now_tick;
    DWORD frame_ms;
    int i;

    (void)previous;
    (void)command_line;
    (void)show;
    memset(&g, 0, sizeof(g));
    g.instance = instance;
    g.running = 1;
    soq3d_init(&g.locator);
    initialize_keys();
    blank3d_vertical_axis_init_gamlib3d(&g.vertical_axis);
    default_world();
    blank3d_fire_frame_sync_init(&g.fire_frame_sync);
    blank3d_cameranaku_init(&g.cameranaku,
                             g.width, g.height,
                             g.fov, g.near_z, g.far_z);
    if (!initialize_meshes()) return 2;
    blank3d_attachment_init(&g.attachments);
    blank3d_weapon_presentation_registry_init(&g.weapon_presentations);
    blank3d_input_init(&g.input);
    blank3d_init_language_runtime();
    blank3d_init_object_runtime();
    blank3d_perception_world_init(&g.perception,
                                  b3d_perception_raycast,
                                  &g.collision);
    (void)blank3d_languages_reload_ddsl2(SCRIPT_DDSL2);
    (void)blank3d_languages_reload_fpil(SCRIPT_FPI);
    load_rpy();
    blank3d_systems_init(&g.systems);
    blank3d_list_cycle_init(&g.list_cycles);
    (void)blank3d_systems_register_cycle_lists(&g.systems, &g.list_cycles);
    b3d_reset_npc_weapon_manager();
    {
        char presentation_status[160];
        presentation_status[0] = '\0';
        if (blank3d_weapon_presentation_load_manifest(
                &g.weapon_presentations,
                "config/weapons/weapons.ini",
                presentation_status, sizeof(presentation_status)) <= 0)
            return 3;
        blank3d_actor_equipment_init(&g.actor_equipment,
            &g.attachments, &g.weapon_presentations);
        if (!blank3d_actor_equipment_define_socket(
                &g.actor_equipment, B3D_PLAYER_ACTOR_ID, "weapon_r",
                B3D_ATTACH89_SOCKET_WEAPON_R, 0))
            return 3;
        for (i = 0; i < MAX_ENEMIES; ++i) {
            if (!blank3d_actor_equipment_define_socket(
                    &g.actor_equipment, 1000 + i, "weapon_r",
                    B3D_ATTACH89_SOCKET_WEAPON_R, 0))
                return 3;
        }
    }
    blank3d_collision_init(&g.collision);
    if (!blank3d_vphysics_init(&g.physics, &g.collision)) return 4;
    blank3d_casing_physics_init(&g.casing_physics, &g.physics);
#if B3D_VPHYSICS_DEMO
    blank3d_vphysics_spawn_demo(&g.physics);
#endif
    blank3d_bolt_init(&g.bolt, &g.collision);
    blank3d_trails_init(&g.trails);
    (void)gwp89_add_provider(&g.systems.weapons, GWP89_SERVICE_RAYCAST,
        120, "blank3d.ccs+sicol", &g.collision,
        blank3d_collision_weapon_provider);
    (void)gwp89_add_provider(&g.systems.weapons, GWP89_SERVICE_CAMERA,
        140, "cameranaku89.receive-provider", &g.cameranaku,
        blank3d_cameranaku_weapon_provider);
    apply_runtime_config();
    update_cameranaku();
    {
        const Blank3DWeaponModules *sniper_modules;
        short base_fov_x100;
        sniper_modules = blank3d_weapon_modules_get(B3D_WEAPON_ID_SNIPER);
        base_fov_x100 = (short)(((long)g.base_fov * 100L) /
                                (long)G3D_FIX_ONE);
        blank3d_sniper_init(&g.sniper, (short)g.width, (short)g.height,
                            base_fov_x100, blank3d_sniper_raycast,
                            &g.collision,
                            sniper_modules
                            ? sniper_modules->scope_preset_name : 0);
    }
    blank3d_hud_init(&g.hud);
    g.player_hp = blank3d_systems_player_health(&g.systems);
    g.rpy_stamp = get_stamp(SCRIPT_RPY);
    g.ddsl2_stamp = get_stamp(SCRIPT_DDSL2);
    g.fpi_stamp = get_stamp(SCRIPT_FPI);
    if (!create_gl_window(instance)) return 1;
    g.active = 1;
    if (!blank3d_audio_init(&g.audio, g.config.audio_enabled)) {
        set_status(blank3d_audio_status(&g.audio));
    }
    set_mouse_capture_state(g.lock_mouse && g.active);
    GetCursorPos(&g.last_mouse);
    last_tick = GetTickCount();
    while (g.running) {
        while (PeekMessage(&message, 0, 0, 0, PM_REMOVE)) {
            TranslateMessage(&message);
            DispatchMessage(&message);
        }
        now_tick = GetTickCount();
        frame_ms = now_tick - last_tick;
        last_tick = now_tick;
        if (frame_ms > 50U) frame_ms = 50U;
        if (frame_ms == 0U) frame_ms = 1U;
        g.frame_ms = (unsigned short)frame_ms;
        g.dt = (g3d_fix)(((unsigned long)frame_ms * (unsigned long)G3D_FIX_ONE) / 1000UL);
        g.time = g3d_fix_add_sat(g.time, g.dt);
        update_world();
        draw_scene();
        Sleep(1U);
    }

    set_mouse_capture_state(0);
    blank3d_audio_shutdown(&g.audio);
    blank3d_input_shutdown(&g.input);
    if (g.gl_context) {
        wglMakeCurrent(0, 0);
        wglDeleteContext(g.gl_context);
    }
    if (g.window && g.device) ReleaseDC(g.window, g.device);
    return 0;
}
