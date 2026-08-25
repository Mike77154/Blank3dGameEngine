#ifndef GSKYBOX89_H
#define GSKYBOX89_H

/*
 * gskybox89 - C89 fixed-point sky background helper.
 * Public domain / CC0-style. See LICENSE.
 *
 * Design goals:
 * - C89 strict
 * - no heap, no malloc/free/realloc
 * - no float/double
 * - fixed-point math only
 * - backend agnostic: renderer receives triangles through callbacks
 */

#ifdef __cplusplus
extern "C" {
#endif

#define GSKYBOX89_VERSION_MAJOR 1
#define GSKYBOX89_VERSION_MINOR 2
#define GSKYBOX89_VERSION_PATCH 0

#define GSKYBOX89_FX_SHIFT 12
#define GSKYBOX89_FX_ONE   4096L
#define GSKYBOX89_FX_HALF  2048L
#define GSKYBOX89_FX_NEG_ONE (-4096L)

#define GSKYBOX89_FACE_POS_X 0
#define GSKYBOX89_FACE_NEG_X 1
#define GSKYBOX89_FACE_POS_Y 2
#define GSKYBOX89_FACE_NEG_Y 3
#define GSKYBOX89_FACE_POS_Z 4
#define GSKYBOX89_FACE_NEG_Z 5
#define GSKYBOX89_FACE_NONE  6

#define GSKYBOX89_CUBE_FACE_COUNT 6
#define GSKYBOX89_CUBE_TRIS_PER_FACE 2
#define GSKYBOX89_CUBE_TRI_COUNT 12
#define GSKYBOX89_CUBE_ALL_FACES 63

#define GSKYBOX89_LAYER_SCREEN 1
#define GSKYBOX89_LAYER_CUBE6  2
#define GSKYBOX89_LAYER_DOME   4
#define GSKYBOX89_LAYER_HYBRID (GSKYBOX89_LAYER_SCREEN | GSKYBOX89_LAYER_CUBE6 | GSKYBOX89_LAYER_DOME)

#define GSKYBOX89_PASS_SCREEN 0
#define GSKYBOX89_PASS_CUBE6  1
#define GSKYBOX89_PASS_DOME   2

#define GSKYBOX89_DEPTH_ALWAYS 0
#define GSKYBOX89_DEPTH_LEQUAL 1
#define GSKYBOX89_DEPTH_EQUAL  2
#define GSKYBOX89_DEPTH_OFF    3

#define GSKYBOX89_CULL_NONE  0
#define GSKYBOX89_CULL_FRONT 1
#define GSKYBOX89_CULL_BACK  2

#define GSKYBOX89_BLEND_OFF   0
#define GSKYBOX89_BLEND_ALPHA 1
#define GSKYBOX89_BLEND_ADD   2

#define GSKYBOX89_OK 0
#define GSKYBOX89_ERR_NULL -1
#define GSKYBOX89_ERR_RANGE -2

#ifndef GSKYBOX89_MAX_DOME_SEGMENTS
#define GSKYBOX89_MAX_DOME_SEGMENTS 16
#endif

#ifndef GSKYBOX89_MAX_DOME_RINGS
#define GSKYBOX89_MAX_DOME_RINGS 5
#endif

typedef long gskybox89_fx;

typedef struct Gskybox89_Color {
    unsigned char r;
    unsigned char g;
    unsigned char b;
    unsigned char a;
} Gskybox89_Color;

typedef struct Gskybox89_Mat3 {
    gskybox89_fx m[9];
} Gskybox89_Mat3;

typedef struct Gskybox89_Vertex {
    gskybox89_fx x;
    gskybox89_fx y;
    gskybox89_fx z;
    gskybox89_fx dir_x;
    gskybox89_fx dir_y;
    gskybox89_fx dir_z;
    gskybox89_fx u;
    gskybox89_fx v;
    Gskybox89_Color color;
    int face;
    int layer;
} Gskybox89_Vertex;

typedef struct Gskybox89_State {
    int pass;
    int depth_test;
    int depth_write;
    int depth_func;
    int cull_mode;
    int blend_mode;
    int lighting_enabled;
    int fog_enabled;
} Gskybox89_State;

typedef void (*Gskybox89_BeginPassFn)(void *user, int pass);
typedef void (*Gskybox89_EndPassFn)(void *user, int pass);
typedef void (*Gskybox89_SetStateFn)(void *user, const Gskybox89_State *state);
typedef void (*Gskybox89_BindFaceFn)(void *user, int layer, int face);
typedef void (*Gskybox89_EmitTriFn)(void *user, const Gskybox89_Vertex *a, const Gskybox89_Vertex *b, const Gskybox89_Vertex *c);

typedef struct Gskybox89_Backend {
    void *user;
    Gskybox89_BeginPassFn begin_pass;
    Gskybox89_EndPassFn end_pass;
    Gskybox89_SetStateFn set_state;
    Gskybox89_BindFaceFn bind_face;
    Gskybox89_EmitTriFn emit_tri;
} Gskybox89_Backend;

typedef struct Gskybox89_Config {
    int layer_mask;

    /* Cube/6-face layer. */
    int cube_use_6_faces;
    int cube_inside_out;
    int cube_fast_shared_vertices;
    int cube_face_mask;
    int cube_uv_flip_u_mask;
    int cube_uv_flip_v_mask;
    int cube_uv_swap_mask;

    /* Screen-space layer. */
    int screen_as_fullscreen_triangle;
    int screen_depth_func;
    Gskybox89_Color screen_top;
    Gskybox89_Color screen_bottom;

    /* Dome-lite layer. max: 16 segments, 5 rings. */
    int dome_segments;
    int dome_rings;
    int dome_blend_mode;
    Gskybox89_Color dome_top;
    Gskybox89_Color dome_horizon;

    /* Shared render-state hints. */
    int render_after_opaque;
    int depth_write;
    int depth_func;
    int cull_mode;
} Gskybox89_Config;

typedef struct Gskybox89_Context {
    Gskybox89_Config cfg;
    Gskybox89_Backend backend;
} Gskybox89_Context;

gskybox89_fx gskybox89_fx_mul(gskybox89_fx a, gskybox89_fx b);
gskybox89_fx gskybox89_fx_lerp(gskybox89_fx a, gskybox89_fx b, gskybox89_fx t);

void gskybox89_color_set(Gskybox89_Color *c, int r, int g, int b, int a);
void gskybox89_mat3_identity(Gskybox89_Mat3 *out_mat);
void gskybox89_mat3_yaw16(Gskybox89_Mat3 *out_mat, int yaw16);
void gskybox89_default_config(Gskybox89_Config *cfg);
void gskybox89_cube_set_uv_xform(Gskybox89_Config *cfg, int flip_u_mask, int flip_v_mask, int swap_mask);
void gskybox89_init(Gskybox89_Context *ctx, const Gskybox89_Config *cfg, const Gskybox89_Backend *backend);
int gskybox89_render(const Gskybox89_Context *ctx, const Gskybox89_Mat3 *camera_rotation_only);

const char *gskybox89_face_name(int face);
const char *gskybox89_pass_name(int pass);

#ifdef __cplusplus
}
#endif

#endif
