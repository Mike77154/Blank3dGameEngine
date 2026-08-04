#include <stdio.h>
#include "fcasing89.h"

static void print_vec(const char *name, FCasing89Vec3 v)
{
    printf("%s=(%ld,%ld,%ld)", name,
        FCASING89_FROM_FIX(v.x),
        FCASING89_FROM_FIX(v.y),
        FCASING89_FROM_FIX(v.z));
}

int main(void)
{
    FCasing89System sys;
    FCasing89Config cfg;
    FCasing89Camera cam;
    FCasing89Spawn spawn;
    FCasing89RenderItem items[32];
    FCasing89Event ev;
    FCasing89Stats st;
    int frame;
    int count;

    fcasing89_default_config(&cfg);
    cfg.max_simulated = 24U;
    cfg.max_fake = 24U;
    cfg.max_spawn_per_frame = 6U;
    cfg.near_distance = FCASING89_TO_FIX(80);
    cfg.fake_distance = FCASING89_TO_FIX(180);
    cfg.global_floor_y = 0L;

    fcasing89_init(&sys, &cfg, 12345UL);

    cam.pos = fcasing89_vec3(FCASING89_TO_FIX(0), FCASING89_TO_FIX(20), FCASING89_TO_FIX(-20));
    cam.valid = 1U;
    cam.reserved0 = 0U;
    cam.reserved1 = 0U;
    fcasing89_set_camera(&sys, &cam);

    spawn.origin = fcasing89_vec3(FCASING89_TO_FIX(0), FCASING89_TO_FIX(18), FCASING89_TO_FIX(0));
    spawn.forward = fcasing89_vec3(0L, 0L, FCASING89_FIX_ONE);
    spawn.right = fcasing89_vec3(FCASING89_FIX_ONE, 0L, 0L);
    spawn.up = fcasing89_vec3(0L, FCASING89_FIX_ONE, 0L);
    spawn.profile_id = FCASING89_PROFILE_RIFLE;
    spawn.count = 1U;
    spawn.importance = 220U;
    spawn.flags = 0U;
    spawn.seed_bias = 0U;
    spawn.local_side_bias = 0L;
    spawn.local_up_bias = 0L;
    spawn.local_back_bias = 0L;

    for (frame = 0; frame < 120; frame++) {
        fcasing89_begin_frame(&sys);
        if ((frame % 4) == 0) {
            fcasing89_emit(&sys, &spawn);
        }
        fcasing89_update(&sys, 16U);
        while (fcasing89_pop_event(&sys, &ev) != 0) {
            if (ev.type == FCASING89_EVENT_BOUNCE) {
                printf("event bounce audio=%u ", (unsigned)ev.audio_id);
                print_vec("pos", ev.pos);
                printf("\n");
            }
        }
        if ((frame % 20) == 0) {
            count = fcasing89_collect_render_items(&sys, items, 32);
            fcasing89_get_stats(&sys, &st);
            printf("frame=%d render=%d active=%u sim=%u fake=%u free=%u emitted=%lu\n",
                frame, count, (unsigned)st.active_total, (unsigned)st.active_sim,
                (unsigned)st.active_fake, (unsigned)st.free_count, st.emitted_total);
            if (count > 0) {
                print_vec(" first", items[0].pos);
                printf(" alpha=%u state=%u\n", (unsigned)items[0].alpha, (unsigned)items[0].state);
            }
        }
    }
    return 0;
}
