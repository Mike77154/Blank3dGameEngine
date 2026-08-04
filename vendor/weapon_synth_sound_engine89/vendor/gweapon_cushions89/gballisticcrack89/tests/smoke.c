#include <stdio.h>
#include "gballisticcrack89.h"

int main(void)
{
    printf("gballisticcrack89: context=%u bytes\n", (unsigned)gbc89_context_bytes());
    return gbc89_platform_ok() ? 0 : 1;
}
