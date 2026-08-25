#include "blank3d_variables.h"

#include <stdio.h>
#include <string.h>

static int require_int(int condition, const char *message)
{
    if (!condition) {
        printf("FAIL: %s\n", message);
        return 0;
    }
    return 1;
}

int main(void)
{
    Blank3DSystems systems;
    Blank3DVariables vars;
    ns_id type_id;
    ns_id value_id;
    ns_fx numeric;
    FlagsValue flag;
    vm89_value dynamic_value;

    memset(&systems, 0, sizeof(systems));
    ns_init(&systems.numbers);
    type_id = ns_define_type(&systems.numbers, "player.health",
                             NS_SCOPE_INSTANCE, NS_KIND_INT,
                             NS_FX_FROM_INT(100), NS_FX_ZERO,
                             NS_FX_FROM_INT(100), NS_OVERFLOW_CLAMP,
                             NS_FLAG_SAVE | NS_FLAG_HUD);
    if (!require_int(type_id != NS_INVALID_ID, "define health")) return 1;
    value_id = ns_attach_type(&systems.numbers, 1, type_id);
    if (!require_int(value_id != NS_INVALID_ID, "attach health owner1")) return 1;
    type_id = ns_define_type(&systems.numbers, "weapon.damage_multiplier",
                             NS_SCOPE_INSTANCE, NS_KIND_FIXED,
                             NS_FX_ONE, NS_FX_ZERO, NS_FX_FROM_INT(8),
                             NS_OVERFLOW_CLAMP, NS_FLAG_SAVE);
    if (!require_int(type_id != NS_INVALID_ID, "define multiplier")) return 1;
    (void)ns_attach_type(&systems.numbers, 1, type_id);

    flagstore_init(&systems.flags, systems.flag_entries, B3D_FLAG_CAPACITY,
                   systems.flag_pool, B3D_FLAG_POOL_CAPACITY);
    if (!require_int(flagstore_set_bool(&systems.flags,
                                        "weapon.can_fire", 1),
                     "seed can_fire")) return 1;

    blank3d_variables_init(&vars, &systems);
    blank3d_variables_set_player_owner(&vars, 1UL);
    if (!require_int(blank3d_variables_instance_create(&vars, 1UL),
                     "create var owner1")) return 1;
    if (!require_int(blank3d_variables_begin_event(&vars, 1UL, 2UL),
                     "begin owner1 event")) return 1;
    if (!blank3d_variables_execute(&vars, 1UL,
        "health = 100; health -= 25; can_fire = false;\n"
        "alerted = true; foo = 3; var temp = 2; foo += temp;\n"
        "weapon.damage_multiplier = 2; global.debug = true;")) {
        printf("DETAIL: %s\n", blank3d_variables_status(&vars));
        if (!require_int(0, "execute owner1 authoring")) return 1;
    }

    value_id = ns_find_value(&systems.numbers, 1, "player.health");
    if (!require_int(value_id != NS_INVALID_ID &&
                     ns_get_by_id(&systems.numbers, value_id, &numeric) == NS_OK &&
                     numeric == NS_FX_FROM_INT(75),
                     "health routed through NumSys")) return 1;
    value_id = ns_find_value(&systems.numbers, 1,
                             "weapon.damage_multiplier");
    if (!require_int(value_id != NS_INVALID_ID &&
                     ns_get_by_id(&systems.numbers, value_id, &numeric) == NS_OK &&
                     numeric == NS_FX_FROM_INT(2),
                     "dotted numeric name routed")) return 1;
    if (!require_int(flagstore_get(&systems.flags, "weapon.can_fire", &flag) &&
                     flag.type == FLAGS_VAL_BOOL && flag.as.i == 0L,
                     "alias routed through existing FlagStore")) return 1;
    if (!require_int(flagstore_get(&systems.flags, "thing.1.alerted", &flag) &&
                     flag.type == FLAGS_VAL_BOOL && flag.as.i != 0L,
                     "dynamic bool namespaced by Thing")) return 1;
    if (!require_int(flagstore_get(&systems.flags, "debug", &flag) &&
                     flag.type == FLAGS_VAL_BOOL && flag.as.i != 0L,
                     "global dynamic flag")) return 1;
    if (!require_int(blank3d_variables_get(&vars, VR89_SCOPE_INSTANCE, 1UL,
                                           "foo", &dynamic_value) &&
                     dynamic_value.type == VM89_VALUE_FIXED &&
                     dynamic_value.fixed_q16 == 5L * 65536L,
                     "free numeric remains dynamic VarStore")) return 1;
    if (!require_int(blank3d_variables_end_event(&vars),
                     "end owner1 event")) return 1;

    if (!require_int(blank3d_variables_instance_create(&vars, 2UL),
                     "create owner2")) return 1;
    if (!require_int(blank3d_variables_begin_event(&vars, 2UL, 2UL),
                     "begin owner2 event")) return 1;
    if (!require_int(blank3d_variables_execute(&vars, 2UL,
                                               "alerted = false; foo = 9;"),
                     "execute owner2")) return 1;
    if (!require_int(flagstore_get(&systems.flags, "thing.2.alerted", &flag) &&
                     flag.type == FLAGS_VAL_BOOL && flag.as.i == 0L,
                     "owner2 dynamic flag independent")) return 1;
    if (!require_int(flagstore_get(&systems.flags, "thing.1.alerted", &flag) &&
                     flag.as.i != 0L,
                     "owner1 flag preserved")) return 1;
    if (!require_int(blank3d_variables_end_event(&vars),
                     "end owner2 event")) return 1;

    printf("OK: VarDSL -> NumSys + Flags + dynamic VarStore by Thing\n");
    return 0;
}
