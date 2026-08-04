#include "numsys.h"
#include "numsys_packs.h"

/*
    Attribute-style modifiers with fixed-point math only:
    - Permanent flat modifier on max health.
    - Timed percent modifier on speed/stamina value.
*/

#define CHECK(expr) do { if (!(expr)) { return 1; } } while (0)

static NS_World world;

int main(void)
{
    ns_owner player;
    ns_id stamina;
    ns_fx value;

    player = 7;
    ns_init(&world);
    CHECK(ns_define_pack(&world, NS_PACK_SURVIVAL, NS_PACK_SURVIVAL_COUNT) == NS_OK);

    stamina = ns_attach(&world, player, "stamina");
    CHECK(stamina >= 0);
    CHECK(ns_set_by_id(&world, stamina, NS_FX_FROM_INT(50)) == NS_OK);

    CHECK(ns_add_modifier_by_id(&world, stamina, 100, NS_MOD_TARGET_MAX,
                                NS_MOD_FLAT, NS_FX_FROM_INT(50),
                                NS_MOD_FOREVER, 0, 0) >= 0);

    CHECK(world.values[stamina].max_value == NS_FX_FROM_INT(150));

    CHECK(ns_add_modifier_by_id(&world, stamina, 200, NS_MOD_TARGET_VALUE,
                                NS_MOD_PERCENT_ADD, NS_FX_FROM_PERCENT(50),
                                2, 0, 0) >= 0);

    CHECK(ns_get_by_id(&world, stamina, &value) == NS_OK);
    CHECK(value == NS_FX_FROM_INT(75));

    CHECK(ns_tick(&world) == NS_OK);
    CHECK(ns_tick(&world) == NS_OK);
    CHECK(ns_count_modifiers_by_value(&world, stamina) == 1);

    return 0;
}
