#include "numsys.h"
#include "numsys_packs.h"

#define CHECK(expr) do { if (!(expr)) { return __LINE__; } } while (0)

static NS_World g_world;
static NS_World g_loaded;
static NS_SnapshotValue g_snapshot[64];

static int test_basic_pack_template_query(void)
{
    ns_id enemy_template;
    ns_id health_type;
    ns_id health_a;
    ns_id health_b;
    NS_Query query;
    ns_id found;
    int count;
    int affected;

    ns_init(&g_world);
    CHECK(ns_define_pack(&g_world, NS_PACK_SURVIVAL, NS_PACK_SURVIVAL_COUNT) == NS_OK);

    enemy_template = ns_define_template(&g_world, "enemy", 0);
    CHECK(enemy_template >= 0);
    CHECK(ns_template_add(&g_world, "enemy", "health") == NS_OK);
    CHECK(ns_template_add(&g_world, "enemy", "infection") == NS_OK);
    CHECK(ns_template_add(&g_world, "enemy", "armor") == NS_OK);

    CHECK(ns_attach_template(&g_world, 1001, enemy_template) == NS_OK);
    CHECK(ns_attach_template_name(&g_world, 1002, "enemy") == NS_OK);

    health_type = ns_find_type(&g_world, "health");
    CHECK(health_type >= 0);
    health_a = ns_find_value_by_type(&g_world, 1001, health_type);
    health_b = ns_find_value(&g_world, 1002, "health");
    CHECK(health_a >= 0);
    CHECK(health_b >= 0);

    CHECK(ns_sub_by_id(&g_world, health_a, NS_FX_FROM_INT(150)) == NS_OK);
    CHECK(ns_is_empty_by_id(&g_world, health_a) == NS_TRUE);

    ns_query_all(&query);
    query.use_type = NS_TRUE;
    query.type_id = health_type;
    query.use_cmp = NS_TRUE;
    query.cmp = NS_CMP_LTE;
    query.rhs = NS_FX_ZERO;
    CHECK(ns_query_next(&g_world, &query, NS_INVALID_ID, &found) == NS_TRUE);
    CHECK(found == health_a);
    CHECK(ns_query_count(&g_world, &query, &count) == NS_OK);
    CHECK(count == 1);

    ns_query_all(&query);
    query.use_type = NS_TRUE;
    query.type_id = health_type;
    CHECK(ns_query_sub(&g_world, &query, NS_FX_FROM_INT(10), &affected) == NS_OK);
    CHECK(affected == 2);
    CHECK(ns_get_or(&g_world, 1002, "health", NS_FX_ZERO) == NS_FX_FROM_INT(90));

    return 0;
}

static int test_events_thresholds_bindings(void)
{
    ns_id hp;
    ns_id threshold;
    ns_id binding;
    NS_Event event_data;
    int saw_threshold;
    int saw_empty;
    int binding_count;

    ns_init(&g_world);
    CHECK(ns_define_pack(&g_world, NS_PACK_SURVIVAL, NS_PACK_SURVIVAL_COUNT) == NS_OK);
    hp = ns_attach(&g_world, 2001, "health");
    CHECK(hp >= 0);

    threshold = ns_add_threshold(&g_world, hp, NS_FX_FROM_INT(25), NS_EDGE_DOWN, NS_TRUE, 77);
    CHECK(threshold >= 0);
    binding = ns_bind_value(&g_world, hp, NS_BIND_BAR, 900, 1, 0);
    CHECK(binding >= 0);
    CHECK(ns_binding_count(&g_world, hp, &binding_count) == NS_OK);
    CHECK(binding_count == 1);

    CHECK(ns_sub_by_id(&g_world, hp, NS_FX_FROM_INT(80)) == NS_OK);
    saw_threshold = NS_FALSE;
    saw_empty = NS_FALSE;
    while (ns_poll_event(&g_world, &event_data) == NS_TRUE) {
        if (event_data.event_type == NS_EVENT_THRESHOLD && event_data.user_code == 77) {
            saw_threshold = NS_TRUE;
        }
        if (event_data.event_type == NS_EVENT_EMPTY) {
            saw_empty = NS_TRUE;
        }
    }
    CHECK(saw_threshold == NS_TRUE);
    CHECK(saw_empty == NS_FALSE);

    CHECK(ns_sub_by_id(&g_world, hp, NS_FX_FROM_INT(30)) == NS_OK);
    while (ns_poll_event(&g_world, &event_data) == NS_TRUE) {
        if (event_data.event_type == NS_EVENT_EMPTY) {
            saw_empty = NS_TRUE;
        }
    }
    CHECK(saw_empty == NS_TRUE);

    return 0;
}

