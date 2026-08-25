#include "satellaborner89.h"
#include <string.h>

int satellaborner89_resolve(const sat89_request *request,
                            sat89_target_provider_fn target_provider,
                            void *provider_user,
                            sat89_result *result)
{
    sat89_target target;
    if (!request || !result) return 0;
    memset(result, 0, sizeof(*result));
    result->target_actor_id = SAT89_TARGET_NONE;
    result->spawn_origin = request->original_origin;
    if (!target_provider) {
        if (request->flags & SAT89_FLAG_REQUIRE_TARGET) return 0;
        result->valid = 1;
        return 1;
    }
    memset(&target, 0, sizeof(target));
    target.actor_id = SAT89_TARGET_NONE;
    if (!target_provider(provider_user, request->owner_actor_id,
                         request->requested_target_id, &target) ||
        !target.valid) {
        if (request->flags & SAT89_FLAG_REQUIRE_TARGET) return 0;
        result->valid = 1;
        return 1;
    }
    result->valid = 1;
    result->target_found = 1;
    result->target_actor_id = target.actor_id;
    result->target_position = target.position;
    result->spawn_origin.x = target.position.x + request->target_offset.x;
    result->spawn_origin.y = target.position.y + request->target_offset.y;
    result->spawn_origin.z = target.position.z + request->target_offset.z;
    return 1;
}
