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
#include "blank3d_pickups.h"
#include "blank3d_variables.h"
#include "blank3d_audio.h"
#include "blank3d_config.h"
#include "blank3d_hud.h"
#include "blank3d_image_assets.h"
#include "blank3d_image_gl.h"
#include "blank3d_skybox89.h"
#include "blank3d_skybox_recipe89.h"
#include "blank3d_time89.h"
#include "blank3d_timeverbs89.h"
#include "blank3d_display_stack89.h"
#include "blank3d_window_win32.h"
#include "blank3d_video_gl89.h"
#include "blank3d_spriteplanes.h"
#include "blank3d_muzzle_image.h"
#include "blank3d_muzzle_light.h"
#include "blank3d_projectile_sprite89.h"
#include "blank3d_projectilevisual2d89_bridge.h"
#include "blank3d_sprite_runtime89.h"
#include "gcrosshair_base89.h"
#include "gscopeprovider89.h"
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
#include "blank3d_player_fire_ray.h"
#include "blank3d_player_projectile_aim.h"
#include "blank3d_languages.h"
#include "blank3d_objects.h"
#include "blank3d_runtime_spine.h"
#include "blank3d_npc_inventory.h"
#include "blank3d_vertical_axis.h"
#include "3d_movementbaseverbs89.h"
#include "3d_movementbaseverbs89_gamlib3d.h"
#include "blank3d_input.h"
#include "blank3d_input_platform.h"
#include "blank3d_list_cycle.h"
#include "blank3d_automotion.h"
#include "blank3d_gloco.h"
#include "blank3d_kinverbs.h"
#include "blank3d_condor.h"
#include "blank3d_motion_attack.h"
#include "blank3d_truth_gate.h"
#include "blank3d_perception.h"
#include "blank3d_perception_ini.h"
#include "blank3d_attachment.h"
#include "blank3d_mechanical_weapon.h"
#include "blank3d_weapon_presentation.h"
#include "blank3d_actor_equipment.h"
#include "actor_system89.h"
#include "blank3d_vphysics.h"
#include "blank3d_casing_physics.h"
#include "blank3d_mount_vehicle.h"
#include "blank3d_vehicle_system.h"
#include "blank3d_faction.h"
#include "blank3d_katana_mesh.h"
#include "blank3d_katana_melee.h"
#include "gtrigger89.h"
#include "gprojectilespawn89.h"
#include "gcasingruntime89.h"
#include "gweaponsnapshot89.h"
#include "satellaborner89.h"
#include "telesearcher89.h"
#include "expandiblefire89.h"
#include "bulletspin89.h"
#include "bulletcircle89.h"
#include "bulletinline89.h"

#ifndef VK_F3
#define VK_F3 0x72
#endif

#ifndef GL_ONE
#define GL_ONE 1
#endif

#define MAX_BULLETS 128
#define MAX_ENEMIES 32
#define MAX_CASINGS 128
#define B3D_BULLET_SPIN_STATE_CAPACITY 64
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
#define SCRIPT_VEHICLE_DDSL2 "scripts/vehicle_driver.ddsl2"
#define SCRIPT_FPI   "scripts/enemy.fpi"
#define CONFIG_FILE  "config/blank3d.toml"
#define GENERAL_CONFIG_FILE "config/GeneralConfig.cfg"

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
#define CAR_BOX_VERTICES     32
#define CAR_BOX_INDICES      64

typedef struct BulletTag {
    int alive;
    int cosmetic_only;
    int cosmetic_arrived;
    int weapon_id;
    int owner_actor_id;
    int owner_team_id;
    int target_actor_id;
    int projectile_id;
    int projectile_mesh_id;
    int trail_id;
    int aoi_trail_id;
    int physics_backend;
    int bolt_projectile_id;
    int damage;
    g3d_fix radius;
    g3d_fix mesh_scale;
    g3d_fix base_radius;
    g3d_fix base_mesh_scale;
    g3d_fix gravity;
    Transform transform;
    Vec3 previous_position;
    Vec3 cosmetic_target;
    Vec3 velocity;
    g3d_fix cosmetic_remaining;
    g3d_fix life;
    g3d_fix life_total;
    int expandible_fire_active;
    ef89_state expandible_fire_state;
    gv89_u32 audio_primary_key;
    gv89_u32 audio_secondary_key;
} Bullet;

typedef struct B3DBulletSpinEntryTag {
    int used;
    int actor_id;
    int weapon_id;
    bs89_state state;
} B3DBulletSpinEntry;

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
    g3d_mesh katana;
    g3d_mesh mount_car;

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
    g3d_vertex katana_vertices[B3D_KATANA_RENDER_VERTEX_CAPACITY];
    g3d_index katana_indices[B3D_KATANA_RENDER_INDEX_CAPACITY];
    g3d_vertex mount_car_vertices[CAR_BOX_VERTICES];
    g3d_index mount_car_indices[CAR_BOX_INDICES];
} MeshCatalog;

typedef struct EngineTag {
    HINSTANCE instance;
    HWND window;
    HDC device;
    HGLRC gl_context;
    int running;
    int active;
    int width;  /* internal render resolution */
    int height; /* internal render resolution */
    Blank3DDisplayStack89 display;
    Blank3DWindowWin32Host window_host;
    Blank3DVideoGL89 video_gl;
    int sniper_screen_width;
    int sniper_screen_height;
    int keys[256];
    int key_pressed[256];
    int key_released[256];
    int mouse_left;
    int mouse_right;
    int mouse_left_pressed;
    int mouse_left_released;
    int mouse_wheel_delta;
    Blank3DInput input;
    Blank3DInputPlatform input_platform;
    Blank3DListCycleRegistry list_cycles;
    unsigned long ddsl_action_mask;
    int lock_mouse;
    int mouse_captured;
    POINT last_mouse;
    unsigned short frame_ms;
    Blank3DTime89 time_system;
    Blank3DTimeVerbs89 time_verbs;

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
    mbv89_context movement_verbs;
    mbv89_actor player_movement_actor;
    mbv89_gamlib3d_adapter movement_gamlib;
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
    Blank3DMuzzleLight muzzle_light;
    Blank3DProjectileSpriteRuntime89 projectile_sprites;
    gtrigger89_state trigger_state;
    int morethanone_pending;
    int morethanone_weapon_id;
    mto89_result morethanone_result;
    B3DBulletSpinEntry bullet_spin_states[B3D_BULLET_SPIN_STATE_CAPACITY];
    unsigned int trigger_spinup_ms;
    unsigned int trigger_charge_max_ms;
    gps89_runtime projectile_spawn_runtime;
    gcr89_runtime casing_runtime;
    gws89_store weapon_snapshots;
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
    soq3d_key player_melee_socket;
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
    AS89_System actors;
    Blank3DSystems systems;
    Blank3DPickupWorld pickups;
    Blank3DVariables variables;
    Blank3DGloco gloco;
    Blank3DKinVerbs kinverbs;
    Blank3DCondor condor;
    GWP89_Manager npc_weapons;
    Blank3DNpcInventoryBank npc_inventory;
    Blank3DAudio audio;
    Blank3DConfig config;
    Blank3DImageAssets images;
    Blank3DText89 text;
    Blank3DSkybox89 skybox;
    Blank3DSkyboxRecipe89 skybox_recipes;
    int skybox_rpyl_override;
    Blank3DSpritePlaneWorld spriteplanes;
    Blank3DSpriteRuntime89 sprite_runtime;
    sprpl89_emit spriteplane_emit;
    Blank3DHud hud;
    Blank3DNumbarSystem numbars;
    Blank3DCollision collision;
    Blank3DVPhysics physics;
    Blank3DCasingPhysics casing_physics;
    Blank3DVehicleSystem vehicles;
    Blank3DKatanaConfig katana_config;
    Blank3DKatanaMelee katana_melee;
    Blank3DFactionSystem factions;
    Blank3DPerceptionWorld perception;
    Blank3DBolt bolt;
    Blank3DTrails trails;
    Blank3DSniper sniper;
    Blank3DCameraNaku cameranaku;
    Blank3DFireFrameSync fire_frame_sync;
    Blank3DClassSystem classes;
    Blank3DRuntimeSpine runtime;
    Blank3DObjects objects;
    FileStamp rpy_stamp;
    FileStamp ddsl2_stamp;
    FileStamp vehicle_ddsl2_stamp;
    FileStamp fpi_stamp;
    char vehicle_ddsl2_path[160];
    char status[256];
} Engine;

static Engine g;

static g3d_fix fix_ratio(long numerator, long denominator);

static int b3d_camera_draw_width(void)
{
    int width;
    width = g.display.gameplay.config.camera_draw_width;
    return width > 0 ? width : (g.width > 0 ? g.width : 1);
}

static int b3d_camera_draw_height(void)
{
    int height;
    height = g.display.gameplay.config.camera_draw_height;
    return height > 0 ? height : (g.height > 0 ? g.height : 1);
}

static g3d_fix b3d_camera_draw_aspect(void)
{
    return fix_ratio(b3d_camera_draw_width(), b3d_camera_draw_height());
}

static int b3d_display_video_provider(void *user,
                                      const gvc89_config *config,
                                      const gvc89_rect *presentation,
                                      int output_width,
                                      int output_height)
{
    Engine *engine;
    engine = (Engine *)user;
    if (!engine || !config || !presentation) return 0;
    engine->width = config->resolution_width;
    engine->height = config->resolution_height;
    if (engine->sniper_screen_width > 0 &&
        (engine->sniper_screen_width != engine->width ||
         engine->sniper_screen_height != engine->height)) {
        blank3d_sniper_set_screen(&engine->sniper,
                                  (short)engine->width,
                                  (short)engine->height);
        engine->sniper_screen_width = engine->width;
        engine->sniper_screen_height = engine->height;
    }
    return blank3d_video_gl89_configure(&engine->video_gl,
                                         config, presentation,
                                         output_width, output_height);
}

static int b3d_display_gameplay_provider(void *user,
                                         const gpss89_config *config)
{
    Engine *engine;
    engine = (Engine *)user;
    if (!engine || !config) return 0;
    if (engine->cameranaku.initialized)
        blank3d_cameranaku_resize(&engine->cameranaku,
                                  config->camera_draw_width,
                                  config->camera_draw_height);
    return 1;
}

static rt_ticks b3d_win32_time_ticks(void *user)
{
    (void)user;
    return (rt_ticks)GetTickCount();
}

static void apply_camera_config(void);
static int b3d_weapon_id_from_text(const char *weapon_text);
static int b3d_enemy_ensure_weapon_inventory(Enemy *enemy);
static int b3d_enemy_equip_weapon_text(Enemy *enemy, const char *weapon_text);
static int b3d_enemy_give_weapon_text(Enemy *enemy, const char *weapon_text);
static void sync_collision_world(void);
static void sync_kinverb_world(void);
static void b3d_movement_sync_steps(void);
static int current_clip_capacity(void);
static void draw_pickup_instance_mesh(const Blank3DPickupInstance *pickup);
static int b3d_actor_current_weapon_id(GWP89_Manager *manager,
                                        int actor_id);
static void build_view_vectors(Vec3 *out_eye, Vec3 *out_forward,
                               Vec3 *out_right, Vec3 *out_up);

static g3d_fix fix_ratio(long numerator, long denominator)
{
    if (denominator == 0L) return 0;
    return g3d_fix_div((g3d_fix)(numerator * G3D_FIX_ONE),
                       (g3d_fix)(denominator * G3D_FIX_ONE));
}


static const Blank3DCameraProfile *b3d_camera_profile_at_index(int index)
{
    return blank3d_cameranaku_profile_at(&g.cameranaku, index);
}

static const Blank3DCameraProfile *b3d_camera_profile(void)
{
    return blank3d_cameranaku_current_profile(&g.cameranaku);
}

static const char *b3d_camera_mode_name(int mode)
{
    const Blank3DCameraProfile *profile;
    profile = b3d_camera_profile_at_index(mode);
    return profile ? profile->name : "Camera";
}

static int b3d_camera_view_style(void)
{
    const Blank3DCameraProfile *profile;
    profile = b3d_camera_profile();
    return profile ? profile->view_style : GWP89_VIEW_THIRD_PERSON;
}

static int b3d_camera_is_fps(void)
{
    return b3d_camera_view_style() == GWP89_VIEW_FPS;
}

static int b3d_camera_is_ots(void)
{
    return b3d_camera_view_style() == GWP89_VIEW_OVER_SHOULDER;
}

static int b3d_camera_fire_view_mode(void)
{
    const Blank3DCameraProfile *profile;
    profile = b3d_camera_profile();
    if (profile && profile->aim_mode == B3D_CAMERA_AIM_CAMERA_ONLY)
        return B3D_PLAYER_FIRE_VIEW_FPS;
    if (b3d_camera_is_fps()) return B3D_PLAYER_FIRE_VIEW_FPS;
    if (b3d_camera_is_ots()) return B3D_PLAYER_FIRE_VIEW_OTS;
    return B3D_PLAYER_FIRE_VIEW_TPS;
}

static void b3d_camera_sync_profile_fields(void)
{
    const Blank3DCameraProfile *profile;
    profile = b3d_camera_profile();
    if (!profile) return;
    g.camera_mode = blank3d_cameranaku_profile_index(&g.cameranaku);
    g.camera_dist = profile->distance;
    g.camera_height = profile->pivot_y;
    g.camera_shoulder = profile->offset_right;
    g.camera_eye_height = profile->pivot_y;
    g.camera_pitch_min = profile->pitch_min;
    g.camera_pitch_max = profile->pitch_max;
    if (profile->mouse_sensitivity > 0)
        g.mouse_sensitivity = profile->mouse_sensitivity;
    g.base_fov = profile->fov;
    g.fov = profile->fov;
    g.near_z = profile->near_clip;
    g.far_z = profile->far_clip;
    blank3d_cameranaku_set_fov(&g.cameranaku, g.fov);
}


static g3d_fix q16_to_q12(long value)
{
    return (g3d_fix)(value / 16L);
}

static long q12_to_q16_long(g3d_fix value)
{
    return (long)value * 16L;
}


static void b3d_register_crosshair_image_assets(void)
{
    int id;
    for (id = 1000; id <= 1041; ++id) {
        const char *rel;
        char path[B3D_IMAGE_PATH_MAX];
        rel = gcb89_asset_filename(id);
        if (!rel || !rel[0]) continue;
        sprintf(path, "config/crosshair/%s", rel);
        (void)blank3d_image_assets_register(&g.images, id, path);
    }
}

static int b3d_scope_image_resolve(void *user, const char *asset_name,
                                   short *out_asset_id)
{
    Blank3DImageAssets *images;
    int id;
    images = (Blank3DImageAssets *)user;
    if (!images || !asset_name || !asset_name[0] || !out_asset_id)
        return GPR89_FALLBACK;
    id = blank3d_image_assets_resolve_path(images, asset_name);
    if (id <= 0 || id > 32767) return GPR89_FALLBACK;
    *out_asset_id = (short)id;
    return GPR89_HANDLED;
}

static void b3d_register_scope_image_provider(void)
{
    gpr89_provider provider;
    gpr89_provider_init(&provider, "imgcc0");
    provider.capabilities = GPR89_CAP_ASSET;
    provider.user = &g.images;
    provider.resolve_asset = b3d_scope_image_resolve;
    (void)blank3d_sniper_register_scope_provider(&g.sniper, &provider);
    gscb89_set_provider_route(&g.sniper.scope_bundle,
                              GPR89_DOMAIN_ASSET,
                              GPR89_MODE_AUTO,
                              "imgcc0");
}

static int b3d_sprite89_draw_host(void *user, int image_id,
                                  int sx, int sy, int sw, int sh,
                                  int dx, int dy, unsigned int flags)
{
    Blank3DImageAssets *images;
    images = (Blank3DImageAssets *)user;
    return blank3d_image_gl_draw_subrect(images, image_id,
                                         sx, sy, sw, sh, dx, dy,
                                         (flags & SA89_DRAW_FLIP_X) != 0U,
                                         (flags & SA89_DRAW_FLIP_Y) != 0U);
}

static void b3d_draw_world_spriteplanes(void)
{
    Vec3 eye;
    Vec3 forward;
    Vec3 right;
    Vec3 up;
    sprpl89_camera cam;
    build_view_vectors(&eye, &forward, &right, &up);
    memset(&cam, 0, sizeof(cam));
    cam.pos = sprpl89_v3((sprpl89_fx)q12_to_q16_long(eye.x),
                      (sprpl89_fx)q12_to_q16_long(eye.y),
                      (sprpl89_fx)q12_to_q16_long(eye.z));
    cam.forward = sprpl89_v3((sprpl89_fx)q12_to_q16_long(forward.x),
                          (sprpl89_fx)q12_to_q16_long(forward.y),
                          (sprpl89_fx)q12_to_q16_long(forward.z));
    cam.right = sprpl89_v3((sprpl89_fx)q12_to_q16_long(right.x),
                        (sprpl89_fx)q12_to_q16_long(right.y),
                        (sprpl89_fx)q12_to_q16_long(right.z));
    cam.up = sprpl89_v3((sprpl89_fx)q12_to_q16_long(up.x),
                     (sprpl89_fx)q12_to_q16_long(up.y),
                     (sprpl89_fx)q12_to_q16_long(up.z));
    cam.world_up = sprpl89_v3(0, SP89_FX_ONE, 0);
    if (blank3d_spriteplanes_emit(&g.spriteplanes, &cam,
                                  &g.spriteplane_emit) == SP89_OK)
        blank3d_image_gl_draw_spriteplanes(&g.images, &g.spriteplane_emit);
}

static void b3d_damage_player(int amount)
{
    if (amount <= 0) return;
    blank3d_systems_damage_player(&g.systems, amount);
    g.player_hp = blank3d_systems_player_health(&g.systems);
    g.player_damage_flash_ms = 520;
}

static int b3d_actor_record_enemy_index(const AS89_Actor *actor)
{
    int index;
    if (!actor || actor->user_ref == 0UL) return -1;
    index = (int)(actor->user_ref - 1UL);
    if (index < 0 || index >= MAX_ENEMIES) return -1;
    return index;
}

static int b3d_as89_query_alive(void *user, int actor_id, int stored_alive)
{
    Engine *engine;
    const AS89_Actor *actor;
    int index;
    engine = (Engine *)user;
    if (engine == 0) return stored_alive;
    if (actor_id == B3D_PLAYER_ACTOR_ID) return engine->player_hp > 0;
    actor = as89_find_const(&engine->actors, actor_id);
    index = b3d_actor_record_enemy_index(actor);
    if (index < 0) return stored_alive;
    return engine->enemies[index].alive ? 1 : 0;
}

static int b3d_as89_query_visible(void *user, int actor_id, int stored_visible)
{
    if (!stored_visible) return 0;
    return b3d_as89_query_alive(user, actor_id, stored_visible);
}

static int b3d_as89_query_position(void *user, int actor_id,
                                   AS89_Position *out_position)
{
    Engine *engine;
    const AS89_Actor *actor;
    const Vec3 *position;
    int index;
    engine = (Engine *)user;
    if (engine == 0 || out_position == 0) return 0;
    if (actor_id == B3D_PLAYER_ACTOR_ID)
        position = &engine->player.position;
    else {
        actor = as89_find_const(&engine->actors, actor_id);
        index = b3d_actor_record_enemy_index(actor);
        if (index < 0 || !engine->enemies[index].alive) return 0;
        position = &engine->enemies[index].transform.position;
    }
    out_position->x = (as89_scalar)position->x;
    out_position->y = (as89_scalar)position->y;
    out_position->z = (as89_scalar)position->z;
    return 1;
}

static unsigned long b3d_as89_query_locator(void *user, int actor_id)
{
    Engine *engine;
    const AS89_Actor *actor;
    int index;
    engine = (Engine *)user;
    if (engine == 0) return 0UL;
    if (actor_id == B3D_PLAYER_ACTOR_ID)
        return (unsigned long)engine->player_key;
    actor = as89_find_const(&engine->actors, actor_id);
    index = b3d_actor_record_enemy_index(actor);
    if (index < 0) return 0UL;
    return (unsigned long)engine->enemy_keys[index];
}

static void b3d_actor_system_reset(void)
{
    AS89_Provider provider;
    as89_init(&g.actors);
    memset(&provider, 0, sizeof(provider));
    provider.user = &g;
    provider.query_alive = b3d_as89_query_alive;
    provider.query_visible = b3d_as89_query_visible;
    provider.query_position = b3d_as89_query_position;
    provider.query_locator = b3d_as89_query_locator;
    as89_set_provider(&g.actors, &provider);
    (void)as89_register(&g.actors, B3D_PLAYER_ACTOR_ID,
                        0UL, 0UL, 1, 1, 0UL);
}

static Enemy *b3d_actor_enemy(int actor_id)
{
    const AS89_Actor *actor;
    int index;
    actor = as89_find_const(&g.actors, actor_id);
    index = b3d_actor_record_enemy_index(actor);
    if (index < 0) return 0;
    return &g.enemies[index];
}

static const Enemy *b3d_actor_enemy_const(int actor_id)
{
    const AS89_Actor *actor;
    int index;
    actor = as89_find_const(&g.actors, actor_id);
    index = b3d_actor_record_enemy_index(actor);
    if (index < 0) return 0;
    return &g.enemies[index];
}

static int b3d_actor_alive(int actor_id)
{
    return as89_is_alive(&g.actors, actor_id);
}

static int b3d_actor_position(int actor_id, Vec3 *out_position)
{
    AS89_Position position;
    if (out_position == 0 || !as89_position(&g.actors, actor_id, &position))
        return 0;
    out_position->x = (g3d_fix)position.x;
    out_position->y = (g3d_fix)position.y;
    out_position->z = (g3d_fix)position.z;
    return 1;
}

