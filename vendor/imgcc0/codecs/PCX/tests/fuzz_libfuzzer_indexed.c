#include "fuzz_common89.h"
int LLVMFuzzerTestOneInput(const unsigned char *data, pcx_size size);
int LLVMFuzzerTestOneInput(const unsigned char *data, pcx_size size)
{
    pcx89_fuzz_one(data, size, PCX89_FUZZ_MODE_INDEXED);
    return 0;
}
