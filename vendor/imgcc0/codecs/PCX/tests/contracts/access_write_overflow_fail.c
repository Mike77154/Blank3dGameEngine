#include "pcx.h"
#include <stdio.h>

int main(void)
{
    unsigned char tiny[4];
    return pcx_read_bytes(stdin, tiny, 64u);
}
