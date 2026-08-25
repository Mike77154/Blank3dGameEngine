#include "bmp/bmp.h"

int main(void)
{
    bmp_u8 tiny[4] = {0, 0, 0, 0};
    bmp_image img;
    return bmp_parse_memory(tiny, 64u, &img);
}
