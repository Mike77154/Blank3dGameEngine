#include <bmp/bmp.h>
#include <stdio.h>

int main(void)
{
    bmp_u32 v = bmp_version_number();
    puts(bmp_version_string());
    return v == BMP_VERSION_NUMBER ? 0 : 1;
}