static soq3d_key b3d_actor_locator_key(int actor_id)
{
    return (soq3d_key)as89_locator(&g.actors, actor_id);
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

/* Shared target resolver for vendored projectile-origin/homing modules.
   The vendors know only actor IDs + positions; Blank3D remains authoritative
   for Actor/GFaction membership and target selection. */
static int b3d_projectile_target_resolve(int owner_actor_id,
                                         int requested_target_id,
                                         int *target_actor_id,
                                         Vec3 *target_position)
{
    const Enemy *owner_enemy;
    Vec3 owner_position;
    Vec3 candidate_position;
    Vec3 delta;
    g3d_fix distance_sq;
    g3d_fix best_distance_sq;
    int best_actor_id;
    int actor_id;
    int i;
    if (!target_actor_id || !target_position) return 0;
    *target_actor_id = B3D_TARGET_NONE;

    if (requested_target_id != B3D_TARGET_NONE &&
        b3d_actor_alive(requested_target_id) &&
        b3d_faction_can_attack_actor(owner_actor_id, requested_target_id) &&
        b3d_actor_position(requested_target_id, target_position)) {
        *target_actor_id = requested_target_id;
        return 1;
    }

    owner_enemy = b3d_actor_enemy_const(owner_actor_id);
    if (owner_enemy && owner_enemy->target_actor_id != B3D_TARGET_NONE &&
        b3d_actor_alive(owner_enemy->target_actor_id) &&
        b3d_faction_can_attack_actor(owner_actor_id,
                                     owner_enemy->target_actor_id) &&
        b3d_actor_position(owner_enemy->target_actor_id, target_position)) {
        *target_actor_id = owner_enemy->target_actor_id;
        return 1;
    }

    if (!b3d_actor_position(owner_actor_id, &owner_position)) return 0;
    best_actor_id = B3D_TARGET_NONE;
    best_distance_sq = 0;

    /* Player is not stored in g.enemies, so consider it explicitly for
       hostile NPC owners. */
    if (owner_actor_id != B3D_PLAYER_ACTOR_ID &&
        b3d_actor_alive(B3D_PLAYER_ACTOR_ID) &&
        b3d_faction_can_attack_actor(owner_actor_id, B3D_PLAYER_ACTOR_ID) &&
        b3d_actor_position(B3D_PLAYER_ACTOR_ID, &candidate_position)) {
        gamlib_vec3_sub(&delta, &candidate_position, &owner_position);
        best_distance_sq = gamlib_vec3_dot(&delta, &delta);
        best_actor_id = B3D_PLAYER_ACTOR_ID;
        *target_position = candidate_position;
    }

    for (i = 0; i < g.enemy_count; ++i) {
        if (!g.enemies[i].alive) continue;
        actor_id = g.enemies[i].weapon_actor_id;
        if (actor_id == owner_actor_id || actor_id == B3D_TARGET_NONE) continue;
        if (!b3d_faction_can_attack_actor(owner_actor_id, actor_id)) continue;
        if (!b3d_actor_position(actor_id, &candidate_position)) continue;
        gamlib_vec3_sub(&delta, &candidate_position, &owner_position);
        distance_sq = gamlib_vec3_dot(&delta, &delta);
        if (best_actor_id == B3D_TARGET_NONE ||
            distance_sq < best_distance_sq) {
            best_actor_id = actor_id;
            best_distance_sq = distance_sq;
            *target_position = candidate_position;
        }
    }
    if (best_actor_id == B3D_TARGET_NONE) return 0;
    *target_actor_id = best_actor_id;
    return 1;
}

static int b3d_satellaborner_target_provider(void *user, int owner_actor_id,
                                              int requested_target_id,
                                              sat89_target *target_out)
{
    Vec3 position;
    int actor_id;
    (void)user;
    if (!target_out ||
        !b3d_projectile_target_resolve(owner_actor_id, requested_target_id,
                                       &actor_id, &position)) return 0;
    memset(target_out, 0, sizeof(*target_out));
    target_out->valid = 1;
    target_out->actor_id = actor_id;
    target_out->position.x = (sat89_fx)position.x;
    target_out->position.y = (sat89_fx)position.y;
    target_out->position.z = (sat89_fx)position.z;
    return 1;
}

static int b3d_telesearcher_target_provider(void *user, int owner_actor_id,
                                             int requested_target_id,
                                             ts89_target *target_out)
{
    Vec3 position;
    int actor_id;
    (void)user;
    if (!target_out ||
        !b3d_projectile_target_resolve(owner_actor_id, requested_target_id,
                                       &actor_id, &position)) return 0;
    memset(target_out, 0, sizeof(*target_out));
    target_out->valid = 1;
    target_out->actor_id = actor_id;
    target_out->position.x = (ts89_fx)position.x;
    target_out->position.y = (ts89_fx)position.y;
    target_out->position.z = (ts89_fx)position.z;
    return 1;
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
        if (blank3d_sniper_is_scoped(&g.sniper))
            (void)blank3d_sniper_trigger_event(&g.sniper, "damage");
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
    if (g.condor.initialized) {
        const Blank3DGlocoBinding *binding;
        unsigned long thing_owner;
        void *subject;
        binding = blank3d_gloco_binding(&g.gloco, target_actor);
        thing_owner = binding ? binding->thing_owner : 0UL;
        subject = target_actor == B3D_PLAYER_ACTOR_ID
                ? (void *)&g.player : (void *)b3d_actor_enemy(target_actor);
        blank3d_condor_emit_actor(&g.condor,
            b3d_actor_alive(target_actor)
                ? B3D_CEA_EVENT_ACTOR_DAMAGE : B3D_CEA_EVENT_ACTOR_DEATH,
            target_actor, thing_owner, source_actor, subject, &event_data);
    }
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

static int b3d_reload_vehicle_ddsl_path(const char *path)
{
    const char *use_path;
    use_path = path && path[0] ? path : SCRIPT_VEHICLE_DDSL2;
    strncpy(g.vehicle_ddsl2_path, use_path,
            sizeof(g.vehicle_ddsl2_path) - 1U);
    g.vehicle_ddsl2_path[sizeof(g.vehicle_ddsl2_path) - 1U] = '\0';
    if (!blank3d_languages_reload_ddsl2_pair(SCRIPT_DDSL2,
                                              g.vehicle_ddsl2_path))
        return 0;
    g.vehicle_ddsl2_stamp = get_stamp(g.vehicle_ddsl2_path);
    return 1;
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
    g.player_melee_socket = soq3d_key_from_cstr("player.melee_r");
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
    /* Independent melee test carrier.  The complete pivot is pushed in
       front of the player, not merely the blade centre.  A giant blade can
       therefore rotate through its whole Mecanim arc without crossing the
       player root.  Positive mount_forward_cm means player-local -Z. */
    pose = local_pose(fix_ratio(g.katana_config.mount_lateral_cm, 100),
                      fix_ratio(g.katana_config.mount_height_cm, 100),
                      g3d_fix_neg_sat(fix_ratio(
                          g.katana_config.mount_forward_cm, 100)));
    soq3d_define_local_socket(&g.locator, g.player_melee_socket,
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
    if (!blank3d_katana_mesh_build(&g.meshes.katana,
            g.meshes.katana_vertices, B3D_KATANA_RENDER_VERTEX_CAPACITY,
            g.meshes.katana_indices, B3D_KATANA_RENDER_INDEX_CAPACITY,
            g.katana_config.preset_index,
            g.katana_config.mesh_scale_thickness_percent,
            g.katana_config.mesh_scale_width_percent,
            g.katana_config.mesh_scale_length_percent)) return 0;

    /* Mount89 drive prototype: one deliberately plain low-poly car box. */
    result = g3d_mesh_init(&g.meshes.mount_car,
                           g.meshes.mount_car_vertices, CAR_BOX_VERTICES,
                           g.meshes.mount_car_indices, CAR_BOX_INDICES);
    if (result != G3D_OK) return 0;
    result = g3d_make_box(&g.meshes.mount_car,
                          g3d_fx_from_ratio(12, 5),
                          g3d_fx_from_ratio(6, 5),
                          g3d_fx_from_int(4),
                          g3d_color_rgba(238U, 242U, 248U, 255U));
    if (result != G3D_OK) return 0;
    return 1;
}

static mbv89_fixed b3d_movement_game_amount(mbv89_game_verb verb)
{
    switch (verb) {
    case MBV89_GAME_STRAFE_LEFT:
    case MBV89_GAME_STRAFE_RIGHT:
        return g.movement_verbs.strafe_step;
    case MBV89_GAME_RUN_FORWARD:
    case MBV89_GAME_RUN_BACKWARD:
        return g.movement_verbs.run_step;
    default:
        return g.movement_verbs.walk_step;
    }
}

static int b3d_movement_game_provider(void *provider_user,
                                      mbv89_context *ctx,
                                      mbv89_actor *actor,
                                      mbv89_game_verb verb);

static void reset_transform(Transform *transform, g3d_fix x, g3d_fix y, g3d_fix z)
{
    transform_init(transform);
    transform->position = gamlib_vec3(x, y, z);
    set_game_yaw(transform, 0);
}

static void default_world(void)
{
    int i;
    reset_transform(&g.player, 0, 0, 0);
    if (g.vehicles.initialized) {
        blank3d_vehicle_system_clear(&g.vehicles);
        if (g.gloco.initialized)
            blank3d_gloco_set_suspended(&g.gloco, B3D_PLAYER_ACTOR_ID, 0);
    }
    g.player_hp = 100;
    g.player_damage_flash_ms = 0;
    g.move_speed = G3D_FIX_FROM_INT(7);
    g.strafe_speed = fix_ratio(11, 2);
    g.vertical_speed = G3D_FIX_FROM_INT(4);
    blank3d_vertical_body_init(&g.player_vertical, &g.player,
                               g.player.position.y, 0);
    mbv89_context_init(&g.movement_verbs);
    mbv89_actor_init(&g.player_movement_actor);
    mbv89_gamlib3d_adapter_init(&g.movement_gamlib);
    mbv89_gamlib3d_adapter_set_move_mode(&g.movement_gamlib,
                                          MBV89_GAMLIB3D_MOVE_FLAT);
    mbv89_gamlib3d_bind_actor(&g.player_movement_actor, &g.player);
    mbv89_gamlib3d_install(&g.movement_verbs, &g.movement_gamlib);
    if (g.gloco.initialized)
        blank3d_gloco_install_movement_verbs(&g.gloco, &g.movement_verbs);
    mbv89_set_game_provider(&g.movement_verbs,
                            b3d_movement_game_provider, 0);
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
    g.camera_mode = 0;
    g.fire_requested = 0;
    g.muzzle_flash_ms = 0;
    blank3d_muzzle_light_init(&g.muzzle_light);
    gtrigger89_init(&g.trigger_state);
    g.trigger_spinup_ms = GATLING_SPINUP_MS;
    g.trigger_charge_max_ms = 900U;
    gweaponsnapshot89_init(&g.weapon_snapshots);
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
        g.bullets[i].cosmetic_only = 0;
        g.bullets[i].cosmetic_arrived = 0;
        reset_transform(&g.bullets[i].transform, 0, 0, 0);
        g.bullets[i].weapon_id = 0;
        g.bullets[i].owner_actor_id = 0;
        g.bullets[i].owner_team_id = 0;
        g.bullets[i].target_actor_id = B3D_TARGET_NONE;
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
        g.bullets[i].cosmetic_target = gamlib_vec3(0, 0, 0);
        g.bullets[i].velocity = gamlib_vec3(0, 0, 0);
        g.bullets[i].cosmetic_remaining = 0;
        g.bullets[i].life = 0;
        g.bullets[i].life_total = 0;
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
                            b3d_camera_draw_aspect(),
                            g.near_z, g.far_z);
    configure_sockets();
}

static void publish_transform(soq3d_key key, const Transform *transform)
{
    soq3d_pose pose;
    bridge_transform_to_pose(transform, &pose);
    soq3d_publish_thing(&g.locator, key, &pose);
}

static g3d_fix b3d_visual_lerp(g3d_fix a, g3d_fix b, g3d_fix t)
{
    g3d_fix delta;
    t = g3d_fix_clamp(t, 0, G3D_FIX_ONE);
    delta = g3d_fix_sub_sat(b, a);
    return g3d_fix_add_sat(a, g3d_fix_mul(delta, t));
}

static void b3d_bullet_visual_scale(const Bullet *bullet,
                                    g3d_fix *radial,
                                    g3d_fix *axial)
{
    const Blank3DWeaponModules *modules;
    g3d_fix elapsed;
    g3d_fix expand_s;
    g3d_fix crush_s;
    g3d_fix t;
    g3d_fix rs;
    g3d_fix rp;
    g3d_fix re;
    g3d_fix as;
    g3d_fix ae;
    if (!radial || !axial) return;
    *radial = G3D_FIX_ONE;
    *axial = G3D_FIX_ONE;
    if (!bullet) return;
    modules = blank3d_weapon_modules_get(bullet->weapon_id);
    if (!modules || modules->projectile_visual !=
            B3D_PROJECTILE_VISUAL_SHANGO_PRESS) return;

    rs = (g3d_fix)(modules->projectile_visual_radial_start_q16 / 16L);
    rp = (g3d_fix)(modules->projectile_visual_radial_peak_q16 / 16L);
    re = (g3d_fix)(modules->projectile_visual_radial_end_q16 / 16L);
    as = (g3d_fix)(modules->projectile_visual_axial_start_q16 / 16L);
    ae = (g3d_fix)(modules->projectile_visual_axial_end_q16 / 16L);
    if (rs <= 0) rs = fix_ratio(1, 4);
    if (rp <= 0) rp = G3D_FIX_ONE;
    if (re <= 0) re = G3D_FIX_ONE;
    if (as <= 0) as = G3D_FIX_ONE;
    if (ae <= 0) ae = fix_ratio(1, 4);

    elapsed = g3d_fix_sub_sat(bullet->life_total, bullet->life);
    if (elapsed < 0) elapsed = 0;
    expand_s = milliseconds_to_seconds_fix(
        modules->projectile_visual_expand_ms);
    crush_s = milliseconds_to_seconds_fix(
        modules->projectile_visual_crush_ms);

    if (expand_s > 0 && elapsed < expand_s) {
        t = g3d_fix_div(elapsed, expand_s);
        *radial = b3d_visual_lerp(rs, rp, t);
        *axial = as;
    } else {
        *radial = rp;
        *axial = as;
        if (crush_s > 0) {
            elapsed = g3d_fix_sub_sat(elapsed, expand_s);
            t = g3d_fix_div(elapsed, crush_s);
            if (t > G3D_FIX_ONE) t = G3D_FIX_ONE;
            *radial = b3d_visual_lerp(rp, re, t);
            *axial = b3d_visual_lerp(as, ae, t);
        }
    }
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
    g3d_fix visual_radial;
    g3d_fix visual_axial;
    g3d_fix radial_scale;
    g3d_fix axial_scale;

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

    b3d_bullet_visual_scale(bullet, &visual_radial, &visual_axial);
    radial_scale = g3d_fix_mul(bullet->mesh_scale, visual_radial);
    axial_scale = g3d_fix_mul(bullet->mesh_scale, visual_axial);
    gamlib_vec3_scale(&scaled_right, &right, radial_scale);
    gamlib_vec3_scale(&scaled_up, &up, radial_scale);
    gamlib_vec3_scale(&scaled_forward, &forward, axial_scale);

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
    g.bullets[bullet_index].cosmetic_only = 0;
    g.bullets[bullet_index].cosmetic_arrived = 0;
    g.bullets[bullet_index].cosmetic_remaining = 0;
    g.bullets[bullet_index].target_actor_id = B3D_TARGET_NONE;
    g.bullets[bullet_index].expandible_fire_active = 0;
    g.bullets[bullet_index].expandible_fire_state.active = 0;
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
            g.bullets[i].weapon_id != B3D_WEAPON_ID_HOMING_ROCKET_LAUNCHER &&
            g.bullets[i].life < shortest_life) {
            candidate = i;
            shortest_life = g.bullets[i].life;
        }
    }
    if (candidate >= 0) release_bullet_slot(candidate);
    return candidate;
}

static int b3d_player_fire_raycast_adapter(
    void *user,
    const GWP89_Vec3 *origin,
    const GWP89_Vec3 *direction,
    gwp89_fx range_fx,
    unsigned int layer_mask,
    Blank3DCollisionHit *out_hit)
{
    return blank3d_collision_raycast((Blank3DCollision *)user,
                                     origin, direction, range_fx,
                                     layer_mask, out_hit);
}

static int b3d_player_uses_camera_hitscan(
    const GWP89_Event *event,
    const Blank3DWeaponModules *modules)
{
    if (!event || !modules) return 0;
    if (event->actor_id != B3D_PLAYER_ACTOR_ID) return 0;
    if (modules->physics_backend != B3D_PHYSICS_LINEAR) return 0;
    /* Origin relocation and homing need a real projectile lifetime. Never
       collapse those recipes into the player-only hitscan path. */
    if (modules->player_physical_projectile_enabled ||
        modules->satellaborner_enabled || modules->telesearcher_enabled ||
        modules->expandible_fire_enabled || modules->bullet_circle_enabled ||
        modules->bullet_spin_enabled || modules->bullet_inline_enabled)
        return 0;
    /* Rockets remain actual world projectiles even though their catalogue
       backend is linear. Only ordinary player small arms become hitscan. */
    if (event->weapon_id == B3D_WEAPON_ID_ROCKET_LAUNCHER) return 0;
    return 1;
}

/*
 * The camera/muzzle ray is gameplay authority and is intentionally never
 * rendered.  A separate short-lived visual projectile moves from the visible
 * muzzle toward the already-resolved impact point and owns the player-facing
 * trail.  Keeping both concepts separate prevents a debug line from exposing
 * the complete hitscan result to the player.
 */
static g3d_fix b3d_player_visual_travel_time(const GWP89_Event *event)
{
    unsigned int travel_ms;
    travel_ms = 96U;
    if (event) {
        if (event->weapon_id == B3D_WEAPON_ID_GATLING)
            travel_ms = 72U;
        else if (event->weapon_id == B3D_WEAPON_ID_MACHINE_GUN)
            travel_ms = 80U;
        else if (event->weapon_id == B3D_WEAPON_ID_SNIPER)
            travel_ms = 64U;
        else if (event->weapon_id == B3D_WEAPON_ID_SHOTGUN)
            travel_ms = 88U;
    }
    return milliseconds_to_seconds_fix(travel_ms);
}

static Vec3 b3d_player_visual_origin(
    int view_mode, Vec3 camera_origin, Vec3 muzzle_origin)
{
    Vec3 delta;
    if (view_mode != B3D_PLAYER_FIRE_VIEW_FPS) return muzzle_origin;
    gamlib_vec3_sub(&delta, &muzzle_origin, &camera_origin);
    if (gamlib_vec3_length(&delta) > fix_ratio(1, 20))
        return muzzle_origin;
    return camera_origin;
}

static int b3d_spawn_player_camera_ray(
    const GWP89_Event *event,
    const Blank3DWeaponModules *modules)
{
    Blank3DPlayerFireRayInput input;
    Blank3DPlayerFireRayResult result;
    Vec3 camera_eye;
    Vec3 camera_forward;
    Vec3 camera_right;
    Vec3 camera_up;
    Vec3 muzzle;
    Vec3 visual_origin;
    Vec3 visual_delta;
    g3d_fix visual_time;
    int damage;
    int slot;

    if (!b3d_player_uses_camera_hitscan(event, modules)) return 0;
    memset(&input, 0, sizeof(input));

    camera_eye = gwp_to_vec3(event->camera_origin);
    camera_forward = gwp_to_vec3(event->camera_forward);
    camera_right = gwp_to_vec3(event->camera_right);
    camera_up = gwp_to_vec3(event->camera_up);
    if (gamlib_vec3_length(&camera_forward) <= G3D_FIX_EPSILON) {
        blank3d_cameranaku_get_view(&g.cameranaku,
                                    &camera_eye,
                                    &camera_forward,
                                    &camera_right,
                                    &camera_up);
    }

    muzzle = gwp_to_vec3(event->muzzle_origin);
    if (gamlib_vec3_length(&muzzle) <= G3D_FIX_EPSILON)
        muzzle = gwp_to_vec3(event->origin);

    input.view_mode = b3d_camera_fire_view_mode();
    input.camera_origin = camera_eye;
    input.camera_forward = camera_forward;
    input.camera_right = camera_right;
    input.camera_up = camera_up;
    input.muzzle_origin = muzzle;
    input.range = event->range_fx > 0
                ? (g3d_fix)event->range_fx
                : G3D_FIX_FROM_INT(120);
    input.spread_degrees = (g3d_fix)event->spread_fx;
    input.pellet_index = event->pellet_index;
    input.pellet_count = event->pellet_count > 0
                       ? event->pellet_count : 1;
    input.layer_mask = B3D_COLLISION_LAYER_WORLD |
                       B3D_COLLISION_LAYER_ENEMY;

    if (!blank3d_player_fire_ray_resolve(
            &input, b3d_player_fire_raycast_adapter,
            &g.collision, &result) || !result.valid)
        return 1;

    damage = gwp89_fx_to_int_round(event->damage_fx);
    if (damage < 1) damage = 1;
    if (result.hit.hit) {
        if (result.hit.enemy_index >= 0 &&
            result.hit.enemy_index < g.enemy_count &&
            g.enemies[result.hit.enemy_index].alive &&
            b3d_faction_can_attack_actor(
                B3D_PLAYER_ACTOR_ID,
                g.enemies[result.hit.enemy_index].weapon_actor_id)) {
            b3d_damage_actor(B3D_PLAYER_ACTOR_ID,
                g.enemies[result.hit.enemy_index].weapon_actor_id,
                damage);
            blank3d_audio_impact(&g.audio, 1, damage);
        } else if (result.hit.material_id == B3D_COLLISION_MATERIAL_WORLD) {
            blank3d_audio_impact(&g.audio, 0, damage);
        }
    }

    /* Damage is already authoritative.  The ray itself stays invisible.
       This slot is a moving, non-colliding visual projectile with a real
       player-facing trail; it cannot alter damage, collision or NPC fire. */
    slot = find_projectile_slot(event->weapon_id);
    if (slot < 0) return 1;
    release_bullet_slot(slot);
    visual_origin = b3d_player_visual_origin(
        input.view_mode, input.camera_origin, input.muzzle_origin);
    gamlib_vec3_sub(&visual_delta, &result.impact_point, &visual_origin);
    visual_time = b3d_player_visual_travel_time(event);
    if (visual_time <= G3D_FIX_EPSILON)
        visual_time = milliseconds_to_seconds_fix(80U);

    g.bullets[slot].alive = 1;
    g.bullets[slot].cosmetic_only = 1;
    g.bullets[slot].cosmetic_arrived = 0;
    g.bullets[slot].weapon_id = event->weapon_id;
    g.bullets[slot].owner_actor_id = event->actor_id;
    g.bullets[slot].owner_team_id = event->team_id;
    g.bullets[slot].projectile_id = event->projectile_id;
    g.bullets[slot].projectile_mesh_id = event->projectile_mesh_id;
    if (modules->projectile_mesh_id > 0)
        g.bullets[slot].projectile_mesh_id = modules->projectile_mesh_id;
    if (g.bullets[slot].projectile_mesh_id < 1 ||
        g.bullets[slot].projectile_mesh_id > PROJECTILE_MESH_COUNT)
        g.bullets[slot].projectile_mesh_id = 1;
    g.bullets[slot].trail_id = event->trail_id != 0
                             ? event->trail_id : 1;
    g.bullets[slot].aoi_trail_id = -1;
    g.bullets[slot].physics_backend = B3D_PHYSICS_LINEAR;
    g.bullets[slot].bolt_projectile_id = -1;
    g.bullets[slot].damage = 0;
    g.bullets[slot].radius = 0;
    g.bullets[slot].mesh_scale =
        (g3d_fix)event->projectile_mesh_scale_fx;
    if (g.bullets[slot].mesh_scale <= 0)
        g.bullets[slot].mesh_scale = fix_ratio(7, 20);
    g.bullets[slot].gravity = 0;
    transform_init(&g.bullets[slot].transform);
    g.bullets[slot].transform.position = visual_origin;
    g.bullets[slot].previous_position = visual_origin;
    g.bullets[slot].cosmetic_target = result.impact_point;
    g.bullets[slot].cosmetic_remaining =
        gamlib_vec3_length(&visual_delta);
    g.bullets[slot].velocity.x =
        g3d_fix_div(visual_delta.x, visual_time);
    g.bullets[slot].velocity.y =
        g3d_fix_div(visual_delta.y, visual_time);
    g.bullets[slot].velocity.z =
        g3d_fix_div(visual_delta.z, visual_time);
    g.bullets[slot].life = g3d_fix_add_sat(
        visual_time, milliseconds_to_seconds_fix(96U));
    g.bullets[slot].life_total = g.bullets[slot].life;
    if (modules->emit_trail &&
        modules->trail_profile != B3D_TRAIL_NONE) {
        g.bullets[slot].aoi_trail_id =
            blank3d_trails_attach(&g.trails, slot,
                                  modules->trail_profile);
        blank3d_trails_emit_q12(&g.trails, slot,
            visual_origin.x, visual_origin.y, visual_origin.z);
    }
    g.bullets[slot].audio_primary_key = 0U;
    g.bullets[slot].audio_secondary_key = 0U;
    publish_bullet(slot);
    return 1;
}

