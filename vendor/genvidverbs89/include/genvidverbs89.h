#ifndef GENVIDVERBS89_H
#define GENVIDVERBS89_H
#include "generalvideoconfigc89.h"
#ifdef __cplusplus
extern "C" {
#endif
#define GVVRB89_UNHANDLED 0
#define GVVRB89_HANDLED 1
#define GVVRB89_ERROR -1
int gvvrb89_action_count(void);
const char *gvvrb89_action_name(int ordinal);
int gvvrb89_condition_count(void);
const char *gvvrb89_condition_name(int ordinal);
int gvvrb89_perform(gvc89_state *state,const char *name,const char **argv,int argc);
int gvvrb89_query(const gvc89_state *state,const char *name,int *out_truth);
#ifdef __cplusplus
}
#endif
#endif
