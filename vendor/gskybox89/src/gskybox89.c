#include "gskybox89.h"

#define GSKYBOX89_CUBE_VERTS_PER_FACE 4
#define GSKYBOX89_U0 0L
#define GSKYBOX89_V0 0L
#define GSKYBOX89_U1 GSKYBOX89_FX_ONE
#define GSKYBOX89_V1 GSKYBOX89_FX_ONE

static Gskybox89_Color gskybox89_make_color(int r, int g, int b, int a)
{
    Gskybox89_Color c;
    if (r < 0) r = 0;
    if (g < 0) g = 0;
    if (b < 0) b = 0;
    if (a < 0) a = 0;
    if (r > 255) r = 255;
    if (g > 255) g = 255;
    if (b > 255) b = 255;
    if (a > 255) a = 255;
    c.r = (unsigned char)r;
    c.g = (unsigned char)g;
    c.b = (unsigned char)b;
    c.a = (unsigned char)a;
    return c;
}

static Gskybox89_Color gskybox89_color_lerp(Gskybox89_Color a, Gskybox89_Color b, gskybox89_fx t)
{
    long it;
    long r;
    long g;
    long bl;
    long al;
    it = GSKYBOX89_FX_ONE - t;
    r = ((long)a.r * it + (long)b.r * t) >> GSKYBOX89_FX_SHIFT;
    g = ((long)a.g * it + (long)b.g * t) >> GSKYBOX89_FX_SHIFT;
    bl = ((long)a.b * it + (long)b.b * t) >> GSKYBOX89_FX_SHIFT;
    al = ((long)a.a * it + (long)b.a * t) >> GSKYBOX89_FX_SHIFT;
    return gskybox89_make_color((int)r, (int)g, (int)bl, (int)al);
}

gskybox89_fx gskybox89_fx_mul(gskybox89_fx a, gskybox89_fx b)
{
    return (a * b) >> GSKYBOX89_FX_SHIFT;
}

gskybox89_fx gskybox89_fx_lerp(gskybox89_fx a, gskybox89_fx b, gskybox89_fx t)
{
    return a + gskybox89_fx_mul(b - a, t);
}

void gskybox89_color_set(Gskybox89_Color *c, int r, int g, int b, int a)
{
    if (c != 0) {
        *c = gskybox89_make_color(r, g, b, a);
    }
}

void gskybox89_mat3_identity(Gskybox89_Mat3 *out_mat)
{
    int i;
    if (out_mat == 0) return;
    for (i = 0; i < 9; ++i) out_mat->m[i] = 0L;
    out_mat->m[0] = GSKYBOX89_FX_ONE;
    out_mat->m[4] = GSKYBOX89_FX_ONE;
    out_mat->m[8] = GSKYBOX89_FX_ONE;
}

/* 16-step fixed yaw table: angle index wraps by &15. */
static const gskybox89_fx gskybox89_cos16[16] = {
    4096L, 3784L, 2896L, 1567L, 0L, -1567L, -2896L, -3784L,
    -4096L, -3784L, -2896L, -1567L, 0L, 1567L, 2896L, 3784L
};

static const gskybox89_fx gskybox89_sin16[16] = {
    0L, 1567L, 2896L, 3784L, 4096L, 3784L, 2896L, 1567L,
    0L, -1567L, -2896L, -3784L, -4096L, -3784L, -2896L, -1567L
};

void gskybox89_mat3_yaw16(Gskybox89_Mat3 *out_mat, int yaw16)
{
    int y;
    gskybox89_fx c;
    gskybox89_fx s;
    if (out_mat == 0) return;
    y = yaw16 & 15;
    c = gskybox89_cos16[y];
    s = gskybox89_sin16[y];
    out_mat->m[0] = c;
    out_mat->m[1] = 0L;
    out_mat->m[2] = s;
    out_mat->m[3] = 0L;
    out_mat->m[4] = GSKYBOX89_FX_ONE;
    out_mat->m[5] = 0L;
    out_mat->m[6] = -s;
    out_mat->m[7] = 0L;
    out_mat->m[8] = c;
}

