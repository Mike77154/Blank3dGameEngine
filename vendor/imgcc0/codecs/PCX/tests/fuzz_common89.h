#ifndef PCX_FUZZ_COMMON89_H
#define PCX_FUZZ_COMMON89_H

#include "../pcx.h"

#define PCX89_FUZZ_MODE_INSPECT 0
#define PCX89_FUZZ_MODE_RGB 1
#define PCX89_FUZZ_MODE_INDEXED 2

static void pcx89_fuzz_one(const pcx_u8 *data, pcx_size size, int mode)
{
    int rc;

    if (data == NULL || size == 0U)
    {
        return;
    }

    rc = PCX_OK;

    if (mode == PCX89_FUZZ_MODE_RGB)
    {
        PCXImage img;
        pcx_image_init(&img);
        rc = pcx_load_memory(data, size, &img);
        pcx_image_release(&img);
    }
    else if (mode == PCX89_FUZZ_MODE_INDEXED)
    {
        PCXIndexedImage img;
        pcx_indexed_image_init(&img);
        rc = pcx_load_indexed_memory(data, size, &img);
        pcx_indexed_image_release(&img);
    }
    else
    {
        PCXFileInfo info;
        rc = pcx_inspect_memory_ex(data, size, &info);
    }
    if (rc == PCX_ERR_INTERNAL)
    {
        return;
    }
}

#endif
