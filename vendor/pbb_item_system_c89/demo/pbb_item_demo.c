#include <stdio.h>
#include "pbb_item_system.h"

enum {
    VAR_HP = 0,
    VAR_MONEY = 1,
    VAR_POTIONS_HELD = 2,
    VAR_TOTAL_TOUCHES = 3
};

enum {
    WORLD_FLAG_DOOR_OPEN = 0
};

static const char *event_name(int type)
{
    switch (type) {
        case PBB_EVENT_ITEM_SPAWNED:
            return "spawned";
        case PBB_EVENT_ITEM_TOUCHED:
            return "touched";
        case PBB_EVENT_ITEM_INTERACTED:
            return "interacted";
        case PBB_EVENT_ITEM_DROPPED:
            return "dropped";
        case PBB_EVENT_ITEM_RULE_BLOCKED:
            return "blocked";
        case PBB_EVENT_ITEM_EFFECT:
            return "effect";
        case PBB_EVENT_ITEM_CONSUMED:
            return "consumed";
        case PBB_EVENT_ITEM_DESTROYED:
            return "destroyed";
        case PBB_EVENT_CUSTOM:
            return "custom";
        default:
            break;
    }

    return "unknown";
}

static void drain_events(PBB_ItemWorld *world)
{
    PBB_ItemEvent ev;

    while (pbb_item_event_pop(world, &ev)) {
        printf("event %-10s actor=%d item=%d def=%d a=%d b=%d c=%d pos=(%d,%d)\n",
               event_name(ev.type),
               ev.actor_id,
               ev.item_id,
               ev.def_id,
               ev.a,
               ev.b,
               ev.c,
               PBB_FIXED_TO_INT(ev.x),
               PBB_FIXED_TO_INT(ev.y));
    }
}

static int add_coin_def(PBB_ItemWorld *world)
{
    int def;
    int first_effect;
    int e;

    def = pbb_item_def_create(world, "coin");
    pbb_item_def_set_defaults(world,
                              def,
                              PBB_ITEMF_ACTIVE | PBB_ITEMF_VISIBLE | PBB_ITEMF_TOUCHABLE,
                              1,
                              PBB_FIXED_HALF,
                              PBB_FIXED_HALF);

    first_effect = pbb_item_effect_add(world,
                                       PBB_EFFECT_ADD_ACTOR_VAR_FROM_ITEM_AMOUNT,
                                       VAR_MONEY,
                                       1,
                                       0,
                                       0,
                                       0,
                                       0);
    e = pbb_item_effect_add(world,
                            PBB_EFFECT_ADD_WORLD_VAR,
                            VAR_TOTAL_TOUCHES,
                            1,
                            0,
                            0,
                            0,
                            0);
    (void)e;
    e = pbb_item_effect_add(world,
                            PBB_EFFECT_CONSUME_ITEM,
                            0,
                            0,
                            0,
                            0,
                            0,
                            0);
    (void)e;

    pbb_item_def_set_effect_block(world, def, PBB_ITEM_HOOK_TOUCH, first_effect, 3);
    return def;
}

static int add_trap_def(PBB_ItemWorld *world)
{
    int def;
    int first_effect;
    int e;

    def = pbb_item_def_create(world, "spike_trap");
    pbb_item_def_set_defaults(world,
                              def,
                              PBB_ITEMF_ACTIVE | PBB_ITEMF_VISIBLE | PBB_ITEMF_TOUCHABLE,
                              1,
                              PBB_FIXED_HALF,
                              PBB_FIXED_HALF);

    first_effect = pbb_item_effect_add(world,
                                       PBB_EFFECT_ADD_ACTOR_VAR,
                                       VAR_HP,
                                       -10,
                                       0,
                                       0,
                                       0,
                                       0);
    e = pbb_item_effect_add(world,
                            PBB_EFFECT_CLAMP_ACTOR_VAR,
                            VAR_HP,
                            0,
                            999,
                            0,
                            0,
                            0);
    (void)e;
    e = pbb_item_effect_add(world,
                            PBB_EFFECT_ADD_WORLD_VAR,
                            VAR_TOTAL_TOUCHES,
                            1,
                            0,
                            0,
                            0,
                            0);
    (void)e;

    pbb_item_def_set_effect_block(world, def, PBB_ITEM_HOOK_TOUCH, first_effect, 3);
    return def;
}

