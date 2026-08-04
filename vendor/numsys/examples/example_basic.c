#include "numsys.h"
#include "numsys_packs.h"

/*
    Basic GameMaker-like use:
    player.health -= 25;
    weapon.ammo.magazine -= 1;

    This example returns 0 when every check passes.
*/

#define CHECK(expr) do { if (!(expr)) { return 1; } } while (0)

static NS_World world;

int main(void)
{
    ns_owner player;
    ns_owner pistol;
    ns_id health;
    ns_id ammo;
    ns_fx hp_percent;

    player = 1;
    pistol = 2;

    ns_init(&world);
    CHECK(ns_define_pack(&world, NS_PACK_SURVIVAL, NS_PACK_SURVIVAL_COUNT) == NS_OK);
    CHECK(ns_define_pack(&world, NS_PACK_SHOOTER, NS_PACK_SHOOTER_COUNT) == NS_OK);

    health = ns_attach(&world, player, "health");
    ammo = ns_attach(&world, pistol, "ammo.magazine");
    CHECK(health >= 0);
    CHECK(ammo >= 0);

    CHECK(ns_sub_by_id(&world, health, NS_FX_FROM_INT(25)) == NS_OK);
    CHECK(ns_sub_by_id(&world, ammo, NS_FX_FROM_INT(1)) == NS_OK);

    CHECK(ns_percent_by_id(&world, health, &hp_percent) == NS_OK);
    CHECK(hp_percent == NS_FX_FROM_RATIO(3, 4));
    CHECK(ns_get_or(&world, pistol, "ammo.magazine", NS_FX_ZERO) == NS_FX_FROM_INT(14));

    return 0;
}
