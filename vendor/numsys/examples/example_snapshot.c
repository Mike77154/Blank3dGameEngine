#include "numsys.h"
#include "numsys_packs.h"

/*
    Save/load without stdio:
    ns_export_values() writes into caller-owned memory.
    Your engine can later serialize that memory with its own file/VFS layer.
*/

#define CHECK(expr) do { if (!(expr)) { return 1; } } while (0)

static NS_World world;
static NS_World loaded;
static NS_SnapshotValue snapshot[32];

int main(void)
{
    int count;

    ns_init(&world);
    CHECK(ns_define_pack(&world, NS_PACK_SURVIVAL, NS_PACK_SURVIVAL_COUNT) == NS_OK);
    CHECK(ns_attach(&world, 1, "health") >= 0);
    CHECK(ns_sub(&world, 1, "health", NS_FX_FROM_INT(35)) == NS_OK);

    CHECK(ns_export_values(&world, snapshot, 32, &count) == NS_OK);

    ns_init(&loaded);
    CHECK(ns_import_values(&loaded, snapshot, count) == NS_OK);
    CHECK(ns_get_or(&loaded, 1, "health", NS_FX_ZERO) == NS_FX_FROM_INT(65));

    return 0;
}
