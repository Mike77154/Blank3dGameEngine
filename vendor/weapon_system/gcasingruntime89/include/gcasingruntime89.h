#ifndef GCASINGRUNTIME89_H
#define GCASINGRUNTIME89_H
#ifdef __cplusplus
extern "C" {
#endif

typedef long gcr89_fx;
typedef struct gcr89_vec3_s { gcr89_fx x,y,z; } gcr89_vec3;
typedef struct gcr89_request_s {
    int actor_id;
    int weapon_id;
    int casing_mesh_id;
    gcr89_fx mesh_scale_fx;
    gcr89_vec3 origin;
    gcr89_vec3 right;
    gcr89_vec3 up;
    gcr89_vec3 forward;
    unsigned long flags;
    const void *source;
} gcr89_request;
typedef struct gcr89_handle_s { int slot; long physics_id; void *object; } gcr89_handle;
typedef int (*gcr89_spawn_fn)(void*,const gcr89_request*,gcr89_handle*);
typedef void (*gcr89_stage_fn)(void*,const gcr89_request*,const gcr89_handle*);
typedef void (*gcr89_release_fn)(void*,const gcr89_handle*);
typedef struct gcr89_providers_s {
    void *user;
    gcr89_spawn_fn world_spawn;
    gcr89_stage_fn physics_spawn;
    gcr89_stage_fn render_spawn;
    gcr89_stage_fn audio_spawn;
    gcr89_stage_fn world_publish;
    gcr89_release_fn world_release;
} gcr89_providers;
typedef struct gcr89_runtime_s { gcr89_providers providers; unsigned long spawned,rejected; } gcr89_runtime;
void gcasingruntime89_init(gcr89_runtime*,const gcr89_providers*);
int gcasingruntime89_spawn(gcr89_runtime*,const gcr89_request*,gcr89_handle*);
#ifdef __cplusplus
}
#endif
#endif