void gskybox89_default_config(Gskybox89_Config *cfg)
{
    if (cfg == 0) return;
    cfg->layer_mask = GSKYBOX89_LAYER_HYBRID;
    cfg->cube_use_6_faces = 1;
    cfg->cube_inside_out = 1;
    cfg->cube_fast_shared_vertices = 1;
    cfg->cube_face_mask = GSKYBOX89_CUBE_ALL_FACES;
    cfg->cube_uv_flip_u_mask = 0;
    cfg->cube_uv_flip_v_mask = 0;
    cfg->cube_uv_swap_mask = 0;
    cfg->screen_as_fullscreen_triangle = 1;
    cfg->screen_depth_func = GSKYBOX89_DEPTH_EQUAL;
    cfg->screen_top = gskybox89_make_color(80, 128, 190, 255);
    cfg->screen_bottom = gskybox89_make_color(170, 200, 230, 255);
    cfg->dome_segments = 12;
    cfg->dome_rings = 5;
    cfg->dome_blend_mode = GSKYBOX89_BLEND_ALPHA;
    cfg->dome_top = gskybox89_make_color(34, 84, 160, 160);
    cfg->dome_horizon = gskybox89_make_color(220, 230, 240, 96);
    cfg->render_after_opaque = 1;
    cfg->depth_write = 0;
    cfg->depth_func = GSKYBOX89_DEPTH_LEQUAL;
    cfg->cull_mode = GSKYBOX89_CULL_FRONT;
}


void gskybox89_cube_set_uv_xform(Gskybox89_Config *cfg, int flip_u_mask, int flip_v_mask, int swap_mask)
{
    if (cfg == 0) return;
    cfg->cube_uv_flip_u_mask = flip_u_mask;
    cfg->cube_uv_flip_v_mask = flip_v_mask;
    cfg->cube_uv_swap_mask = swap_mask;
}

void gskybox89_init(Gskybox89_Context *ctx, const Gskybox89_Config *cfg, const Gskybox89_Backend *backend)
{
    Gskybox89_Config local_cfg;
    if (ctx == 0) return;
    gskybox89_default_config(&local_cfg);
    if (cfg != 0) local_cfg = *cfg;
    if (local_cfg.dome_segments < 4) local_cfg.dome_segments = 4;
    if (local_cfg.dome_segments > GSKYBOX89_MAX_DOME_SEGMENTS) local_cfg.dome_segments = GSKYBOX89_MAX_DOME_SEGMENTS;
    if (local_cfg.dome_rings < 2) local_cfg.dome_rings = 2;
    if (local_cfg.dome_rings > GSKYBOX89_MAX_DOME_RINGS) local_cfg.dome_rings = GSKYBOX89_MAX_DOME_RINGS;
    ctx->cfg = local_cfg;
    if (backend != 0) {
        ctx->backend = *backend;
    } else {
        ctx->backend.user = 0;
        ctx->backend.begin_pass = 0;
        ctx->backend.end_pass = 0;
        ctx->backend.set_state = 0;
        ctx->backend.bind_face = 0;
        ctx->backend.emit_tri = 0;
    }
}

static void gskybox89_apply_rot(const Gskybox89_Mat3 *m, gskybox89_fx x, gskybox89_fx y, gskybox89_fx z, gskybox89_fx *rx, gskybox89_fx *ry, gskybox89_fx *rz)
{
    gskybox89_fx ox;
    gskybox89_fx oy;
    gskybox89_fx oz;
    ox = gskybox89_fx_mul(m->m[0], x) + gskybox89_fx_mul(m->m[1], y) + gskybox89_fx_mul(m->m[2], z);
    oy = gskybox89_fx_mul(m->m[3], x) + gskybox89_fx_mul(m->m[4], y) + gskybox89_fx_mul(m->m[5], z);
    oz = gskybox89_fx_mul(m->m[6], x) + gskybox89_fx_mul(m->m[7], y) + gskybox89_fx_mul(m->m[8], z);
    *rx = ox;
    *ry = oy;
    *rz = oz;
}

