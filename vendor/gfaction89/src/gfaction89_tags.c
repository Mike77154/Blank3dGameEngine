#include "gfaction89_tags.h"

void gfa_tagmask_clear(GFA_TagMask *mask)
{
    int i;
    if (!mask) return;
    for (i = 0; i < GFA_TAG_WORDS; ++i) mask->word[i] = 0UL;
}

int gfa_tagmask_add(GFA_TagMask *mask, int tag_id)
{
    int word_index;
    int bit_index;
    if (!mask) return GFA_FALSE;
    if (tag_id < 0 || tag_id >= GFA_MAX_TAGS) return GFA_FALSE;
    word_index = tag_id / (int)(sizeof(unsigned long) * 8);
    bit_index = tag_id % (int)(sizeof(unsigned long) * 8);
    if (word_index < 0 || word_index >= GFA_TAG_WORDS) return GFA_FALSE;
    mask->word[word_index] |= (1UL << bit_index);
    return GFA_TRUE;
}

int gfa_tagmask_remove(GFA_TagMask *mask, int tag_id)
{
    int word_index;
    int bit_index;
    if (!mask) return GFA_FALSE;
    if (tag_id < 0 || tag_id >= GFA_MAX_TAGS) return GFA_FALSE;
    word_index = tag_id / (int)(sizeof(unsigned long) * 8);
    bit_index = tag_id % (int)(sizeof(unsigned long) * 8);
    if (word_index < 0 || word_index >= GFA_TAG_WORDS) return GFA_FALSE;
    mask->word[word_index] &= ~(1UL << bit_index);
    return GFA_TRUE;
}

int gfa_tagmask_has(const GFA_TagMask *mask, int tag_id)
{
    int word_index;
    int bit_index;
    if (!mask) return GFA_FALSE;
    if (tag_id < 0 || tag_id >= GFA_MAX_TAGS) return GFA_FALSE;
    word_index = tag_id / (int)(sizeof(unsigned long) * 8);
    bit_index = tag_id % (int)(sizeof(unsigned long) * 8);
    if (word_index < 0 || word_index >= GFA_TAG_WORDS) return GFA_FALSE;
    return (mask->word[word_index] & (1UL << bit_index)) ? GFA_TRUE : GFA_FALSE;
}

int gfa_tagmask_intersects(const GFA_TagMask *a, const GFA_TagMask *b)
{
    int i;
    if (!a || !b) return GFA_FALSE;
    for (i = 0; i < GFA_TAG_WORDS; ++i) {
        if ((a->word[i] & b->word[i]) != 0UL) return GFA_TRUE;
    }
    return GFA_FALSE;
}
