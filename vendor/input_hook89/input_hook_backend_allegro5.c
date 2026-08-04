#include "input_hook_backend_allegro5.h"

#include <string.h>

#include <allegro5/allegro.h>
#include <allegro5/keycodes.h>

typedef struct ihk_al5_map {
    int al_key;
    ihk_u8 usage;
} ihk_al5_map;

/* Bit helpers */
static void ihk_set_usage_bit(ihk_u8 *bits, ihk_u8 usage)
{
    bits[(unsigned int)usage >> 3] = (ihk_u8)(bits[(unsigned int)usage >> 3] | (ihk_u8)(1u << ((unsigned int)usage & 7u)));
}

/* Mapping table: Allegro keycodes -> HID keyboard usages.
   Focused on a robust common set (letters, digits, controls, arrows, F-keys, keypad, modifiers).
   Some Allegro builds/platforms may omit certain keycodes; those are conditionally compiled.
*/
static const ihk_al5_map s_map[] = {
    /* Letters */
    { ALLEGRO_KEY_A, 0x04 }, { ALLEGRO_KEY_B, 0x05 }, { ALLEGRO_KEY_C, 0x06 }, { ALLEGRO_KEY_D, 0x07 },
    { ALLEGRO_KEY_E, 0x08 }, { ALLEGRO_KEY_F, 0x09 }, { ALLEGRO_KEY_G, 0x0A }, { ALLEGRO_KEY_H, 0x0B },
    { ALLEGRO_KEY_I, 0x0C }, { ALLEGRO_KEY_J, 0x0D }, { ALLEGRO_KEY_K, 0x0E }, { ALLEGRO_KEY_L, 0x0F },
    { ALLEGRO_KEY_M, 0x10 }, { ALLEGRO_KEY_N, 0x11 }, { ALLEGRO_KEY_O, 0x12 }, { ALLEGRO_KEY_P, 0x13 },
    { ALLEGRO_KEY_Q, 0x14 }, { ALLEGRO_KEY_R, 0x15 }, { ALLEGRO_KEY_S, 0x16 }, { ALLEGRO_KEY_T, 0x17 },
    { ALLEGRO_KEY_U, 0x18 }, { ALLEGRO_KEY_V, 0x19 }, { ALLEGRO_KEY_W, 0x1A }, { ALLEGRO_KEY_X, 0x1B },
    { ALLEGRO_KEY_Y, 0x1C }, { ALLEGRO_KEY_Z, 0x1D },

    /* Digits (top row) */
    { ALLEGRO_KEY_1, 0x1E }, { ALLEGRO_KEY_2, 0x1F }, { ALLEGRO_KEY_3, 0x20 }, { ALLEGRO_KEY_4, 0x21 },
    { ALLEGRO_KEY_5, 0x22 }, { ALLEGRO_KEY_6, 0x23 }, { ALLEGRO_KEY_7, 0x24 }, { ALLEGRO_KEY_8, 0x25 },
    { ALLEGRO_KEY_9, 0x26 }, { ALLEGRO_KEY_0, 0x27 },

    /* Core controls */
    { ALLEGRO_KEY_ENTER,     0x28 },
    { ALLEGRO_KEY_ESCAPE,    0x29 },
    { ALLEGRO_KEY_BACKSPACE, 0x2A },
    { ALLEGRO_KEY_TAB,       0x2B },
    { ALLEGRO_KEY_SPACE,     0x2C },

    /* Punctuation (US-ish) */
    { ALLEGRO_KEY_MINUS,      0x2D },
    { ALLEGRO_KEY_EQUALS,     0x2E },
    { ALLEGRO_KEY_OPENBRACE,  0x2F },
    { ALLEGRO_KEY_CLOSEBRACE, 0x30 },
    { ALLEGRO_KEY_BACKSLASH,  0x31 },
    { ALLEGRO_KEY_SEMICOLON,  0x33 },
    { ALLEGRO_KEY_QUOTE,      0x34 },
    { ALLEGRO_KEY_TILDE,      0x35 }, /* `~ */
    { ALLEGRO_KEY_COMMA,      0x36 },
    { ALLEGRO_KEY_FULLSTOP,   0x37 }, /* . */
    { ALLEGRO_KEY_SLASH,      0x38 },

    { ALLEGRO_KEY_CAPSLOCK,  0x39 },

    /* Function keys */
    { ALLEGRO_KEY_F1,  0x3A }, { ALLEGRO_KEY_F2,  0x3B }, { ALLEGRO_KEY_F3,  0x3C }, { ALLEGRO_KEY_F4,  0x3D },
    { ALLEGRO_KEY_F5,  0x3E }, { ALLEGRO_KEY_F6,  0x3F }, { ALLEGRO_KEY_F7,  0x40 }, { ALLEGRO_KEY_F8,  0x41 },
    { ALLEGRO_KEY_F9,  0x42 }, { ALLEGRO_KEY_F10, 0x43 }, { ALLEGRO_KEY_F11, 0x44 }, { ALLEGRO_KEY_F12, 0x45 },

#ifdef ALLEGRO_KEY_PRINTSCREEN
    { ALLEGRO_KEY_PRINTSCREEN, 0x46 },
#endif
#ifdef ALLEGRO_KEY_SCROLLLOCK
    { ALLEGRO_KEY_SCROLLLOCK,  0x47 },
#endif
#ifdef ALLEGRO_KEY_PAUSE
    { ALLEGRO_KEY_PAUSE,       0x48 },
#endif

    /* Nav cluster */
#ifdef ALLEGRO_KEY_INSERT
    { ALLEGRO_KEY_INSERT,   0x49 },
#endif
#ifdef ALLEGRO_KEY_HOME
    { ALLEGRO_KEY_HOME,     0x4A },
#endif
#ifdef ALLEGRO_KEY_PGUP
    { ALLEGRO_KEY_PGUP,     0x4B },
#endif
#ifdef ALLEGRO_KEY_DELETE
    { ALLEGRO_KEY_DELETE,   0x4C },
#endif
#ifdef ALLEGRO_KEY_END
    { ALLEGRO_KEY_END,      0x4D },
#endif
#ifdef ALLEGRO_KEY_PGDN
    { ALLEGRO_KEY_PGDN,     0x4E },
#endif

    /* Arrows */
    { ALLEGRO_KEY_RIGHT, 0x4F },
    { ALLEGRO_KEY_LEFT,  0x50 },
    { ALLEGRO_KEY_DOWN,  0x51 },
    { ALLEGRO_KEY_UP,    0x52 },

    /* Keypad */
#ifdef ALLEGRO_KEY_NUMLOCK
    { ALLEGRO_KEY_NUMLOCK,       0x53 },
#endif
#ifdef ALLEGRO_KEY_PAD_SLASH
    { ALLEGRO_KEY_PAD_SLASH,     0x54 },
#endif
#ifdef ALLEGRO_KEY_PAD_ASTERISK
    { ALLEGRO_KEY_PAD_ASTERISK,  0x55 },
#endif
#ifdef ALLEGRO_KEY_PAD_MINUS
    { ALLEGRO_KEY_PAD_MINUS,     0x56 },
#endif
#ifdef ALLEGRO_KEY_PAD_PLUS
    { ALLEGRO_KEY_PAD_PLUS,      0x57 },
#endif
#ifdef ALLEGRO_KEY_PAD_ENTER
    { ALLEGRO_KEY_PAD_ENTER,     0x58 },
#endif
#ifdef ALLEGRO_KEY_PAD_1
    { ALLEGRO_KEY_PAD_1,         0x59 },
#endif
#ifdef ALLEGRO_KEY_PAD_2
    { ALLEGRO_KEY_PAD_2,         0x5A },
#endif
#ifdef ALLEGRO_KEY_PAD_3
    { ALLEGRO_KEY_PAD_3,         0x5B },
#endif
#ifdef ALLEGRO_KEY_PAD_4
    { ALLEGRO_KEY_PAD_4,         0x5C },
#endif
#ifdef ALLEGRO_KEY_PAD_5
    { ALLEGRO_KEY_PAD_5,         0x5D },
#endif
#ifdef ALLEGRO_KEY_PAD_6
    { ALLEGRO_KEY_PAD_6,         0x5E },
#endif
#ifdef ALLEGRO_KEY_PAD_7
    { ALLEGRO_KEY_PAD_7,         0x5F },
#endif
#ifdef ALLEGRO_KEY_PAD_8
    { ALLEGRO_KEY_PAD_8,         0x60 },
#endif
#ifdef ALLEGRO_KEY_PAD_9
    { ALLEGRO_KEY_PAD_9,         0x61 },
#endif
#ifdef ALLEGRO_KEY_PAD_0
    { ALLEGRO_KEY_PAD_0,         0x62 },
#endif
#ifdef ALLEGRO_KEY_PAD_DELETE
    { ALLEGRO_KEY_PAD_DELETE,    0x63 }, /* KP dot */
#endif

#ifdef ALLEGRO_KEY_BACKSLASH2
    { ALLEGRO_KEY_BACKSLASH2,    0x64 }, /* Non-US \ | */
#endif

#ifdef ALLEGRO_KEY_MENU
    { ALLEGRO_KEY_MENU,          0x65 },
#endif

    /* Modifiers */
#ifdef ALLEGRO_KEY_LCTRL
    { ALLEGRO_KEY_LCTRL,   0xE0 },
#endif
#ifdef ALLEGRO_KEY_LSHIFT
    { ALLEGRO_KEY_LSHIFT,  0xE1 },
#endif
#ifdef ALLEGRO_KEY_ALT
    { ALLEGRO_KEY_ALT,     0xE2 }, /* Generic ALT; maps to LALT usage */
#endif
#ifdef ALLEGRO_KEY_LWIN
    { ALLEGRO_KEY_LWIN,    0xE3 },
#endif
#ifdef ALLEGRO_KEY_RCTRL
    { ALLEGRO_KEY_RCTRL,   0xE4 },
#endif
#ifdef ALLEGRO_KEY_RSHIFT
    { ALLEGRO_KEY_RSHIFT,  0xE5 },
#endif
#ifdef ALLEGRO_KEY_ALTGR
    { ALLEGRO_KEY_ALTGR,   0xE6 },
#endif
#ifdef ALLEGRO_KEY_RWIN
    { ALLEGRO_KEY_RWIN,    0xE7 }
#endif
};