static void gskybox89_set_vertex(Gskybox89_Vertex *v, const Gskybox89_Mat3 *rot, gskybox89_fx x, gskybox89_fx y, gskybox89_fx z, gskybox89_fx u, gskybox89_fx vv, Gskybox89_Color color, int face, int layer)
{
    gskybox89_fx dx;
    gskybox89_fx dy;
    gskybox89_fx dz;
    gskybox89_apply_rot(rot, x, y, z, &dx, &dy, &dz);
    v->x = x;
    v->y = y;
    v->z = z;
    v->dir_x = dx;
    v->dir_y = dy;
    v->dir_z = dz;
    v->u = u;
    v->v = vv;
    v->color = color;
    v->face = face;
    v->layer = layer;
}

static void gskybox89_begin(const Gskybox89_Context *ctx, int pass, int depth_func, int cull_mode, int blend_mode)
{
    Gskybox89_State st;
    st.pass = pass;
    st.depth_test = (depth_func == GSKYBOX89_DEPTH_OFF) ? 0 : 1;
    st.depth_write = ctx->cfg.depth_write;
    st.depth_func = depth_func;
    st.cull_mode = cull_mode;
    st.blend_mode = blend_mode;
    st.lighting_enabled = 0;
    st.fog_enabled = 0;
    if (ctx->backend.begin_pass != 0) ctx->backend.begin_pass(ctx->backend.user, pass);
    if (ctx->backend.set_state != 0) ctx->backend.set_state(ctx->backend.user, &st);
}

static void gskybox89_end(const Gskybox89_Context *ctx, int pass)
{
    if (ctx->backend.end_pass != 0) ctx->backend.end_pass(ctx->backend.user, pass);
}

static void gskybox89_emit(const Gskybox89_Context *ctx, const Gskybox89_Vertex *a, const Gskybox89_Vertex *b, const Gskybox89_Vertex *c)
{
    if (ctx->backend.emit_tri != 0) ctx->backend.emit_tri(ctx->backend.user, a, b, c);
}

static void gskybox89_bind(const Gskybox89_Context *ctx, int layer, int face)
{
    if (ctx->backend.bind_face != 0) ctx->backend.bind_face(ctx->backend.user, layer, face);
}

static void gskybox89_render_screen(const Gskybox89_Context *ctx, const Gskybox89_Mat3 *rot)
{
    Gskybox89_Vertex a;
    Gskybox89_Vertex b;
    Gskybox89_Vertex c;
    gskybox89_begin(ctx, GSKYBOX89_PASS_SCREEN, ctx->cfg.screen_depth_func, GSKYBOX89_CULL_NONE, GSKYBOX89_BLEND_OFF);
    gskybox89_bind(ctx, GSKYBOX89_LAYER_SCREEN, GSKYBOX89_FACE_NONE);
    gskybox89_set_vertex(&a, rot, -GSKYBOX89_FX_ONE, -GSKYBOX89_FX_ONE, GSKYBOX89_FX_ONE, GSKYBOX89_U0, GSKYBOX89_V1, ctx->cfg.screen_bottom, GSKYBOX89_FACE_NONE, GSKYBOX89_LAYER_SCREEN);
    gskybox89_set_vertex(&b, rot, 3L * GSKYBOX89_FX_ONE, -GSKYBOX89_FX_ONE, GSKYBOX89_FX_ONE, GSKYBOX89_U1, GSKYBOX89_V1, ctx->cfg.screen_bottom, GSKYBOX89_FACE_NONE, GSKYBOX89_LAYER_SCREEN);
    gskybox89_set_vertex(&c, rot, -GSKYBOX89_FX_ONE, 3L * GSKYBOX89_FX_ONE, GSKYBOX89_FX_ONE, GSKYBOX89_U0, GSKYBOX89_V0, ctx->cfg.screen_top, GSKYBOX89_FACE_NONE, GSKYBOX89_LAYER_SCREEN);
    gskybox89_emit(ctx, &a, &b, &c);
    gskybox89_end(ctx, GSKYBOX89_PASS_SCREEN);
}

typedef struct Gskybox89_CubeCorner {
    gskybox89_fx x;
    gskybox89_fx y;
    gskybox89_fx z;
    gskybox89_fx dir_x;
    gskybox89_fx dir_y;
    gskybox89_fx dir_z;
} Gskybox89_CubeCorner;

