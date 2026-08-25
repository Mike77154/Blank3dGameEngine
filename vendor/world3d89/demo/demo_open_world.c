#include "w3d89.h"
#include <stdio.h>

static unsigned char g_mem[262144UL];

static int on_cell(struct w3d_world_s *world, const struct w3d_cell_s *cell, int action, void *user)
{
    (void)world;
    (void)user;
    printf("cell(%d,%d,%d) action=%d state=%u asset=%lu proxy=%lu\n", (int)cell->cx, (int)cell->cy, (int)cell->cz, action, (unsigned)cell->state, cell->asset_ref, cell->proxy_asset_ref);
    return W3D_BRIDGE_ACCEPTED;
}

int main(void)
{
    w3d_world_config cfg;
    w3d_world_callbacks cb;
    w3d_world world;
    w3d_u32 need;
    int x;
    int z;
    w3d_world_status st;

    w3d_world_default_config(&cfg);
    cfg.max_cells = 81U;
    cfg.max_cell_hash_entries = 181U;
    cfg.cell_size = w3d_fp_from_int(16);
    cfg.budget.max_load_requests_per_tick = 6U;
    cfg.budget.max_activate_requests_per_tick = 6U;
    need = w3d_world_memory_required(&cfg);
    if (need > sizeof(g_mem)) return 2;

    cb.cell_event = on_cell;
    cb.entity_event = 0;
    cb.portal_event = 0;
    cb.asset_event = 0;
    cb.render_event = 0;
    cb.collision_event = 0;
    cb.physics_event = 0;
    cb.nav_event = 0;
    cb.audio_event = 0;
    cb.script_event = 0;
    if (w3d_world_init(&world, &cfg, g_mem, sizeof(g_mem), &cb, 0) != W3D_OK) return 3;

    for (z = -4; z <= 4; ++z) {
        for (x = -4; x <= 4; ++x) {
            w3d_cell_desc d;
            d.cx = (w3d_i16)x;
            d.cy = 0;
            d.cz = (w3d_i16)z;
            d.flags = W3D_CELL_FLAG_ADDITIVE;
            d.layer_mask = W3D_LAYER_GAMEPLAY | W3D_LAYER_VISUAL | W3D_LAYER_COLLISION;
            d.phase_mask = W3D_PHASE_DEFAULT;
            d.difficulty_mask = W3D_DIFF_ALL;
            d.service_mask = W3D_SERVICE_ASSET | W3D_SERVICE_RENDER | W3D_SERVICE_COLLISION;
            d.asset_ref = (w3d_u32)(1000 + (z + 4) * 9 + (x + 4));
            d.collision_ref = d.asset_ref + 1000UL;
            d.nav_ref = 0UL;
            d.audio_ref = 0UL;
            d.script_ref = 0UL;
            d.proxy_asset_ref = d.asset_ref + 5000UL;
            d.proxy_distance = w3d_fp_from_int(48);
            w3d_world_define_cell_ex(&world, &d);
        }
    }

    w3d_world_set_stream_source(&world, 0U, w3d_v3_make(0, 0, 0), w3d_fp_from_int(20), w3d_fp_from_int(56), W3D_LAYER_ALL, 1);
    w3d_world_update_streaming(&world);
    w3d_world_get_status(&world, &st);
    printf("active=%u resident=%u load=%u deferred=%u arena=%lu\n", (unsigned)st.active_cell_count, (unsigned)st.resident_cell_count, (unsigned)st.load_requests_last_tick, (unsigned)st.deferred_requests_last_tick, st.arena_used);
    return 0;
}
