#include <stdio.h>
#include "gskybox89.h"

#define PREVIEW_W 320
#define PREVIEW_H 200
#define PREVIEW_NEAR 256L
#define PREVIEW_FOCAL 132L
#define PREVIEW_MAX_CLIP 8

static unsigned char g_pixels[PREVIEW_W * PREVIEW_H * 3];

typedef struct PreviewV {
    long x;
    long y;
    long z;
    int sx;
    int sy;
    int r;
    int g;
    int b;
    int a;
    int layer;
    int face;
} PreviewV;

typedef struct PreviewState {
    int pass;
    int blend_mode;
    int bound_layer;
    int bound_face;
    int tri_total;
    int tri_screen;
    int tri_cube;
    int tri_dome;
} PreviewState;

static int clamp_i(int v, int lo, int hi)
{
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

static void set_pixel_alpha(int x, int y, int r, int g, int b, int a)
{
    long idx;
    int dr;
    int dg;
    int db;
    int ia;
    if (x < 0 || x >= PREVIEW_W || y < 0 || y >= PREVIEW_H) return;
    a = clamp_i(a, 0, 255);
    r = clamp_i(r, 0, 255);
    g = clamp_i(g, 0, 255);
    b = clamp_i(b, 0, 255);
    idx = ((long)y * (long)PREVIEW_W + (long)x) * 3L;
    if (a >= 255) {
        g_pixels[idx + 0] = (unsigned char)r;
        g_pixels[idx + 1] = (unsigned char)g;
        g_pixels[idx + 2] = (unsigned char)b;
    } else if (a > 0) {
        ia = 255 - a;
        dr = (int)g_pixels[idx + 0];
        dg = (int)g_pixels[idx + 1];
        db = (int)g_pixels[idx + 2];
        g_pixels[idx + 0] = (unsigned char)((r * a + dr * ia) / 255);
        g_pixels[idx + 1] = (unsigned char)((g * a + dg * ia) / 255);
        g_pixels[idx + 2] = (unsigned char)((b * a + db * ia) / 255);
    }
}

static long edge_fn(int ax, int ay, int bx, int by, int cx, int cy)
{
    return (long)(cx - ax) * (long)(by - ay) - (long)(cy - ay) * (long)(bx - ax);
}

static void draw_line(int x0, int y0, int x1, int y1, int r, int g, int b, int a)
{
    int dx;
    int sx;
    int dy;
    int sy;
    int err;
    int e2;
    dx = x1 > x0 ? x1 - x0 : x0 - x1;
    sx = x0 < x1 ? 1 : -1;
    dy = y1 > y0 ? y0 - y1 : y1 - y0;
    sy = y0 < y1 ? 1 : -1;
    err = dx + dy;
    while (1) {
        set_pixel_alpha(x0, y0, r, g, b, a);
        if (x0 == x1 && y0 == y1) break;
        e2 = 2 * err;
        if (e2 >= dy) {
            err += dy;
            x0 += sx;
        }
        if (e2 <= dx) {
            err += dx;
            y0 += sy;
        }
    }
}

static int abs_i(int v)
{
    return v < 0 ? -v : v;
}

static void cube_pixel_shader(long ix, long iy, long iz, int face, int *r, int *g, int *b, int *a)
{
    int t;
    int cr;
    int cg;
    int cb;
    int band;
    long dx;
    long dy;
    long dist2;
    (void)face;
    t = (int)(((iy + GSKYBOX89_FX_ONE) * 255L) / (2L * GSKYBOX89_FX_ONE));
    t = clamp_i(t, 0, 255);
    cr = (190 * (255 - t) + 42 * t) / 255;
    cg = (213 * (255 - t) + 94 * t) / 255;
    cb = (238 * (255 - t) + 184 * t) / 255;
    if (iy > -500L && iy < 1400L) {
        band = abs_i((int)((ix / 256L) + (iz / 384L) + (iy / 192L))) % 9;
        if (band == 1 || band == 2) {
            cr = (cr * 3 + 244) / 4;
            cg = (cg * 3 + 242) / 4;
            cb = (cb * 3 + 235) / 4;
        }
    }
    dx = ix - 1150L;
    dy = iy - 1000L;
    dist2 = dx * dx + dy * dy;
    if (iz > 2000L && dist2 < 680000L) {
        cr = 255;
        cg = 210;
        cb = 102;
    }
    if (iz > 2000L && dist2 < 260000L) {
        cr = 255;
        cg = 244;
        cb = 190;
    }
    *r = cr;
    *g = cg;
    *b = cb;
    *a = 250;
}

static void fill_tri(const PreviewV *a, const PreviewV *b, const PreviewV *c, int draw_wire)
{
    int minx;
    int maxx;
    int miny;
    int maxy;
    int x;
    int y;
    long raw_area;
    long area;
    long w0;
    long w1;
    long w2;
    long rr;
    long gg;
    long bb;
    long aa;
    long ix;
    long iy;
    long iz;
    int sr;
    int sg;
    int sb;
    int sa;
    minx = a->sx;
    if (b->sx < minx) minx = b->sx;
    if (c->sx < minx) minx = c->sx;
    maxx = a->sx;
    if (b->sx > maxx) maxx = b->sx;
    if (c->sx > maxx) maxx = c->sx;
    miny = a->sy;
    if (b->sy < miny) miny = b->sy;
    if (c->sy < miny) miny = c->sy;
    maxy = a->sy;
    if (b->sy > maxy) maxy = b->sy;
    if (c->sy > maxy) maxy = c->sy;
    if (maxx < 0 || maxy < 0 || minx >= PREVIEW_W || miny >= PREVIEW_H) return;
    minx = clamp_i(minx, 0, PREVIEW_W - 1);
    maxx = clamp_i(maxx, 0, PREVIEW_W - 1);
    miny = clamp_i(miny, 0, PREVIEW_H - 1);
    maxy = clamp_i(maxy, 0, PREVIEW_H - 1);
    raw_area = edge_fn(a->sx, a->sy, b->sx, b->sy, c->sx, c->sy);
    if (raw_area == 0L) return;
    area = raw_area < 0L ? -raw_area : raw_area;
    for (y = miny; y <= maxy; ++y) {
        for (x = minx; x <= maxx; ++x) {
            w0 = edge_fn(b->sx, b->sy, c->sx, c->sy, x, y);
            w1 = edge_fn(c->sx, c->sy, a->sx, a->sy, x, y);
            w2 = edge_fn(a->sx, a->sy, b->sx, b->sy, x, y);
            if (raw_area < 0L) {
                w0 = -w0;
                w1 = -w1;
                w2 = -w2;
            }
            if (w0 >= 0L && w1 >= 0L && w2 >= 0L) {
                if (a->layer == GSKYBOX89_LAYER_CUBE6) {
                    ix = (a->x * w0 + b->x * w1 + c->x * w2) / area;
                    iy = (a->y * w0 + b->y * w1 + c->y * w2) / area;
                    iz = (a->z * w0 + b->z * w1 + c->z * w2) / area;
                    cube_pixel_shader(ix, iy, iz, a->face, &sr, &sg, &sb, &sa);
                    set_pixel_alpha(x, y, sr, sg, sb, sa);
                } else {
                    rr = ((long)a->r * w0 + (long)b->r * w1 + (long)c->r * w2) / area;
                    gg = ((long)a->g * w0 + (long)b->g * w1 + (long)c->g * w2) / area;
                    bb = ((long)a->b * w0 + (long)b->b * w1 + (long)c->b * w2) / area;
                    aa = ((long)a->a * w0 + (long)b->a * w1 + (long)c->a * w2) / area;
                    set_pixel_alpha(x, y, (int)rr, (int)gg, (int)bb, (int)aa);
                }
            }
        }
    }
    if (draw_wire != 0) {
        draw_line(a->sx, a->sy, b->sx, b->sy, 255, 255, 255, draw_wire);
        draw_line(b->sx, b->sy, c->sx, c->sy, 255, 255, 255, draw_wire);
        draw_line(c->sx, c->sy, a->sx, a->sy, 255, 255, 255, draw_wire);
    }
}

static void color_cube_face(int face, long y, int *r, int *g, int *b, int *a)
{
    int shade;
    int br;
    int bg;
    int bb;
    br = 90;
    bg = 130;
    bb = 190;
    if (face == GSKYBOX89_FACE_POS_X) { br = 118; bg = 155; bb = 215; }
    if (face == GSKYBOX89_FACE_NEG_X) { br = 75; bg = 105; bb = 170; }
    if (face == GSKYBOX89_FACE_POS_Y) { br = 28; bg = 76; bb = 158; }
    if (face == GSKYBOX89_FACE_NEG_Y) { br = 190; bg = 205; bb = 225; }
    if (face == GSKYBOX89_FACE_POS_Z) { br = 90; bg = 145; bb = 222; }
    if (face == GSKYBOX89_FACE_NEG_Z) { br = 68; bg = 100; bb = 160; }
    shade = 205 + (int)(((y + GSKYBOX89_FX_ONE) * 42L) / (2L * GSKYBOX89_FX_ONE));
    shade = clamp_i(shade, 170, 255);
    *r = (br * shade) / 255;
    *g = (bg * shade) / 255;
    *b = (bb * shade) / 255;
    *a = 235;
}

static PreviewV make_v_from_lib(const Gskybox89_Vertex *in_v, int pass)
{
    PreviewV out;
    out.layer = in_v->layer;
    out.face = in_v->face;
    if (in_v->layer == GSKYBOX89_LAYER_SCREEN) {
        out.x = in_v->x;
        out.y = in_v->y;
        out.z = GSKYBOX89_FX_ONE;
        out.sx = (int)(((in_v->x + GSKYBOX89_FX_ONE) * (long)PREVIEW_W) / (2L * GSKYBOX89_FX_ONE));
        out.sy = (int)(((GSKYBOX89_FX_ONE - in_v->y) * (long)PREVIEW_H) / (2L * GSKYBOX89_FX_ONE));
        out.r = (int)in_v->color.r;
        out.g = (int)in_v->color.g;
        out.b = (int)in_v->color.b;
        out.a = (int)in_v->color.a;
    } else {
        out.x = in_v->dir_x;
        out.y = in_v->dir_y;
        out.z = in_v->dir_z;
        out.sx = 0;
        out.sy = 0;
        if (in_v->layer == GSKYBOX89_LAYER_CUBE6) {
            color_cube_face(in_v->face, in_v->dir_y, &out.r, &out.g, &out.b, &out.a);
        } else {
            out.r = (int)in_v->color.r;
            out.g = (int)in_v->color.g;
            out.b = (int)in_v->color.b;
            out.a = (int)in_v->color.a;
            if (pass == GSKYBOX89_PASS_DOME && out.a > 150) out.a = 150;
        }
    }
    return out;
}

static PreviewV interp_v(const PreviewV *a, const PreviewV *b, long t)
{
    PreviewV out;
    long it;
    it = GSKYBOX89_FX_ONE - t;
    out.x = (a->x * it + b->x * t) >> GSKYBOX89_FX_SHIFT;
    out.y = (a->y * it + b->y * t) >> GSKYBOX89_FX_SHIFT;
    out.z = (a->z * it + b->z * t) >> GSKYBOX89_FX_SHIFT;
    out.sx = 0;
    out.sy = 0;
    out.r = (int)(((long)a->r * it + (long)b->r * t) >> GSKYBOX89_FX_SHIFT);
    out.g = (int)(((long)a->g * it + (long)b->g * t) >> GSKYBOX89_FX_SHIFT);
    out.b = (int)(((long)a->b * it + (long)b->b * t) >> GSKYBOX89_FX_SHIFT);
    out.a = (int)(((long)a->a * it + (long)b->a * t) >> GSKYBOX89_FX_SHIFT);
    out.layer = a->layer;
    out.face = a->face;
    return out;
}

static int clip_near(const PreviewV *in_v, int in_n, PreviewV *out_v)
{
    int i;
    int out_n;
    PreviewV s;
    PreviewV e;
    int s_in;
    int e_in;
    long den;
    long t;
    out_n = 0;
    if (in_n <= 0) return 0;
    s = in_v[in_n - 1];
    s_in = s.z >= PREVIEW_NEAR;
    for (i = 0; i < in_n; ++i) {
        e = in_v[i];
        e_in = e.z >= PREVIEW_NEAR;
        if (s_in != 0 && e_in != 0) {
            if (out_n < PREVIEW_MAX_CLIP) out_v[out_n++] = e;
        } else if (s_in != 0 && e_in == 0) {
            den = e.z - s.z;
            if (den != 0L && out_n < PREVIEW_MAX_CLIP) {
                t = ((PREVIEW_NEAR - s.z) << GSKYBOX89_FX_SHIFT) / den;
                out_v[out_n++] = interp_v(&s, &e, t);
            }
        } else if (s_in == 0 && e_in != 0) {
            den = e.z - s.z;
            if (den != 0L && out_n < PREVIEW_MAX_CLIP) {
                t = ((PREVIEW_NEAR - s.z) << GSKYBOX89_FX_SHIFT) / den;
                out_v[out_n++] = interp_v(&s, &e, t);
            }
            if (out_n < PREVIEW_MAX_CLIP) out_v[out_n++] = e;
        }
        s = e;
        s_in = e_in;
    }
    return out_n;
}

static void project_v(PreviewV *v)
{
    long z;
    z = v->z;
    if (z < PREVIEW_NEAR) z = PREVIEW_NEAR;
    v->sx = (PREVIEW_W / 2) + (int)((v->x * PREVIEW_FOCAL) / z);
    v->sy = (PREVIEW_H / 2) - (int)((v->y * PREVIEW_FOCAL) / z);
}

static void preview_begin(void *user, int pass)
{
    PreviewState *s;
    s = (PreviewState *)user;
    s->pass = pass;
}

static void preview_state(void *user, const Gskybox89_State *st)
{
    PreviewState *s;
    s = (PreviewState *)user;
    s->blend_mode = st->blend_mode;
}

static void preview_bind(void *user, int layer, int face)
{
    PreviewState *s;
    s = (PreviewState *)user;
    s->bound_layer = layer;
    s->bound_face = face;
}

static void preview_tri(void *user, const Gskybox89_Vertex *a, const Gskybox89_Vertex *b, const Gskybox89_Vertex *c)
{
    PreviewState *s;
    PreviewV in_v[3];
    PreviewV clipped[PREVIEW_MAX_CLIP];
    PreviewV ta;
    PreviewV tb;
    PreviewV tc;
    int n;
    int i;
    int wire;
    s = (PreviewState *)user;
    s->tri_total += 1;
    if (s->pass == GSKYBOX89_PASS_SCREEN) s->tri_screen += 1;
    if (s->pass == GSKYBOX89_PASS_CUBE6) s->tri_cube += 1;
    if (s->pass == GSKYBOX89_PASS_DOME) s->tri_dome += 1;
    in_v[0] = make_v_from_lib(a, s->pass);
    in_v[1] = make_v_from_lib(b, s->pass);
    in_v[2] = make_v_from_lib(c, s->pass);
    if (s->pass == GSKYBOX89_PASS_SCREEN) {
        fill_tri(&in_v[0], &in_v[1], &in_v[2], 0);
        return;
    }
    n = clip_near(in_v, 3, clipped);
    if (n < 3) return;
    for (i = 0; i < n; ++i) project_v(&clipped[i]);
    wire = 0;
    if (s->pass == GSKYBOX89_PASS_CUBE6) wire = 32;
    if (s->pass == GSKYBOX89_PASS_DOME) wire = 18;
    for (i = 1; i < n - 1; ++i) {
        ta = clipped[0];
        tb = clipped[i];
        tc = clipped[i + 1];
        fill_tri(&ta, &tb, &tc, wire);
    }
}

static void preview_clear(void)
{
    long i;
    for (i = 0; i < (long)PREVIEW_W * (long)PREVIEW_H * 3L; ++i) {
        g_pixels[i] = 0;
    }
}

static int preview_write_ppm(const char *path)
{
    FILE *f;
    size_t written;
    f = fopen(path, "wb");
    if (f == 0) return 0;
    fprintf(f, "P6\n%d %d\n255\n", PREVIEW_W, PREVIEW_H);
    written = fwrite(g_pixels, 1, sizeof(g_pixels), f);
    fclose(f);
    return written == sizeof(g_pixels);
}

int main(int argc, char **argv)
{
    Gskybox89_Context ctx;
    Gskybox89_Config cfg;
    Gskybox89_Backend be;
    Gskybox89_Mat3 rot;
    PreviewState st;
    const char *out_path;
    int rc;
    (void)argc;
    out_path = "preview_gskybox89.ppm";
    if (argv != 0 && argv[1] != 0) out_path = argv[1];

    preview_clear();
    st.pass = 0;
    st.blend_mode = 0;
    st.bound_layer = 0;
    st.bound_face = GSKYBOX89_FACE_NONE;
    st.tri_total = 0;
    st.tri_screen = 0;
    st.tri_cube = 0;
    st.tri_dome = 0;

    gskybox89_default_config(&cfg);
    cfg.layer_mask = GSKYBOX89_LAYER_HYBRID;
    cfg.dome_segments = 16;
    cfg.dome_rings = 5;
    cfg.dome_top.r = 28;
    cfg.dome_top.g = 80;
    cfg.dome_top.b = 170;
    cfg.dome_top.a = 120;
    cfg.dome_horizon.r = 245;
    cfg.dome_horizon.g = 230;
    cfg.dome_horizon.b = 190;
    cfg.dome_horizon.a = 80;

    be.user = &st;
    be.begin_pass = preview_begin;
    be.end_pass = 0;
    be.set_state = preview_state;
    be.bind_face = preview_bind;
    be.emit_tri = preview_tri;

    gskybox89_mat3_yaw16(&rot, 1);
    gskybox89_init(&ctx, &cfg, &be);
    rc = gskybox89_render(&ctx, &rot);
    if (rc != GSKYBOX89_OK) return 2;
    if (!preview_write_ppm(out_path)) return 3;
    printf("gskybox89 preview written: %s\n", out_path);
    printf("triangles total=%d screen=%d cube=%d dome=%d\n", st.tri_total, st.tri_screen, st.tri_cube, st.tri_dome);
    return 0;
}
