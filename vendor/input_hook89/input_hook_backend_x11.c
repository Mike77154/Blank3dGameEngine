#ifdef __unix__

#include "input_hook_backend_x11.h"
#include "ihk_kbmap_linux_evdev.h"

#include <string.h>

#include <X11/Xlib.h>

/* Bit helpers */
static void ihk_set_usage_bit(ihk_u8 *bits, ihk_u8 usage)
{
    bits[(unsigned int)usage >> 3] = (ihk_u8)(bits[(unsigned int)usage >> 3] | (ihk_u8)(1u << ((unsigned int)usage & 7u)));
}

static int ihk_x11_key_is_down(const char keymap[32], int keycode)
{
    /* keycode is 8..255 */
    int idx = keycode >> 3;
    int bit = keycode & 7;
    return (keymap[idx] & (1 << bit)) != 0;
}

static void ihk_x11_poll_keyboard(void *user, ihk_u8 *out_kb_bits, size_t out_bytes)
{
    ihk_x11_backend *b;
    char km[32];
    int kc;

    b = (ihk_x11_backend*)user;
    if (out_bytes > 0) memset(out_kb_bits, 0, out_bytes);
    if (!b || !b->dpy) return;

    XQueryKeymap((Display*)b->dpy, km);

    /* Iterate over all possible X11 keycodes (8..255) */
    for (kc = 8; kc <= 255; ++kc) {
        if (ihk_x11_key_is_down(km, kc)) {
            ihk_u8 usage = 0;

            if (b->use_evdev) {
                int linux_code = kc - b->evdev_offset;
                if (linux_code >= 0) {
                    usage = ihk_kb_usage_from_linux_keycode((unsigned int)linux_code);
                }
            }

            /* If we couldn't map, we just ignore it (keeps HID semantics clean). */
            if (usage) {
                ihk_set_usage_bit(out_kb_bits, usage);
            }
        }
    }
}

static void ihk_x11_shutdown(void *user)
{
    ihk_x11_backend *b = (ihk_x11_backend*)user;
    if (!b) return;
    if (b->owns_display && b->dpy) {
        XCloseDisplay((Display*)b->dpy);
    }
    b->dpy = 0;
    b->owns_display = 0;
}

int ihk_x11_backend_init(ihk_x11_backend *b, Display *dpy)
{
    if (!b) return 0;
    memset(b, 0, sizeof(*b));

    if (dpy) {
        b->dpy = dpy;
        b->owns_display = 0;
    } else {
        b->dpy = XOpenDisplay(0);
        b->owns_display = (b->dpy != 0) ? 1 : 0;
    }

    b->use_evdev = 1;
    b->evdev_offset = 8;

    return b->dpy != 0;
}

void ihk_x11_backend_set_use_evdev(ihk_x11_backend *b, int enable)
{
    if (!b) return;
    b->use_evdev = enable ? 1 : 0;
}

void ihk_x11_backend_set_evdev_offset(ihk_x11_backend *b, int offset)
{
    if (!b) return;
    b->evdev_offset = offset;
}

void ihk_x11_make_backend(ihk_x11_backend *b, ihk_backend *out)
{
    if (!out) return;
    out->user = b;
    out->poll_keyboard = ihk_x11_poll_keyboard;
    out->shutdown = ihk_x11_shutdown;

    /* XQueryKeymap gives server-wide state (effectively global within X11),
       and evdev mapping is layout-independent physical-ish.
    */
    out->capabilities = IHK_CAP_KEYBOARD | IHK_CAP_GLOBAL_CAPTURE | IHK_CAP_LAYOUT_INDEPENDENT;
}

#endif /* __unix__ */
