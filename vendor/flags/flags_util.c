/* flags_util.c - small helpers - C89 */
#include "flags_util.h"

int flagstore_pick_keys(const FlagStore *src,
                        const char **keys, int key_count,
                        FlagStore *dst)
{
    int i;
    int copied;
    FlagsValue v;

    if (!src || !keys || key_count <= 0 || !dst) return 0;

    copied = 0;
    for (i = 0; i < key_count; ++i) {
        const char *k;
        k = keys[i];
        if (!k) continue;
        if (flagstore_get(src, k, &v)) {
            if (flagstore_set(dst, k, &v)) {
                copied++;
            }
        }
    }
    return copied;
}
