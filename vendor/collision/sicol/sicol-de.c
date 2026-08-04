/* ============================================================
 * SICOL-DE minimal legacy backend
 * ------------------------------------------------------------
 * This file is intentionally opt-in to avoid duplicate symbols
 * with sicol-de_compat.c. Define SICOL_BUILD_MINIMAL_LEGACY
 * if you want the original minimal implementation.
 * ============================================================ */

#ifdef SICOL_BUILD_MINIMAL_LEGACY

#include "sicol-de.h"

struct sicol_aabb {
    sicol_fix minx, miny, minz;
    sicol_fix maxx, maxy, maxz;
};

struct sicol_collider {
    int active;
    struct sicol_aabb box;
};

static struct sicol_collider sicol_pool[SICOL_MAX];
static int sicol_count = 0;

void sicol_init(void)
{
    int i;
    for (i = 0; i < SICOL_MAX; ++i)
        sicol_pool[i].active = 0;
    sicol_count = 0;
}

sicol_id sicol_create_aabb(
    sicol_fix x, sicol_fix y, sicol_fix z,
    sicol_fix hx, sicol_fix hy, sicol_fix hz
)
{
    sicol_id id;

    if (sicol_count >= SICOL_MAX)
        return -1;

    id = sicol_count++;
    sicol_pool[id].active = 1;

    sicol_pool[id].box.minx = x - hx;
    sicol_pool[id].box.miny = y - hy;
    sicol_pool[id].box.minz = z - hz;

    sicol_pool[id].box.maxx = x + hx;
    sicol_pool[id].box.maxy = y + hy;
    sicol_pool[id].box.maxz = z + hz;

    return id;
}

sicol_id sicol_create_aabb_fx(
    fx x, fx y, fx z,
    fx hx, fx hy, fx hz
)
{
    return sicol_create_aabb(
        FX_TO_INT(x), FX_TO_INT(y), FX_TO_INT(z),
        FX_TO_INT(hx), FX_TO_INT(hy), FX_TO_INT(hz)
    );
}

void sicol_set_position(
    sicol_id id,
    sicol_fix x, sicol_fix y, sicol_fix z
)
{
    sicol_fix hx, hy, hz;
    struct sicol_aabb *b;

    if (id < 0 || id >= sicol_count) return;
    if (!sicol_pool[id].active) return;

    b = &sicol_pool[id].box;

    hx = (b->maxx - b->minx) / 2;
    hy = (b->maxy - b->miny) / 2;
    hz = (b->maxz - b->minz) / 2;

    b->minx = x - hx;  b->maxx = x + hx;
    b->miny = y - hy;  b->maxy = y + hy;
    b->minz = z - hz;  b->maxz = z + hz;
}

void sicol_set_position_fx(
    sicol_id id,
    fx x, fx y, fx z
)
{
    sicol_set_position(id, FX_TO_INT(x), FX_TO_INT(y), FX_TO_INT(z));
}

int sicol_get_shape(sicol_id id, sicol_shape_t* out_shape)
{
    fx pos[3];
    fx half[3];
    if (!out_shape) return 0;
    if (id < 0 || id >= sicol_count) return 0;
    if (!sicol_pool[id].active) return 0;

    fx_set3(pos,
            FX_FROM_INT((sicol_pool[id].box.minx + sicol_pool[id].box.maxx) / 2),
            FX_FROM_INT((sicol_pool[id].box.miny + sicol_pool[id].box.maxy) / 2),
            FX_FROM_INT((sicol_pool[id].box.minz + sicol_pool[id].box.maxz) / 2));

    fx_set3(half,
            FX_FROM_INT((sicol_pool[id].box.maxx - sicol_pool[id].box.minx) / 2),
            FX_FROM_INT((sicol_pool[id].box.maxy - sicol_pool[id].box.miny) / 2),
            FX_FROM_INT((sicol_pool[id].box.maxz - sicol_pool[id].box.minz) / 2));

    sicol_shape_make_aabb(out_shape, pos, half);
    return 1;
}

int sicol_test(sicol_id a, sicol_id b)
{
    struct sicol_aabb *A, *B;

    if (a < 0 || b < 0) return 0;
    if (a >= sicol_count || b >= sicol_count) return 0;
    if (!sicol_pool[a].active || !sicol_pool[b].active) return 0;

    A = &sicol_pool[a].box;
    B = &sicol_pool[b].box;

    if (A->maxx < B->minx || A->minx > B->maxx) return 0;
    if (A->maxy < B->miny || A->miny > B->maxy) return 0;
    if (A->maxz < B->minz || A->minz > B->maxz) return 0;

    return 1;
}

void sicol_kill(sicol_id id)
{
    if (id < 0 || id >= sicol_count) return;
    sicol_pool[id].active = 0;
}

#endif /* SICOL_BUILD_MINIMAL_LEGACY */
