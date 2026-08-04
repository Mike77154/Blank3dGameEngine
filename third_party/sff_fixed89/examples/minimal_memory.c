#include "sff_api.h"
#include <stdio.h>
#include <string.h>

/* This example assumes you already own the SFF bytes in memory. */

int main(void)
{
    /* Replace these with your own data source. */
    const unsigned char *sff_bytes = 0;
    sff_u32 sff_size = 0u;
    SffFile sff;
    SffOpenOptions opt;
    sff_u32 idx;
    SffSpriteInfo info;

    opt.tolerant = 1;
    opt.codec = 0; /* built-in png89+zlib89 path works if you provide work buffers at decode time */

    if (!sff_bytes || sff_size == 0u) {
        puts("Provide your own SFF bytes first.");
        return 0;
    }

    if (sff_open_memory(&sff, sff_bytes, sff_size, &opt) != SFF_OK) {
        puts("Could not open SFF.");
        return 1;
    }

    if (sff_find_sprite(&sff, 0u, 0u, &idx) != SFF_OK) {
        puts("Sprite 0,0 not found.");
        return 1;
    }

    if (sff_get_sprite_info(&sff, idx, &info) != SFF_OK) {
        puts("Could not query sprite info.");
        return 1;
    }

    printf("sprite 0,0 -> %ux%u axis=(%d,%d)\n",
           (unsigned)info.w, (unsigned)info.h,
           (int)info.axis_x, (int)info.axis_y);
    return 0;
}
