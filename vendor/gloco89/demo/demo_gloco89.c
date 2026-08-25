#include <stdio.h>
#include "gloco89.h"
#include "gloco89_profiles.h"
#include "gloco89_bridge.h"

static int demo_probe(void *user,
                      const GLOCO_Vec3 *from,
                      const GLOCO_Vec3 *to,
                      GLOCO_FX radius,
                      GLOCO_FX height,
                      GLOCO_ProbeResult *out_result)
{
    (void)user;
    (void)from;
    (void)radius;
    (void)height;
    out_result->corrected_pos = *to;
    out_result->ground_normal = gloco_v3(0, GLOCO_FX_ONE, 0);
    out_result->flags = 0;

    /* Simple flat floor for demo. Replace with your engine sweep/capsule test. */
    if (out_result->corrected_pos.y <= 0) {
        out_result->corrected_pos.y = 0;
        out_result->flags |= GLOCO_FLAG_GROUNDED;
    }
    return 1;
}

static void demo_event(void *user, int actor_id, int event_id, int value)
{
    (void)user;
    printf("event actor=%d event=%d value=%d\n", actor_id, event_id, value);
}

static void print_actor(const GLOCO_Context *ctx, int id, int frame)
{
    const GLOCO_Actor *a;
    a = gloco_actor_get_const(ctx, id);
    if (!a) return;
    printf("f=%03d state=%s pos=(%ld,%ld,%ld) vel=(%ld,%ld,%ld) stamina=%ld flags=0x%04x\n",
           frame,
           gloco_state_name(a->state),
           a->pos.x, a->pos.y, a->pos.z,
           a->vel.x, a->vel.y, a->vel.z,
           a->stamina,
           (unsigned int)a->flags);
}

int main(void)
{
    GLOCO_Context ctx;
    GLOCO_Input in;
    GLOCO_Vec3 pos;
    GLOCO_Vec3 fwd;
    GLOCO_Vec3 right;
    int actor;
    int i;

    gloco_init(&ctx);
    gloco_load_default_profile_bank(&ctx);
    gloco_set_callbacks(&ctx, demo_probe, demo_event, 0);

    pos = gloco_v3(0, 0, 0);
    fwd = gloco_v3(0, 0, GLOCO_FX_ONE);
    right = gloco_v3(GLOCO_FX_ONE, 0, 0);
    actor = gloco_actor_create(&ctx, GLOCO_PROFILE_TACTICAL, &pos, &fwd);

    for (i = 0; i < 180; ++i) {
        if (i < 40) {
            in = gloco_bridge_make_camera_input(0, 256, GLOCO_INPUT_WALK, fwd, right);
        } else if (i < 90) {
            in = gloco_bridge_make_camera_input(0, 256, GLOCO_INPUT_RUN, fwd, right);
        } else if (i < 130) {
            in = gloco_bridge_make_camera_input(0, 256, GLOCO_INPUT_RUN | GLOCO_INPUT_SPRINT, fwd, right);
        } else if (i == 130) {
            in = gloco_bridge_make_camera_input(256, 0, GLOCO_INPUT_EVADE, fwd, right);
            in.evade_x = 256;
            in.evade_z = 0;
        } else if (i < 150) {
            in = gloco_bridge_make_camera_input(0, 0, 0, fwd, right);
        } else {
            in = gloco_bridge_make_camera_input(0, 0, GLOCO_INPUT_HARD_STOP, fwd, right);
        }

        gloco_update_actor(&ctx, actor, &in, 16);
        if ((i % 10) == 0 || i == 130) print_actor(&ctx, actor, i);
    }

    return 0;
}
