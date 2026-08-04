#include <stdlib.h>

#include <Carbon/Carbon.h>
#include <ApplicationServices/ApplicationServices.h>

#include "macpckeys_backend.h"

/* ============================================================
   key_pc_code -> CGKeyCode (kVK_*)

   Nota:
   - CGEventSourceKeyState funciona con CGKeyCode.
   - Algunos keys (audio/media, etc.) no están cubiertos aquí.
     Para esos devolvemos -1.
   ============================================================ */

static int key_pc_to_cgkey(key_pc_code key)
{
    switch (key) {
    case KEY_PC_NONE:
    case KEY_PC_ERROR_ROLLOVER:
    case KEY_PC_POST_FAIL:
    case KEY_PC_ERROR_UNDEFINED:
        return -1;

    /* Letras */
    case KEY_PC_A: return kVK_ANSI_A;
    case KEY_PC_B: return kVK_ANSI_B;
    case KEY_PC_C: return kVK_ANSI_C;
    case KEY_PC_D: return kVK_ANSI_D;
    case KEY_PC_E: return kVK_ANSI_E;
    case KEY_PC_F: return kVK_ANSI_F;
    case KEY_PC_G: return kVK_ANSI_G;
    case KEY_PC_H: return kVK_ANSI_H;
    case KEY_PC_I: return kVK_ANSI_I;
    case KEY_PC_J: return kVK_ANSI_J;
    case KEY_PC_K: return kVK_ANSI_K;
    case KEY_PC_L: return kVK_ANSI_L;
    case KEY_PC_M: return kVK_ANSI_M;
    case KEY_PC_N: return kVK_ANSI_N;
    case KEY_PC_O: return kVK_ANSI_O;
    case KEY_PC_P: return kVK_ANSI_P;
    case KEY_PC_Q: return kVK_ANSI_Q;
    case KEY_PC_R: return kVK_ANSI_R;
    case KEY_PC_S: return kVK_ANSI_S;
    case KEY_PC_T: return kVK_ANSI_T;
    case KEY_PC_U: return kVK_ANSI_U;
    case KEY_PC_V: return kVK_ANSI_V;
    case KEY_PC_W: return kVK_ANSI_W;
    case KEY_PC_X: return kVK_ANSI_X;
    case KEY_PC_Y: return kVK_ANSI_Y;
    case KEY_PC_Z: return kVK_ANSI_Z;

    /* Números */
    case KEY_PC_0: return kVK_ANSI_0;
    case KEY_PC_1: return kVK_ANSI_1;
    case KEY_PC_2: return kVK_ANSI_2;
    case KEY_PC_3: return kVK_ANSI_3;
    case KEY_PC_4: return kVK_ANSI_4;
    case KEY_PC_5: return kVK_ANSI_5;
    case KEY_PC_6: return kVK_ANSI_6;
    case KEY_PC_7: return kVK_ANSI_7;
    case KEY_PC_8: return kVK_ANSI_8;
    case KEY_PC_9: return kVK_ANSI_9;

    /* Principal */
    case KEY_PC_ENTER:     return kVK_Return;
    case KEY_PC_ESCAPE:    return kVK_Escape;
    case KEY_PC_BACKSPACE: return kVK_Delete;
    case KEY_PC_TAB:       return kVK_Tab;
    case KEY_PC_SPACE:     return kVK_Space;
    case KEY_PC_MINUS:     return kVK_ANSI_Minus;
    case KEY_PC_EQUAL:     return kVK_ANSI_Equal;
    case KEY_PC_LBRACKET:  return kVK_ANSI_LeftBracket;
    case KEY_PC_RBRACKET:  return kVK_ANSI_RightBracket;
    case KEY_PC_BACKSLASH: return kVK_ANSI_Backslash;
    case KEY_PC_SEMICOLON: return kVK_ANSI_Semicolon;
    case KEY_PC_APOSTROPHE:return kVK_ANSI_Quote;
    case KEY_PC_GRAVE:     return kVK_ANSI_Grave;
    case KEY_PC_COMMA:     return kVK_ANSI_Comma;
    case KEY_PC_DOT:       return kVK_ANSI_Period;
    case KEY_PC_SLASH:     return kVK_ANSI_Slash;
    case KEY_PC_CAPSLOCK:  return kVK_CapsLock;

    /* Funciones */
    case KEY_PC_F1:  return kVK_F1;
    case KEY_PC_F2:  return kVK_F2;
    case KEY_PC_F3:  return kVK_F3;
    case KEY_PC_F4:  return kVK_F4;
    case KEY_PC_F5:  return kVK_F5;
    case KEY_PC_F6:  return kVK_F6;
    case KEY_PC_F7:  return kVK_F7;
    case KEY_PC_F8:  return kVK_F8;
    case KEY_PC_F9:  return kVK_F9;
    case KEY_PC_F10: return kVK_F10;
    case KEY_PC_F11: return kVK_F11;
    case KEY_PC_F12: return kVK_F12;
    case KEY_PC_F13: return kVK_F13;
    case KEY_PC_F14: return kVK_F14;
    case KEY_PC_F15: return kVK_F15;
    case KEY_PC_F16: return kVK_F16;
    case KEY_PC_F17: return kVK_F17;
    case KEY_PC_F18: return kVK_F18;
    case KEY_PC_F19: return kVK_F19;
    case KEY_PC_F20: return kVK_F20;

    /* Navegación */
    case KEY_PC_HOME:      return kVK_Home;
    case KEY_PC_PAGE_UP:   return kVK_PageUp;
    case KEY_PC_DELETE:    return kVK_ForwardDelete;
    case KEY_PC_END:       return kVK_End;
    case KEY_PC_PAGE_DOWN: return kVK_PageDown;
    case KEY_PC_RIGHT:     return kVK_RightArrow;
    case KEY_PC_LEFT:      return kVK_LeftArrow;
    case KEY_PC_DOWN:      return kVK_DownArrow;
    case KEY_PC_UP:        return kVK_UpArrow;
    case KEY_PC_INSERT:    return kVK_Help; /* Insert no existe como tal */

    /* Keypad */
    case KEY_PC_NUM_LOCK:  return kVK_ANSI_KeypadClear; /* aproximación */
    case KEY_PC_KP_DIV:    return kVK_ANSI_KeypadDivide;
    case KEY_PC_KP_MUL:    return kVK_ANSI_KeypadMultiply;
    case KEY_PC_KP_SUB:    return kVK_ANSI_KeypadMinus;
    case KEY_PC_KP_ADD:    return kVK_ANSI_KeypadPlus;
    case KEY_PC_KP_ENTER:  return kVK_ANSI_KeypadEnter;
    case KEY_PC_KP_0:      return kVK_ANSI_Keypad0;
    case KEY_PC_KP_1:      return kVK_ANSI_Keypad1;
    case KEY_PC_KP_2:      return kVK_ANSI_Keypad2;
    case KEY_PC_KP_3:      return kVK_ANSI_Keypad3;
    case KEY_PC_KP_4:      return kVK_ANSI_Keypad4;
    case KEY_PC_KP_5:      return kVK_ANSI_Keypad5;
    case KEY_PC_KP_6:      return kVK_ANSI_Keypad6;
    case KEY_PC_KP_7:      return kVK_ANSI_Keypad7;
    case KEY_PC_KP_8:      return kVK_ANSI_Keypad8;
    case KEY_PC_KP_9:      return kVK_ANSI_Keypad9;
    case KEY_PC_KP_DOT:    return kVK_ANSI_KeypadDecimal;
    case KEY_PC_KP_EQUAL:  return kVK_ANSI_KeypadEquals;

    /* Modificadores */
    case KEY_PC_LCTRL:  return kVK_Control;
    case KEY_PC_LSHIFT: return kVK_Shift;
    case KEY_PC_LALT:   return kVK_Option;
    case KEY_PC_LGUI:   return kVK_Command;
    case KEY_PC_RCTRL:  return kVK_RightControl;
    case KEY_PC_RSHIFT: return kVK_RightShift;
    case KEY_PC_RALT:   return kVK_RightOption;
    case KEY_PC_RGUI:   return kVK_RightCommand;

    default:
        return -1;
    }
}

