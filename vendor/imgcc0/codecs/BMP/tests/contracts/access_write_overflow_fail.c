#include "bmp/bmp.h"

int main(void)
{
    bmp_image img;
    char tiny[4];
    return bmp_format_diagnostics_text(&img, tiny, 64u);
}
