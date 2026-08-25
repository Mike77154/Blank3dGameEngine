#include <bmp/bmp.h>

int main(void)
{
    bmp_metadata meta;
    bmp_metadata_default(&meta);
    return meta.width == 0 ? 0 : 1;
}
