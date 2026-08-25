#include "blank3d_systems.h"
#include <stdio.h>
#include <string.h>

typedef struct PickupSensorTag {
    CT89_Subject subject;
    unsigned long category_mask;
} PickupSensor;

static int gather_player(void *user, const CT89_Probe *probe,
                         CT89_Candidate *out_candidates, int max_candidates)
{
    PickupSensor *sensor;
    (void)probe;
    sensor = (PickupSensor *)user;
    if (!sensor || !out_candidates || max_candidates <= 0) return 0;
    out_candidates[0].subject = sensor->subject;
    out_candidates[0].category_mask = sensor->category_mask;
    out_candidates[0].distance_fx = 0L;
    return 1;
}

static void tick(Blank3DSystems *systems)
{
    GWP89_Vec3 origin;
    GWP89_Vec3 forward;
    GWP89_Vec3 right;
    GWP89_Vec3 up;
    memset(&origin, 0, sizeof(origin));
    memset(&forward, 0, sizeof(forward));
    memset(&right, 0, sizeof(right));
    memset(&up, 0, sizeof(up));
    forward.z = -GWP89_FIX_ONE;
    right.x = GWP89_FIX_ONE;
    up.y = GWP89_FIX_ONE;
    blank3d_systems_update(systems, 16U, 0, 0, 0,
                           GWP89_VIEW_FPS, GWP89_FIX_ONE,
                           &origin, &forward, &right, &up,
                           &origin, &forward);
}

int main(void)
{
    Blank3DSystems systems;
    CT89_SensorProvider sensor_provider;
    PickupSensor sensor;
    CT89_Trigger accepted_trigger;
    CT89_Trigger rejected_trigger;
    int accepted_item;
    int rejected_item;

    blank3d_systems_init_from_ini(&systems, "config/weapons/weapons.ini");

    /* Make weapon 5 absent so the first pickup can actually grant it. */
    if (gkinv_count_item(&systems.inventory, 5U) == 0U) return 1;
    if (gkinv_remove_item(&systems.item_db, &systems.inventory, 5U, 1U) != GKINV_OK) return 2;
    if (gkinv_count_item(&systems.inventory, 5U) != 0U) return 3;

    sensor.subject = (CT89_Subject)B3D_PLAYER_ACTOR_ID;
    sensor.category_mask = 1UL;
    ct89_sensor_provider_init(&sensor_provider);
    sensor_provider.user = &sensor;
    sensor_provider.gather = gather_player;
    ct89_set_contact_provider(blank3d_systems_contact_triggers(&systems),
                              &sensor_provider);

    if (!blank3d_systems_define_weapon_pickup(&systems,
                                               "qa_weapon_5",
                                               5, 1, 1, 0,
                                               500UL,
                                               CT89_SENSOR_TOUCH,
                                               CT89_FX_ONE,
                                               1UL,
                                               CT89_CONSUME_DISABLE_TRIGGER,
                                               &accepted_item,
                                               &accepted_trigger)) return 4;

    tick(&systems);
    if (gkinv_count_item(&systems.inventory, 5U) != 1U) return 5;
    if (blank3d_systems_weapon_id(&systems) != 5) return 6;
    if (pbb_item_is_world_active(blank3d_systems_item_world(&systems),
                                 accepted_item)) return 7;
    if (ct89_trigger_is_enabled(blank3d_systems_contact_triggers(&systems),
                                accepted_trigger)) return 8;

    /* A second copy is over the catalog's max_owned=1 and must remain. */
    if (!blank3d_systems_define_weapon_pickup(&systems,
                                               "qa_weapon_5_rejected",
                                               5, 1, 0, 0,
                                               501UL,
                                               CT89_SENSOR_TOUCH,
                                               CT89_FX_ONE,
                                               1UL,
                                               CT89_CONSUME_DISABLE_TRIGGER,
                                               &rejected_item,
                                               &rejected_trigger)) return 9;
    tick(&systems);
    if (gkinv_count_item(&systems.inventory, 5U) != 1U) return 10;
    if (!pbb_item_is_world_active(blank3d_systems_item_world(&systems),
                                  rejected_item)) return 11;
    if (!ct89_trigger_is_enabled(blank3d_systems_contact_triggers(&systems),
                                 rejected_trigger)) return 12;

    printf("blank3d item/contact/weapon integration: PASS\n");
    return 0;
}
