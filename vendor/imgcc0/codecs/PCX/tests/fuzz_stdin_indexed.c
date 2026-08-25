#include "fuzz_file89.h"
int main(int argc, char **argv)
{
    return pcx89_fuzz_file_main(argc, argv, PCX89_FUZZ_MODE_INDEXED);
}
