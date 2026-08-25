#include "pbb_item_internal.h"
#include <string.h>

void pbb_item_i_copy_name(char *dst, const char *src)
{
    int i;

    if (dst == 0) {
        return;
    }

    if (src == 0) {
        dst[0] = '\0';
        return;
    }

    for (i = 0; i < PBB_ITEM_NAME_SIZE - 1; ++i) {
        dst[i] = src[i];
        if (src[i] == '\0') {
            return;
        }
    }

    dst[PBB_ITEM_NAME_SIZE - 1] = '\0';
}

int pbb_item_i_flag_id_valid(int flag_id)
{
    if (flag_id < 0) {
        return 0;
    }

    if (flag_id >= PBB_ITEM_FLAG_WORDS * 32) {
        return 0;
    }

    return 1;
}

static unsigned long pbb_item_i_flag_mask(int flag_id)
{
    return (1UL << (flag_id & 31));
}

int pbb_item_i_flag_get_words(const unsigned long *words, int flag_id)
{
    int word;
    unsigned long mask;

    if (words == 0) {
        return 0;
    }

    if (!pbb_item_i_flag_id_valid(flag_id)) {
        return 0;
    }

    word = flag_id >> 5;
    mask = pbb_item_i_flag_mask(flag_id);

    if ((words[word] & mask) != 0UL) {
        return 1;
    }

    return 0;
}

int pbb_item_i_flag_set_words(unsigned long *words, int flag_id, int value)
{
    int word;
    unsigned long mask;

    if (words == 0) {
        return 0;
    }

    if (!pbb_item_i_flag_id_valid(flag_id)) {
        return 0;
    }

    word = flag_id >> 5;
    mask = pbb_item_i_flag_mask(flag_id);

    if (value) {
        words[word] |= mask;
    } else {
        words[word] &= ~mask;
    }

    return 1;
}

PBB_Fixed pbb_item_i_fixed_abs(PBB_Fixed v)
{
    if (v < 0) {
        return -v;
    }

    return v;
}

int pbb_item_i_hook_valid(int hook)
{
    if (hook < 0) {
        return 0;
    }

    if (hook >= PBB_ITEM_HOOK_COUNT) {
        return 0;
    }

    return 1;
}

int pbb_item_i_var_valid(int var_id, int max_vars)
{
    if (var_id < 0) {
        return 0;
    }

    if (var_id >= max_vars) {
        return 0;
    }

    return 1;
}

int pbb_item_i_custom_valid(int custom_id)
{
    if (custom_id < 0) {
        return 0;
    }

    if (custom_id >= PBB_ITEM_CUSTOM_COUNT) {
        return 0;
    }

    return 1;
}

