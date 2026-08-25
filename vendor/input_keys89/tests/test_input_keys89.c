#include "input_keys89.h"

#include <stdio.h>
#include <string.h>

static int expect_usage(const char *name, unsigned int expected)
{
    input_key89 key;
    unsigned int usage;
    key = input_keys89_from_name(name);
    if (!input_keys89_keyboard_usage(key, &usage) || usage != expected) {
        printf("FAIL name=%s expected=0x%02x got=0x%02x\n",
               name, expected, key == INPUT_KEY89_NONE ? 0U : INPUT_KEY89_USAGE(key));
        return 0;
    }
    return 1;
}

int main(void)
{
    char name[32];
    input_key89 key;
    unsigned int page;
    unsigned int usage;

    if (!expect_usage("W", 0x1AU)) return 1;
    if (!expect_usage("1", 0x1EU)) return 2;
    if (!expect_usage("0", 0x27U)) return 3;
    if (!expect_usage("Up", 0x52U)) return 4;
    if (!expect_usage("Arrow_Up", 0x52U)) return 5;
    if (!expect_usage("Cursor-Up", 0x52U)) return 6;
    if (!expect_usage("PageUp", 0x4BU)) return 7;
    if (!expect_usage("page_up", 0x4BU)) return 8;
    if (!expect_usage("LeftShift", 0xE1U)) return 9;
    if (!expect_usage("F24", 0x73U)) return 10;
    if (!expect_usage("kp_7", 0x5FU)) return 11;
    if (!expect_usage("Numpad7", 0x5FU)) return 12;
    if (!expect_usage("kb_0x52", 0x52U)) return 13;

    key = input_keys89_from_name("Up");
    if (!input_keys89_to_hid(key, &page, &usage) ||
        page != 0x07U || usage != 0x52U) return 14;

    if (strcmp(input_keys89_name(input_keys89_from_name("W"), name,
                                 (unsigned int)sizeof(name)), "w") != 0) return 15;
    if (strcmp(input_keys89_name(input_keys89_from_name("F24"), name,
                                 (unsigned int)sizeof(name)), "f24") != 0) return 16;
    if (strcmp(input_keys89_name(input_keys89_from_name("LeftShift"), name,
                                 (unsigned int)sizeof(name)), "lshift") != 0) return 17;
    if (strcmp(input_keys89_name(input_keys89_from_name("PageUp"), name,
                                 (unsigned int)sizeof(name)), "page_up") != 0) return 18;

    key = input_keys89_from_hid(0x07U, 0x52U);
    if (key != INPUT_KEY89_UP) return 19;
    if (input_keys89_from_hid(0x100U, 0x52U) != INPUT_KEY89_NONE) return 20;
    key = input_keys89_from_hid(0x0CU, 0xE9U);
    if (strcmp(input_keys89_name(key, name, (unsigned int)sizeof(name)),
               "hid:0c:e9") != 0) return 21;
    if (input_keys89_from_name("hid:0c:e9") != key) return 22;

    puts("input_keys89 standalone tests: OK");
    return 0;
}
