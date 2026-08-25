#include "3d_movementbaseverbs89_gamlib3d.h"

static g3d_fix mbv89_gamlib3d_linear_to_q20_12(mbv89_fixed value)
{
    long q;
    long r;
    q = (long)value / 16L;
    r = (long)value % 16L;
    if (r >= 8L && q < G3D_FIX_MAX) q += 1L;
    else if (r <= -8L && q > G3D_FIX_MIN) q -= 1L;
    return (g3d_fix)q;
}

static g3d_fix mbv89_gamlib3d_turns_to_degrees(mbv89_fixed turns)
{
    long half;
    long rem;
    long result;

    half = (long)turns / 2L;
    rem = (long)turns % 2L;

    if (half > G3D_FIX_MAX / 45L) return G3D_FIX_MAX;
    if (half < G3D_FIX_MIN / 45L) return G3D_FIX_MIN;

    result = half * 45L;
    if (rem > 0L) {
        if (result > G3D_FIX_MAX - 23L) return G3D_FIX_MAX;
        result += 23L;
    } else if (rem < 0L) {
        if (result < G3D_FIX_MIN + 23L) return G3D_FIX_MIN;
        result -= 23L;
    }
    return (g3d_fix)result;
}

void mbv89_gamlib3d_adapter_init(mbv89_gamlib3d_adapter *adapter)
{
    if (adapter == 0) return;
    adapter->move_mode = MBV89_GAMLIB3D_MOVE_FLAT;
}

void mbv89_gamlib3d_adapter_set_move_mode(mbv89_gamlib3d_adapter *adapter,
                                           mbv89_gamlib3d_move_mode mode)
{
    if (adapter == 0) return;
    adapter->move_mode = mode == MBV89_GAMLIB3D_MOVE_6DOF
                       ? MBV89_GAMLIB3D_MOVE_6DOF
                       : MBV89_GAMLIB3D_MOVE_FLAT;
}

void mbv89_gamlib3d_bind_actor(mbv89_actor *actor, Transform *transform)
{
    if (actor == 0) return;
    actor->user = transform;
}

Transform *mbv89_gamlib3d_actor_transform(mbv89_actor *actor)
{
    if (actor == 0) return 0;
    return (Transform *)actor->user;
}

void mbv89_gamlib3d_install(mbv89_context *ctx,
                             mbv89_gamlib3d_adapter *adapter)
{
    if (ctx == 0) return;
    mbv89_set_base_provider(ctx, mbv89_gamlib3d_base_provider, adapter);
}

int mbv89_gamlib3d_base_provider(void *provider_user,
                                 mbv89_context *ctx,
                                 mbv89_actor *actor,
                                 mbv89_base_verb verb,
                                 mbv89_fixed amount)
{
    mbv89_gamlib3d_adapter *adapter;
    Transform *transform;
    g3d_fix linear;
    g3d_fix degrees;
    int flat;
    (void)ctx;

    adapter = (mbv89_gamlib3d_adapter *)provider_user;
    transform = mbv89_gamlib3d_actor_transform(actor);
    if (transform == 0) return MBV89_UNHANDLED;

    flat = adapter == 0 || adapter->move_mode == MBV89_GAMLIB3D_MOVE_FLAT;
    linear = mbv89_gamlib3d_linear_to_q20_12(amount);

    switch (verb) {
        case MBV89_BASE_MOVE_FORWARD:
            if (flat) transform_move_local_flat(transform, 0, 0, linear);
            else transform_move_local(transform, 0, 0, linear);
            return MBV89_HANDLED;
        case MBV89_BASE_MOVE_BACKWARD:
            linear = g3d_fix_neg_sat(linear);
            if (flat) transform_move_local_flat(transform, 0, 0, linear);
            else transform_move_local(transform, 0, 0, linear);
            return MBV89_HANDLED;
        case MBV89_BASE_MOVE_LEFT:
            linear = g3d_fix_neg_sat(linear);
            if (flat) transform_move_local_flat(transform, linear, 0, 0);
            else transform_move_local(transform, linear, 0, 0);
            return MBV89_HANDLED;
        case MBV89_BASE_MOVE_RIGHT:
            if (flat) transform_move_local_flat(transform, linear, 0, 0);
            else transform_move_local(transform, linear, 0, 0);
            return MBV89_HANDLED;
        case MBV89_BASE_MOVE_UP:
            if (flat) transform_move_local_flat(transform, 0, linear, 0);
            else transform_move_local(transform, 0, linear, 0);
            return MBV89_HANDLED;
        case MBV89_BASE_MOVE_DOWN:
            linear = g3d_fix_neg_sat(linear);
            if (flat) transform_move_local_flat(transform, 0, linear, 0);
            else transform_move_local(transform, 0, linear, 0);
            return MBV89_HANDLED;
        default:
            break;
    }

    degrees = mbv89_gamlib3d_turns_to_degrees(amount);
    switch (verb) {
        case MBV89_BASE_ROTATE_FORWARD:
            transform_rotate(transform, 0, degrees, 0);
            return MBV89_HANDLED;
        case MBV89_BASE_ROTATE_BACKWARD:
            transform_rotate(transform, 0, g3d_fix_neg_sat(degrees), 0);
            return MBV89_HANDLED;
        case MBV89_BASE_ROTATE_LEFT:
            transform_rotate(transform, degrees, 0, 0);
            return MBV89_HANDLED;
        case MBV89_BASE_ROTATE_RIGHT:
            transform_rotate(transform, g3d_fix_neg_sat(degrees), 0, 0);
            return MBV89_HANDLED;
        case MBV89_BASE_ROTATE_UP:
            transform_rotate(transform, 0, 0, degrees);
            return MBV89_HANDLED;
        case MBV89_BASE_ROTATE_DOWN:
            transform_rotate(transform, 0, 0, g3d_fix_neg_sat(degrees));
            return MBV89_HANDLED;
        default:
            return MBV89_UNHANDLED;
    }
}