static int test_modifiers_and_tick(void)
{
    ns_id stamina;
    ns_fx value;

    ns_init(&g_world);
    CHECK(ns_define_pack(&g_world, NS_PACK_SURVIVAL, NS_PACK_SURVIVAL_COUNT) == NS_OK);
    stamina = ns_attach(&g_world, 3001, "stamina");
    CHECK(stamina >= 0);
    CHECK(ns_set_by_id(&g_world, stamina, NS_FX_FROM_INT(50)) == NS_OK);

    CHECK(ns_add_modifier_by_id(&g_world, stamina, 1, NS_MOD_TARGET_MAX, NS_MOD_FLAT, NS_FX_FROM_INT(50), NS_MOD_FOREVER, 0, 0) >= 0);
    CHECK(g_world.values[stamina].max_value == NS_FX_FROM_INT(150));

    CHECK(ns_add_modifier_by_id(&g_world, stamina, 2, NS_MOD_TARGET_VALUE, NS_MOD_PERCENT_ADD, NS_FX_FROM_PERCENT(50), 2, 0, 0) >= 0);
    CHECK(ns_get_by_id(&g_world, stamina, &value) == NS_OK);
    CHECK(value == NS_FX_FROM_INT(75));

    CHECK(ns_tick(&g_world) == NS_OK);
    CHECK(ns_get_by_id(&g_world, stamina, &value) == NS_OK);
    CHECK(value > NS_FX_FROM_INT(75));

    CHECK(ns_tick(&g_world) == NS_OK);
    CHECK(ns_count_modifiers_by_value(&g_world, stamina) == 1);
    CHECK(g_world.values[stamina].max_value == NS_FX_FROM_INT(150));

    CHECK(ns_remove_modifier_by_code(&g_world, stamina, 1) == NS_OK);
    CHECK(g_world.values[stamina].max_value == NS_FX_FROM_INT(100));

    return 0;
}

static int test_spill_and_transfer(void)
{
    ns_id shield_type;
    ns_id health_type;
    ns_id shield;
    ns_id health;
    ns_fx spill;
    ns_fx moved;

    ns_init(&g_world);
    health_type = ns_define_type(&g_world, "health", NS_SCOPE_INSTANCE, NS_KIND_FIXED, NS_FX_FROM_INT(100), NS_FX_ZERO, NS_FX_FROM_INT(100), NS_OVERFLOW_CLAMP, NS_FLAG_SAVE);
    shield_type = ns_define_type(&g_world, "shield", NS_SCOPE_INSTANCE, NS_KIND_FIXED, NS_FX_FROM_INT(40), NS_FX_ZERO, NS_FX_FROM_INT(40), NS_OVERFLOW_CLAMP, NS_FLAG_SAVE);
    CHECK(health_type >= 0);
    CHECK(shield_type >= 0);
    health = ns_attach_type(&g_world, 4001, health_type);
    shield = ns_attach_type(&g_world, 4001, shield_type);
    CHECK(health >= 0);
    CHECK(shield >= 0);

    CHECK(ns_sub_spill_by_id(&g_world, shield, health, NS_FX_FROM_INT(70), &spill) == NS_OK);
    CHECK(spill == NS_FX_FROM_INT(30));
    CHECK(ns_get_or(&g_world, 4001, "shield", NS_FX_FROM_INT(-1)) == NS_FX_ZERO);
    CHECK(ns_get_or(&g_world, 4001, "health", NS_FX_ZERO) == NS_FX_FROM_INT(70));

    CHECK(ns_transfer_by_id(&g_world, health, shield, NS_FX_FROM_INT(10), &moved) == NS_OK);
    CHECK(moved == NS_FX_FROM_INT(10));
    CHECK(ns_get_or(&g_world, 4001, "shield", NS_FX_ZERO) == NS_FX_FROM_INT(10));
    CHECK(ns_get_or(&g_world, 4001, "health", NS_FX_ZERO) == NS_FX_FROM_INT(60));

    return 0;
}

static int test_derived_snapshot(void)
{
    ns_id hp;
    ns_id hp_percent_type;
    ns_id hp_percent;
    ns_fx percent;
    int out_count;

    ns_init(&g_world);
    CHECK(ns_define_pack(&g_world, NS_PACK_SURVIVAL, NS_PACK_SURVIVAL_COUNT) == NS_OK);
    hp_percent_type = ns_define_type(&g_world, "health.percent", NS_SCOPE_INSTANCE, NS_KIND_FIXED, NS_FX_ZERO, NS_FX_ZERO, NS_FX_ONE, NS_OVERFLOW_CLAMP, NS_FLAG_HUD);
    CHECK(hp_percent_type >= 0);
    hp = ns_attach(&g_world, 5001, "health");
    hp_percent = ns_attach_type(&g_world, 5001, hp_percent_type);
    CHECK(hp >= 0);
    CHECK(hp_percent >= 0);
    CHECK(ns_sub_by_id(&g_world, hp, NS_FX_FROM_INT(25)) == NS_OK);
    CHECK(ns_add_derived(&g_world, hp_percent, hp, NS_INVALID_ID, NS_DERIVED_PERCENT, NS_FX_ONE, NS_FX_ZERO, 0) >= 0);
    CHECK(ns_get_by_id(&g_world, hp_percent, &percent) == NS_OK);
    CHECK(percent == NS_FX_FROM_RATIO(3, 4));

    CHECK(ns_export_values(&g_world, g_snapshot, 64, &out_count) == NS_OK);
    CHECK(out_count >= 2);

    ns_init(&g_loaded);
    CHECK(ns_import_values(&g_loaded, g_snapshot, out_count) == NS_OK);
    CHECK(ns_get_or(&g_loaded, 5001, "health", NS_FX_ZERO) == NS_FX_FROM_INT(75));

    return 0;
}

int main(void)
{
    int result;

    result = test_basic_pack_template_query();
    if (result != 0) {
        return result;
    }
    result = test_events_thresholds_bindings();
    if (result != 0) {
        return result;
    }
    result = test_modifiers_and_tick();
    if (result != 0) {
        return result;
    }
    result = test_spill_and_transfer();
    if (result != 0) {
        return result;
    }
    result = test_derived_snapshot();
    if (result != 0) {
        return result;
    }

    return 0;
}
