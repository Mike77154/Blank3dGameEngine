#include "sfnt_decoder.h"

static const sfnt_u8 be_test[8] = { 0x12u, 0x34u, 0x89u, 0xabu, 0x00u, 0x00u, 0x00u, 0x00u };

int main(void)
{
    char tag[5];
    sfnt_u16 a;
    sfnt_u32 b;

    a = sfnt_read_u16(be_test);
    b = sfnt_read_u32(be_test);
    if (a != 0x1234u) return 1;
    if (b != 0x123489abu) return 2;
    sfnt_tag_to_chars(SFNT_TAG('h','e','a','d'), tag);
    if (tag[0] != 'h' || tag[1] != 'e' || tag[2] != 'a' || tag[3] != 'd' || tag[4] != 0) return 3;
    return 0;
}
