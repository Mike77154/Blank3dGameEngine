#include "pbb_item_internal.h"
#include <string.h>

int pbb_item_drop_table_create(PBB_ItemWorld *world, const char *name)
{
    PBB_ItemDropTable *table;
    int i;

    if (world == 0) {
        return PBB_ITEM_INVALID_ID;
    }

    for (i = 0; i < PBB_ITEM_MAX_DROP_TABLES; ++i) {
        if (!world->drop_tables[i].used) {
            table = &world->drop_tables[i];
            memset(table, 0, sizeof(*table));
            table->used = 1;
            table->table_id = i;
            pbb_item_i_copy_name(table->name, name);
            return i;
        }
    }

    return PBB_ITEM_INVALID_ID;
}

int pbb_item_drop_table_add_entry(PBB_ItemWorld *world,
                                  int table_id,
                                  int def_id,
                                  int min_amount,
                                  int max_amount,
                                  int chance_per_10000,
                                  PBB_Fixed offset_x,
                                  PBB_Fixed offset_y)
{
    PBB_ItemDropEntry *entry;
    int i;
    int tmp;

    if (world == 0) {
        return PBB_ITEM_INVALID_ID;
    }

    if (table_id < 0 || table_id >= PBB_ITEM_MAX_DROP_TABLES) {
        return PBB_ITEM_INVALID_ID;
    }

    if (!world->drop_tables[table_id].used) {
        return PBB_ITEM_INVALID_ID;
    }

    if (pbb_item_def_get(world, def_id) == 0) {
        return PBB_ITEM_INVALID_ID;
    }

    if (min_amount <= 0) {
        min_amount = 1;
    }

    if (max_amount <= 0) {
        max_amount = min_amount;
    }

    if (max_amount < min_amount) {
        tmp = min_amount;
        min_amount = max_amount;
        max_amount = tmp;
    }

    if (chance_per_10000 < 0) {
        chance_per_10000 = 0;
    }

    if (chance_per_10000 > 10000) {
        chance_per_10000 = 10000;
    }

    for (i = 0; i < PBB_ITEM_MAX_DROP_ENTRIES; ++i) {
        if (!world->drop_entries[i].used) {
            entry = &world->drop_entries[i];
            memset(entry, 0, sizeof(*entry));
            entry->used = 1;
            entry->table_id = table_id;
            entry->def_id = def_id;
            entry->min_amount = min_amount;
            entry->max_amount = max_amount;
            entry->chance_per_10000 = chance_per_10000;
            entry->offset_x = offset_x;
            entry->offset_y = offset_y;
            return i;
        }
    }

    return PBB_ITEM_INVALID_ID;
}

int pbb_item_drop_table_roll_at(PBB_ItemWorld *world,
                                int table_id,
                                int actor_id,
                                PBB_Fixed x,
                                PBB_Fixed y)
{
    PBB_ItemDropEntry *entry;
    PBB_Item *item;
    int i;
    int spawned_count;
    int amount;
    int roll;
    int item_id;

    if (world == 0) {
        return 0;
    }

    if (table_id < 0 || table_id >= PBB_ITEM_MAX_DROP_TABLES) {
        return 0;
    }

    if (!world->drop_tables[table_id].used) {
        return 0;
    }

    if (actor_id != PBB_ITEM_INVALID_ID) {
        if (pbb_item_actor_get(world, actor_id) == 0) {
            return 0;
        }
    }

    spawned_count = 0;
    for (i = 0; i < PBB_ITEM_MAX_DROP_ENTRIES; ++i) {
        entry = &world->drop_entries[i];
        if (entry->used && entry->table_id == table_id) {
            roll = pbb_item_world_rand_range(world, 0, 9999);
            if (roll < entry->chance_per_10000) {
                amount = pbb_item_world_rand_range(world, entry->min_amount, entry->max_amount);
                item_id = pbb_item_i_spawn_internal(world,
                                                  entry->def_id,
                                                  x + entry->offset_x,
                                                  y + entry->offset_y,
                                                  amount,
                                                  PBB_ITEM_STATE_DROPPED,
                                                  1,
                                                  1);
                if (item_id != PBB_ITEM_INVALID_ID) {
                    item = pbb_item_get(world, item_id);
                    if (item != 0) {
                        item->owner_actor_id = actor_id;
                    }
                    pbb_item_event_push(world,
                                        PBB_EVENT_ITEM_DROPPED,
                                        actor_id,
                                        item_id,
                                        entry->def_id,
                                        amount,
                                        table_id,
                                        0,
                                        x + entry->offset_x,
                                        y + entry->offset_y);
                    spawned_count += 1;
                }
            }
        }
    }

    return spawned_count;
}

int pbb_item_drop_table_roll_from_actor(PBB_ItemWorld *world,
                                        int table_id,
                                        int actor_id)
{
    PBB_ItemActor *actor;

    actor = pbb_item_actor_get(world, actor_id);
    if (actor == 0) {
        return 0;
    }

    return pbb_item_drop_table_roll_at(world, table_id, actor_id, actor->x, actor->y);
}
