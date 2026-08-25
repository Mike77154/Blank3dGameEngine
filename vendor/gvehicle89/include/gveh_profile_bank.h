#ifndef GVEH_PROFILE_BANK_H
#define GVEH_PROFILE_BANK_H

#include "gveh.h"

typedef void (*gveh_profile_make_fn)(gveh_profile *p);

gveh_i16 gveh_profile_bank_count(void);
const char *gveh_profile_bank_name(gveh_i16 index);
gveh_i32 gveh_profile_bank_make_by_index(gveh_i16 index, gveh_profile *out_profile);
gveh_i32 gveh_profile_bank_make_by_name(const char *name, gveh_profile *out_profile);

#endif