/*
 * Shared-corner cube fast path.
 *
 * Old/simple path: build 4 logical vertices per face and rotate each one
 * through gskybox89_face_vertex(). That means 6 * 4 = 24 fixed-point
 * vertex rotations for the cube pass.
 *
 * Fast path: rotate the 8 physical cube corners once, then emit each face
 * as exactly 2 indexed triangles. UVs and face IDs are still per-face, so a
 * backend may bind six textures, one atlas, or a native cubemap.
 */
static const signed char gskybox89_cube_corner_signs[8][3] = {
    { -1, -1, -1 },
    {  1, -1, -1 },
    {  1,  1, -1 },
    { -1,  1, -1 },
    { -1, -1,  1 },
    {  1, -1,  1 },
    {  1,  1,  1 },
    { -1,  1,  1 }
};

/* Face-local corner order matches the original per-face generator. */
static const unsigned char gskybox89_cube_face_corner[6][4] = {
    { 2U, 6U, 5U, 1U }, /* +X */
    { 7U, 3U, 0U, 4U }, /* -X */
    { 3U, 2U, 6U, 7U }, /* +Y */
    { 4U, 5U, 1U, 0U }, /* -Y */
    { 6U, 7U, 4U, 5U }, /* +Z */
    { 3U, 2U, 1U, 0U }  /* -Z */
};

static const gskybox89_fx gskybox89_cube_u[4] = {
    GSKYBOX89_U0, GSKYBOX89_U1, GSKYBOX89_U1, GSKYBOX89_U0
};

static const gskybox89_fx gskybox89_cube_v[4] = {
    GSKYBOX89_V0, GSKYBOX89_V0, GSKYBOX89_V1, GSKYBOX89_V1
};

static void gskybox89_cube_build_corners(const Gskybox89_Mat3 *rot, Gskybox89_CubeCorner *out_corners)
{
    int i;
    gskybox89_fx x;
    gskybox89_fx y;
    gskybox89_fx z;
    for (i = 0; i < 8; ++i) {
        x = (gskybox89_cube_corner_signs[i][0] < 0) ? GSKYBOX89_FX_NEG_ONE : GSKYBOX89_FX_ONE;
        y = (gskybox89_cube_corner_signs[i][1] < 0) ? GSKYBOX89_FX_NEG_ONE : GSKYBOX89_FX_ONE;
        z = (gskybox89_cube_corner_signs[i][2] < 0) ? GSKYBOX89_FX_NEG_ONE : GSKYBOX89_FX_ONE;
        out_corners[i].x = x;
        out_corners[i].y = y;
        out_corners[i].z = z;
        gskybox89_apply_rot(rot, x, y, z, &out_corners[i].dir_x, &out_corners[i].dir_y, &out_corners[i].dir_z);
    }
}

static void gskybox89_cube_make_vertex(const Gskybox89_CubeCorner *corners, int face, int local_corner, Gskybox89_Vertex *out_v)
{
    const Gskybox89_CubeCorner *c;
    Gskybox89_Color white;
    int ci;
    if (face < 0) face = 0;
    if (face >= 6) face = 5;
    if (local_corner < 0) local_corner = 0;
    if (local_corner > 3) local_corner = 3;
    ci = (int)gskybox89_cube_face_corner[face][local_corner];
    c = &corners[ci];
    white = gskybox89_make_color(255, 255, 255, 255);
    out_v->x = c->x;
    out_v->y = c->y;
    out_v->z = c->z;
    out_v->dir_x = c->dir_x;
    out_v->dir_y = c->dir_y;
    out_v->dir_z = c->dir_z;
    out_v->u = gskybox89_cube_u[local_corner];
    out_v->v = gskybox89_cube_v[local_corner];
    out_v->color = white;
    out_v->face = face;
    out_v->layer = GSKYBOX89_LAYER_CUBE6;
}

