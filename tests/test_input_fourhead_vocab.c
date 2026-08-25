#include "input_keys89.h"
#include "polls_input_keys89.h"
#include "input_hook.h"
#include "input_backend_keyboard.h"

#include <stdio.h>

static int check_pair(const char *name, key_pc_code legacy, unsigned int usage)
{
    input_key89 key;
    input_key89 from_polls;
    key_pc_code back;
    ihk_key hook_key;

    key = input_keys89_from_name(name);
    from_polls = polls_key_pc_to_input_key89(legacy);
    back = polls_key_pc_from_input_key89(key);
    hook_key = input_hook_key_from_name(name);

    if (key != INPUT_KEY89_KB(usage)) return 0;
    if (from_polls != key) return 0;
    if (back != legacy) return 0;
    if ((input_key89)hook_key != key) return 0;
    return 1;
}

int main(void)
{
    InputKeyboardBackend keyboard;

    if (input_hook_key_from_name("0x52") != IHK_HID_KB(0x52U) ||
        input_hook_key_from_name("kb_0x52") != IHK_HID_KB(0x52U) ||
        input_hook_key_from_name("hid:07:52") != IHK_HID_KB(0x52U)) {
        puts("legacy explicit HID spelling compatibility failed");
        return 6;
    }

    if (!check_pair("W", KEY_PC_W, 0x1AU) ||
        !check_pair("1", KEY_PC_1, 0x1EU) ||
        !check_pair("0", KEY_PC_0, 0x27U) ||
        !check_pair("Up", KEY_PC_UP, 0x52U) ||
        !check_pair("F13", KEY_PC_F13, 0x68U) ||
        !check_pair("F24", KEY_PC_F24, 0x73U) ||
        !check_pair("LeftShift", KEY_PC_LSHIFT, 0xE1U)) {
        puts("four-head canonical vocabulary mapping failed");
        return 1;
    }

    if (input_keyboard_backend_init(&keyboard) != INPUT_OK) return 2;
    if (input_keyboard_backend_bind_input_key89(&keyboard, 0,
                                                 INPUT_KEY89_UP) != INPUT_OK)
        return 3;
    if (input_keyboard_backend_set_input_key89(&keyboard,
                                                INPUT_KEY89_UP, 1) != INPUT_OK)
        return 4;
    if (!input_keyboard_backend_input_key89_down(&keyboard,
                                                  INPUT_KEY89_UP))
        return 5;

    puts("four-head canonical input vocabulary: OK");
    return 0;
}
