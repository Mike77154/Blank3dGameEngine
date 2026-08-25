#include <windows.h>

#include "winpckeys_backend.h"
#include "polls_input_keys89.h"

/* ============================================================
   key_pc_code -> Windows VK_XXX

   Nota:
   - No todas las teclas del usage page tienen un VK directo.
   - Para las que no tienen equivalencia razonable, devolvemos 0.
   ============================================================ */

static int key_pc_to_vk(key_pc_code key)
{
    /* Letras A-Z */
    if (key >= KEY_PC_A && key <= KEY_PC_Z) {
        return (int)('A' + (key - KEY_PC_A));
    }

    /* Números fila superior 0-9 */
    if (key >= KEY_PC_0 && key <= KEY_PC_9) {
        return (int)('0' + (key - KEY_PC_0));
    }

    /* F1-F24 (Windows define VK_F1..VK_F24 consecutivos) */
    if (key >= KEY_PC_F1 && key <= KEY_PC_F24) {
        return (int)(VK_F1 + (key - KEY_PC_F1));
    }

    /* Keypad 1-9 */
    if (key >= KEY_PC_KP_1 && key <= KEY_PC_KP_9) {
        return (int)(VK_NUMPAD1 + (key - KEY_PC_KP_1));
    }

    switch (key) {
    case KEY_PC_NONE:
    case KEY_PC_ERROR_ROLLOVER:
    case KEY_PC_POST_FAIL:
    case KEY_PC_ERROR_UNDEFINED:
        return 0;

    /* Especiales */
    case KEY_PC_ENTER:      return VK_RETURN;
    case KEY_PC_ESCAPE:     return VK_ESCAPE;
    case KEY_PC_BACKSPACE:  return VK_BACK;
    case KEY_PC_TAB:        return VK_TAB;
    case KEY_PC_SPACE:      return VK_SPACE;
    case KEY_PC_CAPSLOCK:   return VK_CAPITAL;

    /* Puntuación / OEM (layout dependiente, pero típico US) */
    case KEY_PC_MINUS:      return VK_OEM_MINUS;
    case KEY_PC_EQUAL:      return VK_OEM_PLUS;
    case KEY_PC_LBRACKET:   return VK_OEM_4;
    case KEY_PC_RBRACKET:   return VK_OEM_6;
    case KEY_PC_BACKSLASH:  return VK_OEM_5;
    case KEY_PC_NONUS_HASH: return VK_OEM_8;
    case KEY_PC_SEMICOLON:  return VK_OEM_1;
    case KEY_PC_APOSTROPHE: return VK_OEM_7;
    case KEY_PC_GRAVE:      return VK_OEM_3;
    case KEY_PC_COMMA:      return VK_OEM_COMMA;
    case KEY_PC_DOT:        return VK_OEM_PERIOD;
    case KEY_PC_SLASH:      return VK_OEM_2;

    /* Navegación */
    case KEY_PC_PRINT_SCREEN: return VK_SNAPSHOT;
    case KEY_PC_SCROLL_LOCK:  return VK_SCROLL;
    case KEY_PC_PAUSE:        return VK_PAUSE;
    case KEY_PC_INSERT:       return VK_INSERT;
    case KEY_PC_HOME:         return VK_HOME;
    case KEY_PC_PAGE_UP:      return VK_PRIOR;
    case KEY_PC_DELETE:       return VK_DELETE;
    case KEY_PC_END:          return VK_END;
    case KEY_PC_PAGE_DOWN:    return VK_NEXT;
    case KEY_PC_RIGHT:        return VK_RIGHT;
    case KEY_PC_LEFT:         return VK_LEFT;
    case KEY_PC_DOWN:         return VK_DOWN;
    case KEY_PC_UP:           return VK_UP;

    /* Keypad */
    case KEY_PC_NUM_LOCK:   return VK_NUMLOCK;
    case KEY_PC_KP_DIV:     return VK_DIVIDE;
    case KEY_PC_KP_MUL:     return VK_MULTIPLY;
    case KEY_PC_KP_SUB:     return VK_SUBTRACT;
    case KEY_PC_KP_ADD:     return VK_ADD;
    case KEY_PC_KP_ENTER:   return VK_RETURN; /* no hay VK aparte */
    case KEY_PC_KP_0:       return VK_NUMPAD0;
    case KEY_PC_KP_DOT:     return VK_DECIMAL;
    case KEY_PC_KP_COMMA:   return VK_SEPARATOR;

    case KEY_PC_NONUS_BACKSLASH:
        /* ISO <>| */
        return VK_OEM_102;

    case KEY_PC_APPLICATION:
        return VK_APPS;
    case KEY_PC_POWER:
#ifdef VK_POWER
        return VK_POWER;
#else
        return 0;
#endif

    case KEY_PC_KP_EQUAL:
#ifdef VK_OEM_NEC_EQUAL
        return VK_OEM_NEC_EQUAL;
#else
        return 0;
#endif

    /* Teclas "editing" / "ui" */
    case KEY_PC_EXECUTE: return VK_EXECUTE;
    case KEY_PC_HELP:    return VK_HELP;
    case KEY_PC_MENU:    return VK_APPS;
    case KEY_PC_SELECT:  return VK_SELECT;

    /* USB HID Keyboard usages 0x78, 0x7A..0x7E (Stop, Undo, Cut,
       Copy, Paste, Find) do not have standard Win32 VK_* identities.
       Do not alias them to consumer/media keys or Ctrl chords here: this
       GetAsyncKeyState backend can only query one real virtual key. */
    case KEY_PC_STOP:
    case KEY_PC_UNDO:
    case KEY_PC_CUT:
    case KEY_PC_COPY:
    case KEY_PC_PASTE:
    case KEY_PC_FIND:
        return 0;

    /* Audio */
    case KEY_PC_MUTE:        return VK_VOLUME_MUTE;
    case KEY_PC_VOLUME_UP:   return VK_VOLUME_UP;
    case KEY_PC_VOLUME_DOWN: return VK_VOLUME_DOWN;

    /* Locking */
    case KEY_PC_LOCKING_CAPS_LOCK:   return VK_CAPITAL;
    case KEY_PC_LOCKING_NUM_LOCK:    return VK_NUMLOCK;
    case KEY_PC_LOCKING_SCROLL_LOCK: return VK_SCROLL;

    /* Sys */
    case KEY_PC_CANCEL: return VK_CANCEL;
    case KEY_PC_CLEAR:  return VK_CLEAR;
    case KEY_PC_PRIOR:  return VK_PRIOR;
    case KEY_PC_RETURN: return VK_RETURN;
    case KEY_PC_CRSEL:  return VK_CRSEL;
    case KEY_PC_EXSEL:  return VK_EXSEL;

    /* Modificadores */
    case KEY_PC_LCTRL:  return VK_LCONTROL;
    case KEY_PC_LSHIFT: return VK_LSHIFT;
    case KEY_PC_LALT:   return VK_LMENU;
    case KEY_PC_LGUI:   return VK_LWIN;
    case KEY_PC_RCTRL:  return VK_RCONTROL;
    case KEY_PC_RSHIFT: return VK_RSHIFT;
    case KEY_PC_RALT:   return VK_RMENU;
    case KEY_PC_RGUI:   return VK_RWIN;

    /* Todo lo demás (international, lang, keypad exótico, etc.) */
    default:
        return 0;
    }
}

