#ifndef GMPLYSS89_H
#define GMPLYSS89_H
#include "gameplayscreensizec89.h"
#ifdef __cplusplus
extern "C" {
#endif
#define GMPLYSS89_UNHANDLED 0
#define GMPLYSS89_HANDLED 1
#define GMPLYSS89_ERROR -1
int gmplyss89_action_count(void);
const char *gmplyss89_action_name(int ordinal);
int gmplyss89_condition_count(void);
const char *gmplyss89_condition_name(int ordinal);
int gmplyss89_perform(gpss89_state *state,const char *name,const char **argv,int argc);
int gmplyss89_query(const gpss89_state *state,const char *name,int *out_truth);
#ifdef __cplusplus
}
#endif
#endif
