#include <assert.h>
#include "pbb_item_system.h"

enum {
    VAR_HP = 0,
    VAR_MONEY = 1,
    VAR_HELD = 2
};

static int count_live_items(PBB_ItemWorld *world)
{
    int i;
    int count;

    count = 0;
    for (i = 0; i < PBB_ITEM_MAX_ITEMS; ++i) {
        if (world->items[i].used) {
            count += 1;
        }
    }

    return count;
}

static int make_coin(PBB_ItemWorld *world)
{
    int def;
    int first;
    int e;

    def = pbb_item_def_create(world, "coin");
    assert(def != PBB_ITEM_INVALID_ID);
    pbb_item_def_set_defaults(world,
                              def,
                              PBB_ITEMF_ACTIVE | PBB_ITEMF_VISIBLE | PBB_ITEMF_TOUCHABLE,
                              1,
                              PBB_FIXED_HALF,
                              PBB_FIXED_HALF);
    pbb_item_def_set_category(world, def, PBB_ITEM_CATEGORY_PICKUP);

    first = pbb_item_effect_add(world,
                                PBB_EFFECT_ADD_ACTOR_VAR_FROM_ITEM_AMOUNT,
                                VAR_MONEY,
                                1,
                                0,
                                0,
                                0,
                                0);
    assert(first != PBB_ITEM_INVALID_ID);
    e = pbb_item_effect_add(world,
                            PBB_EFFECT_CONSUME_ITEM,
                            0,
                            0,
                            0,
                            0,
                            0,
                            0);
    assert(e == first + 1);
    assert(pbb_item_def_set_effect_block(world, def, PBB_ITEM_HOOK_TOUCH, first, 2));
    return def;
}

static int make_potion(PBB_ItemWorld *world)
{
    int def;
    int first_rule;
    int first_drop_effect;
    int first_touch_effect;
    int e;

    def = pbb_item_def_create(world, "potion");
    assert(def != PBB_ITEM_INVALID_ID);
    pbb_item_def_set_defaults(world,
                              def,
                              PBB_ITEMF_ACTIVE | PBB_ITEMF_VISIBLE | PBB_ITEMF_TOUCHABLE | PBB_ITEMF_DROPPABLE,
                              1,
                              PBB_FIXED_HALF,
                              PBB_FIXED_HALF);
    pbb_item_def_set_category(world, def, PBB_ITEM_CATEGORY_PICKUP);

    first_rule = pbb_item_rule_add(world,
                                   PBB_RULE_ACTOR_VAR_GTE_ITEM_AMOUNT,
                                   VAR_HELD,
                                   1,
                                   0,
                                   0);
    assert(first_rule != PBB_ITEM_INVALID_ID);
    assert(pbb_item_def_set_rule_block(world, def, PBB_ITEM_HOOK_DROP, first_rule, 1));

    first_drop_effect = pbb_item_effect_add(world,
                                            PBB_EFFECT_ADD_ACTOR_VAR_FROM_ITEM_AMOUNT,
                                            VAR_HELD,
                                            -1,
                                            0,
                                            0,
                                            0,
                                            0);
    assert(first_drop_effect != PBB_ITEM_INVALID_ID);
    assert(pbb_item_def_set_effect_block(world, def, PBB_ITEM_HOOK_DROP, first_drop_effect, 1));

    first_touch_effect = pbb_item_effect_add(world,
                                             PBB_EFFECT_ADD_ACTOR_VAR_FROM_ITEM_AMOUNT,
                                             VAR_HP,
                                             10,
                                             0,
                                             0,
                                             0,
                                             0);
    assert(first_touch_effect != PBB_ITEM_INVALID_ID);
    e = pbb_item_effect_add(world,
                            PBB_EFFECT_CONSUME_ITEM,
                            0,
                            0,
                            0,
                            0,
                            0,
                            0);
    assert(e == first_touch_effect + 1);
    assert(pbb_item_def_set_effect_block(world, def, PBB_ITEM_HOOK_TOUCH, first_touch_effect, 2));

    return def;
}

