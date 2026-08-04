#include "ccs_world.h"

#include "ccs_collision.h" /* optional: for AABB compute */
#include "ccs_dispatch.h"
#include "ccs_resolve.h"

#include "ccs_broad_grid3d.h"
#include "ccs_broad_sweep.h"

ccs_body ccs_bodies[CCS_MAX_BODIES];
int ccs_body_count = 0;

/* broadphase selection */
static int g_world_broadphase = CCS_WORLD_DEFAULT_BROADPHASE;

/* sweep context (static, sin malloc) */
static CCS_SweepContext g_sweep_ctx;

/* overflow flags (último step) */
static unsigned int g_world_overflow_flags = 0u;

/* ============================================================
   Broadphase selection API
   ============================================================ */

void ccs_world_set_broadphase(int broadphase_id)
{
    if (broadphase_id != CCS_WORLDBP_GRID3D &&
        broadphase_id != CCS_WORLDBP_SWEEP)
        return;

    g_world_broadphase = broadphase_id;
}

int ccs_world_get_broadphase(void)
{
    return g_world_broadphase;
}

unsigned int ccs_world_get_overflow_flags(void)
{
    return g_world_overflow_flags;
}

/* ============================================================
   Internal helpers
   ============================================================ */

static int ccs_world_pair_allowed(int a, int b)
{
    const ccs_body* A;
    const ccs_body* B;

    if (a < 0 || b < 0)
        return 0;
    if (a >= ccs_body_count || b >= ccs_body_count)
        return 0;

    A = &ccs_bodies[a];
    B = &ccs_bodies[b];

    if (!A->shape || !B->shape)
        return 0;

    /* layer/mask filters */
    if ((A->mask & B->layer) == 0u)
        return 0;
    if ((B->mask & A->layer) == 0u)
        return 0;

    return 1;
}

/* floor division for signed ints (deterministic) */
static int ccs_world_floor_div_int(int a, int d)
{
    ccs_u32 ua;
    ccs_u32 ud;
    ccs_u32 q;
    ccs_u32 r;

    if (d <= 0)
        return 0;

    if (a >= 0)
        return a / d;

    /* a < 0 */
    ua = (ccs_u32)a;
    ua = (ccs_u32)(0u - ua);
    ud = (ccs_u32)d;

    q = ua / ud;
    r = ua - (q * ud);

    if (r != 0u)
        q += 1u;

    return -(int)q;
}

static void ccs_world_sync_shape_center(int id)
{
    ccs_body* body;

    if (id < 0 || id >= ccs_body_count)
        return;

    body = &ccs_bodies[id];

    if (!body->shape || !body->position)
        return;

    /* Copiamos posición externa -> center del shape (universo A) */
    ccs_shape_set_center(body->shape, *body->position);
}

static void ccs_world_resolve_pair(int a, int b)
{
    ccs_body* A;
    ccs_body* B;
    ccs_manifold manifold;
    int hit;

    if (!ccs_world_pair_allowed(a, b))
        return;

    A = &ccs_bodies[a];
    B = &ccs_bodies[b];

    /* sync centers from external positions before narrow-phase */
    ccs_world_sync_shape_center(a);
    ccs_world_sync_shape_center(b);

    /* triggers: no resolve (overlap only) */
    if (ccs_shape_is_trigger(A->shape) || ccs_shape_is_trigger(B->shape)) {
        return;
    }

    /* static-static: ignore */
    if (ccs_shape_is_static(A->shape) && ccs_shape_is_static(B->shape)) {
        return;
    }

    hit = ccs_dispatch_collide_manifold(A->shape, B->shape, &manifold);
    if (!hit)
        return;

    /* Resolver penetración con regla static/dynamic */
    if (A->position && B->position) {
        int a_static;
        int b_static;

        a_static = ccs_shape_is_static(A->shape);
        b_static = ccs_shape_is_static(B->shape);

        if (manifold.count > 0) {
            /* escoger el contacto con mayor penetración */
            int best;
            int i;
            ccs_contact c;
            ccs_vec3 push;

            best = 0;
            for (i = 1; i < manifold.count; ++i) {
                if (manifold.contacts[i].penetration >
                    manifold.contacts[best].penetration) {
                    best = i;
                }
            }

            c.normal = manifold.contacts[best].normal;
            c.penetration = manifold.contacts[best].penetration;

            if (a_static && !b_static) {
                push = ccs_vec3_scale(c.normal, c.penetration);
                *B->position = ccs_vec3_add(*B->position, push);
            } else if (b_static && !a_static) {
                push = ccs_vec3_scale(c.normal, c.penetration);
                *A->position = ccs_vec3_sub(*A->position, push);
            } else {
                /* dynamic-dynamic */
                ccs_resolve_manifold(A->position, B->position, &manifold);
            }

            /* re-sync shape centers after moving positions */
            ccs_world_sync_shape_center(a);
            ccs_world_sync_shape_center(b);
        }
    }
}

/* callback interno para grid3d */
static void ccs_world_test_pair_cb(int a, int b)
{
    ccs_world_resolve_pair(a, b);
}

/* ============================================================
   Public API
   ============================================================ */

void ccs_world_clear(void)
{
    int i;

    ccs_body_count = 0;
    g_world_overflow_flags = 0u;

    /* defaults */
    for (i = 0; i < CCS_MAX_BODIES; ++i) {
        ccs_bodies[i].position = 0;
        ccs_bodies[i].shape = 0;
        ccs_bodies[i].layer = 1u;
        ccs_bodies[i].mask = 0xFFFFFFFFu;
        ccs_bodies[i].user_data = 0;
    }

    ccs_broad_grid3d_clear();
    ccs_sweep_init(&g_sweep_ctx);
}