static int ensure_capacity(macpckeys_backend *kb, int required)
{
    key_pc_code *new_ptr;
    int new_cap;
    int i;

    if (!kb) return -1;
    if (required <= 0) return 0;

    if (kb->button_keys && kb->button_capacity >= required) {
        return 0;
    }

    new_cap = kb->button_capacity;
    if (new_cap < 8) new_cap = 8;
    while (new_cap < required) {
        new_cap *= 2;
        if (new_cap < 0) return -1;
    }

    new_ptr = (key_pc_code*)realloc(kb->button_keys, (size_t)new_cap * sizeof(key_pc_code));
    if (!new_ptr) return -1;

    for (i = kb->button_capacity; i < new_cap; ++i) {
        new_ptr[i] = KEY_PC_NONE;
    }

    kb->button_keys = new_ptr;
    kb->button_capacity = new_cap;
    return 0;
}

static int macpckeys_button_state(void *user_data, int button_index)
{
    macpckeys_backend *kb = (macpckeys_backend*)user_data;
    key_pc_code key;
    int code;

    if (!kb) return 0;
    if (button_index < 0 || button_index >= kb->button_capacity) return 0;
    if (!kb->button_keys) return 0;

    key = kb->button_keys[button_index];
    if (key == KEY_PC_NONE) return 0;

    code = key_pc_to_cgkey(key);
    if (code < 0) return 0;

    return CGEventSourceKeyState(kCGEventSourceStateCombinedSessionState, (CGKeyCode)code) ? 1 : 0;
}

