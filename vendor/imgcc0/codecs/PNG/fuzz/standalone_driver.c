#include "fuzz_common.h"

extern int LLVMFuzzerTestOneInput(const unsigned char* data, png_u32 size);

int main(void)
{
    static unsigned char buf[PNG_FUZZ_MAX_INPUT_BYTES];
    png_u32 size;
    int ch;

    size = 0u;
    while ((ch = getchar()) != EOF)
    {
        if (size >= PNG_FUZZ_MAX_INPUT_BYTES)
            return 1;
        buf[size++] = (unsigned char)ch;
    }

    return LLVMFuzzerTestOneInput(buf, size);
}
