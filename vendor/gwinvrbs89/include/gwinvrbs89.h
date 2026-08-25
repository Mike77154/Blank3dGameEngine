#ifndef GWINVRBS89_H
#define GWINVRBS89_H
#include "genwinconfigc89.h"
#ifdef __cplusplus
extern "C" {
#endif
#define GWVRB89_UNHANDLED 0
#define GWVRB89_HANDLED 1
#define GWVRB89_ERROR -1
int gwvrb89_action_count(void);
const char *gwvrb89_action_name(int ordinal);
int gwvrb89_condition_count(void);
const char *gwvrb89_condition_name(int ordinal);
int gwvrb89_perform(gwc89_state *state,const char *name,const char **argv,int argc);
int gwvrb89_query(const gwc89_state *state,const char *name,int *out_truth);
#ifdef __cplusplus
}
#endif
#endif
