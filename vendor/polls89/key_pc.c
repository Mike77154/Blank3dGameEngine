/* key_pc.c - Keymap "PC keyboard" (inspirado en USB HID) - C89 */

#include "key_pc.h"

#include <string.h>
#include <ctype.h>

/* ============================================================
   Tabla de nombres por tecla (snake_case)
   ============================================================ */

static const char *key_names[KEY_PC_COUNT] = {
    "none",
    "error_rollover",
    "post_fail",
    "error_undefined",
    "a",
    "b",
    "c",
    "d",
    "e",
    "f",
    "g",
    "h",
    "i",
    "j",
    "k",
    "l",
    "m",
    "n",
    "o",
    "p",
    "q",
    "r",
    "s",
    "t",
    "u",
    "v",
    "w",
    "x",
    "y",
    "z",
    "0",
    "1",
    "2",
    "3",
    "4",
    "5",
    "6",
    "7",
    "8",
    "9",
    "enter",
    "escape",
    "backspace",
    "tab",
    "space",
    "minus",
    "equal",
    "lbracket",
    "rbracket",
    "backslash",
    "nonus_hash",
    "semicolon",
    "apostrophe",
    "grave",
    "comma",
    "dot",
    "slash",
    "capslock",
    "f1",
    "f2",
    "f3",
    "f4",
    "f5",
    "f6",
    "f7",
    "f8",
    "f9",
    "f10",
    "f11",
    "f12",
    "f13",
    "f14",
    "f15",
    "f16",
    "f17",
    "f18",
    "f19",
    "f20",
    "f21",
    "f22",
    "f23",
    "f24",
    "print_screen",
    "scroll_lock",
    "pause",
    "insert",
    "home",
    "page_up",
    "delete",
    "end",
    "page_down",
    "right",
    "left",
    "down",
    "up",
    "num_lock",
    "kp_div",
    "kp_mul",
    "kp_sub",
    "kp_add",
    "kp_enter",
    "kp_1",
    "kp_2",
    "kp_3",
    "kp_4",
    "kp_5",
    "kp_6",
    "kp_7",
    "kp_8",
    "kp_9",
    "kp_0",
    "kp_dot",
    "nonus_backslash",
    "application",
    "power",
    "kp_equal",
    "execute",
    "help",
    "menu",
    "select",
    "stop",
    "again",
    "undo",
    "cut",
    "copy",
    "paste",
    "find",
    "mute",
    "volume_up",
    "volume_down",
    "locking_caps_lock",
    "locking_num_lock",
    "locking_scroll_lock",
    "kp_comma",
    "kp_equal_as400",
    "international1",
    "international2",
    "international3",
    "international4",
    "international5",
    "international6",
    "international7",
    "international8",
    "international9",
    "lang1",
    "lang2",
    "lang3",
    "lang4",
    "lang5",
    "lang6",
    "lang7",
    "lang8",
    "lang9",
    "alternate_erase",
    "sysreq",
    "cancel",
    "clear",
    "prior",
    "return",
    "separator",
    "out",
    "oper",
    "clear_again",
    "crsel",
    "exsel",
    "kp_00",
    "kp_000",
    "thousands_separator",
    "decimal_separator",
    "currency_unit",
    "currency_subunit",
    "kp_lparen",
    "kp_rparen",
    "kp_lbrace",
    "kp_rbrace",
    "kp_tab",
    "kp_backspace",
    "kp_a",
    "kp_b",
    "kp_c",
    "kp_d",
    "kp_e",
    "kp_f",
    "kp_xor",
    "kp_caret",
    "kp_percent",
    "kp_less",
    "kp_greater",
    "kp_ampersand",
    "kp_dblampersand",
    "kp_vertical_bar",
    "kp_dblvertical_bar",
    "kp_colon",
    "kp_hash",
    "kp_space",
    "kp_at",
    "kp_exclamation",
    "kp_mem_store",
    "kp_mem_recall",
    "kp_mem_clear",
    "kp_mem_add",
    "kp_mem_sub",
    "kp_mem_mul",
    "kp_mem_div",
    "kp_plus_minus",
    "kp_clear",
    "kp_clear_entry",
    "kp_binary",
    "kp_octal",
    "kp_decimal",
    "kp_hexadecimal",
    "lctrl",
    "lshift",
    "lalt",
    "lgui",
    "rctrl",
    "rshift",
    "ralt",
    "rgui"
};

const char *key_pc_name(key_pc_code k)
{
    if ((int)k < 0 || (int)k >= (int)KEY_PC_COUNT) {
        return "unknown";
    }
    return key_names[(int)k];
}

/* ============================================================
   Búsqueda por nombre (case-insensitive)
   ============================================================ */

key_pc_code key_pc_from_name(const char *name)
{
    int i;
    char lower[96];
    size_t n;

    if (!name) {
        return KEY_PC_NONE;
    }

    n = strlen(name);
    if (n >= sizeof(lower)) {
        n = sizeof(lower) - 1;
    }

    /* lower-case */
    {
        size_t j;
        for (j = 0; j < n; ++j) {
            lower[j] = (char)tolower((unsigned char)name[j]);
        }
        lower[n] = '\0';
    }

    for (i = 0; i < (int)KEY_PC_COUNT; ++i) {
        if (strcmp(lower, key_names[i]) == 0) {
            return (key_pc_code)i;
        }
    }

    return KEY_PC_NONE;
}

void key_pc_init(key_pc_context *ctx)
{
    (void)ctx; /* placeholder */
}
