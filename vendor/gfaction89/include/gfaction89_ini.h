#ifndef GFACTION89_INI_H
#define GFACTION89_INI_H

#include "gfaction89.h"

#ifdef __cplusplus
extern "C" {
#endif

int gfa_ini_apply_pair(GFA_World *world, const char *section, const char *key, const char *value);
int gfa_ini_tokenize_csv(const char *text, char tokens[GFA_MAX_TOKENS][GFA_MAX_NAME], int max_tokens);

#ifdef __cplusplus
}
#endif

#endif
