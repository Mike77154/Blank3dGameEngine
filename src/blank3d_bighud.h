#ifndef BLANK3D_BIGHUD_H
#define BLANK3D_BIGHUD_H

#include "bvh.h"
#include "gbar89.h"
#include "blank3d_ecg_vitals.h"

#ifdef __cplusplus
extern "C" {
#endif

#define B3D_BIGHUD_MAX_NODES 32
#define B3D_BIGHUD_NAME_CAP 48
#define B3D_BIGHUD_BIND_CAP 64
#define B3D_BIGHUD_TEXT_CAP 48
#define B3D_BIGHUD_PATH_CAP 192
#define B3D_BIGHUD_ERROR_CAP 256

#define B3D_BIGHUD_NODE_NONE 0
#define B3D_BIGHUD_NODE_BAR 1
#define B3D_BIGHUD_NODE_ECG 2
#define B3D_BIGHUD_NODE_COUNTER 3

#define B3D_BIGHUD_COUNTER_VALUE 0
#define B3D_BIGHUD_COUNTER_PAIR 1

#define B3D_BIGHUD_UNIT_RENDERER_DEFAULT 0
#define B3D_BIGHUD_UNIT_RENDERER_GPROJ_AMMO 1

#define B3D_BIGHUD_ANCHOR_NONE BVH_ANCHOR_NONE
#define B3D_BIGHUD_ANCHOR_TOP_LEFT BVH_ANCHOR_TOP_LEFT
#define B3D_BIGHUD_ANCHOR_TOP_CENTER BVH_ANCHOR_TOP_CENTER
#define B3D_BIGHUD_ANCHOR_TOP_RIGHT BVH_ANCHOR_TOP_RIGHT
#define B3D_BIGHUD_ANCHOR_CENTER BVH_ANCHOR_CENTER
#define B3D_BIGHUD_ANCHOR_BOTTOM_LEFT BVH_ANCHOR_BOTTOM_LEFT
#define B3D_BIGHUD_ANCHOR_BOTTOM_CENTER BVH_ANCHOR_BOTTOM_CENTER
#define B3D_BIGHUD_ANCHOR_BOTTOM_RIGHT BVH_ANCHOR_BOTTOM_RIGHT

typedef struct Blank3DBigHudTelemetryTag {
    long player_health;
    long player_health_max;
    long weapon_loaded;
    long weapon_capacity;
    long weapon_reserve;
    long gameplay_threat;
    long ecg_bpm;
    long damage_flash_ms;
    long first_person;
    long muzzle_flash;

    /* Generic channels used by GFO draw_numbar orchestrators. */
    long numbar_value;
    long numbar_min;
    long numbar_max;
    long numbar_overlay;
    long numbar_overlay_min;
    long numbar_overlay_max;
    long numbar_mid;
    long numbar_segments;
    long numbar_state;
    long numbar_phase;
    long numbar_units;
    long numbar_layer_count;
    long numbar_layer_size;
    long numbar_value2;
} Blank3DBigHudTelemetry;

typedef struct Blank3DBigHudNodeTag {
    int type;
    int enabled;
    int anchor;
    int offset_x;
    int offset_y;
    int width;
    int height;
    int has_pos;
    int has_size;
    int z;

    char name[B3D_BIGHUD_NAME_CAP];
    char bind[B3D_BIGHUD_BIND_CAP];
    char min_bind[B3D_BIGHUD_BIND_CAP];
    char max_bind[B3D_BIGHUD_BIND_CAP];
    char overlay_bind[B3D_BIGHUD_BIND_CAP];
    char overlay_min_bind[B3D_BIGHUD_BIND_CAP];
    char overlay_max_bind[B3D_BIGHUD_BIND_CAP];
    char mid_bind[B3D_BIGHUD_BIND_CAP];
    char segments_bind[B3D_BIGHUD_BIND_CAP];
    char state_bind[B3D_BIGHUD_BIND_CAP];
    char radial_phase_bind[B3D_BIGHUD_BIND_CAP];
    char unit_count_bind[B3D_BIGHUD_BIND_CAP];
    char layer_count_bind[B3D_BIGHUD_BIND_CAP];
    char layer_size_bind[B3D_BIGHUD_BIND_CAP];
    char bind2[B3D_BIGHUD_BIND_CAP];

    GBar89_Meter meter;
    GBar89_VectorPoint unit_points[GBAR89_MAX_VECTOR_POINTS];
    int unit_point_count;
    int unit_renderer_kind;
    int active_reload_visualizer;
    int radial_phase_speed_deg_per_sec;
    long radial_phase_accum_q16;

    int ecg_scale_x_q8;
    int ecg_scale_y_q8;
    unsigned int ecg_content_alpha;
    unsigned int ecg_background_alpha;
    unsigned int ecg_clear_alpha;

    int counter_mode;
    int counter_scale;
    int counter_pad;
    int counter_pad2;
    int counter_gap;
    int counter_shadow;
    int counter_background;
    int counter_border;
    unsigned long counter_color;
    unsigned long counter_shadow_color;
    unsigned long counter_background_color;
    unsigned long counter_border_color;
    char counter_label[B3D_BIGHUD_TEXT_CAP];
    char counter_prefix[B3D_BIGHUD_TEXT_CAP];
    char counter_separator[B3D_BIGHUD_TEXT_CAP];
    char counter_suffix[B3D_BIGHUD_TEXT_CAP];
} Blank3DBigHudNode;

typedef struct Blank3DBigHudTag {
    Blank3DBigHudNode nodes[B3D_BIGHUD_MAX_NODES];
    int node_count;
    int enabled;
    int canvas_width;
    int canvas_height;
    int loaded_from_file;
    char source_path[B3D_BIGHUD_PATH_CAP];
    char last_error[B3D_BIGHUD_ERROR_CAP];
} Blank3DBigHud;

void blank3d_bighud_init(Blank3DBigHud *hud);
int blank3d_bighud_load(Blank3DBigHud *hud,
                        Blank3DEcgVitals *ecg,
                        const char *path);
void blank3d_bighud_install_fallback(Blank3DBigHud *hud,
                                     Blank3DEcgVitals *ecg);
void blank3d_bighud_sort(Blank3DBigHud *hud);
long blank3d_bighud_resolve_binding(const Blank3DBigHudTelemetry *telemetry,
                                    const char *name,
                                    long fallback);
void blank3d_bighud_resolve_rect(const Blank3DBigHudNode *node,
                                 int screen_w,
                                 int screen_h,
                                 int default_w,
                                 int default_h,
                                 int *out_x,
                                 int *out_y,
                                 int *out_w,
                                 int *out_h);
void blank3d_bighud_update_bar(Blank3DBigHudNode *node,
                               const Blank3DBigHudTelemetry *telemetry,
                               unsigned int frame_ms);
const char *blank3d_bighud_error(const Blank3DBigHud *hud);

#ifdef __cplusplus
}
#endif

#endif