/* ============================================================
   Util: asegurar capacidad de bindings
   ============================================================ */

static int ensure_capacity(winpckeys_backend *kb, int required)
{
    if (!kb) return -1;
    if (required < 0 || required > WINPCKEYS_MAX_BUTTONS) return -1;
    kb->button_capacity = WINPCKEYS_MAX_BUTTONS;
    return 0;
}

/* ============================================================
   polls_button_state_fn: botón lógico -> estado DOWN/UP
   ============================================================ */

static int winpckeys_button_state(void *user_data, int button_index)
{
    winpckeys_backend *kb = (winpckeys_backend*)user_data;
    key_pc_code key;
    int vk;

    if (!kb) return 0;
    if (button_index < 0 || button_index >= kb->button_capacity) return 0;
    key = kb->button_keys[button_index];
    if (key == KEY_PC_NONE) return 0;

    vk = key_pc_to_vk(key);
    if (vk == 0) return 0;

    return ( (GetAsyncKeyState(vk) & 0x8000) != 0 );
}

/* ============================================================
   API pública
   ============================================================ */

void winpckeys_backend_init(winpckeys_backend *kb)
{
    if (!kb) return;

    key_pc_init(&kb->key_ctx);

    {
        int i;
        for (i = 0; i < WINPCKEYS_MAX_BUTTONS; ++i)
            kb->button_keys[i] = KEY_PC_NONE;
    }
    kb->button_capacity = WINPCKEYS_MAX_BUTTONS;

    kb->scanner.scanner = NULL;
    kb->scanner.max_buttons = 0;
    kb->scanner.connect = NULL;
    kb->scanner.update = NULL;
    kb->scanner.hold = NULL;
    kb->scanner.pressed = NULL;
    kb->scanner.released = NULL;
    kb->scanner_attached = 0;
}

