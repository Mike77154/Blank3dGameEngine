#include <stdio.h>
#include "chuecka89.h"

int main(void)
{
    printf("sizeof(ch89_context) = %lu bytes\n", (unsigned long)sizeof(ch89_context));
    printf("sizeof(ch89_gesture) = %lu bytes\n", (unsigned long)sizeof(ch89_gesture));
    printf("max strokes          = %u\n", (unsigned)CH89_MAX_STROKES);
    return 0;
}
