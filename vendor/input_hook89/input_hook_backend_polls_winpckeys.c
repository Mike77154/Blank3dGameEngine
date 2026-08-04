/* input_hook_backend_polls_winpckeys.c
   C89-compatible.
*/

#include "input_hook_backend_polls_winpckeys.h"

/*
   winpckeys_backend uses GetAsyncKeyState with VK_* (layout-dependent-ish).
   It often works without focus, so we mark IHK_CAP_GLOBAL_CAPTURE.
*/

const ihk_polls_kb_backend_api *ihk_polls_api_winpckeys(void)
{
    /* static so caller can keep a pointer forever */
    static const ihk_polls_kb_backend_api api = {
        (size_t)sizeof(winpckeys_backend),
        (int)WINPCKEYS_MAX_BUTTONS,
        (ihk_polls_init_fn)winpckeys_backend_init,
        (ihk_polls_bind_fn)winpckeys_bind_button,
        (ihk_polls_update_fn)winpckeys_backend_update,
        (ihk_polls_hold_fn)winpckeys_button_hold,
        (ihk_u32)(IHK_CAP_GLOBAL_CAPTURE)
    };

    return &api;
}
