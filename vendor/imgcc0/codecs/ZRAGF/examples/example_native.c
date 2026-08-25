#include <stdio.h>
#include <string.h>
#include "zragflib.h"

int main(void)
{
    const char *msg = "hola desde zragflib";
    unsigned char comp[512];
    unsigned char out[512];
    zragf_size_t comp_size = sizeof(comp);
    zragf_size_t out_size = sizeof(out);
    zragf_info info;

    if (zragf_compress(msg, strlen(msg), comp, &comp_size, ZRAGF_LEVEL_DEFAULT) != ZRAGF_ST_OK)
        return 1;
    if (zragf_decompress(comp, comp_size, out, &out_size, &info) != ZRAGF_ST_OK)
        return 1;

    fwrite(out, 1, out_size, stdout);
    putchar('\n');
    return 0;
}