static int b3d_projectile_world_spawn_provider(
    void *user,
    const gps89_request *request,
    gps89_handle *handle)
{
    int i;
    int bolt_id;
    Vec3 direction;
    GWP89_Vec3 origin_q12;
    GWP89_Vec3 direction_q12;
    const Blank3DWeaponModules *modules;
    ef89_config fire_config;
    ef89_vec3 fire_spawn;
    (void)user;
    if (!request || !handle) return 0;
    modules = blank3d_weapon_modules_get(request->weapon_id);
    i = find_projectile_slot(request->weapon_id);
    if (i < 0) {
        set_status("projectile provider: pool full; spawn rejected");
        return 0;
    }
    release_bullet_slot(i);
    direction = gamlib_vec3((g3d_fix)request->direction.x,
                            (g3d_fix)request->direction.y,
                            (g3d_fix)request->direction.z);
    if (gamlib_vec3_length(&direction) <= G3D_FIX_EPSILON)
        direction = gamlib_vec3(0, 0, G3D_FIX_ONE);
    else
        gamlib_vec3_normalize(&direction, &direction);
    direction_q12 = vec3_to_gwp(direction);
    origin_q12.x = (gwp89_fx)request->origin.x;
    origin_q12.y = (gwp89_fx)request->origin.y;
    origin_q12.z = (gwp89_fx)request->origin.z;

    g.bullets[i].alive = 1;
    g.bullets[i].cosmetic_only = 0;
    g.bullets[i].weapon_id = request->weapon_id;
    g.bullets[i].owner_actor_id = request->actor_id;
    g.bullets[i].owner_team_id = request->team_id;
    g.bullets[i].target_actor_id = request->target_actor_id;
    g.bullets[i].projectile_id = request->projectile_id;
    g.bullets[i].projectile_mesh_id = request->projectile_mesh_id;
    if (modules && modules->projectile_mesh_id > 0)
        g.bullets[i].projectile_mesh_id = modules->projectile_mesh_id;
    if (g.bullets[i].projectile_mesh_id < 1 ||
        g.bullets[i].projectile_mesh_id > PROJECTILE_MESH_COUNT)
        g.bullets[i].projectile_mesh_id = 1;
    g.bullets[i].trail_id = request->trail_id;
    g.bullets[i].physics_backend = modules
        ? modules->physics_backend : B3D_PHYSICS_LINEAR;
    g.bullets[i].gravity = (g3d_fix)request->gravity_fx;
    g.bullets[i].damage = gwp89_fx_to_int_round((gwp89_fx)request->damage_fx);
    if (g.bullets[i].damage < 1) g.bullets[i].damage = 1;
    g.bullets[i].radius = (g3d_fix)request->radius_fx;
    if (g.bullets[i].radius <= 0) g.bullets[i].radius = fix_ratio(1, 10);
    g.bullets[i].mesh_scale = (g3d_fix)request->mesh_scale_fx;
    if (g.bullets[i].mesh_scale <= 0) g.bullets[i].mesh_scale = G3D_FIX_ONE;
    g.bullets[i].base_radius = g.bullets[i].radius;
    g.bullets[i].base_mesh_scale = g.bullets[i].mesh_scale;
    transform_init(&g.bullets[i].transform);
    g.bullets[i].transform.position = gamlib_vec3(
        (g3d_fix)request->origin.x,
        (g3d_fix)request->origin.y,
        (g3d_fix)request->origin.z);
    g.bullets[i].previous_position = g.bullets[i].transform.position;
    gamlib_vec3_scale(&g.bullets[i].velocity, &direction,
                      (g3d_fix)request->speed_fx);
    g.bullets[i].life = milliseconds_to_seconds_fix(request->life_ms);
    if (g.bullets[i].life <= 0) g.bullets[i].life = G3D_FIX_FROM_INT(2);
    g.bullets[i].life_total = g.bullets[i].life;
    g.bullets[i].expandible_fire_active = 0;
    memset(&g.bullets[i].expandible_fire_state, 0,
           sizeof(g.bullets[i].expandible_fire_state));
    if (modules && modules->expandible_fire_enabled) {
        expandiblefire89_config_default(&fire_config);
        fire_config.scale_start_fx =
            (ef89_fx)(modules->expandible_fire_scale_start_q16 / 16L);
        fire_config.scale_end_fx =
            (ef89_fx)(modules->expandible_fire_scale_end_q16 / 16L);
        fire_config.growth_distance_fx =
            (ef89_fx)(modules->expandible_fire_growth_distance_q16 / 16L);
        fire_config.kill_distance_fx =
            (ef89_fx)(modules->expandible_fire_kill_distance_q16 / 16L);
        fire_spawn.x = (ef89_fx)g.bullets[i].transform.position.x;
        fire_spawn.y = (ef89_fx)g.bullets[i].transform.position.y;
        fire_spawn.z = (ef89_fx)g.bullets[i].transform.position.z;
        expandiblefire89_init(&g.bullets[i].expandible_fire_state,
                              fire_spawn, &fire_config);
        g.bullets[i].expandible_fire_active = 1;
        g.bullets[i].mesh_scale = (g3d_fix)expandiblefire89_apply_scale(
            (ef89_fx)g.bullets[i].base_mesh_scale,
            g.bullets[i].expandible_fire_state.scale_fx);
        if (modules->expandible_fire_collision_growth)
            g.bullets[i].radius = (g3d_fix)expandiblefire89_apply_scale(
                (ef89_fx)g.bullets[i].base_radius,
                g.bullets[i].expandible_fire_state.scale_fx);
    }
    g.bullets[i].aoi_trail_id = -1;
    g.bullets[i].bolt_projectile_id = -1;
    g.bullets[i].audio_primary_key = 0U;
    g.bullets[i].audio_secondary_key = 0U;

    if (modules && modules->physics_backend == B3D_PHYSICS_BOLT3D) {
        bolt_id = blank3d_bolt_spawn(&g.bolt, modules,
                    request->actor_id, &origin_q12, &direction_q12,
                    (gwp89_fx)request->speed_fx,
                    (gwp89_fx)request->radius_fx,
                    (gwp89_fx)request->damage_fx,
                    request->life_ms, &g.bullets[i]);
        if (bolt_id < 0) {
            release_bullet_slot(i);
            set_status("Bolt3D projectile provider: spawn rejected");
            return 0;
        }
        g.bullets[i].bolt_projectile_id = bolt_id;
    }
    handle->slot = i;
    handle->world_id = g.bullets[i].bolt_projectile_id;
    handle->object = &g.bullets[i];
    return 1;
}

static void b3d_projectile_collision_provider(
    void *user,
    const gps89_request *request,
    const gps89_handle *handle)
{
    (void)user;
    (void)request;
    (void)handle;
    /* Blank3D collision consumes the live bullet pool during update. A host
       with persistent collision objects may register them here instead. */
}

static void b3d_projectile_render_provider(
    void *user,
    const gps89_request *request,
    const gps89_handle *handle)
{
    const Blank3DWeaponModules *modules;
    int i;
    (void)user;
    if (!request || !handle) return;
    i = handle->slot;
    if (i < 0 || i >= MAX_BULLETS) return;
    modules = blank3d_weapon_modules_get(request->weapon_id);
    if (modules && modules->emit_trail &&
        modules->trail_profile != B3D_TRAIL_NONE) {
        g.bullets[i].aoi_trail_id =
            blank3d_trails_attach(&g.trails, i, modules->trail_profile);
        blank3d_trails_emit_q12(&g.trails, i,
            g.bullets[i].transform.position.x,
            g.bullets[i].transform.position.y,
            g.bullets[i].transform.position.z);
    }
}

static void b3d_projectile_audio_provider(
    void *user,
    const gps89_request *request,
    const gps89_handle *handle)
{
    int i;
    (void)user;
    if (!request || !handle) return;
    i = handle->slot;
    if (i < 0 || i >= MAX_BULLETS) return;
    blank3d_audio_projectile_begin(
        &g.audio, request->weapon_id,
        gwp89_fx_to_int_round((gwp89_fx)request->speed_fx),
        &g.bullets[i].audio_primary_key,
        &g.bullets[i].audio_secondary_key);
}

static void b3d_projectile_publish_provider(
    void *user,
    const gps89_request *request,
    const gps89_handle *handle)
{
    (void)user;
    if (!request || !handle || handle->slot < 0 ||
        handle->slot >= MAX_BULLETS) return;
    publish_bullet(handle->slot);
    if (request->weapon_id == GATLING_WEAPON_ID)
        set_status("gatling projectile provider: tracer spawned");
    else if (request->weapon_id == SLINGSHOT_WEAPON_ID)
        set_status("slingshot: Bolt3D stone + Aoi Trail spawned");
}

static B3DBulletSpinEntry *b3d_bullet_spin_entry(
    int actor_id, int weapon_id, const Blank3DWeaponModules *modules)
{
    int i;
    int free_index;
    int count;
    int step;
    int direction;
    bs89_config config;
    B3DBulletSpinEntry *entry;
    if (!modules || !modules->bullet_spin_enabled) return 0;
    count = modules->bullet_circle_count;
    if (count < 1) count = 1;
    if (count > 64) count = 64;
    step = modules->bullet_spin_step;
    if (step < 0) step = -step;
    if (step < 1) step = 1;
    direction = modules->bullet_spin_direction < 0
              ? BS89_DIRECTION_REVERSE : BS89_DIRECTION_FORWARD;
    free_index = -1;
    entry = 0;
    for (i = 0; i < B3D_BULLET_SPIN_STATE_CAPACITY; ++i) {
        if (!g.bullet_spin_states[i].used) {
            if (free_index < 0) free_index = i;
            continue;
        }
        if (g.bullet_spin_states[i].actor_id == actor_id &&
            g.bullet_spin_states[i].weapon_id == weapon_id) {
            entry = &g.bullet_spin_states[i];
            break;
        }
    }
    if (!entry) {
        if (free_index < 0) return 0;
        entry = &g.bullet_spin_states[free_index];
        memset(entry, 0, sizeof(*entry));
        entry->used = 1;
        entry->actor_id = actor_id;
        entry->weapon_id = weapon_id;
    }
    if (!entry->state.initialized || entry->state.slot_count != count ||
        entry->state.step != step || entry->state.direction != direction) {
        memset(&config, 0, sizeof(config));
        config.slot_count = count;
        config.start_slot = modules->bullet_spin_start_slot;
        config.step = step;
        config.direction = direction;
        bulletspin89_init(&entry->state, &config);
    }
    return entry;
}

static int b3d_bullet_geometry_basis(const GWP89_Event *event,
                                     const GWP89_Vec3 *direction_q12,
                                     bc89_vec3 *right_out,
                                     bc89_vec3 *up_out)
{
    Vec3 right;
    Vec3 up;
    Vec3 forward;
    Vec3 reference_up;
    if (!direction_q12 || !right_out || !up_out) return 0;
    right = event ? gwp_to_vec3(event->camera_right)
                  : gamlib_vec3(0, 0, 0);
    up = event ? gwp_to_vec3(event->camera_up)
               : gamlib_vec3(0, 0, 0);
    if (gamlib_vec3_length(&right) > G3D_FIX_EPSILON &&
        gamlib_vec3_length(&up) > G3D_FIX_EPSILON) {
        gamlib_vec3_normalize(&right, &right);
        gamlib_vec3_normalize(&up, &up);
    } else {
        forward = gwp_to_vec3(*direction_q12);
        if (gamlib_vec3_length(&forward) <= G3D_FIX_EPSILON) return 0;
        gamlib_vec3_normalize(&forward, &forward);
        reference_up = gamlib_vec3(0, G3D_FIX_ONE, 0);
        gamlib_vec3_cross(&right, &reference_up, &forward);
        if (gamlib_vec3_length(&right) <= fix_ratio(1, 100)) {
            reference_up = gamlib_vec3(G3D_FIX_ONE, 0, 0);
            gamlib_vec3_cross(&right, &reference_up, &forward);
        }
        if (gamlib_vec3_length(&right) <= G3D_FIX_EPSILON) return 0;
        gamlib_vec3_normalize(&right, &right);
        gamlib_vec3_cross(&up, &forward, &right);
        if (gamlib_vec3_length(&up) <= G3D_FIX_EPSILON) return 0;
        gamlib_vec3_normalize(&up, &up);
    }
    right_out->x = (bc89_fx)right.x;
    right_out->y = (bc89_fx)right.y;
    right_out->z = (bc89_fx)right.z;
    up_out->x = (bc89_fx)up.x;
    up_out->y = (bc89_fx)up.y;
    up_out->z = (bc89_fx)up.z;
    return 1;
}

