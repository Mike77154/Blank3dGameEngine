#include <stdio.h>
#include "gskybox89.h"

typedef struct DemoStats {
    int tri_count;
    int pass_count;
} DemoStats;

static void demo_begin(void *user, int pass)
{
    DemoStats *s;
    s = (DemoStats *)user;
    s->pass_count += 1;
    printf("BEGIN %s\n", gskybox89_pass_name(pass));
}

static void demo_end(void *user, int pass)
{
    (void)user;
    printf("END %s\n", gskybox89_pass_name(pass));
}

static void demo_state(void *user, const Gskybox89_State *st)
{
    (void)user;
    printf("STATE pass=%s depth_test=%d depth_write=%d depth_func=%d cull=%d blend=%d\n",
        gskybox89_pass_name(st->pass), st->depth_test, st->depth_write,
        st->depth_func, st->cull_mode, st->blend_mode);
}

static void demo_bind(void *user, int layer, int face)
{
    (void)user;
    printf("BIND layer=%d face=%s\n", layer, gskybox89_face_name(face));
}

static void demo_tri(void *user, const Gskybox89_Vertex *a, const Gskybox89_Vertex *b, const Gskybox89_Vertex *c)
{
    DemoStats *s;
    s = (DemoStats *)user;
    s->tri_count += 1;
    if (s->tri_count <= 8) {
        printf("TRI %03d layer=%d face=%s a_dir=(%ld,%ld,%ld) b_dir=(%ld,%ld,%ld) c_dir=(%ld,%ld,%ld)\n",
            s->tri_count, a->layer, gskybox89_face_name(a->face),
            a->dir_x, a->dir_y, a->dir_z,
            b->dir_x, b->dir_y, b->dir_z,
            c->dir_x, c->dir_y, c->dir_z);
    }
}

int main(void)
{
    Gskybox89_Context ctx;
    Gskybox89_Config cfg;
    Gskybox89_Backend be;
    Gskybox89_Mat3 rot;
    DemoStats stats;
    int rc;

    stats.tri_count = 0;
    stats.pass_count = 0;

    gskybox89_default_config(&cfg);

    /* Mix of level 2, 3 and 4. */
    cfg.layer_mask = GSKYBOX89_LAYER_HYBRID;
    cfg.dome_segments = 12;
    cfg.dome_rings = 5;

    be.user = &stats;
    be.begin_pass = demo_begin;
    be.end_pass = demo_end;
    be.set_state = demo_state;
    be.bind_face = demo_bind;
    be.emit_tri = demo_tri;

    gskybox89_mat3_yaw16(&rot, 2);
    gskybox89_init(&ctx, &cfg, &be);
    rc = gskybox89_render(&ctx, &rot);

    printf("RESULT rc=%d passes=%d tris=%d\n", rc, stats.pass_count, stats.tri_count);
    return rc == GSKYBOX89_OK ? 0 : 1;
}
