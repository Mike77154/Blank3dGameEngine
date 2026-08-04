#include "io/io.h"

#include <string.h>

int ddsl_io_copy_source(char *dst, int dst_cap, const char *src) {
    int n;
    if (!dst || dst_cap <= 0) return 0;
    if (!src) src = "";
    n = (int)strlen(src);
    if (n > dst_cap - 1) n = dst_cap - 1;
    if (n > 0) memcpy(dst, src, (size_t)n);
    dst[n] = '\0';
    return n;
}
