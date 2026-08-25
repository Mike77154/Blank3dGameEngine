#include <stdio.h>
#include "fuzz_c89_common.h"

#ifndef BMP_FUZZ_MODE
#define BMP_FUZZ_MODE BMP_FUZZ_MODE_ALL
#endif

static int read_case(const char *path, bmp_u32 *out_size)
{
    FILE *f;
    bmp_u32 used = 0U;
    bmp_u32 want;
    bmp_u32 got;
    int extra;

    if (!path || !out_size) return 0;
    f = fopen(path, "rb");
    if (!f) return 0;
    while (used < BMP_FUZZ_INPUT_CAPACITY) {
        want = BMP_FUZZ_INPUT_CAPACITY - used;
        if (want > 65536U) want = 65536U;
        got = (bmp_u32)fread(bmp_fuzz_input + used, 1U, want, f);
        used += got;
        if (got != want) {
            if (ferror(f)) { fclose(f); return 0; }
            break;
        }
    }
    if (used == BMP_FUZZ_INPUT_CAPACITY) {
        extra = fgetc(f);
        if (extra != EOF) { fclose(f); return 0; }
    }
    fclose(f);
    *out_size = used;
    return 1;
}

int main(int argc, char **argv)
{
    bmp_u32 size = 0U;
    if (argc != 2) return 2;
    if (!read_case(argv[1], &size)) return 2;
    bmp_fuzz_process(bmp_fuzz_input, size, BMP_FUZZ_MODE);
    return 0;
}
