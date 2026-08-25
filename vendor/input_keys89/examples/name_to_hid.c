#include "input_keys89.h"
#include <stdio.h>

int main(void)
{
    input_key89 key;
    unsigned int usage;
    key = input_keys89_from_name("ArrowUp");
    if (!input_keys89_keyboard_usage(key, &usage)) return 1;
    printf("ArrowUp -> HID keyboard usage 0x%02x\n", usage);
    return 0;
}