void winpckeys_backend_shutdown(winpckeys_backend *kb)
{
    if (!kb) return;

    kb->button_capacity = WINPCKEYS_MAX_BUTTONS;

    kb->scanner_attached = 0;
}

int winpckeys_backend_attach_scanner(winpckeys_backend *kb,
                                    const polls_scanner_iface *scanner_iface)
{
    int rc;

    if (!kb || !scanner_iface) return -1;
    if (!scanner_iface->scanner) return -1;
    if (!scanner_iface->connect || !scanner_iface->update) return -1;
    if (!scanner_iface->hold || !scanner_iface->pressed || !scanner_iface->released) return -1;

    /* Asegurar capacidad sugerida */
    if (scanner_iface->max_buttons > 0) {
        if (ensure_capacity(kb, scanner_iface->max_buttons) != 0) {
            return -1;
        }
    }

    /* Copiar interface */
    kb->scanner = *scanner_iface;

    rc = kb->scanner.connect(kb->scanner.scanner, winpckeys_button_state, (void*)kb);
    if (rc != 0) {
        kb->scanner_attached = 0;
        return -1;
    }

    kb->scanner_attached = 1;
    return 0;
}

int winpckeys_bind_button(winpckeys_backend *kb, int button_index, key_pc_code key)
{
    if (!kb) return -1;
    if (button_index < 0) return -1;

    if (ensure_capacity(kb, button_index + 1) != 0) {
        return -1;
    }

    kb->button_keys[button_index] = key;
    return 0;
}

void winpckeys_backend_update(winpckeys_backend *kb)
{
    if (!kb) return;
    if (!kb->scanner_attached) return;
    if (!kb->scanner.update) return;

    kb->scanner.update(kb->scanner.scanner);
}

int winpckeys_button_hold(const winpckeys_backend *kb, int button_index)
{
    if (!kb) return 0;
    if (!kb->scanner_attached) return 0;
    if (!kb->scanner.hold) return 0;
    return kb->scanner.hold(kb->scanner.scanner, button_index);
}

int winpckeys_button_pressed(const winpckeys_backend *kb, int button_index)
{
    if (!kb) return 0;
    if (!kb->scanner_attached) return 0;
    if (!kb->scanner.pressed) return 0;
    return kb->scanner.pressed(kb->scanner.scanner, button_index);
}

int winpckeys_button_released(const winpckeys_backend *kb, int button_index)
{
    if (!kb) return 0;
    if (!kb->scanner_attached) return 0;
    if (!kb->scanner.released) return 0;
    return kb->scanner.released(kb->scanner.scanner, button_index);
}

int winpckeys_input_key_down(const winpckeys_backend *kb, input_key89 key)
{
    key_pc_code legacy;
    int vk;
    (void)kb;
    legacy = polls_key_pc_from_input_key89(key);
    if (legacy == KEY_PC_NONE) return 0;
    vk = key_pc_to_vk(legacy);
    if (vk == 0) return 0;
    return ((GetAsyncKeyState(vk) & 0x8000) != 0) ? 1 : 0;
}

int winpckeys_bind_input_key89(winpckeys_backend *kb,
                               int button_index,
                               input_key89 key)
{
    key_pc_code legacy;
    legacy = polls_key_pc_from_input_key89(key);
    if (key != INPUT_KEY89_NONE && legacy == KEY_PC_NONE) return -1;
    return winpckeys_bind_button(kb, button_index, legacy);
}
