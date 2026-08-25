#include <string.h>
#include "zragflib.h"

int main(void)
{
    static const zragf_u8 src[] = "c89 smoke";
    zragf_u8 comp[512];
    zragf_u8 out[512];
    zragf_size_t comp_size = sizeof(comp);
    zragf_size_t out_size = sizeof(out);
    zragf_info info;
    zragf_status st;

    st = zragf_compress(src, (zragf_size_t)(sizeof(src) - 1u), comp, &comp_size, ZRAGF_LEVEL_DEFAULT);
    if (st != ZRAGF_ST_OK)
        return 1;

    st = zragf_decompress(comp, comp_size, out, &out_size, &info);
    if (st != ZRAGF_ST_OK)
        return 2;
    if (out_size != (zragf_size_t)(sizeof(src) - 1u))
        return 3;
    if (memcmp(src, out, sizeof(src) - 1u) != 0)
        return 4;
    return 0;
}
