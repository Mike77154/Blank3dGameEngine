#include "polls_input_keys89.h"

static unsigned int polls_key_pc_usage(key_pc_code key)
{
    int k;

    k = (int)key;

    if (key >= KEY_PC_A && key <= KEY_PC_Z)
        return 0x04U + (unsigned int)(key - KEY_PC_A);

    if (key >= KEY_PC_1 && key <= KEY_PC_9)
        return 0x1EU + (unsigned int)(key - KEY_PC_1);
    if (key == KEY_PC_0) return 0x27U;

    if (key >= KEY_PC_ENTER && key <= KEY_PC_CAPSLOCK)
        return 0x28U + (unsigned int)(key - KEY_PC_ENTER);

    if (key >= KEY_PC_F1 && key <= KEY_PC_F12)
        return 0x3AU + (unsigned int)(key - KEY_PC_F1);
    if (key >= KEY_PC_F13 && key <= KEY_PC_F24)
        return 0x68U + (unsigned int)(key - KEY_PC_F13);

    if (key >= KEY_PC_PRINT_SCREEN && key <= KEY_PC_KP_EQUAL)
        return 0x46U + (unsigned int)(key - KEY_PC_PRINT_SCREEN);

    if (key >= KEY_PC_EXECUTE && key <= KEY_PC_EXSEL)
        return 0x74U + (unsigned int)(key - KEY_PC_EXECUTE);

    if (key >= KEY_PC_KP_00 && key <= KEY_PC_KP_HEXADECIMAL)
        return 0xB0U + (unsigned int)(key - KEY_PC_KP_00);

    if (key >= KEY_PC_LCTRL && key <= KEY_PC_RGUI)
        return 0xE0U + (unsigned int)(key - KEY_PC_LCTRL);

    /* The first four legacy values already correspond to HID reserved/error
     * usages and are kept for completeness. */
    if (k >= (int)KEY_PC_NONE && k <= (int)KEY_PC_ERROR_UNDEFINED)
        return (unsigned int)k;

    return 0U;
}

input_key89 polls_key_pc_to_input_key89(key_pc_code key)
{
    unsigned int usage;

    usage = polls_key_pc_usage(key);
    if (usage == 0U && key != KEY_PC_NONE) return INPUT_KEY89_NONE;
    if (key == KEY_PC_NONE) return INPUT_KEY89_NONE;
    return INPUT_KEY89_KB(usage);
}

key_pc_code polls_key_pc_from_input_key89(input_key89 key)
{
    int i;
    unsigned int usage;

    if (key == INPUT_KEY89_NONE) return KEY_PC_NONE;
    if (!input_keys89_keyboard_usage(key, &usage)) return KEY_PC_NONE;

    for (i = 1; i < (int)KEY_PC_COUNT; ++i) {
        if (polls_key_pc_usage((key_pc_code)i) == usage)
            return (key_pc_code)i;
    }

    return KEY_PC_NONE;
}

int polls_key_pc_to_keyboard_usage(key_pc_code key, unsigned int *out_usage)
{
    input_key89 canonical;

    if (!out_usage) return 0;
    canonical = polls_key_pc_to_input_key89(key);
    if (canonical == INPUT_KEY89_NONE) return 0;
    return input_keys89_keyboard_usage(canonical, out_usage);
}
