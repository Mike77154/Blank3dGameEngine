/* key_pc.h - Keymap "PC keyboard" (inspirado en USB HID) - C89
 *
 * ⚠️ Nota importante:
 *   Esta lib (polls) ya NO depende hardcodeadamente de "input_scanner".
 *   key_pc solo define:
 *     - el enum de teclas (completo / extendible)
 *     - utilidades de nombre <-> código
 *
 * Los backends por sistema (win32/linux/mac, etc.) son los que traducen
 * key_pc_code -> código nativo del OS (VK, evdev, CGKeyCode, ...).
 */

#ifndef KEY_PC_HEADER_H
#define KEY_PC_HEADER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

/* ============================================================
   1. Lista de teclas (estilo USB HID / teclado PC)

   - snake_case en key_pc_name()
   - se mantiene la API vieja (key_pc_from_name / key_pc_name)
   - se amplió para cubrir prácticamente todas las teclas típicas
     del "Keyboard/Keypad" usage page.
   ============================================================ */

typedef enum {
    KEY_PC_NONE = 0, /* none */
    KEY_PC_ERROR_ROLLOVER = 1, /* error_rollover */
    KEY_PC_POST_FAIL = 2, /* post_fail */
    KEY_PC_ERROR_UNDEFINED = 3, /* error_undefined */
    KEY_PC_A = 4, /* a */
    KEY_PC_B = 5, /* b */
    KEY_PC_C = 6, /* c */
    KEY_PC_D = 7, /* d */
    KEY_PC_E = 8, /* e */
    KEY_PC_F = 9, /* f */
    KEY_PC_G = 10, /* g */
    KEY_PC_H = 11, /* h */
    KEY_PC_I = 12, /* i */
    KEY_PC_J = 13, /* j */
    KEY_PC_K = 14, /* k */
    KEY_PC_L = 15, /* l */
    KEY_PC_M = 16, /* m */
    KEY_PC_N = 17, /* n */
    KEY_PC_O = 18, /* o */
    KEY_PC_P = 19, /* p */
    KEY_PC_Q = 20, /* q */
    KEY_PC_R = 21, /* r */
    KEY_PC_S = 22, /* s */
    KEY_PC_T = 23, /* t */
    KEY_PC_U = 24, /* u */
    KEY_PC_V = 25, /* v */
    KEY_PC_W = 26, /* w */
    KEY_PC_X = 27, /* x */
    KEY_PC_Y = 28, /* y */
    KEY_PC_Z = 29, /* z */
    KEY_PC_0 = 30, /* 0 */
    KEY_PC_1 = 31, /* 1 */
    KEY_PC_2 = 32, /* 2 */
    KEY_PC_3 = 33, /* 3 */
    KEY_PC_4 = 34, /* 4 */
    KEY_PC_5 = 35, /* 5 */
    KEY_PC_6 = 36, /* 6 */
    KEY_PC_7 = 37, /* 7 */
    KEY_PC_8 = 38, /* 8 */
    KEY_PC_9 = 39, /* 9 */
    KEY_PC_ENTER = 40, /* enter */
    KEY_PC_ESCAPE = 41, /* escape */
    KEY_PC_BACKSPACE = 42, /* backspace */
    KEY_PC_TAB = 43, /* tab */
    KEY_PC_SPACE = 44, /* space */
    KEY_PC_MINUS = 45, /* minus */
    KEY_PC_EQUAL = 46, /* equal */
    KEY_PC_LBRACKET = 47, /* lbracket */
    KEY_PC_RBRACKET = 48, /* rbracket */
    KEY_PC_BACKSLASH = 49, /* backslash */
    KEY_PC_NONUS_HASH = 50, /* nonus_hash */
    KEY_PC_SEMICOLON = 51, /* semicolon */
    KEY_PC_APOSTROPHE = 52, /* apostrophe */
    KEY_PC_GRAVE = 53, /* grave */
    KEY_PC_COMMA = 54, /* comma */
    KEY_PC_DOT = 55, /* dot */
    KEY_PC_SLASH = 56, /* slash */
    KEY_PC_CAPSLOCK = 57, /* capslock */
    KEY_PC_F1 = 58, /* f1 */
    KEY_PC_F2 = 59, /* f2 */
    KEY_PC_F3 = 60, /* f3 */
    KEY_PC_F4 = 61, /* f4 */
    KEY_PC_F5 = 62, /* f5 */
    KEY_PC_F6 = 63, /* f6 */
    KEY_PC_F7 = 64, /* f7 */
    KEY_PC_F8 = 65, /* f8 */
    KEY_PC_F9 = 66, /* f9 */
    KEY_PC_F10 = 67, /* f10 */
    KEY_PC_F11 = 68, /* f11 */
    KEY_PC_F12 = 69, /* f12 */
    KEY_PC_F13 = 70, /* f13 */
    KEY_PC_F14 = 71, /* f14 */
    KEY_PC_F15 = 72, /* f15 */
    KEY_PC_F16 = 73, /* f16 */
    KEY_PC_F17 = 74, /* f17 */
    KEY_PC_F18 = 75, /* f18 */
    KEY_PC_F19 = 76, /* f19 */
    KEY_PC_F20 = 77, /* f20 */
    KEY_PC_F21 = 78, /* f21 */
    KEY_PC_F22 = 79, /* f22 */
    KEY_PC_F23 = 80, /* f23 */
    KEY_PC_F24 = 81, /* f24 */
    KEY_PC_PRINT_SCREEN = 82, /* print_screen */
    KEY_PC_SCROLL_LOCK = 83, /* scroll_lock */
    KEY_PC_PAUSE = 84, /* pause */
    KEY_PC_INSERT = 85, /* insert */
    KEY_PC_HOME = 86, /* home */
    KEY_PC_PAGE_UP = 87, /* page_up */
    KEY_PC_DELETE = 88, /* delete */
    KEY_PC_END = 89, /* end */
    KEY_PC_PAGE_DOWN = 90, /* page_down */
    KEY_PC_RIGHT = 91, /* right */
    KEY_PC_LEFT = 92, /* left */
    KEY_PC_DOWN = 93, /* down */
    KEY_PC_UP = 94, /* up */
    KEY_PC_NUM_LOCK = 95, /* num_lock */
    KEY_PC_KP_DIV = 96, /* kp_div */
    KEY_PC_KP_MUL = 97, /* kp_mul */
    KEY_PC_KP_SUB = 98, /* kp_sub */
    KEY_PC_KP_ADD = 99, /* kp_add */
    KEY_PC_KP_ENTER = 100, /* kp_enter */
    KEY_PC_KP_1 = 101, /* kp_1 */
    KEY_PC_KP_2 = 102, /* kp_2 */
    KEY_PC_KP_3 = 103, /* kp_3 */
    KEY_PC_KP_4 = 104, /* kp_4 */
    KEY_PC_KP_5 = 105, /* kp_5 */
    KEY_PC_KP_6 = 106, /* kp_6 */
    KEY_PC_KP_7 = 107, /* kp_7 */
    KEY_PC_KP_8 = 108, /* kp_8 */
    KEY_PC_KP_9 = 109, /* kp_9 */
    KEY_PC_KP_0 = 110, /* kp_0 */
    KEY_PC_KP_DOT = 111, /* kp_dot */
    KEY_PC_NONUS_BACKSLASH = 112, /* nonus_backslash */
    KEY_PC_APPLICATION = 113, /* application */
    KEY_PC_POWER = 114, /* power */
    KEY_PC_KP_EQUAL = 115, /* kp_equal */
    KEY_PC_EXECUTE = 116, /* execute */
    KEY_PC_HELP = 117, /* help */
    KEY_PC_MENU = 118, /* menu */
    KEY_PC_SELECT = 119, /* select */
    KEY_PC_STOP = 120, /* stop */
    KEY_PC_AGAIN = 121, /* again */
    KEY_PC_UNDO = 122, /* undo */
    KEY_PC_CUT = 123, /* cut */
    KEY_PC_COPY = 124, /* copy */
    KEY_PC_PASTE = 125, /* paste */
    KEY_PC_FIND = 126, /* find */
    KEY_PC_MUTE = 127, /* mute */
    KEY_PC_VOLUME_UP = 128, /* volume_up */
    KEY_PC_VOLUME_DOWN = 129, /* volume_down */
    KEY_PC_LOCKING_CAPS_LOCK = 130, /* locking_caps_lock */
    KEY_PC_LOCKING_NUM_LOCK = 131, /* locking_num_lock */
    KEY_PC_LOCKING_SCROLL_LOCK = 132, /* locking_scroll_lock */
    KEY_PC_KP_COMMA = 133, /* kp_comma */
    KEY_PC_KP_EQUAL_AS400 = 134, /* kp_equal_as400 */
    KEY_PC_INTERNATIONAL1 = 135, /* international1 */
    KEY_PC_INTERNATIONAL2 = 136, /* international2 */
    KEY_PC_INTERNATIONAL3 = 137, /* international3 */
    KEY_PC_INTERNATIONAL4 = 138, /* international4 */
    KEY_PC_INTERNATIONAL5 = 139, /* international5 */
    KEY_PC_INTERNATIONAL6 = 140, /* international6 */
    KEY_PC_INTERNATIONAL7 = 141, /* international7 */
    KEY_PC_INTERNATIONAL8 = 142, /* international8 */
    KEY_PC_INTERNATIONAL9 = 143, /* international9 */
    KEY_PC_LANG1 = 144, /* lang1 */
    KEY_PC_LANG2 = 145, /* lang2 */
    KEY_PC_LANG3 = 146, /* lang3 */
    KEY_PC_LANG4 = 147, /* lang4 */
    KEY_PC_LANG5 = 148, /* lang5 */
    KEY_PC_LANG6 = 149, /* lang6 */
    KEY_PC_LANG7 = 150, /* lang7 */
    KEY_PC_LANG8 = 151, /* lang8 */
    KEY_PC_LANG9 = 152, /* lang9 */
    KEY_PC_ALTERNATE_ERASE = 153, /* alternate_erase */
    KEY_PC_SYSREQ = 154, /* sysreq */
    KEY_PC_CANCEL = 155, /* cancel */
    KEY_PC_CLEAR = 156, /* clear */
    KEY_PC_PRIOR = 157, /* prior */
    KEY_PC_RETURN = 158, /* return */
    KEY_PC_SEPARATOR = 159, /* separator */
    KEY_PC_OUT = 160, /* out */
    KEY_PC_OPER = 161, /* oper */
    KEY_PC_CLEAR_AGAIN = 162, /* clear_again */
    KEY_PC_CRSEL = 163, /* crsel */
    KEY_PC_EXSEL = 164, /* exsel */
    KEY_PC_KP_00 = 165, /* kp_00 */
    KEY_PC_KP_000 = 166, /* kp_000 */
    KEY_PC_THOUSANDS_SEPARATOR = 167, /* thousands_separator */
    KEY_PC_DECIMAL_SEPARATOR = 168, /* decimal_separator */
    KEY_PC_CURRENCY_UNIT = 169, /* currency_unit */
    KEY_PC_CURRENCY_SUBUNIT = 170, /* currency_subunit */
    KEY_PC_KP_LPAREN = 171, /* kp_lparen */
    KEY_PC_KP_RPAREN = 172, /* kp_rparen */
    KEY_PC_KP_LBRACE = 173, /* kp_lbrace */
    KEY_PC_KP_RBRACE = 174, /* kp_rbrace */
    KEY_PC_KP_TAB = 175, /* kp_tab */
    KEY_PC_KP_BACKSPACE = 176, /* kp_backspace */
    KEY_PC_KP_A = 177, /* kp_a */
    KEY_PC_KP_B = 178, /* kp_b */
    KEY_PC_KP_C = 179, /* kp_c */
    KEY_PC_KP_D = 180, /* kp_d */
    KEY_PC_KP_E = 181, /* kp_e */
    KEY_PC_KP_F = 182, /* kp_f */
    KEY_PC_KP_XOR = 183, /* kp_xor */
    KEY_PC_KP_CARET = 184, /* kp_caret */
    KEY_PC_KP_PERCENT = 185, /* kp_percent */
    KEY_PC_KP_LESS = 186, /* kp_less */
    KEY_PC_KP_GREATER = 187, /* kp_greater */
    KEY_PC_KP_AMPERSAND = 188, /* kp_ampersand */
    KEY_PC_KP_DBLAMPERSAND = 189, /* kp_dblampersand */
    KEY_PC_KP_VERTICAL_BAR = 190, /* kp_vertical_bar */
    KEY_PC_KP_DBLVERTICAL_BAR = 191, /* kp_dblvertical_bar */
    KEY_PC_KP_COLON = 192, /* kp_colon */
    KEY_PC_KP_HASH = 193, /* kp_hash */
    KEY_PC_KP_SPACE = 194, /* kp_space */
    KEY_PC_KP_AT = 195, /* kp_at */
    KEY_PC_KP_EXCLAMATION = 196, /* kp_exclamation */
    KEY_PC_KP_MEM_STORE = 197, /* kp_mem_store */
    KEY_PC_KP_MEM_RECALL = 198, /* kp_mem_recall */
    KEY_PC_KP_MEM_CLEAR = 199, /* kp_mem_clear */
    KEY_PC_KP_MEM_ADD = 200, /* kp_mem_add */
    KEY_PC_KP_MEM_SUB = 201, /* kp_mem_sub */
    KEY_PC_KP_MEM_MUL = 202, /* kp_mem_mul */
    KEY_PC_KP_MEM_DIV = 203, /* kp_mem_div */
    KEY_PC_KP_PLUS_MINUS = 204, /* kp_plus_minus */
    KEY_PC_KP_CLEAR = 205, /* kp_clear */
    KEY_PC_KP_CLEAR_ENTRY = 206, /* kp_clear_entry */
    KEY_PC_KP_BINARY = 207, /* kp_binary */
    KEY_PC_KP_OCTAL = 208, /* kp_octal */
    KEY_PC_KP_DECIMAL = 209, /* kp_decimal */
    KEY_PC_KP_HEXADECIMAL = 210, /* kp_hexadecimal */
    KEY_PC_LCTRL = 211, /* lctrl */
    KEY_PC_LSHIFT = 212, /* lshift */
    KEY_PC_LALT = 213, /* lalt */
    KEY_PC_LGUI = 214, /* lgui */
    KEY_PC_RCTRL = 215, /* rctrl */
    KEY_PC_RSHIFT = 216, /* rshift */
    KEY_PC_RALT = 217, /* ralt */
    KEY_PC_RGUI = 218, /* rgui */

    KEY_PC_COUNT = 219
} key_pc_code;

/* ============================================================
   2. Contexto (por si un backend quiere guardar cosas aquí)
   ============================================================ */

typedef struct {
    int dummy; /* placeholder */
} key_pc_context;

/* ============================================================
   3. Funciones públicas
   ============================================================ */

void key_pc_init(key_pc_context *ctx);

/* Traducción de nombre lógico (snake_case) -> key_pc_code */
key_pc_code key_pc_from_name(const char *name);

/* Traducción de key_pc_code -> nombre (snake_case) */
const char *key_pc_name(key_pc_code k);

#ifdef __cplusplus
}
#endif

#endif /* KEY_PC_HEADER_H */
