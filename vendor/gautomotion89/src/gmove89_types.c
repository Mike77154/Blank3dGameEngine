#include "gmove89_types.h"

void gmove89_motion_clear(GMoveMotion89 *motion)
{
    if (motion == 0) {
        return;
    }

    motion->target_position.x = 0L;
    motion->target_position.y = 0L;
    motion->target_position.z = 0L;

    motion->delta.x = 0L;
    motion->delta.y = 0L;
    motion->delta.z = 0L;

    motion->desired_direction.x = 0L;
    motion->desired_direction.y = 0L;
    motion->desired_direction.z = 0L;

    motion->desired_forward.x = 0L;
    motion->desired_forward.y = 0L;
    motion->desired_forward.z = 0L;

    motion->distance_remaining = 0L;
    motion->reached = GMOVE89_FALSE;
    motion->has_translation = GMOVE89_FALSE;
    motion->has_rotation = GMOVE89_FALSE;
    motion->valid = GMOVE89_FALSE;
}

int gmove89_target_resolve(
    const GMoveProvider89 *provider,
    const GMoveTarget89 *target,
    GMoveVec3_89 *out_position
)
{
    if (target == 0 || out_position == 0) {
        return GMOVE89_FALSE;
    }

    if (target->type == GMOVE89_TARGET_POINT) {
        *out_position = target->point;
        return GMOVE89_TRUE;
    }

    if (provider == 0) {
        return GMOVE89_FALSE;
    }

    if (target->type == GMOVE89_TARGET_ENTITY) {
        if (provider->get_position == 0) {
            return GMOVE89_FALSE;
        }
        return provider->get_position(
            provider->user,
            target->entity_id,
            out_position
        );
    }

    if (target->type == GMOVE89_TARGET_SOCKET) {
        if (provider->get_socket_position == 0) {
            return GMOVE89_FALSE;
        }
        return provider->get_socket_position(
            provider->user,
            target->entity_id,
            target->socket_id,
            out_position
        );
    }

    return GMOVE89_FALSE;
}

int gmove89_apply_motion(
    const GMoveProvider89 *provider,
    GMoveId89 entity_id,
    const GMoveMotion89 *motion
)
{
    if (provider == 0 || provider->apply_motion == 0 || motion == 0) {
        return GMOVE89_FALSE;
    }

    return provider->apply_motion(provider->user, entity_id, motion);
}
