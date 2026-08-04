#include <stdio.h>
#include "gcinemathump89.h"

int main(void)
{
    printf("gcinemathump89: context=%u bytes\n", (unsigned)gct89_context_bytes());
    return gct89_platform_ok() ? 0 : 1;
}