static void gskybox89_face_vertex(const Gskybox89_Mat3 *rot, int face, int corner, Gskybox89_Vertex *out_v)
{
    gskybox89_fx x;
    gskybox89_fx y;
    gskybox89_fx z;
    gskybox89_fx u;
    gskybox89_fx v;
    Gskybox89_Color white;
    white = gskybox89_make_color(255, 255, 255, 255);
    x = 0L;
    y = 0L;
    z = 0L;
    u = (corner == 1 || corner == 2) ? GSKYBOX89_U1 : GSKYBOX89_U0;
    v = (corner == 2 || corner == 3) ? GSKYBOX89_V1 : GSKYBOX89_V0;

    if (face == GSKYBOX89_FACE_POS_X) {
        x = GSKYBOX89_FX_ONE;
        y = (corner == 0 || corner == 1) ? GSKYBOX89_FX_ONE : GSKYBOX89_FX_NEG_ONE;
        z = (corner == 0 || corner == 3) ? GSKYBOX89_FX_NEG_ONE : GSKYBOX89_FX_ONE;
    } else if (face == GSKYBOX89_FACE_NEG_X) {
        x = GSKYBOX89_FX_NEG_ONE;
        y = (corner == 0 || corner == 1) ? GSKYBOX89_FX_ONE : GSKYBOX89_FX_NEG_ONE;
        z = (corner == 0 || corner == 3) ? GSKYBOX89_FX_ONE : GSKYBOX89_FX_NEG_ONE;
    } else if (face == GSKYBOX89_FACE_POS_Y) {
        y = GSKYBOX89_FX_ONE;
        x = (corner == 0 || corner == 3) ? GSKYBOX89_FX_NEG_ONE : GSKYBOX89_FX_ONE;
        z = (corner == 0 || corner == 1) ? GSKYBOX89_FX_NEG_ONE : GSKYBOX89_FX_ONE;
    } else if (face == GSKYBOX89_FACE_NEG_Y) {
        y = GSKYBOX89_FX_NEG_ONE;
        x = (corner == 0 || corner == 3) ? GSKYBOX89_FX_NEG_ONE : GSKYBOX89_FX_ONE;
        z = (corner == 0 || corner == 1) ? GSKYBOX89_FX_ONE : GSKYBOX89_FX_NEG_ONE;
    } else if (face == GSKYBOX89_FACE_POS_Z) {
        z = GSKYBOX89_FX_ONE;
        x = (corner == 0 || corner == 3) ? GSKYBOX89_FX_ONE : GSKYBOX89_FX_NEG_ONE;
        y = (corner == 0 || corner == 1) ? GSKYBOX89_FX_ONE : GSKYBOX89_FX_NEG_ONE;
    } else {
        z = GSKYBOX89_FX_NEG_ONE;
        x = (corner == 0 || corner == 3) ? GSKYBOX89_FX_NEG_ONE : GSKYBOX89_FX_ONE;
        y = (corner == 0 || corner == 1) ? GSKYBOX89_FX_ONE : GSKYBOX89_FX_NEG_ONE;
    }

    gskybox89_set_vertex(out_v, rot, x, y, z, u, v, white, face, GSKYBOX89_LAYER_CUBE6);
}


static void gskybox89_apply_cube_uv_xform(const Gskybox89_Context *ctx, int face, Gskybox89_Vertex *v)
{
    gskybox89_fx tmp;
    int bit;
    if (ctx == 0 || v == 0) return;
    if (face < 0 || face >= GSKYBOX89_CUBE_FACE_COUNT) return;
    bit = 1 << face;
    if ((ctx->cfg.cube_uv_swap_mask & bit) != 0) {
        tmp = v->u;
        v->u = v->v;
        v->v = tmp;
    }
    if ((ctx->cfg.cube_uv_flip_u_mask & bit) != 0) v->u = GSKYBOX89_FX_ONE - v->u;
    if ((ctx->cfg.cube_uv_flip_v_mask & bit) != 0) v->v = GSKYBOX89_FX_ONE - v->v;
}

static void gskybox89_apply_cube_uv_xform4(const Gskybox89_Context *ctx, int face, Gskybox89_Vertex *v0, Gskybox89_Vertex *v1, Gskybox89_Vertex *v2, Gskybox89_Vertex *v3)
{
    gskybox89_apply_cube_uv_xform(ctx, face, v0);
    gskybox89_apply_cube_uv_xform(ctx, face, v1);
    gskybox89_apply_cube_uv_xform(ctx, face, v2);
    gskybox89_apply_cube_uv_xform(ctx, face, v3);
}

