#include <stdio.h>
#include "gkinv.h"

enum {
    ITEM_PISTOL = 1,
    ITEM_SHOTGUN = 2,
    ITEM_AMMO = 3
};

static const gkinv_ItemDef items[] = {
    { ITEM_PISTOL, "Pistol", 1u, 2u, 2u, GKINV_ITEM_EQUIPPABLE, GKINV_CAT_WEAPON, GKINV_FX_FROM_INT(2), 0L, 0L },
    { ITEM_SHOTGUN, "Shotgun", 1u, 4u, 2u, GKINV_ITEM_EQUIPPABLE, GKINV_CAT_WEAPON, GKINV_FX_FROM_INT(5), 0L, 0L },
    { ITEM_AMMO, "Ammo", 30u, 1u, 1u, GKINV_ITEM_STACKABLE, GKINV_CAT_AMMO, ((gkinv_fx)(GKINV_FX_ONE / 10L)), 0L, 0L }
};

static const gkinv_ItemDB db = { items, (gkinv_u16)(sizeof(items) / sizeof(items[0])) };

int main(void)
{
    GKINV_DECLARE_SLOTS(case_slots, 12u);
    gkinv_Container attach_case;
    gkinv_u16 index;
    gkinv_u16 i;
    const gkinv_ItemDef *def;

    gkinv_container_init(&attach_case, case_slots, 12u, GKINV_CONT_STACKS | GKINV_CONT_GRID);
    gkinv_container_set_grid(&attach_case, 6u, 4u);

    gkinv_add_item(&db, &attach_case, ITEM_PISTOL, 1u, &index);
    gkinv_add_item(&db, &attach_case, ITEM_AMMO, 30u, &index);
    gkinv_add_item(&db, &attach_case, ITEM_SHOTGUN, 1u, &index);

    i = 0u;
    while (i < attach_case.slot_capacity) {
        if (attach_case.slots[i].item_id != (gkinv_u16)GKINV_EMPTY_ITEM_ID) {
            def = gkinv_db_find(&db, attach_case.slots[i].item_id);
            printf("%s at %u,%u qty %u\n",
                   def != (const gkinv_ItemDef *)0 ? def->name : "?",
                   (unsigned)attach_case.slots[i].x,
                   (unsigned)attach_case.slots[i].y,
                   (unsigned)attach_case.slots[i].quantity);
        }
        i = (gkinv_u16)(i + 1u);
    }

    return 0;
}
