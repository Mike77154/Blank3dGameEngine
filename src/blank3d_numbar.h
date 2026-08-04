#ifndef BLANK3D_NUMBAR_H
#define BLANK3D_NUMBAR_H

#include "blank3d_bighud.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Fixed pools: no heap, no per-frame file access. */
#define B3D_NUMBAR_MAX_INSTANCES 32
#define B3D_NUMBAR_PATH_CAP 192
#define B3D_NUMBAR_NAME_CAP 48
#define B3D_NUMBAR_BIND_CAP 64
#define B3D_NUMBAR_STATUS_CAP 256

struct Blank3DHudTag;

typedef long (*Blank3DNumbarResolveFn)(void *user,
                                        unsigned long entity_id,
                                        void *native_entity,
                                        const char *binding,
                                        long fallback,
                                        int *resolved);

typedef struct Blank3DNumbarOrchestratorTag {
    int enabled;
    int anchor;
    int x;
    int y;
    int scale_x_q8;
    int scale_y_q8;
    int z;
    int canvas_width;
    int canvas_height;
    char name[B3D_NUMBAR_NAME_CAP];
    char preset_path[B3D_NUMBAR_PATH_CAP];
    char value_bind[B3D_NUMBAR_BIND_CAP];
    char min_bind[B3D_NUMBAR_BIND_CAP];
    char max_bind[B3D_NUMBAR_BIND_CAP];
    char overlay_bind[B3D_NUMBAR_BIND_CAP];
    char overlay_min_bind[B3D_NUMBAR_BIND_CAP];
    char overlay_max_bind[B3D_NUMBAR_BIND_CAP];
    char mid_bind[B3D_NUMBAR_BIND_CAP];
    char segments_bind[B3D_NUMBAR_BIND_CAP];
    char state_bind[B3D_NUMBAR_BIND_CAP];
    char phase_bind[B3D_NUMBAR_BIND_CAP];
    char units_bind[B3D_NUMBAR_BIND_CAP];
    char layer_count_bind[B3D_NUMBAR_BIND_CAP];
    char layer_size_bind[B3D_NUMBAR_BIND_CAP];
    char value2_bind[B3D_NUMBAR_BIND_CAP];
    char visible_bind[B3D_NUMBAR_BIND_CAP];
} Blank3DNumbarOrchestrator;

typedef struct Blank3DNumbarInstanceTag {
    int alive;
    int requested;
    unsigned long entity_id;
    void *native_entity;
    char orchestrator_path[B3D_NUMBAR_PATH_CAP];
    Blank3DNumbarOrchestrator orchestrator;
    Blank3DBigHud layout;
} Blank3DNumbarInstance;

typedef struct Blank3DNumbarSystemTag {
    Blank3DNumbarResolveFn resolve;
    void *resolve_user;
    Blank3DNumbarInstance instances[B3D_NUMBAR_MAX_INSTANCES];
    int instance_count;
    char status[B3D_NUMBAR_STATUS_CAP];
} Blank3DNumbarSystem;

void blank3d_numbar_init(Blank3DNumbarSystem *system,
                          Blank3DNumbarResolveFn resolve,
                          void *resolve_user);
void blank3d_numbar_begin_frame(Blank3DNumbarSystem *system);

/* Loads an INI once and requests every [numbar name] section for this frame. */
int blank3d_numbar_request(Blank3DNumbarSystem *system,
                           unsigned long entity_id,
                           void *native_entity,
                           const char *orchestrator_path);

void blank3d_numbar_draw(Blank3DNumbarSystem *system,
                          struct Blank3DHudTag *hud,
                          unsigned int frame_ms);
const char *blank3d_numbar_status(const Blank3DNumbarSystem *system);

#ifdef __cplusplus
}
#endif

#endif
