#include <stdio.h>
#include "glatetail89.h"

int main(void)
{
    printf("glatetail89: context=%u bytes\n", (unsigned)glt89_context_bytes());
    return glt89_platform_ok() ? 0 : 1;
}