static void ihk_allegro5_poll_keyboard(void *user, ihk_u8 *out_kb_bits, size_t out_bytes)
{
    ihk_allegro5_backend *b;
    ALLEGRO_KEYBOARD_STATE st;
    unsigned int i;
    unsigned int count;

    b = (ihk_allegro5_backend*)user;

    /* Clear */
    if (out_bytes > 0) memset(out_kb_bits, 0, out_bytes);

    if (!al_is_keyboard_installed()) {
        if (b && b->auto_install) {
            if (!al_install_keyboard()) {
                return;
            }
        } else {
            return;
        }
    }

    al_get_keyboard_state(&st);

    count = (unsigned int)(sizeof(s_map) / sizeof(s_map[0]));
    for (i = 0; i < count; ++i) {
        if (al_key_down(&st, s_map[i].al_key)) {
            ihk_set_usage_bit(out_kb_bits, s_map[i].usage);
        }
    }
}

void ihk_allegro5_backend_init(ihk_allegro5_backend *b)
{
    if (!b) return;
    b->auto_install = 0;
}

void ihk_allegro5_backend_set_auto_install(ihk_allegro5_backend *b, int enable)
{
    if (!b) return;
    b->auto_install = enable ? 1 : 0;
}

void ihk_allegro5_make_backend(ihk_allegro5_backend *b, ihk_backend *out)
{
    if (!out) return;
    out->user = b;
    out->poll_keyboard = ihk_allegro5_poll_keyboard;
    out->shutdown = 0;
    /* Allegro keycodes are not guaranteed to be layout-independent across OS, so we don't advertise it. */
    out->capabilities = IHK_CAP_KEYBOARD;
}
