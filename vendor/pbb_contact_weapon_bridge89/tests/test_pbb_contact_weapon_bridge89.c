#include "pbb_contact_weapon_bridge89.h"
#include <stdio.h>
#include <string.h>

typedef struct TestHostTag {
    int weapon_count;
    int ammo_count;
    int reject;
} TestHost;

typedef struct TestSensorTag {
    CT89_Subject actor_subject;
    unsigned long category_mask;
} TestSensor;

static int test_grant_weapon(void *user, int host_actor_id,
                             int weapon_id, int amount, int auto_equip)
{
    TestHost *host;
    (void)host_actor_id;
    (void)weapon_id;
    (void)auto_equip;
    host = (TestHost *)user;
    if (!host || host->reject) return 0;
    host->weapon_count += amount;
    return 1;
}

static int test_grant_ammo(void *user, int host_actor_id,
                           int ammo_id, int amount)
{
    TestHost *host;
    (void)host_actor_id;
    (void)ammo_id;
    host = (TestHost *)user;
    if (!host || host->reject) return 0;
    host->ammo_count += amount;
    return 1;
}

static int test_gather(void *user, const CT89_Probe *probe,
                       CT89_Candidate *out_candidates, int max_candidates)
{
    TestSensor *sensor;
    (void)probe;
    sensor = (TestSensor *)user;
    if (!sensor || !out_candidates || max_candidates <= 0) return 0;
    out_candidates[0].subject = sensor->actor_subject;
    out_candidates[0].category_mask = sensor->category_mask;
    out_candidates[0].distance_fx = 0L;
    return 1;
}

static int create_item(PBB_ItemWorld *world, int interactable)
{
    int def_id;
    int effect_id;
    int hook;
    unsigned long flags;
    def_id = pbb_item_def_create(world, interactable ? "manual_weapon" : "touch_weapon");
    if (def_id < 0) return -1;
    flags = PBB_ITEMF_ACTIVE | PBB_ITEMF_VISIBLE;
    if (interactable) flags |= PBB_ITEMF_INTERACTABLE;
    else flags |= PBB_ITEMF_TOUCHABLE;
    if (!pbb_item_def_set_defaults(world, def_id, flags, 1,
                                   PBB_FIXED_HALF, PBB_FIXED_HALF)) return -1;
    if (!pbb_item_def_set_category(world, def_id, PBB_ITEM_CATEGORY_PICKUP)) return -1;
    effect_id = pbb_item_effect_add(world, PBB_EFFECT_CONSUME_ITEM,
                                    0, 0, 0, 0, 0L, 0L);
    if (effect_id < 0) return -1;
    hook = interactable ? PBB_ITEM_HOOK_INTERACT : PBB_ITEM_HOOK_TOUCH;
    if (!pbb_item_def_set_effect_block(world, def_id, hook, effect_id, 1)) return -1;
    return pbb_item_spawn(world, def_id, 0L, 0L, 1);
}

static int setup(PBB_ItemWorld *world, CT89_Context *contact,
                 PBBCTW89_Bridge *bridge, TestHost *host,
                 int *out_actor)
{
    PBBCTW89_WeaponProvider wp;
    int actor;
    pbb_item_world_init(world);
    ct89_init(contact);
    pbbctw89_init(bridge);
    if (pbbctw89_attach(bridge, contact, world) != PBBCTW89_OK) return 0;
    pbbctw89_weapon_provider_init(&wp);
    wp.user = host;
    wp.grant_weapon = test_grant_weapon;
    wp.grant_ammo = test_grant_ammo;
    pbbctw89_set_weapon_provider(bridge, &wp);
    actor = pbb_item_actor_create(world, PBB_ACTOR_CLASS_PLAYER, 1UL,
                                  0L, 0L, PBB_FIXED_HALF, PBB_FIXED_HALF);
    if (actor < 0) return 0;
    if (!pbb_item_actor_set_masks(world, actor,
                                  PBB_ITEM_CATEGORY_ANY,
                                  PBB_ITEM_CATEGORY_ANY)) return 0;
    if (pbbctw89_bind_actor(bridge, 100UL, actor, 7) != PBBCTW89_OK) return 0;
    *out_actor = actor;
    return 1;
}

static int test_touch_accept(void)
{
    PBB_ItemWorld world;
    CT89_Context contact;
    PBBCTW89_Bridge bridge;
    PBBCTW89_Bridge *b;
    TestHost host;
    TestSensor sensor;
    CT89_SensorProvider sp;
    CT89_Trigger trigger;
    int actor;
    int item;
    memset(&host, 0, sizeof(host));
    if (!setup(&world, &contact, &bridge, &host, &actor)) return 0;
    (void)actor;
    item = create_item(&world, 0);
    if (item < 0) return 0;
    b = &bridge;
    if (pbbctw89_bind_weapon_pickup(b, item, 3, 1, 1,
                                    PBBCTW89_HOOK_TOUCH) != PBBCTW89_OK) return 0;
    trigger = pbbctw89_create_item_trigger(b, 200UL, item,
                                           CT89_SENSOR_TOUCH,
                                           CT89_FX_ONE,
                                           1UL,
                                           PBBCTW89_ACTION_TOUCH,
                                           CT89_CONSUME_DISABLE_TRIGGER);
    if (trigger == CT89_TRIGGER_INVALID) return 0;
    sensor.actor_subject = 100UL;
    sensor.category_mask = 1UL;
    ct89_sensor_provider_init(&sp);
    sp.user = &sensor;
    sp.gather = test_gather;
    ct89_set_contact_provider(&contact, &sp);
    if (!ct89_step(&contact, 16UL)) return 0;
    if (host.weapon_count != 1) return 0;
    if (pbb_item_is_world_active(&world, item)) return 0;
    if (ct89_trigger_is_enabled(&contact, trigger)) return 0;
    return 1;
}

