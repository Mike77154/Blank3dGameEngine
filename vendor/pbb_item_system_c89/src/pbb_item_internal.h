#ifndef PBB_ITEM_INTERNAL_H
#define PBB_ITEM_INTERNAL_H

#include "pbb_item_system.h"

void pbb_item_i_copy_name(char *dst, const char *src);
int pbb_item_i_flag_id_valid(int flag_id);
int pbb_item_i_flag_get_words(const unsigned long *words, int flag_id);
int pbb_item_i_flag_set_words(unsigned long *words, int flag_id, int value);
PBB_Fixed pbb_item_i_fixed_abs(PBB_Fixed v);
int pbb_item_i_hook_valid(int hook);
int pbb_item_i_var_valid(int var_id, int max_vars);
int pbb_item_i_custom_valid(int custom_id);
int pbb_item_i_spawn_internal(PBB_ItemWorld *world, int def_id, PBB_Fixed x, PBB_Fixed y, int amount, int state, int run_spawn_hook, int emit_spawn_event);

#endif
