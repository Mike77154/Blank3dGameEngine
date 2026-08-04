/* input_hook_backend_macos_eventtap.h

   Robust macOS backend for input_hook (USB HID-style).
   Uses a CGEventTap to track global key up/down and maintain a bitset.

   ✅ Can capture globally (session event tap) if allowed by the OS.
   ✅ Layout-independent-ish (maps mac virtual keycodes to HID usages).
   ⚠️ Requires accessibility / input monitoring permission on modern macOS.
   ⚠️ Needs -framework ApplicationServices and -lpthread.

   Build:
     clang ... input_hook_backend_macos_eventtap.c -framework ApplicationServices -lpthread
*/

#ifndef INPUT_HOOK_BACKEND_MACOS_EVENTTAP_H
#define INPUT_HOOK_BACKEND_MACOS_EVENTTAP_H

#ifdef __APPLE__

#ifdef __cplusplus
extern "C" {
#endif

#include "input_hook.h"

typedef struct ihk_macos_eventtap_backend {
    int ok;
    int running;

    void *thread; /* pthread_t */
    void *mutex;  /* pthread_mutex_t */

    void *tap;    /* CFMachPortRef */
    void *source; /* CFRunLoopSourceRef */
    void *runloop;/* CFRunLoopRef */

    ihk_u8 kb_bits[IHK_KB_BITS_BYTES];
} ihk_macos_eventtap_backend;

/* Starts the event tap thread. Returns 1 on success, 0 on failure. */
int  ihk_macos_eventtap_backend_init(ihk_macos_eventtap_backend *b);

/* Returns 1 if backend is running/ok */
int  ihk_macos_eventtap_backend_is_ok(const ihk_macos_eventtap_backend *b);

/* Fill an ihk_backend wrapper you can pass into input_hook_init/set_backend */
void ihk_macos_eventtap_make_backend(ihk_macos_eventtap_backend *b, ihk_backend *out);

#ifdef __cplusplus
}
#endif

#endif /* __APPLE__ */

#endif /* INPUT_HOOK_BACKEND_MACOS_EVENTTAP_H */
