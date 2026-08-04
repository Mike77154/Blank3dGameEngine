#include <stdio.h>
#include "gmuzzlegas89.h"

int main(void)
{
    printf("gmuzzlegas89: context=%u bytes\n", (unsigned)gmg89_context_bytes());
    return gmg89_platform_ok() ? 0 : 1;
}
