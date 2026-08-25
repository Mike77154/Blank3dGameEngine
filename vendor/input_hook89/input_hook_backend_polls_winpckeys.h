/* input_hook_backend_polls_winpckeys.h

   Convenience API definition for polls/by_system_backend/win32/winpckeys_backend
   so you can plug it into input_hook via input_hook_backend_polls_keypc.

   C89-compatible.
*/

#ifndef INPUT_HOOK_BACKEND_POLLS_WINPCKEYS_H
#define INPUT_HOOK_BACKEND_POLLS_WINPCKEYS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "input_hook_backend_polls_keypc.h"

/* This header only makes sense on Windows builds where winpckeys exists. */
#include "by_system_backend/win32/winpckeys_backend.h"

/* Returns a pointer to a static ihk_polls_kb_backend_api for winpckeys_backend. */
const ihk_polls_kb_backend_api *ihk_polls_api_winpckeys(void);

#ifdef __cplusplus
}
#endif

#endif /* INPUT_HOOK_BACKEND_POLLS_WINPCKEYS_H */