void macpckeys_backend_init(macpckeys_backend *kb)
{
    if (!kb) return;

    key_pc_init(&kb->key_ctx);

    kb->button_keys = NULL;
    kb->button_capacity = 0;

    kb->scanner.scanner = NULL;
    kb->scanner.max_buttons = 0;
    kb->scanner.connect = NULL;
    kb->scanner.update = NULL;
    kb->scanner.hold = NULL;
    kb->scanner.pressed = NULL;
    kb->scanner.released = NULL;
    kb->scanner_attached = 0;
}

void macpckeys_backend_shutdown(macpckeys_backend *kb)
{
    if (!kb) return;

    if (kb->button_keys) {
        free(kb->button_keys);
        kb->button_keys = NULL;
    }
    kb->button_capacity = 0;

    kb->scanner_attached = 0;
}

int macpckeys_backend_attach_scanner(macpckeys_backend *kb,
                                   const polls_scanner_iface *scanner_iface)
{
    int rc;

    if (!kb || !scanner_iface) return -1;
    if (!scanner_iface->scanner) return -1;
    if (!scanner_iface->connect || !scanner_iface->update) return -1;
    if (!scanner_iface->hold || !scanner_iface->pressed || !scanner_iface->released) return -1;

    if (scanner_iface->max_buttons > 0) {
        if (ensure_capacity(kb, scanner_iface->max_buttons) != 0) {
            return -1;
        }
    }

    kb->scanner = *scanner_iface;

    rc = kb->scanner.connect(kb->scanner.scanner, macpckeys_button_state, (void*)kb);
    if (rc != 0) {
        kb->scanner_attached = 0;
        return -1;
    }

    kb->scanner_attached = 1;
    return 0;
}

int macpckeys_bind_button(macpckeys_backend *kb, int button_index, key_pc_code key)
{
    if (!kb) return -1;
    if (button_index < 0) return -1;

    if (ensure_capacity(kb, button_index + 1) != 0) {
        return -1;
    }

    kb->button_keys[button_index] = key;
    return 0;
}

void macpckeys_backend_update(macpckeys_backend *kb)
{
    if (!kb) return;
    if (!kb->scanner_attached) return;
    if (!kb->scanner.update) return;

    kb->scanner.update(kb->scanner.scanner);
}

int macpckeys_button_hold(const macpckeys_backend *kb, int button_index)
{
    if (!kb) return 0;
    if (!kb->scanner_attached) return 0;
    if (!kb->scanner.hold) return 0;
    return kb->scanner.hold(kb->scanner.scanner, button_index);
}

int macpckeys_button_pressed(const macpckeys_backend *kb, int button_index)
{
    if (!kb) return 0;
    if (!kb->scanner_attached) return 0;
    if (!kb->scanner.pressed) return 0;
    return kb->scanner.pressed(kb->scanner.scanner, button_index);
}

int macpckeys_button_released(const macpckeys_backend *kb, int button_index)
{
    if (!kb) return 0;
    if (!kb->scanner_attached) return 0;
    if (!kb->scanner.released) return 0;
    return kb->scanner.released(kb->scanner.scanner, button_index);
}
