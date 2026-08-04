#include <stdio.h>
#include "gkinv.h"

enum {
    ITEM_POTION = 1,
    ITEM_SWORD = 2,
    ITEM_KEY = 3
};

static const gkinv_ItemDef items[] = {
    { ITEM_POTION, "Potion", 9u, 1u, 1u, GKINV_ITEM_STACKABLE | GKINV_ITEM_CONSUMABLE | GKINV_ITEM_USABLE, GKINV_CAT_CONSUMABLE, GKINV_FX_FROM_INT(1), 0L, 0L },
    { ITEM_SWORD, "Sword", 1u, 1u, 1u, GKINV_ITEM_EQUIPPABLE, GKINV_CAT_WEAPON, GKINV_FX_FROM_INT(3), 0L, 0L },
    { ITEM_KEY, "Key", 1u, 1u, 1u, GKINV_ITEM_KEY, GKINV_CAT_KEY, GKINV_FX_ZERO, 0L, 0L }
};

static const gkinv_ItemDB db = { items, (gkinv_u16)(sizeof(items) / sizeof(items[0])) };

static void print_inventory(const gkinv_ItemDB *item_db, const gkinv_Container *inv)
{
    gkinv_u16 i;
    const gkinv_ItemDef *def;

    i = 0u;
    while (i < inv->slot_capacity) {
        if (inv->slots[i].item_id != (gkinv_u16)GKINV_EMPTY_ITEM_ID) {
            def = gkinv_db_find(item_db, inv->slots[i].item_id);
            printf("slot %u: %s x%u\n", (unsigned)i,
                   def != (const gkinv_ItemDef *)0 ? def->name : "?",
                   (unsigned)inv->slots[i].quantity);
        }
        i = (gkinv_u16)(i + 1u);
    }
}

int main(void)
{
    GKINV_DECLARE_SLOTS(player_slots, 8u);
    gkinv_Container player_inventory;
    gkinv_u16 index;

    gkinv_container_init(&player_inventory, player_slots, 8u, GKINV_CONT_STACKS | GKINV_CONT_ALLOW_SWAP);
    gkinv_add_item(&db, &player_inventory, ITEM_POTION, 5u, &index);
    gkinv_add_item(&db, &player_inventory, ITEM_SWORD, 1u, &index);
    gkinv_add_item(&db, &player_inventory, ITEM_KEY, 1u, &index);

    print_inventory(&db, &player_inventory);
    return 0;
}
