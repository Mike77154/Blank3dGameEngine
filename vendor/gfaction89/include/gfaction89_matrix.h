#ifndef GFACTION89_MATRIX_H
#define GFACTION89_MATRIX_H

#include "gfaction89.h"

#ifdef __cplusplus
extern "C" {
#endif

GFA_Relation gfa_make_relation(int disposition, int priority, int flags, int score_bias);
int gfa_relation_flags_from_disposition(int disposition);
int gfa_relation_base_score(int disposition);

#ifdef __cplusplus
}
#endif

#endif
