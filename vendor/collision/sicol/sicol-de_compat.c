#include "sicol-de.h"
#include "sicol_world.h"

static sicol_world_t g_legacy_world;

void sicol_init(void)
{
    sicol_world_init(&g_legacy_world);
}

sicol_id sicol_create_aabb(
    sicol_fix x, sicol_fix y, sicol_fix z,
    sicol_fix hx, sicol_fix hy, sicol_fix hz
)
{
    return sicol_create_aabb_fx(
        FX_FROM_INT(x), FX_FROM_INT(y), FX_FROM_INT(z),
        FX_FROM_INT(hx), FX_FROM_INT(hy), FX_FROM_INT(hz)
    );
}

sicol_id sicol_create_aabb_fx(
    fx x, fx y, fx z,
    fx hx, fx hy, fx hz
)
{
    sicol_shape_t shape;
    fx pos[3];
    fx half[3];

    fx_set3(pos, x, y, z);
    fx_set3(half, hx, hy, hz);
    sicol_shape_make_aabb(&shape, pos, half);
    return sicol_world_create(&g_legacy_world, &shape, 1u, 0xFFFFFFFFu);
}

void sicol_set_position(
    sicol_id id,
    sicol_fix x, sicol_fix y, sicol_fix z
)
{
    sicol_set_position_fx(id, FX_FROM_INT(x), FX_FROM_INT(y), FX_FROM_INT(z));
}

void sicol_set_position_fx(
    sicol_id id,
    fx x, fx y, fx z
)
{
    fx pos[3];
    fx_set3(pos, x, y, z);
    sicol_world_set_position(&g_legacy_world, id, pos);
}

int sicol_get_shape(sicol_id id, sicol_shape_t* out_shape)
{
    return sicol_world_get_shape(&g_legacy_world, id, out_shape);
}

int sicol_test(sicol_id a, sicol_id b)
{
    return sicol_world_overlap_pair(&g_legacy_world, a, b, 0);
}

void sicol_kill(sicol_id id)
{
    sicol_world_kill(&g_legacy_world, id);
}
