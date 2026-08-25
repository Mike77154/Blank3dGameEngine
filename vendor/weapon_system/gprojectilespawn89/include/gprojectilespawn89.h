#ifndef GPROJECTILESPAWN89_H
#define GPROJECTILESPAWN89_H
#ifdef __cplusplus
extern "C" {
#endif

typedef long gps89_fx;
typedef struct gps89_vec3_s { gps89_fx x,y,z; } gps89_vec3;
typedef struct gps89_request_s {
    int actor_id;
    int team_id;
    int target_actor_id;
    int weapon_id;
    int projectile_id;
    int projectile_mesh_id;
    int trail_id;
    int physics_kind;
    int pellet_index;
    int pellet_count;
    gps89_vec3 origin;
    gps89_vec3 direction;
    gps89_fx speed_fx;
    gps89_fx gravity_fx;
    gps89_fx radius_fx;
    gps89_fx damage_fx;
    gps89_fx mesh_scale_fx;
    unsigned int life_ms;
    unsigned long flags;
    const void *source;
} gps89_request;

typedef struct gps89_handle_s { int slot; long world_id; void *object; } gps89_handle;

typedef int (*gps89_spawn_provider_fn)(void*,const gps89_request*,gps89_handle*);
typedef void (*gps89_stage_provider_fn)(void*,const gps89_request*,const gps89_handle*);
typedef void (*gps89_release_provider_fn)(void*,const gps89_handle*);

typedef struct gps89_providers_s {
    void *user;
    gps89_spawn_provider_fn world_spawn;
    gps89_stage_provider_fn collision_register;
    gps89_stage_provider_fn render_spawn;
    gps89_stage_provider_fn audio_spawn;
    gps89_stage_provider_fn world_publish;
    gps89_release_provider_fn world_release;
} gps89_providers;

typedef struct gps89_runtime_s {
    gps89_providers providers;
    unsigned long accepted;
    unsigned long rejected;
} gps89_runtime;

void gprojectilespawn89_init(gps89_runtime *runtime,const gps89_providers *providers);
int gprojectilespawn89_spawn(gps89_runtime *runtime,const gps89_request *request,gps89_handle *handle_out);
#ifdef __cplusplus
}
#endif
#endif