static int add_blue_switch_def(PBB_ItemWorld *world)
{
    int def;
    int first_effect;
    int e;

    def = pbb_item_def_create(world, "blue_switch");
    pbb_item_def_set_defaults(world,
                              def,
                              PBB_ITEMF_ACTIVE | PBB_ITEMF_VISIBLE | PBB_ITEMF_INTERACTABLE,
                              1,
                              PBB_FIXED_HALF,
                              PBB_FIXED_HALF);

    first_effect = pbb_item_effect_add(world,
                                       PBB_EFFECT_SET_WORLD_FLAG,
                                       WORLD_FLAG_DOOR_OPEN,
                                       0,
                                       0,
                                       0,
                                       0,
                                       0);
    e = pbb_item_effect_add(world,
                            PBB_EFFECT_HIDE_ITEM,
                            0,
                            0,
                            0,
                            0,
                            0,
                            0);
    (void)e;
    e = pbb_item_effect_add(world,
                            PBB_EFFECT_EMIT_EVENT,
                            PBB_EVENT_CUSTOM,
                            777,
                            0,
                            0,
                            0,
                            0);
    (void)e;

    pbb_item_def_set_effect_block(world, def, PBB_ITEM_HOOK_INTERACT, first_effect, 3);
    return def;
}

static int add_potion_def(PBB_ItemWorld *world)
{
    int def;
    int touch_first;
    int drop_rule_first;
    int drop_effect_first;
    int e;
    int r;

    def = pbb_item_def_create(world, "potion");
    pbb_item_def_set_defaults(world,
                              def,
                              PBB_ITEMF_ACTIVE | PBB_ITEMF_VISIBLE | PBB_ITEMF_TOUCHABLE | PBB_ITEMF_DROPPABLE,
                              1,
                              PBB_FIXED_HALF,
                              PBB_FIXED_HALF);

    touch_first = pbb_item_effect_add(world,
                                      PBB_EFFECT_ADD_ACTOR_VAR_FROM_ITEM_AMOUNT,
                                      VAR_HP,
                                      25,
                                      0,
                                      0,
                                      0,
                                      0);
    e = pbb_item_effect_add(world,
                            PBB_EFFECT_CLAMP_ACTOR_VAR,
                            VAR_HP,
                            0,
                            100,
                            0,
                            0,
                            0);
    (void)e;
    e = pbb_item_effect_add(world,
                            PBB_EFFECT_CONSUME_ITEM,
                            0,
                            0,
                            0,
                            0,
                            0,
                            0);
    (void)e;
    pbb_item_def_set_effect_block(world, def, PBB_ITEM_HOOK_TOUCH, touch_first, 3);

    drop_rule_first = pbb_item_rule_add(world,
                                        PBB_RULE_ACTOR_VAR_GTE_ITEM_AMOUNT,
                                        VAR_POTIONS_HELD,
                                        1,
                                        0,
                                        0);
    r = drop_rule_first;
    (void)r;
    pbb_item_def_set_rule_block(world, def, PBB_ITEM_HOOK_DROP, drop_rule_first, 1);

    drop_effect_first = pbb_item_effect_add(world,
                                            PBB_EFFECT_ADD_ACTOR_VAR_FROM_ITEM_AMOUNT,
                                            VAR_POTIONS_HELD,
                                            -1,
                                            0,
                                            0,
                                            0,
                                            0);
    e = pbb_item_effect_add(world,
                            PBB_EFFECT_EMIT_EVENT,
                            PBB_EVENT_CUSTOM,
                            900,
                            0,
                            0,
                            0,
                            0);
    (void)e;
    pbb_item_def_set_effect_block(world, def, PBB_ITEM_HOOK_DROP, drop_effect_first, 2);

    return def;
}

