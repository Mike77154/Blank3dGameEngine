#include "pcx.h"

int main(void)
{
    PCXImage img;
    pcx_image_init(&img);
    pcx_load_memory("", 0u, &img);
    return 0;
}
