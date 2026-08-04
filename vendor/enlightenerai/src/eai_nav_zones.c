#include "eai_nav.h"

EAI_ZoneId eai_zone_create(EAI_Context* ctx, EAI_U8 flags)
{
    EAI_U16 i;

    for (i = 0; i < EAI_MAX_ZONES; ++i)
    {
        if (ctx->zones[i].used == 0)
        {
            ctx->zones[i].used = 1;
            ctx->zones[i].flags = flags;
            ++ctx->zone_count;
            return i;
        }
    }

    return EAI_INVALID_ID;
}

void eai_zone_set_flags(EAI_Context* ctx, EAI_ZoneId zone_id, EAI_U8 flags)
{
    if (zone_id >= EAI_MAX_ZONES || ctx->zones[zone_id].used == 0)
    {
        return;
    }
    ctx->zones[zone_id].flags = flags;
}

int eai_zone_is_enabled(const EAI_Context* ctx, EAI_ZoneId zone_id)
{
    if (zone_id == EAI_INVALID_ID)
    {
        return EAI_TRUE;
    }
    if (zone_id >= EAI_MAX_ZONES || ctx->zones[zone_id].used == 0)
    {
        return EAI_FALSE;
    }
    return ((ctx->zones[zone_id].flags & (EAI_ZONE_FLAG_DISABLED | EAI_ZONE_FLAG_LOCKED)) == 0);
}

void eai_entity_set_zone_lock(EAI_Context* ctx, EAI_EntityId entity_id, EAI_ZoneId zone_id, int enabled)
{
    if (entity_id >= EAI_MAX_ENTITIES || ctx->entities[entity_id].used == 0)
    {
        return;
    }

    if (enabled)
    {
        ctx->entities[entity_id].flags |= EAI_ENTITY_FLAG_LOCK_ZONE;
        ctx->entities[entity_id].zone_lock_id = zone_id;
    }
    else
    {
        ctx->entities[entity_id].flags &= (EAI_U8)(~EAI_ENTITY_FLAG_LOCK_ZONE);
        ctx->entities[entity_id].zone_lock_id = EAI_INVALID_ID;
    }
}