static void gskybox89_emit_cube_face(const Gskybox89_Context *ctx, const Gskybox89_Vertex *v0, const Gskybox89_Vertex *v1, const Gskybox89_Vertex *v2, const Gskybox89_Vertex *v3)
{
    if (ctx->cfg.cube_inside_out != 0) {
        gskybox89_emit(ctx, v0, v2, v1);
        gskybox89_emit(ctx, v0, v3, v2);
    } else {
        gskybox89_emit(ctx, v0, v1, v2);
        gskybox89_emit(ctx, v0, v2, v3);
    }
}

static void gskybox89_render_cube6_fast(const Gskybox89_Context *ctx, const Gskybox89_Mat3 *rot)
{
    int face;
    int mask;
    Gskybox89_CubeCorner corners[8];
    Gskybox89_Vertex v0;
    Gskybox89_Vertex v1;
    Gskybox89_Vertex v2;
    Gskybox89_Vertex v3;
    mask = ctx->cfg.cube_face_mask;
    if (mask == 0) return;
    gskybox89_cube_build_corners(rot, corners);
    for (face = 0; face < 6; ++face) {
        if ((mask & (1 << face)) == 0) continue;
        gskybox89_bind(ctx, GSKYBOX89_LAYER_CUBE6, face);
        gskybox89_cube_make_vertex(corners, face, 0, &v0);
        gskybox89_cube_make_vertex(corners, face, 1, &v1);
        gskybox89_cube_make_vertex(corners, face, 2, &v2);
        gskybox89_cube_make_vertex(corners, face, 3, &v3);
        gskybox89_apply_cube_uv_xform4(ctx, face, &v0, &v1, &v2, &v3);
        gskybox89_emit_cube_face(ctx, &v0, &v1, &v2, &v3);
    }
}

static void gskybox89_render_cube6_safe(const Gskybox89_Context *ctx, const Gskybox89_Mat3 *rot)
{
    int face;
    int mask;
    Gskybox89_Vertex v0;
    Gskybox89_Vertex v1;
    Gskybox89_Vertex v2;
    Gskybox89_Vertex v3;
    mask = ctx->cfg.cube_face_mask;
    if (mask == 0) return;
    for (face = 0; face < 6; ++face) {
        if ((mask & (1 << face)) == 0) continue;
        gskybox89_bind(ctx, GSKYBOX89_LAYER_CUBE6, face);
        gskybox89_face_vertex(rot, face, 0, &v0);
        gskybox89_face_vertex(rot, face, 1, &v1);
        gskybox89_face_vertex(rot, face, 2, &v2);
        gskybox89_face_vertex(rot, face, 3, &v3);
        gskybox89_apply_cube_uv_xform4(ctx, face, &v0, &v1, &v2, &v3);
        gskybox89_emit_cube_face(ctx, &v0, &v1, &v2, &v3);
    }
}

static void gskybox89_render_cube6(const Gskybox89_Context *ctx, const Gskybox89_Mat3 *rot)
{
    gskybox89_begin(ctx, GSKYBOX89_PASS_CUBE6, ctx->cfg.depth_func, ctx->cfg.cull_mode, GSKYBOX89_BLEND_OFF);
    if (ctx->cfg.cube_fast_shared_vertices != 0) {
        gskybox89_render_cube6_fast(ctx, rot);
    } else {
        gskybox89_render_cube6_safe(ctx, rot);
    }
    gskybox89_end(ctx, GSKYBOX89_PASS_CUBE6);
}

static const gskybox89_fx gskybox89_dome_ring_y[5] = {
    4096L, 3784L, 2896L, 1567L, 0L
};

static const gskybox89_fx gskybox89_dome_ring_r[5] = {
    0L, 1567L, 2896L, 3784L, 4096L
};

