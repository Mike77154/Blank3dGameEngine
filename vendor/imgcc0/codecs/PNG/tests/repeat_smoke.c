#include <stdio.h>
#include "png_decoder.h"
#include "png_mem89.h"
int main(int argc, char** argv)
{
    unsigned int i;
    if (argc != 2) return 2;
    png_mem89_reset();
    for (i = 0u; i < 200u; ++i)
    {
        png_image img;
        int err = png_load_file_zlib(argv[1], &img);
        if (err != PNG_DEC_OK) { printf("decode_fail %u %d\n", i, err); return 3; }
        png_free_image(&img);
        if (png_mem89_bytes_used() != 0u) { printf("leak_after_%u=%u\n", i, png_mem89_bytes_used()); return 4; }
    }
    printf("OK peak=%u capacity=%u\n", png_mem89_peak_used(), png_mem89_capacity());
    return 0;
}
