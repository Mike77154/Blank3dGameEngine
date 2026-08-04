/* input_hook_backend_win32_async.h

   Optional Win32 backend (no dependency on polls/input_scanner).
   Uses GetAsyncKeyState to poll keyboard state.

   ✅ Works great for "global-ish" key checks in many cases.
   ⚠️ Limitations:
     - Some keys cannot be differentiated perfectly (e.g. main Enter vs KP Enter)
     - IME / text input is out of scope (this is raw key state)

   Build:
     - Compile this file only on Windows
     - Link with user32
*/

#ifndef INPUT_HOOK_BACKEND_WIN32_ASYNC_H
#define INPUT_HOOK_BACKEND_WIN32_ASYNC_H

#ifdef _WIN32

#ifdef __cplusplus
extern "C" {
#endif

#include "input_hook.h"

typedef struct ihk_win32_async_backend {
    int dummy;
} ihk_win32_async_backend;

/* Create a backend instance.
   - You own the backend struct lifetime.
*/
void ihk_win32_async_backend_init(ihk_win32_async_backend *b);

/* Fill an ihk_backend wrapper you can pass into input_hook_init/set_backend */
void ihk_win32_async_make_backend(ihk_win32_async_backend *b, ihk_backend *out);

#ifdef __cplusplus
}
#endif

#endif /* _WIN32 */

#endif /* INPUT_HOOK_BACKEND_WIN32_ASYNC_H */
