#include "grecoil89_provider.h"

static void grec_provider_zero_output(GRecOutput *out)
{
    if (out == 0) return;
    out->aim_angles.pitch = 0;
    out->aim_angles.yaw = 0;
    out->aim_angles.roll = 0;
    out->camera_angles.pitch = 0;
    out->camera_angles.yaw = 0;
    out->camera_angles.roll = 0;
    out->weapon_angles.pitch = 0;
    out->weapon_angles.yaw = 0;
    out->weapon_angles.roll = 0;
    out->weapon_offset.side = 0;
    out->weapon_offset.up = 0;
    out->weapon_offset.back = 0;
    out->spread = 0;
    out->burst_count = 0;
    out->shot_index = 0;
    out->active = 0;
}

static void grec_provider_copy_output(GRecOutput *dst, const GRecOutput *src)
{
    if (dst == 0 || src == 0) return;
    dst->aim_angles = src->aim_angles;
    dst->camera_angles = src->camera_angles;
    dst->weapon_angles = src->weapon_angles;
    dst->weapon_offset = src->weapon_offset;
    dst->spread = src->spread;
    dst->burst_count = src->burst_count;
    dst->shot_index = src->shot_index;
    dst->active = src->active;
}

static void grec_provider_push(GRecProvider *provider)
{
    if (provider == 0) return;
    if (provider->mode != GREC_PROVIDER_MODE_PUSH) return;

    if (provider->set_rotate != 0) {
        if ((provider->mask & GREC_PROVIDER_AIM_ROTATE) != 0u) {
            provider->set_rotate(provider->user,
                                 GREC_PROVIDER_TARGET_AIM,
                                 &provider->last_output.aim_angles);
        }
        if ((provider->mask & GREC_PROVIDER_CAMERA_ROTATE) != 0u) {
            provider->set_rotate(provider->user,
                                 GREC_PROVIDER_TARGET_CAMERA,
                                 &provider->last_output.camera_angles);
        }
        if ((provider->mask & GREC_PROVIDER_WEAPON_ROTATE) != 0u) {
            provider->set_rotate(provider->user,
                                 GREC_PROVIDER_TARGET_WEAPON,
                                 &provider->last_output.weapon_angles);
        }
    }

    if (provider->set_move != 0 &&
        (provider->mask & GREC_PROVIDER_WEAPON_MOVE) != 0u) {
        provider->set_move(provider->user,
                           GREC_PROVIDER_TARGET_WEAPON,
                           &provider->last_output.weapon_offset);
    }
}

void grec_provider_init(GRecProvider *provider,
                        void *user,
                        GRecProviderRotateFn rotate_fn,
                        GRecProviderMoveFn move_fn)
{
    if (provider == 0) return;
    provider->user = user;
    provider->set_rotate = rotate_fn;
    provider->set_move = move_fn;
    provider->mask = GREC_PROVIDER_ALL;
    provider->mode = GREC_PROVIDER_MODE_PUSH;
    provider->has_output = 0;
    grec_provider_zero_output(&provider->last_output);
}

void grec_provider_set_mode(GRecProvider *provider, grec_u16 mode)
{
    if (provider == 0) return;
    if (mode != GREC_PROVIDER_MODE_OFF &&
        mode != GREC_PROVIDER_MODE_PULL &&
        mode != GREC_PROVIDER_MODE_PUSH) {
        return;
    }
    provider->mode = mode;
}

void grec_provider_set_mask(GRecProvider *provider, grec_u32 mask)
{
    if (provider == 0) return;
    provider->mask = mask & GREC_PROVIDER_ALL;
}

void grec_provider_apply_output(GRecProvider *provider,
                                const GRecOutput *out)
{
    if (provider == 0 || out == 0) return;
    if (provider->mode == GREC_PROVIDER_MODE_OFF) return;
    grec_provider_copy_output(&provider->last_output, out);
    provider->has_output = 1;
    grec_provider_push(provider);
}

void grec_provider_after_fire(GRecProvider *provider,
                              const GRecState *state,
                              const GRecProfile *profile,
                              const GRecContext *ctx)
{
    GRecOutput out;
    if (provider == 0) return;
    if (provider->mode == GREC_PROVIDER_MODE_OFF) return;
    grec_sample(state, profile, ctx, &out);
    grec_provider_apply_output(provider, &out);
}

void grec_provider_after_update(GRecProvider *provider,
                                const GRecState *state,
                                const GRecProfile *profile,
                                const GRecContext *ctx)
{
    GRecOutput out;
    if (provider == 0) return;
    if (provider->mode == GREC_PROVIDER_MODE_OFF) return;
    grec_sample(state, profile, ctx, &out);
    grec_provider_apply_output(provider, &out);
}

int grec_provider_get_rotate(const GRecProvider *provider,
                             grec_u16 target,
                             GRecAngles *out_rotate)
{
    const GRecAngles *src;
    grec_u32 required_mask;

    if (provider == 0 || out_rotate == 0 || !provider->has_output) return 0;

    src = 0;
    required_mask = 0u;
    if (target == GREC_PROVIDER_TARGET_AIM) {
        src = &provider->last_output.aim_angles;
        required_mask = GREC_PROVIDER_AIM_ROTATE;
    } else if (target == GREC_PROVIDER_TARGET_CAMERA) {
        src = &provider->last_output.camera_angles;
        required_mask = GREC_PROVIDER_CAMERA_ROTATE;
    } else if (target == GREC_PROVIDER_TARGET_WEAPON) {
        src = &provider->last_output.weapon_angles;
        required_mask = GREC_PROVIDER_WEAPON_ROTATE;
    } else {
        return 0;
    }

    if ((provider->mask & required_mask) == 0u) return 0;
    *out_rotate = *src;
    return 1;
}

int grec_provider_get_move(const GRecProvider *provider,
                           grec_u16 target,
                           GRecVec3 *out_move)
{
    if (provider == 0 || out_move == 0 || !provider->has_output) return 0;
    if (target != GREC_PROVIDER_TARGET_WEAPON) return 0;
    if ((provider->mask & GREC_PROVIDER_WEAPON_MOVE) == 0u) return 0;
    *out_move = provider->last_output.weapon_offset;
    return 1;
}

void grec_provider_clear(GRecProvider *provider)
{
    if (provider == 0) return;
    grec_provider_zero_output(&provider->last_output);
    provider->has_output = 1;
    grec_provider_push(provider);
}

int grec_provider_has_output(const GRecProvider *provider)
{
    if (provider == 0) return 0;
    return provider->has_output ? 1 : 0;
}