static void spawn_bullet_event(const GWP89_Event *event)
{
    Vec3 camera_eye;
    Vec3 camera_forward;
    Vec3 camera_right;
    Vec3 camera_up;
    GWP89_Event aligned_event;
    const GWP89_Event *launch_event;
    GWP89_Vec3 direction_q12;
    gwp89_fx gravity_q12;
    const Blank3DWeaponModules *modules;
    gps89_request request;
    gps89_handle handle;
    sat89_request satellite_request;
    sat89_result satellite_result;
    bc89_request circle_request;
    bc89_result circle_result;
    bi89_request inline_request;
    bi89_result inline_result;
    B3DBulletSpinEntry *spin_entry;
    gps89_vec3 center_origin;
    GWP89_Vec3 inline_target;
    Vec3 inline_center;
    Vec3 inline_direction;
    Vec3 inline_scaled;
    Vec3 inline_target_vec;
    gwp89_fx inline_distance;
    int circle_slot;
    int inline_target_valid;
    int captured_view_valid;
    if (!event) return;
    modules = blank3d_weapon_modules_get(event->weapon_id);
    if (b3d_spawn_player_camera_ray(event, modules)) return;

    launch_event = event;
    inline_target_valid = 0;
    memset(&inline_target, 0, sizeof(inline_target));
    /* This branch is deliberately player-only. NPCs/turrets never consume the
       presentation camera; they keep their actor/muzzle aim provider. */
    if (event->actor_id == B3D_PLAYER_ACTOR_ID && g.cameranaku.initialized) {
        camera_eye = gwp_to_vec3(event->camera_origin);
        camera_forward = gwp_to_vec3(event->camera_forward);
        camera_right = gwp_to_vec3(event->camera_right);
        camera_up = gwp_to_vec3(event->camera_up);
        captured_view_valid =
            gamlib_vec3_length(&camera_forward) > G3D_FIX_EPSILON;
        if (!captured_view_valid) {
            blank3d_cameranaku_get_view(&g.cameranaku,
                                        &camera_eye, &camera_forward,
                                        &camera_right, &camera_up);
        } else {
            gamlib_vec3_normalize(&camera_forward, &camera_forward);
            if (gamlib_vec3_length(&camera_right) > G3D_FIX_EPSILON)
                gamlib_vec3_normalize(&camera_right, &camera_right);
            if (gamlib_vec3_length(&camera_up) > G3D_FIX_EPSILON)
                gamlib_vec3_normalize(&camera_up, &camera_up);
        }
        aligned_event = *event;
        aligned_event.camera_origin = vec3_to_gwp(camera_eye);
        aligned_event.camera_forward = vec3_to_gwp(camera_forward);
        aligned_event.camera_right = vec3_to_gwp(camera_right);
        aligned_event.camera_up = vec3_to_gwp(camera_up);
        if (blank3d_player_projectile_aim_prepare(
                &aligned_event, b3d_camera_fire_view_mode(),
                b3d_player_fire_raycast_adapter, &g.collision,
                B3D_COLLISION_LAYER_WORLD | B3D_COLLISION_LAYER_ENEMY,
                &aligned_event)) {
            launch_event = &aligned_event;
            inline_target = aligned_event.hit_point;
            inline_target_valid = 1;
        }
    }

    direction_q12 = launch_event->direction;
    gravity_q12 = 0;
    (void)blank3d_universal_aim_finalize_event(
        launch_event, modules, &direction_q12, &gravity_q12);
    memset(&request, 0, sizeof(request));
    request.actor_id = launch_event->actor_id;
    request.team_id = launch_event->team_id;
    request.target_actor_id = B3D_TARGET_NONE;
    request.weapon_id = launch_event->weapon_id;
    request.projectile_id = launch_event->projectile_id;
    request.projectile_mesh_id = launch_event->projectile_mesh_id;
    request.trail_id = launch_event->trail_id;
    request.physics_kind = modules ? modules->physics_backend : B3D_PHYSICS_LINEAR;
    request.pellet_index = launch_event->pellet_index;
    request.pellet_count = launch_event->pellet_count;
    request.origin.x = launch_event->origin.x;
    request.origin.y = launch_event->origin.y;
    request.origin.z = launch_event->origin.z;
    if (modules && modules->satellaborner_enabled) {
        memset(&satellite_request, 0, sizeof(satellite_request));
        satellite_request.owner_actor_id = launch_event->actor_id;
        satellite_request.requested_target_id = SAT89_TARGET_NONE;
        satellite_request.original_origin.x = request.origin.x;
        satellite_request.original_origin.y = request.origin.y;
        satellite_request.original_origin.z = request.origin.z;
        satellite_request.target_offset.x =
            (sat89_fx)(modules->satellaborner_offset_x_q16 / 16L);
        satellite_request.target_offset.y =
            (sat89_fx)(modules->satellaborner_offset_y_q16 / 16L);
        satellite_request.target_offset.z =
            (sat89_fx)(modules->satellaborner_offset_z_q16 / 16L);
        if (satellaborner89_resolve(&satellite_request,
                b3d_satellaborner_target_provider, 0, &satellite_result) &&
            satellite_result.target_found) {
            request.target_actor_id = satellite_result.target_actor_id;
            request.origin.x = satellite_result.spawn_origin.x;
            request.origin.y = satellite_result.spawn_origin.y;
            request.origin.z = satellite_result.spawn_origin.z;
            inline_target.x = (gwp89_fx)satellite_result.target_position.x;
            inline_target.y = (gwp89_fx)satellite_result.target_position.y;
            inline_target.z = (gwp89_fx)satellite_result.target_position.z;
            inline_target_valid = 1;
        }
    }

    /* Geometric projectile origin composition. The center origin remains the
       weapon/relocator authority; the circular helper only offsets that
       origin. Spin state is owned per actor+weapon so independent Gatlings do
       not share barrel phase. */
    center_origin = request.origin;
    circle_slot = launch_event->pellet_index;
    if (circle_slot < 0) circle_slot = 0;
    if (modules && modules->bullet_spin_enabled) {
        spin_entry = b3d_bullet_spin_entry(launch_event->actor_id,
                                            launch_event->weapon_id, modules);
        if (spin_entry)
            (void)bulletspin89_next(&spin_entry->state, &circle_slot);
    }
    if (modules && modules->bullet_circle_enabled) {
        memset(&circle_request, 0, sizeof(circle_request));
        circle_request.center.x = (bc89_fx)center_origin.x;
        circle_request.center.y = (bc89_fx)center_origin.y;
        circle_request.center.z = (bc89_fx)center_origin.z;
        circle_request.radius_fx =
            (bc89_fx)(modules->bullet_circle_radius_q16 / 16L);
        circle_request.slot_index = circle_slot;
        circle_request.slot_count = modules->bullet_circle_count;
        if (circle_request.slot_count < 1) circle_request.slot_count = 1;
        if (circle_request.slot_count > 64) circle_request.slot_count = 64;
        circle_request.phase_turn_q16 = modules->bullet_circle_phase_turn_q16;
        if (b3d_bullet_geometry_basis(launch_event, &direction_q12,
                                     &circle_request.right,
                                     &circle_request.up) &&
            bulletcircle89_resolve(&circle_request, &circle_result) &&
            circle_result.valid) {
            request.origin.x = (gps89_fx)circle_result.origin.x;
            request.origin.y = (gps89_fx)circle_result.origin.y;
            request.origin.z = (gps89_fx)circle_result.origin.z;
        }
    }

    request.direction.x = direction_q12.x;
    request.direction.y = direction_q12.y;
    request.direction.z = direction_q12.z;
    if (modules && modules->bullet_inline_enabled) {
        if (!inline_target_valid) {
            inline_distance =
                (gwp89_fx)(modules->bullet_inline_distance_q16 / 16L);
            if (inline_distance <= 0) inline_distance = launch_event->range_fx;
            if (inline_distance <= 0)
                inline_distance = (gwp89_fx)G3D_FIX_FROM_INT(120);
            inline_center = gamlib_vec3((g3d_fix)center_origin.x,
                                        (g3d_fix)center_origin.y,
                                        (g3d_fix)center_origin.z);
            inline_direction = gwp_to_vec3(direction_q12);
            if (gamlib_vec3_length(&inline_direction) > G3D_FIX_EPSILON) {
                gamlib_vec3_normalize(&inline_direction, &inline_direction);
                gamlib_vec3_scale(&inline_scaled, &inline_direction,
                                  (g3d_fix)inline_distance);
                gamlib_vec3_add(&inline_target_vec, &inline_center,
                                &inline_scaled);
                inline_target = vec3_to_gwp(inline_target_vec);
                inline_target_valid = 1;
            }
        }
        memset(&inline_request, 0, sizeof(inline_request));
        inline_request.origin.x = (bi89_fx)request.origin.x;
        inline_request.origin.y = (bi89_fx)request.origin.y;
        inline_request.origin.z = (bi89_fx)request.origin.z;
        inline_request.target.x = (bi89_fx)inline_target.x;
        inline_request.target.y = (bi89_fx)inline_target.y;
        inline_request.target.z = (bi89_fx)inline_target.z;
        inline_request.fallback_direction.x = (bi89_fx)direction_q12.x;
        inline_request.fallback_direction.y = (bi89_fx)direction_q12.y;
        inline_request.fallback_direction.z = (bi89_fx)direction_q12.z;
        inline_request.target_valid = inline_target_valid;
        if (bulletinline89_resolve(&inline_request, &inline_result) &&
            inline_result.valid) {
            request.direction.x = (gps89_fx)inline_result.direction.x;
            request.direction.y = (gps89_fx)inline_result.direction.y;
            request.direction.z = (gps89_fx)inline_result.direction.z;
        }
    }
    request.speed_fx = launch_event->speed_fx;
    request.gravity_fx = gravity_q12;
    request.radius_fx = launch_event->radius_fx;
    request.damage_fx = launch_event->damage_fx;
    request.mesh_scale_fx = launch_event->projectile_mesh_scale_fx;
    request.life_ms = launch_event->life_ms;
    request.flags = launch_event->flags;
    request.source = launch_event;
    (void)gprojectilespawn89_spawn(&g.projectile_spawn_runtime,
                                    &request, &handle);
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

static int b3d_casing_world_spawn_provider(
    void *user,
    const gcr89_request *request,
    gcr89_handle *handle)
{
    int i;
    (void)user;
    if (!request || !handle) return 0;
    for (i = 0; i < MAX_CASINGS; ++i) {
        if (g.casings[i].alive) continue;
        g.casings[i].alive = 1;
        g.casings[i].weapon_id = request->weapon_id;
        g.casings[i].casing_mesh_id = request->casing_mesh_id;
        if (g.casings[i].casing_mesh_id < 1 ||
            g.casings[i].casing_mesh_id > CASING_MESH_COUNT)
            g.casings[i].casing_mesh_id =
                request->weapon_id == GATLING_WEAPON_ID ? 2 : request->weapon_id;
        if (g.casings[i].casing_mesh_id < 1 ||
            g.casings[i].casing_mesh_id > CASING_MESH_COUNT)
            g.casings[i].casing_mesh_id = 1;
        g.casings[i].mesh_scale = (g3d_fix)request->mesh_scale_fx;
        if (g.casings[i].mesh_scale <= 0)
            g.casings[i].mesh_scale = G3D_FIX_ONE;
        transform_init(&g.casings[i].transform);
        g.casings[i].transform.position = gamlib_vec3(
            (g3d_fix)request->origin.x,
            (g3d_fix)request->origin.y,
            (g3d_fix)request->origin.z);
        g.casings[i].transform.scale = gamlib_vec3(
            g.casings[i].mesh_scale,
            g.casings[i].mesh_scale,
            g.casings[i].mesh_scale);
        g.casings[i].physics_active = 0;
        g.casings[i].velocity = gamlib_vec3(0,0,0);
        g.casings[i].life = G3D_FIX_FROM_INT(3);
        handle->slot = i;
        handle->physics_id = -1;
        handle->object = &g.casings[i];
        return 1;
    }
    return 0;
}

static void b3d_casing_physics_provider(
    void *user,
    const gcr89_request *request,
    const gcr89_handle *handle)
{
    Vec3 right;
    Vec3 up;
    Vec3 forward;
    Vec3 impulse;
    Vec3 offset;
    int i;
    (void)user;
    if (!request || !handle) return;
    i = handle->slot;
    if (i < 0 || i >= MAX_CASINGS) return;
    right = gamlib_vec3((g3d_fix)request->right.x,
                        (g3d_fix)request->right.y,
                        (g3d_fix)request->right.z);
    up = gamlib_vec3((g3d_fix)request->up.x,
                     (g3d_fix)request->up.y,
                     (g3d_fix)request->up.z);
    forward = gamlib_vec3((g3d_fix)request->forward.x,
                          (g3d_fix)request->forward.y,
                          (g3d_fix)request->forward.z);
    g.casings[i].physics_active = blank3d_casing_physics_spawn(
        &g.casing_physics, i, request->weapon_id,
        g.casings[i].casing_mesh_id,
        &g.casings[i].transform.position,
        &right, &up, &forward);
    if (!g.casings[i].physics_active) {
        gamlib_vec3_scale(&impulse, &right,
            request->weapon_id == GATLING_WEAPON_ID ? G3D_FIX_FROM_INT(5) :
            (request->weapon_id == 2 ? G3D_FIX_FROM_INT(4) :
             G3D_FIX_FROM_INT(3)));
        gamlib_vec3_scale(&offset, &up,
            request->casing_mesh_id == 3 ? G3D_FIX_FROM_INT(3) :
            G3D_FIX_FROM_INT(2));
        gamlib_vec3_add(&g.casings[i].velocity, &impulse, &offset);
    }
}

static void b3d_casing_render_provider(
    void *user,
    const gcr89_request *request,
    const gcr89_handle *handle)
{
    (void)user; (void)request; (void)handle;
    /* Blank3D's renderer reads the casing pool directly. Other hosts can
       allocate a render proxy here. */
}

static void b3d_casing_audio_provider(
    void *user,
    const gcr89_request *request,
    const gcr89_handle *handle)
{
    (void)user; (void)handle;
    if (request) blank3d_audio_casing(&g.audio, request->weapon_id);
}

static void b3d_casing_publish_provider(
    void *user,
    const gcr89_request *request,
    const gcr89_handle *handle)
{
    (void)user; (void)request; (void)handle;
}

static void spawn_casing_event(const GWP89_Event *event)
{
    Vec3 right;
    Vec3 up;
    Vec3 forward;
    gcr89_request request;
    gcr89_handle handle;
    if (!event || !blank3d_weapon_modules_has_casing(event->weapon_id))
        return;
    b3d_casing_axes_for_event(event, &right, &up, &forward);
    memset(&request, 0, sizeof(request));
    request.actor_id = event->actor_id;
    request.weapon_id = event->weapon_id;
    request.casing_mesh_id = event->shell_mesh_id;
    request.mesh_scale_fx = event->shell_mesh_scale_fx;
    request.origin.x = event->origin.x;
    request.origin.y = event->origin.y;
    request.origin.z = event->origin.z;
    request.right.x = right.x; request.right.y = right.y; request.right.z = right.z;
    request.up.x = up.x; request.up.y = up.y; request.up.z = up.z;
    request.forward.x = forward.x;
    request.forward.y = forward.y;
    request.forward.z = forward.z;
    request.source = event;
    (void)gcasingruntime89_spawn(&g.casing_runtime, &request, &handle);
}

static void init_weapon_runtime_providers(void)
{
    gps89_providers projectile_providers;
    gcr89_providers casing_providers;
    memset(&projectile_providers, 0, sizeof(projectile_providers));
    projectile_providers.world_spawn = b3d_projectile_world_spawn_provider;
    projectile_providers.collision_register = b3d_projectile_collision_provider;
    projectile_providers.render_spawn = b3d_projectile_render_provider;
    projectile_providers.audio_spawn = b3d_projectile_audio_provider;
    projectile_providers.world_publish = b3d_projectile_publish_provider;
    gprojectilespawn89_init(&g.projectile_spawn_runtime,
                            &projectile_providers);

    memset(&casing_providers, 0, sizeof(casing_providers));
    casing_providers.world_spawn = b3d_casing_world_spawn_provider;
    casing_providers.physics_spawn = b3d_casing_physics_provider;
    casing_providers.render_spawn = b3d_casing_render_provider;
    casing_providers.audio_spawn = b3d_casing_audio_provider;
    casing_providers.world_publish = b3d_casing_publish_provider;
    gcasingruntime89_init(&g.casing_runtime, &casing_providers);
}

static mbv89_fixed b3d_mbv_linear_from_g3d(g3d_fix value)
{
    if (value > (g3d_fix)(INT_MAX / 16)) return (mbv89_fixed)INT_MAX;
    if (value < (g3d_fix)(INT_MIN / 16)) return (mbv89_fixed)INT_MIN;
    return (mbv89_fixed)(value * 16L);
}

static mbv89_fixed b3d_mbv_turns_from_g3d_degrees(g3d_fix degrees)
{
    long q;
    long r;
    long result;
    q = (long)degrees / 45L;
    r = (long)degrees % 45L;
    result = q * 2L + (r * 2L) / 45L;
    if (result > (long)INT_MAX) return (mbv89_fixed)INT_MAX;
    if (result < (long)INT_MIN) return (mbv89_fixed)INT_MIN;
    return (mbv89_fixed)result;
}

static void b3d_movement_sync_steps(void)
{
    g3d_fix walk_distance;
    g3d_fix strafe_distance;
    g3d_fix fly_distance;
    g3d_fix turn_degrees;
    g3d_fix run_distance;

    walk_distance = g3d_fix_mul(g.move_speed, g.dt);
    strafe_distance = g3d_fix_mul(g.strafe_speed, g.dt);
    fly_distance = g3d_fix_mul(g.vertical_speed, g.dt);
    turn_degrees = g3d_fix_mul(g.turn_speed, g.dt);
    run_distance = g3d_fix_add_sat(walk_distance, walk_distance);

    g.movement_verbs.walk_step = b3d_mbv_linear_from_g3d(walk_distance);
    g.movement_verbs.strafe_step = b3d_mbv_linear_from_g3d(strafe_distance);
    g.movement_verbs.fly_step = b3d_mbv_linear_from_g3d(fly_distance);
    g.movement_verbs.run_step = b3d_mbv_linear_from_g3d(run_distance);
    g.movement_verbs.turn_step = b3d_mbv_turns_from_g3d_degrees(turn_degrees);
}

static int b3d_movement_game_provider(void *provider_user,
                                      mbv89_context *ctx,
                                      mbv89_actor *actor,
                                      mbv89_game_verb verb)
{
    (void)provider_user;
    (void)ctx;
    (void)actor;

    /* Player locomotion is suspended while a playerdriving vehicle owns the
       control role.  The separate vehicle_driver.ddsl2 emits the vehicle
       verbs; this layer only prevents the rider from walking inside the seat. */
    if (blank3d_vehicle_system_playerdriving(&g.vehicles)) {
        if (verb == MBV89_GAME_WALK_FORWARD ||
            verb == MBV89_GAME_RUN_FORWARD ||
            verb == MBV89_GAME_WALK_BACKWARD ||
            verb == MBV89_GAME_RUN_BACKWARD ||
            verb == MBV89_GAME_STRAFE_LEFT ||
            verb == MBV89_GAME_STRAFE_RIGHT ||
            verb == MBV89_GAME_TURN_LEFT ||
            verb == MBV89_GAME_TURN_RIGHT)
            return MBV89_HANDLED;
    }

    if (g.kinverbs.initialized &&
        (verb == MBV89_GAME_WALK_FORWARD ||
         verb == MBV89_GAME_WALK_BACKWARD ||
         verb == MBV89_GAME_STRAFE_LEFT ||
         verb == MBV89_GAME_STRAFE_RIGHT ||
         verb == MBV89_GAME_RUN_FORWARD ||
         verb == MBV89_GAME_RUN_BACKWARD) &&
        !blank3d_kinverbs_can_game_verb(&g.kinverbs,
            (unsigned long)B3D_PLAYER_ACTOR_ID, verb,
            b3d_movement_game_amount(verb)))
        return MBV89_HANDLED;

    if (g.gloco.initialized &&
        blank3d_gloco_mbv_game_provider(&g.gloco, verb) == MBV89_HANDLED)
        return MBV89_HANDLED;

    if (verb == MBV89_GAME_FLY_UP || verb == MBV89_GAME_FLY_DOWN) {
        if (!blank3d_vertical_axis_jumping(&g.player_vertical)) {
            (void)blank3d_vertical_axis_fly(
                &g.vertical_axis,
                &g.player_vertical,
                g.vertical_speed,
                g.dt,
                verb == MBV89_GAME_FLY_UP ? FLY89_UP : FLY89_DOWN);
        }
        return MBV89_HANDLED;
    }

    if (verb == MBV89_GAME_TURN_LEFT || verb == MBV89_GAME_TURN_RIGHT) {
        g3d_fix delta;
        delta = g3d_fix_mul(g.turn_speed, g.dt);
        if (verb == MBV89_GAME_TURN_LEFT)
            blank3d_cameranaku_feed_left(&g.cameranaku, delta);
        else
            blank3d_cameranaku_feed_right(&g.cameranaku, delta);
        g.camera_yaw = g.cameranaku.yaw;
        g.camera_pitch = g.cameranaku.pitch;
        g.player.rotation.y = g.camera_yaw;
        return MBV89_HANDLED;
    }

    return MBV89_UNHANDLED;
}

static mbv89_fixed b3d_movement_base_amount(mbv89_base_verb verb)
{
    switch (verb) {
        case MBV89_BASE_MOVE_LEFT:
        case MBV89_BASE_MOVE_RIGHT:
            return g.movement_verbs.strafe_step;
        case MBV89_BASE_MOVE_UP:
        case MBV89_BASE_MOVE_DOWN:
            return g.movement_verbs.fly_step;
        case MBV89_BASE_ROTATE_FORWARD:
        case MBV89_BASE_ROTATE_BACKWARD:
        case MBV89_BASE_ROTATE_LEFT:
        case MBV89_BASE_ROTATE_RIGHT:
        case MBV89_BASE_ROTATE_UP:
        case MBV89_BASE_ROTATE_DOWN:
            return g.movement_verbs.turn_step;
        default:
            return g.movement_verbs.walk_step;
    }
}

static int b3d_try_movement_verb(const char *action)
{
    mbv89_base_verb base_verb;
    mbv89_game_verb game_verb;

    if (!action) return 0;
    b3d_movement_sync_steps();

    /* Legacy Blank3D vertical names are semantic flight, not raw Y edits. */
    if (strcmp(action, "move_up") == 0 ||
        strcmp(action, "move_y_up") == 0) {
        (void)mbv89_perform_game(&g.movement_verbs,
                                 &g.player_movement_actor,
                                 MBV89_GAME_FLY_UP);
        return 1;
    }
    if (strcmp(action, "move_down") == 0 ||
        strcmp(action, "move_y_down") == 0) {
        (void)mbv89_perform_game(&g.movement_verbs,
                                 &g.player_movement_actor,
                                 MBV89_GAME_FLY_DOWN);
        return 1;
    }

    if (mbv89_game_verb_from_name(action, &game_verb)) {
        (void)mbv89_perform_game(&g.movement_verbs,
                                 &g.player_movement_actor,
                                 game_verb);
        return 1;
    }

    if (mbv89_base_verb_from_name(action, &base_verb)) {
        (void)mbv89_perform_base(&g.movement_verbs,
                                 &g.player_movement_actor,
                                 base_verb,
                                 b3d_movement_base_amount(base_verb));
        return 1;
    }

    return 0;
}

static int b3d_gameverb_name_to_movement(const char *name,
                                         mbv89_game_verb *verb)
{
    if (!name || !verb) return 0;
    if (mbv89_game_verb_from_name(name, verb)) return 1;
    if (strcmp(name, "move_forward") == 0 ||
        strcmp(name, "move_foward") == 0) {
        *verb = MBV89_GAME_WALK_FORWARD; return 1;
    }
    if (strcmp(name, "move_backward") == 0 || strcmp(name, "move_back") == 0) {
        *verb = MBV89_GAME_WALK_BACKWARD; return 1;
    }
    if (strcmp(name, "move_left") == 0) {
        *verb = MBV89_GAME_STRAFE_LEFT; return 1;
    }
    if (strcmp(name, "move_right") == 0) {
        *verb = MBV89_GAME_STRAFE_RIGHT; return 1;
    }
    return 0;
}

static int b3d_gameverb_movement_action(void *user, const gverb89_call *call)
{
    mbv89_game_verb verb;
    int actor_id;
    (void)user;
    if (!call || !b3d_gameverb_name_to_movement(call->name, &verb))
        return GVERB89_UNHANDLED;
    actor_id = (int)call->owner;
    if (actor_id == 0) actor_id = B3D_PLAYER_ACTOR_ID;
    b3d_movement_sync_steps();
    if (g.kinverbs.initialized &&
        !blank3d_kinverbs_can_game_verb(&g.kinverbs,
            (unsigned long)actor_id, verb, b3d_movement_game_amount(verb)))
        return GVERB89_HANDLED;
    if (actor_id == B3D_PLAYER_ACTOR_ID) {
        (void)mbv89_perform_game(&g.movement_verbs,
                                 &g.player_movement_actor, verb);
        return GVERB89_HANDLED;
    }
    if (g.gloco.initialized &&
        blank3d_gloco_actor_game_verb(&g.gloco, actor_id, verb)
            == MBV89_HANDLED)
        return GVERB89_HANDLED;
    return GVERB89_UNHANDLED;
}

static int b3d_register_movement_gameverbs(void)
{
    static const char *names[] = {
        "walk_forward", "walk_foward", "walk_backward", "walk_back",
        "strafe_left", "strafe_right",
        "run_forward", "run_foward", "run_backward", "run_back",
        "move_forward", "move_foward", "move_backward", "move_back",
        "move_left", "move_right"
    };
    unsigned int i;
    gverb89_registry *registry;
    registry = blank3d_kinverbs_registry(&g.kinverbs);
    if (!registry) return 0;
    for (i = 0U; i < (unsigned int)(sizeof(names) / sizeof(names[0])); ++i)
        if (!gverb89_register_action(registry, names[i],
                                  b3d_gameverb_movement_action, &g))
            return 0;
    return 1;
}

static int b3d_gameverb_vehicle_condition(void *user,
                                           const gverb89_call *call,
                                           gverb89_result *out)
{
    (void)user;
    if (!call || !out || strcmp(call->name, "vehicle_playerdriving") != 0)
        return GVERB89_UNHANDLED;
    out->truth = blank3d_vehicle_system_playerdriving(&g.vehicles) ? 1 : 0;
    out->value_q16 = out->truth ? 65536L : 0L;
    out->instance_id = g.vehicles.active_player_slot;
    return GVERB89_HANDLED;
}

static int b3d_gameverb_vehicle_action(void *user, const gverb89_call *call)
{
    const Blank3DVehicleInstance *active;
    const Transform *car;
    (void)user;
    if (!call || !blank3d_vehicle_system_playerdriving(&g.vehicles))
        return GVERB89_HANDLED;
    if (strcmp(call->name, "vehicle_forward") == 0)
        blank3d_vehicle_system_forward(&g.vehicles, g.dt, 0);
    else if (strcmp(call->name, "vehicle_run_forward") == 0)
        blank3d_vehicle_system_forward(&g.vehicles, g.dt, 1);
    else if (strcmp(call->name, "vehicle_reverse") == 0)
        blank3d_vehicle_system_backward(&g.vehicles, g.dt, 0);
    else if (strcmp(call->name, "vehicle_run_reverse") == 0)
        blank3d_vehicle_system_backward(&g.vehicles, g.dt, 1);
    else if (strcmp(call->name, "vehicle_steer_left") == 0)
        blank3d_vehicle_system_steer(&g.vehicles, g.dt, 0);
    else if (strcmp(call->name, "vehicle_steer_right") == 0)
        blank3d_vehicle_system_steer(&g.vehicles, g.dt, 1);
    else
        return GVERB89_UNHANDLED;
    active = blank3d_vehicle_system_active_const(&g.vehicles);
    if (active) {
        car = blank3d_mount_vehicle_car_transform_const(&active->mount);
        if (car) {
            g.camera_yaw = car->rotation.y;
            g.cameranaku.yaw = g.camera_yaw;
        }
    }
    return GVERB89_HANDLED;
}

static int b3d_register_vehicle_gameverbs(void)
{
    static const char *actions[] = {
        "vehicle_forward", "vehicle_run_forward",
        "vehicle_reverse", "vehicle_run_reverse",
        "vehicle_steer_left", "vehicle_steer_right"
    };
    gverb89_registry *registry;
    unsigned int i;
    registry = blank3d_kinverbs_registry(&g.kinverbs);
    if (!registry) return 0;
    if (!gverb89_register_condition(registry, "vehicle_playerdriving",
            b3d_gameverb_vehicle_condition, &g)) return 0;
    for (i = 0U; i < (unsigned int)(sizeof(actions) / sizeof(actions[0])); ++i)
        if (!gverb89_register_action(registry, actions[i],
                b3d_gameverb_vehicle_action, &g)) return 0;
    return 1;
}

static unsigned long b3d_gameverb_owner_for_entity(void *user, void *entity)
{
    int i;
    (void)user;
    if (!entity || entity == (void *)&g.player)
        return (unsigned long)B3D_PLAYER_ACTOR_ID;
    for (i = 0; i < MAX_ENEMIES; ++i)
        if (entity == (void *)&g.enemies[i])
            return (unsigned long)g.enemies[i].weapon_actor_id;
    return 0UL;
}

static void apply_ddsl2_action(const char *action)
{
    if (b3d_try_movement_verb(action)) {
        return;
    }
    if (strcmp(action, "jump") == 0) {
        (void)blank3d_vertical_axis_jump(&g.vertical_axis,
                                         &g.player_vertical,
                                         G3D_FIX_FROM_INT(9));
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
    if (sv89_resolve_verb(action) != SV89_VERB_UNKNOWN &&
        value_text && value_text[0] != '\0') {
        if (!blank3d_sprite_runtime89_execute_ddsl(&g.sprite_runtime,
                                                    action, value_text))
            set_status(blank3d_sprite_runtime89_status(&g.sprite_runtime));
        return;
    }
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
    if (blank3d_timeverbs89_execute(&g.time_verbs, action,
                                     value_fixed, value_text,
                                     (const char **)0, 0))
        return;
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
    (void)as89_register(&g.actors, g.enemies[index].weapon_actor_id,
        0UL, (unsigned long)(index + 1), 1, 1, 0UL);
    if (index + 1 > g.enemy_count) g.enemy_count = index + 1;
}

static int b3d_condor_parse_event_type_text(const char *text, int *out_type)
{
    if (!text || !out_type) return 0;
    if (strcmp(text, "any") == 0 || strcmp(text, "*") == 0) {
        *out_type = CEA89_MATCH_ANY;
        return 1;
    }
    if (blank3d_condor_event_type_from_name(text, out_type)) return 1;
    if ((*text >= '0' && *text <= '9') || *text == '-') {
        *out_type = atoi(text);
        return 1;
    }
    return 0;
}

static int b3d_condor_parse_event_code_text(int event_type,
                                             const char *text,
                                             int *out_code)
{
    input_key89 key;
    unsigned int usage;
    if (!text || !out_code) return 0;
    if (strcmp(text, "any") == 0 || strcmp(text, "*") == 0) {
        *out_code = CEA89_MATCH_ANY;
        return 1;
    }
    if (event_type == B3D_CEA_EVENT_INPUT_PRESS ||
        event_type == B3D_CEA_EVENT_INPUT_HOLD ||
        event_type == B3D_CEA_EVENT_INPUT_RELEASE) {
        key = blank3d_input_key_from_name(text);
        if (input_keys89_keyboard_usage(key, &usage) && usage <= 255U) {
            *out_code = (int)usage;
            return 1;
        }
        if (strcmp(text, "mouse_left") == 0 || strcmp(text, "mouse1") == 0) {
            *out_code = B3D_CEA_MOUSE_CODE_BASE; return 1;
        }
        if (strcmp(text, "mouse_right") == 0 || strcmp(text, "mouse2") == 0) {
            *out_code = B3D_CEA_MOUSE_CODE_BASE + 1; return 1;
        }
    }
    if ((*text >= '0' && *text <= '9') || *text == '-') {
        *out_code = atoi(text);
        return 1;
    }
    return 0;
}

static void b3d_language_rpyl_command(void *user, const char *command,
                                      const char **args, int argc)
{
    g3d_fix a;
    g3d_fix b;
    g3d_fix c;
    (void)user;
    if (!command) return;
    if (argc > 0 && !args) return;

    if (strcmp(command, "condor_rule") == 0 && argc >= 4) {
        int event_type;
        int event_code;
        const char *condition_name;
        cea89_rule_id rule;
        if (!b3d_condor_parse_event_type_text(args[0], &event_type) ||
            !b3d_condor_parse_event_code_text(event_type, args[1], &event_code)) {
            set_status("condor_rule: invalid event type/code");
            return;
        }
        condition_name = (strcmp(args[2], "always") == 0 ||
                          strcmp(args[2], "-") == 0) ? 0 : args[2];
        rule = blank3d_condor_rule_gameverbs(&g.condor,
                    event_type, event_code, condition_name, args[3]);
        set_status(rule ? "Condor rule registered" : blank3d_condor_status(&g.condor));
        return;
    }
    if (strcmp(command, "condor_emit") == 0 && argc >= 2) {
        int event_type;
        int event_code;
        if (!b3d_condor_parse_event_type_text(args[0], &event_type) ||
            !b3d_condor_parse_event_code_text(event_type, args[1], &event_code)) {
            set_status("condor_emit: invalid event type/code");
            return;
        }
        (void)blank3d_condor_emit(&g.condor, event_type, event_code,
            (unsigned long)B3D_PLAYER_ACTOR_ID, g.variables.player_owner,
            &g.player, 0);
        return;
    }

    if ((strcmp(command, "skybox") == 0 ||
         strcmp(command, "skybox_recipe") == 0) && argc >= 1) {
        g.skybox_rpyl_override = 1;
        if (strcmp(args[0], "off") == 0 ||
            strcmp(args[0], "none") == 0 ||
            strcmp(args[0], "disable") == 0) {
            blank3d_skybox_recipe89_disable(&g.skybox_recipes);
            set_status(blank3d_skybox_recipe89_status(&g.skybox_recipes));
            return;
        }
        if (blank3d_skybox_recipe89_apply_named(&g.skybox_recipes,
                                                 args[0]))
            set_status(blank3d_skybox_recipe89_status(&g.skybox_recipes));
        else
            set_status(blank3d_skybox_recipe89_status(&g.skybox_recipes));
        return;
    }
    if (strcmp(command, "skybox_recipe_path") == 0 && argc >= 1) {
        g.skybox_rpyl_override = 1;
        (void)blank3d_skybox_recipe89_apply_path(&g.skybox_recipes, args[0]);
        set_status(blank3d_skybox_recipe89_status(&g.skybox_recipes));
        return;
    }
    if (strcmp(command, "skybox_reload") == 0) {
        g.skybox_rpyl_override = 1;
        (void)blank3d_skybox_recipe89_reload(&g.skybox_recipes);
        set_status(blank3d_skybox_recipe89_status(&g.skybox_recipes));
        return;
    }

    if (g.kinverbs.initialized) {
        gverb89_call verb_call;
        memset(&verb_call, 0, sizeof(verb_call));
        verb_call.owner = (unsigned long)B3D_PLAYER_ACTOR_ID;
        verb_call.subject = &g.player;
        verb_call.name = command;
        verb_call.argv = args;
        verb_call.argc = argc;
        if (gverb89_perform(blank3d_kinverbs_registry(&g.kinverbs),
                         &verb_call) == GVERB89_HANDLED)
            return;
    }

    if (strcmp(command, "scene") == 0) return;
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
    if ((strcmp(command, "camera_profile") == 0 ||
         strcmp(command, "camera") == 0) && argc == 1) {
        if (blank3d_cameranaku_set_profile(&g.cameranaku, args[0])) {
            b3d_camera_sync_profile_fields();
            set_status(blank3d_cameranaku_status(&g.cameranaku));
        }
        return;
    }
    if (strcmp(command, "camera") == 0 && argc >= 7 &&
        strcmp(args[1], "dist") == 0 &&
        strcmp(args[3], "height") == 0 &&
        strcmp(args[5], "fov") == 0 &&
        parse_fixed(args[2], &a) && parse_fixed(args[4], &b) &&
        parse_fixed(args[6], &c)) {
        if (strcmp(args[0], "firstperson") == 0)
            (void)blank3d_cameranaku_set_profile(&g.cameranaku, "fps");
        else
            (void)blank3d_cameranaku_set_profile(&g.cameranaku, args[0]);
        b3d_camera_sync_profile_fields();
        g.camera_dist = a;
        g.camera_height = b;
        g.fov = c;
        g.base_fov = c;
        blank3d_cameranaku_set_fov(&g.cameranaku, c);
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
    if (strcmp(command, "image_asset") == 0 && argc >= 2) {
        int image_id;
        image_id = atoi(args[0]);
        if (!blank3d_image_assets_register(&g.images, image_id, args[1]))
            set_status(blank3d_image_assets_error(&g.images));
        return;
    }
    if (strcmp(command, "asset_root") == 0 && argc >= 2) {
        int kind;
        int recursive;
        kind = blank3d_sprite_runtime89_kind(args[0]);
        recursive = argc >= 3 ? atoi(args[2]) != 0 : 1;
        if (kind == AR89_KIND_ANY ||
            !blank3d_sprite_runtime89_add_root(&g.sprite_runtime, kind,
                                                args[1], recursive))
            set_status(blank3d_sprite_runtime89_status(&g.sprite_runtime));
        return;
    }
    if (strcmp(command, "asset_alias") == 0 && argc >= 3) {
        int kind;
        kind = blank3d_sprite_runtime89_kind(args[0]);
        if (kind == AR89_KIND_ANY ||
            !blank3d_sprite_runtime89_alias(&g.sprite_runtime, kind,
                                             args[1], args[2]))
            set_status(blank3d_sprite_runtime89_status(&g.sprite_runtime));
        return;
    }
    if (strcmp(command, "asset_scan") == 0) {
        if (!blank3d_sprite_runtime89_scan(&g.sprite_runtime))
            set_status(blank3d_sprite_runtime89_status(&g.sprite_runtime));
        return;
    }
    if (strcmp(command, "sprite_static") == 0 && argc >= 2) {
        unsigned int duration_ms;
        duration_ms = argc >= 3 ? (unsigned int)atoi(args[2]) : 1000U;
        if (!blank3d_sprite_runtime89_define_static(&g.sprite_runtime,
                                                     args[0], args[1],
                                                     duration_ms))
            set_status(blank3d_sprite_runtime89_status(&g.sprite_runtime));
        return;
    }
    if (strcmp(command, "sprite_grid") == 0 && argc >= 8) {
        int loop_mode;
        loop_mode = blank3d_sprite_runtime89_loop_mode(
                        argc >= 9 ? args[8] : "loop");
        if (!blank3d_sprite_runtime89_define_grid(&g.sprite_runtime,
                args[0], args[1], args[2],
                atoi(args[3]), atoi(args[4]),
                (unsigned int)atoi(args[5]),
                (unsigned int)atoi(args[6]),
                (unsigned int)atoi(args[7]), loop_mode))
            set_status(blank3d_sprite_runtime89_status(&g.sprite_runtime));
        return;
    }
    if (strcmp(command, "sprite_renlist") == 0 && argc >= 1) {
        if (!blank3d_sprite_runtime89_import_renlist_file(
                &g.sprite_runtime, args[0], 0))
            set_status(blank3d_sprite_runtime89_status(&g.sprite_runtime));
        return;
    }
    if (strcmp(command, "sprite_aseprite") == 0 && argc >= 2) {
        if (!blank3d_sprite_runtime89_import_aseprite_file(
                &g.sprite_runtime, args[0], args[1],
                argc >= 3 ? args[2] : 0, 0))
            set_status(blank3d_sprite_runtime89_status(&g.sprite_runtime));
        return;
    }
    if ((strcmp(command, "spriteverb") == 0 && argc >= 2) ||
        (sv89_resolve_verb(command) != SV89_VERB_UNKNOWN && argc >= 1)) {
        const char *target_name;
        const char *verb_name;
        int first_arg;
        char sprite_arg[SV89_ARG_CAP];
        unsigned int used;
        int ai;
        if (strcmp(command, "spriteverb") == 0) {
            target_name = args[0];
            verb_name = args[1];
            first_arg = 2;
        } else {
            target_name = args[0];
            verb_name = command;
            first_arg = 1;
        }
        sprite_arg[0] = '\0';
        used = 0U;
        for (ai = first_arg; ai < argc; ++ai) {
            unsigned int si;
            if (used && used + 1U < (unsigned int)sizeof(sprite_arg))
                sprite_arg[used++] = ' ';
            si = 0U;
            while (args[ai][si] && used + 1U < (unsigned int)sizeof(sprite_arg))
                sprite_arg[used++] = args[ai][si++];
            sprite_arg[used] = '\0';
        }
        if (!blank3d_sprite_runtime89_execute(&g.sprite_runtime,
                target_name, verb_name, sprite_arg[0] ? sprite_arg : 0))
            set_status(blank3d_sprite_runtime89_status(&g.sprite_runtime));
        return;
    }
    if (strcmp(command, "spriteplane") == 0 && argc >= 10 &&
        strcmp(args[1], "pos") == 0 &&
        strcmp(args[5], "size") == 0 &&
        strcmp(args[8], "mode") == 0 &&
        parse_fixed(args[2], &a) && parse_fixed(args[3], &b) &&
        parse_fixed(args[4], &c)) {
        g3d_fix w;
        g3d_fix h;
        Blank3DSpritePlaneSpec spec;
        int argi;
        if (!parse_fixed(args[6], &w) || !parse_fixed(args[7], &h)) return;
        memset(&spec, 0, sizeof(spec));
        spec.x_q16 = q12_to_q16_long(a);
        spec.y_q16 = q12_to_q16_long(b);
        spec.z_q16 = q12_to_q16_long(c);
        spec.width_q16 = q12_to_q16_long(w);
        spec.height_q16 = q12_to_q16_long(h);
        spec.billboard_mode = blank3d_spriteplanes_mode_from_text(args[9]);
        spec.blend_mode = SP89_BLEND_ALPHA;
        spec.depth_test = 1;
        spec.life_ms = 0UL;
        spec.tint_rgba = 0xFFFFFFFFUL;
        spec.plane_axis = 0;
        for (argi = 10; argi + 1 < argc; argi += 2) {
            if (strcmp(args[argi], "axis") == 0)
                spec.plane_axis = blank3d_spriteplanes_axis_from_text(args[argi + 1]);
            else if (strcmp(args[argi], "life") == 0)
                spec.life_ms = (unsigned long)atoi(args[argi + 1]);
            else if (strcmp(args[argi], "blend") == 0)
                spec.blend_mode = strcmp(args[argi + 1], "additive") == 0
                                ? SP89_BLEND_ADDITIVE : SP89_BLEND_ALPHA;
            else if (strcmp(args[argi], "depth") == 0)
                spec.depth_test = atoi(args[argi + 1]) != 0;
        }
        if (blank3d_spriteplanes_spawn_path(&g.spriteplanes, args[0], &spec) < 0)
            set_status(blank3d_image_assets_error(&g.images));
        return;
    }
    if (strcmp(command, "vehicle_ini") == 0 && argc >= 5 &&
        strcmp(args[1], "pos") == 0 &&
        parse_fixed(args[2], &a) && parse_fixed(args[3], &b) &&
        parse_fixed(args[4], &c)) {
        char vehicle_status[192];
        if (!blank3d_vehicle_system_spawn_ini(&g.vehicles, args[0],
                a, b, c, vehicle_status, sizeof(vehicle_status)))
            set_status(vehicle_status[0] ? vehicle_status
                                        : "Vehicle System: INI spawn failed");
        return;
    }
    if (strcmp(command, "vehicle") == 0 && argc >= 5 &&
        strcmp(args[1], "pos") == 0 &&
        parse_fixed(args[2], &a) && parse_fixed(args[3], &b) &&
        parse_fixed(args[4], &c)) {
        int argi;
        g3d_fix px;
        g3d_fix py;
        g3d_fix pz;
        g3d_fix drive_speed;
        g3d_fix run_speed;
        px = a; py = b; pz = c;
        drive_speed = G3D_FIX_FROM_INT(22);
        run_speed = G3D_FIX_FROM_INT(28);
        for (argi = 5; argi + 1 < argc; argi += 2) {
            if (strcmp(args[argi], "speed") == 0 &&
                parse_fixed(args[argi + 1], &a))
                drive_speed = a;
            else if (strcmp(args[argi], "run") == 0 &&
                     parse_fixed(args[argi + 1], &a))
                run_speed = a;
        }
        if (!blank3d_vehicle_system_spawn_profile(&g.vehicles, args[0],
                px, py, pz, drive_speed, run_speed))
            set_status("gvehicle89: unknown vehicle profile");
        return;
    }
    if (strcmp(command, "mount_car") == 0 && argc >= 4 &&
        strcmp(args[0], "pos") == 0 &&
        parse_fixed(args[1], &a) && parse_fixed(args[2], &b) &&
        parse_fixed(args[3], &c)) {
        if (!blank3d_vehicle_system_spawn_profile(&g.vehicles, "warthog89",
                a, b, c, G3D_FIX_FROM_INT(22), G3D_FIX_FROM_INT(28)))
            set_status("mount_car compatibility spawn failed");
        return;
    }
    if ((strcmp(command, "pickup") == 0 ||
         strcmp(command, "weapon_pickup") == 0 ||
         strcmp(command, "ammo_pickup") == 0) && argc >= 5) {
        int forced_kind;
        unsigned int pickup_slot;
        forced_kind = 0;
        if (strcmp(command, "weapon_pickup") == 0)
            forced_kind = B3D_PICKUP_KIND_WEAPON;
        else if (strcmp(command, "ammo_pickup") == 0)
            forced_kind = B3D_PICKUP_KIND_AMMO;
        if (!blank3d_pickups_spawn_rpyl_args(&g.pickups, args, argc,
                                              forced_kind, &pickup_slot))
            set_status(blank3d_pickups_status(&g.pickups));
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
                &automotion_reached)) {
            if (!blank3d_gloco_feed_automotion_position(&g.gloco,
                    enemy->weapon_actor_id, &enemy->transform.position,
                    &automotion_position, speed, 1))
                enemy->transform.position = automotion_position;
        }
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
                &automotion_reached)) {
            if (!blank3d_gloco_feed_automotion_position(&g.gloco,
                    enemy->weapon_actor_id, &enemy->transform.position,
                    &automotion_position, speed, 1))
                enemy->transform.position = automotion_position;
        }
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
                &automotion_position, &automotion_reached)) {
            if (!blank3d_gloco_feed_automotion_position(&g.gloco,
                    enemy->weapon_actor_id, &enemy->transform.position,
                    &automotion_position, speed, 1))
                enemy->transform.position = automotion_position;
        }
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


