#include "bmp/bmp.h"
#include <string.h>

int main(void)
{
    bmp_image img;
    char buffer[128];
    memset(&img, 0, sizeof(img));
    return bmp_format_diagnostics_text(&img, buffer, sizeof(buffer));
}
