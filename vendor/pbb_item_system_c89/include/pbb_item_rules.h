#ifndef PBB_ITEM_RULES_H
#define PBB_ITEM_RULES_H
#include "pbb_item_types.h"
#ifdef __cplusplus
extern "C" {
#endif
int pbb_item_eval_rule_block(PBB_ItemWorld *world, int actor_id, int item_id, int first_rule, int rule_count);
#ifdef __cplusplus
}
#endif
#endif
