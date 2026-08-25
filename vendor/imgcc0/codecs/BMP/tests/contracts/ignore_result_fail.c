#include "bmp.h"

int main(void)
{
    bmp_image img;
    bmp_parse_memory(NULL, 0u, &img);
    return 0;
}
