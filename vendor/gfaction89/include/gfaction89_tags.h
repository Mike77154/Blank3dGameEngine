#ifndef GFACTION89_TAGS_H
#define GFACTION89_TAGS_H

#include "gfaction89.h"

#ifdef __cplusplus
extern "C" {
#endif

void gfa_tagmask_clear(GFA_TagMask *mask);
int gfa_tagmask_add(GFA_TagMask *mask, int tag_id);
int gfa_tagmask_remove(GFA_TagMask *mask, int tag_id);
int gfa_tagmask_has(const GFA_TagMask *mask, int tag_id);
int gfa_tagmask_intersects(const GFA_TagMask *a, const GFA_TagMask *b);

#ifdef __cplusplus
}
#endif

#endif