static int test_touch_reject_keeps_item(void)
{
    PBB_ItemWorld world;
    CT89_Context contact;
    PBBCTW89_Bridge bridge;
    TestHost host;
    TestSensor sensor;
    CT89_SensorProvider sp;
    CT89_Trigger trigger;
    int actor;
    int item;
    memset(&host, 0, sizeof(host));
    host.reject = 1;
    if (!setup(&world, &contact, &bridge, &host, &actor)) return 0;
    (void)actor;
    item = create_item(&world, 0);
    if (item < 0) return 0;
    if (pbbctw89_bind_weapon_pickup(&bridge, item, 3, 1, 0,
                                    PBBCTW89_HOOK_TOUCH) != PBBCTW89_OK) return 0;
    trigger = pbbctw89_create_item_trigger(&bridge, 201UL, item,
                                           CT89_SENSOR_TOUCH,
                                           CT89_FX_ONE, 1UL,
                                           PBBCTW89_ACTION_TOUCH,
                                           CT89_CONSUME_DISABLE_TRIGGER);
    if (trigger == CT89_TRIGGER_INVALID) return 0;
    sensor.actor_subject = 100UL;
    sensor.category_mask = 1UL;
    ct89_sensor_provider_init(&sp);
    sp.user = &sensor;
    sp.gather = test_gather;
    ct89_set_contact_provider(&contact, &sp);
    if (!ct89_step(&contact, 16UL)) return 0;
    if (host.weapon_count != 0) return 0;
    if (!pbb_item_is_world_active(&world, item)) return 0;
    if (!ct89_trigger_is_enabled(&contact, trigger)) return 0;
    return 1;
}

static int test_manual_interact_ammo(void)
{
    PBB_ItemWorld world;
    CT89_Context contact;
    PBBCTW89_Bridge bridge;
    TestHost host;
    CT89_Trigger trigger;
    int actor;
    int item;
    int action;
    memset(&host, 0, sizeof(host));
    if (!setup(&world, &contact, &bridge, &host, &actor)) return 0;
    (void)actor;
    item = create_item(&world, 1);
    if (item < 0) return 0;
    if (pbbctw89_bind_ammo_pickup(&bridge, item, 2, 12,
                                  PBBCTW89_HOOK_INTERACT) != PBBCTW89_OK) return 0;
    trigger = pbbctw89_create_item_trigger(&bridge, 202UL, item,
                                           CT89_SENSOR_MANUAL,
                                           0L, 1UL,
                                           PBBCTW89_ACTION_INTERACT,
                                           CT89_CONSUME_DISABLE_TRIGGER);
    if (trigger == CT89_TRIGGER_INVALID) return 0;
    action = ct89_activate(&contact, trigger, 100UL);
    if (action != CT89_ACTION_ACCEPTED) return 0;
    if (host.ammo_count != 12) return 0;
    if (pbb_item_is_world_active(&world, item)) return 0;
    if (ct89_trigger_is_enabled(&contact, trigger)) return 0;
    return 1;
}

static int test_manual_touch_weapon(void)
{
    PBB_ItemWorld world;
    CT89_Context contact;
    PBBCTW89_Bridge bridge;
    TestHost host;
    CT89_Trigger trigger;
    int actor;
    int item;
    int action;
    memset(&host, 0, sizeof(host));
    if (!setup(&world, &contact, &bridge, &host, &actor)) return 0;
    (void)actor;
    item = create_item(&world, 0);
    if (item < 0) return 0;
    if (pbbctw89_bind_weapon_pickup(&bridge, item, 5, 1, 0,
                                    PBBCTW89_HOOK_TOUCH) != PBBCTW89_OK) return 0;
    trigger = pbbctw89_create_item_trigger(&bridge, 203UL, item,
                                           CT89_SENSOR_MANUAL, 0L, 1UL,
                                           PBBCTW89_ACTION_TOUCH,
                                           CT89_CONSUME_DISABLE_TRIGGER);
    if (trigger == CT89_TRIGGER_INVALID) return 0;
    action = ct89_activate(&contact, trigger, 100UL);
    if (action != CT89_ACTION_ACCEPTED) return 0;
    if (host.weapon_count != 1) return 0;
    if (pbb_item_is_world_active(&world, item)) return 0;
    if (ct89_trigger_is_enabled(&contact, trigger)) return 0;
    return 1;
}

int main(void)
{
    int ok;
    ok = 1;
    ok = ok && test_touch_accept();
    ok = ok && test_touch_reject_keeps_item();
    ok = ok && test_manual_interact_ammo();
    ok = ok && test_manual_touch_weapon();
    if (!ok) {
        printf("pbb_contact_weapon_bridge89: FAIL\n");
        return 1;
    }
    printf("pbb_contact_weapon_bridge89: PASS\n");
    return 0;
}