int ccs_world_add(ccs_vec3* position, ccs_shape* shape)
{
    int id;

    if (ccs_body_count >= CCS_MAX_BODIES)
        return -1;

    id = ccs_body_count;
    ccs_body_count++;

    ccs_bodies[id].position = position;
    ccs_bodies[id].shape = shape;

    ccs_bodies[id].layer = 1u;
    ccs_bodies[id].mask = 0xFFFFFFFFu;
    ccs_bodies[id].user_data = 0;

    /* Mantener el shape sincronizado desde el inicio */
    ccs_world_sync_shape_center(id);

    return id;
}

void ccs_world_remove(int id)
{
    int last;

    if (id < 0 || id >= ccs_body_count)
        return;

    last = ccs_body_count - 1;

    if (id != last) {
        /* swap-remove: mover el último al hueco */
        ccs_bodies[id] = ccs_bodies[last];
    }

    /* limpiar slot final (debug-friendly) */
    ccs_bodies[last].position = 0;
    ccs_bodies[last].shape = 0;
    ccs_bodies[last].layer = 1u;
    ccs_bodies[last].mask = 0xFFFFFFFFu;
    ccs_bodies[last].user_data = 0;

    ccs_body_count--;
}

void ccs_world_step(void)
{
    int i;

    g_world_overflow_flags = 0u;

    if (g_world_broadphase == CCS_WORLDBP_SWEEP) {
        /* ----------------------------------------------------
           Sweep & Prune (generalista)
           ---------------------------------------------------- */

        ccs_sweep_init(&g_sweep_ctx);

        for (i = 0; i < ccs_body_count; ++i) {
            ccs_aabb aabb;

            /* sincroniza centers desde posiciones externas */
            ccs_world_sync_shape_center(i);

            if (!ccs_bodies[i].shape)
                continue;

            aabb = ccs_shape_compute_aabb(ccs_bodies[i].shape);
            ccs_sweep_add(&g_sweep_ctx, &aabb, i);
        }

        ccs_sweep_compute(&g_sweep_ctx);

        if (g_sweep_ctx.pair_overflow)
            g_world_overflow_flags |= CCS_WORLD_OVERFLOW_SWEEP_PAIRS;
        if (g_sweep_ctx.object_overflow)
            g_world_overflow_flags |= CCS_WORLD_OVERFLOW_SWEEP_OBJECTS;

        for (i = 0; i < g_sweep_ctx.pair_count; ++i) {
            int a = g_sweep_ctx.pairs[i].a;
            int b = g_sweep_ctx.pairs[i].b;
            ccs_world_resolve_pair(a, b);
        }

        return;
    }

    /* --------------------------------------------------------
       Grid 3D (rápido, PS1-friendly)
       Requiere configuración de CELL_SIZE y NEIGHBOR_RANGE.
       -------------------------------------------------------- */

    ccs_broad_grid3d_clear();

    for (i = 0; i < ccs_body_count; ++i) {
        ccs_vec3 c;
        int wx;
        int wy;
        int wz;
        int gx;
        int gy;
        int gz;

        /* sincroniza centers desde posiciones externas */
        ccs_world_sync_shape_center(i);

        if (!ccs_bodies[i].shape)
            continue;

        c = ccs_shape_get_center(ccs_bodies[i].shape);

        /* fixed -> world units (enteros) */
        wx = (int)ccs_fixed_to_int_floor(c.x);
        wy = (int)ccs_fixed_to_int_floor(c.y);
        wz = (int)ccs_fixed_to_int_floor(c.z);

        /* world units -> cell indices (biased to allow negatives) */
        gx = ccs_world_floor_div_int(wx, CCS_BG3D_CELL_SIZE) + CCS_BG3D_BIAS_X;
        gy = ccs_world_floor_div_int(wy, CCS_BG3D_CELL_SIZE) + CCS_BG3D_BIAS_Y;
        gz = ccs_world_floor_div_int(wz, CCS_BG3D_CELL_SIZE) + CCS_BG3D_BIAS_Z;

        ccs_broad_grid3d_insert(i, gx, gy, gz);
    }

    if (ccs_broad_grid3d_overflow_count() > 0)
        g_world_overflow_flags |= CCS_WORLD_OVERFLOW_GRID3D_OOB;

    ccs_broad_grid3d_for_each_pair(ccs_world_test_pair_cb);
}

int ccs_world_raycast(
    const ccs_ray* ray,
    ccs_u32 layer_mask,
    ccs_raycast_hit* out_hit,
    int* out_body_id
) {
    int i;
    int found;
    ccs_fixed best_t;

    if (!ray || !out_hit)
        return 0;

    out_hit->hit = 0;
    if (out_body_id) *out_body_id = -1;

    found = 0;
    best_t = ray->tmax;

    for (i = 0; i < ccs_body_count; ++i) {
        ccs_body* body;
        ccs_raycast_hit h;

        body = &ccs_bodies[i];
        if (!body->shape)
            continue;

        if (layer_mask != 0 && (body->layer & layer_mask) == 0)
            continue;

        if (body->position)
            ccs_shape_set_center(body->shape, *body->position);

        if (ccs_raycast_shape(ray, body->shape, &h)) {
            if (!found || h.t < best_t) {
                found = 1;
                best_t = h.t;
                *out_hit = h;
                if (out_body_id) *out_body_id = i;
            }
        }
    }

    return found;
}
