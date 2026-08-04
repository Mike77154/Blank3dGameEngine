#include <stdio.h>
#include <string.h>
#include "gkinv.h"

enum {
    ITEM_GREEN_HERB = 1,
    ITEM_RED_HERB = 2,
    ITEM_MIXED_HERB = 3,
    ITEM_HANDGUN = 4,
    ITEM_AMMO_9MM = 5,
    ITEM_HOSPITAL_KEY = 6
};

typedef struct TestPlayer {
    int hp;
} TestPlayer;

typedef struct TextBuffer {
    char data[2048];
    gkinv_u16 used;
} TextBuffer;

static const gkinv_ItemDef g_items[] = {
    { ITEM_GREEN_HERB, "Green Herb", 3u, 1u, 1u, GKINV_ITEM_STACKABLE | GKINV_ITEM_CONSUMABLE | GKINV_ITEM_USABLE | GKINV_ITEM_COMBINABLE, GKINV_CAT_CONSUMABLE, GKINV_FX_FROM_INT(1), 0L, 0L },
    { ITEM_RED_HERB, "Red Herb", 3u, 1u, 1u, GKINV_ITEM_STACKABLE | GKINV_ITEM_COMBINABLE, GKINV_CAT_CONSUMABLE, GKINV_FX_FROM_INT(1), 0L, 0L },
    { ITEM_MIXED_HERB, "Mixed Herb", 1u, 1u, 1u, GKINV_ITEM_CONSUMABLE | GKINV_ITEM_USABLE, GKINV_CAT_CONSUMABLE, GKINV_FX_FROM_INT(1), 0L, 0L },
    { ITEM_HANDGUN, "Handgun", 1u, 2u, 2u, GKINV_ITEM_EQUIPPABLE | GKINV_ITEM_USABLE | GKINV_ITEM_PERSIST_STATE, GKINV_CAT_WEAPON, GKINV_FX_FROM_INT(2), 0L, 0L },
    { ITEM_AMMO_9MM, "9mm Ammo", 30u, 1u, 1u, GKINV_ITEM_STACKABLE, GKINV_CAT_AMMO, ((gkinv_fx)(GKINV_FX_ONE / 10L)), 0L, 0L },
    { ITEM_HOSPITAL_KEY, "Hospital Key", 1u, 1u, 1u, GKINV_ITEM_KEY | GKINV_ITEM_NO_DISCARD, GKINV_CAT_KEY, GKINV_FX_ZERO, 0L, 0L }
};

static const gkinv_ItemDB g_db = { g_items, (gkinv_u16)(sizeof(g_items) / sizeof(g_items[0])) };

static const gkinv_Recipe g_recipes[] = {
    { ITEM_GREEN_HERB, 1u, ITEM_RED_HERB, 1u, ITEM_MIXED_HERB, 1u, 0UL }
};

static int use_hook(gkinv_Container *container,
                    const gkinv_ItemDB *db,
                    gkinv_u16 slot_index,
                    void *actor,
                    void *target,
                    void *user)
{
    TestPlayer *player;

    GKINV_UNUSED(container);
    GKINV_UNUSED(db);
    GKINV_UNUSED(slot_index);
    GKINV_UNUSED(target);
    GKINV_UNUSED(user);

    player = (TestPlayer *)actor;
    if (player != (TestPlayer *)0) {
        player->hp += 25;
    }
    return GKINV_OK;
}

static int write_to_buffer(void *user, const char *data, gkinv_u16 size)
{
    TextBuffer *buffer;
    gkinv_u16 i;

    buffer = (TextBuffer *)user;
    if (buffer == (TextBuffer *)0) {
        return GKINV_ERR_NULL;
    }
    if ((gkinv_u32)buffer->used + (gkinv_u32)size + 1UL > (gkinv_u32)sizeof(buffer->data)) {
        return GKINV_ERR_FULL;
    }
    i = 0u;
    while (i < size) {
        buffer->data[buffer->used] = data[i];
        buffer->used = (gkinv_u16)(buffer->used + 1u);
        i = (gkinv_u16)(i + 1u);
    }
    buffer->data[buffer->used] = '\0';
    return GKINV_OK;
}

static int expect_ok(int rc, const char *label)
{
    if (rc != GKINV_OK) {
        printf("FAIL %s: %s\n", label, gkinv_result_name(rc));
        return 0;
    }
    return 1;
}

static int expect_true(int cond, const char *label)
{
    if (!cond) {
        printf("FAIL %s\n", label);
        return 0;
    }
    return 1;
}