static int b3d_object_event_actor_id(unsigned long entity_id)
{
    int index;
    if (entity_id == 1UL) return B3D_PLAYER_ACTOR_ID;
    if (entity_id >= 100UL && entity_id < (unsigned long)(100 + MAX_ENEMIES)) {
        index = (int)(entity_id - 100UL);
        return g.enemies[index].weapon_actor_id;
    }
    return 0;
}

static void *b3d_object_event_subject(unsigned long entity_id)
{
    int index;
    if (entity_id == 1UL) return &g.player;
    if (entity_id >= 100UL && entity_id < (unsigned long)(100 + MAX_ENEMIES)) {
        index = (int)(entity_id - 100UL);
        return &g.enemies[index];
    }
    if (entity_id >= B3D_PICKUP_SUBJECT_BASE &&
        entity_id < B3D_PICKUP_SUBJECT_BASE + B3D_PICKUP_MAX_INSTANCES)
        return blank3d_pickups_find_subject(&g.pickups, entity_id);
    return 0;
}

static int b3d_object_begin_event(void *user, unsigned long entity_id,
                                  unsigned long runtime_key,
                                  unsigned long event_id)
{
    int ok;
    int actor_id;
    (void)user;
    ok = blank3d_variables_begin_event(&g.variables, runtime_key, event_id);
    if (ok && g.condor.initialized) {
        actor_id = b3d_object_event_actor_id(entity_id);
        (void)blank3d_condor_emit(&g.condor, B3D_CEA_EVENT_GFO_BEGIN,
            (int)event_id, actor_id > 0 ? (unsigned long)actor_id : entity_id,
            runtime_key, b3d_object_event_subject(entity_id), 0);
    }
    return ok;
}

static void b3d_object_end_event(void *user, unsigned long entity_id,
                                 unsigned long runtime_key,
                                 unsigned long event_id)
{
    int actor_id;
    void *subject;
    (void)user;
    actor_id = b3d_object_event_actor_id(entity_id);
    subject = b3d_object_event_subject(entity_id);
    if (g.condor.initialized) {
        (void)blank3d_condor_emit(&g.condor, B3D_CEA_EVENT_GFO_END,
            (int)event_id, actor_id > 0 ? (unsigned long)actor_id : entity_id,
            runtime_key, subject, 0);
        if (event_id == B3D_OBJECT_EVENT_DESTROY)
            blank3d_condor_emit_thing(&g.condor, B3D_CEA_EVENT_THING_DESTROY,
                runtime_key, actor_id, entity_id, subject);
    }
    if (!blank3d_variables_end_event(&g.variables))
        set_status(blank3d_variables_status(&g.variables));
    if (event_id == B3D_OBJECT_EVENT_DESTROY)
        (void)blank3d_variables_instance_destroy(&g.variables, runtime_key);
}

static void b3d_object_run_ddsl2(void *user, unsigned long entity_id,
                                 void *native_entity, const char *path)
{
    unsigned long owner;
    (void)user;
    (void)entity_id;
    (void)path;
    owner = b3d_gameverb_owner_for_entity(&g, native_entity);
    if (owner == 0UL) owner = (unsigned long)B3D_PLAYER_ACTOR_ID;
    blank3d_languages_set_subject(owner, native_entity);
    g.ddsl_action_mask = 0UL;
    if (!blank3d_languages_tick_ddsl2())
        set_status(blank3d_languages_status());
    b3d_apply_ddsl_action_mask();
    blank3d_languages_set_subject((unsigned long)B3D_PLAYER_ACTOR_ID,
                                  &g.player);
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
    /* GFO owns render authorization. Actor meshes remain batched by
       draw_scene, but world-item pickups render through their own native
       object so *render -> draw_mesh() is no longer a no-op. */
    (void)user;
    (void)init;
    if (entity_id >= B3D_PICKUP_SUBJECT_BASE &&
        entity_id < B3D_PICKUP_SUBJECT_BASE + B3D_PICKUP_MAX_INSTANCES) {
        Blank3DPickupInstance *pickup;
        pickup = (Blank3DPickupInstance *)native_entity;
        if (pickup && pickup->used) {
            draw_pickup_instance_mesh(pickup);
            pickup->render_submit_frame = (unsigned long)g.frame_stamp;
        }
    }
}

