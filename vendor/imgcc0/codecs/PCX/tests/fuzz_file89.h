#ifndef PCX_FUZZ_FILE89_H
#define PCX_FUZZ_FILE89_H

#include <stdio.h>
#include "fuzz_common89.h"

#ifndef PCX89_FUZZ_INPUT_BYTES
#define PCX89_FUZZ_INPUT_BYTES 8388608U
#endif

static pcx_u8 g_pcx89_fuzz_input[PCX89_FUZZ_INPUT_BYTES];

static int pcx89_fuzz_file_main(int argc, char **argv, int mode)
{
    FILE *f;
    pcx_size got;

    f = stdin;
    if (argc > 1)
    {
        f = fopen(argv[1], "rb");
        if (f == NULL)
        {
            return 2;
        }
    }

    got = (pcx_size)fread(g_pcx89_fuzz_input, 1U, PCX89_FUZZ_INPUT_BYTES, f);
    if (f != stdin)
    {
        fclose(f);
    }

    pcx89_fuzz_one(g_pcx89_fuzz_input, got, mode);
    return 0;
}

#endif