static void gskybox89_dome_vertex(const Gskybox89_Context *ctx, const Gskybox89_Mat3 *rot, int ring, int seg, Gskybox89_Vertex *out_v)
{
    int s;
    gskybox89_fx x;
    gskybox89_fx y;
    gskybox89_fx z;
    gskybox89_fx rr;
    gskybox89_fx u;
    gskybox89_fx t;
    Gskybox89_Color c;
    s = seg & 15;
    if (ring < 0) ring = 0;
    if (ring > 4) ring = 4;
    rr = gskybox89_dome_ring_r[ring];
    x = gskybox89_fx_mul(rr, gskybox89_cos16[s]);
    z = gskybox89_fx_mul(rr, gskybox89_sin16[s]);
    y = gskybox89_dome_ring_y[ring];
    u = ((long)s * GSKYBOX89_FX_ONE) / 16L;
    if (ctx->cfg.dome_rings <= 1) {
        t = 0L;
    } else {
        t = ((long)ring * GSKYBOX89_FX_ONE) / (long)(ctx->cfg.dome_rings - 1);
    }
    c = gskybox89_color_lerp(ctx->cfg.dome_top, ctx->cfg.dome_horizon, t);
    gskybox89_set_vertex(out_v, rot, x, y, z, u, t, c, GSKYBOX89_FACE_NONE, GSKYBOX89_LAYER_DOME);
}

static void gskybox89_render_dome(const Gskybox89_Context *ctx, const Gskybox89_Mat3 *rot)
{
    int ring;
    int seg;
    int seg_next;
    Gskybox89_Vertex a;
    Gskybox89_Vertex b;
    Gskybox89_Vertex c;
    Gskybox89_Vertex d;
    gskybox89_begin(ctx, GSKYBOX89_PASS_DOME, ctx->cfg.depth_func, GSKYBOX89_CULL_NONE, ctx->cfg.dome_blend_mode);
    gskybox89_bind(ctx, GSKYBOX89_LAYER_DOME, GSKYBOX89_FACE_NONE);
    for (ring = 0; ring < ctx->cfg.dome_rings - 1; ++ring) {
        for (seg = 0; seg < ctx->cfg.dome_segments; ++seg) {
            seg_next = (seg + 1) % ctx->cfg.dome_segments;
            gskybox89_dome_vertex(ctx, rot, ring, seg, &a);
            gskybox89_dome_vertex(ctx, rot, ring + 1, seg, &b);
            gskybox89_dome_vertex(ctx, rot, ring + 1, seg_next, &c);
            gskybox89_dome_vertex(ctx, rot, ring, seg_next, &d);
            if (ring == 0) {
                gskybox89_emit(ctx, &a, &b, &c);
            } else {
                gskybox89_emit(ctx, &a, &b, &c);
                gskybox89_emit(ctx, &a, &c, &d);
            }
        }
    }
    gskybox89_end(ctx, GSKYBOX89_PASS_DOME);
}

int gskybox89_render(const Gskybox89_Context *ctx, const Gskybox89_Mat3 *camera_rotation_only)
{
    Gskybox89_Mat3 identity;
    const Gskybox89_Mat3 *rot;
    if (ctx == 0) return GSKYBOX89_ERR_NULL;
    if (ctx->backend.emit_tri == 0) return GSKYBOX89_ERR_NULL;
    if (camera_rotation_only == 0) {
        gskybox89_mat3_identity(&identity);
        rot = &identity;
    } else {
        rot = camera_rotation_only;
    }
    if ((ctx->cfg.layer_mask & GSKYBOX89_LAYER_SCREEN) != 0) {
        gskybox89_render_screen(ctx, rot);
    }
    if ((ctx->cfg.layer_mask & GSKYBOX89_LAYER_CUBE6) != 0) {
        gskybox89_render_cube6(ctx, rot);
    }
    if ((ctx->cfg.layer_mask & GSKYBOX89_LAYER_DOME) != 0) {
        gskybox89_render_dome(ctx, rot);
    }
    return GSKYBOX89_OK;
}

const char *gskybox89_face_name(int face)
{
    if (face == GSKYBOX89_FACE_POS_X) return "+X";
    if (face == GSKYBOX89_FACE_NEG_X) return "-X";
    if (face == GSKYBOX89_FACE_POS_Y) return "+Y";
    if (face == GSKYBOX89_FACE_NEG_Y) return "-Y";
    if (face == GSKYBOX89_FACE_POS_Z) return "+Z";
    if (face == GSKYBOX89_FACE_NEG_Z) return "-Z";
    return "NONE";
}

const char *gskybox89_pass_name(int pass)
{
    if (pass == GSKYBOX89_PASS_SCREEN) return "screen";
    if (pass == GSKYBOX89_PASS_CUBE6) return "cube6";
    if (pass == GSKYBOX89_PASS_DOME) return "dome";
    return "unknown";
}
