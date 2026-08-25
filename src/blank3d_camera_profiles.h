#ifndef BLANK3D_CAMERA_PROFILES_H
#define BLANK3D_CAMERA_PROFILES_H

#include "gamlib3d_math.h"
#include "gweapon89.h"
#include "conf_total.h"

#ifdef __cplusplus
extern "C" {
#endif

#define B3D_CAMERA_PROFILE_MAX 16
#define B3D_CAMERA_PROFILE_ID_CAPACITY 48
#define B3D_CAMERA_PROFILE_NAME_CAPACITY 72
#define B3D_CAMERA_PROFILE_PATH_CAPACITY 224
#define B3D_CAMERA_PROFILE_TEXT_CAPACITY 16384
#define B3D_CAMERA_PROFILE_ARENA_CAPACITY 32768

#define B3D_CAMERA_RIG_FPS   1
#define B3D_CAMERA_RIG_ORBIT 2

#define B3D_CAMERA_AIM_CAMERA_ONLY   1
#define B3D_CAMERA_AIM_CAMERA_MUZZLE 2

typedef struct Blank3DCameraProfileTag {
    int loaded;
    int enabled;
    int order;
    int rig;
    int view_style;
    int aim_mode;
    int player_body_visible;
    int player_weapon_visible;
    int collision_enabled;

    char id[B3D_CAMERA_PROFILE_ID_CAPACITY];
    char name[B3D_CAMERA_PROFILE_NAME_CAPACITY];
    char source_path[B3D_CAMERA_PROFILE_PATH_CAPACITY];

    g3d_fix pivot_x;
    g3d_fix pivot_y;
    g3d_fix pivot_z;
    g3d_fix look_x;
    g3d_fix look_y;
    g3d_fix look_z;
    g3d_fix distance;
    g3d_fix min_distance;
    g3d_fix max_distance;
    g3d_fix offset_right;
    g3d_fix offset_up;
    g3d_fix offset_forward;
    g3d_fix pitch_min;
    g3d_fix pitch_max;
    g3d_fix fov;
    g3d_fix near_clip;
    g3d_fix far_clip;
    g3d_fix pos_lag;
    g3d_fix rot_lag;
    g3d_fix fov_lag;
    g3d_fix mouse_sensitivity;
    g3d_fix collision_radius;
} Blank3DCameraProfile;

typedef struct Blank3DCameraCatalogTag {
    Blank3DCameraProfile profiles[B3D_CAMERA_PROFILE_MAX];
    int count;
    int active_index;
    int loaded_from_directory;
    char directory[B3D_CAMERA_PROFILE_PATH_CAPACITY];
    char status[192];
    char text[B3D_CAMERA_PROFILE_TEXT_CAPACITY];
    unsigned char arena[B3D_CAMERA_PROFILE_ARENA_CAPACITY];
} Blank3DCameraCatalog;

void blank3d_camera_profile_defaults(Blank3DCameraProfile *profile,
                                     int rig,
                                     int view_style);
void blank3d_camera_catalog_init(Blank3DCameraCatalog *catalog);
int blank3d_camera_catalog_load(Blank3DCameraCatalog *catalog,
                                const char *directory);
int blank3d_camera_catalog_select_index(Blank3DCameraCatalog *catalog,
                                        int index);
int blank3d_camera_catalog_select_id(Blank3DCameraCatalog *catalog,
                                     const char *id);
int blank3d_camera_catalog_next(Blank3DCameraCatalog *catalog);
const Blank3DCameraProfile *blank3d_camera_catalog_current(
    const Blank3DCameraCatalog *catalog);
Blank3DCameraProfile *blank3d_camera_catalog_current_mutable(
    Blank3DCameraCatalog *catalog);
const Blank3DCameraProfile *blank3d_camera_catalog_at(
    const Blank3DCameraCatalog *catalog,
    int index);
int blank3d_camera_catalog_find_view_style(
    const Blank3DCameraCatalog *catalog,
    int view_style);
const char *blank3d_camera_catalog_status(
    const Blank3DCameraCatalog *catalog);

#ifdef __cplusplus
}
#endif

#endif
