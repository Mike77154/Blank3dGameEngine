#include "pbb_item_internal.h"
#include <string.h>

int pbb_item_drop_from_actor(PBB_ItemWorld *world,
                             int actor_id,
                             int def_id,
                             int amount,
                             PBB_Fixed offset_x,
                             PBB_Fixed offset_y)
{
    PBB_ItemActor *actor;

    actor = pbb_item_actor_get(world, actor_id);
    if (actor == 0) {
        return PBB_ITEM_INVALID_ID;
    }

    return pbb_item_drop_at(world,
                            actor_id,
                            def_id,
                            amount,
                            actor->x + offset_x,
                            actor->y + offset_y);
}

int pbb_item_drop_at(PBB_ItemWorld *world,
                     int actor_id,
                     int def_id,
                     int amount,
                     PBB_Fixed x,
                     PBB_Fixed y)
{
    PBB_ItemDef *def;
    PBB_Item *item;
    int item_id;
    int pass;

    if (world == 0) {
        return PBB_ITEM_INVALID_ID;
    }

    if (actor_id != PBB_ITEM_INVALID_ID) {
        if (pbb_item_actor_get(world, actor_id) == 0) {
            return PBB_ITEM_INVALID_ID;
        }
    }

    def = pbb_item_def_get(world, def_id);
    if (def == 0) {
        return PBB_ITEM_INVALID_ID;
    }

    if (world->strict_droppable && ((def->default_flags & PBB_ITEMF_DROPPABLE) == 0UL)) {
        pbb_item_event_push(world,
                            PBB_EVENT_ITEM_RULE_BLOCKED,
                            actor_id,
                            PBB_ITEM_INVALID_ID,
                            def_id,
                            PBB_ITEM_HOOK_DROP,
                            PBB_ITEM_BLOCK_REASON_NOT_DROPPABLE,
                            0,
                            x,
                            y);
        return PBB_ITEM_INVALID_ID;
    }

    item_id = pbb_item_i_spawn_internal(world,
                                      def_id,
                                      x,
                                      y,
                                      amount,
                                      PBB_ITEM_STATE_DROPPED,
                                      0,
                                      0);
    if (item_id == PBB_ITEM_INVALID_ID) {
        return PBB_ITEM_INVALID_ID;
    }

    item = pbb_item_get(world, item_id);
    if (item == 0) {
        return PBB_ITEM_INVALID_ID;
    }

    item->owner_actor_id = actor_id;

    pass = pbb_item_eval_rule_block(world,
                                    actor_id,
                                    item_id,
                                    def->rule_first[PBB_ITEM_HOOK_DROP],
                                    def->rule_count[PBB_ITEM_HOOK_DROP]);
    if (!pass) {
        pbb_item_event_push(world,
                            PBB_EVENT_ITEM_RULE_BLOCKED,
                            actor_id,
                            item_id,
                            def_id,
                            PBB_ITEM_HOOK_DROP,
                            0,
                            0,
                            x,
                            y);
        pbb_item_destroy(world, item_id);
        return PBB_ITEM_INVALID_ID;
    }

    pbb_item_apply_effect_block(world,
                                actor_id,
                                item_id,
                                def->effect_first[PBB_ITEM_HOOK_SPAWN],
                                def->effect_count[PBB_ITEM_HOOK_SPAWN]);

    item = pbb_item_get(world, item_id);
    if (item == 0) {
        return PBB_ITEM_INVALID_ID;
    }

    pbb_item_event_push(world,
                        PBB_EVENT_ITEM_SPAWNED,
                        PBB_ITEM_INVALID_ID,
                        item_id,
                        def_id,
                        item->amount,
                        PBB_ITEM_STATE_DROPPED,
                        0,
                        item->x,
                        item->y);

    pbb_item_apply_effect_block(world,
                                actor_id,
                                item_id,
                                def->effect_first[PBB_ITEM_HOOK_DROP],
                                def->effect_count[PBB_ITEM_HOOK_DROP]);

    item = pbb_item_get(world, item_id);
    if (item != 0) {
        item->drop_count += 1;
    }

    pbb_item_event_push(world,
                        PBB_EVENT_ITEM_DROPPED,
                        actor_id,
                        item_id,
                        def_id,
                        amount,
                        0,
                        0,
                        x,
                        y);

    return item_id;
}

