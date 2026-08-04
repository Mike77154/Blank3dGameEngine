/* input_hook_backend_win32_llhook.h

   Robust Windows backend for input_hook (USB HID-style).
   Uses WH_KEYBOARD_LL (low-level keyboard hook) to track key up/down globally.

   ✅ Better differentiation than GetAsyncKeyState:
      - Can distinguish main Enter vs KP Enter (extended flag)
      - Can distinguish left/right Ctrl/Alt
   ✅ Global capture in most desktop scenarios.
   ⚠️ Requires a message loop thread (handled internally).
   ⚠️ Some environments may restrict global hooks.

   Build:
     - Compile on Windows
     - Link with user32
*/

#ifndef INPUT_HOOK_BACKEND_WIN32_LLHOOK_H
#define INPUT_HOOK_BACKEND_WIN32_LLHOOK_H

#ifdef _WIN32

#ifdef __cplusplus
extern "C" {
#endif

#include "input_hook.h"

typedef struct ihk_win32_llhook_backend {
    int ok;
    int running;

    void *thread; /* HANDLE */
    unsigned long thread_id;

    void *hook;   /* HHOOK */
    void *cs;     /* CRITICAL_SECTION */

    ihk_u8 kb_bits[IHK_KB_BITS_BYTES];
} ihk_win32_llhook_backend;

/* Starts the hook thread. Returns 1 on success, 0 on failure. */
int  ihk_win32_llhook_backend_init(ihk_win32_llhook_backend *b);

/* Returns 1 if backend is running/ok */
int  ihk_win32_llhook_backend_is_ok(const ihk_win32_llhook_backend *b);

/* Fill an ihk_backend wrapper you can pass into input_hook_init/set_backend */
void ihk_win32_llhook_make_backend(ihk_win32_llhook_backend *b, ihk_backend *out);

#ifdef __cplusplus
}
#endif

#endif /* _WIN32 */

#endif /* INPUT_HOOK_BACKEND_WIN32_LLHOOK_H */
