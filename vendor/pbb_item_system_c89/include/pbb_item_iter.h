#ifndef PBB_ITEM_ITER_H
#define PBB_ITEM_ITER_H
#include "pbb_item_types.h"
#ifdef __cplusplus
extern "C" {
#endif
void pbb_item_iter_begin(PBB_ItemIter *iter, unsigned long required_flags, unsigned long rejected_flags, int state, unsigned long category_mask);
void pbb_item_iter_visible_begin(PBB_ItemIter *iter);
void pbb_item_iter_active_begin(PBB_ItemIter *iter);
int pbb_item_iter_next(PBB_ItemWorld *world, PBB_ItemIter *iter, PBB_Item **out_item);
int pbb_item_iter_next_const(const PBB_ItemWorld *world, PBB_ItemIter *iter, const PBB_Item **out_item);
#ifdef __cplusplus
}
#endif
#endif
