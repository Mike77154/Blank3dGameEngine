/* flags_util.h - small helpers - C89 */
#ifndef FLAGS_UTIL_H
#define FLAGS_UTIL_H

#include "flagstore.h"

/* Equivalent of python's dict_pick(d, keys):
 * - copies keys that exist in src into dst
 * - returns how many were copied
 */
int flagstore_pick_keys(const FlagStore *src,
                        const char **keys, int key_count,
                        FlagStore *dst);

#endif /* FLAGS_UTIL_H */
