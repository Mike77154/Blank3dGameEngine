#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "zragflib.h"
#include "protocol89_hostmem.h"

static int run_case(const unsigned char *src, zragf_size_t src_size)
{
    zragf_size_t comp_cap = src_size * 4u + 512u;
    zragf_size_t decomp_cap = src_size + 64u;
    unsigned char *comp = (unsigned char *)zragf_p89_host_take(comp_cap);
    unsigned char *decomp = (unsigned char *)zragf_p89_host_take(decomp_cap);
    zragf_size_t comp_size = comp_cap;
    zragf_size_t decomp_size = decomp_cap;
    zragf_info info;
    zragf_status cst;
    zragf_status dst;
    int ok = 0;

    if (!comp || !decomp) {
        zragf_p89_host_release(comp);
        zragf_p89_host_release(decomp);
        return 1;
    }

    cst = zragf_compress(src, src_size, comp, &comp_size, ZRAGF_LEVEL_DEFAULT);
    if (cst != ZRAGF_ST_OK) {
        fprintf(stderr, "native compress failed: %d\n", cst);
        goto done;
    }

    dst = zragf_decompress(comp, comp_size, decomp, &decomp_size, &info);
    if (dst != ZRAGF_ST_OK) {
        fprintf(stderr, "native decompress failed: %d\n", dst);
        goto done;
    }

    if (decomp_size != src_size || memcmp(src, decomp, src_size) != 0) {
        fprintf(stderr, "native roundtrip mismatch (src=%lu out=%lu)\n",
                (unsigned long)src_size, (unsigned long)decomp_size);
        goto done;
    }

    if (info.uncompressed_size != (zragf_u32)src_size) {
        fprintf(stderr, "native info size mismatch\n");
        goto done;
    }

    ok = 1;

done:
    zragf_p89_host_release(comp);
    zragf_p89_host_release(decomp);
    return ok ? 0 : 1;
}

int main(void)
{
    static const unsigned char text1[] = "";
    static const unsigned char text2[] = "a";
    static const unsigned char text3[] = "aaaaaa";
    static const unsigned char text4[] = "abcabcabcabc";
    static const unsigned char text5[] = "hello hello hello world world world";
    static const unsigned char bin1[] = {0x00, 0x00, 0x00, 0x01, 0x02, 0x00, 0xFF, 0x10};
    unsigned char rnd[1024];
    zragf_size_t i;

    for (i = 0u; i < sizeof(rnd); ++i)
        rnd[i] = (unsigned char)((i * 37u + 13u) & 0xFFu);

    if (run_case(text1, sizeof(text1) - 1u)) return 1;
    if (run_case(text2, sizeof(text2) - 1u)) return 1;
    if (run_case(text3, sizeof(text3) - 1u)) return 1;
    if (run_case(text4, sizeof(text4) - 1u)) return 1;
    if (run_case(text5, sizeof(text5) - 1u)) return 1;
    if (run_case(bin1, sizeof(bin1))) return 1;
    if (run_case(rnd, sizeof(rnd))) return 1;

    puts("native roundtrip ok");
    return 0;
}
