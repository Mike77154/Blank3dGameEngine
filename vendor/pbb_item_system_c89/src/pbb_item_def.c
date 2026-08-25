#include "pbb_item_internal.h"
#include <string.h>

int pbb_item_def_create(PBB_ItemWorld *world, const char *name)
{
    PBB_ItemDef *def;
    int i;

    if (world == 0) {
        return PBB_ITEM_INVALID_ID;
    }

    for (i = 0; i < PBB_ITEM_MAX_DEFS; ++i) {
        if (!world->defs[i].used) {
            def = &world->defs[i];
            memset(def, 0, sizeof(*def));
            def->used = 1;
            def->def_id = i;
            pbb_item_i_copy_name(def->name, name);
            def->default_flags = PBB_ITEMF_ACTIVE | PBB_ITEMF_VISIBLE | PBB_ITEMF_TOUCHABLE;
            def->default_category_mask = PBB_ITEM_CATEGORY_ANY;
            def->default_amount = 1;
            def->default_half_w = PBB_FIXED_HALF;
            def->default_half_h = PBB_FIXED_HALF;
            return i;
        }
    }

    return PBB_ITEM_INVALID_ID;
}

PBB_ItemDef *pbb_item_def_get(PBB_ItemWorld *world, int def_id)
{
    if (world == 0) {
        return 0;
    }

    if (def_id < 0 || def_id >= PBB_ITEM_MAX_DEFS) {
        return 0;
    }

    if (!world->defs[def_id].used) {
        return 0;
    }

    return &world->defs[def_id];
}

const PBB_ItemDef *pbb_item_def_get_const(const PBB_ItemWorld *world, int def_id)
{
    if (world == 0) {
        return 0;
    }

    if (def_id < 0 || def_id >= PBB_ITEM_MAX_DEFS) {
        return 0;
    }

    if (!world->defs[def_id].used) {
        return 0;
    }

    return &world->defs[def_id];
}

int pbb_item_def_set_defaults(PBB_ItemWorld *world,
                              int def_id,
                              unsigned long default_flags,
                              int default_amount,
                              PBB_Fixed default_half_w,
                              PBB_Fixed default_half_h)
{
    PBB_ItemDef *def;

    def = pbb_item_def_get(world, def_id);
    if (def == 0) {
        return 0;
    }

    def->default_flags = default_flags;
    def->default_amount = default_amount;
    def->default_half_w = default_half_w;
    def->default_half_h = default_half_h;
    return 1;
}

int pbb_item_def_set_category(PBB_ItemWorld *world,
                              int def_id,
                              unsigned long category_mask)
{
    PBB_ItemDef *def;

    def = pbb_item_def_get(world, def_id);
    if (def == 0) {
        return 0;
    }

    def->default_category_mask = category_mask;
    return 1;
}

int pbb_item_def_set_rule_block(PBB_ItemWorld *world,
                                int def_id,
                                int hook,
                                int first_rule,
                                int rule_count)
{
    PBB_ItemDef *def;

    def = pbb_item_def_get(world, def_id);
    if (def == 0) {
        return 0;
    }

    if (!pbb_item_i_hook_valid(hook)) {
        return 0;
    }

    if (rule_count < 0) {
        return 0;
    }

    if (rule_count > 0) {
        if (first_rule < 0) {
            return 0;
        }
        if (first_rule + rule_count > PBB_ITEM_MAX_RULES) {
            return 0;
        }
    }

    def->rule_first[hook] = first_rule;
    def->rule_count[hook] = rule_count;
    return 1;
}

int pbb_item_def_set_effect_block(PBB_ItemWorld *world,
                                  int def_id,
                                  int hook,
                                  int first_effect,
                                  int effect_count)
{
    PBB_ItemDef *def;

    def = pbb_item_def_get(world, def_id);
    if (def == 0) {
        return 0;
    }

    if (!pbb_item_i_hook_valid(hook)) {
        return 0;
    }

    if (effect_count < 0) {
        return 0;
    }

    if (effect_count > 0) {
        if (first_effect < 0) {
            return 0;
        }
        if (first_effect + effect_count > PBB_ITEM_MAX_EFFECTS) {
            return 0;
        }
    }

    def->effect_first[hook] = first_effect;
    def->effect_count[hook] = effect_count;
    return 1;
}

int pbb_item_rule_add(PBB_ItemWorld *world,
                      int type,
                      int a,
                      int b,
                      int c,
                      int d)
{
    int i;
    PBB_ItemRule *rule;

    if (world == 0) {
        return PBB_ITEM_INVALID_ID;
    }

    for (i = 0; i < PBB_ITEM_MAX_RULES; ++i) {
        if (!world->rules[i].used) {
            rule = &world->rules[i];
            memset(rule, 0, sizeof(*rule));
            rule->used = 1;
            rule->type = type;
            rule->a = a;
            rule->b = b;
            rule->c = c;
            rule->d = d;
            return i;
        }
    }

    return PBB_ITEM_INVALID_ID;
}

PBB_ItemRule *pbb_item_rule_get(PBB_ItemWorld *world, int rule_id)
{
    if (world == 0) {
        return 0;
    }

    if (rule_id < 0 || rule_id >= PBB_ITEM_MAX_RULES) {
        return 0;
    }

    if (!world->rules[rule_id].used) {
        return 0;
    }

    return &world->rules[rule_id];
}

int pbb_item_effect_add(PBB_ItemWorld *world,
                        int type,
                        int a,
                        int b,
                        int c,
                        int d,
                        PBB_Fixed fx,
                        PBB_Fixed fy)
{
    int i;
    PBB_ItemEffect *effect;

    if (world == 0) {
        return PBB_ITEM_INVALID_ID;
    }

    for (i = 0; i < PBB_ITEM_MAX_EFFECTS; ++i) {
        if (!world->effects[i].used) {
            effect = &world->effects[i];
            memset(effect, 0, sizeof(*effect));
            effect->used = 1;
            effect->type = type;
            effect->a = a;
            effect->b = b;
            effect->c = c;
            effect->d = d;
            effect->fx = fx;
            effect->fy = fy;
            return i;
        }
    }

    return PBB_ITEM_INVALID_ID;
}

PBB_ItemEffect *pbb_item_effect_get(PBB_ItemWorld *world, int effect_id)
{
    if (world == 0) {
        return 0;
    }

    if (effect_id < 0 || effect_id >= PBB_ITEM_MAX_EFFECTS) {
        return 0;
    }

    if (!world->effects[effect_id].used) {
        return 0;
    }

    return &world->effects[effect_id];
}

