#include "pbb_item_internal.h"

void pbb_item_iter_begin(PBB_ItemIter *iter,
                         unsigned long required_flags,
                         unsigned long rejected_flags,
                         int state,
                         unsigned long category_mask)
{
    if (iter == 0) {
        return;
    }

    iter->index = 0;
    iter->required_flags = required_flags;
    iter->rejected_flags = rejected_flags;
    iter->state = state;
    iter->category_mask = category_mask;
}

void pbb_item_iter_visible_begin(PBB_ItemIter *iter)
{
    pbb_item_iter_begin(iter,
                        PBB_ITEMF_VISIBLE,
                        0UL,
                        PBB_ITEM_STATE_ANY,
                        PBB_ITEM_CATEGORY_ANY);
}

void pbb_item_iter_active_begin(PBB_ItemIter *iter)
{
    pbb_item_iter_begin(iter,
                        PBB_ITEMF_ACTIVE,
                        0UL,
                        PBB_ITEM_STATE_ANY,
                        PBB_ITEM_CATEGORY_ANY);
}

static int pbb_item_iter_matches(const PBB_Item *item, const PBB_ItemIter *iter)
{
    if (item == 0 || iter == 0) {
        return 0;
    }

    if (!item->used) {
        return 0;
    }

    if ((item->flags & iter->required_flags) != iter->required_flags) {
        return 0;
    }

    if ((item->flags & iter->rejected_flags) != 0UL) {
        return 0;
    }

    if (iter->state != PBB_ITEM_STATE_ANY && item->state != iter->state) {
        return 0;
    }

    if (iter->category_mask != PBB_ITEM_CATEGORY_ANY) {
        if ((item->category_mask & iter->category_mask) == 0UL) {
            return 0;
        }
    }

    return 1;
}

int pbb_item_iter_next(PBB_ItemWorld *world,
                       PBB_ItemIter *iter,
                       PBB_Item **out_item)
{
    int i;

    if (world == 0 || iter == 0 || out_item == 0) {
        return 0;
    }

    for (i = iter->index; i < PBB_ITEM_MAX_ITEMS; ++i) {
        if (pbb_item_iter_matches(&world->items[i], iter)) {
            iter->index = i + 1;
            *out_item = &world->items[i];
            return 1;
        }
    }

    *out_item = 0;
    iter->index = PBB_ITEM_MAX_ITEMS;
    return 0;
}

int pbb_item_iter_next_const(const PBB_ItemWorld *world,
                             PBB_ItemIter *iter,
                             const PBB_Item **out_item)
{
    int i;

    if (world == 0 || iter == 0 || out_item == 0) {
        return 0;
    }

    for (i = iter->index; i < PBB_ITEM_MAX_ITEMS; ++i) {
        if (pbb_item_iter_matches(&world->items[i], iter)) {
            iter->index = i + 1;
            *out_item = &world->items[i];
            return 1;
        }
    }

    *out_item = 0;
    iter->index = PBB_ITEM_MAX_ITEMS;
    return 0;
}
