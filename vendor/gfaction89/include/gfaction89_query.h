#ifndef GFACTION89_QUERY_H
#define GFACTION89_QUERY_H

#include "gfaction89.h"

#ifdef __cplusplus
extern "C" {
#endif

int gfa_decision_has_flag(const GFA_Decision *decision, int flag);
int gfa_is_hostile(const GFA_Decision *decision);
int gfa_is_ally(const GFA_Decision *decision);
int gfa_should_flee(const GFA_Decision *decision);
int gfa_should_protect(const GFA_Decision *decision);

#ifdef __cplusplus
}
#endif

#endif
