#include <stdio.h>
#include "grecoil89.h"
#include "grecoil89_profiles.h"
#include "grecoil89_provider.h"

/* Pretend this struct belongs to a completely different transform library. */
typedef struct ExternalTransformLib_s {
    GRecAngles aim_rotation;
    GRecAngles camera_rotation;
    GRecAngles weapon_rotation;
    GRecVec3 weapon_move;
    int rotate_calls;
    int move_calls;
} ExternalTransformLib;

static void external_set_rotate(void *user,
                                grec_u16 target,
                                const GRecAngles *rotate)
{
    ExternalTransformLib *lib;
    lib = (ExternalTransformLib *)user;
    if (target == GREC_PROVIDER_TARGET_AIM) lib->aim_rotation = *rotate;
    else if (target == GREC_PROVIDER_TARGET_CAMERA) lib->camera_rotation = *rotate;
    else if (target == GREC_PROVIDER_TARGET_WEAPON) lib->weapon_rotation = *rotate;
    lib->rotate_calls++;
}

static void external_set_move(void *user,
                              grec_u16 target,
                              const GRecVec3 *move)
{
    ExternalTransformLib *lib;
    lib = (ExternalTransformLib *)user;
    if (target == GREC_PROVIDER_TARGET_WEAPON) lib->weapon_move = *move;
    lib->move_calls++;
}

int main(void)
{
    GRecState state;
    GRecContext ctx;
    GRecProvider provider;
    ExternalTransformLib transforms;
    const GRecProfile *profile;
    int i;

    transforms.rotate_calls = 0;
    transforms.move_calls = 0;
    profile = grec_get_profile(GREC_PROFILE_RIFLE_556);
    if (profile == 0) return 1;

    grec_state_init(&state, 1234u);
    grec_context_for_mode(&ctx, GREC_MODE_HIP);

    grec_provider_init(&provider,
                       &transforms,
                       external_set_rotate,
                       external_set_move);
    grec_provider_set_mode(&provider, GREC_PROVIDER_MODE_PUSH);

    grec_fire(&state, profile, &ctx);
    grec_provider_after_fire(&provider, &state, profile, &ctx);

    for (i = 0; i < 16; ++i) {
        grec_update(&state, profile);
        grec_provider_after_update(&provider, &state, profile, &ctx);
    }

    printf("provider rotate_calls=%d move_calls=%d\n",
           transforms.rotate_calls,
           transforms.move_calls);
    printf("weapon rotate p=%d y=%d r=%d\n",
           (int)transforms.weapon_rotation.pitch,
           (int)transforms.weapon_rotation.yaw,
           (int)transforms.weapon_rotation.roll);
    printf("weapon move side=%d up=%d back=%d\n",
           (int)transforms.weapon_move.side,
           (int)transforms.weapon_move.up,
           (int)transforms.weapon_move.back);

    /* Clear the additive layer when weapon/provider is detached. */
    grec_provider_clear(&provider);
    return 0;
}
