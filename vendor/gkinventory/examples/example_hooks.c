#include <stdio.h>
#include "gkinv.h"

enum {
    ITEM_MEDKIT = 1
};

typedef struct Player {
    int hp;
    int max_hp;
} Player;

static const gkinv_ItemDef items[] = {
    { ITEM_MEDKIT, "Medkit", 2u, 1u, 1u, GKINV_ITEM_STACKABLE | GKINV_ITEM_CONSUMABLE | GKINV_ITEM_USABLE, GKINV_CAT_CONSUMABLE, GKINV_FX_FROM_INT(1), 0L, 0L }
};

static const gkinv_ItemDB db = { items, (gkinv_u16)(sizeof(items) / sizeof(items[0])) };

static int on_use(gkinv_Container *container,
                  const gkinv_ItemDB *item_db,
                  gkinv_u16 slot_index,
                  void *actor,
                  void *target,
                  void *user)
{
    Player *player;

    GKINV_UNUSED(container);
    GKINV_UNUSED(item_db);
    GKINV_UNUSED(slot_index);
    GKINV_UNUSED(target);
    GKINV_UNUSED(user);

    player = (Player *)actor;
    if (player == (Player *)0) {
        return GKINV_ERR_NULL;
    }
    player->hp += 50;
    if (player->hp > player->max_hp) {
        player->hp = player->max_hp;
    }
    return GKINV_OK;
}

int main(void)
{
    GKINV_DECLARE_SLOTS(slots, 4u);
    gkinv_Container inventory;
    gkinv_Hooks hooks;
    gkinv_u16 index;
    Player player;

    player.hp = 30;
    player.max_hp = 100;

    hooks.can_add = (gkinv_CanAddFn)0;
    hooks.can_remove = (gkinv_CanRemoveFn)0;
    hooks.on_use = on_use;
    hooks.on_event = (gkinv_EventFn)0;

    gkinv_container_init(&inventory, slots, 4u, GKINV_CONT_STACKS);
    gkinv_container_set_hooks(&inventory, &hooks, (void *)0);
    gkinv_add_item(&db, &inventory, ITEM_MEDKIT, 1u, &index);
    gkinv_use_slot(&db, &inventory, index, &player, (void *)0);

    printf("player hp: %d\n", player.hp);
    return 0;
}
