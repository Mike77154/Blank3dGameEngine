#include <stdio.h>
#include "gweaponbody89.h"

int main(void)
{
    printf("gweaponbody89: context=%u bytes\n", (unsigned)gwb89_context_bytes());
    return gwb89_platform_ok() ? 0 : 1;
}