static void b3d_object_draw_script(void *user, unsigned long entity_id,
                                   void *native_entity,
                                   const char *language,
                                   const char *code,
                                   unsigned int code_len)
{
    Blank3DRuntimeInstance *runtime_instance;
    unsigned long owner;
    (void)user;
    (void)native_entity;
    if (!language || !code) return;
    if (strcmp(language, "vars") == 0 ||
        strcmp(language, "var") == 0 ||
        strcmp(language, "var_dsl89") == 0 ||
        strcmp(language, "gmlvars") == 0) {
        runtime_instance = blank3d_runtime_find_subject(&g.runtime, entity_id);
        if (!runtime_instance) {
            set_status("VarDSL could not resolve Thing owner");
            return;
        }
        owner = (unsigned long)runtime_instance->thing + 1UL;
        if (!blank3d_variables_execute_slice(&g.variables, owner,
                                              code, code_len))
            set_status(blank3d_variables_status(&g.variables));
        return;
    }
    /* Other script languages remain provider hooks; none are removed. */
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
    int user_slot;
    const GWP89_UserState *reload_user;
    const GWP89_WeaponProfile *reload_profile;
    int reload_duration;
    int reload_window_start;
    int reload_window_end;
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
        return b3d_camera_is_fps() ? 1L : 0L;
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
    user_slot = gwp89_find_user_slot(weapon_manager, weapon_actor_id);
    if (user_slot >= 0) {
        reload_user = &weapon_manager->users[user_slot];
        reload_profile = gwp89_get_weapon(weapon_manager, reload_user->weapon_slot);
        if (reload_profile) {
            reload_duration = gwp89_resolve_numeric_int(weapon_manager, weapon_actor_id,
                reload_profile, GWP89_NUM_RELOAD_MS, reload_profile->reload_ms);
            reload_window_start = gwp89_resolve_numeric_int(weapon_manager, weapon_actor_id,
                reload_profile, GWP89_NUM_ACTIVE_RELOAD_WINDOW_START_MS,
                reload_profile->active_reload_window_start_ms);
            reload_window_end = gwp89_resolve_numeric_int(weapon_manager, weapon_actor_id,
                reload_profile, GWP89_NUM_ACTIVE_RELOAD_WINDOW_END_MS,
                reload_profile->active_reload_window_end_ms);
            if (b3d_numbar_text_eq(binding, "weapon.reload.active") ||
                b3d_numbar_text_eq(binding, "self.weapon.reload.active"))
                return reload_user->reload_active ? 1L : 0L;
            if (b3d_numbar_text_eq(binding, "weapon.reload.elapsed_ms") ||
                b3d_numbar_text_eq(binding, "self.weapon.reload.elapsed_ms"))
                return (long)reload_user->reload_elapsed_ms;
            if (b3d_numbar_text_eq(binding, "weapon.reload.duration_ms") ||
                b3d_numbar_text_eq(binding, "self.weapon.reload.duration_ms"))
                return (long)(reload_duration > 0 ? reload_duration : 1);
            if (b3d_numbar_text_eq(binding, "weapon.active_reload.window_start_ms") ||
                b3d_numbar_text_eq(binding, "self.weapon.active_reload.window_start_ms"))
                return (long)reload_window_start;
            if (b3d_numbar_text_eq(binding, "weapon.active_reload.window_end_ms") ||
                b3d_numbar_text_eq(binding, "self.weapon.active_reload.window_end_ms"))
                return (long)reload_window_end;
            if (b3d_numbar_text_eq(binding, "weapon.active_reload.result") ||
                b3d_numbar_text_eq(binding, "self.weapon.active_reload.result"))
                return (long)reload_user->active_reload_result;
            if (b3d_numbar_text_eq(binding, "weapon.active_reload.enabled") ||
                b3d_numbar_text_eq(binding, "self.weapon.active_reload.enabled"))
                return reload_profile->active_reload_enabled ? 1L : 0L;
        }
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
                const Blank3DObjectDefinition *definition;
                definition = blank3d_objects_definition(
                    &g.objects, object->definition_slot);
                if (definition)
                    text = gfo_sym_name_by_id(&definition->gfo.sym,
                                              value->v.sym);
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

static int b3d_gfo_argument_int(const gfo_value *value, int *out)
{
    if (!value || !out) return 0;
    if (value->t == GFO_VAL_I32) { *out = (int)value->v.i; return 1; }
    if (value->t == GFO_VAL_BOOL) { *out = value->v.b ? 1 : 0; return 1; }
    return 0;
}

static void b3d_object_invoke(void *user, unsigned long entity_id,
                              void *native_entity, const char *name,
                              const gfo_value *argv, unsigned int argc)
{
    char path[B3D_NUMBAR_PATH_CAP];
    (void)user;
    if (name && strcmp(name, "condor_emit") == 0 && argc >= 2U) {
        char event_name[64];
        int event_type;
        int event_code;
        int actor_id;
        unsigned long runtime_owner;
        if (!b3d_gfo_argument_text(entity_id, &argv[0], event_name,
                                   sizeof(event_name)) ||
            !b3d_condor_parse_event_type_text(event_name, &event_type) ||
            !b3d_gfo_argument_int(&argv[1], &event_code)) {
            set_status("condor_emit requires event-name + integer code");
            return;
        }
        actor_id = b3d_object_event_actor_id(entity_id);
        runtime_owner = 0UL;
        {
            const Blank3DGlocoBinding *binding;
            binding = actor_id > 0 ? blank3d_gloco_binding(&g.gloco, actor_id) : 0;
            if (binding) runtime_owner = binding->thing_owner;
        }
        (void)blank3d_condor_emit(&g.condor, event_type, event_code,
            actor_id > 0 ? (unsigned long)actor_id : entity_id,
            runtime_owner, native_entity, 0);
        return;
    }
    if (name && strcmp(name, "condor_rule") == 0 && argc >= 4U) {
        char event_name[64];
        char condition_name[64];
        char action_name[64];
        int event_type;
        int event_code;
        const char *condition_ptr;
        cea89_rule_id rule;
        if (!b3d_gfo_argument_text(entity_id, &argv[0], event_name,
                                   sizeof(event_name)) ||
            !b3d_condor_parse_event_type_text(event_name, &event_type) ||
            !b3d_gfo_argument_int(&argv[1], &event_code) ||
            !b3d_gfo_argument_text(entity_id, &argv[2], condition_name,
                                   sizeof(condition_name)) ||
            !b3d_gfo_argument_text(entity_id, &argv[3], action_name,
                                   sizeof(action_name))) {
            set_status("condor_rule requires event, code, condition, action");
            return;
        }
        condition_ptr = (strcmp(condition_name, "always") == 0 ||
                         strcmp(condition_name, "-") == 0) ? 0 : condition_name;
        rule = blank3d_condor_rule_gameverbs(&g.condor,
                    event_type, event_code, condition_ptr, action_name);
        set_status(rule ? "Condor GFO rule registered" : blank3d_condor_status(&g.condor));
        return;
    }
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
    if (name && strcmp(name, "reload_logic") == 0) {
        set_status("GFO requested logic reload");
        return;
    }
    if (name && g.kinverbs.initialized) {
        gverb89_call call;
        int result;
        memset(&call, 0, sizeof(call));
        call.owner = b3d_gameverb_owner_for_entity(&g, native_entity);
        if (call.owner == 0UL) call.owner = entity_id;
        call.subject = native_entity;
        call.name = name;
        result = gverb89_perform(blank3d_kinverbs_registry(&g.kinverbs),
                                 &call);
        if (result == GVERB89_HANDLED) return;
    }
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

static int b3d_runtime_query_pose(void *user, unsigned long subject_id,
                                  void *native_object,
                                  Blank3DRuntimePose *out_pose)
{
    Transform *transform;
    Enemy *enemy;
    (void)user;
    if (!native_object || !out_pose) return 0;
    memset(out_pose, 0, sizeof(*out_pose));
    if (subject_id == 1UL) {
        transform = (Transform *)native_object;
        out_pose->half_x = 26214L;
        out_pose->half_y = 58982L;
        out_pose->half_z = 26214L;
    } else if (subject_id >= B3D_PICKUP_SUBJECT_BASE &&
               subject_id < B3D_PICKUP_SUBJECT_BASE +
                            B3D_PICKUP_MAX_INSTANCES) {
        Blank3DPickupInstance *pickup;
        pickup = (Blank3DPickupInstance *)native_object;
        transform = &pickup->transform;
        out_pose->half_x = pickup->radius_q16 / 2L;
        out_pose->half_y = pickup->radius_q16 / 2L;
        out_pose->half_z = pickup->radius_q16 / 2L;
    } else {
        enemy = (Enemy *)native_object;
        transform = &enemy->transform;
        out_pose->half_x = 32768L;
        out_pose->half_y = 65536L;
        out_pose->half_z = 32768L;
    }
    out_pose->px = q12_to_q16_long(transform->position.x);
    out_pose->py = q12_to_q16_long(transform->position.y);
    out_pose->pz = q12_to_q16_long(transform->position.z);
    out_pose->rx = q12_to_q16_long(transform->rotation.x);
    out_pose->ry = q12_to_q16_long(transform->rotation.y);
    out_pose->rz = q12_to_q16_long(transform->rotation.z);
    return 1;
}

static int blank3d_init_runtime_spine(void)
{
    Blank3DRuntimeProvider provider;
    memset(&provider, 0, sizeof(provider));
    provider.user = &g;
    provider.query_pose = b3d_runtime_query_pose;
    return blank3d_runtime_spine_init(&g.runtime, &provider);
}

static int blank3d_init_object_runtime(void)
{
    Blank3DObjectHost host;
    memset(&host, 0, sizeof(host));
    host.user = &g;
    host.begin_event = b3d_object_begin_event;
    host.end_event = b3d_object_end_event;
    host.run_ddsl2 = b3d_object_run_ddsl2;
    host.run_fpil = b3d_object_run_fpil;
    host.run_rpyl = b3d_object_run_rpyl;
    host.draw_mesh = b3d_object_draw_mesh;
    host.draw_script = b3d_object_draw_script;
    host.invoke = b3d_object_invoke;
    host.handle = b3d_object_handle;
    blank3d_numbar_init(&g.numbars, b3d_numbar_resolve, &g);
    if (!blank3d_classes_init(&g.classes, 0)) return 0;
    blank3d_objects_init(&g.objects, &host, &g.classes);
    return 1;
}

static void blank3d_rebuild_gfo_entities(void)
{
    unsigned int slot;
    int i;
    char ini_path[128];
    const char *archetype;
    TS89_Thing thing;
    Blank3DRuntimeInstance *runtime_instance;
    unsigned long runtime_key;

    blank3d_objects_clear_instances(&g.objects);
    blank3d_variables_reset_instances(&g.variables);
    if (g.gloco.initialized) blank3d_gloco_reset_bindings(&g.gloco);
    (void)blank3d_classes_unseal(&g.classes);
    blank3d_runtime_spine_reset(&g.runtime);

    if (blank3d_runtime_instance_create(&g.runtime, 1UL,
            (unsigned long)sm3d_hash_cstr("config/entities/player.ini"),
            &g.player, &thing)) {
        runtime_key = (unsigned long)thing + 1UL;
        (void)blank3d_variables_instance_create(&g.variables, runtime_key);
        blank3d_variables_set_player_owner(&g.variables, runtime_key);
        (void)blank3d_runtime_bind_actor(&g.runtime, thing,
                                         B3D_PLAYER_ACTOR_ID);
        runtime_instance = blank3d_runtime_find_thing(&g.runtime, thing);
        if (runtime_instance)
            (void)as89_set_owner_entity(&g.actors, B3D_PLAYER_ACTOR_ID,
                blank3d_runtime_entity_key(runtime_instance->entity));
        if (g.gloco.initialized)
            (void)blank3d_gloco_bind(&g.gloco, B3D_PLAYER_ACTOR_ID, runtime_key,
                &g.player, &g.player_vertical, GLOCO_PROFILE_DEFAULT,
                "config/locomotion/player.ini");
        if (blank3d_objects_spawn_ex(&g.objects,
            "config/entities/player.ini", 1UL, runtime_key,
            &g.player, &slot) && g.condor.initialized)
            blank3d_condor_emit_thing(&g.condor, B3D_CEA_EVENT_THING_SPAWN,
                runtime_key, B3D_PLAYER_ACTOR_ID, 1UL, &g.player);
    }

    for (i = 0; i < g.enemy_count; ++i) {
        if (!g.enemies[i].alive) continue;
        archetype = g.enemies[i].archetype[0] != '\0'
                  ? g.enemies[i].archetype : "zombie";
        sprintf(ini_path, "config/entities/%s.ini", archetype);
        if (!blank3d_runtime_instance_create(&g.runtime,
                (unsigned long)(100 + i),
                (unsigned long)sm3d_hash_cstr(ini_path),
                &g.enemies[i], &thing))
            continue;
        runtime_key = (unsigned long)thing + 1UL;
        (void)blank3d_variables_instance_create(&g.variables, runtime_key);
        (void)blank3d_runtime_bind_actor(&g.runtime, thing,
                                         g.enemies[i].weapon_actor_id);
        runtime_instance = blank3d_runtime_find_thing(&g.runtime, thing);
        if (runtime_instance)
            (void)as89_set_owner_entity(&g.actors,
                g.enemies[i].weapon_actor_id,
                blank3d_runtime_entity_key(runtime_instance->entity));
        if (g.gloco.initialized)
            (void)blank3d_gloco_bind(&g.gloco, g.enemies[i].weapon_actor_id,
                runtime_key, &g.enemies[i].transform, &g.enemies[i].vertical,
                GLOCO_PROFILE_TACTICAL, "config/locomotion/enemy.ini");
        if (blank3d_objects_spawn_ex(&g.objects, ini_path,
            (unsigned long)(100 + i), runtime_key,
            &g.enemies[i], &slot) && g.condor.initialized)
            blank3d_condor_emit_thing(&g.condor, B3D_CEA_EVENT_THING_SPAWN,
                runtime_key, g.enemies[i].weapon_actor_id,
                (unsigned long)(100 + i), &g.enemies[i]);
    }
    for (i = 0; i < B3D_PICKUP_MAX_INSTANCES; ++i) {
        Blank3DPickupInstance *pickup;
        pickup = blank3d_pickups_get(&g.pickups, (unsigned int)i);
        if (!pickup || !pickup->used ||
            !blank3d_pickups_is_world_active(&g.pickups, (unsigned int)i))
            continue;
        if (!blank3d_runtime_instance_create(&g.runtime, pickup->subject_id,
                (unsigned long)sm3d_hash_cstr(pickup->config_path),
                pickup, &thing))
            continue;
        runtime_key = (unsigned long)thing + 1UL;
        (void)blank3d_variables_instance_create(&g.variables, runtime_key);
        if (blank3d_objects_spawn_ex(&g.objects, pickup->config_path,
                pickup->subject_id, runtime_key, pickup, &slot)) {
            pickup->gfo_live = 1;
            if (g.condor.initialized)
                blank3d_condor_emit_thing(&g.condor,
                    B3D_CEA_EVENT_THING_SPAWN, runtime_key, 0,
                    pickup->subject_id, pickup);
        }
    }
    (void)blank3d_runtime_sync(&g.runtime);
    (void)blank3d_classes_seal(&g.classes);
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
    host.gameverbs = blank3d_kinverbs_registry(&g.kinverbs);
    host.gameverb_owner = b3d_gameverb_owner_for_entity;
    blank3d_languages_init(&host);
    blank3d_languages_set_subject((unsigned long)B3D_PLAYER_ACTOR_ID,
                                  &g.player);
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
    blank3d_spriteplanes_clear(&g.spriteplanes);
    default_world();
    blank3d_pickups_reset(&g.pickups);
    b3d_actor_system_reset();
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
        (void)as89_register(&g.actors, g.enemies[0].weapon_actor_id,
            0UL, 1UL, 1, 1, 0UL);
    }
    g.camera_yaw = g.player.rotation.y;
    g.base_fov = g.fov;
    configure_sockets();
    b3d_perception_reset_agents();
    b3d_faction_update_targets();
    blank3d_rebuild_gfo_entities();
    camera_init_perspective(&g.camera, g.fov,
                            b3d_camera_draw_aspect(),
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

    if (b3d_camera_is_fps()) {
        gamlib_vec3_scale(&offset, &world_up, g.camera_eye_height);
        gamlib_vec3_add(&eye, &eye, &offset);
        gamlib_vec3_scale(&offset, &forward, fix_ratio(3, 20));
        gamlib_vec3_add(&eye, &eye, &offset);
    } else {
        gamlib_vec3_scale(&offset, &world_up, g.camera_height);
        gamlib_vec3_add(&eye, &eye, &offset);
        gamlib_vec3_scale(&offset, &forward, g3d_fix_neg_sat(g.camera_dist));
        gamlib_vec3_add(&eye, &eye, &offset);
        if (b3d_camera_is_ots()) {
            gamlib_vec3_scale(&offset, &right, g.camera_shoulder);
            gamlib_vec3_add(&eye, &eye, &offset);
        }
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
    if (!b3d_camera_is_fps() &&
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

static void capture_player_weapon_snapshot(void)
{
    GWP89_Vec3 socket_origin;
    GWP89_Vec3 socket_forward;
    GWP89_Vec3 socket_right;
    GWP89_Vec3 socket_up;
    GWP89_Vec3 camera_origin;
    GWP89_Vec3 camera_forward;
    Vec3 eye;
    Vec3 forward;
    Vec3 right;
    Vec3 up;
    gws89_snapshot snapshot;
    build_weapon_input_vectors(&socket_origin, &socket_forward,
                               &socket_right, &socket_up,
                               &camera_origin, &camera_forward);
    build_view_vectors(&eye, &forward, &right, &up);
    memset(&snapshot, 0, sizeof(snapshot));
    snapshot.valid = 1;
    snapshot.actor_id = B3D_PLAYER_ACTOR_ID;
    snapshot.frame_id = (unsigned long)g.frame_stamp;
    snapshot.weapon_origin.x = socket_origin.x;
    snapshot.weapon_origin.y = socket_origin.y;
    snapshot.weapon_origin.z = socket_origin.z;
    snapshot.muzzle_origin = snapshot.weapon_origin;
    snapshot.muzzle_forward.x = socket_forward.x;
    snapshot.muzzle_forward.y = socket_forward.y;
    snapshot.muzzle_forward.z = socket_forward.z;
    snapshot.muzzle_right.x = socket_right.x;
    snapshot.muzzle_right.y = socket_right.y;
    snapshot.muzzle_right.z = socket_right.z;
    snapshot.muzzle_up.x = socket_up.x;
    snapshot.muzzle_up.y = socket_up.y;
    snapshot.muzzle_up.z = socket_up.z;
    snapshot.camera_origin.x = camera_origin.x;
    snapshot.camera_origin.y = camera_origin.y;
    snapshot.camera_origin.z = camera_origin.z;
    snapshot.camera_forward.x = camera_forward.x;
    snapshot.camera_forward.y = camera_forward.y;
    snapshot.camera_forward.z = camera_forward.z;
    snapshot.camera_right.x = right.x;
    snapshot.camera_right.y = right.y;
    snapshot.camera_right.z = right.z;
    snapshot.camera_up.x = up.x;
    snapshot.camera_up.y = up.y;
    snapshot.camera_up.z = up.z;
    (void)gweaponsnapshot89_publish(&g.weapon_snapshots, &snapshot);
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
                          movement_pct, stress_pct, dt_frames,
                          (unsigned short)(g.frame_ms > 1000U
                              ? 1000U : (g.frame_ms ? g.frame_ms : 1U)),
                          &scope_camera);

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

static void b3d_morethanone_apply_event(GWP89_Event *event)
{
    const mto89_stage *stage;
    unsigned long mask;
    if (!event || !g.morethanone_pending ||
        event->weapon_id != g.morethanone_weapon_id) return;
    stage = &g.morethanone_result.stage;
    mask = stage->override_mask;
    if (mask & MTO89_OVERRIDE_PROJECTILE_ID)
        event->projectile_id = stage->projectile_id;
    if (mask & MTO89_OVERRIDE_PROJECTILE_MESH_ID)
        event->projectile_mesh_id = stage->projectile_mesh_id;
    if (mask & MTO89_OVERRIDE_DAMAGE_Q16)
        event->damage_fx = (gwp89_fx)q16_to_q12(stage->damage_q16);
    if (mask & MTO89_OVERRIDE_SPEED_Q16)
        event->speed_fx = (gwp89_fx)q16_to_q12(stage->speed_q16);
    if (mask & MTO89_OVERRIDE_RADIUS_Q16)
        event->radius_fx = (gwp89_fx)q16_to_q12(stage->radius_q16);
    if (mask & MTO89_OVERRIDE_MESH_SCALE_Q16)
        event->projectile_mesh_scale_fx =
            (gwp89_fx)q16_to_q12(stage->mesh_scale_q16);
    if (mask & MTO89_OVERRIDE_LIFE_MS)
        event->life_ms = stage->life_ms;
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
        b3d_morethanone_apply_event(&event);
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
                if (blank3d_sniper_is_scoped(&g.sniper))
                    (void)blank3d_sniper_trigger_event(&g.sniper, "fire");
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
            if (event.actor_id == B3D_PLAYER_ACTOR_ID) {
                sprintf(title, "reload: %s | clip %d reserve %d",
                        event.weapon_name, event.clip_ammo, event.reserve_ammo);
                set_status(title);
            }
        } else if (event.type == GWP89_EVENT_RELOAD_END) {
            blank3d_audio_reload_end_sync(
                &g.audio, event.weapon_id,
                event.clip_ammo, current_clip_capacity());
            if (event.actor_id == B3D_PLAYER_ACTOR_ID) {
                sprintf(title, "reload complete: %s | clip %d reserve %d",
                        event.weapon_name, event.clip_ammo, event.reserve_ammo);
                set_status(title);
                sprintf(title, "Blank3D | %s | clip %d reserve %d | %s",
                        event.weapon_name, event.clip_ammo, event.reserve_ammo,
                        b3d_camera_mode_name(g.camera_mode));
                SetWindowTextA(g.window, title);
            }
        } else if (event.type == GWP89_EVENT_MUZZLE_REQUEST) {
            const Blank3DWeaponModules *muzzle_modules;
            muzzle_modules = blank3d_weapon_modules_get(event.weapon_id);
            /* MUZZLE_REQUEST is world presentation for every actor.  It must
               never mutate the local-player HUD/crosshair firing state.
               FIRE_ACCEPTED above is the sole owner of g.muzzle_flash_ms and
               already gates that state by B3D_PLAYER_ACTOR_ID. */
            if (muzzle_modules) {
                (void)blank3d_muzzle_image_emit(&g.spriteplanes,
                                                 muzzle_modules, &event);
                (void)blank3d_muzzle_light_emit(&g.muzzle_light,
                                                 muzzle_modules, &event);
            }
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
                    b3d_camera_mode_name(g.camera_mode));
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
            gtrigger89_reset(&g.trigger_state);
            g.morethanone_pending = 0;
            g.morethanone_weapon_id = 0;
            if (event.weapon_id != GATLING_WEAPON_ID)
                blank3d_audio_gatling_release(&g.audio);
            sprintf(title, "Blank3D | %s | clip %d reserve %d | %s",
                    event.weapon_name,
                    event.clip_ammo,
                    event.reserve_ammo,
                    b3d_camera_mode_name(g.camera_mode));
            SetWindowTextA(g.window, title);
        }
    }
    g.morethanone_pending = 0;
    g.morethanone_weapon_id = 0;

    /* A shotgun discharge is one ammo transaction but seven independent
       pellet requests. For the player each request becomes its own camera ray;
       NPCs keep their physical projectile path. Recover only missing indices. */
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
        set_status("shotgun runtime: seven player camera rays preserved");
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
        set_status("gatling player ray: recovered dropped visual request");
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
    gtrigger89_config trigger_config;
    gtrigger89_output trigger_output;
    gws89_snapshot snapshot;
    int view_style;
    int weapon_id;
    long charge_speed_q16;
    long charge_ratio_q16;
    unsigned int module_charge_ms;
    char charge_status[128];
    const Blank3DWeaponModules *modules;

    weapon_id = blank3d_systems_weapon_id(&g.systems);
    modules = blank3d_weapon_modules_get(weapon_id);
    trigger_config.model = GTRIGGER89_MODEL_NORMAL;
    trigger_config.spinup_ms = g.trigger_spinup_ms;
    trigger_config.charge_max_ms = g.trigger_charge_max_ms;
    if (modules && modules->trigger_model == B3D_TRIGGER_SPINUP)
        trigger_config.model = GTRIGGER89_MODEL_SPINUP;
    else if (modules && modules->trigger_model == B3D_TRIGGER_CHARGE_RELEASE)
        trigger_config.model = GTRIGGER89_MODEL_CHARGE_RELEASE;
    else if (modules && modules->trigger_model ==
             B3D_TRIGGER_PRESS_CHARGE_RELEASE) {
        trigger_config.model = GTRIGGER89_MODEL_PRESS_CHARGE_RELEASE;
        if (modules->charge_time_ms > 0U)
            trigger_config.charge_max_ms = modules->charge_time_ms;
    }

    /* DDSL2 owns the logical verb `shoot`; gtrigger89 owns every temporal
       trigger policy after that point. No physical key or weapon-specific
       spin/charge state lives in the runner anymore. */
    gtrigger89_update(&g.trigger_state, &trigger_config,
                      g.fire_requested ? 1 : 0,
                      (unsigned int)g.frame_ms,
                      &trigger_output);
    if (trigger_output.spin_begin)
        blank3d_audio_gatling_begin(&g.audio);
    if (trigger_output.spin_end)
        blank3d_audio_gatling_release(&g.audio);
    if (trigger_output.charge_begin)
        set_status("charge trigger: accumulating provider time");
    if (trigger_output.charge_release && modules) {
        if (trigger_config.model == GTRIGGER89_MODEL_PRESS_CHARGE_RELEASE &&
            modules->more_than_one.enabled) {
            if (mto89_resolve(&modules->more_than_one,
                              trigger_output.charge_elapsed_ms,
                              &g.morethanone_result)) {
                g.morethanone_pending = 1;
                g.morethanone_weapon_id = weapon_id;
                /* The ordinary PRESS already fired the lemon. Only a valid
                   morethanone89 tier authorizes a second semi-auto shot. */
                trigger_output.trigger_down = 1;
                trigger_output.trigger_pressed = 1;
                sprintf(charge_status,
                        "morethanone89: tier %d release projectile %d",
                        g.morethanone_result.stage_index + 1,
                        g.morethanone_result.stage.projectile_id);
                set_status(charge_status);
            } else {
                trigger_output.trigger_down = 0;
                trigger_output.trigger_pressed = 0;
            }
        } else {
            module_charge_ms = trigger_output.charge_elapsed_ms;
            if (modules->charge_time_ms > 0U &&
                g.trigger_charge_max_ms > 0U) {
                module_charge_ms = (unsigned int)(
                    ((unsigned long)trigger_output.charge_elapsed_ms *
                     (unsigned long)modules->charge_time_ms) /
                    (unsigned long)g.trigger_charge_max_ms);
            }
            charge_speed_q16 = blank3d_weapon_modules_charge_speed_q16(
                weapon_id, module_charge_ms);
            charge_ratio_q16 = blank3d_weapon_modules_charge_ratio_q16(
                weapon_id, module_charge_ms);
            blank3d_systems_set_launch_speed_q16(&g.systems, charge_speed_q16);
            sprintf(charge_status,
                    "charge release: charge %ld%% speed %ld",
                    (charge_ratio_q16 * 100L) / 65536L,
                    charge_speed_q16 / 65536L);
            set_status(charge_status);
        }
    }

    /* Prefer the same-frame weapon snapshot published after locomotion and
       socket republishing. Fallback keeps hosts that have not installed the
       snapshot provider source-compatible. */
    if (gweaponsnapshot89_get(&g.weapon_snapshots,
                              B3D_PLAYER_ACTOR_ID,
                              (unsigned long)g.frame_stamp,
                              &snapshot)) {
        socket_origin.x = (gwp89_fx)snapshot.muzzle_origin.x;
        socket_origin.y = (gwp89_fx)snapshot.muzzle_origin.y;
        socket_origin.z = (gwp89_fx)snapshot.muzzle_origin.z;
        socket_forward.x = (gwp89_fx)snapshot.muzzle_forward.x;
        socket_forward.y = (gwp89_fx)snapshot.muzzle_forward.y;
        socket_forward.z = (gwp89_fx)snapshot.muzzle_forward.z;
        socket_right.x = (gwp89_fx)snapshot.muzzle_right.x;
        socket_right.y = (gwp89_fx)snapshot.muzzle_right.y;
        socket_right.z = (gwp89_fx)snapshot.muzzle_right.z;
        socket_up.x = (gwp89_fx)snapshot.muzzle_up.x;
        socket_up.y = (gwp89_fx)snapshot.muzzle_up.y;
        socket_up.z = (gwp89_fx)snapshot.muzzle_up.z;
        camera_origin.x = (gwp89_fx)snapshot.camera_origin.x;
        camera_origin.y = (gwp89_fx)snapshot.camera_origin.y;
        camera_origin.z = (gwp89_fx)snapshot.camera_origin.z;
        camera_forward.x = (gwp89_fx)snapshot.camera_forward.x;
        camera_forward.y = (gwp89_fx)snapshot.camera_forward.y;
        camera_forward.z = (gwp89_fx)snapshot.camera_forward.z;
    } else {
        build_weapon_input_vectors(&socket_origin, &socket_forward,
                                   &socket_right, &socket_up,
                                   &camera_origin, &camera_forward);
    }
    view_style = b3d_camera_view_style();
    blank3d_systems_update(&g.systems, g.frame_ms,
                           trigger_output.trigger_down,
                           trigger_output.trigger_pressed,
                           trigger_output.trigger_released,
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


static void sync_kinverb_world(void)
{
    int i;
    g3d_fix half_width;
    g3d_fix height;
    if (!g.kinverbs.initialized) return;
    blank3d_kinverbs_begin_sync(&g.kinverbs);

    /* Mirrors the two current world colliders. 3DKin is query authority only;
       Collision/VPhysics remain final movement authority. */
    (void)blank3d_kinverbs_add_world_box(&g.kinverbs,
        G3D_FIX_FROM_INT(-40), G3D_FIX_FROM_INT(-2), G3D_FIX_FROM_INT(-20),
        G3D_FIX_FROM_INT(80), G3D_FIX_FROM_INT(2), G3D_FIX_FROM_INT(80),
        GK3D_FLAG_SOLID);
    (void)blank3d_kinverbs_add_world_box(&g.kinverbs,
        G3D_FIX_FROM_INT(-40), 0, G3D_FIX_FROM_INT(59),
        G3D_FIX_FROM_INT(80), G3D_FIX_FROM_INT(10), G3D_FIX_FROM_INT(2),
        GK3D_FLAG_SOLID);

    half_width = (g3d_fix)(13L * G3D_FIX_ONE / 20L);
    height = (g3d_fix)(9L * G3D_FIX_ONE / 5L);
    (void)blank3d_kinverbs_sync_actor(&g.kinverbs,
        (unsigned long)B3D_PLAYER_ACTOR_ID, B3D_PLAYER_ACTOR_ID,
        &g.player, half_width, height, half_width, 1);
    for (i = 0; i < g.enemy_count; ++i) {
        if (!g.enemies[i].alive) continue;
        (void)blank3d_kinverbs_sync_actor(&g.kinverbs,
            (unsigned long)g.enemies[i].weapon_actor_id,
            g.enemies[i].weapon_actor_id, &g.enemies[i].transform,
            half_width, height, half_width, 1);
    }
    blank3d_kinverbs_end_sync(&g.kinverbs);
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
    sync_kinverb_world();
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
    radius = (weapon_id == 7 ||
              weapon_id == B3D_WEAPON_ID_HOMING_ROCKET_LAUNCHER)
             ? G3D_FIX_FROM_INT(8) :
             (weapon_id == B3D_WEAPON_ID_HAND_GRENADE
              ? G3D_FIX_FROM_INT(6) : G3D_FIX_FROM_INT(5));
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
    if ((bullet->weapon_id == 6 || bullet->weapon_id == 7 ||
         bullet->weapon_id == B3D_WEAPON_ID_HOMING_ROCKET_LAUNCHER ||
         bullet->weapon_id == B3D_WEAPON_ID_HAND_GRENADE)) {
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

static int b3d_update_expandible_fire(Bullet *bullet)
{
    const Blank3DWeaponModules *modules;
    ef89_config config;
    ef89_result result;
    ef89_vec3 position;
    int bullet_index;

    if (!bullet || !bullet->alive) return 0;
    if (!bullet->expandible_fire_active) return 1;
    modules = blank3d_weapon_modules_get(bullet->weapon_id);
    if (!modules || !modules->expandible_fire_enabled) {
        bullet->expandible_fire_active = 0;
        return 1;
    }

    expandiblefire89_config_default(&config);
    config.scale_start_fx =
        (ef89_fx)(modules->expandible_fire_scale_start_q16 / 16L);
    config.scale_end_fx =
        (ef89_fx)(modules->expandible_fire_scale_end_q16 / 16L);
    config.growth_distance_fx =
        (ef89_fx)(modules->expandible_fire_growth_distance_q16 / 16L);
    config.kill_distance_fx =
        (ef89_fx)(modules->expandible_fire_kill_distance_q16 / 16L);
    position.x = (ef89_fx)bullet->transform.position.x;
    position.y = (ef89_fx)bullet->transform.position.y;
    position.z = (ef89_fx)bullet->transform.position.z;
    if (!expandiblefire89_step(&bullet->expandible_fire_state,
                               &config, position, &result))
        return 1;

    bullet->mesh_scale = (g3d_fix)expandiblefire89_apply_scale(
        (ef89_fx)bullet->base_mesh_scale, result.scale_fx);
    if (modules->expandible_fire_collision_growth)
        bullet->radius = (g3d_fix)expandiblefire89_apply_scale(
            (ef89_fx)bullet->base_radius, result.scale_fx);
    else
        bullet->radius = bullet->base_radius;

    if (!result.alive) {
        bullet_index = (int)(bullet - g.bullets);
        release_bullet_slot(bullet_index);
        return 0;
    }
    return 1;
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
    const Blank3DWeaponModules *projectile_modules;
    Bullet *event_bullet;
    ts89_request seek_request;
    ts89_result seek_result;

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
        if (g.bullets[i].cosmetic_only) {
            g3d_fix step_length;
            g.bullets[i].previous_position =
                g.bullets[i].transform.position;
            if (!g.bullets[i].cosmetic_arrived) {
                gamlib_vec3_scale(&movement,
                    &g.bullets[i].velocity, g.dt);
                step_length = gamlib_vec3_length(&movement);
                if (step_length + G3D_FIX_EPSILON >=
                    g.bullets[i].cosmetic_remaining) {
                    g.bullets[i].transform.position =
                        g.bullets[i].cosmetic_target;
                    g.bullets[i].cosmetic_remaining = 0;
                    g.bullets[i].cosmetic_arrived = 1;
                    g.bullets[i].velocity = gamlib_vec3(0, 0, 0);
                } else {
                    gamlib_vec3_add(&g.bullets[i].transform.position,
                        &g.bullets[i].transform.position, &movement);
                    g.bullets[i].cosmetic_remaining =
                        g3d_fix_sub_sat(
                            g.bullets[i].cosmetic_remaining, step_length);
                }
                blank3d_trails_emit_q12(&g.trails, i,
                    g.bullets[i].transform.position.x,
                    g.bullets[i].transform.position.y,
                    g.bullets[i].transform.position.z);
            }
            g.bullets[i].life =
                g3d_fix_sub_sat(g.bullets[i].life, g.dt);
            if (g.bullets[i].life <= 0)
                release_bullet_slot(i);
            else
                publish_bullet(i);
            continue;
        }
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

        projectile_modules =
            blank3d_weapon_modules_get(g.bullets[i].weapon_id);
        if (projectile_modules && projectile_modules->telesearcher_enabled) {
            memset(&seek_request, 0, sizeof(seek_request));
            seek_request.owner_actor_id = g.bullets[i].owner_actor_id;
            seek_request.target_actor_id = g.bullets[i].target_actor_id;
            seek_request.position.x = (ts89_fx)g.bullets[i].transform.position.x;
            seek_request.position.y = (ts89_fx)g.bullets[i].transform.position.y;
            seek_request.position.z = (ts89_fx)g.bullets[i].transform.position.z;
            seek_request.velocity.x = (ts89_fx)g.bullets[i].velocity.x;
            seek_request.velocity.y = (ts89_fx)g.bullets[i].velocity.y;
            seek_request.velocity.z = (ts89_fx)g.bullets[i].velocity.z;
            seek_request.speed_fx =
                (ts89_fx)gamlib_vec3_length(&g.bullets[i].velocity);
            seek_request.gain_fx =
                (ts89_fx)(projectile_modules->telesearcher_gain_q16 / 16L);
            seek_request.target_offset.x =
                (ts89_fx)(projectile_modules->telesearcher_offset_x_q16 / 16L);
            seek_request.target_offset.y =
                (ts89_fx)(projectile_modules->telesearcher_offset_y_q16 / 16L);
            seek_request.target_offset.z =
                (ts89_fx)(projectile_modules->telesearcher_offset_z_q16 / 16L);
            if (projectile_modules->telesearcher_reacquire)
                seek_request.flags |= TS89_FLAG_REACQUIRE;
            if (telesearcher89_step(&seek_request,
                    b3d_telesearcher_target_provider, 0, &seek_result) &&
                seek_result.target_found) {
                g.bullets[i].target_actor_id = seek_result.target_actor_id;
                g.bullets[i].velocity.x = (g3d_fix)seek_result.velocity.x;
                g.bullets[i].velocity.y = (g3d_fix)seek_result.velocity.y;
                g.bullets[i].velocity.z = (g3d_fix)seek_result.velocity.z;
            }
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
                    g.bullets[i].weapon_id == 7 ||
                    g.bullets[i].weapon_id == B3D_WEAPON_ID_HOMING_ROCKET_LAUNCHER ||
                    g.bullets[i].weapon_id == B3D_WEAPON_ID_HAND_GRENADE) {
                    apply_explosion_damage(&g.bullets[i].transform.position,
                        g.bullets[i].weapon_id, g.bullets[i].damage, -1,
                        g.bullets[i].owner_actor_id);
                }
                release_bullet_slot(i);
            }
            }
        }
        if (g.bullets[i].alive &&
            !b3d_update_expandible_fire(&g.bullets[i]))
            continue;
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
    if (!blank3d_vehicle_system_playerdriving(&g.vehicles))
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

    /* Snapshot is published only after final locomotion + attachment sync, so
       every fire consumer sees the same frame's player/weapon/muzzle basis. */
    capture_player_weapon_snapshot();
}

static void b3d_katana_debug_line(void *user, hb3_v3 a, hb3_v3 b,
                                      int color, int tag)
{
    Vec3 from;
    Vec3 to;
    unsigned char r;
    unsigned char green;
    unsigned char blue;
    (void)user;
    (void)tag;
    from = gamlib_vec3((g3d_fix)(a.x * 256),
                       (g3d_fix)(a.y * 256),
                       (g3d_fix)(a.z * 256));
    to = gamlib_vec3((g3d_fix)(b.x * 256),
                     (g3d_fix)(b.y * 256),
                     (g3d_fix)(b.z * 256));
    if (color == HB3_DEBUG_COLOR_HURT) {
        r = 80U; green = 255U; blue = 120U;
    } else {
        r = 255U; green = 72U; blue = 72U;
    }
    bridge_gl_draw_segment(&from, &to, r, green, blue);
}

static void sync_katana_melee(void)
{
    soq3d_pose hand_pose;
    Blank3DKatanaHit hit;
    int i;
    if (!g.katana_melee.initialized) return;

    if (soq3d_get_socket(&g.locator, g.player_melee_socket,
                         g.frame_stamp, &hand_pose) == SOQ3D_OK)
        blank3d_katana_melee_set_root_pose(&g.katana_melee, &hand_pose);

    blank3d_katana_melee_clear_targets(&g.katana_melee);
    for (i = 0; i < g.enemy_count; ++i) {
        if (g.enemies[i].faction_is_ally) continue;
        (void)blank3d_katana_melee_set_target_q12(&g.katana_melee,
            g.enemies[i].weapon_actor_id, 2, g.enemies[i].alive,
            g.enemies[i].transform.position.x,
            g.enemies[i].transform.position.y,
            g.enemies[i].transform.position.z,
            fix_ratio(19, 10), fix_ratio(13, 20));
    }

    if (g.key_pressed['K']) {
        if (blank3d_katana_melee_trigger(&g.katana_melee))
            set_status("katana Mecanim slash: physical damage window armed");
    }
    if (g.key_pressed[VK_F3]) {
        g.katana_melee.config.debug_draw =
            !g.katana_melee.config.debug_draw;
        set_status(g.katana_melee.config.debug_draw
                 ? "katana PDC3D hit/hurt volumes visible"
                 : "katana PDC3D hit/hurt volumes hidden");
    }

    (void)blank3d_katana_melee_update(&g.katana_melee, g.frame_ms);
    while (blank3d_katana_melee_poll_hit(&g.katana_melee, &hit)) {
        b3d_damage_actor(B3D_PLAYER_ACTOR_ID, hit.defender_id, hit.damage);
        set_status("katana red OBB hit enemy green box hurtbox");
    }
}

static void draw_katana_melee(void)
{
    const nm89_geometry_packet *packet;
    packet = blank3d_katana_melee_packet(&g.katana_melee);
    if (!packet || !packet->visible) return;
    glPushMatrix();
    bridge_gl_apply_q16_matrix(packet->world.m);
    bridge_gl_draw_mesh(&g.meshes.katana);
    glPopMatrix();
}

static void b3d_sync_pickup_lifecycle(void)
{
    int i;
    unsigned int slot;
    const Blank3DObjectEntity *object_entity;
    Blank3DRuntimeInstance *runtime_instance;
    Blank3DPickupInstance *pickup;
    for (i = 0; i < B3D_PICKUP_MAX_INSTANCES; ++i) {
        pickup = blank3d_pickups_get(&g.pickups, (unsigned int)i);
        if (!pickup || !pickup->used || !pickup->gfo_live) continue;
        if (blank3d_pickups_is_world_active(&g.pickups, (unsigned int)i))
            continue;
        for (slot = 0U; slot < g.objects.count; ++slot) {
            object_entity = blank3d_objects_get(&g.objects, slot);
            if (object_entity && object_entity->alive &&
                object_entity->entity_id == pickup->subject_id) {
                blank3d_objects_kill(&g.objects, slot);
                break;
            }
        }
        runtime_instance = blank3d_runtime_find_subject(&g.runtime,
                                                        pickup->subject_id);
        if (runtime_instance)
            (void)blank3d_runtime_instance_destroy(&g.runtime,
                                                    runtime_instance->thing);
        pickup->gfo_live = 0;
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
    if (g.condor.initialized)
        blank3d_condor_emit_input(&g.condor, &g.input,
            (unsigned long)B3D_PLAYER_ACTOR_ID, g.variables.player_owner,
            &g.player);
    sync_collision_world();
    b3d_faction_update_targets();
    b3d_perception_tick();
    blank3d_gloco_begin_frame(&g.gloco, g.frame_ms);
    blank3d_objects_tick(&g.objects);
    update_motion_attacks();
    update_vertical_axis();
    for (i = 0; i < g.enemy_count; ++i) {
        if (!g.enemies[i].alive) continue;
        if (blank3d_motion_attack_mode(&g.enemies[i].motion_attack) !=
            B3D_MOTION_ATTACK_NONE)
            blank3d_gloco_set_suspended(&g.gloco,
                                        g.enemies[i].weapon_actor_id, 1);
    }
    blank3d_gloco_tick(&g.gloco, g.frame_ms);
    (void)blank3d_vehicle_system_update(&g.vehicles, g.dt);
    (void)blank3d_runtime_sync(&g.runtime);

    if (g.key_pressed['M']) {
        int mount_result;
        mount_result = blank3d_vehicle_system_toggle_player(&g.vehicles);
        if (mount_result == B3D_MOUNT_VEHICLE_NOT_NEAR) {
            set_status("Vehicle System: get close to a drivable box, then press M");
        } else if (mount_result != B3D_MOUNT_VEHICLE_OK) {
            set_status("Vehicle System: mount/unmount failed or playerdriving=0");
        } else if (blank3d_vehicle_system_playerdriving(&g.vehicles)) {
            const Blank3DVehicleInstance *active_vehicle;
            char vehicle_status[192];
            if (g.gloco.initialized)
                blank3d_gloco_set_suspended(&g.gloco,
                                            B3D_PLAYER_ACTOR_ID, 1);
            active_vehicle = blank3d_vehicle_system_active_const(&g.vehicles);
            if (active_vehicle) {
                if (!b3d_reload_vehicle_ddsl_path(
                        active_vehicle->config.driver_ddsl))
                    set_status(blank3d_languages_status());
                sprintf(vehicle_status,
                    "Vehicle DDSL2: PLAYERDRIVING %s [%s]",
                    active_vehicle->config.name,
                    blank3d_mount_vehicle_profile(&active_vehicle->mount));
                set_status(vehicle_status);
            }
        } else {
            if (g.gloco.initialized)
                blank3d_gloco_set_suspended(&g.gloco,
                                            B3D_PLAYER_ACTOR_ID, 0);
            blank3d_vertical_body_init(&g.player_vertical, &g.player,
                                       g.player.position.y, 0);
            (void)b3d_reload_vehicle_ddsl_path(SCRIPT_VEHICLE_DDSL2);
            set_status("Vehicle System: UNMOUNTED - player DDSL locomotion restored");
        }
    }

    if (g.key_pressed['V']) {
        const Blank3DCameraProfile *profile;
        (void)blank3d_cameranaku_next_profile(&g.cameranaku);
        b3d_camera_sync_profile_fields();
        profile = b3d_camera_profile();
        if (profile) {
            char camera_status[192];
            sprintf(camera_status, "CamNaku INI: %s [%s] (%d/%d)",
                    profile->name, profile->id,
                    blank3d_cameranaku_profile_index(&g.cameranaku) + 1,
                    blank3d_cameranaku_profile_count(&g.cameranaku));
            set_status(camera_status);
        }
    }
    if (g.key_pressed['R']) {
        int reload_result;
        char reload_status[192];
        reload_result = blank3d_systems_reload(&g.systems);
        if (reload_result == GWP89_NO_AMMO) {
            sprintf(reload_status, "reload blocked: %s has no reserve ammo",
                    blank3d_systems_weapon_name(&g.systems));
            set_status(reload_status);
        } else if (reload_result == GWP89_CANCELLED) {
            set_status("reload blocked by weapon.can_reload/provider");
        } else if (reload_result < 0) {
            sprintf(reload_status, "reload failed: code %d", reload_result);
            set_status(reload_status);
        }
    }
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
        (void)b3d_reload_vehicle_ddsl_path(g.vehicle_ddsl2_path);
        (void)blank3d_languages_reload_fpil(SCRIPT_FPI);
        apply_camera_config();
        g.ddsl2_stamp = get_stamp(SCRIPT_DDSL2);
        g.vehicle_ddsl2_stamp = get_stamp(g.vehicle_ddsl2_path);
        g.fpi_stamp = get_stamp(SCRIPT_FPI);
        set_status("RPYL + DDSL2 + FPIL vendor runtimes reloaded");
    }
    if (stamp_changed(&g.rpy_stamp, SCRIPT_RPY)) {
        load_rpy();
        apply_camera_config();
    }
    if (stamp_changed(&g.ddsl2_stamp, SCRIPT_DDSL2)) {
        if (b3d_reload_vehicle_ddsl_path(g.vehicle_ddsl2_path))
            set_status("player.ddsl2 hot reloaded by vendored DDSL2");
        else
            set_status(blank3d_languages_status());
    }
    if (stamp_changed(&g.vehicle_ddsl2_stamp,
                      g.vehicle_ddsl2_path[0] ? g.vehicle_ddsl2_path
                                              : SCRIPT_VEHICLE_DDSL2)) {
        if (b3d_reload_vehicle_ddsl_path(g.vehicle_ddsl2_path))
            set_status("active vehicle driver DDSL2 hot reloaded");
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
    sync_katana_melee();
    update_weapon_system();
    b3d_sync_pickup_lifecycle();
    update_projectiles();
    update_casings();
    blank3d_spriteplanes_update(&g.spriteplanes, (unsigned int)g.frame_ms);
    blank3d_sprite_runtime89_step(&g.sprite_runtime, (unsigned int)g.frame_ms);
    blank3d_muzzle_light_update(&g.muzzle_light, (unsigned int)g.frame_ms);

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

    aspect = b3d_camera_draw_aspect();
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


static unsigned int b3d_flame_seconds_to_ms(g3d_fix seconds_q12)
{
    unsigned long value;
    if (seconds_q12 <= 0) return 0U;
    value = ((unsigned long)seconds_q12 * 1000UL) /
            (unsigned long)G3D_FIX_ONE;
    if (value > 65535UL) value = 65535UL;
    return (unsigned int)value;
}

static int b3d_draw_projectile_billboard_skin(const Bullet *bullet,
                                               int projectile_slot,
                                               const Vec3 *camera_right,
                                               const Vec3 *camera_up)
{
    const Blank3DWeaponModules *modules;
    const Blank3DImageAsset *asset;
    PV2D89Sample visual;
    PV2D89RenderPass *pass;
    Vec3 corners[4];
    g3d_fix elapsed_q12;
    unsigned int age_ms;
    unsigned int life_ms;
    unsigned long tint;
    int sx;
    int sy;
    int sw;
    int sh;
    int i;
    int image_id;
    int additive;
    int rc;
    if (!bullet || !camera_right || !camera_up) return 0;
    modules = blank3d_weapon_modules_get(bullet->weapon_id);
    if (!modules) return 0;

    elapsed_q12 = g3d_fix_sub_sat(bullet->life_total, bullet->life);
    if (elapsed_q12 < 0) elapsed_q12 = 0;
    age_ms = b3d_flame_seconds_to_ms(elapsed_q12);
    life_ms = b3d_flame_seconds_to_ms(bullet->life_total);
    rc = blank3d_projectilevisual2d89_build(
        &g.projectile_sprites, modules,
        &bullet->transform.position, camera_right, camera_up,
        bullet->mesh_scale, age_ms, life_ms, projectile_slot, &visual);
    if (rc != PV2D89_OK || !visual.valid || visual.pass_count <= 0) return 0;

    image_id = visual.passes[0].image_handle;
    if (image_id <= 0 || !blank3d_image_assets_load(&g.images, image_id)) return 0;
    asset = blank3d_image_assets_get(&g.images, image_id);
    if (!asset || !asset->loaded || !asset->texture_token) return 0;
    additive = visual.passes[0].blend_mode == PV2D89_BLEND_ADDITIVE;
    if (!blank3d_image_gl_world_effect_begin(&g.images, image_id, additive)) return 0;

    for (i = 0; i < visual.pass_count; ++i) {
        pass = &visual.passes[i];
        if (!pass->enabled || pass->image_handle != image_id) continue;
        if (pass->source_enabled) {
            sx = pass->source_x;
            sy = pass->source_y;
            sw = pass->source_w;
            sh = pass->source_h;
        } else {
            sx = 0;
            sy = 0;
            sw = (int)asset->width;
            sh = (int)asset->height;
        }
        if (sx < 0 || sy < 0 || sw <= 0 || sh <= 0 ||
            (unsigned int)sx + (unsigned int)sw > asset->width ||
            (unsigned int)sy + (unsigned int)sh > asset->height) continue;
        corners[0].x = (g3d_fix)(pass->corners_q16[0].x / 16L);
        corners[0].y = (g3d_fix)(pass->corners_q16[0].y / 16L);
        corners[0].z = (g3d_fix)(pass->corners_q16[0].z / 16L);
        corners[1].x = (g3d_fix)(pass->corners_q16[1].x / 16L);
        corners[1].y = (g3d_fix)(pass->corners_q16[1].y / 16L);
        corners[1].z = (g3d_fix)(pass->corners_q16[1].z / 16L);
        corners[2].x = (g3d_fix)(pass->corners_q16[2].x / 16L);
        corners[2].y = (g3d_fix)(pass->corners_q16[2].y / 16L);
        corners[2].z = (g3d_fix)(pass->corners_q16[2].z / 16L);
        corners[3].x = (g3d_fix)(pass->corners_q16[3].x / 16L);
        corners[3].y = (g3d_fix)(pass->corners_q16[3].y / 16L);
        corners[3].z = (g3d_fix)(pass->corners_q16[3].z / 16L);
        tint = ((unsigned long)pass->tint.r << 24) |
               ((unsigned long)pass->tint.g << 16) |
               ((unsigned long)pass->tint.b << 8) |
               (unsigned long)pass->tint.a;
        blank3d_image_gl_world_quad_subrect(asset, sx, sy, sw, sh,
            corners, pass->flip_x, pass->flip_y, tint);
    }
    blank3d_image_gl_world_effect_end();
    return visual.replace_mesh ? 1 : 0;
}

static void draw_projectile_mesh(const Bullet *bullet,
                                 int projectile_slot,
                                 soq3d_key thing_key,
                                 const g3d_mesh *mesh,
                                 const Vec3 *camera_right,
                                 const Vec3 *camera_up)
{
    soq3d_pose pose;
    soq3d_pose glow_pose;
    const Blank3DWeaponModules *modules;
    int special;
    if (!bullet || !mesh) return;
    modules = blank3d_weapon_modules_get(bullet->weapon_id);
    if (modules && modules->projectile_visual_billboard &&
        modules->projectile_sprite_mode != GWM89_PROJECTILE_SPRITE_NONE &&
        b3d_draw_projectile_billboard_skin(bullet, projectile_slot,
                                            camera_right, camera_up))
        return;
    if (soq3d_get_thing(&g.locator, thing_key,
                        g.frame_stamp, &pose) != SOQ3D_OK) return;
    special = modules && modules->projectile_visual !=
              B3D_PROJECTILE_VISUAL_DEFAULT;
    glPushMatrix();
    bridge_gl_apply_pose(&pose);
    if (special && modules->projectile_visual_unlit) glDisable(GL_LIGHTING);
    if (special && modules->projectile_visual_additive) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    }
    bridge_gl_draw_mesh(mesh);
    glPopMatrix();

    if (special && modules->projectile_visual_additive) {
        glow_pose = pose;
        glow_pose.basis.m00 = (signed int)(((long)glow_pose.basis.m00 * 5L) / 4L);
        glow_pose.basis.m10 = (signed int)(((long)glow_pose.basis.m10 * 5L) / 4L);
        glow_pose.basis.m20 = (signed int)(((long)glow_pose.basis.m20 * 5L) / 4L);
        glow_pose.basis.m01 = (signed int)(((long)glow_pose.basis.m01 * 5L) / 4L);
        glow_pose.basis.m11 = (signed int)(((long)glow_pose.basis.m11 * 5L) / 4L);
        glow_pose.basis.m21 = (signed int)(((long)glow_pose.basis.m21 * 5L) / 4L);
        if (modules->projectile_visual ==
                B3D_PROJECTILE_VISUAL_EXPANDIBLE_FIRE) {
            glow_pose.basis.m02 = (signed int)(((long)glow_pose.basis.m02 * 5L) / 4L);
            glow_pose.basis.m12 = (signed int)(((long)glow_pose.basis.m12 * 5L) / 4L);
            glow_pose.basis.m22 = (signed int)(((long)glow_pose.basis.m22 * 5L) / 4L);
        }
        glPushMatrix();
        bridge_gl_apply_pose(&glow_pose);
        bridge_gl_draw_mesh(mesh);
        glPopMatrix();
        glDisable(GL_BLEND);
    }
    if (special && modules->projectile_visual_unlit) glEnable(GL_LIGHTING);
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

static void draw_pickup_instance_mesh(const Blank3DPickupInstance *pickup)
{
    int part_index;
    const Blank3DPickupPart *part;
    if (!pickup) return;
    /* Test/world pickups are deliberately unlit: they must stay readable
       against the dark Blank3D scene regardless of the current light pose. */
    glDisable(GL_LIGHTING);
    for (part_index = 0; part_index < pickup->part_count; ++part_index) {
        part = &pickup->parts[part_index];
        if (part->used) draw_transform_mesh(&part->transform, &part->mesh);
    }
    glEnable(GL_LIGHTING);
}

static void draw_pickups(void)
{
    int i;
    const Blank3DPickupInstance *pickup;
    for (i = 0; i < B3D_PICKUP_MAX_INSTANCES; ++i) {
        if (!blank3d_pickups_is_world_active(&g.pickups, (unsigned int)i))
            continue;
        pickup = blank3d_pickups_get_const(&g.pickups, (unsigned int)i);
        if (!pickup) continue;
        /* Runtime guarantee: GFO is the normal path, but merely being alive
           is not proof that *render actually submitted geometry this frame.
           If the GFO path did not reach draw_mesh(), submit directly here. */
        if (pickup->render_submit_frame != (unsigned long)g.frame_stamp)
            draw_pickup_instance_mesh(pickup);
    }
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


static void b3d_apply_projectile_visual_stream_light(void)
{
    PV2D89LightAccumulator accum;
    PV2D89LightSample item;
    PV2D89LightSample light;
    PV2D89Recipe recipe;
    const Blank3DWeaponModules *modules;
    int i;
    pv2d89_light_accumulator_init(&accum);
    for (i = 0; i < MAX_BULLETS; ++i) {
        if (!g.bullets[i].alive || g.bullets[i].cosmetic_only) continue;
        modules = blank3d_weapon_modules_get(g.bullets[i].weapon_id);
        if (!modules) continue;
        blank3d_projectilevisual2d89_recipe(modules, &recipe);
        if (!pv2d89_wants_billboard(&recipe) || !recipe.light_enabled) continue;
        memset(&item, 0, sizeof(item));
        item.enabled = 1;
        item.group_id = recipe.weapon_id;
        item.position_q16.x = (long)g.bullets[i].transform.position.x * 16L;
        item.position_q16.y = (long)g.bullets[i].transform.position.y * 16L;
        item.position_q16.z = (long)g.bullets[i].transform.position.z * 16L;
        item.intensity_q16 = recipe.light_intensity_q16;
        item.radius_q16 = recipe.light_radius_q16;
        item.color = recipe.light_color;
        pv2d89_light_accumulator_add(&accum, &item);
    }
    if (!pv2d89_light_accumulator_finish(&accum,
            (unsigned long)g.frame_stamp, &light)) {
        bridge_gl_set_flamethrower_light(0, 0, 0, 0, 0L, 0L, 0U, 0U, 0U);
        return;
    }
    bridge_gl_set_flamethrower_light(1,
        (g3d_fix)(light.position_q16.x / 16L),
        (g3d_fix)(light.position_q16.y / 16L),
        (g3d_fix)(light.position_q16.z / 16L),
        light.intensity_q16, light.radius_q16,
        light.color.r, light.color.g, light.color.b);
}

static void draw_scene(void)
{
    int i;
    const Blank3DCameraProfile *camera_profile;
    Vec3 trail_camera;
    Vec3 flame_right;
    Vec3 flame_up;
    if (!blank3d_video_gl89_begin_render(&g.video_gl))
        set_status(blank3d_video_gl89_status(&g.video_gl));
    bridge_gl_begin_frame(g.width, g.height);
    blank3d_numbar_begin_frame(&g.numbars);
    /* Object/GFO render callbacks need the same camera already installed
       as the batched actor renderer. */
    set_camera();

    /* Sky is background, not a post-process.  Rendering it after opaque
       geometry allowed screen/dome layers to pass the depth test over world
       pixels and visually tint floors, actors and textured objects.  Draw it
       first with its own depth-write-disabled state so every opaque/world
       consumer deterministically paints over it afterwards. */
    build_view_vectors(&trail_camera, 0, &flame_right, &flame_up);
    (void)blank3d_skybox89_gl_render(&g.skybox, &trail_camera);

    {
        Blank3DMuzzleLightSample muzzle_light_sample;
        if (blank3d_muzzle_light_sample(&g.muzzle_light,
                                        &muzzle_light_sample)) {
            bridge_gl_set_muzzle_light(1,
                (g3d_fix)muzzle_light_sample.x_q12,
                (g3d_fix)muzzle_light_sample.y_q12,
                (g3d_fix)muzzle_light_sample.z_q12,
                muzzle_light_sample.intensity_q16,
                muzzle_light_sample.radius_q16,
                (unsigned char)muzzle_light_sample.r,
                (unsigned char)muzzle_light_sample.g,
                (unsigned char)muzzle_light_sample.b);
        } else {
            bridge_gl_set_muzzle_light(0, 0, 0, 0, 0L, 0L, 0U, 0U, 0U);
        }
    }
    b3d_apply_projectile_visual_stream_light();
    blank3d_objects_render(&g.objects);
    if (g.show_grid) bridge_gl_draw_grid(30);
    {
        int vehicle_index;
        for (vehicle_index = 0;
             vehicle_index < blank3d_vehicle_system_count(&g.vehicles);
             ++vehicle_index) {
            const Blank3DVehicleInstance *vehicle_instance;
            const Transform *car;
            vehicle_instance = blank3d_vehicle_system_at_const(
                &g.vehicles, vehicle_index);
            if (!vehicle_instance || !vehicle_instance->used) continue;
            car = blank3d_mount_vehicle_car_transform_const(
                &vehicle_instance->mount);
            if (car) {
                Transform car_visual;
                car_visual = *car;
                car_visual.position.y = g3d_fix_add_sat(
                    car_visual.position.y,
                    vehicle_instance->config.visual_y_offset);
                car_visual.scale.x = vehicle_instance->config.visual_scale_x;
                car_visual.scale.y = vehicle_instance->config.visual_scale_y;
                car_visual.scale.z = vehicle_instance->config.visual_scale_z;
                draw_transform_mesh(&car_visual, &g.meshes.mount_car);
            }
        }
    }
    draw_pickups();

    camera_profile = b3d_camera_profile();
    if (!camera_profile || camera_profile->player_body_visible)
        draw_socket_mesh(g.player_body_socket, &g.meshes.player_body);
    if (!camera_profile || camera_profile->player_weapon_visible)
        draw_actor_weapon(B3D_PLAYER_ACTOR_ID);
    draw_katana_melee();
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
    blank3d_katana_melee_debug_draw(&g.katana_melee,
                                    b3d_katana_debug_line, 0);
    for (i = 0; i < MAX_BULLETS; ++i) {
        if (!g.bullets[i].alive) continue;
        draw_projectile_mesh(&g.bullets[i], i, g.bullet_keys[i],
                             projectile_mesh_for_bullet(&g.bullets[i]),
                             &flame_right, &flame_up);
        /* Never draw the player hitscan authority ray.  Only actual moving
           projectiles may use this tiny fallback segment when no Aoi trail
           provider is available. */
        if (g.bullets[i].cosmetic_only &&
            g.bullets[i].aoi_trail_id < 0 &&
            !g.bullets[i].cosmetic_arrived)
            bridge_gl_draw_segment(&g.bullets[i].previous_position,
                                   &g.bullets[i].transform.position,
                                   220U, 235U, 255U);
        else if (!g.bullets[i].cosmetic_only &&
                 g.bullets[i].aoi_trail_id < 0 &&
                 g.bullets[i].trail_id != 0)
            bridge_gl_draw_segment(&g.bullets[i].previous_position,
                                   &g.bullets[i].transform.position,
                                   255U, 220U, 96U);
    }
    blank3d_trails_draw(&g.trails, trail_camera.x,
                        trail_camera.y, trail_camera.z);
    b3d_draw_world_spriteplanes();
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
    blank3d_hud_set_weapon_ammo_id(&g.hud,
                                   blank3d_systems_ammo_id(&g.systems));
    blank3d_hud_draw(&g.hud, &g.numbars, g.width, g.height,
                     g.player_hp,
                     blank3d_systems_clip(&g.systems),
                     current_clip_capacity(),
                     blank3d_systems_reserve(&g.systems),
                     blank3d_systems_weapon_id(&g.systems),
                     b3d_camera_is_fps(),
                     g.mouse_right,
                     g.muzzle_flash_ms > 0,
                     b3d_player_threat_level(),
                     (unsigned int)g.player_damage_flash_ms,
                     (unsigned int)g.frame_ms,
                     &g.sniper);
    blank3d_image_gl_begin_overlay(g.width, g.height);
    (void)blank3d_sprite_runtime89_render(&g.sprite_runtime);
    blank3d_image_gl_end_overlay();
    if (!blank3d_video_gl89_present(&g.video_gl))
        set_status(blank3d_video_gl89_status(&g.video_gl));
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
        {
            int window_width;
            int window_height;
            window_width = LOWORD(lparam);
            window_height = HIWORD(lparam);
            if (window_width > 0 && window_height > 0)
                (void)blank3d_display_stack89_window_resized(
                    &g.display, window_width, window_height);
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

static int create_display_window(HINSTANCE instance)
{
    gwc89_provider window_provider;

    blank3d_window_win32_host_init(&g.window_host, instance, window_proc,
                                    &g.window, &g.device);
    if (!blank3d_window_win32_make_provider(&g.window_host, &window_provider)) {
        set_status("HOWM89/WindowsWindow89 provider bind failed");
        return 0;
    }
    blank3d_display_stack89_set_window_provider(&g.display, &window_provider);
    if (!blank3d_display_stack89_create_window(&g.display)) {
        set_status(blank3d_display_stack89_status(&g.display));
        blank3d_window_win32_host_shutdown(&g.window_host);
        return 0;
    }
    if (!blank3d_video_gl89_create_context(&g.video_gl, g.device)) {
        set_status(blank3d_video_gl89_status(&g.video_gl));
        (void)gwc89_destroy(&g.display.window);
        blank3d_window_win32_host_shutdown(&g.window_host);
        return 0;
    }
    g.gl_context = g.video_gl.context;
    return 1;
}

static void apply_camera_config(void)
{
    const char *requested_profile;
    g.lock_mouse = g.config.lock_mouse;
    g.mouse_sensitivity = q16_to_q12(g.config.mouse_sensitivity_q16);
    g.zoom_fov = q16_to_q12(g.config.zoom_fov_q16);
    g.sniper_zoom_fov = q16_to_q12(g.config.sniper_zoom_fov_q16);
    g.zoom_speed = q16_to_q12(g.config.zoom_speed_q16);
    if (g.zoom_fov <= 0) g.zoom_fov = G3D_FIX_FROM_INT(45);
    if (g.sniper_zoom_fov <= 0) g.sniper_zoom_fov = G3D_FIX_FROM_INT(18);
    if (g.zoom_speed <= 0) g.zoom_speed = G3D_FIX_FROM_INT(120);

    (void)blank3d_cameranaku_load_profiles(
        &g.cameranaku, g.config.camera_profile_dir);
    requested_profile = g.config.start_first_person
                      ? "fps" : g.config.camera_start_profile;
    if (!blank3d_cameranaku_set_profile(&g.cameranaku,
                                         requested_profile)) {
        if (!blank3d_cameranaku_set_profile(&g.cameranaku,
                                             "tps_centred"))
            (void)blank3d_cameranaku_set_profile_index(&g.cameranaku, 0);
    }
    b3d_camera_sync_profile_fields();
    g.camera_yaw = g.player.rotation.y;
    g.camera_pitch = g3d_fix_clamp(g.camera_pitch,
                                   g.camera_pitch_min,
                                   g.camera_pitch_max);
    blank3d_cameranaku_set_angles(&g.cameranaku,
                                  g.camera_yaw,
                                  g.camera_pitch);
    configure_sockets();
}

static void apply_runtime_config(void)
{
    blank3d_config_load(&g.config, CONFIG_FILE);
    if (blank3d_display_stack89_set_gameplay_sizes(
            &g.display,
            g.config.camera_draw_width, g.config.camera_draw_height,
            g.config.scene_screen_width, g.config.scene_screen_height))
        (void)gpss89_apply(&g.display.gameplay);
    blank3d_skybox_recipe89_set_catalog(&g.skybox_recipes,
                                        g.config.skybox_catalog);
    if (!g.skybox_rpyl_override) {
        if (g.config.skybox_enabled && g.config.skybox_recipe[0]) {
            if (!blank3d_skybox_recipe89_apply_named(&g.skybox_recipes,
                                                      g.config.skybox_recipe))
                blank3d_skybox_recipe89_disable(&g.skybox_recipes);
        } else {
            blank3d_skybox_recipe89_disable(&g.skybox_recipes);
        }
    }
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
    g.trigger_spinup_ms = (unsigned int)g.config.gatling_spinup_ms;
    if (g.config.slingshot_charge_ms < 100)
        g.config.slingshot_charge_ms = 100;
    if (g.config.slingshot_charge_ms > 5000)
        g.config.slingshot_charge_ms = 5000;
    g.trigger_charge_max_ms =
        (unsigned int)g.config.slingshot_charge_ms;

    gwp89_set_ammo(&g.systems.weapons, B3D_PLAYER_ACTOR_ID, 1, g.config.ammo_9mm);
    gwp89_set_ammo(&g.systems.weapons, B3D_PLAYER_ACTOR_ID, 2, g.config.ammo_shells);
    gwp89_set_ammo(&g.systems.weapons, B3D_PLAYER_ACTOR_ID, 3, g.config.ammo_magnum);
    gwp89_set_ammo(&g.systems.weapons, B3D_PLAYER_ACTOR_ID, 4, g.config.ammo_sniper);
    gwp89_set_ammo(&g.systems.weapons, B3D_PLAYER_ACTOR_ID, 5, g.config.ammo_grenades);
    gwp89_set_ammo(&g.systems.weapons, B3D_PLAYER_ACTOR_ID, 6, g.config.ammo_rockets);
    gwp89_set_ammo(&g.systems.weapons, B3D_PLAYER_ACTOR_ID, 7, g.config.ammo_gatling);
    gwp89_set_ammo(&g.systems.weapons, B3D_PLAYER_ACTOR_ID, 8, g.config.ammo_stones);
    gwp89_set_ammo(&g.systems.weapons, B3D_PLAYER_ACTOR_ID, 9, g.config.ammo_hand_grenades);
    gwp89_set_ammo(&g.systems.weapons, B3D_PLAYER_ACTOR_ID, 11, g.config.ammo_shango_cells);
    gwp89_set_ammo(&g.systems.weapons, B3D_PLAYER_ACTOR_ID, 12, g.config.ammo_homing_rockets);
    gwp89_set_ammo(&g.systems.weapons, B3D_PLAYER_ACTOR_ID, 13, g.config.ammo_fuel);
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
    rt_time_source time_source;
    int i;

    (void)previous;
    (void)command_line;
    (void)show;
    memset(&g, 0, sizeof(g));
    g.instance = instance;
    g.running = 1;
    blank3d_video_gl89_init(&g.video_gl);
    blank3d_display_stack89_init(&g.display);
    (void)blank3d_display_stack89_load_general_cfg(&g.display,
                                                    GENERAL_CONFIG_FILE);
    (void)blank3d_config_load(&g.config, CONFIG_FILE);
    {
        gvc89_provider video_provider;
        gpss89_provider gameplay_provider;
        memset(&video_provider, 0, sizeof(video_provider));
        video_provider.user = &g;
        video_provider.apply_video = b3d_display_video_provider;
        blank3d_display_stack89_set_video_provider(&g.display,
                                                    &video_provider);
        memset(&gameplay_provider, 0, sizeof(gameplay_provider));
        gameplay_provider.user = &g;
        gameplay_provider.apply_gameplay_screen =
            b3d_display_gameplay_provider;
        blank3d_display_stack89_set_gameplay_provider(&g.display,
                                                       &gameplay_provider);
    }
    if (!blank3d_display_stack89_set_gameplay_sizes(
            &g.display,
            g.config.camera_draw_width, g.config.camera_draw_height,
            g.config.scene_screen_width, g.config.scene_screen_height) ||
        !blank3d_display_stack89_apply_video_gameplay(&g.display))
        return 2;
    soq3d_init(&g.locator);
    initialize_keys();
    blank3d_vertical_axis_init_gamlib3d(&g.vertical_axis);
    default_world();
    blank3d_fire_frame_sync_init(&g.fire_frame_sync);
    blank3d_cameranaku_init(&g.cameranaku,
                             b3d_camera_draw_width(),
                             b3d_camera_draw_height(),
                             g.fov, g.near_z, g.far_z);
    blank3d_katana_config_defaults(&g.katana_config);
    (void)blank3d_katana_config_load(&g.katana_config,
                                     "config/melee/katana.ini");
    if (!initialize_meshes()) return 2;
    if (!blank3d_katana_melee_init(&g.katana_melee,
                                   &g.katana_config)) return 2;
    blank3d_attachment_init(&g.attachments);
    blank3d_weapon_presentation_registry_init(&g.weapon_presentations);
    blank3d_input_init(&g.input);
    (void)blank3d_input_platform_attach_default(&g.input_platform, &g.input);
    time_source.get_ticks = b3d_win32_time_ticks;
    time_source.ticks_per_second = 1000U;
    time_source.user = (void *)0;
    if (!blank3d_time89_init(&g.time_system, &time_source, 50U, 60U))
        return 2;
    blank3d_kinverbs_init(&g.kinverbs);
    if (!g.kinverbs.initialized || !b3d_register_movement_gameverbs() ||
        !b3d_register_vehicle_gameverbs() ||
        !blank3d_timeverbs89_init(&g.time_verbs, &g.time_system,
                                  blank3d_kinverbs_registry(&g.kinverbs)) ||
        !blank3d_display_stack89_register_verbs(
            &g.display, blank3d_kinverbs_registry(&g.kinverbs)))
        return 2;
    blank3d_init_language_runtime();
    if (!blank3d_init_object_runtime()) return 2;
    if (!blank3d_init_runtime_spine()) return 2;
    blank3d_perception_world_init(&g.perception,
                                  b3d_perception_raycast,
                                  &g.collision);
    (void)b3d_reload_vehicle_ddsl_path(SCRIPT_VEHICLE_DDSL2);
    (void)blank3d_languages_reload_fpil(SCRIPT_FPI);
    blank3d_systems_init(&g.systems);
    blank3d_pickups_init(&g.pickups, &g.systems, &g.player);
    if (!blank3d_vehicle_system_init(&g.vehicles, &g.player,
                                     B3D_PLAYER_ACTOR_ID)) return 2;
    blank3d_variables_init(&g.variables, &g.systems);
    blank3d_condor_init(&g.condor, blank3d_kinverbs_registry(&g.kinverbs),
                        &g.variables);
    blank3d_gloco_init(&g.gloco, &g.systems, &g.variables,
                       &g.vertical_axis, &g.movement_gamlib);
    {
        Blank3DImageBackend image_backend;
        Blank3DSprite89RenderHost sprite_host;
        blank3d_image_assets_init(&g.images);
        blank3d_image_gl_make_backend(&image_backend);
        blank3d_image_assets_set_backend(&g.images, &image_backend);
        blank3d_spriteplanes_init(&g.spriteplanes, &g.images);
        blank3d_sprite_runtime89_init(&g.sprite_runtime, &g.images);
        blank3d_skybox89_init(&g.skybox, &g.images);
        blank3d_skybox_recipe89_init(&g.skybox_recipes, &g.skybox);
        (void)blank3d_config_load(&g.config, CONFIG_FILE);
        blank3d_skybox_recipe89_set_catalog(&g.skybox_recipes,
                                            g.config.skybox_catalog);
        blank3d_skybox89_gl_attach(&g.skybox);
        memset(&sprite_host, 0, sizeof(sprite_host));
        sprite_host.user = &g.images;
        sprite_host.draw = b3d_sprite89_draw_host;
        blank3d_sprite_runtime89_set_render_host(&g.sprite_runtime,
                                                  &sprite_host);
        blank3d_projectile_sprite89_init(&g.projectile_sprites, &g.images,
                                         &g.sprite_runtime.routes);
    }
    load_rpy();
    init_weapon_runtime_providers();
    blank3d_list_cycle_init(&g.list_cycles);
    (void)blank3d_systems_register_cycle_lists(&g.systems, &g.list_cycles);
    b3d_reset_npc_weapon_manager();
    {
        char presentation_status[160];
        presentation_status[0] = '\0';
        if (gweaponpresentation89_load_manifest(
                &g.systems.weapons, &g.weapon_presentations,
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
    blank3d_gloco_attach_physics(&g.gloco, &g.collision, &g.physics);
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
                            ? (sniper_modules->scope_recipe_path[0]
                               ? sniper_modules->scope_recipe_path
                               : sniper_modules->scope_preset_name)
                            : 0);
        g.sniper_screen_width = g.width;
        g.sniper_screen_height = g.height;
    }
    blank3d_text89_init(&g.text);
    g.text.default_pixel_size = g.config.text_pixel_size;
    if (g.config.text_enabled && g.config.text_font_path[0]) {
        if (!blank3d_text89_load_file(&g.text, g.config.text_font_path)) {
            fprintf(stderr, "[MonikaFontCore] %s\n",
                    blank3d_text89_status(&g.text));
        }
    }
    blank3d_hud_init(&g.hud);
    blank3d_hud_set_text_provider(&g.hud, &g.text);
    {
        Blank3DHudSpriteProvider sprite_provider;
        b3d_register_crosshair_image_assets();
        blank3d_image_gl_make_hud_provider(&sprite_provider, &g.images);
        blank3d_hud_set_sprite_provider(&g.hud, &sprite_provider);
        b3d_register_scope_image_provider();
    }
    g.player_hp = blank3d_systems_player_health(&g.systems);
    g.rpy_stamp = get_stamp(SCRIPT_RPY);
    g.ddsl2_stamp = get_stamp(SCRIPT_DDSL2);
    g.vehicle_ddsl2_stamp = get_stamp(SCRIPT_VEHICLE_DDSL2);
    g.fpi_stamp = get_stamp(SCRIPT_FPI);
    if (!create_display_window(instance)) return 1;
    g.active = 1;
    if (!blank3d_audio_init(&g.audio, g.config.audio_enabled)) {
        set_status(blank3d_audio_status(&g.audio));
    }
    set_mouse_capture_state(g.lock_mouse && g.active);
    GetCursorPos(&g.last_mouse);
    while (g.running) {
        unsigned int frame_ms;
        while (PeekMessage(&message, 0, 0, 0, PM_REMOVE)) {
            TranslateMessage(&message);
            DispatchMessage(&message);
        }
        blank3d_time89_update(&g.time_system);
        (void)blank3d_timeverbs89_pump_alarms(&g.time_verbs);
        frame_ms = blank3d_time89_frame_ms(&g.time_system);
        if (frame_ms > 65535U) frame_ms = 65535U;
        g.frame_ms = (unsigned short)frame_ms;
        g.dt = (g3d_fix)(blank3d_time89_delta_q16(&g.time_system) >> 4);
        g.time = g3d_fix_add_sat(g.time, g.dt);
        update_world();
        draw_scene();
        Sleep(1U);
    }

    set_mouse_capture_state(0);
    blank3d_audio_shutdown(&g.audio);
    blank3d_image_assets_shutdown(&g.images);
    blank3d_input_platform_shutdown(&g.input_platform, &g.input);
    blank3d_input_shutdown(&g.input);
    blank3d_video_gl89_destroy(&g.video_gl);
    g.gl_context = 0;
    (void)gwc89_destroy(&g.display.window);
    blank3d_window_win32_host_shutdown(&g.window_host);
    return 0;
}
