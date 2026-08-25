#ifndef GWEAPONSNAPSHOT89_H
#define GWEAPONSNAPSHOT89_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef GWS89_CAPACITY
#define GWS89_CAPACITY 32
#endif

typedef long gws89_fx;
typedef struct gws89_vec3_s { gws89_fx x,y,z; } gws89_vec3;

typedef struct gws89_snapshot_s {
    int valid;
    int actor_id;
    unsigned long frame_id;
    gws89_vec3 weapon_origin;
    gws89_vec3 muzzle_origin;
    gws89_vec3 muzzle_forward;
    gws89_vec3 muzzle_right;
    gws89_vec3 muzzle_up;
    gws89_vec3 camera_origin;
    gws89_vec3 camera_forward;
    gws89_vec3 camera_right;
    gws89_vec3 camera_up;
} gws89_snapshot;

typedef struct gws89_store_s {
    gws89_snapshot entries[GWS89_CAPACITY];
    unsigned long writes;
    unsigned long misses;
} gws89_store;

void gweaponsnapshot89_init(gws89_store *store);
int gweaponsnapshot89_publish(gws89_store *store,
                              const gws89_snapshot *snapshot);
int gweaponsnapshot89_get(const gws89_store *store,
                          int actor_id,
                          unsigned long frame_id,
                          gws89_snapshot *out);

#ifdef __cplusplus
}
#endif
#endif
