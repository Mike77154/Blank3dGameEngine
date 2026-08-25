#include "pbb_item_internal.h"

static int pbb_item_eval_one_rule(PBB_ItemWorld *world,
                                  int actor_id,
                                  int item_id,
                                  const PBB_ItemRule *rule)
{
    PBB_ItemActor *actor;
    PBB_Item *item;
    int value;

    if (rule == 0) {
        return 0;
    }

    if (rule->type == PBB_RULE_NONE || rule->type == PBB_RULE_ALWAYS) {
        return 1;
    }

    actor = pbb_item_actor_get(world, actor_id);
    item = pbb_item_get(world, item_id);

    switch (rule->type) {
        case PBB_RULE_ACTOR_CLASS_ANY:
            if (actor == 0) {
                return 0;
            }
            return ((actor->class_mask & (unsigned long)rule->a) != 0UL);

        case PBB_RULE_ACTOR_CLASS_NONE:
            if (actor == 0) {
                return 0;
            }
            return ((actor->class_mask & (unsigned long)rule->a) == 0UL);

        case PBB_RULE_ACTOR_TEAM_ANY:
            if (actor == 0) {
                return 0;
            }
            return ((actor->team_mask & (unsigned long)rule->a) != 0UL);

        case PBB_RULE_ACTOR_TEAM_NONE:
            if (actor == 0) {
                return 0;
            }
            return ((actor->team_mask & (unsigned long)rule->a) == 0UL);

        case PBB_RULE_ACTOR_FLAG_SET:
            if (actor == 0) {
                return 0;
            }
            return pbb_item_i_flag_get_words(actor->flags, rule->a);

        case PBB_RULE_ACTOR_FLAG_CLEAR:
            if (actor == 0) {
                return 0;
            }
            return !pbb_item_i_flag_get_words(actor->flags, rule->a);

        case PBB_RULE_WORLD_FLAG_SET:
            return pbb_item_i_flag_get_words(world->world_flags, rule->a);

        case PBB_RULE_WORLD_FLAG_CLEAR:
            return !pbb_item_i_flag_get_words(world->world_flags, rule->a);

        case PBB_RULE_ITEM_FLAG_SET:
            if (item == 0) {
                return 0;
            }
            return ((item->flags & (unsigned long)rule->a) != 0UL);

        case PBB_RULE_ITEM_FLAG_CLEAR:
            if (item == 0) {
                return 0;
            }
            return ((item->flags & (unsigned long)rule->a) == 0UL);

        case PBB_RULE_ACTOR_VAR_EQ:
            if (actor == 0 || !pbb_item_i_var_valid(rule->a, PBB_ITEM_MAX_ACTOR_VARS)) {
                return 0;
            }
            return (actor->vars[rule->a] == rule->b);

        case PBB_RULE_ACTOR_VAR_NE:
            if (actor == 0 || !pbb_item_i_var_valid(rule->a, PBB_ITEM_MAX_ACTOR_VARS)) {
                return 0;
            }
            return (actor->vars[rule->a] != rule->b);

        case PBB_RULE_ACTOR_VAR_GTE:
            if (actor == 0 || !pbb_item_i_var_valid(rule->a, PBB_ITEM_MAX_ACTOR_VARS)) {
                return 0;
            }
            return (actor->vars[rule->a] >= rule->b);

        case PBB_RULE_ACTOR_VAR_LTE:
            if (actor == 0 || !pbb_item_i_var_valid(rule->a, PBB_ITEM_MAX_ACTOR_VARS)) {
                return 0;
            }
            return (actor->vars[rule->a] <= rule->b);

        case PBB_RULE_WORLD_VAR_EQ:
            if (!pbb_item_i_var_valid(rule->a, PBB_ITEM_MAX_WORLD_VARS)) {
                return 0;
            }
            return (world->world_vars[rule->a] == rule->b);

        case PBB_RULE_WORLD_VAR_NE:
            if (!pbb_item_i_var_valid(rule->a, PBB_ITEM_MAX_WORLD_VARS)) {
                return 0;
            }
            return (world->world_vars[rule->a] != rule->b);

        case PBB_RULE_WORLD_VAR_GTE:
            if (!pbb_item_i_var_valid(rule->a, PBB_ITEM_MAX_WORLD_VARS)) {
                return 0;
            }
            return (world->world_vars[rule->a] >= rule->b);

        case PBB_RULE_WORLD_VAR_LTE:
            if (!pbb_item_i_var_valid(rule->a, PBB_ITEM_MAX_WORLD_VARS)) {
                return 0;
            }
            return (world->world_vars[rule->a] <= rule->b);

        case PBB_RULE_ITEM_CUSTOM_EQ:
            if (item == 0 || !pbb_item_i_custom_valid(rule->a)) {
                return 0;
            }
            return (item->custom[rule->a] == rule->b);

        case PBB_RULE_ITEM_CUSTOM_NE:
            if (item == 0 || !pbb_item_i_custom_valid(rule->a)) {
                return 0;
            }
            return (item->custom[rule->a] != rule->b);

        case PBB_RULE_ITEM_CUSTOM_GTE:
            if (item == 0 || !pbb_item_i_custom_valid(rule->a)) {
                return 0;
            }
            return (item->custom[rule->a] >= rule->b);

        case PBB_RULE_ITEM_CUSTOM_LTE:
            if (item == 0 || !pbb_item_i_custom_valid(rule->a)) {
                return 0;
            }
            return (item->custom[rule->a] <= rule->b);

        case PBB_RULE_ITEM_STATE_EQ:
            if (item == 0) {
                return 0;
            }
            return (item->state == rule->a);

        case PBB_RULE_ITEM_STATE_NE:
            if (item == 0) {
                return 0;
            }
            return (item->state != rule->a);

        case PBB_RULE_ITEM_AMOUNT_GTE:
            if (item == 0) {
                return 0;
            }
            return (item->amount >= rule->a);

        case PBB_RULE_ITEM_AMOUNT_LTE:
            if (item == 0) {
                return 0;
            }
            return (item->amount <= rule->a);

        case PBB_RULE_TOUCH_COUNT_LT:
            if (item == 0) {
                return 0;
            }
            return (item->touch_count < rule->a);

        case PBB_RULE_INTERACT_COUNT_LT:
            if (item == 0) {
                return 0;
            }
            return (item->interact_count < rule->a);

        case PBB_RULE_OWNER_IS_ACTOR:
            if (item == 0 || actor == 0) {
                return 0;
            }
            return (item->owner_actor_id == actor_id);

        case PBB_RULE_OWNER_IS_NOT_ACTOR:
            if (item == 0 || actor == 0) {
                return 0;
            }
            return (item->owner_actor_id != actor_id);

        case PBB_RULE_ACTOR_VAR_GTE_ITEM_AMOUNT:
            if (actor == 0 || item == 0 || !pbb_item_i_var_valid(rule->a, PBB_ITEM_MAX_ACTOR_VARS)) {
                return 0;
            }
            value = item->amount;
            if (rule->b != 0) {
                value = item->amount * rule->b;
            }
            return (actor->vars[rule->a] >= value);

        case PBB_RULE_CALL_CUSTOM:
            if (world->custom_rule == 0) {
                return 0;
            }
            return world->custom_rule(world, actor_id, item_id, rule);

        default:
            break;
    }

    return 0;
}

int pbb_item_eval_rule_block(PBB_ItemWorld *world,
                             int actor_id,
                             int item_id,
                             int first_rule,
                             int rule_count)
{
    int i;
    int rule_id;
    PBB_ItemRule *rule;

    if (world == 0) {
        return 0;
    }

    if (rule_count <= 0) {
        return 1;
    }

    if (first_rule < 0) {
        return 0;
    }

    if (first_rule + rule_count > PBB_ITEM_MAX_RULES) {
        return 0;
    }

    for (i = 0; i < rule_count; ++i) {
        rule_id = first_rule + i;
        rule = pbb_item_rule_get(world, rule_id);
        if (rule == 0) {
            return 0;
        }
        if (!pbb_item_eval_one_rule(world, actor_id, item_id, rule)) {
            return 0;
        }
    }

    return 1;
}

