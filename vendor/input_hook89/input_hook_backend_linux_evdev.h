/* input_hook_backend_linux_evdev.h

   Linux "evdev" backend for input_hook (USB HID-style).

   Reads /dev/input/event* and tracks key state from EV_KEY events.

   ✅ Can be truly global (works on Wayland too) IF your process has permission to read the device.
   ✅ Layout-independent (physical-ish) because Linux keycodes are scancode-ish.
   ⚠️ Requires permissions:
        - Run as root, OR
        - Add user to "input" group / udev rules, depending on distro.
   ⚠️ Needs -lpthread.

   Build:
     - Link with pthread
*/

#ifndef INPUT_HOOK_BACKEND_LINUX_EVDEV_H
#define INPUT_HOOK_BACKEND_LINUX_EVDEV_H

#ifdef __cplusplus
extern "C" {
#endif

#include "input_hook.h"

#ifdef __linux__

typedef struct ihk_linux_evdev_backend {
    int fd;
    int owns_fd;
    int running;
    int ok;

    /* reader thread */
    void *thread; /* pthread_t stored opaquely to avoid including pthread in header */
    void *mutex;  /* pthread_mutex_t stored opaquely */

    ihk_u8 kb_bits[IHK_KB_BITS_BYTES];

    char device_path[512]; /* best-effort; for debugging */
} ihk_linux_evdev_backend;

/* Initialize backend.
   - device_path can be:
       - NULL: auto-detect first keyboard (best-effort)
       - "/dev/input/eventX"
       - "/dev/input/by-id/..." (recommended)
   Returns 1 on success, 0 on failure.
*/
int  ihk_linux_evdev_backend_init(ihk_linux_evdev_backend *b, const char *device_path);

/* Optional: check if backend started OK (permissions, etc.) */
int  ihk_linux_evdev_backend_is_ok(const ihk_linux_evdev_backend *b);

/* Fill an ihk_backend wrapper you can pass into input_hook_init/set_backend */
void ihk_linux_evdev_make_backend(ihk_linux_evdev_backend *b, ihk_backend *out);

#endif /* __linux__ */

#ifdef __cplusplus
}
#endif

#endif /* INPUT_HOOK_BACKEND_LINUX_EVDEV_H */
