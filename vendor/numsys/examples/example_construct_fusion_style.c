#include "numsys.h"
#include "numsys_packs.h"

/*
    Construct/Fusion-style usage:
    - A template acts like a family/object preset.
    - Values attached to each owner behave like alterable values.
    - Queries act like simple event-sheet picking.
*/

#define CHECK(expr) do { if (!(expr)) { return 1; } } while (0)

static NS_World world;

int main(void)
{
    ns_id enemy_template;
    ns_id health_type;
    NS_Query query;
    int dead_count;
    int affected;

    ns_init(&world);
    CHECK(ns_define_pack(&world, NS_PACK_SURVIVAL, NS_PACK_SURVIVAL_COUNT) == NS_OK);

    enemy_template = ns_define_template(&world, "enemy", 0);
    CHECK(enemy_template >= 0);
    CHECK(ns_template_add(&world, "enemy", "health") == NS_OK);
    CHECK(ns_template_add(&world, "enemy", "infection") == NS_OK);
    CHECK(ns_template_add(&world, "enemy", "armor") == NS_OK);

    CHECK(ns_attach_template(&world, 101, enemy_template) == NS_OK);
    CHECK(ns_attach_template(&world, 102, enemy_template) == NS_OK);
    CHECK(ns_attach_template(&world, 103, enemy_template) == NS_OK);

    CHECK(ns_sub(&world, 102, "health", NS_FX_FROM_INT(120)) == NS_OK);

    health_type = ns_find_type(&world, "health");
    CHECK(health_type >= 0);

    ns_query_all(&query);
    query.use_type = NS_TRUE;
    query.type_id = health_type;
    query.use_cmp = NS_TRUE;
    query.cmp = NS_CMP_LTE;
    query.rhs = NS_FX_ZERO;

    CHECK(ns_query_count(&world, &query, &dead_count) == NS_OK);
    CHECK(dead_count == 1);

    ns_query_all(&query);
    query.use_type = NS_TRUE;
    query.type_id = health_type;
    CHECK(ns_query_sub(&world, &query, NS_FX_FROM_INT(10), &affected) == NS_OK);
    CHECK(affected == 3);

    return 0;
}
