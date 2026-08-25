#include "pcx.h"

int main(void)
{
    unsigned char tiny[4] = {0, 0, 0, 0};
    PCXImage img;
    pcx_image_init(&img);
    return pcx_load_memory(tiny, 64u, &img);
}
