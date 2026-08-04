/* input_hook_backend_x11.h

   X11 backend for input_hook (USB HID-style) on Linux/Unix desktops.

   Strategy:
     - Polls XQueryKeymap() (256 keycodes state bitmap)
     - By default assumes Xorg "evdev" keycode mapping: X11 keycode = Linux keycode + 8
       This is very common on modern Linux X11 stacks.
     - Converts Linux keycode -> HID usage using ihk_kbmap_linux_evdev.

   ✅ Can behave like "global capture" within X11 (not Wayland).
   ⚠️ Requires X11 (libX11).
   ⚠️ If you're on Wayland, this won't see global state (you'd need compositor APIs or evdev with permissions).

   Build:
     - Link with X11: -lX11
*/

#ifndef INPUT_HOOK_BACKEND_X11_H
#define INPUT_HOOK_BACKEND_X11_H

#ifdef __cplusplus
extern "C" {
#endif

#include "input_hook.h"

#ifdef __unix__

/* Forward-declare Display without pulling Xlib in the header */
typedef struct _XDisplay Display;

typedef struct ihk_x11_backend {
    Display *dpy;
    int owns_display;
    int use_evdev;     /* default 1 */
    int evdev_offset;  /* default 8 (X11 keycode = linux_code + 8) */
} ihk_x11_backend;

/* If dpy is NULL, opens default display and owns it. Returns 1 on success. */
int  ihk_x11_backend_init(ihk_x11_backend *b, Display *dpy);

/* Optional toggles */
void ihk_x11_backend_set_use_evdev(ihk_x11_backend *b, int enable);
void ihk_x11_backend_set_evdev_offset(ihk_x11_backend *b, int offset);

/* Fill an ihk_backend wrapper you can pass into input_hook_init/set_backend */
void ihk_x11_make_backend(ihk_x11_backend *b, ihk_backend *out);

#endif /* __unix__ */

#ifdef __cplusplus
}
#endif

#endif /* INPUT_HOOK_BACKEND_X11_H */
