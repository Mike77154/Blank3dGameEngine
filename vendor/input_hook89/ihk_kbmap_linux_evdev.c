#include "ihk_kbmap_linux_evdev.h"

#ifdef __linux__

#include <linux/input.h>

/* Map linux input keycodes (KEY_*) to USB HID keyboard usages.
   Reference: USB HID Usage Tables (Keyboard/Keypad page 0x07)
*/
ihk_u8 ihk_kb_usage_from_linux_keycode(unsigned int code)
{
    /* Letters: KEY_A..KEY_Z are 30..55 on Linux */
    if (code >= KEY_A && code <= KEY_Z) {
        /* KEY_A=30 -> usage 0x04 */
        return (ihk_u8)(0x04u + (ihk_u8)(code - KEY_A));
    }

    /* Top-row digits: KEY_1..KEY_0 are 2..11 on Linux */
    if (code >= KEY_1 && code <= KEY_9) {
        return (ihk_u8)(0x1Eu + (ihk_u8)(code - KEY_1));
    }
    if (code == KEY_0) return 0x27u;

    /* Function keys */
    if (code >= KEY_F1 && code <= KEY_F12) {
        /* F1 usage 0x3A */
        return (ihk_u8)(0x3Au + (ihk_u8)(code - KEY_F1));
    }
    if (code >= KEY_F13 && code <= KEY_F24) {
        /* F13 usage 0x68 */
        return (ihk_u8)(0x68u + (ihk_u8)(code - KEY_F13));
    }

    switch (code) {
    /* Control cluster */
    case KEY_ENTER:      return 0x28u;
    case KEY_ESC:        return 0x29u;
    case KEY_BACKSPACE:  return 0x2Au;
    case KEY_TAB:        return 0x2Bu;
    case KEY_SPACE:      return 0x2Cu;

    /* Punctuation row (US layout) */
    case KEY_MINUS:      return 0x2Du;
    case KEY_EQUAL:      return 0x2Eu;
    case KEY_LEFTBRACE:  return 0x2Fu;
    case KEY_RIGHTBRACE: return 0x30u;
    case KEY_BACKSLASH:  return 0x31u;
    case KEY_SEMICOLON:  return 0x33u;
    case KEY_APOSTROPHE: return 0x34u;
    case KEY_GRAVE:      return 0x35u;
    case KEY_COMMA:      return 0x36u;
    case KEY_DOT:        return 0x37u;
    case KEY_SLASH:      return 0x38u;

    case KEY_CAPSLOCK:   return 0x39u;

    /* Print / Scroll / Pause */
    case KEY_SYSRQ:      return 0x46u; /* Print Screen */
    case KEY_SCROLLLOCK: return 0x47u;
    case KEY_PAUSE:      return 0x48u;

    /* Insert / Home / PgUp / Delete / End / PgDn */
    case KEY_INSERT:     return 0x49u;
    case KEY_HOME:       return 0x4Au;
    case KEY_PAGEUP:     return 0x4Bu;
    case KEY_DELETE:     return 0x4Cu;
    case KEY_END:        return 0x4Du;
    case KEY_PAGEDOWN:   return 0x4Eu;

    /* Arrows */
    case KEY_RIGHT:      return 0x4Fu;
    case KEY_LEFT:       return 0x50u;
    case KEY_DOWN:       return 0x51u;
    case KEY_UP:         return 0x52u;

    /* Keypad */
    case KEY_NUMLOCK:    return 0x53u;
    case KEY_KPSLASH:    return 0x54u;
    case KEY_KPASTERISK: return 0x55u;
    case KEY_KPMINUS:    return 0x56u;
    case KEY_KPPLUS:     return 0x57u;
    case KEY_KPENTER:    return 0x58u;
    case KEY_KP1:        return 0x59u;
    case KEY_KP2:        return 0x5Au;
    case KEY_KP3:        return 0x5Bu;
    case KEY_KP4:        return 0x5Cu;
    case KEY_KP5:        return 0x5Du;
    case KEY_KP6:        return 0x5Eu;
    case KEY_KP7:        return 0x5Fu;
    case KEY_KP8:        return 0x60u;
    case KEY_KP9:        return 0x61u;
    case KEY_KP0:        return 0x62u;
    case KEY_KPDOT:      return 0x63u;
    case KEY_102ND:      return 0x64u; /* Non-US \ and | */

    /* Application (Menu) */
    case KEY_MENU:       return 0x65u;

    /* Modifiers */
    case KEY_LEFTCTRL:   return 0xE0u;
    case KEY_LEFTSHIFT:  return 0xE1u;
    case KEY_LEFTALT:    return 0xE2u;
    case KEY_LEFTMETA:   return 0xE3u;
    case KEY_RIGHTCTRL:  return 0xE4u;
    case KEY_RIGHTSHIFT: return 0xE5u;
    case KEY_RIGHTALT:   return 0xE6u;
    case KEY_RIGHTMETA:  return 0xE7u;

    default:
        break;
    }

    return 0;
}

#else

ihk_u8 ihk_kb_usage_from_linux_keycode(unsigned int code)
{
    (void)code;
    return 0;
}

#endif /* __linux__ */
