#include <stdio.h>
#include "3d_movementbaseverbs89.h"
#include "3d_movementbaseverbs89_gamlib3d.h"

static int check(int condition, const char *message)
{
    if (!condition) {
        printf("FAIL: %s\n", message);
        return 0;
    }
    return 1;
}

int main(void)
{
    mbv89_context ctx;
    mbv89_actor actor;
    mbv89_gamlib3d_adapter adapter;
    Transform transform;
    Vec3 forward;
    mbv89_base_verb base;
    mbv89_game_verb game;
    int ok;

    ok = 1;
    mbv89_context_init(&ctx);
    mbv89_actor_init(&actor);
    mbv89_gamlib3d_adapter_init(&adapter);
    transform_init(&transform);
    mbv89_gamlib3d_bind_actor(&actor, &transform);
    mbv89_gamlib3d_install(&ctx, &adapter);

    ok &= check(mbv89_move_forward(&ctx, &actor, MBV89_FIXED_ONE) == MBV89_OK,
                "Gamlib3D move_forward failed");
    ok &= check(transform.position.z == -G3D_FIX_ONE,
                "Gamlib3D forward must follow its local -Z convention");

    ok &= check(mbv89_move_right(&ctx, &actor, MBV89_FIXED_ONE) == MBV89_OK,
                "Gamlib3D move_right failed");
    ok &= check(transform.position.x == G3D_FIX_ONE,
                "Gamlib3D move_right did not move +X");

    transform.rotation.y = 0;
    ok &= check(mbv89_rotate_left(&ctx, &actor, MBV89_FIXED_ONE / 4) == MBV89_OK,
                "Gamlib3D rotate_left failed");
    ok &= check(transform.rotation.y == G3D_FIX_FROM_INT(90),
                "Gamlib3D left turn must be +90 degrees from forward -Z");
    transform_get_local_axes(&transform, 0, 0, &forward);
    ok &= check(forward.x < 0 && forward.z == 0,
                "Gamlib3D +90 yaw must face local left (-X)");

    transform.rotation.y = 0;
    ok &= check(mbv89_rotate_right(&ctx, &actor, MBV89_FIXED_ONE / 4) == MBV89_OK,
                "Gamlib3D rotate_right failed");
    ok &= check(transform.rotation.y == G3D_FIX_FROM_INT(-90),
                "Gamlib3D right turn must be -90 degrees");
    transform_get_local_axes(&transform, 0, 0, &forward);
    ok &= check(forward.x > 0 && forward.z == 0,
                "Gamlib3D -90 yaw must face local right (+X)");

    ok &= check(mbv89_base_verb_from_name("move_back", &base) &&
                base == MBV89_BASE_MOVE_BACKWARD,
                "base alias move_back failed");
    ok &= check(mbv89_game_verb_from_name("walk_back", &game) &&
                game == MBV89_GAME_WALK_BACKWARD,
                "game alias walk_back failed");

    if (!ok) return 1;
    printf("PASS: 3d_movementbaseverbs89 Gamlib3D adapter\n");
    return 0;
}