int main(void)
{
    PBB_ItemWorld world;
    int player;
    int coin_def;
    int potion_def;
    int loot_def;
    int item;
    int dropped;
    int before_count;
    int after_count;
    int gc_count;
    PBB_ItemIter iter;
    PBB_Item *iter_item;
    PBB_ItemSaveState save;

    pbb_item_world_init(&world);

    coin_def = make_coin(&world);
    potion_def = make_potion(&world);

    player = pbb_item_actor_create(&world,
                                   PBB_ACTOR_CLASS_PLAYER,
                                   1UL,
                                   PBB_FIXED_FROM_INT(5),
                                   PBB_FIXED_FROM_INT(5),
                                   PBB_FIXED_HALF,
                                   PBB_FIXED_HALF);
    assert(player != PBB_ITEM_INVALID_ID);
    assert(pbb_item_actor_set_var(&world, player, VAR_HP, 10));
    assert(pbb_item_actor_set_var(&world, player, VAR_MONEY, 0));
    assert(pbb_item_actor_set_var(&world, player, VAR_HELD, 0));
    assert(pbb_item_actor_set_masks(&world, player, PBB_ITEM_CATEGORY_PICKUP, PBB_ITEM_CATEGORY_ANY));

    item = pbb_item_spawn(&world, coin_def, PBB_FIXED_FROM_INT(5), PBB_FIXED_FROM_INT(5), 7);
    assert(item != PBB_ITEM_INVALID_ID);
    assert(pbb_item_touch_actor_all(&world, player) == 1);
    assert(pbb_item_actor_get_var(&world, player, VAR_MONEY) == 7);
    assert(pbb_item_get(&world, item) != 0);
    assert(pbb_item_get(&world, item)->state == PBB_ITEM_STATE_CONSUMED);

    pbb_item_event_clear(&world);
    assert(count_live_items(&world) == 1);
    dropped = pbb_item_drop_from_actor(&world,
                                       player,
                                       potion_def,
                                       1,
                                       PBB_FIXED_FROM_INT(1),
                                       0);
    assert(dropped == PBB_ITEM_INVALID_ID);
    assert(pbb_item_actor_get_var(&world, player, VAR_HELD) == 0);
    assert(count_live_items(&world) == 1);

    assert(pbb_item_actor_set_var(&world, player, VAR_HELD, 2));
    dropped = pbb_item_drop_from_actor(&world,
                                       player,
                                       potion_def,
                                       1,
                                       PBB_FIXED_FROM_INT(1),
                                       0);
    assert(dropped != PBB_ITEM_INVALID_ID);
    assert(pbb_item_actor_get_var(&world, player, VAR_HELD) == 1);

    pbb_item_actor_set_box(&world,
                           player,
                           PBB_FIXED_FROM_INT(6),
                           PBB_FIXED_FROM_INT(5),
                           PBB_FIXED_HALF,
                           PBB_FIXED_HALF);
    assert(pbb_item_touch_actor_all(&world, player) == 1);
    assert(pbb_item_actor_get_var(&world, player, VAR_HP) == 20);

    /* v0.2: category masks can block touches before rules/effects. */
    loot_def = make_coin(&world);
    assert(loot_def != PBB_ITEM_INVALID_ID);
    assert(pbb_item_def_set_category(&world, loot_def, PBB_ITEM_CATEGORY_LOOT));
    assert(pbb_item_actor_set_masks(&world, player, PBB_ITEM_CATEGORY_PICKUP, PBB_ITEM_CATEGORY_ANY));
    pbb_item_actor_set_box(&world, player, PBB_FIXED_FROM_INT(6), PBB_FIXED_FROM_INT(5), PBB_FIXED_HALF, PBB_FIXED_HALF);
    item = pbb_item_spawn(&world, loot_def, PBB_FIXED_FROM_INT(6), PBB_FIXED_FROM_INT(5), 3);
    assert(item != PBB_ITEM_INVALID_ID);
    assert(pbb_item_touch_actor_all(&world, player) == 0);
    assert(pbb_item_actor_get_var(&world, player, VAR_MONEY) == 7);
    assert(pbb_item_actor_set_masks(&world, player, PBB_ITEM_CATEGORY_PICKUP | PBB_ITEM_CATEGORY_LOOT, PBB_ITEM_CATEGORY_ANY));
    assert(pbb_item_touch_actor_all(&world, player) == 1);
    assert(pbb_item_actor_get_var(&world, player, VAR_MONEY) == 10);

    /* v0.2: strict droppable is opt-in and honors PBB_ITEMF_DROPPABLE. */
    pbb_item_world_set_strict_droppable(&world, 1);
    dropped = pbb_item_drop_from_actor(&world, player, coin_def, 1, 0, 0);
    assert(dropped == PBB_ITEM_INVALID_ID);
    assert(pbb_item_actor_set_var(&world, player, VAR_HELD, 0));
    dropped = pbb_item_drop_from_actor(&world, player, potion_def, 1, PBB_FIXED_FROM_INT(1), 0);
    assert(dropped == PBB_ITEM_INVALID_ID);
    assert(pbb_item_actor_set_var(&world, player, VAR_HELD, 1));
    dropped = pbb_item_drop_from_actor(&world, player, potion_def, 1, PBB_FIXED_FROM_INT(1), 0);
    assert(dropped != PBB_ITEM_INVALID_ID);

    /* v0.2: item iterators provide renderer/query-friendly access. */
    item = pbb_item_spawn(&world, coin_def, PBB_FIXED_FROM_INT(9), PBB_FIXED_FROM_INT(5), 1);
    assert(item != PBB_ITEM_INVALID_ID);
    pbb_item_iter_begin(&iter,
                        PBB_ITEMF_VISIBLE,
                        0UL,
                        PBB_ITEM_STATE_WORLD,
                        PBB_ITEM_CATEGORY_PICKUP);
    assert(pbb_item_iter_next(&world, &iter, &iter_item));
    assert(iter_item != 0);

    /* v0.2: consumed slots can be garbage-collected, or auto-recycled globally. */
    before_count = pbb_item_count_used(&world);
    gc_count = pbb_item_gc_consumed(&world);
    after_count = pbb_item_count_used(&world);
    assert(gc_count > 0);
    assert(after_count == before_count - gc_count);

    pbb_item_world_set_recycle_consumed(&world, 1);
    before_count = pbb_item_count_used(&world);
    pbb_item_actor_set_box(&world, player, PBB_FIXED_FROM_INT(9), PBB_FIXED_FROM_INT(5), PBB_FIXED_HALF, PBB_FIXED_HALF);
    item = pbb_item_spawn(&world, coin_def, PBB_FIXED_FROM_INT(9), PBB_FIXED_FROM_INT(5), 2);
    assert(item != PBB_ITEM_INVALID_ID);
    assert(pbb_item_count_used(&world) == before_count + 1);
    assert(pbb_item_touch_actor_all(&world, player) >= 1);
    assert(pbb_item_count_used(&world) <= before_count + 1);
    pbb_item_world_set_recycle_consumed(&world, 0);

    /* v0.2: runtime save/load leaves definitions and callbacks intact. */
    assert(pbb_item_actor_set_var(&world, player, VAR_MONEY, 123));
    assert(pbb_item_save_runtime(&world, &save));
    assert(pbb_item_actor_set_var(&world, player, VAR_MONEY, 0));
    assert(pbb_item_load_runtime(&world, &save));
    assert(pbb_item_actor_get_var(&world, player, VAR_MONEY) == 123);

    return 0;
}
