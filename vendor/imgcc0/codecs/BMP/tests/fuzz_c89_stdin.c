#include <stdio.h>
#include "fuzz_c89_common.h"

#ifndef BMP_FUZZ_MODE
#define BMP_FUZZ_MODE BMP_FUZZ_MODE_ALL
#endif

int main(void)
{
    bmp_u32 used = 0U;
    bmp_u32 want;
    bmp_u32 got;

    while (used < BMP_FUZZ_INPUT_CAPACITY) {
        want = BMP_FUZZ_INPUT_CAPACITY - used;
        if (want > 65536U) want = 65536U;
        got = (bmp_u32)fread(bmp_fuzz_input + used, 1U, want, stdin);
        used += got;
        if (got != want) break;
    }
    bmp_fuzz_process(bmp_fuzz_input, used, BMP_FUZZ_MODE);
    return 0;
}
