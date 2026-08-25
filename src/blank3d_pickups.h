#ifndef BLANK3D_PICKUPS_H
#define BLANK3D_PICKUPS_H

#include "blank3d_systems.h"
#include "../vendor/gamlib3d/gamlib3d_transform.h"
#include "../vendor/giffany_shapes3d/g3d_shapes.h"

#ifdef __cplusplus
extern "C" {
#endif

#define B3D_PICKUP_MAX_INSTANCES 16
#define B3D_PICKUP_MAX_PARTS 4
#define B3D_PICKUP_PATH_CAP 160
#define B3D_PICKUP_NAME_CAP 48
#define B3D_PICKUP_SUBJECT_BASE 5000UL
#define B3D_PICKUP_BOX_VERTEX_CAP 32
#define B3D_PICKUP_BOX_INDEX_CAP 64

#define B3D_PICKUP_KIND_WEAPON 1
#define B3D_PICKUP_KIND_AMMO 2
#define B3D_PICKUP_KIND_HEALTH B3D_SYSTEMS_PICKUP_KIND_HEALTH

typedef struct Blank3DPickupPartTag {
    int used;
    Transform transform;
    g3d_mesh mesh;
    g3d_vertex vertices[B3D_PICKUP_BOX_VERTEX_CAP];
    g3d_index indices[B3D_PICKUP_BOX_INDEX_CAP];
} Blank3DPickupPart;

typedef struct Blank3DPickupInstanceTag {
    int used;
    int gfo_live;
    unsigned long render_submit_frame;
    unsigned long subject_id;
    char name[B3D_PICKUP_NAME_CAP];
    char config_path[B3D_PICKUP_PATH_CAP];
    char gfo_path[B3D_PICKUP_PATH_CAP];
    char mesh_recipe_path[B3D_PICKUP_PATH_CAP];
    int kind;
    int resource_id;
    int amount;
    int auto_equip;
    Transform transform;
    long radius_q16;
    int item_id;
    CT89_Trigger trigger;
    int part_count;
    Blank3DPickupPart parts[B3D_PICKUP_MAX_PARTS];
} Blank3DPickupInstance;

typedef struct Blank3DPickupWorldTag {
    Blank3DSystems *systems;
    Transform *player_transform;
    CT89_SensorProvider contact_provider;
    Blank3DPickupInstance instances[B3D_PICKUP_MAX_INSTANCES];
    char status[192];
} Blank3DPickupWorld;

void blank3d_pickups_init(Blank3DPickupWorld *world,
                          Blank3DSystems *systems,
                          Transform *player_transform);
void blank3d_pickups_reset(Blank3DPickupWorld *world);
int blank3d_pickups_spawn_ini(Blank3DPickupWorld *world,
                              const char *config_path,
                              g3d_fix x, g3d_fix y, g3d_fix z,
                              int forced_kind,
                              unsigned int *out_slot);
int blank3d_pickups_spawn_rpyl_args(Blank3DPickupWorld *world,
                                    const char **args, int argc,
                                    int forced_kind,
                                    unsigned int *out_slot);
int blank3d_pickups_is_world_active(const Blank3DPickupWorld *world,
                                    unsigned int slot);
Blank3DPickupInstance *blank3d_pickups_get(Blank3DPickupWorld *world,
                                          unsigned int slot);
const Blank3DPickupInstance *blank3d_pickups_get_const(
    const Blank3DPickupWorld *world, unsigned int slot);
Blank3DPickupInstance *blank3d_pickups_find_subject(Blank3DPickupWorld *world,
                                                    unsigned long subject_id);
const Blank3DPickupInstance *blank3d_pickups_find_subject_const(
    const Blank3DPickupWorld *world, unsigned long subject_id);
const char *blank3d_pickups_status(const Blank3DPickupWorld *world);

#ifdef __cplusplus
}
#endif

#endif