int main(void)
{
    GKINV_DECLARE_SLOTS(inv_slots, 8u);
    GKINV_DECLARE_SLOTS(grid_slots, 8u);
    GKINV_DECLARE_SLOTS(load_slots, 8u);
    gkinv_Container inv;
    gkinv_Container grid;
    gkinv_Container loaded;
    gkinv_Hooks hooks;
    TestPlayer player;
    TextBuffer buffer;
    gkinv_u16 index;
    int pass;
    int rc;

    pass = 1;
    player.hp = 50;
    buffer.used = 0u;
    buffer.data[0] = '\0';

    gkinv_container_init(&inv, inv_slots, 8u, GKINV_CONT_STACKS | GKINV_CONT_ALLOW_SWAP);
    hooks.can_add = (gkinv_CanAddFn)0;
    hooks.can_remove = (gkinv_CanRemoveFn)0;
    hooks.on_use = use_hook;
    hooks.on_event = (gkinv_EventFn)0;
    gkinv_container_set_hooks(&inv, &hooks, (void *)0);

    pass = pass && expect_ok(gkinv_add_item(&g_db, &inv, ITEM_GREEN_HERB, 2u, &index), "add herb x2");
    pass = pass && expect_ok(gkinv_add_item(&g_db, &inv, ITEM_GREEN_HERB, 2u, &index), "add herb x2 second stack");
    pass = pass && expect_true(gkinv_count_item(&inv, ITEM_GREEN_HERB) == 4u, "herb count 4");
    pass = pass && expect_true(inv.count == 2u, "herb uses two stacks");
    pass = pass && expect_ok(gkinv_remove_item(&g_db, &inv, ITEM_GREEN_HERB, 1u), "remove herb");
    pass = pass && expect_true(gkinv_count_item(&inv, ITEM_GREEN_HERB) == 3u, "herb count 3");

    pass = pass && expect_ok(gkinv_add_item(&g_db, &inv, ITEM_RED_HERB, 1u, &index), "add red herb");
    pass = pass && expect_ok(gkinv_find_item(&inv, ITEM_GREEN_HERB, &index), "find green herb");
    {
        gkinv_u16 red_index;
        pass = pass && expect_ok(gkinv_find_item(&inv, ITEM_RED_HERB, &red_index), "find red herb");
        rc = gkinv_combine_slots(&g_db, &inv, index, red_index, g_recipes, (gkinv_u16)(sizeof(g_recipes) / sizeof(g_recipes[0])));
    }
    pass = pass && expect_ok(rc, "combine herbs");
    pass = pass && expect_true(gkinv_count_item(&inv, ITEM_MIXED_HERB) == 1u, "mixed herb exists");

    pass = pass && expect_ok(gkinv_find_item(&inv, ITEM_MIXED_HERB, &index), "find mixed herb");
    pass = pass && expect_ok(gkinv_use_slot(&g_db, &inv, index, &player, (void *)0), "use mixed herb");
    pass = pass && expect_true(player.hp == 75, "hook healed player");

    gkinv_container_init(&grid, grid_slots, 8u, GKINV_CONT_STACKS | GKINV_CONT_GRID);
    gkinv_container_set_grid(&grid, 4u, 4u);
    pass = pass && expect_ok(gkinv_add_item(&g_db, &grid, ITEM_HANDGUN, 1u, &index), "grid add handgun");
    pass = pass && expect_true(grid.slots[index].x == 0u && grid.slots[index].y == 0u, "handgun at origin");
    pass = pass && expect_ok(gkinv_move_grid_slot(&g_db, &grid, index, 2u, 2u), "grid move handgun");
    pass = pass && expect_true(grid.slots[index].x == 2u && grid.slots[index].y == 2u, "handgun moved");
    pass = pass && expect_ok(gkinv_add_item(&g_db, &grid, ITEM_AMMO_9MM, 30u, &index), "grid add ammo");

    pass = pass && expect_ok(gkinv_save_text(&grid, write_to_buffer, &buffer), "save text");
    gkinv_container_init(&loaded, load_slots, 8u, GKINV_CONT_STACKS | GKINV_CONT_GRID);
    gkinv_container_set_grid(&loaded, 4u, 4u);
    pass = pass && expect_ok(gkinv_load_text(&g_db, &loaded, buffer.data), "load text");
    pass = pass && expect_true(loaded.count == grid.count, "loaded count matches");
    pass = pass && expect_ok(gkinv_validate_container(&g_db, &loaded), "validate loaded");

    if (pass) {
        printf("GKINV tests passed.\n");
        return 0;
    }
    return 1;
}