int main(void)
{
    PBB_ItemWorld world;
    int player;
    int enemy;
    int coin_def;
    int trap_def;
    int switch_def;
    int potion_def;
    int potion_item;
    int drop_table;
    int touched;
    int interacted;
    int spawned;

    pbb_item_world_init(&world);
    pbb_item_world_set_rng(&world, 1234UL);

    coin_def = add_coin_def(&world);
    trap_def = add_trap_def(&world);
    switch_def = add_blue_switch_def(&world);
    potion_def = add_potion_def(&world);

    player = pbb_item_actor_create(&world,
                                   PBB_ACTOR_CLASS_PLAYER,
                                   1UL,
                                   PBB_FIXED_FROM_INT(10),
                                   PBB_FIXED_FROM_INT(10),
                                   PBB_FIXED_HALF,
                                   PBB_FIXED_HALF);
    enemy = pbb_item_actor_create(&world,
                                  PBB_ACTOR_CLASS_ENEMY,
                                  2UL,
                                  PBB_FIXED_FROM_INT(20),
                                  PBB_FIXED_FROM_INT(10),
                                  PBB_FIXED_HALF,
                                  PBB_FIXED_HALF);

    pbb_item_actor_set_var(&world, player, VAR_HP, 50);
    pbb_item_actor_set_var(&world, player, VAR_MONEY, 0);
    pbb_item_actor_set_var(&world, player, VAR_POTIONS_HELD, 2);

    pbb_item_spawn(&world, coin_def, PBB_FIXED_FROM_INT(10), PBB_FIXED_FROM_INT(10), 5);
    pbb_item_spawn(&world, trap_def, PBB_FIXED_FROM_INT(10), PBB_FIXED_FROM_INT(10), 1);
    pbb_item_spawn(&world, switch_def, PBB_FIXED_FROM_INT(12), PBB_FIXED_FROM_INT(10), 1);

    printf("-- after setup --\n");
    drain_events(&world);

    touched = pbb_item_touch_actor_all(&world, player);
    printf("\nplayer touched %d item(s)\n", touched);
    printf("hp=%d money=%d touches=%d\n",
           pbb_item_actor_get_var(&world, player, VAR_HP),
           pbb_item_actor_get_var(&world, player, VAR_MONEY),
           pbb_item_world_get_var(&world, VAR_TOTAL_TOUCHES));
    drain_events(&world);

    interacted = pbb_item_interact_actor_nearest(&world,
                                                 player,
                                                 PBB_FIXED_FROM_INT(4),
                                                 PBB_FIXED_FROM_INT(4));
    printf("\ninteracted=%d door_open=%d\n",
           interacted,
           pbb_item_world_get_flag(&world, WORLD_FLAG_DOOR_OPEN));
    drain_events(&world);

    potion_item = pbb_item_drop_from_actor(&world,
                                           player,
                                           potion_def,
                                           1,
                                           PBB_FIXED_FROM_INT(2),
                                           0);
    printf("\ndropped potion item=%d potions_held=%d\n",
           potion_item,
           pbb_item_actor_get_var(&world, player, VAR_POTIONS_HELD));
    drain_events(&world);

    pbb_item_actor_set_box(&world,
                           player,
                           PBB_FIXED_FROM_INT(12),
                           PBB_FIXED_FROM_INT(10),
                           PBB_FIXED_HALF,
                           PBB_FIXED_HALF);
    touched = pbb_item_touch_actor_all(&world, player);
    printf("\nafter touching dropped potion: touched=%d hp=%d\n",
           touched,
           pbb_item_actor_get_var(&world, player, VAR_HP));
    drain_events(&world);

    drop_table = pbb_item_drop_table_create(&world, "enemy_loot");
    pbb_item_drop_table_add_entry(&world,
                                  drop_table,
                                  coin_def,
                                  3,
                                  8,
                                  10000,
                                  0,
                                  0);
    pbb_item_drop_table_add_entry(&world,
                                  drop_table,
                                  potion_def,
                                  1,
                                  1,
                                  10000,
                                  PBB_FIXED_FROM_INT(1),
                                  0);
    spawned = pbb_item_drop_table_roll_from_actor(&world, drop_table, enemy);
    printf("\nenemy drop table spawned %d item(s)\n", spawned);
    drain_events(&world);

    return 0;
}
