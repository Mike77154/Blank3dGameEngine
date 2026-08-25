#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "zragflib.h"
#include "protocol89_hostmem.h"
#include "protocol89_fixed.h"

int main(void)
{
    const zragf_size_t n = 1u << 20;
    unsigned char *src = (unsigned char *)zragf_p89_host_take(n);
    unsigned char *comp = (unsigned char *)zragf_p89_host_take(n + 65536u);
    unsigned char *out = (unsigned char *)zragf_p89_host_take(n + 1024u);
    zragf_size_t i;
    zragf_size_t comp_size;
    zragf_size_t out_size;
    clock_t t0, t1, t2;
    zragf_info info;

    if (!src || !comp || !out)
        return 1;

    for (i = 0u; i < n; ++i)
        src[i] = (unsigned char)((i * 7u) & 0xFFu);

    comp_size = n + 65536u;
    out_size = n + 1024u;

    t0 = clock();
    if (zragf_compress(src, n, comp, &comp_size, ZRAGF_LEVEL_DEFAULT) != ZRAGF_ST_OK)
        return 1;
    t1 = clock();
    if (zragf_decompress(comp, comp_size, out, &out_size, &info) != ZRAGF_ST_OK)
        return 1;
    t2 = clock();

    printf("input=%lu compressed=%lu ratio=%d\n",
           (unsigned long)n,
           (unsigned long)comp_size,
           (zragf_fx)comp_size / (zragf_fx)n);
    printf("compress_ms=%d decompress_ms=%d\n",
           1000 * (zragf_fx)(t1 - t0) / (zragf_fx)CLOCKS_PER_SEC,
           1000 * (zragf_fx)(t2 - t1) / (zragf_fx)CLOCKS_PER_SEC);

    zragf_p89_host_release(src);
    zragf_p89_host_release(comp);
    zragf_p89_host_release(out);
    return 0;
}
