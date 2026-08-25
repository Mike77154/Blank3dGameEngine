#include "pbb_item_internal.h"
#include <string.h>

int pbb_item_apply_effect_block(PBB_ItemWorld *world,
                                int actor_id,
                                int item_id,
                                int first_effect,
                                int effect_count)
{
    int i;
    int effect_id;
    int applied;
    PBB_ItemEffect *effect;

    if (world == 0) {
        return 0;
    }

    if (effect_count <= 0) {
        return 0;
    }

    if (first_effect < 0) {
        return 0;
    }

    if (first_effect + effect_count > PBB_ITEM_MAX_EFFECTS) {
        return 0;
    }

    applied = 0;
    for (i = 0; i < effect_count; ++i) {
        effect_id = first_effect + i;
        effect = pbb_item_effect_get(world, effect_id);
        if (effect != 0) {
            if (pbb_item_apply_effect(world, actor_id, item_id, effect)) {
                applied += 1;
            }
        }
    }

    return applied;
}

int pbb_item_apply_effect(PBB_ItemWorld *world,
                          int actor_id,
                          int item_id,
                          const PBB_ItemEffect *effect)
{
    PBB_ItemActor *actor;
    PBB_Item *item;
    int spawned;
    int event_type;
    int delta;
    int min_value;
    int max_value;
    int def_id;
    PBB_Fixed x;
    PBB_Fixed y;

    if (world == 0 || effect == 0) {
        return 0;
    }

    if (effect->type == PBB_EFFECT_NONE) {
        return 1;
    }

    actor = pbb_item_actor_get(world, actor_id);
    item = pbb_item_get(world, item_id);

    switch (effect->type) {
        case PBB_EFFECT_ADD_ACTOR_VAR:
            if (actor == 0 || !pbb_item_i_var_valid(effect->a, PBB_ITEM_MAX_ACTOR_VARS)) {
                return 0;
            }
            actor->vars[effect->a] += effect->b;
            return 1;

        case PBB_EFFECT_SET_ACTOR_VAR:
            if (actor == 0 || !pbb_item_i_var_valid(effect->a, PBB_ITEM_MAX_ACTOR_VARS)) {
                return 0;
            }
            actor->vars[effect->a] = effect->b;
            return 1;

        case PBB_EFFECT_CLAMP_ACTOR_VAR:
            if (actor == 0 || !pbb_item_i_var_valid(effect->a, PBB_ITEM_MAX_ACTOR_VARS)) {
                return 0;
            }
            min_value = effect->b;
            max_value = effect->c;
            if (actor->vars[effect->a] < min_value) {
                actor->vars[effect->a] = min_value;
            }
            if (actor->vars[effect->a] > max_value) {
                actor->vars[effect->a] = max_value;
            }
            return 1;

        case PBB_EFFECT_ADD_WORLD_VAR:
            if (!pbb_item_i_var_valid(effect->a, PBB_ITEM_MAX_WORLD_VARS)) {
                return 0;
            }
            world->world_vars[effect->a] += effect->b;
            return 1;

        case PBB_EFFECT_SET_WORLD_VAR:
            if (!pbb_item_i_var_valid(effect->a, PBB_ITEM_MAX_WORLD_VARS)) {
                return 0;
            }
            world->world_vars[effect->a] = effect->b;
            return 1;

        case PBB_EFFECT_CLAMP_WORLD_VAR:
            if (!pbb_item_i_var_valid(effect->a, PBB_ITEM_MAX_WORLD_VARS)) {
                return 0;
            }
            min_value = effect->b;
            max_value = effect->c;
            if (world->world_vars[effect->a] < min_value) {
                world->world_vars[effect->a] = min_value;
            }
            if (world->world_vars[effect->a] > max_value) {
                world->world_vars[effect->a] = max_value;
            }
            return 1;

        case PBB_EFFECT_SET_ACTOR_FLAG:
            if (actor == 0) {
                return 0;
            }
            return pbb_item_i_flag_set_words(actor->flags, effect->a, 1);

        case PBB_EFFECT_CLEAR_ACTOR_FLAG:
            if (actor == 0) {
                return 0;
            }
            return pbb_item_i_flag_set_words(actor->flags, effect->a, 0);

        case PBB_EFFECT_TOGGLE_ACTOR_FLAG:
            if (actor == 0 || !pbb_item_i_flag_id_valid(effect->a)) {
                return 0;
            }
            if (pbb_item_i_flag_get_words(actor->flags, effect->a)) {
                pbb_item_i_flag_set_words(actor->flags, effect->a, 0);
            } else {
                pbb_item_i_flag_set_words(actor->flags, effect->a, 1);
            }
            return 1;

        case PBB_EFFECT_SET_WORLD_FLAG:
            return pbb_item_i_flag_set_words(world->world_flags, effect->a, 1);

        case PBB_EFFECT_CLEAR_WORLD_FLAG:
            return pbb_item_i_flag_set_words(world->world_flags, effect->a, 0);

        case PBB_EFFECT_TOGGLE_WORLD_FLAG:
            if (!pbb_item_i_flag_id_valid(effect->a)) {
                return 0;
            }
            if (pbb_item_i_flag_get_words(world->world_flags, effect->a)) {
                pbb_item_i_flag_set_words(world->world_flags, effect->a, 0);
            } else {
                pbb_item_i_flag_set_words(world->world_flags, effect->a, 1);
            }
            return 1;

        case PBB_EFFECT_SET_ITEM_FLAG:
            if (item == 0) {
                return 0;
            }
            item->flags |= (unsigned long)effect->a;
            return 1;

        case PBB_EFFECT_CLEAR_ITEM_FLAG:
            if (item == 0) {
                return 0;
            }
            item->flags &= ~((unsigned long)effect->a);
            return 1;

        case PBB_EFFECT_TOGGLE_ITEM_FLAG:
            if (item == 0) {
                return 0;
            }
            item->flags ^= (unsigned long)effect->a;
            return 1;

        case PBB_EFFECT_SET_ITEM_STATE:
            if (item == 0) {
                return 0;
            }
            item->state = effect->a;
            return 1;

        case PBB_EFFECT_HIDE_ITEM:
            return pbb_item_hide(world, item_id);

        case PBB_EFFECT_SHOW_ITEM:
            return pbb_item_show(world, item_id);

        case PBB_EFFECT_CONSUME_ITEM:
            if (item == 0) {
                return 0;
            }
            def_id = item->def_id;
            x = item->x;
            y = item->y;
            item->state = PBB_ITEM_STATE_CONSUMED;
            item->flags &= ~(PBB_ITEMF_ACTIVE | PBB_ITEMF_VISIBLE | PBB_ITEMF_TOUCHABLE | PBB_ITEMF_INTERACTABLE);
            pbb_item_event_push(world,
                                PBB_EVENT_ITEM_CONSUMED,
                                actor_id,
                                item_id,
                                def_id,
                                0,
                                0,
                                0,
                                x,
                                y);
            if ((world->recycle_consumed_items ||
                 ((item->flags & PBB_ITEMF_RECYCLE_ON_CONSUME) != 0UL)) &&
                ((item->flags & PBB_ITEMF_PERSISTENT) == 0UL)) {
                memset(item, 0, sizeof(*item));
            }
            return 1;

        case PBB_EFFECT_DESTROY_ITEM:
            return pbb_item_destroy(world, item_id);

        case PBB_EFFECT_SET_ITEM_AMOUNT:
            if (item == 0) {
                return 0;
            }
            item->amount = effect->a;
            return 1;

        case PBB_EFFECT_ADD_ITEM_AMOUNT:
            if (item == 0) {
                return 0;
            }
            item->amount += effect->a;
            if (item->amount < 0) {
                item->amount = 0;
            }
            return 1;

        case PBB_EFFECT_SET_ITEM_CUSTOM:
            if (item == 0 || !pbb_item_i_custom_valid(effect->a)) {
                return 0;
            }
            item->custom[effect->a] = effect->b;
            return 1;

        case PBB_EFFECT_ADD_ITEM_CUSTOM:
            if (item == 0 || !pbb_item_i_custom_valid(effect->a)) {
                return 0;
            }
            item->custom[effect->a] += effect->b;
            return 1;

        case PBB_EFFECT_ADD_ACTOR_VAR_FROM_ITEM_AMOUNT:
            if (actor == 0 || item == 0 || !pbb_item_i_var_valid(effect->a, PBB_ITEM_MAX_ACTOR_VARS)) {
                return 0;
            }
            delta = item->amount * effect->b;
            actor->vars[effect->a] += delta;
            return 1;

        case PBB_EFFECT_ADD_WORLD_VAR_FROM_ITEM_AMOUNT:
            if (item == 0 || !pbb_item_i_var_valid(effect->a, PBB_ITEM_MAX_WORLD_VARS)) {
                return 0;
            }
            delta = item->amount * effect->b;
            world->world_vars[effect->a] += delta;
            return 1;

        case PBB_EFFECT_SPAWN_ITEM_AT_ITEM:
            if (item == 0) {
                return 0;
            }
            spawned = pbb_item_i_spawn_internal(world,
                                              effect->a,
                                              item->x + effect->fx,
                                              item->y + effect->fy,
                                              effect->b,
                                              PBB_ITEM_STATE_WORLD,
                                              1,
                                              1);
            if (spawned == PBB_ITEM_INVALID_ID) {
                return 0;
            }
            return 1;

        case PBB_EFFECT_SPAWN_ITEM_AT_ACTOR:
            if (actor == 0) {
                return 0;
            }
            spawned = pbb_item_i_spawn_internal(world,
                                              effect->a,
                                              actor->x + effect->fx,
                                              actor->y + effect->fy,
                                              effect->b,
                                              PBB_ITEM_STATE_WORLD,
                                              1,
                                              1);
            if (spawned == PBB_ITEM_INVALID_ID) {
                return 0;
            }
            return 1;

        case PBB_EFFECT_SET_ITEM_OWNER_ACTOR:
            if (item == 0 || actor == 0) {
                return 0;
            }
            item->owner_actor_id = actor_id;
            return 1;

        case PBB_EFFECT_CLEAR_ITEM_OWNER:
            if (item == 0) {
                return 0;
            }
            item->owner_actor_id = PBB_ITEM_INVALID_ID;
            return 1;

        case PBB_EFFECT_MOVE_ITEM_TO_ACTOR:
            if (item == 0 || actor == 0) {
                return 0;
            }
            item->x = actor->x + effect->fx;
            item->y = actor->y + effect->fy;
            return 1;

        case PBB_EFFECT_EMIT_EVENT:
            event_type = effect->a;
            if (event_type == PBB_EVENT_NONE) {
                event_type = PBB_EVENT_CUSTOM;
            }
            def_id = PBB_ITEM_INVALID_ID;
            x = 0;
            y = 0;
            if (item != 0) {
                def_id = item->def_id;
                x = item->x;
                y = item->y;
            } else if (actor != 0) {
                x = actor->x;
                y = actor->y;
            }
            return pbb_item_event_push(world,
                                       event_type,
                                       actor_id,
                                       item_id,
                                       def_id,
                                       effect->b,
                                       effect->c,
                                       effect->d,
                                       x,
                                       y);

        case PBB_EFFECT_CALL_CUSTOM:
            if (world->custom_effect == 0) {
                return 0;
            }
            return world->custom_effect(world, actor_id, item_id, effect);

        default:
            break;
    }

    return 0;
}

