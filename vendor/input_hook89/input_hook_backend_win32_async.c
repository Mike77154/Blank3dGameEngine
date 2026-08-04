#ifdef _WIN32

#include "input_hook_backend_win32_async.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

/* Map keyboard-page HID usage (0..255) -> Win32 virtual-key (VK_*)
   Return 0 if not supported.
*/
static int ihk_win_vk_from_kb_usage(ihk_u8 u)
{
    /* Letters */
    if (u >= 0x04 && u <= 0x1D) {
        return 'A' + (int)(u - 0x04);
    }

    /* Digits top row: 1..9,0 */
    if (u >= 0x1E && u <= 0x26) {
        return '1' + (int)(u - 0x1E);
    }
    if (u == 0x27) {
        return '0';
    }

    /* Core control keys */
    switch (u) {
    case 0x28: return VK_RETURN;   /* Enter */
    case 0x29: return VK_ESCAPE;   /* Esc */
    case 0x2A: return VK_BACK;     /* Backspace */
    case 0x2B: return VK_TAB;      /* Tab */
    case 0x2C: return VK_SPACE;    /* Space */
    }

    /* Punctuation (US keyboard) */
    switch (u) {
    case 0x2D: return VK_OEM_MINUS;   /* - */
    case 0x2E: return VK_OEM_PLUS;    /* = */
    case 0x2F: return VK_OEM_4;       /* [ */
    case 0x30: return VK_OEM_6;       /* ] */
    case 0x31: return VK_OEM_5;       /* \\ */
    case 0x32: return VK_OEM_102;     /* non-us # and ~ (often) */
    case 0x33: return VK_OEM_1;       /* ; */
    case 0x34: return VK_OEM_7;       /* ' */
    case 0x35: return VK_OEM_3;       /* ` */
    case 0x36: return VK_OEM_COMMA;   /* , */
    case 0x37: return VK_OEM_PERIOD;  /* . */
    case 0x38: return VK_OEM_2;       /* / */
    }

    /* Locks */
    switch (u) {
    case 0x39: return VK_CAPITAL;  /* Caps Lock */
    case 0x47: return VK_SCROLL;   /* Scroll Lock */
    case 0x53: return VK_NUMLOCK;  /* Num Lock */
    }

    /* Function keys */
    if (u >= 0x3A && u <= 0x45) {
        return VK_F1 + (int)(u - 0x3A);
    }

    /* Navigation */
    switch (u) {
    case 0x49: return VK_INSERT;
    case 0x4A: return VK_HOME;
    case 0x4B: return VK_PRIOR; /* PageUp */
    case 0x4C: return VK_DELETE;
    case 0x4D: return VK_END;
    case 0x4E: return VK_NEXT;  /* PageDown */

    case 0x4F: return VK_RIGHT;
    case 0x50: return VK_LEFT;
    case 0x51: return VK_DOWN;
    case 0x52: return VK_UP;
    }

    /* System / misc */
    switch (u) {
    case 0x46: return VK_SNAPSHOT; /* Print Screen */
    case 0x48: return VK_PAUSE;    /* Pause */
    case 0x65: return VK_APPS;     /* Application/Menu */
    }

    /* Keypad */
    switch (u) {
    case 0x54: return VK_DIVIDE;
    case 0x55: return VK_MULTIPLY;
    case 0x56: return VK_SUBTRACT;
    case 0x57: return VK_ADD;
    case 0x58: return VK_RETURN;   /* KP Enter (can't perfectly distinguish here) */

    case 0x59: return VK_NUMPAD1;
    case 0x5A: return VK_NUMPAD2;
    case 0x5B: return VK_NUMPAD3;
    case 0x5C: return VK_NUMPAD4;
    case 0x5D: return VK_NUMPAD5;
    case 0x5E: return VK_NUMPAD6;
    case 0x5F: return VK_NUMPAD7;
    case 0x60: return VK_NUMPAD8;
    case 0x61: return VK_NUMPAD9;
    case 0x62: return VK_NUMPAD0;
    case 0x63: return VK_DECIMAL;
    }

    /* Modifiers */
    switch (u) {
    case 0xE0: return VK_LCONTROL;
    case 0xE1: return VK_LSHIFT;
    case 0xE2: return VK_LMENU;    /* left alt */
    case 0xE3: return VK_LWIN;
    case 0xE4: return VK_RCONTROL;
    case 0xE5: return VK_RSHIFT;
    case 0xE6: return VK_RMENU;    /* right alt */
    case 0xE7: return VK_RWIN;
    }

    return 0;
}

static void ihk_win32_poll_keyboard(void *user, ihk_u8 *out_kb_bits, size_t out_bytes)
{
    ihk_u32 i;
    (void)user;

    if (!out_kb_bits || out_bytes < IHK_KB_BITS_BYTES) return;

    /* Clear */
    for (i = 0; i < IHK_KB_BITS_BYTES; ++i) {
        out_kb_bits[i] = 0;
    }

    /* Scan all 0..255 usages on keyboard page */
    for (i = 0; i < IHK_KB_USAGE_COUNT; ++i) {
        ihk_u8 usage = (ihk_u8)i;
        int vk = ihk_win_vk_from_kb_usage(usage);
        if (vk != 0) {
            SHORT s = GetAsyncKeyState(vk);
            if (s & 0x8000) {
                /* set bit */
                ihk_u8 byte = (ihk_u8)(usage >> 3);
                ihk_u8 mask = (ihk_u8)(1u << (usage & 7u));
                out_kb_bits[byte] = (ihk_u8)(out_kb_bits[byte] | mask);
            }
        }
    }
}

void ihk_win32_async_backend_init(ihk_win32_async_backend *b)
{
    if (!b) return;
    b->dummy = 0;
}

void ihk_win32_async_make_backend(ihk_win32_async_backend *b, ihk_backend *out)
{
    if (!out) return;

    out->user = (void*)b;
    out->poll_keyboard = ihk_win32_poll_keyboard;
    out->shutdown = 0;
    out->capabilities = IHK_CAP_KEYBOARD | IHK_CAP_GLOBAL_CAPTURE;
}

#endif /* _WIN32 */

#ifndef _WIN32
typedef int ihk_win32_async_backend_translation_unit_not_empty;
#endif
