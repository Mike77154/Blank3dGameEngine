#include <stdio.h>
#include <string.h>
#include "zragflib.h"

int main(void)
{
    static const zragf_u8 src[] = "c89 example payload";
    zragf_u8 comp[512];
    zragf_u8 out[512];
    zragf_size_t comp_size = sizeof(comp);
    zragf_size_t out_size = sizeof(out);
    zragf_info info;

    if (zragf_compress(src, (zragf_size_t)(sizeof(src) - 1u), comp, &comp_size, ZRAGF_LEVEL_DEFAULT) != ZRAGF_ST_OK)
        return 1;
    if (zragf_decompress(comp, comp_size, out, &out_size, &info) != ZRAGF_ST_OK)
        return 2;
    printf("c89 roundtrip=%s size=%lu\n",
           (memcmp(src, out, sizeof(src) - 1u) == 0) ? "ok" : "bad",
           (unsigned long)comp_size);
    return 0;
}
